
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

#ifndef UNSHARPWINDOW_H
#define UNSHARPWINDOW_H

#include "guicast.h"
#include "unsharp.inc"
#include "unsharpwindow.inc"


#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL       0
#define RESET_RADIUS	1
#define RESET_AMOUNT    2
#define RESET_THRESHOLD 3

#define RADIUS_MIN      0.10
#define RADIUS_MAX      120.00
#define AMOUNT_MIN      0.00
#define AMOUNT_MAX      5.00
#define THRESHOLD_MIN   0
#define THRESHOLD_MAX 255



class UnsharpRadiusText : public BC_TumbleTextBox
{
public:
	UnsharpRadiusText(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y);
	~UnsharpRadiusText();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};

class UnsharpRadiusSlider : public BC_FSlider
{
public:
	UnsharpRadiusSlider(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y, int w);
	~UnsharpRadiusSlider();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};


class UnsharpAmountText : public BC_TumbleTextBox
{
public:
	UnsharpAmountText(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y);
	~UnsharpAmountText();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};


class UnsharpAmountSlider : public BC_FSlider
{
public:
	UnsharpAmountSlider(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y, int w);
	~UnsharpAmountSlider();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};


class UnsharpThresholdText : public BC_TumbleTextBox
{
public:
	UnsharpThresholdText(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y);
	~UnsharpThresholdText();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};

class UnsharpThresholdSlider : public BC_ISlider
{
public:
	UnsharpThresholdSlider(UnsharpWindow *window,
		UnsharpMain *plugin,
		int x, int y, int w);
	~UnsharpThresholdSlider();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};


class UnsharpClr : public BC_Button
{
public:
	UnsharpClr(UnsharpWindow *window, UnsharpMain *plugin,
		int x, int y, int clear);
	~UnsharpClr();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
	int clear;
};


class UnsharpReset : public BC_GenericButton
{
public:
	UnsharpReset(UnsharpWindow *window, UnsharpMain *plugin, int x, int y);
	~UnsharpReset();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};

class UnsharpDefaultSettings : public BC_GenericButton
{
public:
	UnsharpDefaultSettings(UnsharpWindow *window, UnsharpMain *plugin, int x, int y, int w);
	~UnsharpDefaultSettings();
	int handle_event();
	UnsharpWindow *window;
	UnsharpMain *plugin;
};



class UnsharpWindow : public PluginClientWindow
{
public:
	UnsharpWindow(UnsharpMain *plugin);
	~UnsharpWindow();

	void create_objects();
	void update_gui(int clear);

	UnsharpMain *plugin;

	UnsharpRadiusText *radius_text;
	UnsharpRadiusSlider *radius_slider;
	UnsharpClr *radius_clr;

	UnsharpAmountText *amount_text;
	UnsharpAmountSlider *amount_slider;
	UnsharpClr *amount_clr;

	UnsharpThresholdText *threshold_text;
	UnsharpThresholdSlider *threshold_slider;
	UnsharpClr *threshold_clr;

	UnsharpReset *reset;
	UnsharpDefaultSettings *default_settings;
};


#endif
