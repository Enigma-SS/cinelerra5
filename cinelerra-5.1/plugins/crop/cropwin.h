
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

/*
 * 2019. Derivative by Translate plugin. This plugin works also with Proxy.
 * It uses Percent values instead of Pixel value coordinates.
*/

#ifndef CROPWIN_H
#define CROPWIN_H


class CropThread;
class CropWin;
class CropLeftText;
class CropLeftSlider;
class CropTopText;
class CropTopSlider;
class CropRightText;
class CropRightSlider;
class CropBottomText;
class CropBottomSlider;
class CropPositionXText;
class CropPositionXSlider;
class CropPositionYText;
class CropPositionYSlider;
class CropReset;
class CropEdgesClr;

#include "guicast.h"
#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "crop.h"

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL    0
#define RESET_LEFT   1
#define RESET_TOP    2
#define RESET_RIGHT  3
#define RESET_BOTTOM 4
#define RESET_POSITION_X 5
#define RESET_POSITION_Y 6


class CropWin : public PluginClientWindow
{
public:
	CropWin(CropMain *client);
	~CropWin();

	void create_objects();
	void update(int clear);

	// Crop: Left, Top, Right, Bottom
	CropLeftText *crop_left_text;
	CropLeftSlider *crop_left_slider;
	CropEdgesClr *crop_left_clr;

	CropTopText *crop_top_text;
	CropTopSlider *crop_top_slider;
	CropEdgesClr *crop_top_clr;

	CropRightText *crop_right_text;
	CropRightSlider *crop_right_slider;
	CropEdgesClr *crop_right_clr;

	CropBottomText *crop_bottom_text;
	CropBottomSlider *crop_bottom_slider;
	CropEdgesClr *crop_bottom_clr;

	// Crop: Position_X, Position_Y
	CropPositionXText *crop_position_x_text;
	CropPositionXSlider *crop_position_x_slider;
	CropEdgesClr *crop_position_x_clr;

	CropPositionYText *crop_position_y_text;
	CropPositionYSlider *crop_position_y_slider;
	CropEdgesClr *crop_position_y_clr;

	CropMain *client;
	CropReset *reset;
};


class CropLeftText : public BC_TumbleTextBox
{
public:
	CropLeftText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropLeftText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropLeftSlider : public BC_FSlider
{
public:
	CropLeftSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropLeftSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropTopText : public BC_TumbleTextBox
{
public:
	CropTopText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropTopText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropTopSlider : public BC_FSlider
{
public:
	CropTopSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropTopSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropRightText : public BC_TumbleTextBox
{
public:
	CropRightText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropRightText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropRightSlider : public BC_FSlider
{
public:
	CropRightSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropRightSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropBottomText : public BC_TumbleTextBox
{
public:
	CropBottomText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropBottomText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropBottomSlider : public BC_FSlider
{
public:
	CropBottomSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropBottomSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropPositionXText : public BC_TumbleTextBox
{
public:
	CropPositionXText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropPositionXText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropPositionXSlider : public BC_FSlider
{
public:
	CropPositionXSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropPositionXSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropPositionYText : public BC_TumbleTextBox
{
public:
	CropPositionYText(CropWin *win,
		CropMain *client,
		int x,
		int y);
	~CropPositionYText();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

class CropPositionYSlider : public BC_FSlider
{
public:
	CropPositionYSlider(CropWin *win, CropMain *client,
		int x, int y, int w);
	~CropPositionYSlider();
	int handle_event();
	CropWin *win;
	CropMain *client;
};

class CropEdgesClr : public BC_Button
{
public:
	CropEdgesClr(CropWin *win, CropMain *client,
		int x, int y, int clear);
	~CropEdgesClr();
	int handle_event();
	CropWin *win;
	CropMain *client;
	int clear;
};

class CropReset : public BC_GenericButton
{
public:
	CropReset(CropMain *client, CropWin *win, int x, int y);
	~CropReset();
	int handle_event();
	CropMain *client;
	CropWin *win;
};

#endif
