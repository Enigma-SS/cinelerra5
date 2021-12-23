
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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
#include "auto.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "bdcreate.h"
#include "cache.h"
#include "channelinfo.h"
#include "convert.h"
#include "cplayback.h"
#include "cropvideo.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "dbwindow.h"
#include "dvdcreate.h"
#include "edl.h"
#include "edlsession.h"
#include "exportedl.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatauto.h"
#include "keys.h"
#include "language.h"
#include "levelwindow.h"
#include "loadfile.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "menuattacheffect.h"
#include "menuattachtransition.h"
#include "menuaeffects.h"
#include "menueditlength.h"
#include "menutransitionlength.h"
#include "menuveffects.h"
#include "mixersalign.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "new.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "preferences.h"
#include "proxy.h"
#include "preferencesthread.h"
#include "quit.h"
#include "record.h"
#include "render.h"
#include "savefile.h"
#include "setformat.h"
#include "swindow.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "viewmenu.h"
#include "zoombar.h"
#include "zwindow.h"
#include "zwindowgui.h"

#include <string.h>


MainMenu::MainMenu(MWindow *mwindow, MWindowGUI *gui, int w)
 : BC_MenuBar(0, 0, w)
{
	this->gui = gui;
	this->mwindow = mwindow;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Program Window");
}

MainMenu::~MainMenu()
{
}

void MainMenu::create_objects()
{
	BC_Menu *viewmenu, *windowmenu, *settingsmenu, *trackmenu;
	PreferencesMenuitem *preferences;

	add_menu(filemenu = new BC_Menu(_("File")));
	filemenu->add_item(new_project = new NewProject(mwindow));
	new_project->create_objects();

// file loaders
	filemenu->add_item(load_file = new Load(mwindow, this));
	load_file->create_objects();
	filemenu->add_item(load_recent = new LoadRecent(mwindow, this));
	load_recent->create_objects();

// new and load can be undone so no need to prompt save
	Save *save;                   //  affected by saveas
	filemenu->add_item(save = new Save(mwindow));
	SaveAs *saveas;
	filemenu->add_item(saveas = new SaveAs(mwindow));
	save->create_objects(saveas);
	saveas->set_mainmenu(this);
	filemenu->add_item(new SaveProject(mwindow));
	filemenu->add_item(new SaveSession(mwindow));

	filemenu->add_item(record_menu_item = new RecordMenuItem(mwindow));
#ifdef HAVE_DVB
	filemenu->add_item(new ChannelScan(mwindow));
#endif
#ifdef HAVE_COMMERCIAL
	if( mwindow->has_commercials() )
		filemenu->add_item(new DbWindowScan(mwindow));
#endif
	filemenu->add_item(new SubttlSWin(mwindow));

	filemenu->add_item(render = new RenderItem(mwindow));
	filemenu->add_item(new ExportEDLItem(mwindow));
	filemenu->add_item(new BatchRenderMenuItem(mwindow));
	filemenu->add_item(new CreateBD_MenuItem(mwindow));
	filemenu->add_item(new CreateDVD_MenuItem(mwindow));
	filemenu->add_item(new BC_MenuItem("-"));
	filemenu->add_item(quit_program = new Quit(mwindow));
	quit_program->create_objects(save);
	filemenu->add_item(dump_menu = new MainDumpsMenu(mwindow));
	dump_menu->create_objects();
	filemenu->add_item(new LoadBackup(mwindow));
	filemenu->add_item(new SaveBackup(mwindow));

	BC_Menu *editmenu;
	add_menu(editmenu = new BC_Menu(_("Edit")));
	editmenu->add_item(undo = new Undo(mwindow));
	editmenu->add_item(redo = new Redo(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new Cut(mwindow));
	editmenu->add_item(new Copy(mwindow));
	editmenu->add_item(new Paste(mwindow));
	editmenu->add_item(new PasteSilence(mwindow));
	editmenu->add_item(clear_menu = new EditClearMenu(mwindow));
	clear_menu->create_objects();
	editmenu->add_item(new TrimSelection(mwindow));
	editmenu->add_item(new SelectAll(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new MenuEditShuffle(mwindow));
	editmenu->add_item(new MenuEditReverse(mwindow));
	editmenu->add_item(new MenuEditLength(mwindow));
	editmenu->add_item(new MenuEditAlign(mwindow));
	editmenu->add_item(new MenuTransitionLength(mwindow));
	editmenu->add_item(new DetachTransitions(mwindow));
	editmenu->add_item(new BC_MenuItem("-"));
	editmenu->add_item(new CutCommercials(mwindow));
	editmenu->add_item(new PasteSubttl(mwindow));

	BC_Menu *keyframemenu;
	add_menu(keyframemenu = new BC_Menu(_("Keyframes")));
	keyframemenu->add_item(new CutKeyframes(mwindow));
	keyframemenu->add_item(new CopyKeyframes(mwindow));
	keyframemenu->add_item(new PasteKeyframes(mwindow));
	keyframemenu->add_item(new ClearKeyframes(mwindow));
	keyframemenu->add_item(set_auto_curves = new SetAutomationCurveMode(mwindow));
	set_auto_curves->create_objects();
	keyframemenu->add_item(keyframe_curve_type = new KeyframeCurveType(mwindow));
	keyframe_curve_type->create_objects();
	keyframe_curve_type->update(mwindow->edl->local_session->floatauto_type);
	keyframemenu->add_item(keyframe_create = new KeyframeCreate(mwindow));
	keyframe_create->create_objects();
	keyframemenu->add_item(new BC_MenuItem("-"));
	keyframemenu->add_item(new CopyDefaultKeyframe(mwindow));
	keyframemenu->add_item(new PasteDefaultKeyframe(mwindow));




	add_menu(audiomenu = new BC_Menu(_("Audio")));
	audiomenu->add_item(new AddAudioTrack(mwindow));
	audiomenu->add_item(new DefaultATransition(mwindow));
	audiomenu->add_item(new MapAudio1(mwindow));
	audiomenu->add_item(new MapAudio2(mwindow));
	audiomenu->add_item(new MenuAttachTransition(mwindow, TRACK_AUDIO));
	audiomenu->add_item(new MenuAttachEffect(mwindow, TRACK_AUDIO));
	audiomenu->add_item(aeffects = new MenuAEffects(mwindow));

	add_menu(videomenu = new BC_Menu(_("Video")));
	videomenu->add_item(new AddVideoTrack(mwindow));
	videomenu->add_item(new DefaultVTransition(mwindow));
	videomenu->add_item(new MenuAttachTransition(mwindow, TRACK_VIDEO));
	videomenu->add_item(new MenuAttachEffect(mwindow, TRACK_VIDEO));
	videomenu->add_item(veffects = new MenuVEffects(mwindow));

	add_menu(trackmenu = new BC_Menu(_("Tracks")));
	trackmenu->add_item(new MoveTracksUp(mwindow));
	trackmenu->add_item(new MoveTracksDown(mwindow));
	trackmenu->add_item(new RollTracksUp(mwindow));
	trackmenu->add_item(new RollTracksDown(mwindow));
	trackmenu->add_item(new DeleteTracks(mwindow));
	trackmenu->add_item(new DeleteFirstTrack(mwindow));
	trackmenu->add_item(new DeleteLastTrack(mwindow));
	trackmenu->add_item(new ConcatenateTracks(mwindow));
	trackmenu->add_item(new AlignTimecodes(mwindow));
	AppendTracks *append_tracks;
	trackmenu->add_item(append_tracks = new AppendTracks(mwindow));
	append_tracks->create_objects();
	trackmenu->add_item(new AddSubttlTrack(mwindow));

	add_menu(settingsmenu = new BC_Menu(_("Settings")));

	settingsmenu->add_item(new SetFormat(mwindow));
	settingsmenu->add_item(preferences = new PreferencesMenuitem(mwindow));
	settingsmenu->add_item(proxy = new ProxyMenuItem(mwindow));
	proxy->create_objects();
	ConvertMenuItem *convert;
	settingsmenu->add_item(convert = new ConvertMenuItem(mwindow));
	convert->create_objects();
	mwindow->preferences_thread = preferences->thread;
	settingsmenu->add_item(cursor_on_frames = new CursorOnFrames(mwindow));
	settingsmenu->add_item(labels_follow_edits = new LabelsFollowEdits(mwindow));
	settingsmenu->add_item(plugins_follow_edits = new PluginsFollowEdits(mwindow));
	settingsmenu->add_item(keyframes_follow_edits = new KeyframesFollowEdits(mwindow));
	settingsmenu->add_item(typeless_keyframes = new TypelessKeyframes(mwindow));
	settingsmenu->add_item(new BC_MenuItem("-"));
	settingsmenu->add_item(new SaveSettingsNow(mwindow));
	settingsmenu->add_item(loop_playback = new LoopPlayback(mwindow));
	settingsmenu->add_item(brender_active = new SetBRenderActive(mwindow));
// set scrubbing speed
//	ScrubSpeed *scrub_speed;
//	settingsmenu->add_item(scrub_speed = new ScrubSpeed(mwindow));
//	if(mwindow->edl->session->scrub_speed == .5)
//		scrub_speed->set_text(_("Fast Shuttle"));






	add_menu(viewmenu = new BC_Menu(_("View")));
	viewmenu->add_item(show_assets = new ShowAssets(mwindow, "0"));
	viewmenu->add_item(show_titles = new ShowTitles(mwindow, "1"));
	viewmenu->add_item(show_transitions = new ShowTransitions(mwindow, "2"));
	viewmenu->add_item(fade_automation = new ShowAutomation(mwindow, _("Fade"), "3", AUTOMATION_FADE));
	viewmenu->add_item(mute_automation = new ShowAutomation(mwindow, _("Mute"), "4", AUTOMATION_MUTE));
	viewmenu->add_item(mode_automation = new ShowAutomation(mwindow, _("Overlay mode"), "5", AUTOMATION_MODE));
	viewmenu->add_item(pan_automation = new ShowAutomation(mwindow, _("Pan"), "6", AUTOMATION_PAN));
	viewmenu->add_item(plugin_automation = new PluginAutomation(mwindow, "7"));
	viewmenu->add_item(mask_automation = new ShowAutomation(mwindow, _("Mask"), "8", AUTOMATION_MASK));
	viewmenu->add_item(speed_automation = new ShowAutomation(mwindow, _("Speed"), "9", AUTOMATION_SPEED));

	camera_x = new ShowAutomation(mwindow, _("Camera X"), "Ctrl-Shift-X", AUTOMATION_CAMERA_X);
	camera_x->set_ctrl();  camera_x->set_shift();   viewmenu->add_item(camera_x);
	camera_y = new ShowAutomation(mwindow, _("Camera Y"), "Ctrl-Shift-Y", AUTOMATION_CAMERA_Y);
	camera_y->set_ctrl();  camera_y->set_shift();   viewmenu->add_item(camera_y);
	camera_z = new ShowAutomation(mwindow, _("Camera Z"), "Ctrl-Shift-Z", AUTOMATION_CAMERA_Z);
	camera_z->set_ctrl();  camera_z->set_shift();  viewmenu->add_item(camera_z);
	project_x = new ShowAutomation(mwindow, _("Projector X"), "Alt-Shift-X", AUTOMATION_PROJECTOR_X);
	project_x->set_alt();  project_x->set_shift();  viewmenu->add_item(project_x);
	project_y = new ShowAutomation(mwindow, _("Projector Y"), "Alt-Shift-Y", AUTOMATION_PROJECTOR_Y);
	project_y->set_alt();  project_y->set_shift();  viewmenu->add_item(project_y);
	project_z = new ShowAutomation(mwindow, _("Projector Z"), "Alt-Shift-Z", AUTOMATION_PROJECTOR_Z);
	project_z->set_alt();  project_z->set_shift();  viewmenu->add_item(project_z);

	add_menu(windowmenu = new BC_Menu(_("Window")));
	windowmenu->add_item(show_vwindow = new ShowVWindow(mwindow));
	windowmenu->add_item(show_awindow = new ShowAWindow(mwindow));
	windowmenu->add_item(show_cwindow = new ShowCWindow(mwindow));
	windowmenu->add_item(show_gwindow = new ShowGWindow(mwindow));
	windowmenu->add_item(show_lwindow = new ShowLWindow(mwindow));
	windowmenu->add_item(new BC_MenuItem("-"));
	windowmenu->add_item(split_x = new SplitX(mwindow));
	windowmenu->add_item(split_y = new SplitY(mwindow));
	windowmenu->add_item(mixer_items = new MixerItems(mwindow));
	mixer_items->create_objects();
	windowmenu->add_item(new TileWindows(mwindow,_("Tile left"),0));
	windowmenu->add_item(new TileWindows(mwindow,_("Tile right"),1));
	windowmenu->add_item(new BC_MenuItem("-"));

	windowmenu->add_item(new TileWindows(mwindow,_("Default positions"),-1,_("Ctrl-P"),'p'));
	windowmenu->add_item(load_layout = new LoadLayout(mwindow, _("Load layout..."),LAYOUT_LOAD));
	load_layout->create_objects();
	windowmenu->add_item(save_layout = new LoadLayout(mwindow, _("Save layout..."),LAYOUT_SAVE));
	save_layout->create_objects();
}

int MainMenu::load_defaults(BC_Hash *defaults)
{
	init_loads(defaults);
	init_aeffects(defaults);
	init_veffects(defaults);
	return 0;
}

void MainMenu::update_toggles(int use_lock)
{
	if(use_lock) lock_window("MainMenu::update_toggles");
	labels_follow_edits->set_checked(mwindow->edl->session->labels_follow_edits);
	plugins_follow_edits->set_checked(mwindow->edl->session->plugins_follow_edits);
	keyframes_follow_edits->set_checked(mwindow->edl->session->autos_follow_edits);
	typeless_keyframes->set_checked(mwindow->edl->session->typeless_keyframes);
	cursor_on_frames->set_checked(mwindow->edl->session->cursor_on_frames);
	loop_playback->set_checked(mwindow->edl->local_session->loop_playback);

	show_assets->set_checked(mwindow->edl->session->show_assets);
	show_titles->set_checked(mwindow->edl->session->show_titles);
	show_transitions->set_checked(mwindow->edl->session->auto_conf->transitions);
	fade_automation->update_toggle();
	mute_automation->update_toggle();
	pan_automation->update_toggle();
	camera_x->update_toggle();
	camera_y->update_toggle();
	camera_z->update_toggle();
	project_x->update_toggle();
	project_y->update_toggle();
	project_z->update_toggle();
	plugin_automation->set_checked(mwindow->edl->session->auto_conf->plugins);
	mode_automation->update_toggle();
	mask_automation->update_toggle();
	speed_automation->update_toggle();
	split_x->set_checked(mwindow->gui->pane[TOP_RIGHT_PANE] != 0);
	split_y->set_checked(mwindow->gui->pane[BOTTOM_LEFT_PANE] != 0);

	if(use_lock) mwindow->gui->unlock_window();
}

int MainMenu::save_defaults(BC_Hash *defaults)
{
	save_loads(defaults);
	save_aeffects(defaults);
	save_veffects(defaults);
	return 0;
}





int MainMenu::quit()
{
	quit_program->handle_event();
	return 0;
}





// ================================== load most recent

int MainMenu::init_aeffects(BC_Hash *defaults)
{
	total_aeffects = defaults->get((char*)"TOTAL_AEFFECTS", 0);

	char string[BCTEXTLEN], title[BCTEXTLEN];
	if(total_aeffects) audiomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->get(string, title);
		audiomenu->add_item(aeffect[i] = new MenuAEffectItem(aeffects, title));
	}
	return 0;
}

int MainMenu::init_veffects(BC_Hash *defaults)
{
	total_veffects = defaults->get((char*)"TOTAL_VEFFECTS", 0);

	char string[BCTEXTLEN], title[BCTEXTLEN];
	if(total_veffects) videomenu->add_item(new BC_MenuItem("-"));

	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->get(string, title);
		videomenu->add_item(veffect[i] = new MenuVEffectItem(veffects, title));
	}
	return 0;
}

void MainMenu::init_loads(BC_Hash *defaults)
{
// total_loads for legacy xml
	int total_loads = defaults->get((char*)"TOTAL_LOADS", 0);
	int loads_total = defaults->get((char*)"LOADS_TOTAL", 0);
	if( loads_total < total_loads ) loads_total = total_loads;

	char string[BCTEXTLEN], path[BCTEXTLEN];
	FileSystem dir;
//printf("MainMenu::init_loads 2\n");

	for( int i=0; i<loads_total; ++i ) {
		sprintf(string, "LOADPREVIOUS%d", i);
//printf("MainMenu::init_loads 3\n");
		defaults->get(string, path);
		if( load.size() < TOTAL_LOADS )
			load.append(new LoadRecentItem(path));
	}
}

// ============================ save most recent

int MainMenu::save_aeffects(BC_Hash *defaults)
{
	defaults->update((char*)"TOTAL_AEFFECTS", total_aeffects);
	char string[BCTEXTLEN];
	for(int i = 0; i < total_aeffects; i++)
	{
		sprintf(string, "AEFFECTRECENT%d", i);
		defaults->update(string, aeffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_veffects(BC_Hash *defaults)
{
	defaults->update((char*)"TOTAL_VEFFECTS", total_veffects);
	char string[BCTEXTLEN];
	for(int i = 0; i < total_veffects; i++)
	{
		sprintf(string, "VEFFECTRECENT%d", i);
		defaults->update(string, veffect[i]->get_text());
	}
	return 0;
}

int MainMenu::save_loads(BC_Hash *defaults)
{
// legacy to prevent segv, older code cant tolerate total_loads>10
	int loads_total = load.size();
	int total_loads = MIN(10, loads_total);
	defaults->update((char*)"LOADS_TOTAL", loads_total);
	defaults->update((char*)"TOTAL_LOADS", total_loads);
	char string[BCTEXTLEN];
	for( int i=0; i<loads_total; ++i ) {
		sprintf(string, "LOADPREVIOUS%d", i);
		defaults->update(string, load[i]->path);
	}
	return 0;
}

// =================================== add most recent

int MainMenu::add_aeffect(char *title)
{
// add bar for first effect
	if(total_aeffects == 0)
	{
		audiomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_aeffects; i++)
	{
		if(!strcmp(aeffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				aeffect[j]->set_text(aeffect[j - 1]->get_text());
			}
			aeffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_aeffects < TOTAL_EFFECTS)
	{
		audiomenu->add_item(
			aeffect[total_aeffects] = new MenuAEffectItem(aeffects, (char*)""));
		total_aeffects++;
	}

// cycle effect down
	for(int i = total_aeffects - 1; i > 0; i--)
	{
	// set menu item text
		aeffect[i]->set_text(aeffect[i - 1]->get_text());
	}

// set up the new effect
	aeffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_veffect(char *title)
{
// add bar for first effect
	if(total_veffects == 0)
	{
		videomenu->add_item(new BC_MenuItem("-"));
	}

// test for existing copy of effect
	for(int i = 0; i < total_veffects; i++)
	{
		if(!strcmp(veffect[i]->get_text(), title))     // already exists
		{                                // swap for top effect
			for(int j = i; j > 0; j--)   // move preceeding effects down
			{
				veffect[j]->set_text(veffect[j - 1]->get_text());
			}
			veffect[0]->set_text(title);
			return 1;
		}
	}

// add another blank effect
	if(total_veffects < TOTAL_EFFECTS)
	{
		videomenu->add_item(veffect[total_veffects] =
			new MenuVEffectItem(veffects, (char*)""));
		total_veffects++;
	}

// cycle effect down
	for(int i = total_veffects - 1; i > 0; i--)
	{
// set menu item text
		veffect[i]->set_text(veffect[i - 1]->get_text());
	}

// set up the new effect
	veffect[0]->set_text(title);
	return 0;
}

int MainMenu::add_load(char *path)
{
	return load.add_load(path);
}



// ================================== menu items

MainDumpsSubMenu::MainDumpsSubMenu(BC_MenuItem *menu_item)
 : BC_SubMenu()
{
	this->menu_item = menu_item;
}
MainDumpsSubMenu::~MainDumpsSubMenu()
{
}


MainDumpsMenu::MainDumpsMenu(MWindow *mwindow)
 : BC_MenuItem(_("Dumps..."))
{
	this->mwindow = mwindow;
	this->dumps_menu = 0;
}
MainDumpsMenu::~MainDumpsMenu()
{
}

void MainDumpsMenu::create_objects()
{
	add_submenu(dumps_menu = new MainDumpsSubMenu(this));
//	dumps_menu->add_item(new DumpCICache(mwindow));
	dumps_menu->add_item(new DumpEDL(mwindow));
	dumps_menu->add_item(new DumpPlugins(mwindow));
	dumps_menu->add_item(new DumpAssets(mwindow));
	dumps_menu->add_item(new DumpUndo(mwindow));
};


DumpCICache::DumpCICache(MWindow *mwindow)
 : BC_MenuItem(_("Dump CICache"))
{ this->mwindow = mwindow; }

int DumpCICache::handle_event()
{
//	mwindow->cache->dump();
	return 1;
}

DumpEDL::DumpEDL(MWindow *mwindow)
 : BC_MenuItem(_("Dump EDL"))
{
	this->mwindow = mwindow;
}

int DumpEDL::handle_event()
{
	mwindow->dump_edl();
	return 1;
}

DumpPlugins::DumpPlugins(MWindow *mwindow)
 : BC_MenuItem(_("Dump Plugins"))
{
	this->mwindow = mwindow;
}

int DumpPlugins::handle_event()
{
	mwindow->dump_plugins();
	return 1;
}

DumpAssets::DumpAssets(MWindow *mwindow)
 : BC_MenuItem(_("Dump Assets"))
{ this->mwindow = mwindow; }

int DumpAssets::handle_event()
{
	mwindow->edl->assets->dump();
	return 1;
}

DumpUndo::DumpUndo(MWindow *mwindow)
 : BC_MenuItem(_("Dump Undo"))
{
	this->mwindow = mwindow;
}

int DumpUndo::handle_event()
{
	mwindow->dump_undo();
	return 1;
}

// ================================================= edit

Undo::Undo(MWindow *mwindow) : BC_MenuItem(_("Undo"), "z or Ctrl-z", 'z')
{
	this->mwindow = mwindow;
}
int Undo::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->undo_entry(mwindow->gui);
	return 1;
}
int Undo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];
	sprintf(string, _("Undo %s"), new_caption);
	set_text(string);
	return 0;
}


Redo::Redo(MWindow *mwindow) : BC_MenuItem(_("Redo"), _("Shift-Z"), 'Z')
{
	set_shift(1);
	this->mwindow = mwindow;
}

int Redo::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->redo_entry(mwindow->gui);
	return 1;
}
int Redo::update_caption(const char *new_caption)
{
	char string[BCTEXTLEN];
	sprintf(string, _("Redo %s"), new_caption);
	set_text(string);
	return 0;
}

CutKeyframes::CutKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Cut keyframes"), _("Shift-X"), 'X')
{
	set_shift();
	this->mwindow = mwindow;
}

int CutKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut_automation();
	return 1;
}

CopyKeyframes::CopyKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Copy keyframes"), _("Shift-C"), 'C')
{
	set_shift();
	this->mwindow = mwindow;
}

int CopyKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy_automation();
	return 1;
}

PasteKeyframes::PasteKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Paste keyframes"), _("Shift-V"), 'V')
{
	set_shift();
	this->mwindow = mwindow;
}

int PasteKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_automation();
	return 1;
}

ClearKeyframes::ClearKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Clear keyframes"), _("Shift-Del"), DELETE)
{
	set_shift();
	this->mwindow = mwindow;
}

int ClearKeyframes::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->clear_automation();
	return 1;
}


SetAutomationCurveItem::SetAutomationCurveItem(SetAutomationCurveMode *set_curve_mode, int id)
 : BC_MenuItem(FloatAuto::curve_name(id))
{
	this->set_curve_mode = set_curve_mode;
	this->id = id;
}

int SetAutomationCurveItem::handle_event()
{
	set_curve_mode->mwindow->set_automation_mode((FloatAuto::t_mode)id);
	return 1;
}

SetAutoCurveModeMenu::SetAutoCurveModeMenu(SetAutomationCurveMode *curve_mode)
: BC_SubMenu()
{
	this->curve_mode = curve_mode;
}

SetAutomationCurveMode::SetAutomationCurveMode(MWindow *mwindow)
 : BC_MenuItem(_("Set curve modes..."))
{
	this->mwindow = mwindow;
	curve_mode_menu = 0;
}

void SetAutomationCurveMode::create_objects()
{
	add_submenu(curve_mode_menu = new SetAutoCurveModeMenu(this));
	for( int id=FloatAuto::SMOOTH; id<=FloatAuto::BUMP; ++id )
		curve_mode_menu->add_item(new SetAutomationCurveItem(this, id));
}


KeyframeCurveType::KeyframeCurveType(MWindow *mwindow)
 : BC_MenuItem(_("Create curve type..."))
{
	this->mwindow = mwindow;
	this->curve_menu = 0;
}
KeyframeCurveType::~KeyframeCurveType()
{
}

void KeyframeCurveType::create_objects()
{
	add_submenu(curve_menu = new KeyframeCurveTypeMenu(this));
	for( int i=FloatAuto::SMOOTH; i<=FloatAuto::BUMP; ++i ) {
		KeyframeCurveTypeItem *curve_type_item = new KeyframeCurveTypeItem(i, this);
		curve_menu->add_submenuitem(curve_type_item);
	}
}

void KeyframeCurveType::update(int curve_type)
{
	for( int i=0; i<curve_menu->total_items(); ++i ) {
		KeyframeCurveTypeItem *curve_type_item = (KeyframeCurveTypeItem *)curve_menu->get_item(i);
		curve_type_item->set_checked(curve_type_item->type == curve_type);
	}
}

int KeyframeCurveType::handle_event()
{
	return 1;
}

KeyframeCurveTypeMenu::KeyframeCurveTypeMenu(KeyframeCurveType *menu_item)
 : BC_SubMenu()
{
	this->menu_item = menu_item;
}
KeyframeCurveTypeMenu::~KeyframeCurveTypeMenu()
{
}

KeyframeCurveTypeItem::KeyframeCurveTypeItem(int type, KeyframeCurveType *main_item)
 : BC_MenuItem(FloatAuto::curve_name(type))
{
	this->type = type;
	this->main_item = main_item;
}
KeyframeCurveTypeItem::~KeyframeCurveTypeItem()
{
}

int KeyframeCurveTypeItem::handle_event()
{
	main_item->update(type);
	main_item->mwindow->set_keyframe_type(type);
	return 1;
}


KeyframeCreateItem::KeyframeCreateItem(KeyframeCreate *keyframe_create,
			const char *text, int mask)
 : BC_MenuItem(text)
{
	this->keyframe_create = keyframe_create;
	this->mask = mask;
}

int KeyframeCreateItem::handle_event()
{
	MWindow *mwindow = keyframe_create->mwindow;
	int mode = mwindow->edl->local_session->floatauto_type;
	int mask = this->mask;
	if( !mask ) { // visible
		int *autos = mwindow->edl->session->auto_conf->autos;
		int modes = (1<<AUTOMATION_FADE) + (1<<AUTOMATION_SPEED) + 
			(7<<AUTOMATION_CAMERA_X) + (7<<AUTOMATION_PROJECTOR_X);
		for( int i=0; i<AUTOMATION_TOTAL; modes>>=1, ++i ) {
			if( !(modes & 1) ) continue;
			if( autos[i] ) mask |= (1<<i);
		}
	}
	mwindow->create_keyframes(mask, mode);
	return 1;
}

KeyframeCreateMenu::KeyframeCreateMenu(KeyframeCreate *keyframe_create)
: BC_SubMenu()
{
	this->keyframe_create = keyframe_create;
}

KeyframeCreate::KeyframeCreate(MWindow *mwindow)
 : BC_MenuItem(_("Create keyframes..."))
{
	this->mwindow = mwindow;
	keyframe_create_menu = 0;
}

void KeyframeCreate::create_objects()
{
	add_submenu(keyframe_create_menu = new KeyframeCreateMenu(this));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Visible"), 0));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Fade"),
				(1<<AUTOMATION_FADE)));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Speed"),
				(1<<AUTOMATION_SPEED)));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Camera XYZ"),
				(7<<AUTOMATION_CAMERA_X)));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Projector XYZ"),
				(7<<AUTOMATION_PROJECTOR_X)));
	keyframe_create_menu->add_item(new KeyframeCreateItem(this, _("Fade+Speed+XYZ"),
				(1<<AUTOMATION_FADE) + (1<<AUTOMATION_SPEED) + 
				(7<<AUTOMATION_CAMERA_X) + (7<<AUTOMATION_PROJECTOR_X)));
}


CutDefaultKeyframe::CutDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Cut default keyframe"), _("Alt-x"), 'x')
{
	set_alt();
	this->mwindow = mwindow;
}

int CutDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut_default_keyframe();
	return 1;
}

CopyDefaultKeyframe::CopyDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Copy default keyframe"), _("Alt-c"), 'c')
{
	set_alt();
	this->mwindow = mwindow;
}

int CopyDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy_default_keyframe();
	return 1;
}

PasteDefaultKeyframe::PasteDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Paste default keyframe"), _("Alt-v"), 'v')
{
	set_alt();
	this->mwindow = mwindow;
}

int PasteDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_default_keyframe();
	return 1;
}

ClearDefaultKeyframe::ClearDefaultKeyframe(MWindow *mwindow)
 : BC_MenuItem(_("Clear default keyframe"), _("Alt-Del"), DELETE)
{
	set_alt();
	this->mwindow = mwindow;
}

int ClearDefaultKeyframe::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->clear_default_keyframe();
	return 1;
}

Cut::Cut(MWindow *mwindow)
 : BC_MenuItem(_("Split | Cut"), "x", 'x')
{
	this->mwindow = mwindow;
}

int Cut::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->cut();
	return 1;
}

Copy::Copy(MWindow *mwindow)
 : BC_MenuItem(_("Copy"), "c", 'c')
{
	this->mwindow = mwindow;
}

int Copy::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->copy();
	return 1;
}

Paste::Paste(MWindow *mwindow)
 : BC_MenuItem(_("Paste"), "v", 'v')
{
	this->mwindow = mwindow;
}

int Paste::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste();
	return 1;
}

EditClearSubMenu::EditClearSubMenu(BC_MenuItem *menu_item)
 : BC_SubMenu()
{
	this->menu_item = menu_item;
}
EditClearSubMenu::~EditClearSubMenu()
{
}

EditClearMenu::EditClearMenu(MWindow *mwindow)
 : BC_MenuItem(_("Clear..."))
{
	this->mwindow = mwindow;
	this->clear_sub_menu = 0;
}
EditClearMenu::~EditClearMenu()
{
}

void EditClearMenu::create_objects()
{
	add_submenu(clear_sub_menu = new EditClearSubMenu(this));
	clear_sub_menu->add_item(new Clear(mwindow));
	clear_sub_menu->add_item(new MuteSelection(mwindow));
	clear_sub_menu->add_item(new ClearSelect(mwindow));
	clear_sub_menu->add_item(new ClearLabels(mwindow));
	clear_sub_menu->add_item(new ClearHardEdges(mwindow));
};

Clear::Clear(MWindow *mwindow)
 : BC_MenuItem(_("Clear"), _("Del"), DELETE)
{
	this->mwindow = mwindow;
}

int Clear::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->clear_entry();
	}
	return 1;
}

PasteSilence::PasteSilence(MWindow *mwindow)
 : BC_MenuItem(_("Paste silence"), _("Shift-Space"), ' ')
{
	this->mwindow = mwindow;
	set_shift();
}

int PasteSilence::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_silence();
	return 1;
}

SelectAll::SelectAll(MWindow *mwindow)
 : BC_MenuItem(_("Select All"), "a", 'a')
{
	this->mwindow = mwindow;
}

int SelectAll::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->select_all();
	return 1;
}

ClearHardEdges::ClearHardEdges(MWindow *mwindow) : BC_MenuItem(_("Clear Hard Edges"))
{
	this->mwindow = mwindow;
}

int ClearHardEdges::handle_event()
{
	mwindow->clear_hard_edges();
	return 1;
}

ClearLabels::ClearLabels(MWindow *mwindow) : BC_MenuItem(_("Clear labels"))
{
	this->mwindow = mwindow;
}

int ClearLabels::handle_event()
{
	mwindow->clear_labels();
	return 1;
}

ClearSelect::ClearSelect(MWindow *mwindow) : BC_MenuItem(_("Clear Select"),"Ctrl-Shift-A",'A')
{
	set_ctrl(1);
	set_shift(1);
	this->mwindow = mwindow;
}

int ClearSelect::handle_event()
{
	mwindow->clear_select();
	return 1;
}

CutCommercials::CutCommercials(MWindow *mwindow) : BC_MenuItem(_("Cut ads"))
{
	this->mwindow = mwindow;
}

int CutCommercials::handle_event()
{
	mwindow->cut_commercials();
	return 1;
}

DetachTransitions::DetachTransitions(MWindow *mwindow)
 : BC_MenuItem(_("Detach transitions"))
{
	this->mwindow = mwindow;
}

int DetachTransitions::handle_event()
{
	mwindow->detach_transitions();
	return 1;
}

MuteSelection::MuteSelection(MWindow *mwindow)
 : BC_MenuItem(_("Mute Region"), "m", 'm')
{
	this->mwindow = mwindow;
}

int MuteSelection::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->mute_selection();
	return 1;
}


TrimSelection::TrimSelection(MWindow *mwindow)
 : BC_MenuItem(_("Trim Selection"))
{
	this->mwindow = mwindow;
}

int TrimSelection::handle_event()
{
	mwindow->trim_selection();
	return 1;
}












// ============================================= audio

AddAudioTrack::AddAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), "t", 't')
{
	this->mwindow = mwindow;
}

int AddAudioTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_audio_track_entry(0, 0);
	return 1;
}

DeleteAudioTrack::DeleteAudioTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteAudioTrack::handle_event()
{
	return 1;
}

DefaultATransition::DefaultATransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), "u", 'u')
{
	this->mwindow = mwindow;
}

int DefaultATransition::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_audio_transition();
	return 1;
}


MapAudio1::MapAudio1(MWindow *mwindow)
 : BC_MenuItem(_("Map 1:1"))
{
	this->mwindow = mwindow;
}

int MapAudio1::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_1_TO_1);
	return 1;
}

MapAudio2::MapAudio2(MWindow *mwindow)
 : BC_MenuItem(_("Map 5.1:2"))
{
	this->mwindow = mwindow;
}

int MapAudio2::handle_event()
{
	mwindow->map_audio(MWindow::AUDIO_5_1_TO_2);
	return 1;
}




// ============================================= video


AddVideoTrack::AddVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add track"), _("Shift-T"), 'T')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddVideoTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_video_track_entry(1, 0);
	return 1;
}


DeleteVideoTrack::DeleteVideoTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
}

int DeleteVideoTrack::handle_event()
{
	return 1;
}



ResetTranslation::ResetTranslation(MWindow *mwindow)
 : BC_MenuItem(_("Reset Translation"))
{
	this->mwindow = mwindow;
}

int ResetTranslation::handle_event()
{
	return 1;
}



DefaultVTransition::DefaultVTransition(MWindow *mwindow)
 : BC_MenuItem(_("Default Transition"), _("Shift-U"), 'U')
{
	set_shift();
	this->mwindow = mwindow;
}

int DefaultVTransition::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_video_transition();
	return 1;
}














// ============================================ settings

DeleteTracks::DeleteTracks(MWindow *mwindow)
 : BC_MenuItem(_("Delete tracks"))
{
	this->mwindow = mwindow;
}

int DeleteTracks::handle_event()
{
	mwindow->delete_tracks();
	return 1;
}

DeleteFirstTrack::DeleteFirstTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete first track"), "Shift-D", 'D')
{
	set_shift(1);
	this->mwindow = mwindow;
}

int DeleteFirstTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		Track *track = mwindow->edl->tracks->first;
		if( track ) mwindow->delete_track(track);
	}
	return 1;
}

DeleteLastTrack::DeleteLastTrack(MWindow *mwindow)
 : BC_MenuItem(_("Delete last track"), "Ctrl-d", 'd')
{
	set_ctrl(1);
	this->mwindow = mwindow;
}

int DeleteLastTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		Track *track = mwindow->edl->tracks->last;
		if( track ) mwindow->delete_track(track);
	}
	return 1;
}

MoveTracksUp::MoveTracksUp(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks up"), _("Shift-Up"), UP)
{
	this->mwindow = mwindow;
	set_shift();
}

int MoveTracksUp::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->move_tracks_up();
	return 1;
}

MoveTracksDown::MoveTracksDown(MWindow *mwindow)
 : BC_MenuItem(_("Move tracks down"), _("Shift-Down"), DOWN)
{
	this->mwindow = mwindow;
	set_shift();
}

int MoveTracksDown::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->move_tracks_down();
	return 1;
}


RollTracksUp::RollTracksUp(MWindow *mwindow)
 : BC_MenuItem(_("Roll tracks up"), _("Ctrl-Shift-Up"), UP)
{
	this->mwindow = mwindow;
	set_ctrl();
	set_shift();
}

int RollTracksUp::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->roll_tracks_up();
	return 1;
}

RollTracksDown::RollTracksDown(MWindow *mwindow)
 : BC_MenuItem(_("Roll tracks down"), _("Ctrl-Shift-Down"), DOWN)
{
	this->mwindow = mwindow;
	set_ctrl();
	set_shift();
}

int RollTracksDown::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->roll_tracks_down();
	return 1;
}




ConcatenateTracks::ConcatenateTracks(MWindow *mwindow)
 : BC_MenuItem(_("Concatenate tracks"))
{
	set_shift();
	this->mwindow = mwindow;
}

int ConcatenateTracks::handle_event()
{
	mwindow->concatenate_tracks();
	return 1;
}





LoopPlayback::LoopPlayback(MWindow *mwindow)
 : BC_MenuItem(_("Loop Playback"), _("Shift-L"), 'L')
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->local_session->loop_playback);
	set_shift();
}

int LoopPlayback::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->toggle_loop_playback();
		set_checked(mwindow->edl->local_session->loop_playback);
	}
	return 1;
}



// ============================================= subtitle


AddSubttlTrack::AddSubttlTrack(MWindow *mwindow)
 : BC_MenuItem(_("Add subttl"), _("Shift-Y"), 'Y')
{
	set_shift();
	this->mwindow = mwindow;
}

int AddSubttlTrack::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->add_subttl_track_entry(1, 0);
	return 1;
}

PasteSubttl::PasteSubttl(MWindow *mwindow)
 : BC_MenuItem(_("paste subttl"), "y", 'y')
{
	this->mwindow = mwindow;
}

int PasteSubttl::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->gui->swindow->paste_subttl();
	return 1;
}


SetBRenderActive::SetBRenderActive(MWindow *mwindow)
 : BC_MenuItem(_("Toggle background rendering"),_("Shift-G"),'G')
{
	this->mwindow = mwindow;
	set_shift(1);
}

int SetBRenderActive::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		int v = mwindow->brender_active ? 0 : 1;
		set_checked(v);
		mwindow->set_brender_active(v);
	}
	return 1;
}


LabelsFollowEdits::LabelsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit labels"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->labels_follow_edits);
}

int LabelsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->set_labels_follow_edits(get_checked());
	return 1;
}




PluginsFollowEdits::PluginsFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Edit effects"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->plugins_follow_edits);
}

int PluginsFollowEdits::handle_event()
{
	set_checked(get_checked() ^ 1);
	mwindow->edl->session->plugins_follow_edits = get_checked();
	return 1;
}




KeyframesFollowEdits::KeyframesFollowEdits(MWindow *mwindow)
 : BC_MenuItem(_("Keyframes follow edits"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->autos_follow_edits);
}

int KeyframesFollowEdits::handle_event()
{
	mwindow->edl->session->autos_follow_edits ^= 1;
	set_checked(!get_checked());
	return 1;
}


CursorOnFrames::CursorOnFrames(MWindow *mwindow)
 : BC_MenuItem(_("Align cursor on frames"),_("Ctrl-a"),'a')
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->cursor_on_frames);
	set_ctrl(1);
}

int CursorOnFrames::handle_event()
{
	mwindow->edl->session->cursor_on_frames = !mwindow->edl->session->cursor_on_frames;
	set_checked(mwindow->edl->session->cursor_on_frames);
	return 1;
}


TypelessKeyframes::TypelessKeyframes(MWindow *mwindow)
 : BC_MenuItem(_("Typeless keyframes"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->edl->session->typeless_keyframes);
}

int TypelessKeyframes::handle_event()
{
	mwindow->edl->session->typeless_keyframes = !mwindow->edl->session->typeless_keyframes;
	set_checked(mwindow->edl->session->typeless_keyframes);
	return 1;
}


ScrubSpeed::ScrubSpeed(MWindow *mwindow)
 : BC_MenuItem(_("Slow Shuttle"))
{
	this->mwindow = mwindow;
}

int ScrubSpeed::handle_event()
{
	if(mwindow->edl->session->scrub_speed == .5)
	{
		mwindow->edl->session->scrub_speed = 2;
		set_text(_("Slow Shuttle"));
	}
	else
	{
		mwindow->edl->session->scrub_speed = .5;
		set_text(_("Fast Shuttle"));
	}
	return 1;
}

SaveSettingsNow::SaveSettingsNow(MWindow *mwindow)
 : BC_MenuItem(_("Save settings now"),_("Ctrl-Shift-S"),'S')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_shift(1);
}

int SaveSettingsNow::handle_event()
{
	mwindow->save_defaults();
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved settings."));
	return 1;
}



// ============================================ window





ShowVWindow::ShowVWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Viewer"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_vwindow);
}
int ShowVWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->gui->unlock_window();
		if( !mwindow->session->show_vwindow )
			mwindow->show_vwindow(1);
		else
			mwindow->hide_vwindow(1);
		mwindow->gui->lock_window("ShowVWindow::handle_event");
		set_checked(mwindow->session->show_vwindow);
	}
	return 1;
}

ShowAWindow::ShowAWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Resources"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_awindow);
}
int ShowAWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->gui->unlock_window();
		if( !mwindow->session->show_awindow )
			mwindow->show_awindow();
		else
			mwindow->hide_awindow();
		mwindow->gui->lock_window("ShowAWindow::handle_event");
		set_checked(mwindow->session->show_awindow);

	}
	return 1;
}

ShowCWindow::ShowCWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Compositor"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_cwindow);
}
int ShowCWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->gui->unlock_window();
		if( !mwindow->session->show_cwindow )
			mwindow->show_cwindow();
		else
			mwindow->hide_cwindow();
		mwindow->gui->lock_window("ShowCWindow::handle_event");
		set_checked(mwindow->session->show_cwindow);
	}
	return 1;
}


ShowGWindow::ShowGWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Overlays"), _("Ctrl-0"), '0')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->session->show_gwindow);
}
int ShowGWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		mwindow->gui->unlock_window();
		if( !mwindow->session->show_gwindow )
			mwindow->show_gwindow();
		else
			mwindow->hide_gwindow();
		mwindow->gui->lock_window("ShowGWindow::handle_event");
		set_checked(mwindow->session->show_gwindow);
	}
	return 1;
}


ShowLWindow::ShowLWindow(MWindow *mwindow)
 : BC_MenuItem(_("Show Levels"))
{
	this->mwindow = mwindow;
	set_checked(mwindow->session->show_lwindow);
}
int ShowLWindow::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {

		mwindow->gui->unlock_window();
		if( !mwindow->session->show_lwindow )
			mwindow->show_lwindow();
		else
			mwindow->hide_lwindow();
		mwindow->gui->lock_window("ShowLWindow::handle_event");
		set_checked(mwindow->session->show_lwindow);
	}
	return 1;
}

TileWindows::TileWindows(MWindow *mwindow, const char *item_title, int config,
		const char *hot_keytext, int hot_key)
 : BC_MenuItem(item_title, hot_keytext, hot_key)
{
	this->mwindow = mwindow;
	this->config = config;
	if( hot_key ) set_ctrl(1);
}
int TileWindows::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION ) {
		int window_config = config >= 0 ? config :
			mwindow->session->window_config;
		if( mwindow->tile_windows(window_config) ) {
			mwindow->restart_status = 1;
			mwindow->gui->set_done(0);
		}
	}
	return 1;
}

SplitX::SplitX(MWindow *mwindow)
 : BC_MenuItem(_("Split X pane"), _("Ctrl-1"), '1')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->gui->pane[TOP_RIGHT_PANE] != 0);
}
int SplitX::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->split_x();
	return 1;
}

SplitY::SplitY(MWindow *mwindow)
 : BC_MenuItem(_("Split Y pane"), _("Ctrl-2"), '2')
{
	this->mwindow = mwindow;
	set_ctrl(1);
	set_checked(mwindow->gui->pane[BOTTOM_LEFT_PANE] != 0);
}
int SplitY::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->split_y();
	return 1;
}


MixerItems::MixerItems(MWindow *mwindow)
 : BC_MenuItem(_("Mixers..."))
{
	this->mwindow = mwindow;
}

void MixerItems::create_objects()
{
	BC_SubMenu *mixer_submenu = new BC_SubMenu();
	add_submenu(mixer_submenu);
	mixer_submenu->add_submenuitem(new MixerViewer(this));
	mixer_submenu->add_submenuitem(new DragTileMixers(this));
	mixer_submenu->add_submenuitem(new AlignMixers(this));
	mixer_submenu->add_submenuitem(new MixMasters(this));
}

int MixerItems::activate_submenu()
{
	BC_SubMenu *mixer_submenu = (BC_SubMenu *)get_submenu();
	int k = mixer_submenu->total_items();
	while( --k >= 0 ) {
		MixerItem *mixer_item = (MixerItem *)mixer_submenu->get_item(k);
		if( mixer_item->idx < 0 ) continue;
		mixer_submenu->del_item(mixer_item);
	}
	int n = mwindow->edl->mixers.size();
	for( int i=0; i<n; ++i ) {
		Mixer *mixer = mwindow->edl->mixers[i];
		if( !mixer ) continue;
		MixerItem *mixer_item = new MixerItem(this, mixer->title, mixer->idx);
		mixer_submenu->add_submenuitem(mixer_item);
	}
	return BC_MenuItem::activate_submenu();
}

MixerItem::MixerItem(MixerItems *mixer_items, const char *text, int idx)
 : BC_MenuItem(text)
{
	this->mixer_items = mixer_items;
	this->idx = idx;
}

MixerItem::MixerItem(MixerItems *mixer_items, const char *text, const char *hotkey_text, int hotkey)
 : BC_MenuItem(text, hotkey_text, hotkey)
{
	this->mixer_items = mixer_items;
	this->idx = -1;
}

int MixerItem::handle_event()
{
	if( idx < 0 ) return 0;
	MWindow *mwindow = mixer_items->mwindow;
	Mixer *mixer = mwindow->edl->mixers.get_mixer(idx);
	if( !mixer ) return 0;
	ZWindow *zwindow = mwindow->get_mixer(idx);
	if( !zwindow )
		zwindow = mwindow->get_mixer(mixer);
	if( !zwindow->zgui ) {
		zwindow->set_title(mixer->title);
		zwindow->start();
	}
	zwindow->zgui->lock_window("MixerItem::handle_event");
	zwindow->zgui->raise_window();
	zwindow->zgui->unlock_window();
	mwindow->refresh_mixers();
	return 1;
}

MixerViewer::MixerViewer(MixerItems *mixer_items)
 : MixerItem(mixer_items, _("Mixer Viewer"), _("Shift-M"), 'M')
{
	set_shift(1);
}

int MixerViewer::handle_event()
{
	MWindow *mwindow = mixer_items->mwindow;
	mwindow->start_mixer();
	return 1;
}

DragTileMixers::DragTileMixers(MixerItems *mixer_items)
 : MixerItem(mixer_items, _("Drag Tile mixers"), "Alt-t", 't')
{
	set_alt();
	drag_box = 0;
}

DragTileMixers::~DragTileMixers()
{
	delete drag_box;
}

int DragTileMixers::handle_event()
{
	if( !drag_box ) {
		MWindow *mwindow = mixer_items->mwindow;
		drag_box = new TileMixersDragBox(mwindow->gui);
	}
	if( !drag_box->running() )
		drag_box->start(this);
	return 1;
}

TileMixersDragBox::TileMixersDragBox(MWindowGUI *gui)
 : BC_DragBox(gui)
{
	tile_mixers = 0;
}

void TileMixersDragBox::start(DragTileMixers *tile_mixers)
{
	this->tile_mixers = tile_mixers;
	start_drag();
}

int TileMixersDragBox::handle_done_event(int x0, int y0, int x1, int y1)
{
	MWindow *mwindow = tile_mixers->mixer_items->mwindow;
	if( x0 >= x1 || y0 >= y1 ) x0 = x1 = y0 = y1 = 0;
	mwindow->session->tile_mixers_x = x0;
	mwindow->session->tile_mixers_y = y0;
	mwindow->session->tile_mixers_w = x1 - x0;
	mwindow->session->tile_mixers_h = y1 - y0;
	mwindow->tile_mixers(x0, y0, x1, y1);
	tile_mixers = 0;
	return 1;
}

AlignMixers::AlignMixers(MixerItems *mixer_items)
 : MixerItem(mixer_items, _("Align mixers"), "", 0)
{
}

int AlignMixers::handle_event()
{
	MWindow *mwindow = mixer_items->mwindow;
	int wx, wy;
	mwindow->gui->get_abs_cursor(wx, wy);
	mwindow->mixers_align->start_dialog(wx, wy);
	return 1;
}

MixMasters::MixMasters(MixerItems *mixer_items)
 : MixerItem(mixer_items, _("Mix masters"), "", 0)
{
}

int MixMasters::handle_event()
{
	MWindow *mwindow = mixer_items->mwindow;
	mwindow->mix_masters();
	return 1;
}


AlignTimecodes::AlignTimecodes(MWindow *mwindow)
 : BC_MenuItem(_("Align Timecodes"))
{
	this->mwindow = mwindow;
}

int AlignTimecodes::handle_event()
{
	mwindow->align_timecodes();
	return 1;
}


LoadLayoutItem::LoadLayoutItem(LoadLayout *load_layout, const char *text, int idx, int hotkey)
 : BC_MenuItem(text, "", hotkey)
{
	this->idx = idx;
	this->load_layout = load_layout;
	if( hotkey ) {
		char hot_txt[BCSTRLEN];
		sprintf(hot_txt, _("Ctrl-Shift+F%d"), hotkey-KEY_F1+1);
		set_ctrl();  set_shift();
		set_hotkey_text(hot_txt);
	}
}


int LoadLayoutItem::handle_event()
{
// key_press hotkey skips over activate_submenu
	load_layout->update();
	MWindow *mwindow = load_layout->mwindow;
	switch( load_layout->action ) {
	case LAYOUT_LOAD:
		mwindow->load_layout(layout_file);
		break;
	case LAYOUT_SAVE: {
		int wx = 0, wy = 0;
		mwindow->gui->get_abs_cursor(wx, wy);
		load_layout->layout_dialog->start_confirm_dialog(wx, wy, idx);
		break; }
	}
	return 1;
}

LoadLayout::LoadLayout(MWindow *mwindow, const char *text, int action)
 : BC_MenuItem(text)
{
	this->mwindow = mwindow;
	this->action = action;
	this->layout_dialog = new LoadLayoutDialog(this);
}

LoadLayout::~LoadLayout()
{
	delete layout_dialog;
}

void LoadLayout::create_objects()
{
	BC_SubMenu *layout_submenu = new BC_SubMenu();
	add_submenu(layout_submenu);

	for( int i=0; i<LAYOUTS_MAX; ++i ) {
		char text[BCSTRLEN];
		sprintf(text, _("Layout %d"), i+1);
		LoadLayoutItem *item = new LoadLayoutItem(this, text, i,
				action==LAYOUT_LOAD ? KEY_F1+i : 0);
		layout_submenu->add_submenuitem(item);
	}
}

int LoadLayout::activate_submenu()
{
	update();
	return BC_MenuItem::activate_submenu();
}

void LoadLayout::update()
{
	FileSystem fs;
	fs.set_filter("layout*_rc");
	int ret = fs.update(File::get_config_path());
	int sz = !ret ? fs.dir_list.size() : 0;
	BC_SubMenu *layout_submenu = get_submenu();

	for( int i=0; i<LAYOUTS_MAX; ++i ) {
		LoadLayoutItem* item = (LoadLayoutItem *)
			layout_submenu->get_item(i);
		char layout_text[BCSTRLEN];  layout_text[0] = 0;
		int n = sz, id = i+1;
		while( --n >= 0 ) {
			char *np = fs.dir_list[n]->name;
			char *cp = strrchr(np, '_'), *bp = 0;
			int no = strtol(np+6, &bp, 10);
			if( no != id || !bp ) continue;
			if( bp == cp ) {  n = -1;  break; }
			if( *bp++ == '_' && bp < cp && !strcmp(cp, "_rc") ) {
				int k = cp - bp;  char *tp = layout_text;
				if( k > LAYOUT_NAME_LEN ) k = LAYOUT_NAME_LEN;
				while( --k >= 0 ) *tp++ = *bp++;
				*tp = 0;
				break;
			}
		}
		strcpy(item->layout_text, layout_text);
		char *lp = item->layout_file;
		int k = sprintf(lp, LAYOUT_FILE, id);
		if( n >= 0 && layout_text[0] )
			sprintf(lp + k-2, "%s_rc", layout_text);
		else
			sprintf(layout_text, _("Layout %d"), id);
		item->set_text(layout_text);
	}
}

LoadLayoutDialog::LoadLayoutDialog(LoadLayout *load_layout)
{
	this->load_layout = load_layout;
	wx = 0;  wy = 0;
	idx = -1;
	lgui = 0;
}

LoadLayoutDialog::~LoadLayoutDialog()
{
	close_window();
}

void LoadLayoutDialog::handle_done_event(int result)
{
	if( result ) return;
	char layout_file[BCSTRLEN];
	BC_SubMenu *layout_submenu = load_layout->get_submenu();
	LoadLayoutItem* item =
		(LoadLayoutItem *) layout_submenu->get_item(idx);
	snprintf(layout_file, sizeof(layout_file), "%s", item->layout_file);
	load_layout->mwindow->delete_layout(layout_file);
	int k = sprintf(layout_file, LAYOUT_FILE, idx+1);
	const char *text = lgui->name_text->get_text();
	if( text[0] )
		snprintf(layout_file + k-2, sizeof(layout_file)-k+2, "%s_rc", text);
	load_layout->mwindow->save_layout(layout_file);
}

void LoadLayoutDialog::handle_close_event(int result)
{
	lgui = 0;
}

BC_Window *LoadLayoutDialog::new_gui()
{
	lgui = new LoadLayoutConfirm(this, wx, wy);
	lgui->create_objects();
	return lgui;
}

void LoadLayoutDialog::start_confirm_dialog(int wx, int wy, int idx)
{
	close_window();
	this->wx = wx;  this->wy = wy;
	this->idx = idx;
	start();
}

LoadLayoutNameText::LoadLayoutNameText(LoadLayoutConfirm *confirm,
		int x, int y, int w, const char *text)
 : BC_TextBox(x, y, w, 1, text)
{
	this->confirm = confirm;
}

LoadLayoutNameText::~LoadLayoutNameText()
{
}

int LoadLayoutNameText::handle_event()
{
	const char *text = get_text();
	int len = strlen(text), k = 0;
	char new_text[BCTEXTLEN];
	for( int i=0; i<len; ++i ) {
		int ch = text[i];
		if( (ch>='A' && ch<='Z') || (ch>='a' && ch<='z') ||
		    (ch>='0' && ch<='9') || ch=='_' )
			new_text[k++] = ch;
	}
	new_text[k] = 0;  len = k;
	int i = len - LAYOUT_NAME_LEN;
	if( i >= 0 ) {
		k = 0;
		while( i < len ) new_text[k++] = new_text[i++];
		new_text[k] = 0;
	}
	update(new_text);
	return 1;
}

LoadLayoutConfirm::LoadLayoutConfirm(LoadLayoutDialog *layout_dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Layout"), x, y, xS(300),yS(140), xS(300),yS(140), 0)
{
	this->layout_dialog = layout_dialog;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Window Layouts");
}

LoadLayoutConfirm::~LoadLayoutConfirm()
{
}

void LoadLayoutConfirm::create_objects()
{
	int xs10 = xS(10), xs20 = xS(20);
	int ys10 = yS(10);
	lock_window("LoadLayoutConfirm::create_objects");
	int x = xs10, y = ys10;
	BC_SubMenu *layout_submenu = layout_dialog->load_layout->get_submenu();
	LoadLayoutItem *item = (LoadLayoutItem *)
		layout_submenu->get_item(layout_dialog->idx);
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Layout Name:")));
	int x1 = x + title->get_w() + xs10;
	add_subwindow(title = new BC_Title(x1, y, item->get_text()));
	y += title->get_h() + ys10;
	add_subwindow(name_text = new LoadLayoutNameText(this,
		x, y, get_w()-x-xs20, item->layout_text));
	y += name_text->get_h();
	x1 = x + xS(80);
	char legend[BCTEXTLEN];
	sprintf(legend, _("a-z,A-Z,0-9_ only, %dch max"), LAYOUT_NAME_LEN);
	add_subwindow(title = new BC_Title(x1, y, legend));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	unlock_window();
}


LoadRecentItem::LoadRecentItem(const char *path)
{
	this->path = cstrdup(path);
}

LoadRecentItem::~LoadRecentItem()
{
	delete [] path;
}

int LoadRecentItems::add_load(char *path)
{
// test for existing copy
	FileSystem fs;
	char name[BCTEXTLEN], text[BCTEXTLEN];
	fs.extract_name(name, path);
	int loads_total = size();
	int ret = 0, k = loads_total;
	LoadRecentItem *load_item = 0;

	for( int i=0; !ret && i<loads_total; ++i ) {
		load_item = get(i);
		fs.extract_name(text, load_item->path);
		if( strcmp(name, text) ) continue;
		k = i;  ret = 1; // already exists, move to top
	}
	if( !ret ) { // adding a new one
		while( loads_total >= TOTAL_LOADS )
			remove_object_number(--loads_total);
		insert(new LoadRecentItem(path), 0);
	}
	else if( k > 0 ) { // cycle loads
		while( --k >= 0 ) set(k+1, get(k));
		set(0, load_item);
	}
	return ret;
}

LoadRecentItems::LoadRecentItems()
{
}

LoadRecentItems::~LoadRecentItems()
{
	remove_all_objects();
}

LoadRecent::LoadRecent(MWindow *mwindow, MainMenu *main_menu)
 : BC_MenuItem(_("Load Recent..."))
{
	this->mwindow = mwindow;
	this->main_menu = main_menu;
	total_items = 0;
}
LoadRecent::~LoadRecent()
{
}

void LoadRecent::create_objects()
{
	add_submenu(submenu = new LoadRecentSubMenu(this));
}

LoadPrevious *LoadRecent::get_next_item()
{
	int k = total_items++;
	if( k < submenu->total_items() )
		return (LoadPrevious *)submenu->get_item(k);
	LoadPrevious *load_prev = new LoadPrevious(mwindow, main_menu->load_file);
	submenu->add_item(load_prev);
	return load_prev;
}

int LoadRecent::activate_submenu()
{
	total_items = 0;
	scan_items(1);
	if( total_items > 0 ) {
		LoadPrevious *load_prev = get_next_item();
		load_prev->set_text("-");
		load_prev->set_path("");
	}
	scan_items(0);
	while( total_items < submenu->total_items() )
		submenu->del_item(0);
	return BC_MenuItem::activate_submenu();
}

void LoadRecent::scan_items(int use_xml)
{
	FileSystem fs;
	int loads_total = main_menu->load.size();
	for( int i=0; i<loads_total; ++i ) {
		LoadRecentItem *recent = main_menu->load[i];
		char name[BCTEXTLEN];
		fs.extract_name(name, recent->path);
		const char *cp = strrchr(name, '.');
		if( !cp || strcasecmp(cp+1,"xml") ? use_xml : !use_xml ) continue;
		LoadPrevious *load_prev = get_next_item();
		load_prev->set_text(name);
		load_prev->set_path(recent->path);
	}
}

LoadRecentSubMenu::LoadRecentSubMenu(LoadRecent *load_recent)
 : BC_SubMenu()
{
	this->load_recent = load_recent;
}

LoadRecentSubMenu::~LoadRecentSubMenu()
{
}

