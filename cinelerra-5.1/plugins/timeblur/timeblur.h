
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef TIMEBLUR_H
#define TIMEBLUR_H

#include "loadbalance.h"
#include "pluginvclient.h"
#include "linklist.h"
#include <stdint.h>

class TimeBlurConfig;
class TimeBlurMain;
class TimeBlurStripePackage;
class TimeBlurStripeUnit;
class TimeBlurStripeEngine;

class TimeBlurConfig
{
public:
	TimeBlurConfig();
	int equivalent(TimeBlurConfig &that);
	void copy_from(TimeBlurConfig &that);
	void interpolate(TimeBlurConfig &prev, TimeBlurConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);

	int frames;
};

class TimeBlurMain : public PluginVClient
{
public:
	TimeBlurMain(PluginServer *server);
	~TimeBlurMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	PLUGIN_CLASS_MEMBERS(TimeBlurConfig)

	VFrame *input;
	TimeBlurStripeEngine *stripe_engine;

	int w, h;
	VFrame *fframe;
	int64_t last_position;
	int last_frames;
};

enum { ADD_TEMP, ADD_FFRM, ADD_FFRMS, ADD_TEMPS, SUB_TEMPS };

class TimeBlurStripePackage : public LoadPackage
{
public:
	TimeBlurStripePackage();
	int y0, y1;
};

class TimeBlurStripeUnit : public LoadClient
{
public:
	TimeBlurStripeUnit(TimeBlurStripeEngine *server, TimeBlurMain *plugin);
	void process_package(LoadPackage *package);
	TimeBlurStripeEngine *server;
	TimeBlurMain *plugin;
};

class TimeBlurStripeEngine : public LoadServer
{
public:
	TimeBlurStripeEngine(TimeBlurMain *plugin, int total_clients, int total_packages);
	void process_packages(int operation);
	void init_packages();
	LoadClient *new_client();
	LoadPackage *new_package();
	TimeBlurMain *plugin;
	int operation;
};

#endif
