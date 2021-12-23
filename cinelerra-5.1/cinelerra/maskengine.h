
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

#ifndef MASKENGINE_H
#define MASKENGINE_H


#include "condition.inc"
#include "loadbalance.h"
#include "maskautos.inc"
#include "maskauto.inc"
#include "mutex.inc"
#include "vframe.inc"

typedef uint16_t temp_t; // temp is A16

class MaskEngine;

// Values for step
enum
{
	DO_MASK,
	DO_FEATHER_X,
	DO_FEATHER_Y,
	DO_MASK_BLEND,
	DO_APPLY,
};


class MaskPackage : public LoadPackage
{
public:
	MaskPackage();
	~MaskPackage();

	int start_x, end_x;
	int start_y, end_y;
};

class MaskUnit : public LoadClient
{
public:
	MaskUnit(MaskEngine *engine);
	~MaskUnit();

	void clear_mask(VFrame *msk, int a);
	void draw_line(int x1, int y1, int x2, int y2);
	void draw_fill();
	void draw_feather(int ix1,int iy1, int ix2,int iy2);
	void draw_edge(int ix, int iy);
	void draw_filled_polygon(MaskEdge &edge);
	void feather_x(VFrame *in, VFrame *out);
	void feather_y(VFrame *in, VFrame *out);
	void mask_blend(VFrame *in, VFrame *mask, float r, float v);
	void apply_mask_alpha(VFrame *output, VFrame *mask);

	void process_package(LoadPackage *package);

	MaskEngine *engine;
	MaskPackage *pkg;
	int mask_model;
	int bc, fc;
	int start_x, end_x;
	int start_y, end_y;
};

class MaskEngine : public LoadServer
{
public:
	MaskEngine(int cpus);
	~MaskEngine();

	void do_mask(VFrame *output,
// Position relative to project, compensated for playback direction
		int64_t start_position_project,
		MaskAutos *keyframe_set,
		MaskAuto *keyframe,
		MaskAuto *default_auto);
	int points_equivalent(MaskPoints *new_points,
		MaskPoints *points);
	void clear_mask(VFrame *msk, int a);
	void draw_point_spot(float r);

	void delete_packages();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *mask, *in, *out;
	MaskEdges edges;
	MaskPointSets point_sets;
	ArrayList<float> faders;
	ArrayList<float> feathers;
	int step, total_submasks;
	int recalculate;
	MaskEdge *edge;
	float r, v;
	float *psf;
	int psz;
};



#endif
