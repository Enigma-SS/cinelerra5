
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

#ifndef MASKAUTO_H
#define MASKAUTO_H


#include "arraylist.h"
#include "auto.h"
#include "maskauto.inc"
#include "maskautos.inc"

class MaskPoint
{
public:
	MaskPoint();

	int operator==(MaskPoint& ptr);
	MaskPoint& operator=(MaskPoint& ptr);
	void copy_from(MaskPoint &ptr);

	float x, y;
// Incoming acceleration
	float control_x1, control_y1;
// Outgoing acceleration
	float control_x2, control_y2;
};

class MaskCoord { public: double x, y, z; };

class MaskEdge : public ArrayList<MaskCoord>
{
public:
	MaskCoord &append(double x, double y, double z=0) {
		MaskCoord &c = ArrayList<MaskCoord>::append();
		c.x = x;  c.y = y;  c.z = z;
		return c;
	}
	void load(MaskPoints &points, float ofs);
};

class MaskEdges : public ArrayList<MaskEdge*> {
public:
	MaskEdges() {}
	~MaskEdges() { remove_all_objects(); }
};

class MaskPoints : public ArrayList<MaskPoint *>
{
public:
	void clear() { remove_all_objects(); }
	MaskPoints() {}
	~MaskPoints() { clear(); }
};

class MaskPointSets : public ArrayList<MaskPoints*>
{
public:
	void clear() { remove_all_objects(); }
	MaskPointSets() {}
	~MaskPointSets() { clear(); }
};

#define FEATHER_MAX 100
// GL reg limit 1024 incls shader param list
#define MAX_FEATHER 1000

class SubMask
{
public:
	SubMask(MaskAuto *keyframe, int no);
	~SubMask();

	int operator==(SubMask& ptr);
	int equivalent(SubMask& ptr);
	void copy_from(SubMask& ptr, int do_name=1);
	void load(FileXML *file);
	void copy(FileXML *file);
	void dump(FILE *fp);

	char name[BCSTRLEN];
	float fader;
	float feather;
	MaskPoints points;
	MaskAuto *keyframe;
};

class MaskAuto : public Auto
{
public:
	MaskAuto(EDL *edl, MaskAutos *autos);
	~MaskAuto();

	int operator==(Auto &that);
	int operator==(MaskAuto &that);
	bool is_maskauto() { return true; }
	int identical(MaskAuto *src);
	void load(FileXML *file);
	void copy(int64_t start, int64_t end, FileXML *file, int default_auto);
	void copy_from(Auto *src);
	int interpolate_from(Auto *a1, Auto *a2, int64_t position, Auto *templ=0);
	void copy_from(MaskAuto *src);
// Copy data but not position
	void copy_data(MaskAuto *src);
	void get_points(MaskPoints *points,
		int submask);
	void set_points(MaskPoints *points,
		int submask);

// Copy parameters to this which differ between ref & src
	void update_parameter(MaskAuto *ref, MaskAuto *src);

	void dump(FILE *fp);
// Retrieve submask with clamping
	SubMask* get_submask(int number);
// Translates all submasks
	void translate_submasks(float translate_x, float translate_y);
// scale all submasks
	void scale_submasks(int orig_scale, int new_scale);
	int has_active_mask();

	ArrayList<SubMask*> masks;
	int apply_before_plugins;
	int disable_opengl_masking;
};

// shader buffer unsized array vec only seems to work for dvec (05/2019)
class MaskSpot { public: double x, y; };

class MaskSpots : public ArrayList<MaskSpot>
{
public:
	MaskSpot &append() { return ArrayList<MaskSpot>::append(); }
	MaskSpot &append(double x, double y) {
		MaskSpot &s = append();
		s.x = x;  s.y = y;
		return s;
	}
};

#endif
