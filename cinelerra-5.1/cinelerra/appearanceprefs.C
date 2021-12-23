
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

#include "appearanceprefs.h"
#include "deleteallindexes.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "shbtnprefs.h"
#include "theme.h"


AppearancePrefs::AppearancePrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	hms = 0;
	hmsf = 0;
	timecode = 0;
	samples = 0;
	frames = 0;
	hex = 0;
	feet = 0;
	layout_scale = 0;
	thumbnails = 0;
	thumbnail_size = 0;
	vicon_size = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Appearance");
}

AppearancePrefs::~AppearancePrefs()
{
	delete hms;
	delete hmsf;
	delete timecode;
	delete samples;
	delete frames;
	delete hex;
	delete feet;
	delete layout_scale;
	delete thumbnails;
	delete thumbnail_size;
	delete vicon_size;
}


void AppearancePrefs::create_objects()
{
	int xs5 = xS(5), xs10 = xS(10), xs30 = xS(30);
	int ys5 = yS(5), ys10 = yS(10), ys15 = yS(15);
	int ys20 = yS(20), ys35 = yS(35);
	BC_Resources *resources = BC_WindowBase::get_resources();
	int margin = mwindow->theme->widget_border;
	char string[BCTEXTLEN];
	int x0 = mwindow->theme->preferencesoptions_x;
	int y0 = mwindow->theme->preferencesoptions_y;
	int x = x0, y = y0, x1 = x + xS(100);

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Layout:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Layout section");
	y += title->get_h() + ys10;
	int y1 = y;

	ViewTheme *theme;
	add_subwindow(title = new BC_Title(x, y, _("Theme:")));
	title->context_help_set_keyword("Layout section");
	add_subwindow(theme = new ViewTheme(x1, y, pwindow));
	theme->create_objects();
	theme->context_help_set_keyword("Layout section");
	y += theme->get_h() + ys5;

	x = x0;
	ViewPluginIcons *plugin_icons;
	add_subwindow(title = new BC_Title(x, y, _("Plugin Icons:")));
	title->context_help_set_keyword("Updatable Icon Image Support");
	add_subwindow(plugin_icons = new ViewPluginIcons(x1, y, pwindow));
	plugin_icons->create_objects();
	plugin_icons->context_help_set_keyword("Updatable Icon Image Support");
	y += plugin_icons->get_h() + ys10;
	add_subwindow(title = new BC_Title(x, y, _("Language:")));
	title->context_help_set_keyword("Layout section");
	LayoutLocale *layout_locale;
	add_subwindow(layout_locale = new LayoutLocale(x1, y, pwindow));
	layout_locale->create_objects();
	layout_locale->context_help_set_keyword("Layout section");
	y += layout_locale->get_h() + ys15;
	x1 = get_w()/2;

	int x2 = x1 + xS(160), y2 = y;
	y = y1;

	add_subwindow(title = new BC_Title(x1, y, _("Layout Scale:")));
	title->context_help_set_keyword("Layout section");
	layout_scale = new ViewLayoutScale(pwindow, this, x2, y);
	layout_scale->create_objects();
	y += layout_scale->get_h() + ys5;
	add_subwindow(title = new BC_Title(x1, y, _("View thumbnail size:")));
	title->context_help_set_keyword("Layout section");
	thumbnail_size = new ViewThumbnailSize(pwindow, this, x2, y);
	thumbnail_size->create_objects();
	y += thumbnail_size->get_h() + ys5;
	add_subwindow(title = new BC_Title(x1, y, _("Vicon quality:")));
	title->context_help_set_keyword("Layout section");
	vicon_size = new ViewViconSize(pwindow, this, x2, y);
	vicon_size->create_objects();
	y += vicon_size->get_h() + ys5;
	add_subwindow(title = new BC_Title(x1, y, _("Vicon color mode:")));
	title->context_help_set_keyword("Layout section");
	add_subwindow(vicon_color_mode = new ViewViconColorMode(pwindow, x2, y));
	vicon_color_mode->create_objects();
	vicon_color_mode->context_help_set_keyword("Layout section");
	y += vicon_color_mode->get_h() + ys5;
	y = bmax(y, y2);	
	y += ys10;
	add_subwindow(new BC_Bar(xs5, y, get_w() - xs10));
	y += ys15;

	y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Time Format:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Time Format section");
	y += title->get_h() + ys10;
	add_subwindow(hms = new TimeFormatHMS(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_HMS,
		x, y));
	hms->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(hmsf = new TimeFormatHMSF(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_HMSF,
		x, y));
	hmsf->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(timecode = new TimeFormatTimecode(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_TIMECODE,
		x, y));
	timecode->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(samples = new TimeFormatSamples(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES,
		x, y));
	samples->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(hex = new TimeFormatHex(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SAMPLES_HEX,
		x, y));
	hex->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(frames = new TimeFormatFrames(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_FRAMES,
		x, y));
	frames->context_help_set_keyword("Time Format section");
	y += ys20;
	add_subwindow(feet = new TimeFormatFeet(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_FEET_FRAMES,
		x, y));
	feet->context_help_set_keyword("Time Format section");
	x += feet->get_w() + xS(15);
	add_subwindow(title = new BC_Title(x, y, _("Frames per foot:")));
	title->context_help_set_keyword("Time Format section");
	x += title->get_w() + margin;
	sprintf(string, "%0.2f", pwindow->thread->edl->session->frames_per_foot);
	add_subwindow(new TimeFormatFeetSetting(pwindow,
		x, y - ys5, 	string));
	x = x0;
	y += ys20;
	add_subwindow(seconds = new TimeFormatSeconds(pwindow, this,
		pwindow->thread->edl->session->time_format == TIME_SECONDS,
		x, y));
	seconds->context_help_set_keyword("Time Format section");
	y += ys35;
	y2 = y;
	
	x = x1;  y = y1;
	add_subwindow(title = new BC_Title(x, y, _("Color:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Color section");
	y += ys35;
	add_subwindow(title = new BC_Title(x, y, _("Highlighting Inversion color:")));
	title->context_help_set_keyword("Color section");
	x += title->get_w() + margin;
	char hex_color[BCSTRLEN];
	sprintf(hex_color, "%06x", preferences->highlight_inverse);
        add_subwindow(new HighlightInverseColor(pwindow, x, y, hex_color));
	x2 = x;  x = x1;
	y += ys35;
	add_subwindow(title = new BC_Title(x, y, _("Composer BG Color:")));
	title->context_help_set_keyword("Color section");
	int clr_color = pwindow->thread->edl->session->cwindow_clear_color;
        add_subwindow(cwdw_bg_color = new Composer_BG_Color(pwindow,
		x2, y, xS(80), yS(24), clr_color));
	draw_3d_border(x2-2,y-2, xS(80)+4,xS(24)+4, 1);
	cwdw_bg_color->create_objects();
	cwdw_bg_color->context_help_set_keyword("Color section");
	x2 += cwdw_bg_color->get_w();
	y += ys35;

	add_subwindow(title = new BC_Title(x1, y, _("YUV color space:")));
	title->context_help_set_keyword("Color Space and Color Range");
	x = x2 - xS(120);
	add_subwindow(yuv_color_space = new YuvColorSpace(x, y, pwindow));
	yuv_color_space->create_objects();
	yuv_color_space->context_help_set_keyword("Color Space and Color Range");
	y += yuv_color_space->get_h() + ys5;

	add_subwindow(title = new BC_Title(x1, y, _("YUV color range:")));
	title->context_help_set_keyword("Color Space and Color Range");
	x = x2 - xS(100);
	add_subwindow(yuv_color_range = new YuvColorRange(x, y, pwindow));
	yuv_color_range->create_objects();
	yuv_color_range->context_help_set_keyword("Color Space and Color Range");
	y += yuv_color_range->get_h() + ys35;
	if( y2 < y ) y2 = y;

	add_subwindow(new BC_Bar(x0, y2, get_w()-x0 - xs30));
	y += ys35;

	x = x0;  y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Warnings:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Warnings section");
	y += title->get_h() + ys10;
	UseWarnIndecies *idx_warn = new UseWarnIndecies(pwindow, x, y);
	add_subwindow(idx_warn);
	idx_warn->context_help_set_keyword("Warnings section");
	y += idx_warn->get_h() + ys5;
	BD_WarnRoot *bdwr_warn = new BD_WarnRoot(pwindow, x, y);
	add_subwindow(bdwr_warn);
	bdwr_warn->context_help_set_keyword("Blu-ray Workaround for Mount");
	y += bdwr_warn->get_h() + ys5;
	UseWarnFileRef *warn_ref = new UseWarnFileRef(pwindow, x, y);
	add_subwindow(warn_ref);
	warn_ref->context_help_set_keyword("File by Reference");
	y += warn_ref->get_h() + ys5;
	
	add_subwindow(new BC_Bar(x0, y, warn_ref->get_w()-x0 - xs30));
	y += ys15;

	add_subwindow(title = new BC_Title(x, y, _("Dangerous:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Dangerous section");
	y += title->get_h() + ys10;

	
	UseUnsafeGUI *unsafe_gui = new UseUnsafeGUI(pwindow, x, y);
	add_subwindow(unsafe_gui);
	unsafe_gui->context_help_set_keyword("Advanced features");
	y += unsafe_gui->get_h() + ys5;
	OngoingBackups *ongoing_backups = new OngoingBackups(pwindow, x, y);
	add_subwindow(ongoing_backups);
	ongoing_backups->context_help_set_keyword("Backup and Perpetual Session");
	y += ongoing_backups->get_h() + ys5;

	x = get_w() / 3 + xs30;
	y = y1;
	add_subwindow(title = new BC_Title(x, y, _("Flags:"), LARGEFONT,
		resources->text_default));
	title->context_help_set_keyword("Flags section");
	y += title->get_h() + ys10;
	y1 = y;
	AutocolorAssets *autocolor_assets = new AutocolorAssets(pwindow, x, y);
	add_subwindow(autocolor_assets);
	autocolor_assets->context_help_set_keyword("Color Title Bars and Assets");
	y += autocolor_assets->get_h() + ys5;
	PerpetualSession *perpetual = new PerpetualSession(x, y, pwindow);
	add_subwindow(perpetual);
	perpetual->context_help_set_keyword("Backup and Perpetual Session");
	y += perpetual->get_h() + ys5;
	RectifyAudioToggle *rect_toggle = new RectifyAudioToggle(x, y, pwindow);
	add_subwindow(rect_toggle);
	rect_toggle->context_help_set_keyword("Flags section");
	y += rect_toggle->get_h() + ys5;
	CtrlToggle *ctrl_toggle = new CtrlToggle(x, y, pwindow);
	add_subwindow(ctrl_toggle);
	ctrl_toggle->context_help_set_keyword("Selection Methods");
	y += ctrl_toggle->get_h() + ys5;
	ForwardRenderDisplacement *displacement = new ForwardRenderDisplacement(pwindow, x, y);
	add_subwindow(displacement);
	displacement->context_help_set_keyword("Always Show Next Frame");
	y += displacement->get_h() + ys5;
	UseTipWindow *tip_win = new UseTipWindow(pwindow, x, y);
	add_subwindow(tip_win);
	tip_win->context_help_set_keyword("Flags section");
	y += tip_win->get_h() + ys5;

	x = 2*get_w() / 3 - xs30;
	y = y1;
	add_subwindow(thumbnails = new ViewThumbnails(x, y, pwindow));
	thumbnails->context_help_set_keyword("Video Icons \\/ Audio Icons");
	y += thumbnails->get_h() + ys5;
	PopupMenuBtnup *pop_win = new PopupMenuBtnup(pwindow, x, y);
	add_subwindow(pop_win);
	pop_win->context_help_set_keyword("Flags section");
	y += pop_win->get_h() + ys5;
	GrabFocusPolicy *grab_input_focus = new GrabFocusPolicy(pwindow, x, y);
	add_subwindow(grab_input_focus);
	grab_input_focus->context_help_set_keyword("Flags section");
	y += grab_input_focus->get_h() + ys5;
	ActivateFocusPolicy *focus_activate = new ActivateFocusPolicy(pwindow, x, y);
	add_subwindow(focus_activate);
	focus_activate->context_help_set_keyword("Flags section");
	y += focus_activate->get_h() + ys5;
	DeactivateFocusPolicy *focus_deactivate = new DeactivateFocusPolicy(pwindow, x, y);
	add_subwindow(focus_deactivate);
	focus_deactivate->context_help_set_keyword("Flags section");
	y += focus_deactivate->get_h() + ys5;
	AutoRotate *auto_rotate = new AutoRotate(pwindow, x, y);
	add_subwindow(auto_rotate);
	auto_rotate->context_help_set_keyword("Flags section");
	y += auto_rotate->get_h() + ys5;
}

int AppearancePrefs::update(int new_value)
{
	pwindow->thread->redraw_times = 1;
	pwindow->thread->edl->session->time_format = new_value;
	hms->update(new_value == TIME_HMS);
	hmsf->update(new_value == TIME_HMSF);
	timecode->update(new_value == TIME_TIMECODE);
	samples->update(new_value == TIME_SAMPLES);
	hex->update(new_value == TIME_SAMPLES_HEX);
	frames->update(new_value == TIME_FRAMES);
	feet->update(new_value == TIME_FEET_FRAMES);
	seconds->update(new_value == TIME_SECONDS);
	return 0;
}


TimeFormatHMS::TimeFormatHMS(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_HMS_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHMS::handle_event()
{
	tfwindow->update(TIME_HMS);
	return 1;
}

TimeFormatHMSF::TimeFormatHMSF(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_HMSF_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHMSF::handle_event()
{
	tfwindow->update(TIME_HMSF);
	return 1;
}

TimeFormatTimecode::TimeFormatTimecode(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_TIMECODE_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatTimecode::handle_event()
{
	tfwindow->update(TIME_TIMECODE);
	return 1;
}

TimeFormatSamples::TimeFormatSamples(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SAMPLES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatSamples::handle_event()
{
	tfwindow->update(TIME_SAMPLES);
	return 1;
}

TimeFormatFrames::TimeFormatFrames(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_FRAMES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatFrames::handle_event()
{
	tfwindow->update(TIME_FRAMES);
	return 1;
}

TimeFormatHex::TimeFormatHex(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SAMPLES_HEX_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatHex::handle_event()
{
	tfwindow->update(TIME_SAMPLES_HEX);
	return 1;
}

TimeFormatSeconds::TimeFormatSeconds(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_SECONDS_TEXT)
{
	this->pwindow = pwindow;
	this->tfwindow = tfwindow;
}

int TimeFormatSeconds::handle_event()
{
	tfwindow->update(TIME_SECONDS);
	return 1;
}

TimeFormatFeet::TimeFormatFeet(PreferencesWindow *pwindow, AppearancePrefs *tfwindow, int value, int x, int y)
 : BC_Radial(x, y, value, TIME_FEET_FRAMES_TEXT)
{ this->pwindow = pwindow; this->tfwindow = tfwindow; }

int TimeFormatFeet::handle_event()
{
	tfwindow->update(TIME_FEET_FRAMES);
	return 1;
}

TimeFormatFeetSetting::TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, char *string)
 : BC_TextBox(x, y, xS(90), 1, string)
{ this->pwindow = pwindow; }

int TimeFormatFeetSetting::handle_event()
{
	pwindow->thread->edl->session->frames_per_foot = atof(get_text());
	if(pwindow->thread->edl->session->frames_per_foot < 1) pwindow->thread->edl->session->frames_per_foot = 1;
	return 0;
}


ViewTheme::ViewTheme(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, xS(200), pwindow->thread->preferences->theme, 1)
{
	this->pwindow = pwindow;
}
ViewTheme::~ViewTheme()
{
}

void ViewTheme::create_objects()
{
	ArrayList<PluginServer*> themes;
	MWindow::search_plugindb(0, 0, 0, 0, 1, themes);

	for(int i = 0; i < themes.total; i++) {
		add_item(new ViewThemeItem(this, themes.values[i]->title));
	}
}

int ViewTheme::handle_event()
{
	return 1;
}

ViewThemeItem::ViewThemeItem(ViewTheme *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int ViewThemeItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->theme, get_text());
	popup->handle_event();
	return 1;
}


ViewPluginIcons::ViewPluginIcons(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, xS(200), pwindow->thread->preferences->plugin_icons, 1)
{
	this->pwindow = pwindow;
}
ViewPluginIcons::~ViewPluginIcons()
{
}

void ViewPluginIcons::create_objects()
{
	add_item(new ViewPluginIconItem(this, DEFAULT_PICON));
	FileSystem fs;
	const char *plugin_path = File::get_plugin_path();
	char picon_path[BCTEXTLEN];
	snprintf(picon_path,sizeof(picon_path)-1,"%s/picon", plugin_path);
	if( fs.update(picon_path) ) return;
	for( int i=0; i<fs.dir_list.total; ++i ) {
		char *fs_path = fs.dir_list[i]->path;
		if( !fs.is_dir(fs_path) ) continue;
		char *cp = strrchr(fs_path,'/');
		cp = !cp ? fs_path : cp+1;
		if( !strcmp(cp,DEFAULT_PICON) ) continue;
		add_item(new ViewPluginIconItem(this, cp));
	}
}

int ViewPluginIcons::handle_event()
{
	return 1;
}

ViewPluginIconItem::ViewPluginIconItem(ViewPluginIcons *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int ViewPluginIconItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->plugin_icons, get_text());
	popup->handle_event();
	return 1;
}

LayoutLocale::LayoutLocale(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, xS(200), pwindow->thread->preferences->locale, 1)
{
	this->pwindow = pwindow;
}
LayoutLocale::~LayoutLocale()
{
}

const char *LayoutLocale::locale_list[] = { LOCALE_LIST, 0 };

void LayoutLocale::create_objects()
{
	for( const char *tp, **lp=locale_list; (tp=*lp)!=0; ++lp )
		add_item(new LayoutLocaleItem(this, tp));
}

int LayoutLocale::handle_event()
{
	return 1;
}

LayoutLocaleItem::LayoutLocaleItem(LayoutLocale *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}

int LayoutLocaleItem::handle_event()
{
	popup->set_text(get_text());
	strcpy(popup->pwindow->thread->preferences->locale, get_text());
	popup->handle_event();
	return 1;
}

ViewLayoutScale::ViewLayoutScale(PreferencesWindow *pwindow,
		AppearancePrefs *aprefs, int x, int y)
 : BC_TumbleTextBox(aprefs,
	pwindow->thread->preferences->layout_scale,
	0.f, 10.f, x, y, xS(80), 2)
{
	this->pwindow = pwindow;
	this->aprefs = aprefs;
	set_increment(0.1);
}

int ViewLayoutScale::handle_event()
{
	float v = atof(get_text());
	pwindow->thread->preferences->layout_scale = v;
	return 1;
}


ViewThumbnails::ViewThumbnails(int x,
	int y,
	PreferencesWindow *pwindow)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->preferences->use_thumbnails, _("Use thumbnails in resource window"))
{
	this->pwindow = pwindow;
}

int ViewThumbnails::handle_event()
{
	pwindow->thread->preferences->use_thumbnails = get_value();
	return 1;
}


ViewThumbnailSize::ViewThumbnailSize(PreferencesWindow *pwindow,
		AppearancePrefs *aprefs, int x, int y)
 : BC_TumbleTextBox(aprefs,
	pwindow->thread->preferences->awindow_picon_h,
	16, 512, x, y, xS(80))

{
	this->pwindow = pwindow;
	this->aprefs = aprefs;
}

int ViewThumbnailSize::handle_event()
{
	int v = atoi(get_text());
	bclamp(v, 16,512);
	pwindow->thread->preferences->awindow_picon_h = v;
	return 1;
}

ViewViconSize::ViewViconSize(PreferencesWindow *pwindow,
		AppearancePrefs *aprefs, int x, int y)
 : BC_TumbleTextBox(aprefs,
	pwindow->thread->preferences->vicon_size,
	16, 512, x, y, xS(80))

{
	this->pwindow = pwindow;
	this->aprefs = aprefs;
}

int ViewViconSize::handle_event()
{
	int v = atoi(get_text());
	bclamp(v, 16,512);
	pwindow->thread->preferences->vicon_size = v;
	return 1;
}

ViewViconColorMode::ViewViconColorMode(PreferencesWindow *pwindow, int x, int y)
 : BC_PopupMenu(x, y, xS(100),
	_(vicon_color_modes[pwindow->thread->preferences->vicon_color_mode]), 1)
{
	this->pwindow = pwindow;
}
ViewViconColorMode::~ViewViconColorMode()
{
}

const char *ViewViconColorMode::vicon_color_modes[] = {
	N_("Low"),
	N_("Med"),
	N_("High"),
};

void ViewViconColorMode::create_objects()
{
	for( int id=0,nid=sizeof(vicon_color_modes)/sizeof(vicon_color_modes[0]); id<nid; ++id )
		add_item(new ViewViconColorModeItem(this, _(vicon_color_modes[id]), id));
	handle_event();
}

int ViewViconColorMode::handle_event()
{
	set_text(_(vicon_color_modes[pwindow->thread->preferences->vicon_color_mode]));
	return 1;
}

ViewViconColorModeItem::ViewViconColorModeItem(ViewViconColorMode *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int ViewViconColorModeItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->vicon_color_mode = id;
	return popup->handle_event();
}


UseTipWindow::UseTipWindow(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->preferences->use_tipwindow,
	_("Show tip of the day"))
{
	this->pwindow = pwindow;
}
int UseTipWindow::handle_event()
{
	pwindow->thread->preferences->use_tipwindow = get_value();
	return 1;
}


UseWarnIndecies::UseWarnIndecies(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->warn_indexes,
	_("ffmpeg probe warns rebuild indexes"))
{
	this->pwindow = pwindow;
}

int UseWarnIndecies::handle_event()
{
	pwindow->thread->preferences->warn_indexes = get_value();
	return 1;
}

UseUnsafeGUI::UseUnsafeGUI(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->unsafe_gui,
	_("Unsafe GUI in batchrender"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Save to EDL path option becomes available and will overwrite EDL on disk. \n Warn if jobs/session mismatch option is available but can be unchecked."));
}

int UseUnsafeGUI::handle_event()
{
	pwindow->thread->preferences->unsafe_gui = get_value();
	return 1;
}

OngoingBackups::OngoingBackups(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->ongoing_backups,
	_("Autosave continuous backups"))
{
	this->pwindow = pwindow;
	set_tooltip(_("When you stop Cinelerra, all but the newest 50 will be deleted but you risk \n running out of disk space if you do a lot of work without restarting."));
}

int OngoingBackups::handle_event()
{
	pwindow->thread->preferences->ongoing_backups = get_value();
	return 1;
}

BD_WarnRoot::BD_WarnRoot(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->bd_warn_root,
	_("Create Bluray warns if not root"))
{
	this->pwindow = pwindow;
}

int BD_WarnRoot::handle_event()
{
	pwindow->thread->preferences->bd_warn_root = get_value();
	return 1;
}

UseWarnFileRef::UseWarnFileRef(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->warn_fileref,
	_("Warn on creating file references"))
{
	this->pwindow = pwindow;
}

int UseWarnFileRef::handle_event()
{
	pwindow->thread->preferences->warn_fileref = get_value();
	return 1;
}


PopupMenuBtnup::PopupMenuBtnup(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->popupmenu_btnup,
	_("Popups activate on button up"))
{
	this->pwindow = pwindow;
}

int PopupMenuBtnup::handle_event()
{
	pwindow->thread->preferences->popupmenu_btnup = get_value();
	return 1;
}

GrabFocusPolicy::GrabFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->grab_input_focus) != 0,
	_("Set Input Focus when window entered"))
{
	this->pwindow = pwindow;
}

int GrabFocusPolicy::handle_event()
{
	pwindow->thread->preferences->grab_input_focus = get_value();
	return 1;
}

ActivateFocusPolicy::ActivateFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->textbox_focus_policy & CLICK_ACTIVATE) != 0,
	_("Click to activate text focus"))
{
	this->pwindow = pwindow;
}

int ActivateFocusPolicy::handle_event()
{
	if( get_value() )
		pwindow->thread->preferences->textbox_focus_policy |= CLICK_ACTIVATE;
	else
		pwindow->thread->preferences->textbox_focus_policy &= ~CLICK_ACTIVATE;
	return 1;
}

DeactivateFocusPolicy::DeactivateFocusPolicy(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, (pwindow->thread->preferences->textbox_focus_policy & CLICK_DEACTIVATE) != 0,
	_("Click to deactivate text focus"))
{
	this->pwindow = pwindow;
}

int DeactivateFocusPolicy::handle_event()
{
	if( get_value() )
		pwindow->thread->preferences->textbox_focus_policy |= CLICK_DEACTIVATE;
	else
		pwindow->thread->preferences->textbox_focus_policy &= ~CLICK_DEACTIVATE;
	return 1;
}

AutoRotate::AutoRotate(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->auto_rotate != 0,
	_("Auto rotate ffmpeg media"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Automatically rotates media if legal rotation metadata in file."));
}

int AutoRotate::handle_event()
{
	pwindow->thread->preferences->auto_rotate = get_value();
	return 1;
}

ForwardRenderDisplacement::ForwardRenderDisplacement(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->forward_render_displacement,
	_("Always show next frame"))
{
	this->pwindow = pwindow;
}

int ForwardRenderDisplacement::handle_event()
{
	pwindow->thread->preferences->forward_render_displacement = get_value();
	return 1;
}

AutocolorAssets::AutocolorAssets(PreferencesWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->preferences->autocolor_assets,
	_("Autocolor assets"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Displays automatically generated color overlay for the \n edits on the timeline that belong to the same media file."));
}

int AutocolorAssets::handle_event()
{
	pwindow->thread->preferences->autocolor_assets = get_value();
	return 1;
}

HighlightInverseColor::HighlightInverseColor(PreferencesWindow *pwindow, int x, int y, const char *hex)
 : BC_TextBox(x, y, xS(80), 1, hex)
{
	this->pwindow = pwindow;
}

int HighlightInverseColor::handle_event()
{
	int inverse_color = strtoul(get_text(),0,16);
	if( (inverse_color &= 0xffffff) == 0 ) {
		inverse_color = 0xffffff;
		char string[BCSTRLEN];
		sprintf(string,"%06x", inverse_color);
		update(string);
	}
	pwindow->thread->preferences->highlight_inverse = inverse_color;
	return 1;
}

// num. order of those entries must be same as in
// guicast/bccolors.inc

const char *YuvColorSpace::color_space[MAX_COLOR_SPACE] = {
	N_("BT601_NTSC"), // COLOR SPACE BT601_NTSC
	N_("BT709"), // COLOR_SPACE_BT709
	N_("BT2020 NCL"), // COLOR_SPACE_BT2020_NCL
	N_("BT601_PAL"), // COLOR_SPACE_BT601_PAL
	N_("BT2020 CL"), // COLOR_SPACE_BT2020_CL
};

YuvColorSpace::YuvColorSpace(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, xS(140),
	_(color_space[pwindow->thread->preferences->yuv_color_space]), 1)
{
	this->pwindow = pwindow;
}
YuvColorSpace::~YuvColorSpace()
{
}

void YuvColorSpace::create_objects()
{
	for( int id=0,nid=sizeof(color_space)/sizeof(color_space[0]); id<nid; ++id )
		add_item(new YuvColorSpaceItem(this, _(color_space[id]), id));
	handle_event();
}

int YuvColorSpace::handle_event()
{
	set_text(_(color_space[pwindow->thread->preferences->yuv_color_space]));
	return 1;
}

YuvColorSpaceItem::YuvColorSpaceItem(YuvColorSpace *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int YuvColorSpaceItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->yuv_color_space = id;
	return popup->handle_event();
}


const char *YuvColorRange::color_range[] = {
	N_("JPEG"), // COLOR_RANGE_JPEG
	N_("MPEG"), // COLOR_RANGE_MPEG
};

YuvColorRange::YuvColorRange(int x, int y, PreferencesWindow *pwindow)
 : BC_PopupMenu(x, y, xS(100),
	_(color_range[pwindow->thread->preferences->yuv_color_range]), 1)
{
	this->pwindow = pwindow;
}
YuvColorRange::~YuvColorRange()
{
}

void YuvColorRange::create_objects()
{
	for( int id=0,nid=sizeof(color_range)/sizeof(color_range[0]); id<nid; ++id )
		add_item(new YuvColorRangeItem(this, _(color_range[id]), id));
	handle_event();
}

int YuvColorRange::handle_event()
{
	set_text(color_range[pwindow->thread->preferences->yuv_color_range]);
	return 1;
}

YuvColorRangeItem::YuvColorRangeItem(YuvColorRange *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int YuvColorRangeItem::handle_event()
{
	popup->set_text(get_text());
	popup->pwindow->thread->preferences->yuv_color_range = id;
	return popup->handle_event();
}

PerpetualSession::PerpetualSession(int x, int y, PreferencesWindow *pwindow)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->perpetual_session, _("Perpetual session"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Resume previous session on startup with undo/redo stack saved between sessions. \n On startup, previous project is loaded as if there was no stoppage."));
}

int PerpetualSession::handle_event()
{
	pwindow->thread->preferences->perpetual_session = get_value();
	return 1;
}

CtrlToggle::CtrlToggle(int x, int y, PreferencesWindow *pwindow)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->ctrl_toggle, _("Clears before toggle"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Drag and Drop editing - when using LMB on edit,\n clears all selected edits except this one."));
}

int CtrlToggle::handle_event()
{
	pwindow->thread->preferences->ctrl_toggle = get_value();
	return 1;
}

RectifyAudioToggle::RectifyAudioToggle(int x, int y, PreferencesWindow *pwindow)
 : BC_CheckBox(x, y,
	pwindow->thread->preferences->rectify_audio, _("Timeline Rectify Audio"))
{
	this->pwindow = pwindow;
	set_tooltip(_("Displays rectified audio showing only positive half of the waveform \n resulting in waveform stretched more over the height of the track."));
}

int RectifyAudioToggle::handle_event()
{
	pwindow->thread->preferences->rectify_audio = get_value();
	return 1;
}

Composer_BG_Color::Composer_BG_Color(PreferencesWindow *pwindow,
		int x, int y, int w, int h, int color)
 : ColorBoxButton(_("Composer BG color"), x, y, w, h, color, -1, 1)
{
	this->pwindow = pwindow;
}

Composer_BG_Color::~Composer_BG_Color()
{
}

void Composer_BG_Color::handle_done_event(int result)
{
	if( result ) {
		pwindow->lock_window("Composer_BG_Color::handle_done_event");
		update_gui(orig_color, orig_alpha);
		pwindow->unlock_window();
		handle_new_color(orig_color, orig_alpha);
	}
}

int Composer_BG_Color::handle_new_color(int color, int alpha)
{
	pwindow->thread->edl->session->cwindow_clear_color = color;
	return 1;
}

