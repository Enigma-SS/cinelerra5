
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

#ifndef SHARPENWINDOW_H
#define SHARPENWINDOW_H

#include "guicast.h"
#include "filexml.h"
#include "mutex.h"
#include "sharpen.h"
#include "theme.h"

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL 0
#define RESET_SHARPEN 1

class SharpenWindow;
class SharpenInterlace;
class SharpenText;
class SharpenSlider;
class SharpenHorizontal;
class SharpenLuminance;
class SharpenReset;
class SharpenDefaultSettings;
class SharpenClr;

class SharpenWindow : public PluginClientWindow
{
public:
	SharpenWindow(SharpenMain *client);
	~SharpenWindow();

	void create_objects();
	void update_gui(int clear);

	SharpenMain *client;

	SharpenText *sharpen_text;
	SharpenSlider *sharpen_slider;
	SharpenClr *sharpen_Clr;

	SharpenInterlace *sharpen_interlace;
	SharpenHorizontal *sharpen_horizontal;
	SharpenLuminance *sharpen_luminance;

	SharpenReset *reset;
	SharpenDefaultSettings *default_settings;
};

class SharpenText : public BC_TumbleTextBox
{
public:
	SharpenText(SharpenMain *client, SharpenWindow *gui, int x, int y);
	~SharpenText();
	int handle_event();

	SharpenMain *client;
	SharpenWindow *gui;
};

class SharpenSlider : public BC_ISlider
{
public:
	SharpenSlider(SharpenMain *client, SharpenWindow *gui, int x, int y, int w);
	~SharpenSlider();
	int handle_event();

	SharpenWindow *gui;
	SharpenMain *client;
};

class SharpenInterlace : public BC_CheckBox
{
public:
	SharpenInterlace(SharpenMain *client, int x, int y);
	~SharpenInterlace();
	int handle_event();

	SharpenMain *client;
};

class SharpenHorizontal : public BC_CheckBox
{
public:
	SharpenHorizontal(SharpenMain *client, int x, int y);
	~SharpenHorizontal();
	int handle_event();

	SharpenMain *client;
};

class SharpenLuminance : public BC_CheckBox
{
public:
	SharpenLuminance(SharpenMain *client, int x, int y);
	~SharpenLuminance();
	int handle_event();

	SharpenMain *client;
};

class SharpenReset : public BC_GenericButton
{
public:
	SharpenReset(SharpenMain *client, SharpenWindow *gui, int x, int y);
	~SharpenReset();
	int handle_event();
	SharpenMain *client;
	SharpenWindow *gui;
};

class SharpenDefaultSettings : public BC_GenericButton
{
public:
	SharpenDefaultSettings(SharpenMain *client, SharpenWindow *gui, int x, int y, int w);
	~SharpenDefaultSettings();
	int handle_event();
	SharpenMain *client;
	SharpenWindow *gui;
};

class SharpenClr : public BC_Button
{
public:
	SharpenClr(SharpenMain *client, SharpenWindow *gui, int x, int y);
	~SharpenClr();
	int handle_event();
	SharpenMain *client;
	SharpenWindow *gui;
};

#endif
