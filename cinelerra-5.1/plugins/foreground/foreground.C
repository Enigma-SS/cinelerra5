
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

#include "foreground.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "vframe.h"


REGISTER_PLUGIN(ForegroundMain)


ForegroundConfig::ForegroundConfig()
{
	r = 0xff;
	g = 0xff;
	b = 0xff;
	a = 0xff;
}

int ForegroundConfig::equivalent(ForegroundConfig &that)
{
	return (r == that.r && g == that.g && b == that.b &&
		a == that.a);
}

void ForegroundConfig::copy_from(ForegroundConfig &that)
{
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void ForegroundConfig::interpolate(ForegroundConfig &prev, ForegroundConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);


	r = (int)(prev.r * prev_scale + next.r * next_scale);
	g = (int)(prev.g * prev_scale + next.g * next_scale);
	b = (int)(prev.b * prev_scale + next.b * next_scale);
	a = (int)(prev.a * prev_scale + next.a * next_scale);
}

int ForegroundConfig::get_color()
{
	int result = (r << 16) | (g << 8) | (b);
	return result;
}


ForegroundColors::ForegroundColors(ForegroundWindow *window, ForegroundMain *plugin)
 : ColorGUI(window)
{
	this->window = window;
	this->plugin = plugin;
}
ForegroundColors::~ForegroundColors()
{
}

int ForegroundColors::handle_new_color(int output, int alpha)
{
	plugin->config.r = (output>>16) & 0xff;
	plugin->config.g = (output>>8) & 0xff;
	plugin->config.b = (output) & 0xff;
	plugin->config.a = alpha;
	plugin->send_configure_change();
	return 1;
}
void ForegroundColors::update_gui(int output, int alpha)
{
}

ForegroundWindow::ForegroundWindow(ForegroundMain *plugin)
 : PluginClientWindow(plugin,
	ColorWindow::calculate_w(), ColorWindow::calculate_h() + BC_OKButton::calculate_h(),
	ColorWindow::calculate_w(), ColorWindow::calculate_h() + BC_OKButton::calculate_h(),
	0)
{
	this->plugin = plugin;
	colors = new ForegroundColors(this, plugin);
}

ForegroundWindow::~ForegroundWindow()
{
	delete colors;
}

void ForegroundWindow::create_objects()
{
	colors = new ForegroundColors(this, plugin);
 	int color = plugin->config.get_color();
  	int alpha = plugin->config.a;
	colors->start_selection(color, alpha, 1);
	colors->create_objects();
	show_window();
}

void ForegroundWindow::update()
{
 	int color = plugin->config.get_color();
  	int alpha = plugin->config.a;
	colors->update_gui(color, alpha);
}


ForegroundMain::ForegroundMain(PluginServer *server)
 : PluginVClient(server)
{
}

ForegroundMain::~ForegroundMain()
{
}

const char* ForegroundMain::plugin_title() { return N_("Foreground"); }
int ForegroundMain::is_realtime() { return 1; }

NEW_WINDOW_MACRO(ForegroundMain, ForegroundWindow)

LOAD_CONFIGURATION_MACRO(ForegroundMain, ForegroundConfig)

int ForegroundMain::is_synthesis()
{
	return 1;
}


int ForegroundMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	int need_alpha = config.a != 0xff;
	if( need_alpha ) {
		read_frame(frame, 0, start_position,
			frame_rate, get_use_opengl());
	}

	int w = frame->get_w();
	int h = frame->get_h();


#define MAIN_LOOP(type, temp, components, is_yuv) \
{ \
	temp c1, c2, c3, a; \
	temp transparency, max; \
	if( is_yuv ) { \
		yuv.rgb_to_yuv_8(config.r, config.g, config.b, \
			c1, c2, c3); \
		a = config.a; \
		transparency = 0xff - a; \
		max = 0xff; \
/* multiply alpha */ \
		if( components == 3 ) { \
			c1 = a * c1 / 0xff; \
			c2 = (a * c2 + 0x80 * transparency) / 0xff; \
			c3 = (a * c3 + 0x80 * transparency) / 0xff; \
		} \
	} \
	else { \
		c1 = config.r; c2 = config.g; c3 = config.b; a = config.a; \
		transparency = 0xff - a; \
		max = 0xff; \
		if( sizeof(type) == 4 ) { \
			c1 /= 0xff; c2 /= 0xff; c3 /= 0xff; a /= 0xff; \
			transparency /= 0xff; \
			max = 1.0; \
		} \
/* multiply alpha */ \
		if(components == 3) { \
			c1 = a * c1 / max; \
			c2 = a * c2 / max; \
			c3 = a * c3 / max; \
		} \
	} \
 \
	for( int i=0; i<h; ++i ) { \
		type *row = (type*)frame->get_rows()[i]; \
		for( int j=0; j<w; ++j ) { \
			if( components == 3 ) { \
				*row++ = c1; \
				*row++ = c2; \
				*row++ = c3; \
			} \
			else { \
				row[0] = (transparency * row[0] + a * c1) / max; \
				row[1] = (transparency * row[1] + a * c2) / max; \
				row[2] = (transparency * row[2] + a * c3) / max; \
				row[3] = MAX(row[3], a); \
				row += components; \
			} \
		} \
	} \
}
	switch( frame->get_color_model() ) {
	case BC_RGB888:     MAIN_LOOP(uint8_t, int, 3, 0) break;
	case BC_RGB_FLOAT:  MAIN_LOOP(float, float, 3, 0) break;
	case BC_YUV888:     MAIN_LOOP(uint8_t, int, 3, 1) break;
	case BC_RGBA8888:   MAIN_LOOP(uint8_t, int, 4, 0) break;
	case BC_RGBA_FLOAT: MAIN_LOOP(float, float, 4, 0) break;
	case BC_YUVA8888:   MAIN_LOOP(uint8_t, int, 4, 1) break;
	}

	return 0;
}


void ForegroundMain::update_gui()
{
	if( !thread ) return;
	ForegroundWindow *window = (ForegroundWindow*)thread->window;
	if( !window ) return;
	if( !load_configuration() ) return;
	window->lock_window("ForegroundMain::update_gui");
	window->update();
	window->unlock_window();
}


void ForegroundMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("FOREGROUND");
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.terminate_string();
}

void ForegroundMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("FOREGROUND") ) {
			config.r = input.tag.get_property("R", config.r);
			config.g = input.tag.get_property("G", config.g);
			config.b = input.tag.get_property("B", config.b);
			config.a = input.tag.get_property("A", config.a);
		}
	}
}

