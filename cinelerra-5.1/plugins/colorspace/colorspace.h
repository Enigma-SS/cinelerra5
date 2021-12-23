
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

#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

class ColorSpaceMain;

#include "bchash.h"
#include "mutex.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "colorspacewindow.h"
#include <sys/types.h>

class ColorSpaceConfig;
class ColorSpaceMain;
class ColorSpacePackage;
class ColorSpaceUnit;
class ColorSpaceEngine;

class XTable {
public:
	typedef int lut_t;
	enum lut_typ { yuv2yuv, rgb2yuv, yuv2rgb, rgb2rgb };

	XTable();
	~XTable();
	int init_yuv2yuv(double iKr, double iKb, double oKr, double oKb);
	int init_rgb2yuv(double Kr, double Kb);
	int init_yuv2rgb(double Kr, double Kb);
	int init_rgb2rgb(double iKr, double iKb, double oKr, double oKb);

	void init(int len, int inv,
		int inp_model, int inp_space, int inp_range,
		int out_model, int out_space, int out_range);
	void alloc_lut(int len);
	void create_table(lut_t **lut, int len, float *vars);
	void create_tables(int len);
	int inverse();
	void process(VFrame *inp, VFrame *out, int row1, int row2);

	int typ, len, inv;
	int inp_model, inp_space, inp_range;
	int out_model, out_space, out_range;

	lut_t imin[3], omin[3], imax[3], omax[3];
	lut_t izro[3], ozro[3], irng[3], orng[3];
	float izrf[3], ozrf[3], omnf[3], omxf[3];

	union {
		struct {
			float Yy, Uy, Vy;
			float Yu, Uu, Vu;
			float Yv, Uv, Vv;
		};
		struct {
			float Ry, Gy, By;
			float Ru, Gu, Bu;
			float Rv, Gv, Bv;
		};
		struct {
			float Yr, Ur, Vr;
			float Yg, Ug, Vg;
			float Yb, Ub, Vb;
		};
		struct {
			float Rr, Gr, Br;
			float Rg, Gg, Bg;
			float Rb, Gb, Bb;
		};
		float eqns[3][3];
	};
	lut_t *luts[3][3];
};


class ColorSpaceConfig
{
public:
	ColorSpaceConfig();
	int inverse;
	int inp_colorspace, inp_colorrange;
	int out_colorspace, out_colorrange;
};

class ColorSpacePackage : public LoadPackage
{
public:
	ColorSpacePackage();
	int row1, row2;
};

class ColorSpaceUnit : public LoadClient
{
public:
	ColorSpaceUnit(ColorSpaceMain *plugin, ColorSpaceEngine *engine);
	void process_package(LoadPackage *package);
	ColorSpaceEngine *engine;
	ColorSpaceMain *plugin;

};

class ColorSpaceEngine : public LoadServer
{
public:
	ColorSpaceEngine(ColorSpaceMain *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	ColorSpaceMain *plugin;
};


class ColorSpaceMain : public PluginVClient
{
public:
	ColorSpaceMain(PluginServer *server);
	~ColorSpaceMain();

	PLUGIN_CLASS_MEMBERS(ColorSpaceConfig);
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int handle_opengl();
	int create_luts();

	int inp_color_space, inp_color_range;
	int out_color_space, out_color_range;
	VFrame *inp, *out;
	XTable *xtable;
	ColorSpaceEngine *engine;
};


#endif
