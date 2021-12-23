
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

#ifndef SCALE_H
#define SCALE_H

// the simplest plugin possible

class ScaleMain;
class ScaleConstrain;
class ScaleThread;
class ScaleWin;

class ScaleUseScale;
class ScaleUseSize;

class ScaleXFactorText;
class ScaleXFactorSlider;
class ScaleYFactorText;
class ScaleYFactorSlider;
class ScaleWidthText;
class ScaleWidthSlider;
class ScaleHeightText;
class ScaleHeightSlider;

class ScaleClr;
class ScaleReset;

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL    0
#define RESET_X_FACTOR 1
#define RESET_Y_FACTOR 2
#define RESET_WIDTH  3
#define RESET_HEIGHT 4

#define MIN_FACTOR  0.00
#define MAX_FACTOR 10.00
#define MAX_WIDTH 16384
#define MAX_HEIGHT 9216


#include "bchash.h"
#include "guicast.h"
#include "mwindow.inc"
#include "mutex.h"
#include "new.h"
#include "scalewin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

#define FIXED_SCALE 0
#define FIXED_SIZE 1

class ScaleConfig
{
public:
	ScaleConfig();
	void reset(int clear);
	void copy_from(ScaleConfig &src);
	int equivalent(ScaleConfig &src);
	void interpolate(ScaleConfig &prev,
		ScaleConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	int type;
	float x_factor, y_factor;
	int width, height;
	int constrain;
};





class ScaleUseScale : public BC_Radial
{
public:
	ScaleUseScale(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleUseScale();
	int handle_event();

	ScaleWin *win;
	ScaleMain *client;
};

class ScaleUseSize : public BC_Radial
{
public:
	ScaleUseSize(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleUseSize();
	int handle_event();

	ScaleWin *win;
	ScaleMain *client;
};

class ScaleConstrain : public BC_CheckBox
{
public:
	ScaleConstrain(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleConstrain();
	int handle_event();

	ScaleWin *win;
	ScaleMain *client;
};

class ScaleWin : public PluginClientWindow
{
public:
	ScaleWin(ScaleMain *client);
	~ScaleWin();

	void create_objects();
	void update(int clear);

	void update_scale_size_enable();

	ScaleMain *client;

	FrameSizePulldown *pulldown;
	ScaleUseScale *use_scale;
	ScaleUseSize *use_size;
	ScaleConstrain *constrain;

	ScaleXFactorText *x_factor_text;
	ScaleXFactorSlider *x_factor_slider;
	ScaleClr *x_factor_clr;

	ScaleYFactorText *y_factor_text;
	ScaleYFactorSlider *y_factor_slider;
	ScaleClr *y_factor_clr;

	ScaleWidthText *width_text;
	ScaleWidthSlider *width_slider;
	ScaleClr *width_clr;

	ScaleHeightText *height_text;
	ScaleHeightSlider *height_slider;
	ScaleClr *height_clr;

	ScaleReset *reset;
};



class ScaleMain : public PluginVClient
{
public:
	ScaleMain(PluginServer *server);
	~ScaleMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(ScaleConfig)
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	void calculate_transfer(VFrame *frame,
		float &in_x1,
		float &in_x2,
		float &in_y1,
		float &in_y2,
		float &out_x1,
		float &out_x2,
		float &out_y1,
		float &out_y2);
	int handle_opengl();
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void set_type(int type);


	PluginServer *server;
	OverlayFrame *overlayer;   // To scale images
};

class ScaleXFactorText : public BC_TumbleTextBox
{
public:
	ScaleXFactorText(ScaleWin *win, ScaleMain *client,
		int x,
		int y);
	~ScaleXFactorText();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
	int enabled;
};

class ScaleXFactorSlider : public BC_FSlider
{
public:
	ScaleXFactorSlider(ScaleWin *win, ScaleMain *client,
		int x, int y, int w);
	~ScaleXFactorSlider();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
};

class ScaleYFactorText : public BC_TumbleTextBox
{
public:
	ScaleYFactorText(ScaleWin *win, ScaleMain *client,
		int x,
		int y);
	~ScaleYFactorText();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
	int enabled;
};

class ScaleYFactorSlider : public BC_FSlider
{
public:
	ScaleYFactorSlider(ScaleWin *win, ScaleMain *client,
		int x, int y, int w);
	~ScaleYFactorSlider();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
};

class ScaleWidthText : public BC_TumbleTextBox
{
public:
	ScaleWidthText(ScaleWin *win, ScaleMain *client,
		int x,
		int y);
	~ScaleWidthText();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
	int enabled;
};

class ScaleWidthSlider : public BC_ISlider
{
public:
	ScaleWidthSlider(ScaleWin *win, ScaleMain *client,
		int x, int y, int w);
	~ScaleWidthSlider();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
};

class ScaleHeightText : public BC_TumbleTextBox
{
public:
	ScaleHeightText(ScaleWin *win, ScaleMain *client,
		int x,
		int y);
	~ScaleHeightText();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
	int enabled;
};

class ScaleHeightSlider : public BC_ISlider
{
public:
	ScaleHeightSlider(ScaleWin *win, ScaleMain *client,
		int x, int y, int w);
	~ScaleHeightSlider();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
};


class ScaleClr : public BC_Button
{
public:
	ScaleClr(ScaleWin *win, ScaleMain *client,
		int x, int y, int clear);
	~ScaleClr();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
	int clear;
};

class ScaleReset : public BC_GenericButton
{
public:
	ScaleReset(ScaleWin *win, ScaleMain *client, int x, int y);
	~ScaleReset();
	int handle_event();
	ScaleWin *win;
	ScaleMain *client;
};

#endif
