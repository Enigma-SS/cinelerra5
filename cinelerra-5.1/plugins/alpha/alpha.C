
/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "alpha.h"
#include "theme.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "vframe.h"

REGISTER_PLUGIN(AlphaMain)


AlphaConfig::AlphaConfig()
{
	reset();
}

void AlphaConfig::reset()
{
	a = 1.;
}

int AlphaConfig::equivalent(AlphaConfig &that)
{
	return a == that.a;
}

void AlphaConfig::copy_from(AlphaConfig &that)
{
	a = that.a;
}

void AlphaConfig::interpolate(AlphaConfig &prev, AlphaConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	a = (prev.a * prev_scale + next.a * next_scale);
}


AlphaText::AlphaText(AlphaWindow *window, AlphaMain *plugin,
		int x, int y)
 : BC_TumbleTextBox(window, (float)plugin->config.a,
	(float)OPACITY_MIN, (float)OPACITY_MAX, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
	set_increment(0.1);
}
AlphaText::~AlphaText()
{
}

int AlphaText::handle_event()
{
	float min = OPACITY_MIN, max = OPACITY_MAX;
	float output = atof(get_text());

	if(output > max) output = max;
	else if(output < min) output = min;
	plugin->config.a = output;
	window->alpha_slider->update(plugin->config.a);
	window->alpha_text->update(plugin->config.a);
	plugin->send_configure_change();
	return 1;
}

AlphaSlider::AlphaSlider(AlphaWindow *window, AlphaMain *plugin,
		int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, OPACITY_MIN, OPACITY_MAX, plugin->config.a)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.001);
}
AlphaSlider::~AlphaSlider()
{
}

int AlphaSlider::handle_event()
{
	plugin->config.a = get_value();
	window->alpha_text->update(plugin->config.a);
	plugin->send_configure_change();
	return 1;
}

AlphaClr::AlphaClr(AlphaWindow *window, AlphaMain *plugin, int x, int y)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->window = window;
	this->plugin = plugin;
}
AlphaClr::~AlphaClr()
{
}
int AlphaClr::handle_event()
{
	plugin->config.reset();
	window->update();
	plugin->send_configure_change();
	return 1;
}


#define ALPHA_W xS(420)
#define ALPHA_H yS(60)

AlphaWindow::AlphaWindow(AlphaMain *plugin)
 : PluginClientWindow(plugin, ALPHA_W, ALPHA_H, ALPHA_W, ALPHA_H, 0)
{
	this->plugin = plugin;
}

AlphaWindow::~AlphaWindow()
{
}

void AlphaWindow::create_objects()
{
	int xs10 = xS(10), xs200 = xS(200);
	int ys10 = yS(10);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Title *title;

	y += ys10;
	add_subwindow(title = new BC_Title(x, y, _("Alpha:")));
	alpha_text = new AlphaText(this, plugin, (x + x2), y);
	alpha_text->create_objects();
	add_subwindow(alpha_slider = new AlphaSlider(this, plugin, x3, y, xs200));
	clr_x = x3 + alpha_slider->get_w() + x;
	add_subwindow(alpha_clr = new AlphaClr(this, plugin, clr_x, y));
	show_window();
}

void AlphaWindow::update()
{
	float alpha = plugin->config.a;
	alpha_text->update(alpha);
	alpha_slider->update(alpha);
}


AlphaMain::AlphaMain(PluginServer *server)
 : PluginVClient(server)
{
}

AlphaMain::~AlphaMain()
{
}

const char* AlphaMain::plugin_title() { return N_("Alpha"); }
int AlphaMain::is_realtime() { return 1; }

NEW_WINDOW_MACRO(AlphaMain, AlphaWindow)

LOAD_CONFIGURATION_MACRO(AlphaMain, AlphaConfig)

int AlphaMain::is_synthesis()
{
	return 1;
}


int AlphaMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame, 0, start_position, frame_rate, get_use_opengl());
	int w = frame->get_w(), h = frame->get_h();

#define MAIN_LOOP(type, components, is_yuv, max) do { \
	if( components == 4 ) { \
		for( int y=0; y<h; ++y ) { \
			type *row = (type*)frame->get_rows()[y]; \
			for( int x=0; x<w; ++x ) { \
				row[3] = a * max; \
				row += components; \
			} \
		} \
	} \
	else if( is_yuv ) { \
		type ofs = (max+1)/2; \
		for( int y=0; y<h; ++y ) { \
			type *row = (type*)frame->get_rows()[y]; \
			for( int x=0; x<w; ++x ) { \
				row[0] = row[0] * a; \
				row[1] = (row[1]-ofs) * a + ofs; \
				row[2] = (row[2]-ofs) * a + ofs; \
				row += components; \
			} \
		} \
	} \
	else {\
		for( int y=0; y<h; ++y ) { \
			type *row = (type*)frame->get_rows()[y]; \
			for( int x=0; x<w; ++x ) { \
				row[0] *= a; \
				row[1] *= a; \
				row[2] *= a; \
				row += components; \
			} \
		} \
	} \
} while(0)

	float a = config.a;
	switch( frame->get_color_model() ) {
	case BC_RGB888:     MAIN_LOOP(uint8_t, 3, 0, 0xff);  break;
	case BC_RGB_FLOAT:  MAIN_LOOP(float,   3, 0, 1.0 );  break;
	case BC_YUV888:     MAIN_LOOP(uint8_t, 3, 1, 0xff);  break;
	case BC_RGBA8888:   MAIN_LOOP(uint8_t, 4, 0, 0xff);  break;
	case BC_RGBA_FLOAT: MAIN_LOOP(float,   4, 0, 1.0 );  break;
	case BC_YUVA8888:   MAIN_LOOP(uint8_t, 4, 1, 0xff);  break;
	}

	return 0;
}


void AlphaMain::update_gui()
{
	if( !thread ) return;
	AlphaWindow *window = (AlphaWindow*)thread->window;
	if( !window ) return;
	if( !load_configuration() ) return;
	window->lock_window("AlphaMain::update_gui");
	window->update();
	window->unlock_window();
}


void AlphaMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("ALPHA");
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void AlphaMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("ALPHA") ) {
			config.a = input.tag.get_property("A", config.a);
		}
	}
}

