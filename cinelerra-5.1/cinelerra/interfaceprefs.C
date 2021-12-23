
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "deleteallindexes.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "probeprefs.h"
#include "interfaceprefs.h"
#include "shbtnprefs.h"
#include "theme.h"

#define MOVE_RIPPLE_TITLE N_("All Edits (ripple)")
#define MOVE_ROLL_TITLE   N_("One Edit  (roll)")
#define MOVE_SLIP_TITLE   N_("Src Only  (slip)")
#define MOVE_SLIDE_TITLE  N_("Move Edit (slide)")
#define MOVE_EDGE_TITLE   N_("Drag Edge (edge)")
#define MOVE_EDITS_DISABLED_TITLE N_("No effect")


InterfacePrefs::InterfacePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	min_db = 0;
	max_db = 0;
	shbtn_dialog = 0;
	file_probe_dialog = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Interface");
}

InterfacePrefs::~InterfacePrefs()
{
	delete min_db;
	delete max_db;
	delete shbtn_dialog;
	delete file_probe_dialog;
}


void InterfacePrefs::create_objects()
{
	int xs4 = xS(4), xs5 = xS(5), xs10 = xS(10), xs30 = xS(30);
	int ys5 = yS(5), ys10 = yS(10), ys30 = yS(30), ys35 = yS(35);
	BC_Resources *resources = BC_WindowBase::get_resources();
	int margin = mwindow->theme->widget_border;
	char string[BCTEXTLEN];
	int x0 = mwindow->theme->preferencesoptions_x;
	int y0 = mwindow->theme->preferencesoptions_y;
	int x = x0, y = y0;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Editing:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Editing section");
	y += ys35;

	int x2 = get_w()/2, y2 = y;
	x = x2;
	add_subwindow(title = new BC_Title(x, y, _("Keyframe reticle:")));
	title->context_help_set_keyword("Using Autos");
	y += title->get_h() + ys5;
	keyframe_reticle = new KeyframeReticle(pwindow, this, x, y,
		&pwindow->thread->preferences->keyframe_reticle);
	add_subwindow(keyframe_reticle);
	keyframe_reticle->create_objects();
	keyframe_reticle->context_help_set_keyword("Using Autos");

	y += ys30;
	add_subwindow(title = new BC_Title(x, y, _("Snapshot path:")));
	title->context_help_set_keyword("Snapshot \\/ Grabshot");
	y += title->get_h() + ys5;
	add_subwindow(snapshot_path = new SnapshotPathText(pwindow, this, x, y, get_w()-x-xs30));
	snapshot_path->context_help_set_keyword("Snapshot \\/ Grabshot");

	x = x0;  y = y2;
	add_subwindow(title = new BC_Title(x, y, _("Clicking on edit boundaries does what:")));
	title->context_help_set_keyword("Using the Drag Handle with Trim");
	y += title->get_h() + ys10;
	add_subwindow(title = new BC_Title(x, y, _("Button 1:")));
	title->context_help_set_keyword("Using the Drag Handle with Trim");
	int x1 = x + xS(100);

	ViewBehaviourText *text;
	add_subwindow(text = new ViewBehaviourText(x1, y - ys5,
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[0]),
			pwindow,
			&(pwindow->thread->edl->session->edit_handle_mode[0])));
	text->create_objects();
	text->context_help_set_keyword("Using the Drag Handle with Trim");
	y += ys30;
	add_subwindow(title = new BC_Title(x, y, _("Button 2:")));
	title->context_help_set_keyword("Using the Drag Handle with Trim");
	add_subwindow(text = new ViewBehaviourText(x1,
		y - ys5,
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[1]),
			pwindow,
			&(pwindow->thread->edl->session->edit_handle_mode[1])));
	text->create_objects();
	text->context_help_set_keyword("Using the Drag Handle with Trim");
	y += ys30;
	add_subwindow(title = new BC_Title(x, y, _("Button 3:")));
	title->context_help_set_keyword("Using the Drag Handle with Trim");
	add_subwindow(text = new ViewBehaviourText(x1, y - ys5,
		behavior_to_text(pwindow->thread->edl->session->edit_handle_mode[2]),
			pwindow,
			&(pwindow->thread->edl->session->edit_handle_mode[2])));
	text->create_objects();
	text->context_help_set_keyword("Using the Drag Handle with Trim");
	y += text->get_h() + ys30;

	x = x0;
	add_subwindow(new BC_Bar(xs5, y, get_w() - xs10));
	y += ys5;
	add_subwindow(title = new BC_Title(x, y, _("Operation:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Operation section");

	int y1 = y;
	y += yS(15);

	AndroidRemote *android_remote = new AndroidRemote(pwindow, x2, y);
	add_subwindow(android_remote);
	android_remote->context_help_set_keyword("Android Remote Control");
	y += android_remote->get_h() + ys10;
	int x3 = x2;
	add_subwindow(title = new BC_Title(x3, y, _("Port:")));
	title->context_help_set_keyword("Android Remote Control");
	x3 += title->get_w() + margin;
	AndroidPort *android_port = new AndroidPort(pwindow, x3, y);
	add_subwindow(android_port);
	android_port->context_help_set_keyword("Android Remote Control");
	x3 += android_port->get_w() + 2*margin;
	add_subwindow(title = new BC_Title(x3, y, _("PIN:")));
	title->context_help_set_keyword("Android Remote Control");
	x3 += title->get_w() + margin;
	AndroidPIN *android_pin = new AndroidPIN(pwindow, x3, y);
	add_subwindow(android_pin);
	android_pin->context_help_set_keyword("Android Remote Control");
	y += android_port->get_h() + 3*margin;

	ShBtnPrefs *shbtn_prefs = new ShBtnPrefs(pwindow, this, x2, y);
	add_subwindow(shbtn_prefs);
	shbtn_prefs->context_help_set_keyword("Menu Bar Shell Commands");
	x3 = x2 + shbtn_prefs->get_w() + 2*margin;
	add_subwindow(reload_plugins = new PrefsReloadPlugins(pwindow, this, x3, y));
	reload_plugins->context_help_set_keyword("Operation section");
	y += reload_plugins->get_h() + 3*margin;

	add_subwindow(title = new BC_Title(x2, y, _("Nested Proxy Path:")));
	title->context_help_set_keyword("Proxy");
	y += title->get_h() + ys10;
	PrefsNestedProxyPath *nested_proxy_path = new PrefsNestedProxyPath(pwindow, this,
			x2, y, get_w()-x2-xs30);
	add_subwindow(nested_proxy_path);
	nested_proxy_path->context_help_set_keyword("Proxy");
	y += xs30;

	add_subwindow(title = new BC_Title(x2, y, _("Default LV2_PATH:")));
	title->context_help_set_keyword("Audio LV2 \\/ Calf Plugins");
	y += title->get_h() + ys10;
	PrefsLV2PathText *lv2_path_text = new PrefsLV2PathText(pwindow, this,
			x2, y, get_w()-x2-xs30);
	add_subwindow(lv2_path_text);
	lv2_path_text->context_help_set_keyword("Audio LV2 \\/ Calf Plugins");
	y += xs30;

	y2 = y;
	x = x0;  y = y1 + ys35;
	add_subwindow(file_probes = new PrefsFileProbes(pwindow, this, x, y));
	file_probes->context_help_set_keyword("Probe Order when Loading Media");
	y += ys30;

	PrefsTrapSigSEGV *trap_segv = new PrefsTrapSigSEGV(this, x, y);
	add_subwindow(trap_segv);
	trap_segv->context_help_set_keyword("Operation section");
	x1 = x + trap_segv->get_w() + xs10;
	add_subwindow(title = new BC_Title(x1, y, _("(must be root)"), MEDIUMFONT, RED));
	title->context_help_set_keyword("Operation section");
	y += ys30;

	PrefsTrapSigINTR *trap_intr = new PrefsTrapSigINTR(this, x, y);
	add_subwindow(trap_intr);
	trap_intr->context_help_set_keyword("Operation section");
	add_subwindow(title = new BC_Title(x1, y, _("(must be root)"), MEDIUMFONT, RED));
	title->context_help_set_keyword("Operation section");
	y += ys30;

	yuv420p_dvdlace = new PrefsYUV420P_DVDlace(pwindow, this, x, y);
	add_subwindow(yuv420p_dvdlace);
	yuv420p_dvdlace->context_help_set_keyword("Dvd Interlaced Chroma");
	y += ys30;

	add_subwindow(title = new BC_Title(x1=x, y + ys5, _("Min DB for meter:")));
	title->context_help_set_keyword("Sound Level Meters Window");
	x1 += title->get_w() + xs4;
	sprintf(string, "%d", pwindow->thread->edl->session->min_meter_db);
	add_subwindow(min_db = new MeterMinDB(pwindow, string, x1, y));
	min_db->context_help_set_keyword("Sound Level Meters Window");
	x1 += min_db->get_w() + xs4;
	add_subwindow(title = new BC_Title(x1, y + ys5, _("Max:")));
	title->context_help_set_keyword("Sound Level Meters Window");
	x1 += title->get_w() + xs4;
	sprintf(string, "%d", pwindow->thread->edl->session->max_meter_db);
	add_subwindow(max_db = new MeterMaxDB(pwindow, string, x1, y));
	max_db->context_help_set_keyword("Sound Level Meters Window");
	y += ys30;

	StillImageUseDuration *use_stduration = new StillImageUseDuration(pwindow,
		pwindow->thread->edl->session->si_useduration, x, y);
	add_subwindow(use_stduration);
	use_stduration->context_help_set_keyword("Working with Still Images");
	x1 = x + use_stduration->get_w() + xs10;
	StillImageDuration *stduration = new StillImageDuration(pwindow, x1, y);
	add_subwindow(stduration);
	stduration->context_help_set_keyword("Working with Still Images");
	x1 += stduration->get_w() + xs10;
	add_subwindow(title = new BC_Title(x1, y, _("Seconds")));
	title->context_help_set_keyword("Working with Still Images");
	y += ys30;

	PrefsAutostartLV2UI *autostart_lv2ui = new PrefsAutostartLV2UI(x, y,pwindow);
	add_subwindow(autostart_lv2ui);
	autostart_lv2ui->context_help_set_keyword("Audio LV2 \\/ Calf Plugins");
	y += autostart_lv2ui->get_h() + ys10;

	if( y2 > y ) y = y2;
	x = x0;
	add_subwindow(new BC_Bar(xs5, y, get_w() - xs10));
	y += ys5;

	add_subwindow(title = new BC_Title(x, y, _("Index files:"), LARGEFONT, resources->text_default));
	title->context_help_set_keyword("Index Files section");
	y += ys30;

	add_subwindow(title = new BC_Title(x, y + ys5,
		_("Index files go here:"), MEDIUMFONT, resources->text_default));
	title->context_help_set_keyword("Index Files section");
	x1 = x + xS(230);
	add_subwindow(ipathtext = new IndexPathText(x1, y, pwindow,
		pwindow->thread->preferences->index_directory));
	ipathtext->context_help_set_keyword("Index Files section");
	x1 +=  ipathtext->get_w();
	add_subwindow(ipath = new BrowseButton(mwindow->theme, this, ipathtext, x1, y,
		pwindow->thread->preferences->index_directory,
		_("Index Path"), _("Select the directory for index files"), 1));
	ipath->context_help_set_keyword("Index Files section");

	y += ys30;
	add_subwindow(title = new BC_Title(x, y + ys5, _("Size of index file in KB:"),
		MEDIUMFONT, resources->text_default));
	title->context_help_set_keyword("Index Files section");
	sprintf(string, "%jd", pwindow->thread->preferences->index_size/1024);
	add_subwindow(isize = new IndexSize(x + xS(230), y, pwindow, string));
	isize->context_help_set_keyword("Index Files section");
	add_subwindow(new ScanCommercials(pwindow, xS(400),y));

	y += ys30;
	add_subwindow(title = new BC_Title(x, y + ys5, _("Number of index files to keep:"),
		MEDIUMFONT, resources->text_default));
	title->context_help_set_keyword("Index Files section");
	sprintf(string, "%ld", (long)pwindow->thread->preferences->index_count);
	add_subwindow(icount = new IndexCount(x + xS(230), y, pwindow, string));
	icount->context_help_set_keyword("Index Files section");
	add_subwindow(del_indexes = new DeleteAllIndexes(mwindow, pwindow, xS(400), y,
		_("Delete existing indexes"), "[*.idx][*.toc][*.mkr]"));
	del_indexes->context_help_set_keyword("Index Files section");
	y += ys30;
	add_subwindow(ffmpeg_marker_files = new IndexFFMPEGMarkerFiles(this, x, y));
	ffmpeg_marker_files->context_help_set_keyword("Index Files section");
	add_subwindow(del_clipngs = new DeleteAllIndexes(mwindow, pwindow, xS(400), y,
		_("Delete clip thumbnails"), "clip_*.png"));
	del_clipngs->context_help_set_keyword("Index Files section");
}

const char* InterfacePrefs::behavior_to_text(int mode)
{
	switch( mode ) {
	case MOVE_RIPPLE: return _(MOVE_RIPPLE_TITLE);
	case MOVE_ROLL: return _(MOVE_ROLL_TITLE);
	case MOVE_SLIP: return _(MOVE_SLIP_TITLE);
	case MOVE_SLIDE: return _(MOVE_SLIDE_TITLE);
	case MOVE_EDGE: return _(MOVE_EDGE_TITLE);
	case MOVE_EDITS_DISABLED: return _(MOVE_EDITS_DISABLED_TITLE);
	}
	return "";
}

IndexPathText::IndexPathText(int x, int y, PreferencesWindow *pwindow, char *text)
 : BC_TextBox(x, y, xS(240), 1, text)
{
	this->pwindow = pwindow;
}

IndexPathText::~IndexPathText() {}

int IndexPathText::handle_event()
{
	strcpy(pwindow->thread->preferences->index_directory, get_text());
	return 1;
}




IndexSize::IndexSize(int x, int y, PreferencesWindow *pwindow, char *text)
 : BC_TextBox(x, y, xS(100), 1, text)
{
	this->pwindow = pwindow;
}

int IndexSize::handle_event()
{
	long result;

	result = atol(get_text()) * 1024;
	if( result < 65536 ) result = 65536;
	//if(result < 500000) result = 500000;
	pwindow->thread->preferences->index_size = result;
	return 0;
}



IndexCount::IndexCount(int x, int y, PreferencesWindow *pwindow, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->pwindow = pwindow;
}

int IndexCount::handle_event()
{
	long result;

	result = atol(get_text());
	if(result < 1) result = 1;
	pwindow->thread->preferences->index_count = result;
	return 0;
}



IndexFFMPEGMarkerFiles::IndexFFMPEGMarkerFiles(InterfacePrefs *iface_prefs, int x, int y)
 : BC_CheckBox(x, y,
	iface_prefs->pwindow->thread->preferences->ffmpeg_marker_indexes,
	_("build ffmpeg marker indexes"))
{
	this->iface_prefs = iface_prefs;
}
IndexFFMPEGMarkerFiles::~IndexFFMPEGMarkerFiles()
{
}

int IndexFFMPEGMarkerFiles::handle_event()
{
	iface_prefs->pwindow->thread->preferences->ffmpeg_marker_indexes = get_value();
	return 1;
}


ViewBehaviourText::ViewBehaviourText(int x, int y, const char *text, PreferencesWindow *pwindow,
	int *output)
 : BC_PopupMenu(x, y, xS(250), text)
{
	this->output = output;
}

ViewBehaviourText::~ViewBehaviourText()
{
}

int ViewBehaviourText::handle_event()
{
	return 0;
}

void ViewBehaviourText::create_objects()
{
	for( int mode=0; mode<=EDIT_HANDLE_MODES; ++mode )
		add_item(new ViewBehaviourItem(this,
			InterfacePrefs::behavior_to_text(mode), mode));
}


ViewBehaviourItem::ViewBehaviourItem(ViewBehaviourText *popup,
		const char *text, int behaviour)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->behaviour = behaviour;
}

ViewBehaviourItem::~ViewBehaviourItem()
{
}

int ViewBehaviourItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = behaviour;
	return 1;
}


MeterMinDB::MeterMinDB(PreferencesWindow *pwindow, char *text, int x, int y)
 : BC_TextBox(x, y, xS(50), 1, text)
{
	this->pwindow = pwindow;
}

int MeterMinDB::handle_event()
{
	pwindow->thread->redraw_meters = 1;
	pwindow->thread->edl->session->min_meter_db = atol(get_text());
	return 0;
}


MeterMaxDB::MeterMaxDB(PreferencesWindow *pwindow, char *text, int x, int y)
 : BC_TextBox(x, y, xS(50), 1, text)
{
	this->pwindow = pwindow;
}

int MeterMaxDB::handle_event()
{
	pwindow->thread->redraw_meters = 1;
	pwindow->thread->edl->session->max_meter_db = atol(get_text());
	return 0;
}


ScanCommercials::ScanCommercials(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->preferences->scan_commercials,
	_("Scan for commercials during toc build"))
{
	this->pwindow = pwindow;
// *** CONTEXT_HELP ***
	context_help_set_keyword("The commercial DB");
}
int ScanCommercials::handle_event()
{
	pwindow->thread->preferences->scan_commercials = get_value();
	return 1;
}


AndroidRemote::AndroidRemote(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->android_remote,
	_("Android Remote Control"))
{
	this->pwindow = pwindow;
}
int AndroidRemote::handle_event()
{
	pwindow->thread->preferences->android_remote = get_value();
	return 1;
}

AndroidPIN::AndroidPIN(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, xS(180), 1, pwindow->thread->preferences->android_pin)
{
	this->pwindow = pwindow;
}

int AndroidPIN::handle_event()
{
	char *txt = pwindow->thread->preferences->android_pin;
	int len = sizeof(pwindow->thread->preferences->android_pin);
	strncpy(txt, get_text(), len);
	return 1;
}


AndroidPort::AndroidPort(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, xS(72), 1, pwindow->thread->preferences->android_port)
{
	this->pwindow = pwindow;
}

int AndroidPort::handle_event()
{
	unsigned short port = atoi(get_text());
	if( port < 1024 ) port = 1024;
	pwindow->thread->preferences->android_port = port;
	char str[BCSTRLEN];
	sprintf(str,"%u",port);
	update(str);
	return 1;
}

int InterfacePrefs::start_shbtn_dialog()
{
	if( !shbtn_dialog )
		shbtn_dialog = new ShBtnEditDialog(pwindow);
	shbtn_dialog->start();
	return 1;
}

ShBtnPrefs::ShBtnPrefs(PreferencesWindow *pwindow, InterfacePrefs *iface_prefs, int x, int y)
 : BC_GenericButton(x, y, _("Shell Commands"))
{
	this->pwindow = pwindow;
	this->iface_prefs = iface_prefs;
	set_tooltip(_("Main Menu Shell Commands"));
}

int ShBtnPrefs::handle_event()
{
	return iface_prefs->start_shbtn_dialog();
}


StillImageUseDuration::StillImageUseDuration(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Import images with a duration of"))
{
	this->pwindow = pwindow;
}

int StillImageUseDuration::handle_event()
{
	pwindow->thread->edl->session->si_useduration = get_value();
	return 1;
}

StillImageDuration::StillImageDuration(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, xS(70), 1, pwindow->thread->edl->session->si_duration)
{
	this->pwindow = pwindow;
}
int StillImageDuration::handle_event()
{
	pwindow->thread->edl->session->si_duration = atof(get_text());
	return 1;
}


HairlineItem::HairlineItem(KeyframeReticle *popup, int hairline)
 : BC_MenuItem(popup->hairline_to_string(hairline))
{
	this->popup = popup;
	this->hairline = hairline;
}

HairlineItem::~HairlineItem()
{
}

int HairlineItem::handle_event()
{
	popup->pwindow->thread->redraw_overlays = 1;
	popup->set_text(get_text());
	*(popup->output) = hairline;
	return 1;
}


KeyframeReticle::KeyframeReticle(PreferencesWindow *pwindow,
	InterfacePrefs *iface_prefs, int x, int y, int *output)
 : BC_PopupMenu(x, y, xS(220), hairline_to_string(*output))
{
	this->pwindow = pwindow;
	this->iface_prefs = iface_prefs;
	this->output = output;
}

KeyframeReticle::~KeyframeReticle()
{
}

const char *KeyframeReticle::hairline_to_string(int type)
{
	switch( type ) {
	case HAIRLINE_NEVER:    return _("Never");
	case HAIRLINE_DRAGGING:	return _("Dragging");
	case HAIRLINE_ALWAYS:   return _("Always");
	}
	return _("Unknown");
}

void KeyframeReticle::create_objects()
{
	add_item(new HairlineItem(this, HAIRLINE_NEVER));
	add_item(new HairlineItem(this, HAIRLINE_DRAGGING));
	add_item(new HairlineItem(this, HAIRLINE_ALWAYS));
}

PrefsTrapSigSEGV::PrefsTrapSigSEGV(InterfacePrefs *subwindow, int x, int y)
 : BC_CheckBox(x, y,
	subwindow->pwindow->thread->preferences->trap_sigsegv,
	_("trap sigSEGV"))
{
	this->subwindow = subwindow;
}
PrefsTrapSigSEGV::~PrefsTrapSigSEGV()
{
}
int PrefsTrapSigSEGV::handle_event()
{
	subwindow->pwindow->thread->preferences->trap_sigsegv = get_value();
	return 1;
}

PrefsTrapSigINTR::PrefsTrapSigINTR(InterfacePrefs *subwindow, int x, int y)
 : BC_CheckBox(x, y,
	subwindow->pwindow->thread->preferences->trap_sigintr,
	_("trap sigINT"))
{
	this->subwindow = subwindow;
}
PrefsTrapSigINTR::~PrefsTrapSigINTR()
{
}
int PrefsTrapSigINTR::handle_event()
{
	subwindow->pwindow->thread->preferences->trap_sigintr = get_value();
	return 1;
}


void InterfacePrefs::start_probe_dialog()
{
	if( !file_probe_dialog )
		file_probe_dialog = new FileProbeDialog(pwindow);
	file_probe_dialog->start();
}

PrefsFileProbes::PrefsFileProbes(PreferencesWindow *pwindow,
		InterfacePrefs *subwindow, int x, int y)
 : BC_GenericButton(x, y, _("Probe Order"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
	set_tooltip(_("File Open Probe Ordering"));
}

int PrefsFileProbes::handle_event()
{
	subwindow->start_probe_dialog();
	return 1;
}


PrefsYUV420P_DVDlace::PrefsYUV420P_DVDlace(PreferencesWindow *pwindow,
	InterfacePrefs *subwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->dvd_yuv420p_interlace,
	_("Use yuv420p dvd interlace format"))
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

int PrefsYUV420P_DVDlace::handle_event()
{
	pwindow->thread->preferences->dvd_yuv420p_interlace = get_value();
	return 1;
}


SnapshotPathText::SnapshotPathText(PreferencesWindow *pwindow,
	InterfacePrefs *subwindow, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, pwindow->thread->preferences->snapshot_path)
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

SnapshotPathText::~SnapshotPathText()
{
}

int SnapshotPathText::handle_event()
{
	strcpy(pwindow->thread->preferences->snapshot_path, get_text());
	return 1;
}

PrefsAutostartLV2UI::PrefsAutostartLV2UI(int x, int y, PreferencesWindow *pwindow)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->autostart_lv2ui, _("Auto start lv2 gui"))
{
	this->pwindow = pwindow;
}
int PrefsAutostartLV2UI::handle_event()
{
	pwindow->thread->preferences->autostart_lv2ui = get_value();
	return 1;
}

PrefsReloadPlugins::PrefsReloadPlugins(PreferencesWindow *pwindow,
	InterfacePrefs *iface_prefs, int x, int y)
 : BC_GenericButton(x, y, _("Reload plugin index"))
{
	this->pwindow = pwindow;
	this->iface_prefs = iface_prefs;
}

int PrefsReloadPlugins::handle_event()
{
	pwindow->thread->reload_plugins = 1;
	text_color(get_resources()->button_highlighted);
	draw_face(1);
	return 1;
}

PrefsLV2PathText::PrefsLV2PathText(PreferencesWindow *pwindow,
	InterfacePrefs *subwindow, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, pwindow->thread->preferences->lv2_path)
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

PrefsLV2PathText::~PrefsLV2PathText()
{
}

int PrefsLV2PathText::handle_event()
{
	strcpy(pwindow->thread->preferences->lv2_path, get_text());
	return 1;
}

PrefsNestedProxyPath::PrefsNestedProxyPath(PreferencesWindow *pwindow,
	InterfacePrefs *subwindow, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, pwindow->thread->preferences->nested_proxy_path)
{
	this->pwindow = pwindow;
	this->subwindow = subwindow;
}

PrefsNestedProxyPath::~PrefsNestedProxyPath()
{
}

int PrefsNestedProxyPath::handle_event()
{
	strcpy(pwindow->thread->preferences->nested_proxy_path, get_text());
	return 1;
}

