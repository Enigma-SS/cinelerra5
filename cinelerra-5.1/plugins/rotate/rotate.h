
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

#ifndef ROTATE_H
#define ROTATE_H



#include "affine.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "rotateframe.h"
#include "vframe.h"


#include <string.h>


class RotateEffect;
class RotateWindow;
class RotateToggle;
class RotateDrawPivot;
class RotateInterpolate;
class RotateAngleText;
class RotateAngleSlider;
class RotatePivotXText;
class RotatePivotXSlider;
class RotatePivotYText;
class RotatePivotYSlider;
class RotateClr;
class RotateReset;


#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL     0
#define RESET_ANGLE   1
#define RESET_PIVOT_X 2
#define RESET_PIVOT_Y 3


class RotateConfig
{
public:
	RotateConfig();

	void reset(int clear);
	int equivalent(RotateConfig &that);
	void copy_from(RotateConfig &that);
	void interpolate(RotateConfig &prev,
		RotateConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	float angle;
	float pivot_x;
	float pivot_y;
	int draw_pivot;
//	int bilinear;
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window,
		RotateEffect *plugin,
		int init_value,
		int x,
		int y,
		int value,
		const char *string);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};

class RotateDrawPivot : public BC_CheckBox
{
public:
	RotateDrawPivot(RotateWindow *window,
		RotateEffect *plugin,
		int x,
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};

class RotateInterpolate : public BC_CheckBox
{
public:
	RotateInterpolate(RotateEffect *plugin, int x, int y);
	int handle_event();
	RotateEffect *plugin;
};

class RotateAngleText : public BC_TumbleTextBox
{
public:
	RotateAngleText(RotateWindow *window,
		RotateEffect *plugin,
		int x,
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateAngleSlider : public BC_FSlider
{
public:
	RotateAngleSlider(RotateWindow *window,
		RotateEffect *plugin,
		int x, int y, int w);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};


class RotatePivotXText : public BC_TumbleTextBox
{
public:
	RotatePivotXText(RotateWindow *window,
		RotateEffect *plugin,
		int x,
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotatePivotXSlider : public BC_FSlider
{
public:
	RotatePivotXSlider(RotateWindow *window,
		RotateEffect *plugin,
		int x, int y, int w);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};


class RotatePivotYText : public BC_TumbleTextBox
{
public:
	RotatePivotYText(RotateWindow *window,
		RotateEffect *plugin,
		int x,
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotatePivotYSlider : public BC_FSlider
{
public:
	RotatePivotYSlider(RotateWindow *window,
		RotateEffect *plugin,
		int x, int y, int w);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};




class RotateClr : public BC_Button
{
public:
	RotateClr(RotateWindow *window, RotateEffect *plugin,
		int x, int y, int clear);
	~RotateClr();
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
	int clear;
};


class RotateReset : public BC_GenericButton
{
public:
	RotateReset(RotateEffect *plugin, RotateWindow *window,
	int x, int y);
	~RotateReset();
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
};


class RotateWindow : public PluginClientWindow
{
public:
	RotateWindow(RotateEffect *plugin);

	void create_objects();

	int update();
	int update_sliders();
	int update_texts();
	int update_toggles();

	RotateEffect *plugin;
	RotateToggle *toggle180neg;
	RotateToggle *toggle90neg;
	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;

	RotateAngleText *rotate_angle_text;
	RotateAngleSlider *rotate_angle_slider;
	RotateClr *rotate_angle_clr;

	RotateDrawPivot *draw_pivot;

	RotatePivotXText *rotate_pivot_x_text;
	RotatePivotXSlider *rotate_pivot_x_slider;
	RotateClr *rotate_pivot_x_clr;

	RotatePivotYText *rotate_pivot_y_text;
	RotatePivotYSlider *rotate_pivot_y_slider;
	RotateClr *rotate_pivot_y_clr;

//	RotateInterpolate *bilinear;
	RotateReset *reset;
};




class RotateEffect : public PluginVClient
{
public:
	RotateEffect(PluginServer *server);
	~RotateEffect();

	PLUGIN_CLASS_MEMBERS(RotateConfig)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();

	AffineEngine *engine;
	int need_reconfigure;
};

#endif
