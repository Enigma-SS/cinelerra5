
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "awindow.h"
#include "awindowgui.h"
#include "clip.h"
#include "confirmsave.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "formatcheck.h"
#include "indexfile.h"
#include "keyframe.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "menueffects.h"
#include "playbackengine.h"
#include "pluginarray.h"
#include "pluginserver.h"
#include "preferences.h"
#include "render.h"
#include "sighandler.h"
#include "theme.h"
#include "tracks.h"



MenuEffects::MenuEffects(MWindow *mwindow)
 : BC_MenuItem(_("Render effect..."))
{
	this->mwindow = mwindow;
}

MenuEffects::~MenuEffects()
{
}


int MenuEffects::handle_event()
{
	thread->set_title("");
	thread->start();
	return 1;
}





MenuEffectPacket::MenuEffectPacket(char *path, int64_t start, int64_t end)
{
	this->start = start;
	this->end = end;
	strcpy(this->path, path);
}

MenuEffectPacket::~MenuEffectPacket()
{
}






MenuEffectThread::MenuEffectThread(MWindow *mwindow, MenuEffects *menu_item)
{
	this->mwindow = mwindow;
	this->menu_item = menu_item;
	title[0] = 0;
	dead_plugins = new ArrayList<PluginServer*>;
}

MenuEffectThread::~MenuEffectThread()
{
	delete dead_plugins;
}





int MenuEffectThread::set_title(const char *title)
{
	strcpy(this->title, title);
	return 0;
}

// for recent effect menu items and running new effects
// prompts for an effect if title is blank
void MenuEffectThread::run()
{
	for(int i = 0; i < dead_plugins->size(); i++)
	{
		delete dead_plugins->get(i);
	}
	dead_plugins->remove_all();




// get stuff from main window
	ArrayList<PluginServer*> *plugindb = mwindow->plugindb;
	BC_Hash *defaults = mwindow->defaults;
	ArrayList<BC_ListBoxItem*> plugin_list;
	ArrayList<PluginServer*> local_plugindb;
	char string[1024];
	int result = 0;
// Default configuration
	Asset *default_asset = new Asset;
// Output
	ArrayList<Indexable*> assets;


// check for recordable tracks
	if(!get_recordable_tracks(default_asset))
	{
		sprintf(string, _("No recordable tracks specified."));
		ErrorBox error(_(PROGRAM_NAME ": Error"));
		error.create_objects(string);
		error.run_window();
		default_asset->Garbage::remove_user();
		return;
	}

// check for plugins
	if(!plugindb->total)
	{
		sprintf(string, _("No plugins available."));
		ErrorBox error(_(PROGRAM_NAME ": Error"));
		error.create_objects(string);
		error.run_window();
		default_asset->Garbage::remove_user();
		return;
	}


// get default attributes for output file
// used after completion
	get_derived_attributes(default_asset, defaults);
//	to_tracks = defaults->get("RENDER_EFFECT_TO_TRACKS", 1);
	load_mode = defaults->get("RENDER_EFFECT_LOADMODE", LOADMODE_PASTE);
	use_labels = defaults->get("RENDER_FILE_PER_LABEL", 0);

// get plugin information
	int need_plugin = !strlen(title) ? 1 : 0;
// generate a list of plugins for the window
	if( need_plugin ) {
		mwindow->search_plugindb(default_asset->audio_data,
			default_asset->video_data, -1, 0, 0, local_plugindb);
		for(int i = 0; i < local_plugindb.total; i++) {
			plugin_list.append(new BC_ListBoxItem(_(local_plugindb.values[i]->title)));
		}
	}

// find out which effect to run and get output file
	int plugin_number, format_error = 0;

	do
	{
		{
			MenuEffectWindow window(mwindow,
				this,
				need_plugin ? &plugin_list : 0,
				default_asset);
			window.create_objects();
			result = window.run_window();
			plugin_number = window.result;
		}

		if(!result)
		{
			FormatCheck format_check(default_asset);
			format_error = format_check.check_format();
		}
	}while(format_error && !result);

// save defaults
	save_derived_attributes(default_asset, defaults);
	defaults->update("RENDER_EFFECT_LOADMODE", load_mode);
	defaults->update("RENDER_EFFECT_FILE_PER_LABEL", use_labels);
	mwindow->save_defaults();

// get plugin server to use and delete the plugin list
	PluginServer *plugin_server = 0;
	PluginServer *plugin = 0;
	if(need_plugin)
	{
		plugin_list.remove_all_objects();
		if(plugin_number > -1)
		{
			plugin_server = local_plugindb.values[plugin_number];
			strcpy(title, plugin_server->title);
		}
	}
	else
	{
		for(int i = 0; i < plugindb->total && !plugin_server; i++)
		{
			if(!strcmp(plugindb->values[i]->title, title))
			{
				plugin_server = plugindb->values[i];
				plugin_number = i;
			}
		}
	}

// Update the  most recently used effects and copy the plugin server.
	if(plugin_server)
	{
		plugin = new PluginServer(*plugin_server);
		fix_menu(title);
	}

	if(!result && !strlen(default_asset->path))
	{
		result = 1;        // no output path given
		ErrorBox error(_(PROGRAM_NAME ": Error"));
		error.create_objects(_("No output file specified."));
		error.run_window();
	}

	if(!result && plugin_number < 0)
	{
		result = 1;        // no output path given
		ErrorBox error(_(PROGRAM_NAME ": Error"));
		error.create_objects(_("No effect selected."));
		error.run_window();
	}

// Configuration for realtime plugins.
	KeyFrame plugin_data;

// get selection to render
// Range
	double total_start, total_end;

	total_start = mwindow->edl->local_session->get_selectionstart();


	if(mwindow->edl->local_session->get_selectionend() ==
		mwindow->edl->local_session->get_selectionstart())
		total_end = mwindow->edl->tracks->total_length();
	else
		total_end = mwindow->edl->local_session->get_selectionend();



// get native units for range
	total_start = to_units(total_start, 0);
	total_end = to_units(total_end, 1);



// Trick boundaries in case of a non-realtime synthesis plugin
	if(plugin &&
		!plugin->realtime &&
		total_end == total_start) total_end = total_start + 1;

// Units are now in the track's units.
	int64_t total_length = (int64_t)total_end - (int64_t)total_start;
// length of output file

	if(!result && total_length <= 0)
	{
		result = 1;        // no output path given
		ErrorBox error(_(PROGRAM_NAME ": Error"));
		error.create_objects(_("No selected range to process."));
		error.run_window();
	}

// ========================= get keyframe from user
	if(!result)
	{
// ========================= realtime plugin
// no get_parameters
		if(plugin->realtime)
		{
// Open a prompt GUI
			MenuEffectPrompt prompt(mwindow);
			prompt.create_objects();
			char title[BCTEXTLEN];
			sprintf(title, _(PROGRAM_NAME ": %s"), plugin->title);

// Open the plugin GUI
			plugin->set_mwindow(mwindow);
			plugin->set_keyframe(&plugin_data);
			plugin->set_prompt(&prompt);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0);
// Must set parameters since there is no plugin object to draw from.
			plugin->get_parameters((int64_t)total_start,
				(int64_t)total_end,
				1);
			plugin->show_gui();

// wait for user input
			result = prompt.run_window();

// Close plugin.
			plugin->save_data(&plugin_data);
			plugin->hide_gui();

// Can't delete here.
			dead_plugins->append(plugin);
			default_asset->sample_rate = mwindow->edl->session->sample_rate;
			default_asset->frame_rate = mwindow->edl->session->frame_rate;
			realtime = 1;
		}
		else
// ============================non realtime plugin
		{
			plugin->set_mwindow(mwindow);
			plugin->open_plugin(0, mwindow->preferences, mwindow->edl, 0);
			result = plugin->get_parameters((int64_t)total_start,
				(int64_t)total_end,
				get_recordable_tracks(default_asset));
// some plugins can change the sample rate and the frame rate


			if(!result)
			{
				default_asset->sample_rate = plugin->get_samplerate();
				default_asset->frame_rate = plugin->get_framerate();
			}
			delete plugin;
			realtime = 0;
		}

// Should take from first recordable track
		default_asset->width = mwindow->edl->session->output_w;
		default_asset->height = mwindow->edl->session->output_h;
	}

	int range = File::is_image_render(default_asset->format) ?
		RANGE_1FRAME : RANGE_SELECTION;
	int strategy = Render::get_strategy(mwindow->preferences->use_renderfarm, use_labels, range);
// Process the total length in fragments
	ArrayList<MenuEffectPacket*> packets;
	if(!result)
	{
		Label *current_label = mwindow->edl->labels->first;
		mwindow->stop_brender();

		int current_number;
		int number_start;
		int total_digits;
		Render::get_starting_number(default_asset->path,
			current_number,
			number_start,
			total_digits);



// Construct all packets for single overwrite confirmation
		for(int64_t fragment_start = (int64_t)total_start, fragment_end;
			fragment_start < (int64_t)total_end;
			fragment_start = fragment_end)
		{
// Get fragment end
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
			{
				while(current_label  &&
					to_units(current_label->position, 0) <= fragment_start)
					current_label = current_label->next;
				if(!current_label)
					fragment_end = (int64_t)total_end;
				else
					fragment_end = to_units(current_label->position, 0);
			}
			else
			{
				fragment_end = (int64_t)total_end;
			}

// Get path
			char path[BCTEXTLEN];
			if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
				Render::create_filename(path,
					default_asset->path,
					current_number,
					total_digits,
					number_start);
			else
				strcpy(path, default_asset->path);
			current_number++;

			MenuEffectPacket *packet = new MenuEffectPacket(path,
				fragment_start,
				fragment_end);
			packets.append(packet);
		}


// Test existence of files
		ArrayList<char*> paths;
		for(int i = 0; i < packets.total; i++)
		{
			paths.append(packets.values[i]->path);
		}
		result = ConfirmSave::test_files(mwindow, &paths);
		paths.remove_all();
	}



	for(int current_packet = 0;
		current_packet < packets.total && !result;
		current_packet++)
	{
		Asset *asset = new Asset(*default_asset);
		MenuEffectPacket *packet = packets.values[current_packet];
		int64_t fragment_start = packet->start;
		int64_t fragment_end = packet->end;
		strcpy(asset->path, packet->path);

		assets.append(asset);
		File *file = new File;

// Open the output file after getting the information because the sample rate
// is needed here.
		if(!result)
		{
// open output file in write mode
			file->set_processors(mwindow->preferences->processors);
			if(file->open_file(mwindow->preferences,
				asset,
				0,
				1))
			{
// open failed
				sprintf(string, _("Couldn't open %s"), asset->path);
				ErrorBox error(_(PROGRAM_NAME ": Error"));
				error.create_objects(string);
				error.run_window();
				result = 1;
			}
			else
			{
				mwindow->sighandler->push_file(file);
				IndexFile::delete_index(mwindow->preferences,
					asset);
			}
		}

// run plugins
		if(!result)
		{
// position file
			PluginArray *plugin_array;
			plugin_array = create_plugin_array();

			plugin_array->start_plugins(mwindow,
				mwindow->edl,
				plugin_server,
				&plugin_data,
				fragment_start,
				fragment_end,
				file);
			plugin_array->run_plugins();

			plugin_array->stop_plugins();
			mwindow->sighandler->pull_file(file);
			file->close_file();
			asset->audio_length = file->asset->audio_length;
			asset->video_length = file->asset->video_length;
			delete plugin_array;
		}

		delete file;
	}

	packets.remove_all_objects();

// paste output to tracks
	if(!result && load_mode != LOADMODE_NOTHING)
	{
		mwindow->gui->lock_window("MenuEffectThread::run");

		mwindow->undo->update_undo_before("", 0);
		if(load_mode == LOADMODE_PASTE)
			mwindow->clear(0);

		mwindow->load_assets(&assets, -1, load_mode, 0, 0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->plugins_follow_edits,
			mwindow->edl->session->autos_follow_edits,
			0); // overwrite

		mwindow->save_backup();
		mwindow->undo->update_undo_after(title, LOAD_ALL);

		mwindow->restart_brender();
		mwindow->update_plugin_guis();
		mwindow->gui->update(1, FORCE_REDRAW, 1, 1, 1, 1, 0);
		mwindow->sync_parameters(CHANGE_ALL);
		mwindow->gui->unlock_window();

		mwindow->awindow->gui->async_update_assets();
	}

	for(int i = 0; i < assets.total; i++)
		assets.values[i]->Garbage::remove_user();
	assets.remove_all();

	default_asset->Garbage::remove_user();
}







MenuEffectItem::MenuEffectItem(MenuEffects *menueffect, const char *string)
 : BC_MenuItem(string)
{
	this->menueffect = menueffect;
}
int MenuEffectItem::handle_event()
{
	menueffect->thread->set_title(get_text());
	menueffect->thread->start();
	return 1;
}












MenuEffectWindow::MenuEffectWindow(MWindow *mwindow,
	MenuEffectThread *menueffects,
	ArrayList<BC_ListBoxItem*> *plugin_list,
	Asset *asset)
 : BC_Window(_(PROGRAM_NAME ": Render effect"),
		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1) - mwindow->session->menueffect_h / 2,
		mwindow->session->menueffect_w,
		mwindow->session->menueffect_h,
		xS(580), yS(350), 1, 0, 1)
{
	this->menueffects = menueffects;
	this->plugin_list = plugin_list;
	this->asset = asset;
	this->mwindow = mwindow;
	file_title = 0;
	format_tools = 0;
	loadmode = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Rendered Effects");
}

MenuEffectWindow::~MenuEffectWindow()
{
	lock_window("MenuEffectWindow::~MenuEffectWindow");
	delete format_tools;
	unlock_window();
}



void MenuEffectWindow::create_objects()
{
	int x, y;
	result = -1;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	lock_window("MenuEffectWindow::create_objects");
// only add the list if needed
	if(plugin_list)
	{
		add_subwindow(list_title = new BC_Title(mwindow->theme->menueffect_list_x,
			mwindow->theme->menueffect_list_y,
			_("Select an effect")));
		int ys5 = yS(5);
		add_subwindow(list = new MenuEffectWindowList(this,
			mwindow->theme->menueffect_list_x,
			mwindow->theme->menueffect_list_y + list_title->get_h() + ys5,
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - ys5,
			plugin_list));
	}

	add_subwindow(file_title = new BC_Title(
		mwindow->theme->menueffect_file_x,
		mwindow->theme->menueffect_file_y,
		(char*)(menueffects->use_labels ?
			_("Select the first file to render to:") :
			_("Select a file to render to:"))));

	x = mwindow->theme->menueffect_tools_x;
	y = mwindow->theme->menueffect_tools_y;
	format_tools = new FormatTools(mwindow,
					this,
					asset);
	format_tools->create_objects(x, y, asset->audio_data, asset->video_data,
		0, 0, 0, 1, 0, 0, &menueffects->use_labels, 0);

	loadmode = new LoadMode(mwindow, this, x, y, &menueffects->load_mode);
	loadmode->create_objects();

	add_subwindow(new MenuEffectWindowOK(this));
	add_subwindow(new MenuEffectWindowCancel(this));
	show_window();
	unlock_window();
}

int MenuEffectWindow::resize_event(int w, int h)
{
	mwindow->session->menueffect_w = w;
	mwindow->session->menueffect_h = h;
	mwindow->theme->get_menueffect_sizes(plugin_list ? 1 : 0);

	if(plugin_list)
	{
		list_title->reposition_window(mwindow->theme->menueffect_list_x,
			mwindow->theme->menueffect_list_y);
		int ys5 = yS(5);
		list->reposition_window(mwindow->theme->menueffect_list_x,
			mwindow->theme->menueffect_list_y + list_title->get_h() + ys5,
			mwindow->theme->menueffect_list_w,
			mwindow->theme->menueffect_list_h - list_title->get_h() - ys5);
	}

	if(file_title) file_title->reposition_window(mwindow->theme->menueffect_file_x,
		mwindow->theme->menueffect_file_y);
	int x = mwindow->theme->menueffect_tools_x;
	int y = mwindow->theme->menueffect_tools_y;
	if(format_tools) format_tools->reposition_window(x, y);
	if(loadmode) loadmode->reposition_window(x, y);
	return 0;
}



MenuEffectWindowOK::MenuEffectWindowOK(MenuEffectWindow *window)
 : BC_OKButton(window)
{
	this->window = window;
}

int MenuEffectWindowOK::handle_event()
{
	if(window->plugin_list)
		window->result = window->list->get_selection_number(0, 0);

	window->set_done(0);
	return 1;
}

int MenuEffectWindowOK::keypress_event()
{
	if(get_keypress() == RETURN)
	{
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

MenuEffectWindowCancel::MenuEffectWindowCancel(MenuEffectWindow *window)
 : BC_CancelButton(window)
{
	this->window = window;
}

int MenuEffectWindowCancel::handle_event()
{
	window->set_done(1);
	return 1;
}

int MenuEffectWindowCancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

MenuEffectWindowList::MenuEffectWindowList(MenuEffectWindow *window,
	int x,
	int y,
	int w,
	int h,
	ArrayList<BC_ListBoxItem*> *plugin_list)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, plugin_list)
{
	this->window = window;
}

int MenuEffectWindowList::handle_event()
{
	window->result = get_selection_number(0, 0);
	window->set_done(0);
	return 1;
}

// *** CONTEXT_HELP ***
int MenuEffectWindowList::keypress_event()
{
	int item;
	char title[BCTEXTLEN];

//	printf("MenuEffectWindowList::keypress_event: %d\n", get_keypress());

	// If not our context help keystroke, redispatch it
	// to the event handler of the base class
	if (get_keypress() != 'h' || ! alt_down() ||
	    ! is_tooltip_event_win() || ! cursor_inside())
		return BC_ListBox::keypress_event();

	// Try to show help for the plugin currently under mouse
	title[0] = '\0';
	item = get_highlighted_item();
	if (item >= 0 && item < window->plugin_list->total)
		strcpy(title, window->plugin_list->values[item]->get_text());

	// If some plugin is highlighted, show its help
	// Otherwise show more general help
	if (title[0]) {
		if (! strcmp(title, "Overlay")) {
			// "Overlay" plugin title is ambiguous
			if (window->asset->audio_data)
				strcat(title, " \\(Audio\\)");
			if (window->asset->video_data)
				strcat(title, " \\(Video\\)");
		}
		if (! strncmp(title, "F_", 2)) {
			// FFmpeg plugins can be audio or video
			if (window->asset->audio_data)
				strcpy(title, "FFmpeg Audio Plugins");
			if (window->asset->video_data)
				strcpy(title, "FFmpeg Video Plugins");
		}
		context_help_show(title);
		return 1;
	}
	else {
		context_help_show("Rendered Effects");
		return 1;
	}
	context_help_show("Rendered Effects");
	return 1;
}

#define PROMPT_TEXT _("Set up effect panel and hit \"OK\"")
#define MEP_W xS(260)
#define MEP_H yS(100)

MenuEffectPrompt::MenuEffectPrompt(MWindow *mwindow)
 : BC_Window(_(PROGRAM_NAME ": Effect Prompt"),
		mwindow->gui->get_abs_cursor_x(1) - MEP_W/2,
		mwindow->gui->get_abs_cursor_y(1) - MEP_H/2,
		MenuEffectPrompt::calculate_w(mwindow->gui),
		MenuEffectPrompt::calculate_h(mwindow->gui),
		MenuEffectPrompt::calculate_w(mwindow->gui),
		MenuEffectPrompt::calculate_h(mwindow->gui),
		0, 0, 1)
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Rendered Effects");
}

int MenuEffectPrompt::calculate_w(BC_WindowBase *gui)
{
	int w = BC_Title::calculate_w(gui, PROMPT_TEXT) + xS(10);
	w = MAX(w, BC_OKButton::calculate_w() + BC_CancelButton::calculate_w() + xS(30));
	return w;
}

int MenuEffectPrompt::calculate_h(BC_WindowBase *gui)
{
	int h = BC_Title::calculate_h(gui, PROMPT_TEXT);
	h += BC_OKButton::calculate_h() + yS(30);
	return h;
}


void MenuEffectPrompt::create_objects()
{
	lock_window("MenuEffectPrompt::create_objects");
	int x = xS(10), y = yS(10);
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, PROMPT_TEXT));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	raise_window();
	unlock_window();
}

