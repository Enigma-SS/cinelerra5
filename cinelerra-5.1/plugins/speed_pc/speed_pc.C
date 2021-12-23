
/*
 * CINELERRA
 * Copyright (C) 2008-2016 Adam Williams <broadcast at earthling dot net>
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

/*
 * 2020. Derivative by ReframeRT plugin for a more easy use.
 * It uses percentage value of the speed referred to originl speed (=100%).
 * Some old ReframeRT parameters (Stretch and denom) have not been deleted,
 * for future development, if any.
 * Stretch and denom variables are set to a constant value:
 * Stretch= 1; denom= 100.00.
 * Speed_MIN= 1.00%; Speed_MAX= 1000.00% 
 */

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "speed_pc.h"
#include "guicast.h"
#include "language.h"
#include "pluginvclient.h"
#include "theme.h"
#include "transportque.h"

#include <string.h>




REGISTER_PLUGIN(SpeedPc);



SpeedPcConfig::SpeedPcConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}

void SpeedPcConfig::reset(int clear)
{
	switch(clear) {
		case RESET_SPEED :
			num = 100.00;
			denom = 100.0;
			stretch = 1;
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			num = 100.00;
			denom = 100.0;
			stretch = 1;
			interp = 0;
			optic_flow = 1;
			break;
	}
}

int SpeedPcConfig::equivalent(SpeedPcConfig &src)
{
	return fabs(num - src.num) < 0.0001 &&
		fabs(denom - src.denom) < 0.0001 &&
		stretch == src.stretch &&
		interp == src.interp;
}

void SpeedPcConfig::copy_from(SpeedPcConfig &src)
{
	this->num = src.num;
	this->denom = src.denom;
	this->stretch = src.stretch;
	this->interp = src.interp;
}

void SpeedPcConfig::interpolate(SpeedPcConfig &prev,
	SpeedPcConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	this->interp = prev.interp;
	this->stretch = prev.stretch;
	this->denom = prev.denom;

	if (this->interp && prev_frame != next_frame)
	{
		double next_weight = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
		double prev_weight = (double)(next_frame - current_frame) / (next_frame - prev_frame);
		double prev_slope = prev.num / prev.denom, next_slope = next.num / next.denom;
		// for interpolation, this is (for now) a simple linear slope to the next keyframe.
		double scale = prev_slope * prev_weight + next_slope * next_weight;
		this->num = this->denom * scale;
	}
	else
	{
		this->num = prev.num;
	}
}

void SpeedPcConfig::boundaries()
{
	if(num < 0.0001) num = 0.0001;
	if(denom < 0.0001) denom = 0.0001;
}








SpeedPcWindow::SpeedPcWindow(SpeedPc *plugin)
 : PluginClientWindow(plugin, xS(420), yS(150), xS(420), yS(150), 0)  // Note: with "Stretch" and "Downsample" gui yS was yS(210)
{
	this->plugin = plugin;
}

SpeedPcWindow::~SpeedPcWindow()
{
}

void SpeedPcWindow::create_objects()
{
	int xs10 = xS(10), xs64 = xS(64), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x2 = xS(80), x3 = xS(180);
	int x = xs10, y = ys10;
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Bar *bar;

	add_subwindow(new BC_Title(x, y, _("Preset:")));
	x = x + x2; 
	add_subwindow(toggle25pc = new SpeedPcToggle(plugin, this,
		plugin->config.num == 25, x, y, 25, "25%"));
	x += xs64;
	add_subwindow(toggle50pc = new SpeedPcToggle(plugin, this,
		plugin->config.num == 50, x, y, 50, "50%"));
	x += xs64;
	add_subwindow(toggle100pc = new SpeedPcToggle(plugin, this,
		plugin->config.num == 100, x, y, 100, "100%"));
	x += xs64;
	add_subwindow(toggle200pc = new SpeedPcToggle(plugin, this,
		plugin->config.num == 200, x, y, 200, "200%"));
	x += xs64;
	add_subwindow(toggle400pc = new SpeedPcToggle(plugin, this,
		plugin->config.num == 400, x, y, 400, "400%"));
	x = xs10;  y += ys30;

	add_tool(new BC_Title(x, y, _("Speed:")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	speed_pc_text = new SpeedPcText(plugin, this, (x + x2), y);
	speed_pc_text->create_objects();
	speed_pc_slider = new SpeedPcSlider(plugin, this, x3, y, xs200);
	add_subwindow(speed_pc_slider);
	clr_x = x3 + speed_pc_slider->get_w() + x;
	add_subwindow(speed_pc_clr = new SpeedPcClr(plugin, this,
		clr_x, y, RESET_SPEED));
	y += ys30;

// REM 2020-06-23
/*
	add_subwindow(stretch = new SpeedPcStretch(plugin, this, x, y));
	y += yS(30);
	add_subwindow(downsample = new SpeedPcDownsample(plugin, this, x, y));
	y += yS(30);
*/

	add_subwindow(interpolate = new SpeedPcInterpolate(plugin, this, x, y));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new SpeedPcReset(plugin, this, x, y));

	update(RESET_ALL);
	show_window();
}

void SpeedPcWindow::update(int clear)
{
	switch(clear) {
		case RESET_SPEED :
			speed_pc_text->update((float)plugin->config.num);
			speed_pc_slider->update((float)plugin->config.num);
			update_toggles();
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			speed_pc_text->update((float)plugin->config.num);
			speed_pc_slider->update((float)plugin->config.num);
			update_toggles();

// OLD ReframeRT code
/*
			stretch->update(plugin->config.stretch);
			downsample->update(!plugin->config.stretch);
*/
			interpolate->update(plugin->config.interp);
			break;
	}
}


int SpeedPcWindow::update_toggles()
{
	toggle25pc->update(EQUIV(plugin->config.num, 25));
	toggle50pc->update(EQUIV(plugin->config.num, 50));
	toggle100pc->update(EQUIV(plugin->config.num, 100));
	toggle200pc->update(EQUIV(plugin->config.num, 200));
	toggle400pc->update(EQUIV(plugin->config.num, 400));
	return 0;
}


SpeedPcToggle::SpeedPcToggle(SpeedPc *plugin, SpeedPcWindow *gui,
	int init_value,
	int x,
	int y,
	int value,
	const char *string)
 : BC_Radial(x, y, init_value, string)
{
	this->value = value;
	this->plugin = plugin;
	this->gui = gui;
}

int SpeedPcToggle::handle_event()
{
	plugin->config.num = (float)value;
	gui->update(RESET_SPEED);
	plugin->send_configure_change();
	return 1;
}



/* *********************************** */
/* **** SPEED     ******************** */
SpeedPcText::SpeedPcText(SpeedPc *plugin, SpeedPcWindow *gui,
	int x,
	int y)
 : BC_TumbleTextBox(gui, (float)plugin->config.num,
	(float)1.00, (float)1000.00, x, y, xS(60), 2)
{
	this->plugin = plugin;
	this->gui = gui;
}

SpeedPcText::~SpeedPcText()
{
}

int SpeedPcText::handle_event()
{
	plugin->config.num = atof(get_text());
	plugin->config.denom = 100.00;
	plugin->config.stretch = 1;
	plugin->config.boundaries();
	gui->update(RESET_SPEED);
	plugin->send_configure_change();
	return 1;
}

SpeedPcSlider::SpeedPcSlider(SpeedPc *plugin, SpeedPcWindow *gui,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 1.00, 1000.00, plugin->config.num)
{
	this->plugin = plugin;
	this->gui = gui;
	enable_show_value(0); // Hide caption
	set_precision(1.00);
}

SpeedPcSlider::~SpeedPcSlider()
{
}

int SpeedPcSlider::handle_event()
{
	plugin->config.num = get_value();
	plugin->config.denom = 100.00;
	plugin->config.stretch = 1;
	gui->update(RESET_SPEED);
	plugin->send_configure_change();
	return 1;
}
/* *********************************** */


SpeedPcStretch::SpeedPcStretch(SpeedPc *plugin,
	SpeedPcWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, plugin->config.stretch, _("Stretch"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int SpeedPcStretch::handle_event()
{
	plugin->config.stretch = get_value();
	gui->downsample->update(!get_value());
	plugin->send_configure_change();
	return 1;
}


SpeedPcDownsample::SpeedPcDownsample(SpeedPc *plugin, SpeedPcWindow *gui,
	int x,
	int y)
 : BC_Radial(x, y, !plugin->config.stretch, _("Downsample"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int SpeedPcDownsample::handle_event()
{
	plugin->config.stretch = !get_value();
	gui->stretch->update(!get_value());
	plugin->send_configure_change();
	return 1;
}

SpeedPcInterpolate::SpeedPcInterpolate(SpeedPc *plugin,	SpeedPcWindow *gui,
	int x,
	int y)
 : BC_CheckBox(x, y, 0, _("Interpolate"))
{
	this->plugin = plugin;
	this->gui = gui;
	set_tooltip(_("Interpolate between keyframes"));
}

int SpeedPcInterpolate::handle_event()
{
	plugin->config.interp = get_value();
	gui->interpolate->update(get_value());
	plugin->send_configure_change();
	return 1;
}


SpeedPcClr::SpeedPcClr(SpeedPc *plugin,	SpeedPcWindow *gui, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->clear = clear;
}
SpeedPcClr::~SpeedPcClr()
{
}
int SpeedPcClr::handle_event()
{
	plugin->config.reset(clear);
	gui->update(clear);
	plugin->send_configure_change();
	return 1;
}


SpeedPcReset::SpeedPcReset(SpeedPc *plugin, SpeedPcWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
SpeedPcReset::~SpeedPcReset()
{
}
int SpeedPcReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	gui->update(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}



SpeedPc::SpeedPc(PluginServer *server)
 : PluginVClient(server)
{
}

SpeedPc::~SpeedPc()
{

}

const char* SpeedPc::plugin_title() { return N_("Speed PerCent"); }
int SpeedPc::is_realtime() { return 1; }
int SpeedPc::is_synthesis() { return 1; }


NEW_WINDOW_MACRO(SpeedPc, SpeedPcWindow)
LOAD_CONFIGURATION_MACRO(SpeedPc, SpeedPcConfig)

int SpeedPc::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int64_t input_frame = get_source_start();
	SpeedPcConfig prev_config, next_config;
	KeyFrame *tmp_keyframe, *next_keyframe = get_prev_keyframe(get_source_start());
	int64_t tmp_position, next_position;
	int64_t segment_len;
	double input_rate = frame_rate;
	int is_current_keyframe;

// if there are no keyframes, the default keyframe is used, and its position is always 0;
// if there are keyframes, the first keyframe can be after the effect start (and it controls settings before it)
// so let's calculate using a fake keyframe with the same settings but position == effect start
	KeyFrame *fake_keyframe = new KeyFrame();
	fake_keyframe->copy_from(next_keyframe);
	fake_keyframe->position = local_to_edl(get_source_start());
	next_keyframe = fake_keyframe;

	// calculate input_frame accounting for all previous keyframes
	do
	{
		tmp_keyframe = next_keyframe;
		next_keyframe = get_next_keyframe(tmp_keyframe->position+1, 0);

		tmp_position = edl_to_local(tmp_keyframe->position);
		next_position = edl_to_local(next_keyframe->position);

		is_current_keyframe =
			next_position > start_position // the next keyframe is after the current position
			|| next_keyframe->position == tmp_keyframe->position // there are no more keyframes
			|| !next_keyframe->position; // there are no keyframes at all

		if (is_current_keyframe)
			segment_len = start_position - tmp_position;
		else
			segment_len = next_position - tmp_position;

		read_data(next_keyframe);
		next_config.copy_from(config);
		read_data(tmp_keyframe);
		prev_config.copy_from(config);
		config.interpolate(prev_config, next_config, tmp_position, next_position, tmp_position + segment_len);

		// the area under the curve is the number of frames to advance
		// as long as interpolate() uses a linear slope we can use geometry to determine this
		// if interpolate() changes to use a curve then this needs use (possibly) the definite integral
		double prev_scale = prev_config.num / 100.00;
		double config_scale = config.num / 100.00;
		input_frame += (int64_t)(segment_len * ((prev_scale + config_scale) / 2));
	} while (!is_current_keyframe);

// Change rate
	if (!config.stretch)
	{
		input_rate *= config.num / 100.00;

	}

// printf("SpeedPc::process_buffer %d %lld %f %lld %f\n",
// __LINE__,
// start_position,
// frame_rate,
// input_frame,
// input_rate);

	read_frame(frame,
		0,
		input_frame,
		input_rate,
		0);

	delete fake_keyframe;

	return 0;
}



void SpeedPc::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("SPEED_PC");
	output.tag.set_property("SPEED", config.num);
	output.tag.set_property("DENOM", config.denom);
	output.tag.set_property("STRETCH", config.stretch);
	output.tag.set_property("INTERPOLATE", config.interp);
	output.append_tag();
	output.tag.set_title("/SPEED_PC");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void SpeedPc::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("SPEED_PC"))
		{
			config.num = input.tag.get_property("SPEED", config.num);
			config.denom = input.tag.get_property("DENOM", config.denom);
			config.stretch = input.tag.get_property("STRETCH", config.stretch);
			config.interp = input.tag.get_property("INTERPOLATE", config.interp);
		}
	}
}

void SpeedPc::update_gui()
{
	if(thread)
	{
		int changed = load_configuration();

		if(changed)
		{
			SpeedPcWindow* window = (SpeedPcWindow*)thread->window;
			window->lock_window("SpeedPc::update_gui");
			window->update(RESET_ALL);
// OLD ReframeRT code
/*
			window->stretch->update(config.stretch);
			window->downsample->update(!config.stretch);
*/
			window->unlock_window();
		}
	}
}





