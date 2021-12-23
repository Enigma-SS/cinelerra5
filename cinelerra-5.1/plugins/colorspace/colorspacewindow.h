
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

#ifndef ColorSpaceWINDOW_H
#define ColorSpaceWINDOW_H

#include "guicast.h"

class ColorSpaceSpace;
class ColorSpaceRange;
class ColorSpaceThread;
class ColorSpaceWindow;

#include "filexml.h"
#include "mutex.h"
#include "colorspace.h"


class ColorSpaceSpace : public BC_PopupMenu
{
	static const char *color_space[3];
public:
	ColorSpaceSpace(ColorSpaceWindow *gui, int x, int y, int *value);
	~ColorSpaceSpace();
	void create_objects();
	void update();
	int handle_event();

	ColorSpaceWindow *gui;
	int *value;
};

class ColorSpaceSpaceItem : public BC_MenuItem
{
public:
	ColorSpaceSpaceItem(ColorSpaceSpace *popup, const char *text, int id);
	int handle_event();

	ColorSpaceSpace *popup;
	int id;
};

class ColorSpaceRange : public BC_PopupMenu
{
	static const char *color_range[2];
public:
	ColorSpaceRange(ColorSpaceWindow *gui, int x, int y, int *value);
	~ColorSpaceRange();
	void create_objects();
	void update();
	int handle_event();

	ColorSpaceWindow *gui;
	int *value;
};

class ColorSpaceRangeItem : public BC_MenuItem
{
public:
	ColorSpaceRangeItem(ColorSpaceRange *popup, const char *text, int id);
	int handle_event();

	ColorSpaceRange *popup;
	int id;
};

class ColorSpaceInverse : public BC_CheckBox
{
public:
	ColorSpaceInverse(ColorSpaceWindow *gui, int x, int y, int *value);
	~ColorSpaceInverse();
	void update();
	int handle_event();

	ColorSpaceWindow *gui;
};


class ColorSpaceWindow : public PluginClientWindow
{
public:
	ColorSpaceWindow(ColorSpaceMain *plugin);
	~ColorSpaceWindow();

	void create_objects();

	void update();

	ColorSpaceMain *plugin;
	ColorSpaceInverse *inverse;
	ColorSpaceSpace *inp_space, *out_space;
	ColorSpaceRange *inp_range, *out_range;
};

#endif
