
/*
 * CINELERRA
 * Copyright (C) 2020 Adam Williams <broadcast at earthling dot net>
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

#include "bccmodels.h"
#include "filexml.h"
#include "language.h"
#include "colorspace.h"
#include "pluginserver.h"
#include "preferences.h"

#include <stdio.h>
#include <string.h>


REGISTER_PLUGIN(ColorSpaceMain)

ColorSpaceConfig::ColorSpaceConfig()
{
	inverse = 0;
	inp_colorspace = BC_COLORS_BT601_NTSC;
	inp_colorrange = BC_COLORS_JPEG;
	out_colorspace = BC_COLORS_BT709;
	out_colorrange = BC_COLORS_JPEG;
}

ColorSpaceMain::ColorSpaceMain(PluginServer *server)
 : PluginVClient(server)
{
	inp_color_space = -1;
	inp_color_range = -1;
	out_color_space = -1;
	out_color_range = -1;
	xtable = 0;
	engine = 0;
}

ColorSpaceMain::~ColorSpaceMain()
{
	delete xtable;
	delete engine;
}

const char* ColorSpaceMain::plugin_title() { return N_("ColorSpace"); }
int ColorSpaceMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(ColorSpaceMain, ColorSpaceWindow)


void ColorSpaceMain::update_gui()
{
	if( !thread ) return;
	load_configuration();
	ColorSpaceWindow *window = (ColorSpaceWindow *)thread->window;
	window->lock_window();
	window->update();
	window->unlock_window();
}


int ColorSpaceMain::load_configuration()
{
	KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 1;
}


void ColorSpaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("COLORSPACE");
	output.tag.set_property("INVERSE", config.inverse);
	output.tag.set_property("INP_COLORSPACE", config.inp_colorspace);
	output.tag.set_property("INP_COLORRANGE", config.inp_colorrange);
	output.tag.set_property("OUT_COLORSPACE", config.out_colorspace);
	output.tag.set_property("OUT_COLORRANGE", config.out_colorrange);
	output.append_tag();
	output.tag.set_title("/COLORSPACE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void ColorSpaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("COLORSPACE") ) {
			config.inverse = input.tag.
				get_property("INVERSE", config.inverse);
			config.inp_colorspace = input.tag.
				get_property("INP_COLORSPACE", config.inp_colorspace);
			config.inp_colorrange = input.tag.
				get_property("INP_COLORRANGE", config.inp_colorrange);
			config.out_colorspace = input.tag.
				get_property("OUT_COLORSPACE", config.out_colorspace);
			config.out_colorrange = input.tag.
				get_property("OUT_COLORRANGE", config.out_colorrange);
		}
	}
}

XTable::XTable()
{
	this->typ = -1;
 	this->len = 0;
	this->inv = 0;
	memset(eqns, 0, sizeof(eqns));
 	memset(luts, 0, sizeof(luts));
	for( int i=0; i<3; ++i ) {
		imin[i] = omin[i] = 0;  imax[i] = omax[i] = 0xff;
		izro[i] = ozro[i] = 0;  irng[i] = orng[i] = 0xff;
		izrf[i] = ozrf[i] = 0;  omnf[i] = 0;  omxf[i] = 1;
	}
}

XTable::~XTable()
{
	alloc_lut(0);
}

void XTable::create_table(lut_t **lut, int len, float *vars)
{
	int s = len == 0x100 ? (24-8) : (24-16);
	for( int i=0; i<3; ++i ) {
		lut_t imn = imin[i], imx = imax[i];
		lut_t omn = omin[i], omx = omax[i];
		double irng = imx+1 - imn;
		double orng = omx+1 - omn;
		double r = vars[i] * orng / irng;
		lut_t izr = izro[i], v = r*(imn-izr);
		lut_t *tbl = lut[i]; int k;
		for( k=0; (k<<s)<imn; ++k ) tbl[k] = v;
		for(  ; (v=k<<s)<imx; ++k ) tbl[k] = r*(v-izr);
		for( v=r*(imx-izr); k<len; ++k ) tbl[k] = v;
	}
}

void XTable::create_tables(int len)
{
	for( int i=0; i<3; ++i )
		create_table(luts[i], len, eqns[i]);
}


void XTable::alloc_lut(int len)
{
	if( this->len == len ) return;
	for( int i=0; i<3; ++i ) {
		for( int j=0; j<3; ++j ) {
			delete [] luts[i][j];
			luts[i][j] = len>0 ? new lut_t[len] : 0;
		}
	}
	this->len = len;
}

// these eqns were derived using python sympy
// and the conversion forms in bccolors.h
/*
>>> from sympy import *
>>> var('iKr,iKg,iKb,oKr,oKg,oKb,R,G,B,Y,U,V,Py,Pr,Pb')
(iKr, iKg, iKb, oKr, oKg, oKb, R, G, B, Y, U, V, Py, Pr, Pb)
>>>
>>> Y  =   iKr * R  +  iKg * G  +  iKb * B
>>> U  = - 0.5*iKr/(1-iKb)*R - 0.5*iKg/(1-iKb)*G + 0.5*B
>>> V  =   0.5*R - 0.5*iKg/(1-iKr)*G - 0.5*iKb/(1-iKr)*B
>>>
>>> r = Y + V * 2*(1-oKr)
>>> g = Y - V * 2*oKr*(1-oKr)/oKg - U * 2*oKb*(1-oKb)/oKg
>>> b = Y                         + U * 2*(1-oKb)
>>>
>>> factor(r,(R,G,B))
-1.0*(B*(1.0*iKb*iKr - 1.0*iKb*oKr) + G*(1.0*iKg*iKr - 1.0*iKg*oKr)
 + R*(1.0*iKr**2 - 1.0*iKr*oKr + 1.0*oKr - 1.0))/(1 - iKr)
>>> factor((1.0*iKr**2 - 1.0*iKr*oKr + 1.0*oKr - 1.0))
1.0*(iKr - 1)*(iKr - oKr + 1)
>>> factor((1.0*iKg*iKr - 1.0*iKg*oKr))
1.0*iKg*(iKr - oKr)
>>> factor((1.0*iKb*iKr - 1.0*iKb*oKr))
1.0*iKb*(iKr - oKr)
>>>
>>> factor(g,(R,G,B))       strlen(result)=778
results: eqn terms r*R + g*G + b*B, where r,g,b are the eqns coefs.
with some renaming, this can be done symetrically with Y,U,V
each coef eqn r,g,b can be reduced using factor([r,g,b])
which creates eqns forms that simplify to the used calculation
>>> factor(y,(Y,U,V))
results in: y*Y + u*U + v*V which y,u,v are the eqns factors
with same simplify and use
*/

//   yuv->(cs)->rgb->(cs)->yuv
int XTable::init_yuv2yuv(double iKr, double iKb, double oKr, double oKb)
{
	double iKg = 1 - iKr - iKb;
	double oKg = 1 - oKr - oKb;
	double d = iKg;
	Yy = iKg*(oKb + oKg + oKr) / d;
	Uy = 2*(iKb - 1)*(iKb*oKg - oKb*iKg) / d;
	Vy = -2*(iKr - 1)*(iKg*oKr - oKg*iKr) / d;
	d = (iKg*(1 - oKb));
	Yu = -0.5*iKg*(oKb + oKg + oKr - 1) / d;
	Uu = -(iKb - 1)*(iKb*oKg - oKb*iKg + iKg) / d;
	Vu = (iKr - 1)*(iKg*oKr - oKg*iKr) / d;
	d = (iKg*(1 - oKr));
	Yv = -0.5*iKg*(oKb + oKg + oKr - 1) / d;
	Uv = -(iKb - 1)*(iKb*oKg - oKb*iKg) / d;
	Vv = (iKr - 1)*(iKg*oKr - iKg - oKg*iKr) / d;
	return yuv2yuv;
}

//   rgb->(cs)->yuv
int XTable::init_rgb2yuv(double Kr, double Kb)
{
	double Kg = 1 - Kr - Kb;
	Ry = Kr;
	Gy = 1 - Kr - Kb;
	By = Kb;
	Ru = -0.5*Kr / (1 - Kb);
	Gu = -0.5*Kg / (1 - Kb);
	Bu =  0.5;
	Rv =  0.5;
	Gv = -0.5*Kg / (1 - Kr);
	Bv = -0.5*Kb / (1 - Kr);
	return rgb2yuv;
}

//   yuv->(cs)->rgb
int XTable::init_yuv2rgb(double Kr, double Kb)
{
	double Kg = 1 - Kr - Kb;
	Yr =  1.0;
	Ur =  2*(1 - Kr);
	Vr =  0.0;
	Yg =  1.0;
	Ug = -2*Kr*(1 - Kr) / Kg;
	Vg = -2*Kb*(1 - Kb) / Kg;
	Yb =  1.0;
	Ub =  0.0;
	Vb =  2*(1 - Kb);
	return yuv2rgb;
}

//   rgb->(cs)->yuv->(cs)->rgb
int XTable::init_rgb2rgb(double iKr, double iKb, double oKr, double oKb)
{
	double iKg = 1 - iKr - iKb;
	double oKg = 1 - oKr - oKb;
	double d = (1 - iKr);
	Rr = -(iKr - 1)*(iKr - oKr + 1) / d;
	Gr = -iKg*(iKr - oKr) / d;
	Br = -iKb*(iKr - oKr) / d;
	d = (oKg*(1 - iKb)*(1 - iKr));
	Rg = (iKr - 1)*(iKb*oKg*iKr + iKb*oKr*oKr - iKb*oKr +
		oKb*oKb*iKr - oKb*iKr - oKg*iKr - oKr*oKr + oKr) / d;
	Gg = iKg*(iKb*oKg*iKr - iKb*oKg + iKb*oKr*oKr - iKb*oKr +
		oKb*oKb*iKr - oKb*oKb - oKb*iKr + oKb - oKg*iKr +
		oKg - oKr*oKr + oKr) / d;
	Bg = (iKb - 1)*(iKb*oKg*iKr - iKb*oKg + iKb*oKr*oKr -
		 iKb*oKr + oKb*oKb*iKr - oKb*oKb - oKb*iKr + oKb) / d;
	d = (1 - iKb);
	Rb = -iKr*(iKb - oKb) / d;
	Gb = -iKg*(iKb - oKb) / d;
	Bb = -(iKb - 1)*(iKb - oKb + 1) / d;
	return rgb2rgb;
}

void XTable::init(int len, int inv,
		int inp_model, int inp_space, int inp_range,
		int out_model, int out_space, int out_range)
{
	if( this->typ >= 0 && this->len == len && this->inv == inv &&
	    this->inp_model == inp_model && this->out_model == out_model &&
	    this->inp_space == inp_space && this->out_space == out_space &&
	    this->inp_range == inp_range && this->out_range == out_range )
	   	 return;

	alloc_lut(len);
	this->inv = inv;
	this->inp_model = inp_model;  this->out_model = out_model;
	this->inp_space = inp_space;  this->out_space = out_space;
	this->inp_range = inp_range;  this->out_range = out_range;

	double iKr = BT601_NTSC_Kr, iKb = BT601_NTSC_Kb;
	double oKr = BT601_NTSC_Kr, oKb = BT601_NTSC_Kb;
	int impg = 0, ompg = 0;
	switch( inp_space ) {
	default:
	case BC_COLORS_BT601_NTSC:  iKr = BT601_NTSC_Kr;   iKb = BT601_NTSC_Kb;   break;
	case BC_COLORS_BT601_PAL:  iKr = BT601_PAL_Kr;   iKb = BT601_PAL_Kb;   break;
	case BC_COLORS_BT709:  iKr = BT709_Kr;   iKb = BT709_Kb;   break;
	case BC_COLORS_BT2020_NCL: 
	case BC_COLORS_BT2020_CL: iKr = BT2020_Kr;  iKb = BT2020_Kb;  break;
	}
	switch( out_space ) {
	default:
	case BC_COLORS_BT601_NTSC:  oKr = BT601_NTSC_Kr;   oKb = BT601_NTSC_Kb;   break;
	case BC_COLORS_BT601_PAL:  oKr = BT601_PAL_Kr;   oKb = BT601_PAL_Kb;   break;
	case BC_COLORS_BT709:  oKr = BT709_Kr;   oKb = BT709_Kb;   break;
	case BC_COLORS_BT2020_NCL: 
	case BC_COLORS_BT2020_CL: oKr = BT2020_Kr;  oKb = BT2020_Kb;  break;
	}

	int iyuv = BC_CModels::is_yuv(inp_model);
	int oyuv = BC_CModels::is_yuv(out_model);
	this->typ = iyuv ?
 		(oyuv ? init_yuv2yuv(iKr,iKb, oKr,oKb) :
			init_yuv2rgb(iKr,iKb)) :
		(oyuv ? init_rgb2yuv(oKr,oKb) :
			init_rgb2rgb(iKr,iKb, oKr, oKb));

	switch( inp_range ) {
	default:
	case BC_COLORS_JPEG: impg = 0;  break;
	case BC_COLORS_MPEG: impg = 1;  break;
	}
	switch( out_range ) {
	default:
	case BC_COLORS_JPEG: ompg = 0;  break;
	case BC_COLORS_MPEG: ompg = 1;  break;
	}

// mpg ? mpeg : jpeg/rgb
	imin[0] = impg ? 0x100000 : 0x000000;
	imin[1] = impg ? 0x100000 : 0x000000;
	imin[2] = impg ? 0x100000 : 0x000000;
	imax[0] = impg ? 0xebffff : 0xffffff;
	imax[1] = impg ? 0xf0ffff : 0xffffff;
	imax[2] = impg ? 0xf0ffff : 0xffffff;
	omin[0] = ompg ? 0x100000 : 0x000000;
	omin[1] = ompg ? 0x100000 : 0x000000;
	omin[2] = ompg ? 0x100000 : 0x000000;
	omax[0] = ompg ? 0xebffff : 0xffffff;
	omax[1] = ompg ? 0xf0ffff : 0xffffff;
	omax[2] = ompg ? 0xf0ffff : 0xffffff;
	izro[0] = imin[0];
	izro[1] = iyuv ? 0x800000 : imin[1];
	izro[2] = iyuv ? 0x800000 : imin[2];
	ozro[0] = omin[0];
	ozro[1] = oyuv ? 0x800000 : omin[1];
	ozro[2] = oyuv ? 0x800000 : omin[2];
	for( int i=0; i<3; ++i ) {
		irng[i] = imax[i]+1 - imin[i];
		orng[i] = omax[i]+1 - omin[i];
		int sz = 0x1000000;
		izrf[i] = (float)izro[i] / sz;
		ozrf[i] = (float)ozro[i] / sz;
		omnf[i] = (float)omin[i] / sz;
		omxf[i] = (float)(omax[i]+1) / sz;
	}
	if( inv )
		inverse();
	if( len > 0 )
		create_tables(len);
// prescale eqns for opengl
	for( int i=0; i<3; ++i ) {
		float *eqn = eqns[i];
		float s = (float)orng[i] / irng[i];
		for( int j=0; j<3; ++j ) eqn[j] *= s;
	}
#if 0
printf("XTable::init len=%06x\n"
 " impg=%d, ompg=%d, iyuv=%d, oyuv=%d\n"
 " imin=%06x,%06x,%06x, imax=%06x,%06x,%06x\n"
 " omin=%06x,%06x,%06x, omax=%06x,%06x,%06x\n"
 " izro=%06x,%06x,%06x, ozro=%06x,%06x,%06x\n"
 " izrf=%0.3f,%0.3f,%0.3f, ozrf=%0.3f,%0.3f,%0.3f\n"
 " omnf=%0.3f,%0.3f,%0.3f, omxf=%0.3f,%0.3f,%0.3f\n"
 " eqns= %6.3f,%6.3f,%6.3f\n"
 "       %6.3f,%6.3f,%6.3f\n"
 "       %6.3f,%6.3f,%6.3f\n",
 len, impg, ompg, iyuv, oyuv,
 imin[0], imin[1], imin[2], imax[0], imax[1], imax[2],
 omin[0], omin[1], omin[2], omax[0], omax[1], omax[2],
 izro[0], izro[1], izro[2], ozro[0], ozro[1], ozro[2],
 izrf[0], izrf[0], izrf[0], ozrf[0], ozrf[0], ozrf[0],
 omnf[0], omnf[0], omnf[0], omxf[0], omxf[0], omxf[0],
 eqns[0][0], eqns[0][1], eqns[0][2],
 eqns[1][0], eqns[1][1], eqns[1][2],
 eqns[2][0], eqns[2][1], eqns[2][2]);
#endif
}

/*
out = (inp-izro)*eqns + ozro
inverse: invert(eqns), swap(inp,out), swap(izro,ozro)
inp = (out-ozro)*iqns + izro
*/
int XTable::inverse()
{
// [[ a b c ],
//  [ d e f ],
//  [ g h i ]] inverse =
// 1/(a(ei-fh) + b(fg-di) + c(dh-eg)) *
//    [[ ei-fh, ch-bi, bf-ce ],
//     [ fg-di, ai-cg, cd-af ],
//     [ dh-eg, bg-ah, ae-bd ]]
	float a = eqns[0][0], b = eqns[0][1], c = eqns[0][2];
	float d = eqns[1][0], e = eqns[1][1], f = eqns[1][2];
	float g = eqns[2][0], h = eqns[2][1], i = eqns[2][2];
	float s = a*(e*i-f*h) + b*(f*g-d*i) + c*(d*h-e*g);
	float eps = 1e-4;
	if( s < eps ) return 1;
	s = 1.f / s;
	eqns[0][0] = s*(e*i-f*h);  eqns[0][1] = s*(c*h-b*i);  eqns[0][2] = s*(b*f-c*e);
	eqns[1][0] = s*(f*g-d*i);  eqns[1][1] = s*(a*i-c*g);  eqns[1][2] = s*(c*d-a*f);
	eqns[2][0] = s*(d*h-e*g);  eqns[2][1] = s*(b*g-a*h);  eqns[2][2] = s*(a*e-b*d);
	for( int v,i=0; i<3; ++i ) {
		v = imin[i];  imin[i] = omin[i];  omin[i] = v;
		v = imax[i];  imax[i] = omax[i];  omax[i] = v;
		v = irng[i];  irng[i] = orng[i];  orng[i] = v;
		v = izro[i];  izro[i] = ozro[i];  ozro[i] = v;
		int sz = 0x1000000;
		omnf[i] = (float)omin[i] / sz;
		omxf[i] = (float)(omax[i]+1) / sz;
		izrf[i] = (float)izro[i] / sz;
		ozrf[i] = (float)ozro[i] / sz;
	}
	return 0;
}


#define PROCESS_LUTS(type, comps) { \
	for( int y=row1; y<row2; ++y ) { \
		type *ip = (type*)irows[y]; \
		type *op = (type*)orows[y]; \
		for( int x=0; x<w; ++x ) { \
			for( int i=0; i<3; ++i ) { \
				lut_t omn = omin[i], omx = omax[i]; \
				lut_t **lut = luts[i], v = ozro[i]; \
				for( int j=0; j<3; ++j ) v += lut[j][ip[j]]; \
				op[i] = (v<omn ? omn : v>omx ? omx : v) >> s; \
			} \
			ip += comps;  op += comps; \
		} \
	} \
}

#define PROCESS_EQNS(type, comps) { \
	for( int y=row1; y<row2; ++y ) { \
		type *ip = (type*)irows[y]; \
		type *op = (type*)orows[y]; \
		for( int x=0; x<w; ++x ) { \
			for( int i=0; i<3; ++i ) { \
				float omn = omnf[i], omx = omxf[i]; \
				type v = ozrf[i];  float *eqn = eqns[i]; \
				for( int j=0; j<3; ++j ) v += eqn[j]*(ip[j] - izrf[j]); \
				op[i] = v<omn ? omn : v>omx ? omx : v; \
			} \
			ip += comps;  op += comps; \
		} \
	} \
}

void XTable::process(VFrame *inp, VFrame *out, int row1, int row2)
{
	int s = len == 0x100 ? (24-8) : (24-16);
	int w = inp->get_w(), comps = 3;
	uint8_t **irows = inp->get_rows();
	uint8_t **orows = out->get_rows();

	switch( inp->get_color_model() ) {
	case BC_RGBA8888:
	case BC_YUVA8888:
		comps = 4;
	case BC_RGB888:
	case BC_YUV888:
		PROCESS_LUTS(uint8_t, comps);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		comps = 4;
	case BC_RGB161616:
	case BC_YUV161616:
		PROCESS_LUTS(uint16_t, comps);
		break;
	case BC_RGBA_FLOAT:
		comps = 4;
	case BC_RGB_FLOAT:
		PROCESS_EQNS(float, comps);
		break;
	}
}

int ColorSpaceMain::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
//printf(" inv=%d, ispc=%d, irng=%d, ospc=%d, orng=%d\n", config.inverse,
// config.inp_colorspace, config.inp_colorrange,
// config.out_colorspace, config.out_colorrange);
	if( input->get_color_model() == output->get_color_model() &&
	    config.inp_colorspace == config.out_colorspace &&
	    config.inp_colorrange == config.out_colorrange )
		return 0;

	int color_model = input->get_color_model();
// opengl < 0, float == 0, tables > 0
	int len = get_use_opengl() ? -1 :
		BC_CModels::is_float(color_model) ? 0 :
		BC_CModels::calculate_pixelsize(color_model)/
		    BC_CModels::components(color_model) == 2 ?
			 0x10000 : 0x100;
	if( !engine && len >= 0 ) {
		int cpus = output->get_w()*output->get_h() / 0x80000 + 1;
  		int max_cpus = BC_Resources::machine_cpus / 2;
		if( cpus > max_cpus ) cpus = max_cpus;
		if( cpus < 1 ) cpus = 1;
		engine = new ColorSpaceEngine(this, cpus);
	}

	if( !xtable ) xtable = new XTable();
	xtable->init(len, config.inverse,
		color_model, config.inp_colorspace, config.inp_colorrange,
		color_model, config.out_colorspace, config.out_colorrange);
	inp = input;  out = output;
	if( get_use_opengl() )
		run_opengl();
	else
		engine->process_packages();
	return 0;
}


void ColorSpaceUnit::process_package(LoadPackage *package)
{
	ColorSpacePackage *pkg = (ColorSpacePackage*)package;
	plugin->xtable->process(plugin->inp, plugin->out, pkg->row1, pkg->row2);
}

ColorSpaceEngine::ColorSpaceEngine(ColorSpaceMain *plugin, int cpus)
// : LoadServer(1, 1)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void ColorSpaceEngine::init_packages()
{
	int row = 0, h = plugin->inp->get_h();
	for( int i=0, n=get_total_packages(); i<n; ) {
		ColorSpacePackage *pkg = (ColorSpacePackage*)get_package(i);
		pkg->row1 = row;
		pkg->row2 = row = (++i * h) / n;
	}
}

LoadClient* ColorSpaceEngine::new_client()
{
	return new ColorSpaceUnit(plugin, this);
}

LoadPackage* ColorSpaceEngine::new_package()
{
	return new ColorSpacePackage();
}

ColorSpacePackage::ColorSpacePackage()
 : LoadPackage()
{
}

ColorSpaceUnit::ColorSpaceUnit(ColorSpaceMain *plugin, ColorSpaceEngine *engine)
 : LoadClient(engine)
{
        this->plugin = plugin;
}



int ColorSpaceMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *colorspace_frag =
		"uniform sampler2D tex;\n"
		"uniform mat3 eqns;\n"
		"uniform vec3 izrf, ozrf;\n"
		"uniform vec3 omnf, omxf;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
		"	gl_FragColor.rgb = clamp((gl_FragColor.rgb-izrf)*eqns + ozrf, omnf, omxf);\n"
		"}\n";
	inp->to_texture();
	inp->enable_opengl();
	inp->bind_texture(0);

	unsigned int frag_shader = VFrame::make_shader(0, colorspace_frag, 0);
	if( xtable && frag_shader > 0 ) {
		glUseProgram(frag_shader);
		glUniform1i(glGetUniformLocation(frag_shader, "tex"), 0);
		glUniformMatrix3fv(glGetUniformLocation(frag_shader, "eqns"), 1, false, xtable->eqns[0]);
		glUniform3fv(glGetUniformLocation(frag_shader, "izrf"), 1, xtable->izrf);
		glUniform3fv(glGetUniformLocation(frag_shader, "ozrf"), 1, xtable->ozrf);
		glUniform3fv(glGetUniformLocation(frag_shader, "omnf"), 1, xtable->omnf);
		glUniform3fv(glGetUniformLocation(frag_shader, "omxf"), 1, xtable->omxf);
	}

	out->enable_opengl();
	VFrame::init_screen(out->get_w(), out->get_h());
	out->draw_texture();
	glUseProgram(0);
	out->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}

