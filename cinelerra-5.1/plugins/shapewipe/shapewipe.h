
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

#ifndef SHAPEWIPE_H
#define SHAPEWIPE_H

class ShapeWipeConfig;
class ShapeWipeMain;
class ShapeWipeWindow;
class ShapeWipeW2B;
class ShapeWipeB2W;
class ShapeWipeTumble;
class ShapeWipeFeather;
class ShapeWipeFSlider;
class ShapeWipeReset;
class ShapeWipeShape;
class ShapeWipePreserveAspectRatio;
class ShapePackage;
class ShapeUnit;
class ShapeEngine;

#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

class ShapeWipeW2B : public BC_Radial
{
public:
	ShapeWipeW2B(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};

class ShapeWipeB2W : public BC_Radial
{
public:
	ShapeWipeB2W(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipeTumble : public BC_Tumbler
{
public:
	ShapeWipeTumble(ShapeWipeMain *client,
		ShapeWipeWindow *window,
		int x,
		int y);

	int handle_up_event();
	int handle_down_event();

	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeFeather : public BC_TumbleTextBox
{
public:
	ShapeWipeFeather(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y);
	int handle_event();

	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeFSlider : public BC_FSlider
{
public:
	ShapeWipeFSlider(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y, int w);
	int handle_event();

	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeReset : public BC_Button
{
public:
	ShapeWipeReset(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y);
	int handle_event();

	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipeShape : public BC_PopupTextBox
{
public:
	ShapeWipeShape(ShapeWipeMain *client, ShapeWipeWindow *window,
		int x, int y, int text_w, int list_h);
	int handle_event();
	ShapeWipeMain *client;
	ShapeWipeWindow *window;
};

class ShapeWipePreserveAspectRatio : public BC_CheckBox
{
public:
	ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
		ShapeWipeWindow *window,
		int x,
		int y);
	int handle_event();
	ShapeWipeMain *plugin;
	ShapeWipeWindow *window;
};


class ShapeWipeWindow : public PluginClientWindow
{
public:
	ShapeWipeWindow(ShapeWipeMain *plugin);
	~ShapeWipeWindow();
	void create_objects();
	void next_shape();
	void prev_shape();

	ShapeWipeMain *plugin;
	ShapeWipeW2B *left;
	ShapeWipeB2W *right;
	ShapeWipeTumble *shape_tumbler;
	ShapeWipeShape *shape_text;
	ShapeWipeFeather *shape_feather;
	ShapeWipeFSlider *shape_fslider;
	ShapeWipeReset *shape_reset;
	ArrayList<BC_ListBoxItem*> shapes;
};

class ShapeWipeConfig
{
public:
	ShapeWipeConfig();
	~ShapeWipeConfig();
	void read_xml(KeyFrame *keyframe);
	void save_xml(KeyFrame *keyframe);

	int direction;
	float feather;
	int preserve_aspect;
	char shape_name[BCTEXTLEN];
};

class ShapeWipeMain : public PluginVClient
{
public:
	ShapeWipeMain(PluginServer *server);
	~ShapeWipeMain();

	PLUGIN_CLASS_MEMBERS(ShapeWipeConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int uses_gui();
	int is_transition();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);

	int read_pattern_image(char *shape_name,
		int new_frame_width, int new_frame_height);
	void reset_pattern_image();
	void init_shapes();

	VFrame *input, *output;
	ShapeEngine *engine;
	ArrayList<char*> shape_paths;
	ArrayList<char*> shape_titles;

	int shapes_initialized;
	int threshold;
	char current_filename[BCTEXTLEN];
	char current_name[BCTEXTLEN];
	unsigned char **pattern_image;
	int frame_width;
	int frame_height;
	int preserve_aspect;
	int last_preserve_aspect;
};


class ShapePackage : public LoadPackage
{
public:
	ShapePackage();
	int y1, y2;
};

class ShapeUnit : public LoadClient
{
public:
	ShapeUnit(ShapeEngine *server);
	~ShapeUnit();
	void process_package(LoadPackage *package);
	ShapeEngine *server;
	unsigned char threshold;
};

class ShapeEngine : public LoadServer
{
public:
	ShapeEngine(ShapeWipeMain *plugin,
		int total_clients, int total_packages);
	~ShapeEngine();

	void init_packages();
	LoadClient *new_client();
	LoadPackage *new_package();
	ShapeWipeMain *plugin;
};

#endif
