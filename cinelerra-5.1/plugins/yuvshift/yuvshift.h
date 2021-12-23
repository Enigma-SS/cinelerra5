
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

#ifndef YUVSHIFT_H
#define YUVSHIFT_H

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

#define MAXVALUE 100

#define RESET_ALL  0
#define RESET_Y_DX 1
#define RESET_Y_DY 2
#define RESET_U_DX 3
#define RESET_U_DY 4
#define RESET_V_DX 5
#define RESET_V_DY 6

class YUVShiftEffect;
class YUVShiftWindow;
class YUVShiftIText;
class YUVShiftISlider;
class YUVShiftReset;
class YUVShiftClr;


class YUVShiftConfig
{
public:
	YUVShiftConfig();

	void reset(int clear);
	void copy_from(YUVShiftConfig &src);
	int equivalent(YUVShiftConfig &src);
	void interpolate(YUVShiftConfig &prev,
		YUVShiftConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int y_dx, y_dy, u_dx, u_dy, v_dx, v_dy;
};

class YUVShiftIText : public BC_TumbleTextBox
{
public:
	YUVShiftIText(YUVShiftWindow *window, YUVShiftEffect *plugin,
		YUVShiftISlider *slider, int *output, int x, int y, int min, int max);
	~YUVShiftIText();
	int handle_event();
	YUVShiftWindow *window;
	YUVShiftEffect *plugin;
	YUVShiftISlider *slider;
	int *output;
	int min, max;
};

class YUVShiftISlider : public BC_ISlider
{
public:
	YUVShiftISlider(YUVShiftEffect *plugin, YUVShiftIText *text, int *output, int x, int y);
	int handle_event();
	YUVShiftEffect *plugin;
	YUVShiftIText *text;
	int *output;
};

class YUVShiftReset : public BC_GenericButton
{
public:
	YUVShiftReset(YUVShiftEffect *plugin, YUVShiftWindow *window, int x, int y);
	~YUVShiftReset();
	int handle_event();
	YUVShiftEffect *plugin;
	YUVShiftWindow *window;
};

class YUVShiftClr : public BC_Button
{
public:
	YUVShiftClr(YUVShiftEffect *plugin, YUVShiftWindow *window, int x, int y, int clear);
	~YUVShiftClr();
	int handle_event();
	YUVShiftEffect *plugin;
	YUVShiftWindow *window;
	int clear;
};

class YUVShiftWindow : public PluginClientWindow
{
public:
	YUVShiftWindow(YUVShiftEffect *plugin);
	void create_objects();
	void update_gui(int clear);

	YUVShiftIText *y_dx_text;
	YUVShiftISlider *y_dx_slider;
	YUVShiftClr *y_dx_Clr;
	YUVShiftIText *y_dy_text;
	YUVShiftISlider *y_dy_slider;
	YUVShiftClr *y_dy_Clr;

	YUVShiftIText *u_dx_text;
	YUVShiftISlider *u_dx_slider;
	YUVShiftClr *u_dx_Clr;
	YUVShiftIText *u_dy_text;
	YUVShiftISlider *u_dy_slider;
	YUVShiftClr *u_dy_Clr;

	YUVShiftIText *v_dx_text;
	YUVShiftISlider *v_dx_slider;
	YUVShiftClr *v_dx_Clr;
	YUVShiftIText *v_dy_text;
	YUVShiftISlider *v_dy_slider;
	YUVShiftClr *v_dy_Clr;

	YUVShiftEffect *plugin;
	YUVShiftReset *reset;
};



class YUVShiftEffect : public PluginVClient
{
	VFrame *temp_frame;
public:
	YUVShiftEffect(PluginServer *server);
	~YUVShiftEffect();


	PLUGIN_CLASS_MEMBERS(YUVShiftConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
};

#endif
