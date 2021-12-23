
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

#ifndef CROP_H
#define CROP_H

// the simplest plugin possible

class CropMain;

#include "bchash.h"
#include "mutex.h"
#include "cropwin.h"
#include "overlayframe.h"
#include "pluginvclient.h"

class CropConfig
{
public:
	CropConfig();
	void reset(int clear);
	int equivalent(CropConfig &that);
	void copy_from(CropConfig &that);
	void interpolate(CropConfig &prev,
		CropConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	float crop_l, crop_t, crop_r, crop_b;
	float position_x, position_y;
};


class CropMain : public PluginVClient
{
public:
	CropMain(PluginServer *server);
	~CropMain();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(CropConfig)
	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);


	OverlayFrame *overlayer;   // To translate images
	VFrame *temp_frame;
};


#endif
