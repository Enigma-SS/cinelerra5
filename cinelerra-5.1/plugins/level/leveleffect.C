
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
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "leveleffect.h"
#include "samples.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <errno.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

REGISTER_PLUGIN(SoundLevelEffect)


SoundLevelConfig::SoundLevelConfig()
{
	duration = 1.0;
}

void SoundLevelConfig::copy_from(SoundLevelConfig &that)
{
	duration = that.duration;
}

int SoundLevelConfig::equivalent(SoundLevelConfig &that)
{
	return EQUIV(duration, that.duration);
}

void SoundLevelConfig::interpolate(SoundLevelConfig &prev, SoundLevelConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	duration = prev.duration;
}


SoundLevelDuration::SoundLevelDuration(SoundLevelEffect *plugin, int x, int y)
 : BC_FSlider(x, y, 0, xS(180), yS(180), 0.0, 10.0, plugin->config.duration)
{
	this->plugin = plugin;
	set_precision(0.1);
}

int SoundLevelDuration::handle_event()
{
	plugin->config.duration = get_value();
	plugin->send_configure_change();
	return 1;
}


SoundLevelWindow::SoundLevelWindow(SoundLevelEffect *plugin)
 : PluginClientWindow(plugin, xS(350), yS(120), xS(350), yS(120), 0)
{
	this->plugin = plugin;
}

void SoundLevelWindow::create_objects()
{
	int xs10 = xS(10), xs150 = xS(150);
	int ys10 = yS(10), ys35 = yS(35);
//printf("SoundLevelWindow::create_objects 1\n");
	int x = xs10, y = ys10;


	add_subwindow(new BC_Title(x, y, _("Duration (seconds):")));
	add_subwindow(duration = new SoundLevelDuration(plugin, x + xs150, y));
	y += ys35;
	add_subwindow(new BC_Title(x, y, _("Max soundlevel (dB):")));
	add_subwindow(soundlevel_max = new BC_Title(x + xs150, y, "0.0"));
	y += ys35;
	add_subwindow(new BC_Title(x, y, _("RMS soundlevel (dB):")));
	add_subwindow(soundlevel_rms = new BC_Title(x + xs150, y, "0.0"));

	show_window();
	flush();
//printf("SoundLevelWindow::create_objects 2\n");
}


SoundLevelEffect::SoundLevelEffect(PluginServer *server)
 : PluginAClient(server)
{

	reset();
}

SoundLevelEffect::~SoundLevelEffect()
{

}


LOAD_CONFIGURATION_MACRO(SoundLevelEffect, SoundLevelConfig)

NEW_WINDOW_MACRO(SoundLevelEffect, SoundLevelWindow)



void SoundLevelEffect::reset()
{
	rms_accum = 0;
	max_accum = 0;
	accum_size = 0;
}

const char* SoundLevelEffect::plugin_title() { return N_("SoundLevel"); }
int SoundLevelEffect::is_realtime() { return 1; }


void SoundLevelEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("SOUNDLEVEL") ) {
			config.duration = input.tag.get_property("DURATION", config.duration);
		}
	}
}

void SoundLevelEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SOUNDLEVEL");
	output.tag.set_property("DURATION", config.duration);
	output.append_tag();
	output.tag.set_title("/SOUNDLEVEL");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}


void SoundLevelEffect::update_gui()
{
	if( !thread ) return;
	SoundLevelWindow *window = (SoundLevelWindow*)thread->window;
	if( !window ) return;
	int reconfigure = load_configuration();
	int pending = pending_gui_frame();
	if( !reconfigure && !pending ) return;
	window->lock_window();
	if( reconfigure ) {
		window->duration->update(config.duration);
	}
	if( pending ) {
		double pos = get_tracking_position();
		double dir = get_tracking_direction() == PLAY_REVERSE ? -1 : 1;
		SoundLevelClientFrame *frame =
			(SoundLevelClientFrame *)get_gui_frame(pos, dir);
		char string[BCSTRLEN];
		sprintf(string, "%.2f", DB::todb(frame->max));
		window->soundlevel_max->update(string);
		sprintf(string, "%.2f", DB::todb(frame->rms));
		window->soundlevel_rms->update(string);
	}
	window->unlock_window();
}

int SoundLevelEffect::process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr)
{
	load_configuration();

	int sample_rate = get_project_samplerate();
	int fragment = config.duration * sample_rate;
	int64_t position = get_source_position();
	accum_size += size;
	double *input_samples = input_ptr->get_data();
	for( int i=0; i<size; ++i ) {
		double value = fabs(input_samples[i]);
		if(value > max_accum) max_accum = value;
		rms_accum += value * value;
		if( ++accum_size >= fragment ) {
			rms_accum = sqrt(rms_accum / accum_size);
			SoundLevelClientFrame *level_frame = new SoundLevelClientFrame();
			level_frame->max = max_accum;
			level_frame->rms = rms_accum;
			level_frame->position = (double)(position+i)/get_project_samplerate();
			add_gui_frame(level_frame);
			rms_accum = 0;
			max_accum = 0;
			accum_size = 0;
		}
	}
	return 0;
}

void SoundLevelEffect::render_gui(void *data, int size)
{
	if( !thread ) return;
	SoundLevelWindow *window = (SoundLevelWindow*)thread->window;
	if( !window ) return;
	window->lock_window();
	char string[BCTEXTLEN];
	double *arg = (double*)data;
	sprintf(string, "%.2f", DB::todb(arg[0]));
	window->soundlevel_max->update(string);
	sprintf(string, "%.2f", DB::todb(arg[1]));
	window->soundlevel_rms->update(string);
	window->flush();
	window->unlock_window();
}

void SoundLevelEffect::render_stop()
{
	rms_accum = 0;
	max_accum = 0;
	accum_size = 0;
}

