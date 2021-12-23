
/*
 * CINELERRA
 * Copyright (C) 2008-2016 Adam Williams <broadcast at earthling dot net>
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
 * 2020. Derivative by ReframeRT plugin for a more easy use.
 * It uses percentage value of the speed referred to originl speed (=100%).
 * Some old ReframeRT parameters (Stretch and denom) have not been deleted,
 * for future development, if any.
 * Stretch and denom variables are set to a constant value:
 * Stretch= 1; denom= 100.00.
 * Speed_MIN= 1.00%; Speed_MAX= 1000.00% 
 */

#ifndef SPEED_PC_H
#define SPEED_PC_H


#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"
#include "transportque.h"


#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL    0
#define RESET_SPEED  1

class SpeedPcConfig;
class SpeedPcText;
class SpeedPcSlider;
class SpeedPcStretch;
class SpeedPcDownsample;
class SpeedPcInterpolate;
class SpeedPcClr;
class SpeedPc;
class SpeedPcWindow;
class SpeedPcReset;



class SpeedPcConfig
{
public:
	SpeedPcConfig();
	void boundaries();
	int equivalent(SpeedPcConfig &src);
	void reset(int clear);
	void copy_from(SpeedPcConfig &src);
	void interpolate(SpeedPcConfig &prev,
		SpeedPcConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	double num;
	double denom;
	int stretch;
	int interp;
	int optic_flow;
};



class SpeedPcToggle : public BC_Radial
{
public:
	SpeedPcToggle(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int init_value,
		int x,
		int y,
		int value,
		const char *string);
	int handle_event();

	SpeedPc *plugin;
	SpeedPcWindow *gui;
	int value;
};

class SpeedPcText : public BC_TumbleTextBox
{
public:
	SpeedPcText(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int x,
		int y);
	~SpeedPcText();
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcSlider : public BC_FSlider
{
public:
	SpeedPcSlider(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int x, int y, int w);
	~SpeedPcSlider();
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcClr : public BC_Button
{
public:
	SpeedPcClr(SpeedPc *plugin, SpeedPcWindow *gui,
		int x, int y, int clear);
	~SpeedPcClr();
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
	int clear;
};

class SpeedPcStretch : public BC_Radial
{
public:
	SpeedPcStretch(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int x,
		int y);
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcDownsample : public BC_Radial
{
public:
	SpeedPcDownsample(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int x,
		int y);
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcInterpolate : public BC_CheckBox
{
public:
	SpeedPcInterpolate(SpeedPc *plugin,
		SpeedPcWindow *gui,
		int x,
		int y);
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcReset : public BC_GenericButton
{
public:
	SpeedPcReset(SpeedPc *plugin, SpeedPcWindow *gui, int x, int y);
	~SpeedPcReset();
	int handle_event();
	SpeedPc *plugin;
	SpeedPcWindow *gui;
};

class SpeedPcWindow : public PluginClientWindow
{
public:
	SpeedPcWindow(SpeedPc *plugin);
	~SpeedPcWindow();
	void create_objects();
	void update(int clear);

	int update_toggles();

	SpeedPc *plugin;

	SpeedPcToggle *toggle25pc;
	SpeedPcToggle *toggle50pc;
	SpeedPcToggle *toggle100pc;
	SpeedPcToggle *toggle200pc;
	SpeedPcToggle *toggle400pc;

	SpeedPcText *speed_pc_text;
	SpeedPcSlider *speed_pc_slider;
	SpeedPcClr *speed_pc_clr;
	SpeedPcStretch *stretch;
	SpeedPcDownsample *downsample;
	SpeedPcInterpolate *interpolate;

	SpeedPcReset *reset;
};


class SpeedPc : public PluginVClient
{
public:
	SpeedPc(PluginServer *server);
	~SpeedPc();

	PLUGIN_CLASS_MEMBERS(SpeedPcConfig)

	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_realtime();
	int is_synthesis();
	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
};

#endif