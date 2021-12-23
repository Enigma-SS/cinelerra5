
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
#include "language.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>

#define MAXVALUE 100

#define RESET_ALL 0
#define RESET_Y_SLIDER 1
#define RESET_U_SLIDER 2
#define RESET_V_SLIDER 3

class YUVEffect;
class YUVWindow;
class YUVFText;
class YUVFSlider;
class YUVReset;
class YUVClr;


class YUVConfig
{
public:
	YUVConfig();
	void reset(int clear);

	void copy_from(YUVConfig &src);
	int equivalent(YUVConfig &src);
	void interpolate(YUVConfig &prev,
		YUVConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame);

	float y, u, v;
};




class YUVFText : public BC_TumbleTextBox
{
public:
	YUVFText(YUVWindow *window, YUVEffect *plugin,
		YUVFSlider *slider, float *output, int x, int y, float min, float max);
	~YUVFText();
	int handle_event();
	YUVWindow *window;
	YUVEffect *plugin;
	YUVFSlider *slider;
	float *output;
	float min, max;
};


class YUVFSlider : public BC_FSlider
{
public:
	YUVFSlider(YUVEffect *plugin, YUVFText *text, float *output, int x, int y);
	~YUVFSlider();
	int handle_event();
	YUVEffect *plugin;
	YUVFText *text;
	float *output;
};


class YUVReset : public BC_GenericButton
{
public:
	YUVReset(YUVEffect *plugin, YUVWindow *window, int x, int y);
	~YUVReset();
	int handle_event();
	YUVEffect *plugin;
	YUVWindow *window;
};

class YUVClr : public BC_Button
{
public:
	YUVClr(YUVEffect *plugin, YUVWindow *window, int x, int y, int clear);
	~YUVClr();
	int handle_event();
	YUVEffect *plugin;
	YUVWindow *window;
	int clear;
};

class YUVWindow : public PluginClientWindow
{
public:
	YUVWindow(YUVEffect *plugin);
	void create_objects();
	void update_gui(int clear);

	YUVFText *y_text;
	YUVFSlider *y_slider;
	YUVClr *y_Clr;

	YUVFText *u_text;
	YUVFSlider *u_slider;
	YUVClr *u_Clr;

	YUVFText *v_text;
	YUVFSlider *v_slider;
	YUVClr *v_Clr;

	YUVEffect *plugin;
	YUVReset *reset;
};



class YUVEffect : public PluginVClient
{
public:
	YUVEffect(PluginServer *server);
	~YUVEffect();


	PLUGIN_CLASS_MEMBERS(YUVConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
};





REGISTER_PLUGIN(YUVEffect)







YUVConfig::YUVConfig()
{
	reset(RESET_ALL);
}

void YUVConfig::reset(int clear)
{
	switch(clear) {
		case RESET_Y_SLIDER : y = 0;
			break;
		case RESET_U_SLIDER : u = 0;
			break;
		case RESET_V_SLIDER : v = 0;
			break;
		case RESET_ALL :
		default:
			y = u = v = 0;
			break;
	}
}

void YUVConfig::copy_from(YUVConfig &src)
{
	y = src.y;
	u = src.u;
	v = src.v;
}

int YUVConfig::equivalent(YUVConfig &src)
{
	return EQUIV(y, src.y) && EQUIV(u, src.u) && EQUIV(v, src.v);
}

void YUVConfig::interpolate(YUVConfig &prev,
	YUVConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	y = prev.y * prev_scale + next.y * next_scale;
	u = prev.u * prev_scale + next.u * next_scale;
	v = prev.v * prev_scale + next.v * next_scale;
}








YUVFText::YUVFText(YUVWindow *window, YUVEffect *plugin,
	YUVFSlider *slider, float *output, int x, int y, float min, float max)
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

YUVFText::~YUVFText()
{
}

int YUVFText::handle_event()
{
	*output = atof(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


YUVFSlider::YUVFSlider(YUVEffect *plugin, YUVFText *text, float *output, int x, int y)
 : BC_FSlider(x, y, 0, xS(200), xS(200), -MAXVALUE, MAXVALUE, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

YUVFSlider::~YUVFSlider()
{
}

int YUVFSlider::handle_event()
{
	*output = get_value();
	text->update(*output);
	plugin->send_configure_change();
	return 1;
}


YUVReset::YUVReset(YUVEffect *plugin, YUVWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
YUVReset::~YUVReset()
{
}
int YUVReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


YUVClr::YUVClr(YUVEffect *plugin, YUVWindow *window, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
YUVClr::~YUVClr()
{
}
int YUVClr::handle_event()
{
	// clear==1 ==> Y slider
	// clear==2 ==> U slider
	// clear==3 ==> V slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}


YUVWindow::YUVWindow(YUVEffect *plugin)
 : PluginClientWindow(plugin, xS(420), yS(160), xS(420), yS(160), 0)
{
	this->plugin = plugin;
}

void YUVWindow::create_objects()
{
	int xs10 = xS(10);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Bar *bar;

	y += ys10;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y_text = new YUVFText(this, plugin,
		0, &plugin->config.y, (x + x2), y, -MAXVALUE, MAXVALUE);
	y_text->create_objects();
	y_slider = new YUVFSlider(plugin, y_text, &plugin->config.y, x3, y);
	add_subwindow(y_slider);
	y_text->slider = y_slider;
	clr_x = x3 + y_slider->get_w() + x;
	add_subwindow(y_Clr = new YUVClr(plugin, this, clr_x, y, RESET_Y_SLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("U:")));
	u_text = new YUVFText(this, plugin,
		0, &plugin->config.u, (x + x2), y, -MAXVALUE, MAXVALUE);
	u_text->create_objects();
	u_slider = new YUVFSlider(plugin, u_text, &plugin->config.u, x3, y);
	add_subwindow(u_slider);
	u_text->slider = u_slider;
	add_subwindow(u_Clr = new YUVClr(plugin, this, clr_x, y, RESET_U_SLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("V:")));
	v_text = new YUVFText(this, plugin,
		0, &plugin->config.v, (x + x2), y, -MAXVALUE, MAXVALUE);
	v_text->create_objects();
	v_slider = new YUVFSlider(plugin, v_text, &plugin->config.v, x3, y);
	add_subwindow(v_slider);
	v_text->slider = v_slider;
	add_subwindow(v_Clr = new YUVClr(plugin, this, clr_x, y, RESET_V_SLIDER));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new YUVReset(plugin, this, x, y));

	show_window();
	flush();
}


// for Reset button
void YUVWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_Y_SLIDER :
			y_text->update(plugin->config.y);
			y_slider->update(plugin->config.y);
			break;
		case RESET_U_SLIDER :
			u_text->update(plugin->config.u);
			u_slider->update(plugin->config.u);
			break;
		case RESET_V_SLIDER :
			v_text->update(plugin->config.v);
			v_slider->update(plugin->config.v);
			break;
		case RESET_ALL :
		default:
			y_text->update(plugin->config.y);
			y_slider->update(plugin->config.y);
			u_text->update(plugin->config.u);
			u_slider->update(plugin->config.u);
			v_text->update(plugin->config.v);
			v_slider->update(plugin->config.v);
			break;
	}
}






YUVEffect::YUVEffect(PluginServer *server)
 : PluginVClient(server)
{

}
YUVEffect::~YUVEffect()
{

}

const char* YUVEffect::plugin_title() { return N_("YUV"); }
int YUVEffect::is_realtime() { return 1; }


NEW_WINDOW_MACRO(YUVEffect, YUVWindow)
LOAD_CONFIGURATION_MACRO(YUVEffect, YUVConfig)

void YUVEffect::update_gui()
{
	if(thread)
	{
		thread->window->lock_window();
		load_configuration();
		((YUVWindow*)thread->window)->y_text->update(config.y);
		((YUVWindow*)thread->window)->y_slider->update(config.y);
		((YUVWindow*)thread->window)->u_text->update(config.u);
		((YUVWindow*)thread->window)->u_slider->update(config.u);
		((YUVWindow*)thread->window)->v_text->update(config.v);
		((YUVWindow*)thread->window)->v_slider->update(config.v);
		thread->window->unlock_window();
	}
}

void YUVEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("YUV");
	output.tag.set_property("Y", config.y);
	output.tag.set_property("U", config.u);
	output.tag.set_property("V", config.v);
	output.append_tag();
	output.tag.set_title("/YUV");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void YUVEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	while(!input.read_tag())
	{
		if(input.tag.title_is("YUV"))
		{
			config.y = input.tag.get_property("Y", config.y);
			config.u = input.tag.get_property("U", config.u);
			config.v = input.tag.get_property("V", config.v);
		}
	}
}


#define YUV_MACRO(type, temp_type, max, components, use_yuv) \
{ \
	for(int i = 0; i < input->get_h(); i++) \
	{ \
		type *in_row = (type*)input->get_rows()[i]; \
		type *out_row = (type*)output->get_rows()[i]; \
		const float round = (sizeof(type) == 4) ? 0.0 : 0.5; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			if(use_yuv) \
			{ \
				int y = (int)((float)in_row[0] * y_scale + round); \
				int u = (int)((float)(in_row[1] - (max / 2 + 1)) * u_scale + round) + (max / 2 + 1); \
				int v = (int)((float)(in_row[2] - (max / 2 + 1)) * v_scale + round) + (max / 2 + 1); \
				out_row[0] = CLIP(y, 0, max); \
				out_row[1] = CLIP(u, 0, max); \
				out_row[2] = CLIP(v, 0, max); \
			} \
			else \
			{ \
				temp_type y, u, v, r, g, b; \
				if(sizeof(type) == 4) \
				{ \
					YUV::yuv.rgb_to_yuv_f(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
				else \
				if(sizeof(type) == 2) \
				{ \
					YUV::yuv.rgb_to_yuv_16(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
				else \
				{ \
					YUV::yuv.rgb_to_yuv_8(in_row[0], in_row[1], in_row[2], y, u, v); \
				} \
 \
 				if(sizeof(type) < 4) \
				{ \
					CLAMP(y, 0, max); \
					CLAMP(u, 0, max); \
					CLAMP(v, 0, max); \
 \
					y = temp_type((float)y * y_scale + round); \
					u = temp_type((float)(u - (max / 2 + 1)) * u_scale + round) + (max / 2 + 1); \
					v = temp_type((float)(v - (max / 2 + 1)) * v_scale + round) + (max / 2 + 1); \
 \
					CLAMP(y, 0, max); \
					CLAMP(u, 0, max); \
					CLAMP(v, 0, max); \
				} \
				else \
				{ \
					y = temp_type((float)y * y_scale + round); \
					u = temp_type((float)u * u_scale + round); \
					v = temp_type((float)v * v_scale + round); \
				} \
 \
				if(sizeof(type) == 4) \
					YUV::yuv.yuv_to_rgb_f(r, g, b, y, u, v); \
				else \
				if(sizeof(type) == 2) \
					YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
				else \
					YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v); \
 \
				out_row[0] = r; \
				out_row[1] = g; \
				out_row[2] = b; \
			} \
		 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}

int YUVEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

	if(EQUIV(config.y, 0) && EQUIV(config.u, 0) && EQUIV(config.v, 0))
	{
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
	}
	else
	{
		int w = input->get_w();

		float y_scale = (float)(config.y + MAXVALUE) / MAXVALUE;
		float u_scale = (float)(config.u + MAXVALUE) / MAXVALUE;
		float v_scale = (float)(config.v + MAXVALUE) / MAXVALUE;

		if(u_scale > 1) u_scale = 1 + (u_scale - 1) * 4;
		if(v_scale > 1) v_scale = 1 + (v_scale - 1) * 4;

		switch(input->get_color_model())
		{
			case BC_RGB_FLOAT:
				YUV_MACRO(float, float, 1, 3, 0)
				break;

			case BC_RGB888:
				YUV_MACRO(unsigned char, int, 0xff, 3, 0)
				break;

			case BC_YUV888:
				YUV_MACRO(unsigned char, int, 0xff, 3, 1)
				break;

			case BC_RGB161616:
				YUV_MACRO(uint16_t, int, 0xffff, 3, 0)
				break;

			case BC_YUV161616:
				YUV_MACRO(uint16_t, int, 0xffff, 3, 1)
				break;

			case BC_RGBA_FLOAT:
				YUV_MACRO(float, float, 1, 4, 0)
				break;

			case BC_RGBA8888:
				YUV_MACRO(unsigned char, int, 0xff, 4, 0)
				break;

			case BC_YUVA8888:
				YUV_MACRO(unsigned char, int, 0xff, 4, 1)
				break;

			case BC_RGBA16161616:
				YUV_MACRO(uint16_t, int, 0xffff, 4, 0)
				break;

			case BC_YUVA16161616:
				YUV_MACRO(uint16_t, int, 0xffff, 4, 1)
				break;
		}



	}
	return 0;
}


