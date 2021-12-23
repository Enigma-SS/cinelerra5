
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef TIMEBLURWINDOW_H
#define TIMEBLURWINDOW_H

#include "timeblur.h"

class TimeBlurSelect;
class TimeBlurClearFrames;
class TimeBlurFrames;
class TimeBlurWindow;

class TimeBlurSelect : public BC_GenericButton
{
public:
	TimeBlurSelect(TimeBlurMain *plugin, TimeBlurWindow *gui, int x, int y);
	int handle_event();
	TimeBlurMain *plugin;
	TimeBlurWindow *gui;
};

class TimeBlurClearFrames : public BC_Button
{
public:
	TimeBlurClearFrames(TimeBlurMain *plugin, TimeBlurWindow *gui, int x, int y);
	int handle_event();
	TimeBlurMain *plugin;
	TimeBlurWindow *gui;
};

class TimeBlurFrames : public BC_TumbleTextBox
{
public:
	TimeBlurFrames(TimeBlurMain *plugin, TimeBlurWindow *gui, int x, int y);
	int handle_event();
	void update(int frames);

	TimeBlurMain *plugin;
	TimeBlurWindow *gui;
};

class TimeBlurWindow : public PluginClientWindow
{
public:
	TimeBlurWindow(TimeBlurMain *plugin);
	~TimeBlurWindow();

	void create_objects();
	TimeBlurMain *plugin;
	TimeBlurSelect *select;
	TimeBlurFrames *frames;
	TimeBlurClearFrames *clear_frames;
};

#endif
