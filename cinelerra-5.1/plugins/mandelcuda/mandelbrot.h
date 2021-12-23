/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef MANDELCUDA_H
#define MANDELCUDA_H

#include "pluginvclient.h"

typedef struct {
    unsigned char x, y, z, w;
} uchar4;

#include "mandelcuda.h"

class MandelbrotConfig;
class Mandelbrot;

class MandelbrotConfig
{
public:
	MandelbrotConfig();

	int equivalent(MandelbrotConfig &that);
	void copy_from(MandelbrotConfig &that);
	void interpolate(MandelbrotConfig &prev, MandelbrotConfig &next, 
		long prev_frame, long next_frame, long current_frame);

	int is_julia;
	float x_off, y_off, scale;
	float x_julia, y_julia;
	int color, crunch, step;

	void limits();
	void reset();
	void startJulia();
};

class Mandelbrot : public PluginVClient
{
public:
	Mandelbrot(PluginServer *server);
	~Mandelbrot();
	PLUGIN_CLASS_MEMBERS2(MandelbrotConfig)
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int handle_opengl();

	void GetSample(int sampleIndex, float &x, float &y);
	void renderImage();
	void displayFunc();
	void initData();
	void init();

	VFrame *output, *vfrm;
	int color_model, pass;
	int img_w, img_h;
	int pbo_id;
	int animation_frame;
	MandelCuda cuda;

	void init_cuda();
	void finish_cuda();
};

#endif
