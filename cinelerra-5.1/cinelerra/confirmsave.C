
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

#include "asset.h"
#include "bchash.h"
#include "confirmsave.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"


ConfirmSave::ConfirmSave()
{
}

ConfirmSave::~ConfirmSave()
{
}

int ConfirmSave::get_save_path(MWindow *mwindow, char *filename)
{
	int result = 1;
	char directory[BCTEXTLEN];
	sprintf(directory, "~");
	mwindow->defaults->get("DIRECTORY", directory);
	while( result ) {
 		int mx, my;  mwindow->gui->get_abs_cursor(mx, my);
		my -= BC_WindowBase::get_resources()->filebox_h / 2;
		char string[BCTEXTLEN];
		sprintf(string, _("Enter a filename to save as"));
		BC_FileBox *filebox = new BC_FileBox(mx, my, directory,
			_(PROGRAM_NAME ": Save"), string);
		filebox->lock_window("ConfirmSave::get_save_path");
		filebox->create_objects();
		filebox->context_help_set_keyword("Saving Project Files");
		filebox->unlock_window();
		result = filebox->run_window();
		mwindow->defaults->update("DIRECTORY", filebox->get_submitted_path());
		strcpy(filename, filebox->get_submitted_path());
		delete filebox;
		if( result == 1 ) return 1;	// user cancelled
		if( !filename[0] ) return 1;	// no filename given
// Extend the filename with .xml
		if( strlen(filename) < 4 ||
		    strcasecmp(&filename[strlen(filename) - 4], ".xml") ) {
			strcat(filename, ".xml");
		}
		result = ConfirmSave::test_file(mwindow, filename);
	}
	return result;
}

int ConfirmSave::test_file(MWindow *mwindow, char *path)
{
	ArrayList<char*> paths;
	paths.append(path);
	int result = test_files(mwindow, &paths);
	paths.remove_all();
	return result;
}

int ConfirmSave::test_files(MWindow *mwindow, ArrayList<char*> *paths)
{
	ArrayList<BC_ListBoxItem*> list;
	int result = 0;

	for(int i = 0; i < paths->size(); i++) {
		char *path = paths->values[i];
		if( !access(path, F_OK) ) {
			list.append(new BC_ListBoxItem(path));
		}
	}

	if(list.total) {
		if(mwindow) {
			ConfirmSaveWindow window(mwindow, &list);
			window.create_objects();
			window.raise_window();
			result = window.run_window();
		}
		else {
			printf(_("The following files exist:\n"));
			for(int i = 0; i < list.total; i++) {
				printf("    %s\n", list.values[i]->get_text());
			}
			printf(_("Won't overwrite existing files.\n"));
			result = 1;
		}
		list.remove_all_objects();
		return result;
	}
	else {
		list.remove_all_objects();
		return 0;
	}

	return result;
}


#define CSW_W xS(400)
#define CSW_H yS(150)

ConfirmSaveWindow::ConfirmSaveWindow(MWindow *mwindow,
	ArrayList<BC_ListBoxItem*> *list)
 : BC_Window(_(PROGRAM_NAME ": File Exists"),
		mwindow->gui->get_abs_cursor_x(1) - CSW_W/2,
		mwindow->gui->get_abs_cursor_y(1) - CSW_H/2,
		CSW_W, CSW_H)
{
	this->list = list;
}

ConfirmSaveWindow::~ConfirmSaveWindow()
{
}


void ConfirmSaveWindow::create_objects()
{
	int xs10 = xS(10);
	int ys10 = yS(10), ys30 = yS(30);
	int x = xs10, y = ys10;
	lock_window("ConfirmSaveWindow::create_objects");
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	add_subwindow(title = new BC_Title(x,
		y,
		_("The following files exist.  Overwrite them?")));
	y += ys30;
	add_subwindow(listbox = new BC_ListBox(x, y,
		get_w() - x - xs10,
		get_h() - y - BC_OKButton::calculate_h() - ys10,
		LISTBOX_TEXT,
		list));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}

int ConfirmSaveWindow::resize_event(int w, int h)
{
	int xs10 = xS(10);
	int ys10 = yS(10), ys30 = yS(30);
	int x = xs10, y = ys10;
	title->reposition_window(x, y);
	y += ys30;
	listbox->reposition_window(x, y,
		w - x - xs10,
		h - y - BC_OKButton::calculate_h() - ys10);
	return 1;
}

