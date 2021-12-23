
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

#include "bcsignals.h"
#include "condition.h"
#include "clip.h"
#include "maskauto.h"
#include "maskautos.h"
#include "maskengine.h"
#include "mutex.h"
#include "track.h"
#include "transportque.inc"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

void write_mask(VFrame *vfrm, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    uint8_t **rows = (uint8_t**)vfrm->get_rows();
    int w = vfrm->get_w(), h = vfrm->get_h();
    int m = vfrm->get_color_model();
    int bpp = m==BC_A8? 1 : 2;
    fprintf(fp,"P5\n%d %d\n%d\n",w,h,255);
    for( int y=0; y<h; ++y ) {
      uint8_t *bp = rows[y];
      for( int x=0; x<w; ++x,bp+=bpp ) {
        int b = m==BC_A8 ? *(uint8_t*)bp : *(uint16_t*)bp>>8;
        putc(b,fp);
      }
    }
    fflush(fp);
    if( fp != stdout ) fclose(fp);
  }
}

MaskPackage::MaskPackage()
{
}

MaskPackage::~MaskPackage()
{
}

MaskUnit::MaskUnit(MaskEngine *engine)
 : LoadClient(engine)
{
	this->engine = engine;
}

MaskUnit::~MaskUnit()
{
}

void MaskUnit::clear_mask(VFrame *msk, int a)
{
	temp_t **mrows = (temp_t **)msk->get_rows();
	int w = msk->get_w();
	for( int y=start_y; y<end_y; ++y ) {
		temp_t *mrow = mrows[y];
		for( int x=0; x<w; ++x ) mrow[x] = a;
	}
}

void MaskUnit::draw_line(int ix1, int iy1, int ix2, int iy2)
{
	if( iy1 == iy2 ) return;
	int x1 = iy1 < iy2 ? ix1 : ix2;
	int y1 = iy1 < iy2 ? iy1 : iy2;
	int x2 = iy1 < iy2 ? ix2 : ix1;
	int y2 = iy1 < iy2 ? iy2 : iy1;
	float slope = (float)(x2-x1) / (y2-y1);
	int dy = y1 - start_y;
	int i = dy < 0 ? (y1=start_y, -dy) : 0;
	if( y2 > end_y ) y2 = end_y;
	if( y2 < start_y || y1 >= end_y ) return;

	VFrame *in = engine->in;
	int w1 = in->get_w()-1;
	temp_t **irows = (temp_t **)in->get_rows();
	for( int y=y1; y<y2; ++i,++y ) {
		int x = (int)(i*slope + x1);
		bclamp(x, 0, w1);
		irows[y][x] = irows[y][x] == fc ? bc : fc;
	}
}
void MaskUnit::draw_fill()
{
	VFrame *in = engine->in;
	temp_t **irows = (temp_t**)in->get_rows();
	int w = in->get_w();

	for( int y=start_y; y<end_y; ++y ) {
		temp_t *irow = irows[y];
		int total = 0;
		for( int x=0; x<w; ++x )
			if( irow[x] == fc ) ++total;
		if( total < 2 ) continue;
		if( total & 0x1 ) --total;
		int inside = 0;
		for( int x=0; x<w; ++x ) {
			if( irow[x]==fc && total>0 ) {
				--total;
				inside = 1-inside;
			}
			else if( inside )
				irow[x] = fc;
		}
	}
}

void MaskUnit::draw_feather(int ix1,int iy1, int ix2,int iy2)
{
	int x1 = iy1 < iy2 ? ix1 : ix2;
	int y1 = iy1 < iy2 ? iy1 : iy2;
	int x2 = iy1 < iy2 ? ix2 : ix1;
	int y2 = iy1 < iy2 ? iy2 : iy1;
	VFrame *in = engine->in;
	int h = in->get_h();
	if( y2 < 0 || y1 >= h ) return;

	int x = x1, y = y1;
	int dx = x2-x1, dy = y2-y1;
	int dx2 = 2*dx, dy2 = 2*dy;
	if( dx < 0 ) dx = -dx;
	int m = dx > dy ? dx : dy, i = m;
	if( dy >= dx ) {
		if( dx2 >= 0 ) do {     /* +Y, +X */
			draw_edge(x, y++);
			if( (m -= dx2) < 0 ) { m += dy2;  ++x; }
		} while( --i >= 0 );
		else do {	       /* +Y, -X */
			draw_edge(x, y++);
			if( (m += dx2) < 0 ) { m += dy2;  --x; }
		} while( --i >= 0 );
	}
	else {
		if( dx2 >= 0 ) do {     /* +X, +Y */
			draw_edge(x++, y);
			if( (m -= dy2) < 0 ) { m += dx2;  ++y; }
		} while( --i >= 0 );
		else do {	       /* -X, +Y */
			draw_edge(x--, y);
			if( (m -= dy2) < 0 ) { m -= dx2;  ++y; }
		} while( --i >= 0 );
	}
}

void MaskUnit::draw_edge(int ix, int iy)
{
	if( iy < start_y || iy >= end_y ) return;
	VFrame *in = engine->in;
	temp_t **irows = (temp_t **)in->get_rows();
	irows[iy][ix] = fc;
}
void MaskUnit::draw_filled_polygon(MaskEdge &edge)
{
	for( int i=0; i<edge.size(); ++i ) {
		MaskCoord a = edge[i];
		MaskCoord b = i<edge.size()-1 ? edge[i+1] : edge[0];
		draw_line(a.x,a.y, b.x,b.y);
	}
	draw_fill();
}

void MaskUnit::feather_x(VFrame *in, VFrame *out)
{
	float *psf = engine->psf;  int psz = engine->psz;
	temp_t **orows = (temp_t**)out->get_rows();
	temp_t **irows = (temp_t**)in->get_rows();
	int w = in->get_w(), w1 = w-1;
	for( int y=start_y; y<end_y; ++y ) {
		temp_t *rp = irows[y]; 
		for( int x=0; x<w; ++x ) {
			temp_t c = rp[x], f = c * psf[0];
			for( int i=1; i<psz; ++i ) {
				int l = x-i;	if( l < 0 ) l = 0;
				int r = x+i;	if( r > w1 ) r = w1;
				temp_t g = bmax(rp[l], rp[r]) * psf[i];
				if( f < g ) f = g;
			}
			orows[y][x] = c > f ? c : f;
		}
	}
}
void MaskUnit::feather_y(VFrame *in, VFrame *out)
{
	float *psf = engine->psf;  int psz = engine->psz;
	temp_t **orows = (temp_t**)out->get_rows();
	temp_t **irows = (temp_t**)in->get_rows();
	int h = in->get_h(), h1 = h-1;
	for( int y=0; y<h; ++y ) {
		temp_t *rp = irows[y];
		for( int x=start_x; x<end_x; ++x ) {
			temp_t c = rp[x], f = c * psf[0];
			for( int i=1; i<psz; ++i ) {
				int d = y-i;	if( d < 0 ) d = 0;
				int u = y+i;	if( u > h1 ) u = h1;
				temp_t *drow = irows[d], *urow = irows[u];
				temp_t g = bmax(drow[x], urow[x]) * psf[i];
				if( f < g ) f = g;
			}
			orows[y][x] = c > f ? c : f;
		}
	}
}
void MaskUnit::mask_blend(VFrame *in, VFrame *mask, float r, float v)
{
	temp_t **irows = (temp_t**)in->get_rows();
	temp_t **mrows = (temp_t**)mask->get_rows();
	const int mn = 0x0000, mx = 0xffff;
	int iv = v>=0 ? 1 : -1;
	float rr = r!=0 ? r : 1;
	float rv = rr*v>=0. ? 1 : -1;;
	int vv = (v>=0. ? 1.-v : 1.+v) * mx;
	unsigned fg = rv>0. ? vv : mx;
	unsigned bg = rv>0. ? mx : vv;
	int w = in->get_w();
	int b = r<0 ? mx : mn;
	for( int y=start_y; y<end_y; ++y ) {
		temp_t *irow = irows[y], *mrow = mrows[y];
		for( int x=0; x<w; ++x ) {
			temp_t c = irow[x];
			if( c == b ) continue;
			temp_t a = (c*fg + (mx-c)*bg) / mx;
			temp_t *cp = mrow + x, color = *cp;
			if( iv*(color-a) > 0 ) *cp = a;
		}
	}
}
void MaskUnit::apply_mask_alpha(VFrame *output, VFrame *mask)
{
	int w = mask->get_w();
	uint8_t **orows = output->get_rows();
	temp_t **mrows = (temp_t **)mask->get_rows();
#define APPLY_MASK_ALPHA(cmodel, type, max, components, do_yuv) \
case cmodel: \
for( int y=start_y; y<end_y; ++y ) { \
	type *orow = (type*)orows[y]; \
	temp_t *mrow = mrows[y]; \
	for( int x=0; x<w; ++x ) { \
		temp_t a = mrow[x]; \
		if( components == 4 ) { \
			orow[x*4 + 3] = orow[x*4 + 3]*a / 0xffff; \
		} \
		else { \
			orow[x*3 + 0] = orow[x*3 + 0]*a / 0xffff; \
			orow[x*3 + 1] = orow[x*3 + 1]*a / 0xffff; \
			orow[x*3 + 2] = orow[x*3 + 2]*a / 0xffff; \
			if( do_yuv ) { \
				a = 0xffff-a; \
				type chroma_offset = (int)(max + 1) / 2; \
				orow[x*3 + 1] += chroma_offset*a / 0xffff; \
				orow[x*3 + 2] += chroma_offset*a / 0xffff; \
			} \
		} \
	} \
} break

	switch( engine->output->get_color_model() ) { \
	APPLY_MASK_ALPHA(BC_RGB888, uint8_t, 0xff, 3, 0); \
	APPLY_MASK_ALPHA(BC_RGB_FLOAT, float, 1.0, 3, 0); \
	APPLY_MASK_ALPHA(BC_YUV888, uint8_t, 0xff, 3, 1); \
	APPLY_MASK_ALPHA(BC_RGBA_FLOAT, float, 1.0, 4, 0); \
	APPLY_MASK_ALPHA(BC_YUVA8888, uint8_t, 0xff, 4, 1); \
	APPLY_MASK_ALPHA(BC_RGBA8888, uint8_t, 0xff, 4, 0); \
	APPLY_MASK_ALPHA(BC_RGB161616, uint16_t, 0xffff, 3, 0); \
	APPLY_MASK_ALPHA(BC_YUV161616, uint16_t, 0xffff, 3, 1); \
	APPLY_MASK_ALPHA(BC_YUVA16161616, uint16_t, 0xffff, 4, 1); \
	APPLY_MASK_ALPHA(BC_RGBA16161616, uint16_t, 0xffff, 4, 0); \
	}
}


void MaskUnit::process_package(LoadPackage *package)
{
	pkg = (MaskPackage*)package;
	start_x = pkg->start_x;  end_x = pkg->end_x;
	start_y = pkg->start_y;  end_y = pkg->end_y;
	MaskEdge *edge = engine->edge;
	float r = engine->r, v = engine->v;
	VFrame *in = engine->in;
	VFrame *out = engine->out;
	VFrame *mask = engine->mask;
	switch( engine->step ) {
	case DO_MASK: {
// Draw masked region of polygons on in
		if( edge->size() < 3 ) break;
		bc = r>=0 ? 0 : 0xffff;
		fc = r>=0 ? 0xffff : 0;
		clear_mask(in, bc);
		if( bc == fc ) break;
		draw_filled_polygon(*edge);
		break; }
	case DO_FEATHER_X: {
		feather_x(in, out);
		break; }
	case DO_FEATHER_Y: {
		feather_y(out, in);
		break; }
	case DO_MASK_BLEND: {
		mask_blend(in, mask, r, v);
		break; }
	case DO_APPLY: {
		apply_mask_alpha(engine->output, mask);
		break; }
	}
}


MaskEngine::MaskEngine(int cpus)
 : LoadServer(cpus, 2*cpus)
// : LoadServer(1, 1)
{
	mask = 0;
	in = 0;
	out = 0;
}

MaskEngine::~MaskEngine()
{
	delete mask;
	delete in;
	delete out;
}

int MaskEngine::points_equivalent(MaskPoints *new_points,
	MaskPoints *points)
{
//printf("MaskEngine::points_equivalent %d %d\n", new_points->total, points->total);
	if( new_points->total != points->total ) return 0;

	for( int i = 0; i < new_points->total; i++ ) {
		if( !(*new_points->get(i) == *points->get(i)) ) return 0;
	}

	return 1;
}

void MaskEngine::clear_mask(VFrame *msk, int a)
{
	temp_t **mrows = (temp_t **)msk->get_rows();
	int w = msk->get_w(), h = msk->get_h();
	for( int y=0; y<h; ++y ) {
		temp_t *mrow = mrows[y];
		for( int x=0; x<w; ++x ) mrow[x] = a;
	}
}
void MaskEngine::draw_point_spot(float r)
{
	double sig2 = -log(255.0)/((double)r*r);
	for( int i=0; i<psz; ++i )
		psf[i] = exp(i*i * sig2);
}

void MaskEngine::do_mask(VFrame *output,
	int64_t start_position_project,
	MaskAutos *keyframe_set,
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	this->output = output;
	recalculate = 0;
	int mask_model = 0;

	switch( output->get_color_model() ) {
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		mask_model = BC_A_FLOAT;
		break;

	case BC_RGB888:
	case BC_RGBA8888:
	case BC_YUV888:
	case BC_YUVA8888:
		mask_model = BC_A8;
		break;

	case BC_RGB161616:
	case BC_RGBA16161616:
	case BC_YUV161616:
	case BC_YUVA16161616:
		mask_model = BC_A16;
		break;
	}

// Determine if recalculation is needed
	int mask_w = output->get_w(), mask_h = output->get_h();
	if( mask && ( mask->get_color_model() != mask_model ||
	    mask->get_w() != mask_w || mask->get_h() != mask_h ) ) {
		delete mask;  mask = 0;
		recalculate = 1;
	}
	if( in && ( in->get_w() != mask_w || in->get_h() != mask_h ) ) {
		delete in;  in = 0;
		delete out; out = 0;
	}

	total_submasks = keyframe_set->total_submasks(start_position_project, PLAY_FORWARD);
	if( total_submasks != point_sets.size() )
		recalculate = 1;

	for( int i=0; i<total_submasks && !recalculate; ++i ) {
		float new_fader = keyframe_set->get_fader(start_position_project, i, PLAY_FORWARD);
		if( new_fader != faders[i] ) { recalculate = 1;  break; }
		float new_feather = keyframe_set->get_feather(start_position_project, i, PLAY_FORWARD);
		if( new_feather != feathers[i] ) { recalculate = 1;  break; }
		MaskPoints points;
		keyframe_set->get_points(&points, i,
				start_position_project, PLAY_FORWARD);
		if( !points_equivalent(&points, point_sets[i]) )
			recalculate = 1;
	}

	if( recalculate ) {
		if( !in ) in = new VFrame(mask_w, mask_h, BC_A16, 0);
		if( !out ) out = new VFrame(mask_w, mask_h, BC_A16, 0);
		if( !mask ) mask = new VFrame(mask_w, mask_h, BC_A16, 0);
		for( int i = 0; i < point_sets.total; i++ ) {
			MaskPoints *points = point_sets[i];
			points->remove_all_objects();
		}
		point_sets.remove_all_objects();
		edges.remove_all_objects();
		faders.remove_all();
		feathers.remove_all();

		float cc = 1;
		int show_mask = keyframe_set->track->masks;
		for( int i=0; i<total_submasks; ++i ) {
			float fader = keyframe_set->get_fader(start_position_project, i, PLAY_FORWARD);
			float v = fader / 100;
			faders.append(v);
			float feather = keyframe_set->get_feather(start_position_project, i, PLAY_FORWARD);
			feathers.append(feather);
			MaskPoints *points = new MaskPoints();
			keyframe_set->get_points(points, i, start_position_project, PLAY_FORWARD);
			point_sets.append(points);
			MaskEdge &edge = *edges.append(new MaskEdge());
			if( !fader || !((show_mask>>i) & 1) || !points->size() ) continue;
			edge.load(*points, 0);
			if( v >= 0 ) continue;
			float vv = 1 + v;
			if( cc > vv ) cc = vv;
		}
		clear_mask(mask, cc*0xffff);
// draw mask
		for( int k=0; k<edges.size(); ++k ) {
			this->edge = edges[k];
			this->r = feathers[k];
			this->v = faders[k];
			if( !this->v ) continue;
			if( this->edge->size() < 3 ) continue;
			float rr = fabsf(r);
			if( rr > 1024 ) rr = 1024; // MAX
			psf = new float[psz = rr+1];
			draw_point_spot(r);
//write_mask(mask, "/tmp/mask%d.pgm", k);
			step = DO_MASK;
			process_packages();
//write_mask(in, "/tmp/in0%d.pgm", k);
			step = DO_FEATHER_X;
			process_packages();
//write_mask(out, "/tmp/out1%d.pgm", k);
			step = DO_FEATHER_Y;
			process_packages();
			step = DO_MASK_BLEND;
			process_packages();
			delete [] psf;  psf = 0;
//write_mask(in, "/tmp/in2%d.pgm", k);
//printf("edge %d\n",k);
		}
	}
//write_mask(mask, "/tmp/mask.pgm");
		step = DO_APPLY;
		process_packages();
}

void MaskEngine::init_packages()
{
SET_TRACE
//printf("MaskEngine::init_packages 1\n");
	int x1 = 0, y1 = 0, i = 0, n = get_total_packages();
	int out_w = output->get_w(), out_h = output->get_h();
SET_TRACE
	while( i < n ) {
		MaskPackage *pkg = (MaskPackage*)get_package(i++);
		int x2 = (out_w * i) / n, y2 = (out_h * i) / n;
		pkg->start_x = x1;  pkg->end_x = x2;
		pkg->start_y = y1;  pkg->end_y = y2;
		x1 = x2;  y1 = y2;
	}
SET_TRACE
//printf("MaskEngine::init_packages 2\n");
}

LoadClient* MaskEngine::new_client()
{
	return new MaskUnit(this);
}

LoadPackage* MaskEngine::new_package()
{
	return new MaskPackage;
}

