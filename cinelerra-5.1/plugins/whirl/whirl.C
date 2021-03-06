
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



#include <stdint.h>
#include <string.h>

class WhirlEffect;
class WhirlWindow;
class WhirlEngine;
class WhirlFText;
class WhirlFSlider;
class WhirlReset;
class WhirlDefaultSettings;
class WhirlClr;

#define MAXRADIUS 100
#define MAXPINCH 100

#define RESET_DEFAULT_SETTINGS 10
#define RESET_ALL    0
#define RESET_RADIUS 1
#define RESET_PINCH  2
#define RESET_ANGLE  3

#define RADIUS_MIN   0.00
#define RADIUS_MAX 100.00
#define PINCH_MIN    0.00
#define PINCH_MAX  100.00
#define ANGLE_MIN    0.00
#define ANGLE_MAX  360.00



class WhirlConfig
{
public:
	WhirlConfig();
	void reset(int clear);

	void copy_from(WhirlConfig &src);
	int equivalent(WhirlConfig &src);
	void interpolate(WhirlConfig &prev,
		WhirlConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	float angle;
	float pinch;
	float radius;
};


class WhirlFText : public BC_TumbleTextBox
{
public:
	WhirlFText(WhirlWindow *window, WhirlEffect *plugin,
		WhirlFSlider *slider, float *output, int x, int y, float min, float max);
	~WhirlFText();
	int handle_event();
	WhirlWindow *window;
	WhirlEffect *plugin;
	WhirlFSlider *slider;
	float *output;
	float min, max;
};


class WhirlFSlider : public BC_FSlider
{
public:
	WhirlFSlider(WhirlEffect *plugin,
		WhirlFText *text, float *output, int x, int y,
		float min, float max);
	~WhirlFSlider();
	int handle_event();
	WhirlEffect *plugin;
	WhirlFText *text;
	float *output;
};


class WhirlReset : public BC_GenericButton
{
public:
	WhirlReset(WhirlEffect *plugin, WhirlWindow *window, int x, int y);
	~WhirlReset();
	int handle_event();
	WhirlEffect *plugin;
	WhirlWindow *window;
};

class WhirlDefaultSettings : public BC_GenericButton
{
public:
	WhirlDefaultSettings(WhirlEffect *plugin, WhirlWindow *window, int x, int y, int w);
	~WhirlDefaultSettings();
	int handle_event();
	WhirlEffect *plugin;
	WhirlWindow *window;
};

class WhirlClr : public BC_Button
{
public:
	WhirlClr(WhirlEffect *plugin, WhirlWindow *window, int x, int y, int clear);
	~WhirlClr();
	int handle_event();
	WhirlEffect *plugin;
	WhirlWindow *window;
	int clear;
};



class WhirlWindow : public PluginClientWindow
{
public:
	WhirlWindow(WhirlEffect *plugin);
	void create_objects();
	void update_gui(int clear);
	WhirlEffect *plugin;

	WhirlFText *radius_text;
	WhirlFSlider *radius_slider;
	WhirlClr *radius_Clr;

	WhirlFText *pinch_text;
	WhirlFSlider *pinch_slider;
	WhirlClr *pinch_Clr;

	WhirlFText *angle_text;
	WhirlFSlider *angle_slider;
	WhirlClr *angle_Clr;

	WhirlReset *reset;
	WhirlDefaultSettings *default_settings;
};





class WhirlPackage : public LoadPackage
{
public:
	WhirlPackage();
	int row1, row2;
};

class WhirlUnit : public LoadClient
{
public:
	WhirlUnit(WhirlEffect *plugin, WhirlEngine *server);
	void process_package(LoadPackage *package);
	WhirlEngine *server;
	WhirlEffect *plugin;

};


class WhirlEngine : public LoadServer
{
public:
	WhirlEngine(WhirlEffect *plugin, int cpus);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	WhirlEffect *plugin;
};



class WhirlEffect : public PluginVClient
{
public:
	WhirlEffect(PluginServer *server);
	~WhirlEffect();

	PLUGIN_CLASS_MEMBERS(WhirlConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	WhirlEngine *engine;
	VFrame *temp_frame;
	VFrame *input, *output;
	int need_reconfigure;
};






REGISTER_PLUGIN(WhirlEffect)














WhirlConfig::WhirlConfig()
{
	reset(RESET_ALL);
}

void WhirlConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ALL :
			radius = 0.0;
			pinch = 0.0;
			angle = 0.0;
			break;
		case RESET_RADIUS : radius = 0.0;
			break;
		case RESET_PINCH : pinch = 0.0;
			break;
		case RESET_ANGLE : angle = 0.0;
			break;
		case RESET_DEFAULT_SETTINGS :
		default:
			radius = 50.0;
			pinch = 10.0;
			angle = 180.0;
			break;
	}
}

void WhirlConfig::copy_from(WhirlConfig &src)
{
	this->angle = src.angle;
	this->pinch = src.pinch;
	this->radius = src.radius;
}

int WhirlConfig::equivalent(WhirlConfig &src)
{
	return EQUIV(this->angle, src.angle) &&
		EQUIV(this->pinch, src.pinch) &&
		EQUIV(this->radius, src.radius);
}

void WhirlConfig::interpolate(WhirlConfig &prev,
	WhirlConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pinch = prev.pinch * prev_scale + next.pinch * next_scale;
	this->radius = prev.radius * prev_scale + next.radius * next_scale;
}










WhirlWindow::WhirlWindow(WhirlEffect *plugin)
 : PluginClientWindow(plugin, xS(420), yS(160), xS(420), yS(160), 0)
{
	this->plugin = plugin;
}



void WhirlWindow::create_objects()
{
	int xs10 = xS(10), xs100 = xS(100);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22
	int defaultBtn_w = xs100;

	BC_Bar *bar;

	y += ys10;
	add_subwindow(new BC_Title(x, y, _("Radius:")));
	radius_text = new WhirlFText(this, plugin,
		0, &plugin->config.radius, (x + x2), y, RADIUS_MIN, RADIUS_MAX);
	radius_text->create_objects();
	radius_slider = new WhirlFSlider(plugin,
		radius_text, &plugin->config.radius, x3, y, RADIUS_MIN, RADIUS_MAX);
	add_subwindow(radius_slider);
	radius_text->slider = radius_slider;
	clr_x = x3 + radius_slider->get_w() + x;
	add_subwindow(radius_Clr = new WhirlClr(plugin, this, clr_x, y, RESET_RADIUS));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Pinch:")));
	pinch_text = new WhirlFText(this, plugin,
		0, &plugin->config.pinch, (x + x2), y, PINCH_MIN, PINCH_MAX);
	pinch_text->create_objects();
	pinch_slider = new WhirlFSlider(plugin,
		pinch_text, &plugin->config.pinch, x3, y, PINCH_MIN, PINCH_MAX);
	add_subwindow(pinch_slider);
	pinch_text->slider = pinch_slider;
	add_subwindow(pinch_Clr = new WhirlClr(plugin, this, clr_x, y, RESET_PINCH));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Angle:")));
	angle_text = new WhirlFText(this, plugin,
		0, &plugin->config.angle, (x + x2), y, ANGLE_MIN, ANGLE_MAX);
	angle_text->create_objects();
	angle_slider = new WhirlFSlider(plugin,
		angle_text, &plugin->config.angle, x3, y, ANGLE_MIN, ANGLE_MAX);
	add_subwindow(angle_slider);
	angle_text->slider = angle_slider;
	add_subwindow(angle_Clr = new WhirlClr(plugin, this, clr_x, y, RESET_ANGLE));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new WhirlReset(plugin, this, x, y));
	add_subwindow(default_settings = new WhirlDefaultSettings(plugin, this,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}


// for Reset button
void WhirlWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_RADIUS :
			radius_text->update(plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			break;
		case RESET_PINCH :
			pinch_text->update(plugin->config.pinch);
			pinch_slider->update(plugin->config.pinch);
			break;
		case RESET_ANGLE :
			angle_text->update(plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			radius_text->update(plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			pinch_text->update(plugin->config.pinch);
			pinch_slider->update(plugin->config.pinch);
			angle_text->update(plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			break;
	}
}






WhirlFText::WhirlFText(WhirlWindow *window, WhirlEffect *plugin,
	WhirlFSlider *slider, float *output, int x, int y, float min, float max)
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

WhirlFText::~WhirlFText()
{
}

int WhirlFText::handle_event()
{
	*output = atof(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


WhirlFSlider::WhirlFSlider(WhirlEffect *plugin,
	WhirlFText *text, float *output, int x, int y, float min, float max)
 : BC_FSlider(x, y, 0, xS(200), xS(200), min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

WhirlFSlider::~WhirlFSlider()
{
}

int WhirlFSlider::handle_event()
{
	*output = get_value();
	text->update(*output);
	plugin->send_configure_change();
	return 1;
}


WhirlReset::WhirlReset(WhirlEffect *plugin, WhirlWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
WhirlReset::~WhirlReset()
{
}
int WhirlReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}

WhirlDefaultSettings::WhirlDefaultSettings(WhirlEffect *plugin, WhirlWindow *window, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->window = window;
}
WhirlDefaultSettings::~WhirlDefaultSettings()
{
}
int WhirlDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	window->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}

WhirlClr::WhirlClr(WhirlEffect *plugin, WhirlWindow *window, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
WhirlClr::~WhirlClr()
{
}
int WhirlClr::handle_event()
{
	// clear==1 ==> Radius slider
	// clear==2 ==> Pinch slider
	// clear==3 ==> Angle slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}





WhirlEffect::WhirlEffect(PluginServer *server)
 : PluginVClient(server)
{
	need_reconfigure = 1;
	engine = 0;
	temp_frame = 0;

}

WhirlEffect::~WhirlEffect()
{

	if(engine) delete engine;
	if(temp_frame) delete temp_frame;
}






const char* WhirlEffect::plugin_title() { return N_("Whirl"); }
int WhirlEffect::is_realtime() { return 1; }



NEW_WINDOW_MACRO(WhirlEffect, WhirlWindow)


void WhirlEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((WhirlWindow*)thread->window)->angle_text->update(config.angle);
		((WhirlWindow*)thread->window)->angle_slider->update(config.angle);
		((WhirlWindow*)thread->window)->pinch_text->update(config.pinch);
		((WhirlWindow*)thread->window)->pinch_slider->update(config.pinch);
		((WhirlWindow*)thread->window)->radius_text->update(config.radius);
		((WhirlWindow*)thread->window)->radius_slider->update(config.radius);
		thread->window->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(WhirlEffect, WhirlConfig)




void WhirlEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("WHIRL");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("PINCH", config.pinch);
	output.tag.set_property("RADIUS", config.radius);
	output.append_tag();
	output.tag.set_title("/WHIRL");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void WhirlEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("WHIRL"))
			{
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.pinch = input.tag.get_property("PINCH", config.pinch);
				config.radius = input.tag.get_property("RADIUS", config.radius);
			}
		}
	}
}

int WhirlEffect::process_realtime(VFrame *input, VFrame *output)
{
	need_reconfigure |= load_configuration();
	this->input = input;
	this->output = output;

	if(EQUIV(config.angle, 0) ||
		(EQUIV(config.radius, 0) && EQUIV(config.pinch, 0)))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		if(input->get_rows()[0] == output->get_rows()[0])
		{
			if(!temp_frame) temp_frame = new VFrame(input->get_w(), input->get_h(),
				input->get_color_model(), 0);
			temp_frame->copy_from(input);
			this->input = temp_frame;
		}

		if(!engine) engine = new WhirlEngine(this, PluginClient::smp + 1);

		engine->process_packages();
	}
	return 0;
}








WhirlPackage::WhirlPackage()
 : LoadPackage()
{
}



WhirlUnit::WhirlUnit(WhirlEffect *plugin, WhirlEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}



static int calc_undistorted_coords(double cen_x,
			double cen_y,
			double scale_x,
			double scale_y,
			double radius,
			double radius2,
			double radius3,
			double pinch,
			double wx,
			double wy,
			double &whirl,
			double &x,
			double &y)
{
	double dx, dy;
	double d, factor;
	double dist;
	double ang, sina, cosa;
	int inside;

/* Distances to center, scaled */

	dx = (wx - cen_x) * scale_x;
	dy = (wy - cen_y) * scale_y;

/* Distance^2 to center of *circle* (scaled ellipse) */

	d = dx * dx + dy * dy;

/*  If we are inside circle, then distort.
 *  Else, just return the same position
 */

	inside = (d < radius2);

	if(inside)
    {
    	dist = sqrt(d / radius3) / radius;

/* Pinch */

    	factor = pow(sin(M_PI / 2 * dist), -pinch);

    	dx *= factor;
    	dy *= factor;

/* Whirl */

    	factor = 1.0 - dist;

    	ang = whirl * factor * factor;

    	sina = sin(ang);
    	cosa = cos(ang);

    	x = (cosa * dx - sina * dy) / scale_x + cen_x;
    	y = (sina * dx + cosa * dy) / scale_y + cen_y;
    }

	return inside;
}



#define GET_PIXEL(components, x, y, input_rows) \
	input_rows[CLIP(y, 0, (h - 1))] + components * CLIP(x, 0, (w - 1))






static float bilinear(double x, double y, double *values)
{
	double m0, m1;
	x = fmod(x, 1.0);
	y = fmod(y, 1.0);

	if(x < 0.0) x += 1.0;
	if(y < 0.0) y += 1.0;

	m0 = (double)values[0] + x * ((double)values[1] - values[0]);
	m1 = (double)values[2] + x * ((double)values[3] - values[2]);
	return m0 + y * (m1 - m0);
}





#define WHIRL_MACRO(type, max, components) \
{ \
	type **input_rows = (type**)plugin->input->get_rows(); \
/* Compiler error requires separate arrays */ \
	double top_values[4], bot_values[4]; \
	for( int i=0; i<4; ++i ) top_values[i] = bot_values[i] = 0; \
	for(int row = pkg->row1 / 2; row <= (pkg->row2 + pkg->row1) / 2; row++) \
	{ \
		type *top_row = (type*)plugin->output->get_rows()[row]; \
		type *bot_row = (type*)plugin->output->get_rows()[h - row - 1]; \
		type *top_p = top_row; \
		type *bot_p = bot_row + components * w - components; \
		type *pixel1; \
		type *pixel2; \
		type *pixel3; \
		type *pixel4; \
 \
		for(int col = 0; col < w; col++) \
		{ \
			if(calc_undistorted_coords(cen_x, \
				cen_y, \
				scale_x, \
				scale_y, \
				radius, \
				radius2, \
				radius3, \
				pinch, \
				col, \
				row, \
				whirl, \
				cx, \
				cy)) \
			{ \
/* Inside distortion area */ \
/* Do top */ \
				if(cx >= 0.0) \
					ix = (int)cx; \
				else \
					ix = -((int)-cx + 1); \
 \
				if(cy >= 0.0) \
					iy = (int)cy; \
				else \
					iy = -((int)-cy + 1); \
 \
				pixel1 = GET_PIXEL(components, ix,     iy,     input_rows); \
				pixel2 = GET_PIXEL(components, ix + 1, iy,     input_rows); \
				pixel3 = GET_PIXEL(components, ix,     iy + 1, input_rows); \
				pixel4 = GET_PIXEL(components, ix + 1, iy + 1, input_rows); \
 \
				top_values[0] = pixel1[0]; \
				top_values[1] = pixel2[0]; \
				top_values[2] = pixel3[0]; \
				top_values[3] = pixel4[0]; \
				top_p[0] = (type)bilinear(cx, cy, top_values); \
 \
				top_values[0] = pixel1[1]; \
				top_values[1] = pixel2[1]; \
				top_values[2] = pixel3[1]; \
				top_values[3] = pixel4[1]; \
				top_p[1] = (type)bilinear(cx, cy, top_values); \
 \
				top_values[0] = pixel1[2]; \
				top_values[1] = pixel2[2]; \
				top_values[2] = pixel3[2]; \
				top_values[3] = pixel4[2]; \
				top_p[2] = (type)bilinear(cx, cy, top_values); \
 \
				if(components == 4) \
				{ \
					top_values[0] = pixel1[3]; \
					top_values[1] = pixel2[3]; \
					top_values[2] = pixel3[3]; \
					top_values[3] = pixel4[3]; \
					top_p[3] = (type)bilinear(cx, cy, top_values); \
				} \
 \
				top_p += components; \
 \
/* Do bottom */ \
	    		cx = cen_x + (cen_x - cx); \
	    		cy = cen_y + (cen_y - cy); \
 \
	    		if(cx >= 0.0) \
					ix = (int)cx; \
	    		else \
					ix = -((int)-cx + 1); \
 \
	    		if(cy >= 0.0) \
					iy = (int)cy; \
	    		else \
					iy = -((int)-cy + 1); \
 \
				pixel1 = GET_PIXEL(components, ix,     iy,     input_rows); \
				pixel2 = GET_PIXEL(components, ix + 1, iy,     input_rows); \
				pixel3 = GET_PIXEL(components, ix,     iy + 1, input_rows); \
				pixel4 = GET_PIXEL(components, ix + 1, iy + 1, input_rows); \
 \
 \
 \
				bot_values[0] = pixel1[0]; \
				bot_values[1] = pixel2[0]; \
				bot_values[2] = pixel3[0]; \
				bot_values[3] = pixel4[0]; \
				bot_p[0] = (type)bilinear(cx, cy, bot_values); \
 \
				bot_values[0] = pixel1[1]; \
				bot_values[1] = pixel2[1]; \
				bot_values[2] = pixel3[1]; \
				bot_values[3] = pixel4[1]; \
				bot_p[1] = (type)bilinear(cx, cy, bot_values); \
 \
				bot_values[0] = pixel1[2]; \
				bot_values[1] = pixel2[2]; \
				bot_values[2] = pixel3[2]; \
				bot_values[3] = pixel4[2]; \
				bot_p[2] = (type)bilinear(cx, cy, bot_values); \
 \
				if(components == 4) \
				{ \
					bot_values[0] = pixel1[3]; \
					bot_values[1] = pixel2[3]; \
					bot_values[2] = pixel3[3]; \
					bot_values[3] = pixel4[3]; \
					bot_p[3] = (type)bilinear(cx, cy, bot_values); \
				} \
 \
				bot_p -= components; \
 \
 \
			} \
			else \
			{ \
/* Outside distortion area */ \
/* Do top */ \
				top_p[0] = input_rows[row][components * col + 0]; \
				top_p[1] = input_rows[row][components * col + 1]; \
				top_p[2] = input_rows[row][components * col + 2]; \
				if(components == 4) top_p[3] = input_rows[row][components * col + 3]; \
 \
 \
				top_p += components; \
 \
/* Do bottom */ \
				int bot_offset = w * components - col * components - components; \
				int bot_row = h - 1 - row; \
				bot_p[0] = input_rows[bot_row][bot_offset + 0]; \
				bot_p[1] = input_rows[bot_row][bot_offset + 1]; \
				bot_p[2] = input_rows[bot_row][bot_offset + 2]; \
				if(components == 4) bot_p[3] = input_rows[bot_row][bot_offset + 3]; \
				bot_p -= components; \
			} \
		} \
	} \
}

void WhirlUnit::process_package(LoadPackage *package)
{
	WhirlPackage *pkg = (WhirlPackage*)package;
	int w = plugin->input->get_w();
	int h = plugin->input->get_h();
	double whirl = plugin->config.angle * M_PI / 180;
	double pinch = plugin->config.pinch / MAXPINCH;
  	double cx, cy;
    int ix, iy;
	double cen_x = (double)(w-1) / 2.0;
	double cen_y = (double)(h-1) / 2.0;
	double radius = MAX(w, h);
	double radius3 = plugin->config.radius / MAXRADIUS;
  	double radius2 = radius * radius * radius3;
	double scale_x;
	double scale_y;


//printf("WhirlUnit::process_package 1 %f %f %f\n",
//	plugin->config.angle, plugin->config.pinch, plugin->config.radius);
	if(w < h)
	{
    	scale_x = (double)h / w;
    	scale_y = 1.0;
	}
	else
	if(w > h)
	{
    	scale_x = 1.0;
    	scale_y = (double)w / h;
	}
	else
	{
    	scale_x = 1.0;
    	scale_y = 1.0;
	}



	switch(plugin->input->get_color_model())
	{
		case BC_RGB_FLOAT:
			WHIRL_MACRO(float, 1, 3);
			break;
		case BC_RGB888:
		case BC_YUV888:
			WHIRL_MACRO(unsigned char, 0xff, 3);
			break;
		case BC_RGBA_FLOAT:
			WHIRL_MACRO(float, 1, 4);
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			WHIRL_MACRO(unsigned char, 0xff, 4);
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			WHIRL_MACRO(uint16_t, 0xffff, 3);
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			WHIRL_MACRO(uint16_t, 0xffff, 4);
			break;

	}
}







WhirlEngine::WhirlEngine(WhirlEffect *plugin, int cpus)
// : LoadServer(1, 1)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}
void WhirlEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		WhirlPackage *pkg = (WhirlPackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}

}

LoadClient* WhirlEngine::new_client()
{
	return new WhirlUnit(plugin, this);
}

LoadPackage* WhirlEngine::new_package()
{
	return new WhirlPackage;
}

