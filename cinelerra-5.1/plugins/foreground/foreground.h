
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

#ifndef FOREGROUND_H
#define FOREGROUND_H

class ForegroundConfig;
class ForegroundColors;
class ForegroundWindow;
class ForegroundMain;

#include "bccolors.h"
#include "colorpicker.h"
#include "filexml.inc"
#include "guicast.h"
#include "pluginvclient.h"
#include "vframe.inc"

class ForegroundConfig
{
public:
	ForegroundConfig();

	int equivalent(ForegroundConfig &that);
	void copy_from(ForegroundConfig &that);
	void interpolate(ForegroundConfig &prev, ForegroundConfig &next,
		long prev_frame, long next_frame, long current_frame);
// Int to hex triplet conversion
	int get_color();

	int r, g, b, a;
};

class ForegroundColors : public ColorGUI
{
public:
	ForegroundColors(ForegroundWindow *window,
			ForegroundMain *plugin);
	virtual ~ForegroundColors();

	int handle_new_color(int color, int alpha);
	void update_gui(int color, int alpha);

	ForegroundWindow *window;
	ForegroundMain *plugin;
};


class ForegroundWindow : public PluginClientWindow
{
public:
	ForegroundWindow(ForegroundMain *plugin);
	~ForegroundWindow();

	void create_objects();
	void update();

	int close_event() { return colors->close_gui(); }
	int cursor_motion_event() { return colors->cursor_motion_gui(); }
	int button_press_event() { return colors->button_press_gui(); }
	int button_release_event() { return colors->button_release_gui(); }
	void done_event(int result) { colors->close_gui(); }

	ForegroundMain *plugin;
	ForegroundColors *colors;
};

class ForegroundMain : public PluginVClient
{
public:
	ForegroundMain(PluginServer *server);
	~ForegroundMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int is_synthesis();

	PLUGIN_CLASS_MEMBERS(ForegroundConfig)
	YUV yuv;
};

#endif
