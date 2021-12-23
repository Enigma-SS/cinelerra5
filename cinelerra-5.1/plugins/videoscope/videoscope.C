
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
#include "bccolors.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "pluginvclient.h"
#include "fonts.h"
#include "scopewindow.h"
#include "theme.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>


class VideoScopeEffect;
class VideoScopeWindow;

class VideoScopeConfig
{
public:
	VideoScopeConfig();
};

class VideoScopeWindow : public ScopeGUI
{
public:
	VideoScopeWindow(VideoScopeEffect *plugin);
	~VideoScopeWindow();

	void create_objects();
	void toggle_event();
	int resize_event(int w, int h);

	VideoScopeEffect *plugin;
};

class VideoScopeEffect : public PluginVClient
{
public:
	VideoScopeEffect(PluginServer *server);
	~VideoScopeEffect();


	PLUGIN_CLASS_MEMBERS2(VideoScopeConfig)
	int process_realtime(VFrame *input, VFrame *output);
	int is_realtime();
	void render_gui(void *input);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int use_hist, use_wave, use_vector;
	int use_hist_parade, use_wave_parade;
	int use_wave_gain, use_vect_gain;
	int use_smooth, use_graticule;

	int w, h;
	VFrame *input;
};


VideoScopeConfig::VideoScopeConfig()
{
}

VideoScopeWindow::VideoScopeWindow(VideoScopeEffect *plugin)
 : ScopeGUI(plugin, plugin->w, plugin->h)
{
	this->plugin = plugin;
}

VideoScopeWindow::~VideoScopeWindow()
{
}

void VideoScopeWindow::create_objects()
{
	use_hist = plugin->use_hist;
	use_wave = plugin->use_wave;
	use_vector = plugin->use_vector;
	use_hist_parade = plugin->use_hist_parade;
	use_wave_parade = plugin->use_wave_parade;
	use_wave_gain = plugin->use_wave_gain;
	use_vect_gain = plugin->use_vect_gain;
	use_smooth = plugin->use_smooth;
	use_refresh = -1;
	use_release = 0;
	use_graticule = plugin->use_graticule;

	ScopeGUI::create_objects();
}

void VideoScopeWindow::toggle_event()
{
	plugin->use_hist = use_hist;
	plugin->use_wave = use_wave;
	plugin->use_vector = use_vector;
	plugin->use_hist_parade = use_hist_parade;
	plugin->use_wave_parade = use_wave_parade;
	plugin->use_wave_gain = use_wave_gain;
	plugin->use_vect_gain = use_vect_gain;
	plugin->use_smooth = use_smooth;
	plugin->use_graticule = use_graticule;
// Make it reprocess
	plugin->send_configure_change();
}


int VideoScopeWindow::resize_event(int w, int h)
{
	ScopeGUI::resize_event(w, h);
	plugin->w = w;
	plugin->h = h;
// Make it reprocess
	plugin->send_configure_change();
	return 1;
}

REGISTER_PLUGIN(VideoScopeEffect)

VideoScopeEffect::VideoScopeEffect(PluginServer *server)
 : PluginVClient(server)
{
	w = MIN_SCOPE_W;
	h = MIN_SCOPE_H;
	use_hist = 0;
	use_wave = 0;
	use_vector = 1;
	use_hist_parade = 1;
	use_wave_parade = 1;
	use_wave_gain = 5;
	use_vect_gain = 5;
	use_smooth = 1;
	use_graticule = 0;
}

VideoScopeEffect::~VideoScopeEffect()
{
}

const char* VideoScopeEffect::plugin_title() { return N_("VideoScope"); }
int VideoScopeEffect::is_realtime() { return 1; }

int VideoScopeEffect::load_configuration()
{
	return 0;
}

void VideoScopeEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("VIDEOSCOPE");
	if( is_defaults() ) {
		output.tag.set_property("W", w);
		output.tag.set_property("H", h);
		output.tag.set_property("USE_HIST", use_hist);
		output.tag.set_property("USE_WAVE", use_wave);
		output.tag.set_property("USE_VECTOR", use_vector);
		output.tag.set_property("USE_HIST_PARADE", use_hist_parade);
		output.tag.set_property("USE_WAVE_PARADE", use_wave_parade);
		output.tag.set_property("USE_WAVE_GAIN", use_wave_gain);
		output.tag.set_property("USE_VECT_GAIN", use_vect_gain);
		output.tag.set_property("USE_SMOOTH", use_smooth);
		output.tag.set_property("USE_GRATICULE", use_graticule);

	}
	output.append_tag();
	output.tag.set_title("/VIDEOSCOPE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void VideoScopeEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("VIDEOSCOPE") ) {
			if( is_defaults() ) {
				w = input.tag.get_property("W", w);
				h = input.tag.get_property("H", h);
				use_hist = input.tag.get_property("USE_HIST", use_hist);
				use_wave = input.tag.get_property("USE_WAVE", use_wave);
				use_vector = input.tag.get_property("USE_VECTOR", use_vector);
				use_hist_parade = input.tag.get_property("USE_HIST_PARADE", use_hist_parade);
				use_wave_parade = input.tag.get_property("USE_WAVE_PARADE", use_wave_parade);
				use_wave_gain = input.tag.get_property("USE_WAVE_GAIN", use_wave_gain);
				use_vect_gain = input.tag.get_property("USE_VECT_GAIN", use_vect_gain);
				use_smooth = input.tag.get_property("USE_SMOOTH", use_smooth);
				use_graticule = input.tag.get_property("USE_GRATICULE", use_graticule);
			}
		}
	}
}

NEW_WINDOW_MACRO(VideoScopeEffect, VideoScopeWindow)

int VideoScopeEffect::process_realtime(VFrame *input, VFrame *output)
{

	send_render_gui(input);
	if( input->get_rows()[0] != output->get_rows()[0] )
		output->copy_from(input);
	return 1;
}

void VideoScopeEffect::render_gui(void *input)
{
	if( !thread ) return;
	VideoScopeWindow *window = ((VideoScopeWindow*)thread->window);
	window->lock_window();
	this->input = (VFrame*)input;
	window->process(this->input);
	window->unlock_window();
}

