
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

#ifndef LOADMODE_H
#define LOADMODE_H

#include "guicast.h"
#include "loadmode.inc"
#include "mwindow.inc"
#include "theme.inc"

class LoadModeItem : public BC_ListBoxItem
{
public:
	LoadModeItem(const char *text, int value);
	int value;
};

class LoadModeToggle : public BC_Toggle
{
public:
	LoadModeToggle(int x, int y, LoadMode *window, int value,
		int *output, const char *images, const char *tooltip);
	int handle_event();
	LoadMode *window;
	int id, *output;
};

class LoadMode
{
public:
	LoadMode(MWindow *mwindow, BC_WindowBase *window,
		int x, int y, int *load_mode, int *edl_mode=0,
		int use_nothing=1, int line_wrap=0);
	~LoadMode();
	void create_objects();
	int reposition_window(int x, int y);
	static void load_mode_geometry(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap,
		int *pw, int *ph);
	static int calculate_w(BC_WindowBase *gui, Theme *theme,
		int use_nothing=1, int use_nested=0, int line_wrap=0);
	static int calculate_h(BC_WindowBase *gui, Theme *theme,
		int use_nothing=1, int use_nested=0, int line_wrap=0);
	int get_h();
	int get_x();
	int get_y();

	const char *mode_to_text(int mode);
	void update();
	int set_line_wrap(int v);

	BC_Title *load_title, *edl_title;
	BC_TextBox *textbox;
	LoadModeListBox *listbox;
	MWindow *mwindow;
	BC_WindowBase *window;
	int x, y, *load_mode, *edl_mode;
	int use_nothing, line_wrap;
	LoadModeToggle *mode[TOTAL_LOADMODES];
	ArrayList<LoadModeItem*> load_modes;
};

class LoadModeListBox : public BC_ListBox
{
public:
	LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode, int x, int y);
	~LoadModeListBox();

	int handle_event();

	BC_WindowBase *window;
	LoadMode *loadmode;
};

#endif
