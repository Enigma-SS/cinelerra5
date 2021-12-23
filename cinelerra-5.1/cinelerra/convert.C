/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "assets.h"
#include "audiodevice.h"
#include "autos.h"
#include "bchash.h"
#include "bcpot.h"
#include "bctimer.h"
#include "cache.h"
#include "convert.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "formattools.h"
#include "indexfile.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "panautos.h"
#include "panauto.h"
#include "preferences.h"
#include "render.h"
#include "theme.h"
#include "tracks.h"


#define WIDTH xS(400)
#define HEIGHT yS(360)
#define MAX_SCALE 16

ConvertRender::ConvertRender(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	suffix = 0;
	format_asset = 0;
	progress = 0;
	progress_timer = new Timer;
	convert_progress = 0;
	counter_lock = new Mutex("ConvertDialog::counter_lock");
	total_rendered = 0;
	remove_originals = 0;
	failed = 0;  canceled = 0;
	result = 0;
	beep = 0;
	to_proxy = 0;
	renderer = 0;
}

ConvertRender::~ConvertRender()
{
	if( running() ) {
		canceled = 1;
		if( renderer )
			renderer->set_result(1);
		cancel();
		join();
	}
	delete renderer;
	delete [] suffix;
	delete progress;
	delete counter_lock;
	delete progress_timer;
	delete convert_progress;
	if( format_asset )
		format_asset->remove_user();
	reset();
}

void ConvertRender::reset()
{
	for( int i=0,n=orig_idxbls.size(); i<n; ++i )
		orig_idxbls[i]->remove_user();
	orig_idxbls.remove_all();
	for( int i=0,n=orig_copies.size(); i<n; ++i )
		orig_copies[i]->remove_user();
	orig_copies.remove_all();
	for( int i=0,n=needed_idxbls.size(); i<n; ++i )
		needed_idxbls[i]->remove_user();
	needed_idxbls.remove_all();
	for( int i=0,n=needed_copies.size(); i<n; ++i )
		needed_copies[i]->remove_user();
	needed_copies.remove_all();
}

void ConvertRender::to_convert_path(char *new_path, Indexable *idxbl)
{
	if( to_proxy ) {
		char *bp = idxbl->path, *cp = strrchr(bp, '/');
		if( cp ) bp = cp+1;
		char filename[BCTEXTLEN], proxy_path[BCTEXTLEN];
		strcpy(filename, bp);
		File::getenv_path(proxy_path, mwindow->preferences->nested_proxy_path);
		sprintf(new_path, "%s/%s", proxy_path, filename);
	}
	else
		strcpy(new_path, idxbl->path);
	int n = strlen(suffix);
	char *ep = new_path + strlen(new_path);
	char *sfx = strrchr(new_path, '.');
	if( sfx ) {
// insert suffix, path.sfx => path+suffix-sfx.ext
		char *bp = ep, *cp = (ep += n);
		while( --bp > sfx ) *--cp = *bp;
		*--cp = '-';
	}
	else {
// insert suffix, path => path+suffix.ext
		sfx = ep;  ep += n;
	}
	for( const char *cp=suffix; --n>=0; ++cp ) *sfx++ = *cp;
	*ep++ = '.';
	const char *ext = format_asset->format == FILE_FFMPEG ?
			format_asset->fformat :
			File::get_tag(format_asset->format);
	while( *ext ) *ep++ = *ext++;
	*ep = 0;
}

int ConvertRender::from_convert_path(char *path, Indexable *idxbl)
{
	strcpy(path, idxbl->path);
	char *ep = path + strlen(path);
	const char *ext = format_asset->format == FILE_FFMPEG ?
			format_asset->fformat :
			File::get_tag(format_asset->format);
	const char *rp = ext + strlen(ext);
	do {
		--rp;  --ep;
	} while( rp>=ext && ep>=path && *ep == *rp );
	if( rp >= ext || ep < path && *--ep != '.' ) return 1;   // didnt find .ext
	int n = strlen(suffix), len = 0;
	char *lp = ep - n;  // <suffix-chars>+.ext
	for( ; --lp>=path ; ++len ) {
		if( strncmp(lp, suffix, n) ) continue;
		if( !len || lp[n] == '-' ) break;
	}
	if( lp < path ) return 1;  // didnt find suffix
	if( *(rp=lp+n) == '-' ) {
// remove suffix, path+suffix-sfx.ext >= path.sfx
		*lp++ = '.';  ++rp;
		while( rp < ep ) *lp++ = *rp++;
	}
// remove suffix, path+suffix.ext => path
	*lp = 0;
	return 0;
}

double ConvertRender::get_video_length(Indexable *idxbl)
{
	int64_t video_frames = idxbl->get_video_frames();
	double frame_rate = idxbl->get_frame_rate();
	if( video_frames < 0 && mwindow->edl->session->si_useduration )
		video_frames = mwindow->edl->session->si_duration * frame_rate;
	if( video_frames < 0 ) video_frames = 1;
	return !video_frames ? 0 : video_frames / frame_rate;
}

double ConvertRender::get_audio_length(Indexable *idxbl)
{
	int64_t audio_samples = idxbl->get_audio_samples();
	return !audio_samples ? 0 :
		(double)audio_samples / idxbl->get_sample_rate();
}

double ConvertRender::get_length(Indexable *idxbl)
{
	return bmax(get_video_length(idxbl), get_audio_length(idxbl));
}

int ConvertRender::match_format(Asset *asset)
{
// close enough
	return format_asset->audio_data == asset->audio_data &&
		format_asset->video_data == asset->video_data ? 1 : 0;
}

EDL *ConvertRender::convert_edl(EDL *edl, Indexable *idxbl)
{
	Asset *copy_asset = edl->assets->get_asset(idxbl->path);
	if( !copy_asset ) return 0;
	if( !copy_asset->layers && !copy_asset->channels ) return 0;
// make a clip from 1st video track and audio tracks
	EDL *copy_edl = new EDL(edl);
	copy_edl->create_objects();
	copy_edl->set_path(copy_asset->path);
	char path[BCTEXTLEN];
	FileSystem fs;  fs.extract_name(path, copy_asset->path);
	strcpy(copy_edl->local_session->clip_title, path);
	strcpy(copy_edl->local_session->clip_notes, _("Transcode clip"));

	double video_length = get_video_length(idxbl);
	double audio_length = get_audio_length(idxbl);
	copy_edl->session->video_tracks =
		 video_length > 0 ? 1 : 0;
	copy_edl->session->audio_tracks =
		 audio_length > 0 ? copy_asset->channels : 0;

	copy_edl->create_default_tracks();
	Track *current = copy_edl->session->video_tracks ?
		copy_edl->tracks->first : 0;
	for( int vtrack=0; current; current=NEXT ) {
		if( current->data_type != TRACK_VIDEO ) continue;
		current->insert_asset(copy_asset, 0, video_length, 0, vtrack);
		break;
	}
	current = copy_edl->session->audio_tracks ?
		copy_edl->tracks->first : 0;
	for( int atrack=0; current; current=NEXT ) {
		if( current->data_type != TRACK_AUDIO ) continue;
		current->insert_asset(copy_asset, 0, audio_length, 0, atrack);
		Autos *pan_autos = current->automation->autos[AUTOMATION_PAN];
		PanAuto *pan_auto = (PanAuto*)pan_autos->get_auto_for_editing(-1);
		for( int i=0; i < MAXCHANNELS; ++i ) pan_auto->values[i] = 0;
		pan_auto->values[atrack++] = 1;
		if( atrack >= MAXCHANNELS ) atrack = 0;
	}
	copy_edl->folder_no = AW_MEDIA_FOLDER;
	return copy_edl;
}

int ConvertRender::add_original(EDL *edl, Indexable *idxbl)
{
	char new_path[BCTEXTLEN];
// if idxbl already a convert
	if( !from_convert_path(new_path, idxbl) ) return 0;
// don't convert if not readable
	if( idxbl->is_asset && access(idxbl->path, R_OK) ) return 0;
// found readable unconverted asset
	to_convert_path(new_path, idxbl);
// add to orig_idxbls & orig_copies if it isn't already there.
	int got_it = 0;
	for( int i = 0; !got_it && i<orig_copies.size(); ++i )
		got_it = !strcmp(orig_copies[i]->path, new_path);
	if( got_it ) return 0;
	idxbl->add_user();
	orig_idxbls.append(idxbl);
	int needed = 1;
	Asset *convert = edl->assets->get_asset(new_path);
	if( !convert ) {
		convert = new Asset(new_path);
		FileSystem fs;
		if( fs.get_size(new_path) > 0 ) {
// copy already on disk
			int64_t orig_mtime = fs.get_date(idxbl->path);
			int64_t convert_mtime = fs.get_date(new_path);
// render needed if it is too old
			if( orig_mtime < convert_mtime ) {
				File file;
				int ret = file.open_file(mwindow->preferences, convert, 1, 0);
// render not needed if can use copy
				if( ret == FILE_OK ) {
					if( match_format(file.asset) ) {
						mwindow->mainindexes->add_indexable(convert);
						mwindow->mainindexes->start_build();
						needed = 0;
					}
					else
						needed = -1;
				}
			}
		}
	}
	else if( match_format(convert) ) {
// dont render if copy already an assets
		convert->add_user();
		needed = 0;
	}
	else
		needed = -1;
	if( needed < 0 ) {
		eprintf(_("transcode target file exists but is incorrect format:\n%s\n"
			  "remove file from disk before transcode to new format.\n"), new_path);
		return -1;
	}
	orig_copies.append(convert);
	if( needed ) {
		convert->copy_format(format_asset, 0);
// new compression parameters
		convert->video_data = format_asset->video_data;
		if( convert->video_data ) {
			convert->layers = 1;
			convert->width = idxbl->get_w();
			if( convert->width & 1 ) ++convert->width;
			convert->actual_width = convert->width;
			convert->height = idxbl->get_h();
			if( convert->height & 1 ) ++convert->height;
			convert->actual_height = convert->height;
			convert->frame_rate = mwindow->edl->session->frame_rate;
		}
		convert->audio_data = format_asset->audio_data;
		if( convert->audio_data ) {
			convert->sample_rate = mwindow->edl->session->sample_rate;
		}
		convert->folder_no = AW_MEDIA_FOLDER;
		add_needed(idxbl, convert);
	}
	return 1;
}

void ConvertRender::add_needed(Indexable *idxbl, Asset *convert)
{
	needed_idxbls.append(idxbl);
	idxbl->add_user();
	needed_copies.append(convert);
	convert->add_user();
}


int ConvertRender::is_canceled()
{
	return progress->is_cancelled();
}

int ConvertRender::find_convertable_assets(EDL *edl)
{
	reset();
	Asset *orig_asset = edl->assets->first;
	int count = 0;
	for( ; orig_asset; orig_asset=orig_asset->next ) {
		int ret = add_original(edl, orig_asset);
		if( ret < 0 ) return -1;
		if( ret ) ++count;
	}
	return count;
}

void ConvertRender::set_format(Asset *asset, const char *suffix, int to_proxy)
{
	delete [] this->suffix;
	this->suffix = cstrdup(suffix);
	if( !format_asset )
		format_asset = new Asset();
	format_asset->copy_from(asset, 0);
	this->to_proxy = to_proxy;
}

void ConvertRender::start_convert(float beep, int remove_originals)
{
	this->beep = beep;
	this->remove_originals = remove_originals;
	start();
}

void ConvertRender::run()
{
	mwindow->stop_brender();
	result = 0;
	total_rendered = 0;
	failed = 0;  canceled = 0;

	progress_timer->update();
	start_progress();

	for( int i=0; !failed && !canceled && i<needed_copies.size(); ++i )
		create_copy(i);

	canceled = progress->is_cancelled();
printf(_("convert: failed=%d canceled=%d\n"), failed, canceled);
	double elapsed_time = progress_timer->get_scaled_difference(1);

	char elapsed[BCSTRLEN], text[BCSTRLEN];
	Units::totext(elapsed, elapsed_time, TIME_HMS2);
	printf(_("TranscodeRender::run: done in %s\n"), elapsed);
	if( canceled )
		strcpy(text, _("transcode cancelled"));
	else if( failed )
		strcpy(text, _("transcode failed"));
	else
		sprintf(text, _("transcode %d files, render time %s"),
			needed_copies.size(), elapsed);
// stop progress bar
	stop_progress(text);

	if( !failed ) {
		mwindow->finish_convert(remove_originals);
	}
	else if( !canceled ) {
		eprintf(_("Error making transcode."));
	}

	if( !canceled && beep > 0 ) {
		if( failed ) {
			mwindow->beep(4000., 0.5, beep);
			usleep(250000);
			mwindow->beep(1000., 0.5, beep);
			usleep(250000);
			mwindow->beep(4000., 0.5, beep);
		}
		else
			mwindow->beep(2000., 2.0, beep);
	}
	mwindow->restart_brender();
}

void ConvertRender::start_progress()
{
	double total_len= 0;
	for( int i = 0; i < needed_idxbls.size(); i++ ) {
		Indexable *orig_idxbl = needed_idxbls[i];
		double length = get_length(orig_idxbl);
		total_len += length;
	}
	int64_t total_samples = total_len * format_asset->sample_rate;
	mwindow->gui->lock_window("Render::start_progress");
	progress = mwindow->mainprogress->
		start_progress(_("Transcode files..."), total_samples);
	mwindow->gui->unlock_window();
	convert_progress = new ConvertProgress(mwindow, this);
	convert_progress->start();
}

void ConvertRender::stop_progress(const char *msg)
{
	delete convert_progress;  convert_progress = 0;
	mwindow->gui->lock_window("ConvertRender::stop_progress");
	progress->update(0);
	mwindow->mainprogress->end_progress(progress);
	progress = 0;
	mwindow->gui->show_message(msg);
	mwindow->gui->update_default_message();
	mwindow->gui->unlock_window();
}


ConvertPackageRenderer::ConvertPackageRenderer(ConvertRender *render)
 : PackageRenderer()
{
	this->render = render;
}

ConvertPackageRenderer::~ConvertPackageRenderer()
{
}

int ConvertPackageRenderer::get_master()
{
	return 1;
}

int ConvertPackageRenderer::get_result()
{
	return render->result;
}

void ConvertPackageRenderer::set_result(int value)
{
	if( value )
		render->result = value;
}

void ConvertPackageRenderer::set_progress(int64_t value)
{
	render->counter_lock->lock("ConvertPackageRenderer::set_progress");
// Increase total rendered for all nodes
	render->total_rendered += value;
	render->counter_lock->unlock();
}

int ConvertPackageRenderer::progress_cancelled()
{
	return render->progress && render->progress->is_cancelled();
}

ConvertProgress::ConvertProgress(MWindow *mwindow, ConvertRender *render)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->render = render;
	last_value = 0;
}

ConvertProgress::~ConvertProgress()
{
	cancel();
	join();
}

void ConvertProgress::run()
{
	Thread::disable_cancel();
	for( ;; ) {
		if( render->total_rendered != last_value ) {
			render->progress->update(render->total_rendered);
			last_value = render->total_rendered;
		}

		Thread::enable_cancel();
		sleep(1);
		Thread::disable_cancel();
	}
}

void ConvertRender::create_copy(int i)
{
	Indexable *orig_idxbl = needed_idxbls[i];
	Asset *needed_copy = needed_copies[i];
	EDL *edl = convert_edl(mwindow->edl, orig_idxbl);
	double length = get_length(orig_idxbl);
	renderer = new ConvertPackageRenderer(this);
	renderer->initialize(mwindow, edl, mwindow->preferences, needed_copy);
	PackageDispatcher dispatcher;
	dispatcher.create_packages(mwindow, edl, mwindow->preferences,
		SINGLE_PASS, needed_copy, 0, length, 0);
	RenderPackage *package = dispatcher.get_package(0);
	if( !renderer->render_package(package) ) {
		Asset *asset = mwindow->edl->assets->update(needed_copy);
		mwindow->mainindexes->add_indexable(asset);
		mwindow->mainindexes->start_build();
	}
	else
		failed = 1;
	delete renderer;  renderer = 0;
	edl->remove_user();
}

ConvertWindow::ConvertWindow(MWindow *mwindow, ConvertDialog *dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Transcode settings"), x, y, WIDTH, HEIGHT,
		-1, -1, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->dialog = dialog;
	format_tools = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transcode");
}

ConvertWindow::~ConvertWindow()
{
	lock_window("ConvertWindow::~ConvertWindow");
	delete format_tools;
	unlock_window();
}


void ConvertWindow::create_objects()
{
	lock_window("ConvertWindow::create_objects");
	int margin = mwindow->theme->widget_border;
	int lmargin = margin + xS(10);

	int x = lmargin;
	int y = margin + yS(10);

	BC_Title *text;
	add_subwindow(text = new BC_Title(x, y,
		_("Render untagged assets and replace in project")));
	y += text->get_h() + margin + yS(10);
	int y1 = y;
	y += BC_Title::calculate_h(this, _("Tag suffix:")) + margin + yS(10);

	x = lmargin;
	format_tools = new ConvertFormatTools(mwindow, this, dialog->asset);
	format_tools->create_objects(x, y, 1, 1, 1, 1, 0, 1, 0, 1, // skip the path
		0, 0);

	x = lmargin;
	add_subwindow(text = new BC_Title(x, y1, _("Tag suffix:")));
	x = format_tools->format_text->get_x();
	add_subwindow(suffix_text = new ConvertSuffixText(this, dialog, x, y1));
	x = lmargin;
	y += margin + yS(10);

	add_subwindow(remove_originals = new ConvertRemoveOriginals(this, x, y));
	x = lmargin;
	y += remove_originals->get_h() + margin;
	add_subwindow(to_proxy_path = new ConvertToProxyPath(this, x, y));
	y += to_proxy_path->get_h() + margin;

	add_subwindow(beep_on_done = new ConvertBeepOnDone(this, x, y));
	x += beep_on_done->get_w() + margin + xS(10);
	add_subwindow(new BC_Title(x, y+yS(10), _("Beep on done volume")));
//	y += beep_on_done->get_h() + margin;

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}

ConvertSuffixText::ConvertSuffixText(ConvertWindow *gui,
		ConvertDialog *dialog, int x, int y)
 : BC_TextBox(x, y, 160, 1, dialog->suffix)
{
	this->gui = gui;
	this->dialog = dialog;
}

ConvertSuffixText::~ConvertSuffixText()
{
}

int ConvertSuffixText::handle_event()
{
	strcpy(dialog->suffix, get_text());
	return 1;
}

ConvertFormatTools::ConvertFormatTools(MWindow *mwindow, ConvertWindow *gui, Asset *asset)
 : FormatTools(mwindow, gui, asset)
{
	this->gui = gui;
}

void ConvertFormatTools::update_format()
{
	asset->save_defaults(mwindow->defaults, "CONVERT_", 1, 1, 0, 0, 0);
	FormatTools::update_format();
}


ConvertMenuItem::ConvertMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Transcode..."),  _("Alt-e"), 'e')
{
	this->mwindow = mwindow;
	set_alt();
	dialog = 0;
}
ConvertMenuItem::~ConvertMenuItem()
{
	delete dialog;
}

void ConvertMenuItem::create_objects()
{
	dialog = new ConvertDialog(mwindow);
}

int ConvertMenuItem::handle_event()
{
	mwindow->gui->unlock_window();
	dialog->start();
	mwindow->gui->lock_window("ConvertMenuItem::handle_event");
	return 1;
}


ConvertDialog::ConvertDialog(MWindow *mwindow)
{
	this->mwindow = mwindow;
	gui = 0;
	asset = new Asset;
	strcpy(suffix, ".transcode");
// quicker than some, not as good as others
	asset->format = FILE_FFMPEG;
	strcpy(asset->fformat, "mp4");
	strcpy(asset->vcodec, "h264.mp4");
	asset->ff_video_bitrate = 2560000;
	asset->video_data = 1;
	strcpy(asset->acodec, "h264.mp4");
	asset->ff_audio_bitrate = 256000;
	asset->audio_data = 1;
	remove_originals = 1;
	beep = 0;
	to_proxy = 0;
}

ConvertDialog::~ConvertDialog()
{
	close_window();
	asset->remove_user();
}

BC_Window* ConvertDialog::new_gui()
{
	asset->format = FILE_FFMPEG;
	asset->frame_rate = mwindow->edl->session->frame_rate;
	asset->sample_rate = mwindow->edl->session->sample_rate;
	asset->load_defaults(mwindow->defaults, "CONVERT_", 1, 1, 0, 1, 0);
	remove_originals = mwindow->defaults->get("CONVERT_REMOVE_ORIGINALS", remove_originals);
	beep = mwindow->defaults->get("CONVERT_BEEP", beep);
	to_proxy = mwindow->defaults->get("CONVERT_TO_PROXY", to_proxy);
	mwindow->defaults->get("CONVERT_SUFFIX", suffix);
	mwindow->gui->lock_window("ConvertDialog::new_gui");
	int cx, cy;
	mwindow->gui->get_abs_cursor(cx, cy);
	gui = new ConvertWindow(mwindow, this, cx - WIDTH/2, cy - HEIGHT/2);
	gui->create_objects();
	mwindow->gui->unlock_window();
	return gui;
}

void ConvertDialog::handle_close_event(int result)
{
	if( result ) return;
	mwindow->defaults->update("CONVERT_SUFFIX", suffix);
	mwindow->defaults->update("CONVERT_REMOVE_ORIGINALS", remove_originals);
	mwindow->defaults->update("CONVERT_BEEP", beep);
	mwindow->defaults->update("CONVERT_TO_PROXY", to_proxy);
	asset->save_defaults(mwindow->defaults, "CONVERT_", 1, 1, 0, 1, 0);
	mwindow->start_convert(asset, suffix, beep, to_proxy, remove_originals);
}


ConvertRemoveOriginals::ConvertRemoveOriginals(ConvertWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->dialog->remove_originals, _("Remove originals from project"))
{
	this->gui = gui;
}

ConvertRemoveOriginals::~ConvertRemoveOriginals()
{
}

int ConvertRemoveOriginals::handle_event()
{
	gui->dialog->remove_originals = get_value();
	return 1;
}

ConvertToProxyPath::ConvertToProxyPath(ConvertWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->dialog->to_proxy, _("Into Nested Proxy directory"))
{
	this->gui = gui;
}

ConvertToProxyPath::~ConvertToProxyPath()
{
}

int ConvertToProxyPath::handle_event()
{
	gui->dialog->to_proxy = get_value();
	return 1;
}

ConvertBeepOnDone::ConvertBeepOnDone(ConvertWindow *gui, int x, int y)
 : BC_FPot(x, y, gui->dialog->beep*100.f, 0.f, 100.f)
{
	this->gui = gui;
}

int ConvertBeepOnDone::handle_event()
{
	gui->dialog->beep = get_value()/100.f;
	return 1;
}

