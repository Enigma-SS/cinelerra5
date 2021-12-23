
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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
 */

#ifndef __ALPHA_H__
#define __ALPHA_H__

#define OPACITY_MIN 0.f
#define OPACITY_MAX 1.f

class AlphaConfig;
class AlphaColors;
class AlphaWindow;
class AlphaMain;
class AlphaText;
class AlphaSlider;
class AlphaClr;

#include "bccolors.h"
#include "filexml.inc"
#include "guicast.h"
#include "pluginvclient.h"
#include "vframe.inc"

class AlphaConfig
{
public:
	AlphaConfig();
	void reset();

	int equivalent(AlphaConfig &that);
	void copy_from(AlphaConfig &that);
	void interpolate(AlphaConfig &prev, AlphaConfig &next,
		long prev_frame, long next_frame, long current_frame);

	float a;
};

class AlphaText : public BC_TumbleTextBox
{
public:
	AlphaText(AlphaWindow *window, AlphaMain *plugin, int x, int y);
	~AlphaText();
	int handle_event();

	AlphaWindow *window;
	AlphaMain *plugin;
};

class AlphaSlider : public BC_FSlider
{
public:
	AlphaSlider(AlphaWindow *window, AlphaMain *plugin, int x, int y, int w);
	~AlphaSlider();
	int handle_event();

	AlphaWindow *window;
	AlphaMain *plugin;
};

class AlphaClr : public BC_Button
{
public:
	AlphaClr(AlphaWindow *window, AlphaMain *plugin, int x, int y);
	~AlphaClr();
	int handle_event();

	AlphaWindow *window;
	AlphaMain *plugin;
};

class AlphaWindow : public PluginClientWindow
{
public:
	AlphaWindow(AlphaMain *plugin);
	~AlphaWindow();

	void create_objects();
	void update();

	AlphaMain *plugin;
	AlphaText *alpha_text;
	AlphaSlider *alpha_slider;
	AlphaClr *alpha_clr;
};

class AlphaMain : public PluginVClient
{
public:
	AlphaMain(PluginServer *server);
	~AlphaMain();

	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();

	PLUGIN_CLASS_MEMBERS(AlphaConfig)
};

#endif
