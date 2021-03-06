
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "bchash.h"
#include "edl.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "language.h"
#include "loadfile.h"
#include "loadmode.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "theme.h"



#include <string.h>

Load::Load(MWindow *mwindow, MainMenu *mainmenu)
 : BC_MenuItem(_("Load files..."), "o", 'o')
{
	this->mwindow = mwindow;
	this->mainmenu = mainmenu;
	this->thread = 0;
}

Load::~Load()
{
	delete thread;
}

void Load::create_objects()
{
	thread = new LoadFileThread(mwindow, this);
}

int Load::handle_event()
{
	mwindow->gui->unlock_window();
	thread->start();
	mwindow->gui->lock_window("Load::handle_event");
	return 1;
}






LoadFileThread::LoadFileThread(MWindow *mwindow, Load *load)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->load = load;
	this->window = 0;
	load_mode = LOADMODE_RESOURCESONLY;
	edl_mode = LOADMODE_EDL_CLIP;
}

LoadFileThread::~LoadFileThread()
{
	close_window();
}

BC_Window* LoadFileThread::new_gui()
{
	char default_path[BCTEXTLEN];

	sprintf(default_path, "~");
	mwindow->defaults->get("DEFAULT_LOADPATH", default_path);
	load_mode = mwindow->defaults->get("LOAD_MODE", load_mode);

	mwindow->gui->lock_window("LoadFileThread::new_gui");
	window = new LoadFileWindow(mwindow, this, default_path);
	mwindow->gui->unlock_window();

	window->create_objects();
	return window;
}

void LoadFileThread::handle_done_event(int result)
{
	window->lock_window("LoadFileThread::handle_done_event");
	window->hide_window();
	window->unlock_window();

	if( !result ) load_apply();
}

void LoadFileThread::load_apply()
{
	mwindow->defaults->update("DEFAULT_LOADPATH", window->get_submitted_path());
	mwindow->defaults->update("LOAD_MODE", load_mode);
	if( edl_mode == LOADMODE_EDL_FILEREF )
		mwindow->show_warning(
			&mwindow->preferences->warn_fileref,
			_("Other projects can change this project\n"
			  "and this can become a broken link"));
	ArrayList<char*> path_list;
	path_list.set_array_delete();

// Collect all selected files
	char *in_path;
	for( int i=0; (in_path = window->get_path(i))!=0; ++i ) {
		int k = path_list.size();
		while( --k >= 0 && strcmp(in_path, path_list.values[k]) );
		if( k < 0 ) path_list.append(cstrdup(in_path));
	}

// No file selected
	if( !path_list.size() ) return;
	int replaced = load_mode == LOADMODE_REPLACE ||
            load_mode == LOADMODE_REPLACE_CONCATENATE ? 1 : 0;

	mwindow->interrupt_indexes();
	mwindow->gui->lock_window("LoadFileThread::run");
	mwindow->load_filenames(&path_list, load_mode, edl_mode, replaced);
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
	mwindow->gui->unlock_window();
	path_list.remove_all_objects();

	mwindow->save_backup();
	mwindow->restart_brender();
	mwindow->session->changes_made = !replaced ? 1 : 0;
}


LoadFileWindow::LoadFileWindow(MWindow *mwindow,
	LoadFileThread *thread,
	char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 		mwindow->gui->get_abs_cursor_y(1) -
			BC_WindowBase::get_resources()->filebox_h / 2,
		init_directory,
		_(PROGRAM_NAME ": Load"),
		_("Select files to load:"),
		0,
		0,
		1,
		mwindow->theme->loadfile_pad)
{
	this->thread = thread;
	this->mwindow = mwindow;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Loading Files");
}

LoadFileWindow::~LoadFileWindow()
{
	lock_window("LoadFileWindow::~LoadFileWindow");
	delete loadmode;
	unlock_window();
}

void LoadFileWindow::create_objects()
{
	lock_window("LoadFileWindow::create_objects");
	BC_FileBox::create_objects();

	int x = get_w() / 2 - LoadMode::calculate_w(this, mwindow->theme) / 2;
	int y = get_y_margin();
// always start as clip to match historical behavior
	thread->edl_mode = LOADMODE_EDL_CLIP;
	loadmode = new LoadMode(mwindow, this, x, y,
		&thread->load_mode, &thread->edl_mode, 0, 1);
	loadmode->create_objects();
	const char *apply =  _("Apply");
	x = 3*get_w()/4 - BC_GenericButton::calculate_w(this, apply)/2;
	y = get_h() - BC_CancelButton::calculate_h() - yS(16);
	add_subwindow(load_file_apply = new LoadFileApply(this, x, y, apply));

	show_window(1);
	unlock_window();

}

int LoadFileWindow::resize_event(int w, int h)
{
	draw_background(0, 0, w, h);
	BC_FileBox::resize_event(w, h);
	int x = w / 2 - LoadMode::calculate_w(this, mwindow->theme) / 2;
	int y = get_y_margin();
	loadmode->reposition_window(x, y);
	const char *apply =  load_file_apply->get_text();
	x = 3*get_w()/4 - BC_GenericButton::calculate_w(this, apply)/2;
	y = get_h() - BC_CancelButton::calculate_h() - yS(16);
	load_file_apply->reposition_window(x, y);
	flush();
	return 1;
}


LoadFileApply::LoadFileApply(LoadFileWindow *load_file_window,
		int x, int y, const char *text)
 : BC_GenericButton(x, y, text)
{
	this->load_file_window = load_file_window;
}

int LoadFileApply::handle_event()
{
	load_file_window->thread->load_apply();
	return 1;
}


LocateFileWindow::LocateFileWindow(MWindow *mwindow,
	char *init_directory,
	char *old_filename)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
 		mwindow->gui->get_abs_cursor_y(1),
		init_directory,
		_(PROGRAM_NAME ": Locate file"),
		old_filename)
{
	this->mwindow = mwindow;
}

LocateFileWindow::~LocateFileWindow()
{
}







LoadPrevious::LoadPrevious(MWindow *mwindow, Load *loadfile)
 : BC_MenuItem("")
{
	this->mwindow = mwindow;
	this->loadfile = loadfile;
}

int LoadPrevious::handle_event()
{
	if( !path[0] ) return 1;
	ArrayList<char*> path_list;
	path_list.set_array_delete();
	char *out_path;
	int load_mode = mwindow->defaults->get("LOAD_MODE", LOADMODE_REPLACE);

	path_list.append(out_path = new char[strlen(path) + 1]);
	strcpy(out_path, path);

	mwindow->load_filenames(&path_list, LOADMODE_REPLACE);
	mwindow->gui->mainmenu->add_load(path_list.values[0]);
	path_list.remove_all_objects();

	mwindow->defaults->update("LOAD_MODE", load_mode);
	mwindow->save_backup();
	mwindow->session->changes_made = 0;
	return 1;
}

int LoadPrevious::set_path(const char *path)
{
	strcpy(this->path, path);
	return 0;
}



LoadBackup::LoadBackup(MWindow *mwindow)
 : BC_MenuItem(_("Load backup"))
{
	this->mwindow = mwindow;
}

int LoadBackup::handle_event()
{
	mwindow->load_backup();
	mwindow->session->changes_made = 1;
	return 1;
}


