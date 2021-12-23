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
 * 
 */

#include "asset.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "comprmultigui.h"
#include "cursors.h"
#include "edl.h"
#include "edlsession.h"
#include "eqcanvas.h"
#include "file.h"
#include "language.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"

#include <string.h>

ComprMultiWindow::ComprMultiWindow(ComprMultiEffect *plugin)
 : PluginClientWindow(plugin, xS(650),yS(560), xS(650),yS(560), 0)
{
	this->plugin = plugin;
	char string[BCTEXTLEN];
// set the default directory
	sprintf(string, "%s/compressormulti.rc", File::get_config_path());
	defaults = new BC_Hash(string);
	defaults->load();
	plugin->config.current_band = defaults->get("CURRENT_BAND", plugin->config.current_band);
	gain_frame = 0;
	freq_frame = 0;
}

ComprMultiWindow::~ComprMultiWindow()
{
	defaults->update("CURRENT_BAND", plugin->config.current_band);
	defaults->save();
	delete defaults;

	delete eqcanvas;
	delete reaction;
	delete x_text;
	delete y_text;
	delete trigger;
	delete decay;
	delete gain_frame;
	delete freq_frame;
}


void ComprMultiWindow::create_objects()
{
	int margin = client->get_theme()->widget_border;
	int x = margin, y = margin;
	int control_margin = xS(150);
	int canvas_y2 = get_h() * 2 / 3;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, "In:"));
	int y2 = y + title->get_h() + margin;
	EDL *edl = plugin->get_edl();
	add_subwindow(in = new BC_Meter(x, y2, METER_VERT,
		canvas_y2 - y2,
		edl->session->min_meter_db,
		edl->session->max_meter_db,
		edl->session->meter_format,
		1, // use_titles
		-1)); // span
	x += in->get_w() + margin;

	add_subwindow(title = new BC_Title(x, y, "Gain:"));
	add_subwindow(gain_change = new BC_Meter(x, y2, METER_VERT,
		canvas_y2 - y2,
		MIN_GAIN_CHANGE,
		MAX_GAIN_CHANGE,
		METER_DB,
		1, // use_titles
		-1, // span
		0, // downmix
		1)); // is_gain_change
	x += gain_change->get_w() + xS(35);

	add_subwindow(title = new BC_Title(x, y, _("Current band:")));

	int x1 = title->get_x() + title->get_w() + margin;
	char string[BCTEXTLEN];
	for( int i = 0; i < TOTAL_BANDS; i++ ) {
		sprintf(string, "%d", i + 1);
		add_subwindow(band[i] = new ComprMultiBand(this, plugin, 
			x1, y, i, string));
		x1 += band[i]->get_w() + margin;
	}
	y += band[0]->get_h() + 1;


	add_subwindow(title = new BC_Title(x, y,
			_("Sound level (Press shift to snap to grid):")));
	y += title->get_h() + 1;
	add_subwindow(canvas = new ComprMultiCanvas(plugin, this,
		x, y, get_w() - x - control_margin - xS(10), canvas_y2 - y));
	y += canvas->get_h() + yS(30);

	add_subwindow(title = new BC_Title(margin, y, _("Bandwidth:")));
	y += title->get_h();
	eqcanvas = new EQCanvas(this, margin, y,
		canvas->get_w() + x - margin, get_h() - y - margin,
		plugin->config.min_db, 0.0);
	eqcanvas->freq_divisions = 10;
	eqcanvas->initialize();

	x = get_w() - control_margin;
	y = margin;
	add_subwindow(title = new BC_Title(x, y, _("Attack secs:")));
	y += title->get_h();
	reaction = new ComprMultiReaction(plugin, this, x, y);
	reaction->create_objects();
	y += reaction->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Release secs:")));
	y += title->get_h();
	decay = new ComprMultiDecay(plugin, this, x, y);
	decay->create_objects();
	y += decay->get_h() + margin;

	add_subwindow(solo = new ComprMultiSolo(plugin, x, y));
	y += solo->get_h() + margin;
	add_subwindow(bypass = new ComprMultiBypass(plugin, x, y));
	y += bypass->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
	y += title->get_h();

	y_text = new ComprMultiY(plugin, this, x, y);
	y_text->create_objects();
	y += y_text->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Input:")));
	y += title->get_h();
	x_text = new ComprMultiX(plugin, this, x, y);
	x_text->create_objects();
	y += x_text->get_h() + margin;

	add_subwindow(clear = new ComprMultiClear(plugin, this, x, y));
	y += clear->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Gain:")));
	
	int x2 = get_w() - (margin + BC_Pot::calculate_w()) * 2;
	int x3 = get_w() - (margin + BC_Pot::calculate_w());
	add_subwindow(title = new BC_Title(x2, y, _("Freq range:")));
	y += title->get_h() + margin;

	add_subwindow(mkup_gain = new ComprMultiMkupGain(plugin, this, x, y,
		&plugin->config.bands[plugin->config.current_band].mkup_gain,
		-10., 10.));
// the previous high frequency
	add_subwindow(freq1 = new ComprMultiQPot(this, plugin, x2, y,
		plugin->config.current_band == 0 ? 0 :
		&plugin->config.bands[plugin->config.current_band-1].freq));
// the current high frequency
	add_subwindow(freq2 = new ComprMultiQPot(this, plugin, x3, y,
		plugin->config.current_band!=TOTAL_BANDS-1 ? 0 :
		&plugin->config.bands[plugin->config.current_band].freq));
	y += freq1->get_h() + margin;

	BC_Bar *bar;
	add_subwindow(bar = new BC_Bar(x, y, get_w() - x - margin));
	y += bar->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Trigger Type:")));
	y += title->get_h();
	add_subwindow(input = new ComprMultiInput(plugin, this, x, y));
	input->create_objects();
	y += input->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Channel:")));
	y += title->get_h();

	trigger = new ComprMultiTrigger(plugin, this, x, y);
	trigger->create_objects();
	if( plugin->config.input != ComprMultiConfig::TRIGGER ) trigger->disable();
	y += trigger->get_h() + margin;

	add_subwindow(smooth = new ComprMultiSmooth(plugin, x, y));
	y += smooth->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Steepness:")));
	add_subwindow(q = new ComprMultiFPot(this, plugin, 
		get_w() - margin - BC_Pot::calculate_w(), y, 
		&plugin->config.q, 0, 1));
	y += q->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Window size:")));
	y += title->get_h();
	add_subwindow(size = new ComprMultiSize(this, plugin, x, y));
	size->create_objects();
	size->update(plugin->config.window_size);
	y += size->get_h() + margin;

	add_subwindow(reset = new ComprMultiReset(plugin, this, x, y));

	canvas->create_objects();
	update_eqcanvas();
	show_window();
}

// called when the user selects a different band
void ComprMultiWindow::update()
{
	int curr_band = plugin->config.current_band;
	for( int i = 0; i < TOTAL_BANDS; i++ )
		band[i]->update(curr_band == i);

	int *ptr1 = !curr_band ? 0 :
		&plugin->config.bands[curr_band-1].freq;
	freq1->output = ptr1;
	if( ptr1 ) {
		freq1->update(*ptr1);
		freq1->enable();
	}
	else {
		freq1->update(0);
		freq1->disable();
	}

// top band edits the penultimate band
	int *ptr2 = curr_band == TOTAL_BANDS-1 ? 0 :
		&plugin->config.bands[curr_band].freq;
	freq2->output = ptr2;
	if( ptr2 ) {
		freq2->update(*ptr2);
		freq2->enable();
	}
	else {
		freq2->update(0);
		freq2->disable();
	}

	q->update(plugin->config.q);
	BandConfig *band_config = &plugin->config.bands[curr_band];
	double *ptr3 = &band_config->mkup_gain;
	mkup_gain->output = ptr3;
	mkup_gain->update(*ptr3);
	solo->update(band_config->solo);
	bypass->update(band_config->bypass);
	size->update(plugin->config.window_size);

	if( atol(trigger->get_text()) != plugin->config.trigger ) {
		trigger->update((int64_t)plugin->config.trigger);
	}

	if( strcmp(input->get_text(), ComprMultiInput::value_to_text(plugin->config.input)) ) {
		input->set_text(ComprMultiInput::value_to_text(plugin->config.input));
	}

	if( plugin->config.input != ComprMultiConfig::TRIGGER && trigger->get_enabled() ) {
		trigger->disable();
	}
	else
	if( plugin->config.input == ComprMultiConfig::TRIGGER && !trigger->get_enabled() ) {
		trigger->enable();
	}

	if( !EQUIV(atof(reaction->get_text()), band_config->attack_len) ) {
		reaction->update((float)band_config->attack_len);
	}

	if( !EQUIV(atof(decay->get_text()), band_config->release_len) ) {
		decay->update((float)band_config->release_len);
	}

	smooth->update(plugin->config.smoothing_only);
	if( canvas->current_operation == ComprMultiCanvas::DRAG ) {
		x_text->update((float)band_config->levels.values[canvas->current_point].x);
		y_text->update((float)band_config->levels.values[canvas->current_point].y);
	}

	canvas->update();
	update_eqcanvas();
}




void ComprMultiWindow::update_eqcanvas()
{
	plugin->calculate_envelope();
// filter GUI frames by band & data type
	int have_meter = 0;
	CompressorClientFrame *frame = 0;
// gdb plugin->dump_frames();
	double tracking_position = plugin->get_tracking_position();
	int dir = plugin->get_tracking_direction() == PLAY_REVERSE ? -1 : 1;
	while( (frame = (CompressorClientFrame*)plugin->next_gui_frame()) ) {
		if( dir*(frame->position - tracking_position) > 0 ) break;
		if( frame->band == plugin->config.current_band ) {
// only frames for desired band
			switch( frame->type ) {
			case FREQ_COMPRESSORFRAME: {
				delete freq_frame;
				freq_frame = (CompressorFreqFrame *)
					plugin->get_gui_frame(0, 0);
				continue; }
			case GAIN_COMPRESSORFRAME: {
				delete gain_frame;
				gain_frame = (CompressorGainFrame *)
					plugin->get_gui_frame(0, 0);
				have_meter = 1;
				continue; }
			}
		}
		delete plugin->get_gui_frame(0, 0);
	}
	if( have_meter ) {
		gain_change->update(gain_frame->gain, 0);
		in->update(gain_frame->level, 0);
	}

#ifndef DRAW_AFTER_BANDPASS
	eqcanvas->update_spectrogram(freq_frame); 
#else
	eqcanvas->update_spectrogram(freq_frame,
		plugin->config.current_band * plugin->config.window_size / 2,
		TOTAL_BANDS * plugin->config.window_size / 2,
		plugin->config.window_size);
#endif

// draw the active band on top of the others
	for( int pass = 0; pass < 2; pass++ ) {
		for( int band = 0; band < TOTAL_BANDS; band++ ) {
			if( band == plugin->config.current_band && pass == 0 ||
				band != plugin->config.current_band && pass == 1 ) {
				continue;
			}

			eqcanvas->draw_envelope(plugin->band_states[band]->envelope,
				plugin->PluginAClient::project_sample_rate,
				plugin->config.window_size,
				band == plugin->config.current_band,
				0);
		}
	}
	eqcanvas->canvas->flash(1);
}

int ComprMultiWindow::resize_event(int w, int h)
{
	return 1;
}


ComprMultiFPot::ComprMultiFPot(ComprMultiWindow *gui, ComprMultiEffect *plugin, 
		int x, int y, double *output, double min, double max)
 : BC_FPot(x, y, *output, min, max)
{
	this->gui = gui;
	this->plugin = plugin;
	this->output = output;
	set_precision(0.01);
}

int ComprMultiFPot::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	gui->update_eqcanvas();
	return 1;
}


ComprMultiQPot::ComprMultiQPot(ComprMultiWindow *gui, ComprMultiEffect *plugin, 
		int x, int y, int *output)
 : BC_QPot(x, y, output ? *output : 0)
{
	this->gui = gui;
	this->plugin = plugin;
	this->output = output;
}


int ComprMultiQPot::handle_event()
{
	if( output ) {
		*output = get_value();
		plugin->send_configure_change();
		gui->update_eqcanvas();
	}
	return 1;
}


ComprMultiSize::ComprMultiSize(ComprMultiWindow *gui, 
	ComprMultiEffect *plugin, int x, int y)
 : BC_PopupMenu(x, y, xS(100), "4096", 1)
{
	this->gui = gui;
	this->plugin = plugin;
}

int ComprMultiSize::handle_event()
{
	plugin->config.window_size = atoi(get_text());
	plugin->send_configure_change();
	gui->update_eqcanvas();
	return 1;
}

void ComprMultiSize::create_objects()
{
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	add_item(new BC_MenuItem("262144"));
}

void ComprMultiSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	set_text(string);
}


ComprMultiMkupGain::ComprMultiMkupGain(ComprMultiEffect *plugin,
		ComprMultiWindow *window, int x, int y,
		double *output, double min, double max)
 : BC_FPot(x, y, *output, min, max)
{
	this->window = window;
	this->plugin = plugin;
	this->output = output;
	set_precision(0.01);
}

int ComprMultiMkupGain::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	window->canvas->update();
	return 1;
}


ComprMultiCanvas::ComprMultiCanvas(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y, int w, int h)
 : CompressorCanvasBase(&plugin->config, plugin, window, x, y, w, h)
{
}

void ComprMultiCanvas::update_window()
{
	((ComprMultiWindow*)window)->update();
}


ComprMultiReaction::ComprMultiReaction(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y) 
 : BC_TumbleTextBox(window,
	(float)plugin->config.bands[plugin->config.current_band].attack_len,
	(float)MIN_ATTACK, (float)MAX_ATTACK, x, y, xS(100))
{
	this->plugin = plugin;
	set_increment(0.1);
	set_precision(2);
}

int ComprMultiReaction::handle_event()
{
	plugin->config.bands[plugin->config.current_band].attack_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}



ComprMultiDecay::ComprMultiDecay(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y) 
 : BC_TumbleTextBox(window,
	(float)plugin->config.bands[plugin->config.current_band].release_len,
	(float)MIN_DECAY, (float)MAX_DECAY, x, y, xS(100))
{
	this->plugin = plugin;
	set_increment(0.1);
	set_precision(2);
}
int ComprMultiDecay::handle_event()
{
	plugin->config.bands[plugin->config.current_band].release_len = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


ComprMultiX::ComprMultiX(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y) 
 : BC_TumbleTextBox(window, (float)0.0,
	plugin->config.min_db, plugin->config.max_db, x, y, xS(100))
{
	this->plugin = plugin;
	this->window = window;
	set_increment(0.1);
	set_precision(2);
}
int ComprMultiX::handle_event()
{
	BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

	int current_point = window->canvas->current_point;
	if( current_point < band_config->levels.total ) {
		band_config->levels.values[current_point].x = atof(get_text());
		window->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}


ComprMultiY::ComprMultiY(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y) 
 : BC_TumbleTextBox(window, (float)0.0,
	plugin->config.min_db, plugin->config.max_db, x, y, xS(100))
{
	this->plugin = plugin;
	this->window = window;
	set_increment(0.1);
	set_precision(2);
}
int ComprMultiY::handle_event()
{
	BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];

	int current_point = window->canvas->current_point;
	if( current_point < band_config->levels.total ) {
		band_config->levels.values[current_point].y = atof(get_text());
		window->canvas->update();
		plugin->send_configure_change();
	}
	return 1;
}


ComprMultiTrigger::ComprMultiTrigger(ComprMultiEffect *plugin, 
		ComprMultiWindow *window, int x, int y) 
 : BC_TumbleTextBox(window, (int)plugin->config.trigger,
	MIN_TRIGGER, MAX_TRIGGER, x, y, xS(100))
{
	this->plugin = plugin;
}
int ComprMultiTrigger::handle_event()
{
	plugin->config.trigger = atol(get_text());
	plugin->send_configure_change();
	return 1;
}


ComprMultiInput::ComprMultiInput(ComprMultiEffect *plugin,
	ComprMultiWindow *window, int x, int y) 
 : BC_PopupMenu(x, y, xS(100), 
	ComprMultiInput::value_to_text(plugin->config.input), 1)
{
	this->plugin = plugin;
	this->window = window;
}
int ComprMultiInput::handle_event()
{
	plugin->config.input = text_to_value(get_text());
	window->update();
	plugin->send_configure_change();
	return 1;
}

void ComprMultiInput::create_objects()
{
	for( int i = 0; i < 3; i++ ) {
		add_item(new BC_MenuItem(value_to_text(i)));
	}
}

const char* ComprMultiInput::value_to_text(int value)
{
	switch( value ) {
	case ComprMultiConfig::TRIGGER: return "Trigger";
	case ComprMultiConfig::MAX: return "Maximum";
	case ComprMultiConfig::SUM: return "Total";
	}

	return "Trigger";
}

int ComprMultiInput::text_to_value(char *text)
{
	for( int i = 0; i < 3; i++ ) {
		if( !strcmp(value_to_text(i), text) ) return i;
	}

	return ComprMultiConfig::TRIGGER;
}


ComprMultiClear::ComprMultiClear(ComprMultiEffect *plugin,
		ComprMultiWindow *window, int x, int y) 
 : BC_GenericButton(x, y, _("Clear"))
{
	this->plugin = plugin;
	this->window = window;
}

int ComprMultiClear::handle_event()
{
	BandConfig *band_config = &plugin->config.bands[plugin->config.current_band];
	band_config->mkup_gain = 0.0;
	band_config->levels.remove_all();
//plugin->config.dump();
	window->update();
	plugin->send_configure_change();
	return 1;
}

ComprMultiReset::ComprMultiReset(ComprMultiEffect *plugin,
		ComprMultiWindow *window, int x, int y) 
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}

int ComprMultiReset::handle_event()
{
	plugin->config.q = 1.0;
	plugin->config.window_size = 4096;
	plugin->config.reset_bands();
	plugin->config.reset_base();
//plugin->config.dump();
	window->update();
	plugin->send_configure_change();
	return 1;
}


ComprMultiSmooth::ComprMultiSmooth(ComprMultiEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.smoothing_only, _("Smooth only"))
{
	this->plugin = plugin;
}

int ComprMultiSmooth::handle_event()
{
	plugin->config.smoothing_only = get_value();
	plugin->send_configure_change();
	return 1;
}


ComprMultiSolo::ComprMultiSolo(ComprMultiEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.bands[plugin->config.current_band].solo, _("Solo band"))
{
	this->plugin = plugin;
}

int ComprMultiSolo::handle_event()
{
	plugin->config.bands[plugin->config.current_band].solo = get_value();
	for( int i = 0; i < TOTAL_BANDS; i++ ) {
		if( i != plugin->config.current_band ) {
			plugin->config.bands[i].solo = 0;
		}
	}
	plugin->send_configure_change();
	return 1;
}


ComprMultiBypass::ComprMultiBypass(ComprMultiEffect *plugin, int x, int y) 
 : BC_CheckBox(x, y, plugin->config.bands[plugin->config.current_band].bypass, _("Bypass band"))
{
	this->plugin = plugin;
}

int ComprMultiBypass::handle_event()
{
	plugin->config.bands[plugin->config.current_band].bypass = get_value();
	plugin->send_configure_change();
	return 1;
}


ComprMultiBand::ComprMultiBand(ComprMultiWindow *window, 
		ComprMultiEffect *plugin, int x, int y, int number,
		char *text)
 : BC_Radial(x, y, plugin->config.current_band == number, text)
{
	this->window = window;
	this->plugin = plugin;
	this->number = number;
}

int ComprMultiBand::handle_event()
{
	if( plugin->config.current_band != number ) {
		plugin->config.current_band = number;
		window->update();
	}
	return 1;
}

// dump envelope sum
//	 printf("ComprMultiWindow::update_eqcanvas %d\n", __LINE__);
//	 for( int i = 0; i < plugin->config.window_size / 2; i++ )
//	 {
//		 double sum = 0;
//		 for( int band = 0; band < TOTAL_BANDS; band++ )
//		 {
//			 sum += plugin->engines[band]->envelope[i];
//		 }
//		 
//		 printf("%f ", sum);
//		 for( int band = 0; band < TOTAL_BANDS; band++ )
//		 {
//			 printf("%f ", plugin->engines[band]->envelope[i]);
//		 }
//		 printf("\n");
//	 }


