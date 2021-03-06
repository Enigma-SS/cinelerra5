
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"



#include <string.h>
#include <stdint.h>


#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)

#define RESET_ALL   0
#define RESET_DEPTH 1
#define RESET_ANGLE 2

#define DEPTH_MIN   1.00
#define DEPTH_MAX 100.00
#define ANGLE_MIN   1.00
#define ANGLE_MAX 360.00


class PolarEffect;
class PolarEngine;
class PolarWindow;
class PolarFText;
class PolarFSlider;
class PolarReset;
class PolarClr;


class PolarConfig
{
public:
	PolarConfig();

	void reset(int clear);
	void copy_from(PolarConfig &src);
	int equivalent(PolarConfig &src);
	void interpolate(PolarConfig &prev,
		PolarConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	int polar_to_rectangular;
	float depth;
	float angle;
	int backwards;
	int invert;
};



class PolarFText : public BC_TumbleTextBox
{
public:
	PolarFText(PolarWindow *window, PolarEffect *plugin,
		PolarFSlider *slider, float *output, int x, int y, float min, float max);
	~PolarFText();
	int handle_event();
	PolarWindow *window;
	PolarEffect *plugin;
	PolarFSlider *slider;
	float *output;
	float min, max;
};

class PolarFSlider : public BC_FSlider
{
public:
	PolarFSlider(PolarEffect *plugin,
		PolarFText *text, float *output, int x, int y,
		float min, float max, int w);
	~PolarFSlider();
	int handle_event();
	PolarEffect *plugin;
	PolarFText *text;
	float *output;
};


class PolarReset : public BC_GenericButton
{
public:
	PolarReset(PolarEffect *plugin, PolarWindow *window, int x, int y);
	~PolarReset();
	int handle_event();
	PolarEffect *plugin;
	PolarWindow *window;
};

class PolarClr : public BC_Button
{
public:
	PolarClr(PolarEffect *plugin, PolarWindow *window, int x, int y, int clear);
	~PolarClr();
	int handle_event();
	PolarEffect *plugin;
	PolarWindow *window;
	int clear;
};

class PolarWindow : public PluginClientWindow
{
public:
	PolarWindow(PolarEffect *plugin);
	void create_objects();
	void update_gui(int clear);
	PolarEffect *plugin;

	PolarFText *depth_text;
	PolarFSlider *depth_slider;
	PolarClr *depth_Clr;

	PolarFText *angle_text;
	PolarFSlider *angle_slider;
	PolarClr *angle_Clr;

	PolarReset *reset;
};





class PolarPackage : public LoadPackage
{
public:
	PolarPackage();
	int row1, row2;
};

class PolarUnit : public LoadClient
{
public:
	PolarUnit(PolarEffect *plugin, PolarEngine *server);
	void process_package(LoadPackage *package);
	PolarEffect *plugin;
};

class PolarEngine : public LoadServer
{
public:
	PolarEngine(PolarEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	PolarEffect *plugin;
};

class PolarEffect : public PluginVClient
{
public:
	PolarEffect(PluginServer *server);
	~PolarEffect();

	PLUGIN_CLASS_MEMBERS(PolarConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PolarEngine *engine;
	VFrame *temp_frame;
	VFrame *input, *output;
	int need_reconfigure;
};



REGISTER_PLUGIN(PolarEffect)



PolarConfig::PolarConfig()
{
	reset(RESET_ALL);
}

void PolarConfig::reset(int clear)
{
	switch(clear) {
		case RESET_DEPTH : depth = 1.0;
			break;
		case RESET_ANGLE : angle = 1.0;
			break;
		case RESET_ALL :
		default:
			angle = 1.0;
			depth = 1.0;
			backwards = 0;
			invert = 0;
			polar_to_rectangular = 1;
			break;
	}
}

void PolarConfig::copy_from(PolarConfig &src)
{
	this->angle = src.angle;
	this->depth = src.depth;
	this->backwards = src.backwards;
	this->invert = src.invert;
	this->polar_to_rectangular = src.polar_to_rectangular;
}

int PolarConfig::equivalent(PolarConfig &src)
{
	return EQUIV(this->angle, src.angle) && EQUIV(this->depth, src.depth);
}

void PolarConfig::interpolate(PolarConfig &prev,
		PolarConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->depth = prev.depth * prev_scale + next.depth * next_scale;
	this->angle = prev.angle * prev_scale + next.angle * next_scale;
}






PolarWindow::PolarWindow(PolarEffect *plugin)
 : PluginClientWindow(plugin,
	xS(420),
	yS(130),
	xS(420),
	yS(130),
	0)
{
	this->plugin = plugin;
}

void PolarWindow::create_objects()
{
	int xs10 = xS(10), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Bar *bar;

	y += ys10;
	add_subwindow(new BC_Title(x, y, _("Depth:")));
	depth_text = new PolarFText(this, plugin,
		0, &plugin->config.depth, (x + x2), y, DEPTH_MIN, DEPTH_MAX);
	depth_text->create_objects();
	depth_slider = new PolarFSlider(plugin,
		depth_text, &plugin->config.depth, x3, y, DEPTH_MIN, DEPTH_MAX, xs200);
	add_subwindow(depth_slider);
	depth_text->slider = depth_slider;
	clr_x = x3 + depth_slider->get_w() + x;
	add_subwindow(depth_Clr = new PolarClr(plugin, this, clr_x, y, RESET_DEPTH));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Angle:")));
	angle_text = new PolarFText(this, plugin,
		0, &plugin->config.angle, (x + x2), y, ANGLE_MIN, ANGLE_MAX);
	angle_text->create_objects();
	angle_slider = new PolarFSlider(plugin,
		angle_text, &plugin->config.angle, x3, y, ANGLE_MIN, ANGLE_MAX, xs200);
	add_subwindow(angle_slider);
	angle_text->slider = angle_slider;
	add_subwindow(angle_Clr = new PolarClr(plugin, this, clr_x, y, RESET_ANGLE));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new PolarReset(plugin, this, x, y));

	show_window();
	flush();
}

// for Reset button
void PolarWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_DEPTH :
			depth_text->update(plugin->config.depth);
			depth_slider->update(plugin->config.depth);
			break;
		case RESET_ANGLE :
			angle_text->update(plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			break;
		case RESET_ALL :
		default:
			depth_text->update(plugin->config.depth);
			depth_slider->update(plugin->config.depth);
			angle_text->update(plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			break;
	}
}





PolarFText::PolarFText(PolarWindow *window, PolarEffect *plugin,
	PolarFSlider *slider, float *output, int x, int y, float min, float max)
 : BC_TumbleTextBox(window, *output,
	min, max, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
	this->output = output;
	this->slider = slider;
	this->min = min;
	this->max = max;
	set_increment(0.1);
}

PolarFText::~PolarFText()
{
}

int PolarFText::handle_event()
{
	*output = atof(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


PolarFSlider::PolarFSlider(PolarEffect *plugin,
	PolarFText *text, float *output, int x, int y, float min, float max, int w)
 : BC_FSlider(x, y, 0, w, w, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

PolarFSlider::~PolarFSlider()
{
}

int PolarFSlider::handle_event()
{
	*output = get_value();
	text->update(*output);
	plugin->send_configure_change();
	return 1;
}


PolarReset::PolarReset(PolarEffect *plugin, PolarWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
PolarReset::~PolarReset()
{
}
int PolarReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


PolarClr::PolarClr(PolarEffect *plugin, PolarWindow *window, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
PolarClr::~PolarClr()
{
}
int PolarClr::handle_event()
{
	// clear==1 ==> Depth slider
	// clear==2 ==> Angle slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}




PolarEffect::PolarEffect(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	temp_frame = 0;
	engine = 0;

}

PolarEffect::~PolarEffect()
{

	if(temp_frame) delete temp_frame;
	if(engine) delete engine;
}



const char* PolarEffect::plugin_title() { return N_("Polar"); }
int PolarEffect::is_realtime() { return 1; }




NEW_WINDOW_MACRO(PolarEffect, PolarWindow)

void PolarEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((PolarWindow*)thread->window)->angle_text->update(config.angle);
		((PolarWindow*)thread->window)->angle_slider->update(config.angle);
		((PolarWindow*)thread->window)->depth_text->update(config.depth);
		((PolarWindow*)thread->window)->depth_slider->update(config.depth);
		thread->window->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(PolarEffect, PolarConfig)



void PolarEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("POLAR");
	output.tag.set_property("DEPTH", config.depth);
	output.tag.set_property("ANGLE", config.angle);
	output.append_tag();
	output.tag.set_title("/POLAR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void PolarEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("POLAR"))
		{
			config.depth = input.tag.get_property("DEPTH", config.depth);
			config.angle = input.tag.get_property("ANGLE", config.angle);
		}
	}
}

int PolarEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();

	this->input = input;
	this->output = output;

	if(EQUIV(config.depth, 0) || EQUIV(config.angle, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		if(input->get_rows()[0] == output->get_rows()[0])
		{
			if(!temp_frame)
				temp_frame = new VFrame(input->get_w(), input->get_h(),
					input->get_color_model(), 0);
			temp_frame->copy_from(input);
			this->input = temp_frame;
		}


		if(!engine) engine = new PolarEngine(this, PluginClient::smp + 1);

		engine->process_packages();
	}
	return 0;
}





PolarPackage::PolarPackage()
 : LoadPackage()
{
}




PolarUnit::PolarUnit(PolarEffect *plugin, PolarEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}


static int calc_undistorted_coords(int wx,
			 int wy,
			 int w,
			 int h,
			 float depth,
			 double angle,
			 int polar_to_rectangular,
			 int backwards,
			 int inverse,
			 double cen_x,
			 double cen_y,
			 double &x,
			 double &y)
{
	int inside;
	double phi, phi2;
	double xx, xm, ym, yy;
	int xdiff, ydiff;
	double r;
	double m;
	double xmax, ymax, rmax;
	double x_calc, y_calc;
	double xi, yi;
	double circle, angl, t;
	int x1, x2, y1, y2;

/* initialize */

	phi = 0.0;
	r = 0.0;

	x1 = 0;
	y1 = 0;
	x2 = w;
	y2 = h;
	xdiff = x2 - x1;
	ydiff = y2 - y1;
	xm = xdiff / 2.0;
	ym = ydiff / 2.0;
	circle = depth;
	angle = angle;
	angl = (double)angle / 180.0 * M_PI;

    if(polar_to_rectangular)
    {
        if(wx >= cen_x)
		{
	  		if(wy > cen_y)
	    	{
	      		phi = M_PI -
					atan(((double)(wx - cen_x)) /
					((double)(wy - cen_y)));
	      		r   = sqrt(SQR(wx - cen_x) +
					SQR(wy - cen_y));
	    	}
	  		else
			if(wy < cen_y)
	    	{
	    		phi = atan(((double)(wx - cen_x)) /
					((double)(cen_y - wy)));
	    		r   = sqrt(SQR(wx - cen_x) +
					SQR(cen_y - wy));
	    	}
			else
	    	{
	    		phi = M_PI / 2;
	    		r   = wx - cen_x;
	    	}
		}
      	else
		if(wx < cen_x)
		{
			if(wy < cen_y)
	    	{
	    		phi = 2 * M_PI -
					atan(((double)(cen_x -wx)) /
				    ((double)(cen_y - wy)));
	    		r   = sqrt(SQR(cen_x - wx) +
					SQR(cen_y - wy));
	    	}
			else
			if(wy > cen_y)
	    	{
	    		phi = M_PI +
					atan(((double)(cen_x - wx)) /
					((double)(wy - cen_y)));
	    		r   = sqrt(SQR(cen_x - wx) +
					SQR(wy - cen_y));
	    	}
			else
	    	{
	    		phi = 1.5 * M_PI;
	    		r   = cen_x - wx;
	    	}
		}
      	if (wx != cen_x)
		{
			m = fabs(((double)(wy - cen_y)) /
				((double)(wx - cen_x)));
		}
    	else
		{
		    m = 0;
		}

    	if(m <= ((double)(y2 - y1) /
			(double)(x2 - x1)))
		{
			if(wx == cen_x)
	    	{
	    		xmax = 0;
	    		ymax = cen_y - y1;
	    	}
			else
	    	{
	    		xmax = cen_x - x1;
	    		ymax = m * xmax;
	    	}
		}
    	else
		{
			ymax = cen_y - y1;
			xmax = ymax / m;
		}

    	rmax = sqrt((double)(SQR(xmax) + SQR(ymax)));

    	t = ((cen_y - y1) < (cen_x - x1)) ? (cen_y - y1) : (cen_x - x1);
    	rmax = (rmax - t) / 100 * (100 - circle) + t;

    	phi = fmod(phi + angl, 2 * M_PI);

    	if(backwards)
			x_calc = x2 - 1 - (x2 - x1 - 1) / (2 * M_PI) * phi;
    	else
			x_calc = (x2 - x1 - 1) / (2 * M_PI) * phi + x1;

    	if(inverse)
			y_calc = (y2 - y1) / rmax * r + y1;
    	else
			y_calc = y2 - (y2 - y1) / rmax * r;

    	xi = (int)(x_calc + 0.5);
    	yi = (int)(y_calc + 0.5);

    	if(WITHIN(0, xi, w - 1) && WITHIN(0, yi, h - 1))
		{
			x = x_calc;
			y = y_calc;

			inside = 1;
		}
    	else
		{
			inside = 0;
		}
    }
	else
    {
    	if(backwards)
			phi = (2 * M_PI) * (x2 - wx) / xdiff;
    	else
			phi = (2 * M_PI) * (wx - x1) / xdiff;

    	phi = fmod (phi + angl, 2 * M_PI);

    	if(phi >= 1.5 * M_PI)
			phi2 = 2 * M_PI - phi;
    	else
		if (phi >= M_PI)
			phi2 = phi - M_PI;
		else
		if(phi >= 0.5 * M_PI)
	    	phi2 = M_PI - phi;
		else
	        phi2 = phi;

    	xx = tan (phi2);
    	if(xx != 0)
			m = (double)1.0 / xx;
    	else
			m = 0;

    	if(m <= ((double)(ydiff) / (double)(xdiff)))
		{
			if(phi2 == 0)
	    	{
	    		xmax = 0;
	    		ymax = ym - y1;
	    	}
			else
	    	{
	    		xmax = xm - x1;
	    		ymax = m * xmax;
	    	}
		}
    	else
		{
			ymax = ym - y1;
			xmax = ymax / m;
		}

    	rmax = sqrt((double)(SQR(xmax) + SQR(ymax)));

    	t = ((ym - y1) < (xm - x1)) ? (ym - y1) : (xm - x1);

    	rmax = (rmax - t) / 100.0 * (100 - circle) + t;

    	if(inverse)
			r = rmax * (double)((wy - y1) / (double)(ydiff));
    	else
			r = rmax * (double)((y2 - wy) / (double)(ydiff));

    	xx = r * sin (phi2);
    	yy = r * cos (phi2);

    	if(phi >= 1.5 * M_PI)
		{
			x_calc = (double)xm - xx;
			y_calc = (double)ym - yy;
		}
    	else
		if(phi >= M_PI)
		{
	    	x_calc = (double)xm - xx;
	    	y_calc = (double)ym + yy;
		}
		else
		if(phi >= 0.5 * M_PI)
	    {
	    	x_calc = (double)xm + xx;
	    	y_calc = (double)ym + yy;
	    }
		else
	    {
	    	x_calc = (double)xm + xx;
	    	y_calc = (double)ym - yy;
	    }

    	xi = (int)(x_calc + 0.5);
    	yi = (int)(y_calc + 0.5);

    	if(WITHIN(0, xi, w - 1) &&
			WITHIN(0, yi, h - 1))
		{
			x = x_calc;
			y = y_calc;

			inside = 1;
    	}
    	else
		{
			inside = 0;
		}
    }

	return inside;
}

static double bilinear(double x, double y, double *values)
{
	double m0, m1;
	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if(x < 0.0) x += 1.0;
	if(y < 0.0) y += 1.0;

	m0 = values[0] + x * (values[1] - values[0]);
	m1 = values[2] + x * (values[3] - values[2]);
	return m0 + y * (m1 - m0);
}

#define GET_PIXEL(x, y, components, input_rows) \
	input_rows[CLIP((y), 0, ((h) - 1))] + components * CLIP((x), 0, ((w) - 1))

#define POLAR_MACRO(type, max, components, chroma_offset) \
{ \
	type **in_rows = (type**)plugin->input->get_rows(); \
	type **out_rows = (type**)plugin->output->get_rows(); \
	double values[4]; \
 \
	for(int y = pkg->row1; y < pkg->row2; y++) \
	{ \
		type *output_row = out_rows[y]; \
 \
		for(int x = 0; x < w; x++) \
		{ \
			type *output_pixel = output_row + x * components; \
			if(calc_undistorted_coords(x, \
				y, \
				w, \
				h, \
				plugin->config.depth, \
				plugin->config.angle, \
				plugin->config.polar_to_rectangular, \
				plugin->config.backwards, \
				plugin->config.invert, \
				cen_x, \
				cen_y, \
				cx, \
				cy)) \
			{ \
				type *pixel1 = GET_PIXEL((int)cx,     (int)cy,	 components, in_rows); \
				type *pixel2 = GET_PIXEL((int)cx + 1, (int)cy,	 components, in_rows); \
				type *pixel3 = GET_PIXEL((int)cx,     (int)cy + 1, components, in_rows); \
				type *pixel4 = GET_PIXEL((int)cx + 1, (int)cy + 1, components, in_rows); \
 \
				values[0] = pixel1[0]; \
				values[1] = pixel2[0]; \
				values[2] = pixel3[0]; \
				values[3] = pixel4[0]; \
				output_pixel[0] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[1]; \
				values[1] = pixel2[1]; \
				values[2] = pixel3[1]; \
				values[3] = pixel4[1]; \
				output_pixel[1] = (type)bilinear(cx, cy, values); \
 \
				values[0] = pixel1[2]; \
				values[1] = pixel2[2]; \
				values[2] = pixel3[2]; \
				values[3] = pixel4[2]; \
				output_pixel[2] = (type)bilinear(cx, cy, values); \
 \
				if(components == 4) \
				{ \
					values[0] = pixel1[3]; \
					values[1] = pixel2[3]; \
					values[2] = pixel3[3]; \
					values[3] = pixel4[3]; \
					output_pixel[3] = (type)bilinear(cx, cy, values); \
				} \
			} \
			else \
			{ \
				output_pixel[0] = 0; \
				output_pixel[1] = chroma_offset; \
				output_pixel[2] = chroma_offset; \
				if(components == 4) output_pixel[3] = max; \
			} \
		} \
	} \
}


void PolarUnit::process_package(LoadPackage *package)
{
	PolarPackage *pkg = (PolarPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	double cx;
	double cy;
	double cen_x = (double)(w - 1) / 2.0;
	double cen_y = (double)(h - 1) / 2.0;

	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			POLAR_MACRO(float, 1, 3, 0x0)
			break;
		case BC_RGBA_FLOAT:
			POLAR_MACRO(float, 1, 4, 0x0)
			break;
		case BC_RGB888:
			POLAR_MACRO(unsigned char, 0xff, 3, 0x0)
			break;
		case BC_RGBA8888:
			POLAR_MACRO(unsigned char, 0xff, 4, 0x0)
			break;
		case BC_RGB161616:
			POLAR_MACRO(uint16_t, 0xffff, 3, 0x0)
			break;
		case BC_RGBA16161616:
			POLAR_MACRO(uint16_t, 0xffff, 4, 0x0)
			break;
		case BC_YUV888:
			POLAR_MACRO(unsigned char, 0xff, 3, 0x80)
			break;
		case BC_YUVA8888:
			POLAR_MACRO(unsigned char, 0xff, 4, 0x80)
			break;
		case BC_YUV161616:
			POLAR_MACRO(uint16_t, 0xffff, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			POLAR_MACRO(uint16_t, 0xffff, 4, 0x8000)
			break;
	}
}




PolarEngine::PolarEngine(PolarEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}

void PolarEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		PolarPackage *pkg = (PolarPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}

LoadClient* PolarEngine::new_client()
{
	return new PolarUnit(plugin, this);
}

LoadPackage* PolarEngine::new_package()
{
	return new PolarPackage;
}



