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

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "comprmulti.h"
#include "comprmultigui.h"
#include "bchash.h"
#include "eqcanvas.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>

REGISTER_PLUGIN(ComprMultiEffect)

// More potential compressor algorithms:
// Use single reaction time parameter.  Negative reaction time uses
// readahead.  Positive reaction time uses slope.

// Smooth input stage if readahead.
// Determine slope from current smoothed sample to every sample in readahead area.
// Once highest slope is found, count of number of samples remaining until it is
// reached.  Only search after this count for the next highest slope.
// Use highest slope to determine smoothed value.

// Smooth input stage if not readahead.
// For every sample, calculate slope needed to reach current sample from
// current smoothed value in the reaction time.  If higher than current slope,
// make it the current slope and count number of samples remaining until it is
// reached.  If this count is met and no higher slopes are found, base slope
// on current sample when count is met.

// Gain stage.
// For every sample, calculate gain from smoothed input value.

ComprMultiConfig::ComprMultiConfig()
 : CompressorConfigBase(TOTAL_BANDS)
{
	q = 1.0;
	window_size = 4096;
}

void ComprMultiConfig::copy_from(ComprMultiConfig &that)
{
	CompressorConfigBase::copy_from(that);

	window_size = that.window_size;
	q = that.q;
}

int ComprMultiConfig::equivalent(ComprMultiConfig &that)
{
	if( !CompressorConfigBase::equivalent(that) )
		return 0;

	if( !EQUIV(q, that.q) ||
		window_size != that.window_size ) {
		return 0;
	}

	return 1;
}

void ComprMultiConfig::interpolate(ComprMultiConfig &prev, ComprMultiConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


ComprMultiEffect::ComprMultiEffect(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	for( int i = 0; i < TOTAL_BANDS; i++ )
		band_states[i] = new BandState(this, i);
}

ComprMultiEffect::~ComprMultiEffect()
{
   	delete_dsp();
	for( int i = 0; i < TOTAL_BANDS; i++ ) {
		delete band_states[i];
	}
}

void ComprMultiEffect::delete_dsp()
{
#ifndef DRAW_AFTER_BANDPASS
	if( input_buffer ) {
		for( int i = 0; i < PluginClient::total_in_buffers; i++ )
			delete input_buffer[i];
		delete [] input_buffer;
	}
	input_buffer = 0;
	input_size = 0;
	new_input_size = 0;
#endif
	if( fft ) {
		for( int i = 0; i < PluginClient::total_in_buffers; i++ )
			delete fft[i];
		delete [] fft;
	}

	for( int i = 0; i < TOTAL_BANDS; i++ ) {
		band_states[i]->delete_dsp();
	}

	filtered_size = 0;
	fft = 0;
}


void ComprMultiEffect::reset()
{
	for( int i = 0; i < TOTAL_BANDS; i++ )
		band_states[i] = 0;

#ifndef DRAW_AFTER_BANDPASS
	input_buffer = 0;
	input_size = 0;
	new_input_size = 0;
#endif
	input_start = 0;
	filtered_size = 0;
	last_position = 0;
	fft = 0;
	need_reconfigure = 1;
	config.current_band = 0;
}

const char* ComprMultiEffect::plugin_title() { return N_("Compressor Multi"); }
int ComprMultiEffect::is_realtime() { return 1; }
int ComprMultiEffect::is_multichannel() { return 1; }


void ComprMultiEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	for( int i = 0; i < TOTAL_BANDS; i++ )
		config.bands[i].levels.remove_all();

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("COMPRESSOR_MULTI") ) {
			config.trigger = input.tag.get_property("TRIGGER", config.trigger);
			config.smoothing_only = input.tag.get_property("SMOOTHING_ONLY", config.smoothing_only);
			config.input = input.tag.get_property("INPUT", config.input);
			config.q = input.tag.get_property("Q", config.q);
			config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
		}
		else if( input.tag.title_is("COMPRESSORBAND") ) {
			int number = input.tag.get_property("NUMBER", 0);
			config.bands[number].read_data(&input, 1);
		}
	}

	config.boundaries();
}

void ComprMultiEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("COMPRESSOR_MULTI");
	output.tag.set_property("TRIGGER", config.trigger);
	output.tag.set_property("SMOOTHING_ONLY", config.smoothing_only);
	output.tag.set_property("INPUT", config.input);
	output.tag.set_property("Q", config.q);
	output.tag.set_property("WINDOW_SIZE", config.window_size);
	output.append_tag();
	output.append_newline();

	for( int band = 0; band < TOTAL_BANDS; band++ ) {
		BandConfig *band_config = &config.bands[band];
		band_config->save_data(&output, band, 1);
	}


	output.tag.set_title("/COMPRESSOR_MULTI");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void ComprMultiEffect::dump_frames()
{
	printf("tracking %f, direction %d\n", get_tracking_position(), get_tracking_direction());
	CompressorClientFrame *cfp = (CompressorClientFrame *)client_frames.first;
	for( int n=0; cfp; cfp=(CompressorClientFrame *)cfp->next,++n ) {
		switch( cfp->type ) {
		case GAIN_COMPRESSORFRAME: {
			CompressorGainFrame *gfp = (CompressorGainFrame *)cfp;
			printf("%3d: band %d, gain pos=%f, gain=%f, level=%f\n", n, cfp->band,
				gfp->position, gfp->gain, gfp->level);
			break; }
		case FREQ_COMPRESSORFRAME: {
			CompressorFreqFrame *ffp = (CompressorFreqFrame *)cfp;
			printf("%3d: band %d, freq pos=%f, max=%f/%f, size=%d\n", n, ffp->band,
				ffp->position, ffp->freq_max, ffp->time_max, ffp->data_size);
			break; }
		}
	}
}

void ComprMultiEffect::update_gui()
{
//	dump_frames();
	if( !thread ) return;
	ComprMultiWindow *window = (ComprMultiWindow *)thread->window;
	if( !window ) return;
// user can't change levels when loading configuration
	window->lock_window("ComprMultiEffect::update_gui 1");
	int reconfigured = 0;
// Can't update points if the user is editing
	if( window->canvas->is_dragging() )
		reconfigured = load_configuration();
//printf("ComprMultiEffect::update_gui %d %d %d\n", __LINE__, reconfigured, total_frames);
	if( reconfigured )
		window->update();
	if( pending_gui_frame() )
		window->update_eqcanvas();
	window->unlock_window();
}


void ComprMultiEffect::render_stop()
{
	if( !thread ) return;
	ComprMultiWindow *window = (ComprMultiWindow*)thread->window;
	if( !window ) return;
	window->lock_window("ComprMultiEffect::render_stop");
	window->in->reset();
	window->gain_change->update(1, 0);
	delete window->gain_frame;  window->gain_frame = 0;
	delete window->freq_frame;  window->freq_frame = 0;
	window->unlock_window();
}


LOAD_CONFIGURATION_MACRO(ComprMultiEffect, ComprMultiConfig)
NEW_WINDOW_MACRO(ComprMultiEffect, ComprMultiWindow)


int ComprMultiEffect::process_buffer(int64_t size, Samples **buffer,
		int64_t start_position, int sample_rate)
{
	start_pos = (double)start_position / sample_rate;
	dir = get_direction() == PLAY_REVERSE ? -1 : 1;
	need_reconfigure |= load_configuration();
	int channels = PluginClient::total_in_buffers;
	if( need_reconfigure ) {
		need_reconfigure = 0;
//		min_x = DB::fromdb(config.min_db);  max_x = 1.0;
//		min_y = DB::fromdb(config.min_db);  max_y = 1.0;

		if( fft && fft[0]->window_size != config.window_size ) {
			for( int i = 0; i < channels; i++ )
 				delete fft[i];
			delete [] fft;
			fft = 0;
		}

		if( !fft ) {
			fft = new ComprMultiFFT*[channels];
			for( int i = 0; i < channels; i++ ) {
				fft[i] = new ComprMultiFFT(this, i);
				fft[i]->initialize(config.window_size, TOTAL_BANDS);
			}
		}

		for( int i = 0; i < TOTAL_BANDS; i++ )
			band_states[i]->reconfigure();
	}

// reset after seeking
	if( last_position != start_position ) {
		if( fft ) {
			for( int i = 0; i < channels; i++ )
 				if( fft[i] ) fft[i]->delete_fft();
		}

#ifndef DRAW_AFTER_BANDPASS
		input_size = 0;
#endif
		filtered_size = 0;
		input_start = start_position;
		for( int band = 0; band < TOTAL_BANDS; band++ ) {
			BandState *band_state = band_states[band];
			if( band_state->engine )
				band_state->engine->reset();
		}
	}

// process frequency domain for all bands simultaneously
// read enough samples ahead to process all the bands
	int new_filtered_size = 0;
	for( int band = 0; band < TOTAL_BANDS; band++ ) {
		BandState *band_state = band_states[band];
		if( !band_state->engine )
			band_state->engine = new CompressorEngine(&config, band);
		int attack_samples, release_samples, preview_samples;
		band_state->engine->calculate_ranges(
			&attack_samples, &release_samples, &preview_samples,
			sample_rate);

		if( preview_samples > new_filtered_size )
			new_filtered_size = preview_samples;
	}
	new_filtered_size += size;
	for( int band = 0; band < TOTAL_BANDS; band++ )
		band_states[band]->allocate_filtered(new_filtered_size);

// Append data to the buffers to fill the readahead area.
	int remain = new_filtered_size - filtered_size;

	if( remain > 0 ) {
// printf("ComprMultiEffect::process_buffer %d filtered_size=%ld remain=%d\n",
// __LINE__, filtered_size, remain);
		for( int channel = 0; channel < channels; channel++ ) {
#ifndef DRAW_AFTER_BANDPASS
			new_input_size = input_size;
#endif
// create an array of filtered buffers for the output
			Samples *filtered_arg[TOTAL_BANDS];
			for( int band = 0; band < TOTAL_BANDS; band++ ) {
				new_spectrogram_frames[band] = 0;
				filtered_arg[band] = band_states[band]->filtered_buffer[channel];
// temporarily set the output to the end to append data
				filtered_arg[band]->set_offset(filtered_size);
			}

// starting position of the input reads
			int64_t start = input_start + dir * filtered_size;
// printf("ComprMultiEffect::process_buffer %d start=%ld remain=%d\n", __LINE__, start, remain);
			fft[channel]->process_buffer(start, remain, filtered_arg, get_direction());

			for( int band = 0; band < TOTAL_BANDS; band++ ) {
				filtered_arg[band]->set_offset(0);
			}
//printf("ComprMultiEffect::process_buffer %d new_input_size=%ld\n", __LINE__, new_input_size);
		}
	}

#ifndef DRAW_AFTER_BANDPASS
	input_size = new_input_size;
#endif
	filtered_size = new_filtered_size;

// process time domain for each band separately
	for( int band = 0; band < TOTAL_BANDS; band++ ) {
		BandState *band_state = band_states[band];
		CompressorEngine *engine = band_state->engine;

		engine->process(band_states[band]->filtered_buffer,
			band_states[band]->filtered_buffer,
			size, sample_rate, channels, start_position);

		for( int i = 0; i < engine->gui_gains.size(); i++ ) {
			CompressorGainFrame *frame = new CompressorGainFrame();
			frame->position = start_pos + dir*engine->gui_offsets[i];
			frame->gain = engine->gui_gains.get(i);
			frame->level = engine->gui_levels.get(i);
			frame->band = band;
			add_gui_frame(frame);
		}
	}

// Add together filtered buffers + unfiltered buffer.
// Apply the solo here.
	int have_solo = 0;
	for( int band = 0; band < TOTAL_BANDS; band++ ) {
		if( config.bands[band].solo ) {
			have_solo = 1;
			break;
		}
	}

	for( int channel = 0; channel < channels; channel++ ) {
		double *dst = buffer[channel]->get_data();
		bzero(dst, size * sizeof(double));

		for( int band = 0; band < TOTAL_BANDS; band++ ) {
			if( !have_solo || config.bands[band].solo ) {
				double *src = band_states[band]->filtered_buffer[channel]->get_data();
				for( int i = 0; i < size; i++ ) {
					dst[i] += src[i];
				}
			}
		}
	}

// shift input buffers
	for( int band = 0; band < TOTAL_BANDS; band++ ) {

		for( int i = 0; i < channels; i++ ) {
			memcpy(band_states[band]->filtered_buffer[i]->get_data(),
				band_states[band]->filtered_buffer[i]->get_data() + size,
				(filtered_size - size) * sizeof(double));

		}
	}

#ifndef DRAW_AFTER_BANDPASS
	for( int i = 0; i < channels; i++ ) {
		memcpy(input_buffer[i]->get_data(),
			input_buffer[i]->get_data() + size,
			(input_size - size) * sizeof(double));
	}
	input_size -= size;
#endif

// update the counters
	filtered_size -= size;
	input_start += dir * size;
	last_position = start_position + dir * size;
	return 0;
}

void ComprMultiEffect::allocate_input(int new_size)
{
#ifndef DRAW_AFTER_BANDPASS
	if( !input_buffer ||
		new_size > input_buffer[0]->get_allocated() ) {
		Samples **new_input_buffer = new Samples*[get_total_buffers()];

		for( int i = 0; i < get_total_buffers(); i++ ) {
			new_input_buffer[i] = new Samples(new_size);

			if( input_buffer ) {
				memcpy(new_input_buffer[i]->get_data(),
					input_buffer[i]->get_data(),
					input_buffer[i]->get_allocated() * sizeof(double));
				delete input_buffer[i];
			}
		}

		if( input_buffer ) delete [] input_buffer;
		input_buffer = new_input_buffer;
   	}
#endif // !DRAW_AFTER_BANDPASS
}


void ComprMultiEffect::calculate_envelope()
{
	for( int i = 0; i < TOTAL_BANDS; i++ ) {
		band_states[i]->calculate_envelope();
	}
}


BandState::BandState(ComprMultiEffect *plugin, int band)
{
	this->plugin = plugin;
	this->band = band;
	reset();
}

BandState::~BandState()
{
	delete_dsp();
}

void BandState::delete_dsp()
{
	delete [] envelope;
	levels.remove_all();
	if( filtered_buffer ) {
		for( int i = 0; i < plugin->total_in_buffers; i++ ) {
			delete filtered_buffer[i];
		}
		delete [] filtered_buffer;
	}
	if( engine ) {
		delete engine;
	}
	reset();
}

void BandState::reset()
{
	engine = 0;
	envelope = 0;
	envelope_allocated = 0;
	filtered_buffer = 0;

	next_target = 1.0;
	previous_target = 1.0;
	target_samples = 1;
	target_current_sample = -1;
	current_value = 1.0;
}

void BandState::reconfigure()
{
// Calculate linear transfer from db
	levels.remove_all();

	BandConfig *config = &plugin->config.bands[band];
	for( int i = 0; i < config->levels.total; i++ ) {
		levels.append();
		levels.values[i].x = DB::fromdb(config->levels.values[i].x);
		levels.values[i].y = DB::fromdb(config->levels.values[i].y);
	}

	calculate_envelope();
}


void BandState::allocate_filtered(int new_size)
{
	if( !filtered_buffer ||
		new_size > filtered_buffer[0]->get_allocated() ) {
		Samples **new_filtered_buffer = new Samples*[plugin->get_total_buffers()];
		for( int i = 0; i < plugin->get_total_buffers(); i++ ) {
			new_filtered_buffer[i] = new Samples(new_size);

			if( filtered_buffer ) {
				memcpy(new_filtered_buffer[i]->get_data(),
					filtered_buffer[i]->get_data(),
					filtered_buffer[i]->get_allocated() * sizeof(double));
				delete filtered_buffer[i];
			}
		}

		if( filtered_buffer ) delete [] filtered_buffer;
		filtered_buffer = new_filtered_buffer;
	}
}


void BandState::calculate_envelope()
{
// the window size changed
	if( envelope && envelope_allocated < plugin->config.window_size / 2 ) {
		delete [] envelope;
		envelope = 0;
	}

	if( !envelope ) {
		envelope_allocated = plugin->config.window_size / 2;
		envelope = new double[envelope_allocated];
	}

// number of slots in the edge
	double edge = (1.0 - plugin->config.q) * TOTALFREQS / 2;
	int max_freq = Freq::tofreq_f(TOTALFREQS - 1);
	int nyquist = plugin->project_sample_rate / 2;
	int freq = plugin->config.bands[band].freq;

// max frequency of all previous bands is the low
	int low = 0;
	for( int i=0; i<band; ++i ) {
		if( plugin->config.bands[i].freq > low )
			low = plugin->config.bands[i].freq;
	}
	int high = max_freq;
// limit the frequencies
	if( band < TOTAL_BANDS-1 ) high = freq;
	if( high >= max_freq ) { high = max_freq;  edge = 0; }
// hard edges on the lowest & highest
	if( band == 0 && high <= 0 ) edge = 0;
	if( low > high ) low = high;
// number of slots to arrive at 1/2 power
#ifndef LOG_CROSSOVER
// linear
	double edge2 = edge / 2;
#else
// log
	double edge2 = edge * 6 / -INFINITYGAIN;
#endif
	double low_slot = Freq::fromfreq_f(low);
	double high_slot = Freq::fromfreq_f(high);
// shift slots to allow crossover
	if( band < TOTAL_BANDS-1 ) high_slot -= edge2;
	if( band > 0 ) low_slot += edge2;

	for( int i = 0; i < plugin->config.window_size / 2; i++ ) {
		double freq = i * nyquist / (plugin->config.window_size / 2);
		double slot = Freq::fromfreq_f(freq);
// sum of previous bands
		double prev_sum = 0;
		for( int prev_band = 0; prev_band < band; prev_band++ ) {
			double *prev_envelope = plugin->band_states[prev_band]->envelope;
			prev_sum += prev_envelope[i];
		}

		if( slot < high_slot )
// remain of previous bands
			envelope[i] = 1.0 - prev_sum;
		else if( slot < high_slot + edge ) {
// next crossover
			double remain = 1.0 - prev_sum;
#ifndef LOG_CROSSOVER
// linear
			envelope[i] = remain - remain * (slot - high_slot) / edge;
#else
// log TODO
			envelope[i] = DB::fromdb((slot - high_slot) * INFINITYGAIN / edge);
#endif

		}
		else
			envelope[i] = 0.0;
	}
}

#if 0
void BandState::process_readbehind(int size,
		int reaction_samples, int decay_samples, int trigger)
{
	if( target_current_sample < 0 )
		target_current_sample = reaction_samples;
	double current_slope = (next_target - previous_target) / reaction_samples;
	double *trigger_buffer = filtered_buffer[trigger]->get_data();
	int channels = plugin->get_total_buffers();
	for( int i = 0; i < size; i++ ) {
// Get slope required to reach current sample from smoothed sample over reaction
// length.
		double sample = 0;
		switch( plugin->config.input ) {
		case ComprMultiConfig::MAX: {
			double max = 0;
			for( int j = 0; j < channels; j++ ) {
				sample = fabs(filtered_buffer[j]->get_data()[i]);
				if( sample > max ) max = sample;
			}
			sample = max;
			break; }

		case ComprMultiConfig::TRIGGER:
			sample = fabs(trigger_buffer[i]);
			break;

		case ComprMultiConfig::SUM: {
			double max = 0;
			for( int j = 0; j < channels; j++ ) {
				sample = fabs(filtered_buffer[j]->get_data()[i]);
				max += sample;
			}
			sample = max;
			break; }
		}

		double new_slope = (sample - current_value) / reaction_samples;

// Slope greater than current slope
		if( new_slope >= current_slope &&
			(current_slope >= 0 ||
			new_slope >= 0) ) {
			next_target = sample;
			previous_target = current_value;
			target_current_sample = 0;
			target_samples = reaction_samples;
			current_slope = new_slope;
		}
		else
		if( sample > next_target && current_slope < 0 ) {
			next_target = sample;
			previous_target = current_value;
			target_current_sample = 0;
			target_samples = decay_samples;
			current_slope = (sample - current_value) / decay_samples;
		}
// Current smoothed sample came up without finding higher slope
		if( target_current_sample >= target_samples ) {
			next_target = sample;
			previous_target = current_value;
			target_current_sample = 0;
			target_samples = decay_samples;
			current_slope = (sample - current_value) / decay_samples;
		}

// Update current value and store gain
		current_value = (next_target * target_current_sample +
			previous_target * (target_samples - target_current_sample)) /
			target_samples;

		target_current_sample++;

		if( plugin->config.smoothing_only ) {
			for( int j = 0; j < channels; j++ ) {
				filtered_buffer[j]->get_data()[i] = current_value;
			}
		}
		else
		if( !plugin->config.bands[band].bypass ) {
			double gain = plugin->config.calculate_gain(band, current_value);
			for( int j = 0; j < channels; j++ ) {
				filtered_buffer[j]->get_data()[i] *= gain;
			}
		}
	}
}

void BandState::process_readahead(int size, int preview_samples,
		int reaction_samples, int decay_samples, int trigger)
{
	if( target_current_sample < 0 ) target_current_sample = target_samples;
	double current_slope = (next_target - previous_target) /
		target_samples;
	double *trigger_buffer = filtered_buffer[trigger]->get_data();
	int channels = plugin->get_total_buffers();
	for( int i = 0; i < size; i++ ) {
// Get slope from current sample to every sample in preview_samples.
// Take highest one or first one after target_samples are up.

// For optimization, calculate the first slope we really need.
// Assume every slope up to the end of preview_samples has been calculated and
// found <= to current slope.
		int first_slope = preview_samples - 1;
// Need new slope immediately
		if( target_current_sample >= target_samples )
			first_slope = 1;

		for( int j = first_slope; j < preview_samples; j++ ) {
			int buffer_offset = i + j;
			double sample = 0;
			switch( plugin->config.input ) {
			case ComprMultiConfig::MAX: {
				double max = 0;
				for( int k = 0; k < channels; k++ ) {
					sample = fabs(filtered_buffer[k]->get_data()[buffer_offset]);
					if( sample > max ) max = sample;
				}
				sample = max;
				break; }

			case ComprMultiConfig::TRIGGER:
				sample = fabs(trigger_buffer[buffer_offset]);
				break;

			case ComprMultiConfig::SUM: {
				double max = 0;
				for( int k = 0; k < channels; k++ ) {
					sample = fabs(filtered_buffer[k]->get_data()[buffer_offset]);
					max += sample;
				}
				sample = max;
				break; }
			}

			double new_slope = (sample - current_value) / j;
// Got equal or higher slope
			if( new_slope >= current_slope &&
				(current_slope >= 0 ||
				new_slope >= 0) ) {
				target_current_sample = 0;
				target_samples = j;
				current_slope = new_slope;
				next_target = sample;
				previous_target = current_value;
			}
			else
			if( sample > next_target && current_slope < 0 ) {
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) /
					decay_samples;
				next_target = sample;
				previous_target = current_value;
			}

// Hit end of current slope range without finding higher slope
			if( target_current_sample >= target_samples ) {
				target_current_sample = 0;
				target_samples = decay_samples;
				current_slope = (sample - current_value) / decay_samples;
				next_target = sample;
				previous_target = current_value;
			}
		}

// Update current value and multiply gain
		current_value = (next_target * target_current_sample +
			previous_target * (target_samples - target_current_sample)) /
			target_samples;

		target_current_sample++;

		if( plugin->config.smoothing_only ) {
			for( int j = 0; j < channels; j++ ) {
				filtered_buffer[j]->get_data()[i] = current_value;
			}
		}
		else
		if( !plugin->config.bands[band].bypass ) {
			double gain = plugin->config.calculate_gain(band, current_value);
			for( int j = 0; j < channels; j++ ) {
				filtered_buffer[j]->get_data()[i] *= gain;
			}
		}
	}
}
#endif

ComprMultiFFT::ComprMultiFFT(ComprMultiEffect *plugin, int channel)
{
	this->plugin = plugin;
	this->channel = channel;
}

ComprMultiFFT::~ComprMultiFFT()
{
}

int ComprMultiFFT::signal_process(int band)
{
	int sample_rate = plugin->PluginAClient::project_sample_rate;
	BandState *band_state = plugin->band_states[band];

// Create new spectrogram frame for updating the GUI
	frame = 0;
	if(
#ifndef DRAW_AFTER_BANDPASS
		band == 0 &&
#endif
		((plugin->config.input != ComprMultiConfig::TRIGGER && channel == 0) ||
		channel == plugin->config.trigger) ) {
#ifndef DRAW_AFTER_BANDPASS
		int total_data = window_size / 2;
#else
		int total_data = TOTAL_BANDS * window_size / 2;
#endif

// store all bands in the same GUI frame
		frame = new CompressorFreqFrame();
		frame->band = band;
		frame->data_size = total_data;
		frame->data = new double[total_data];
		bzero(frame->data, sizeof(double) * total_data);
		frame->nyquist = sample_rate / 2;

// 		int attack_samples, release_samples, preview_samples;
//		band_state->engine->calculate_ranges(&attack_samples,
//			 &release_samples, &preview_samples, sample_rate);

// FFT advances 1/2 a window for each spectrogram frame
		int n = plugin->new_spectrogram_frames[band]++;
		double sample_pos = (n * window_size / 2) / sample_rate;
		frame->position = plugin->start_pos + plugin->dir * sample_pos;
//if( band == 1 ) {
// printf("ComprMultiFFT::signal_process %d top_position=%ld frame->position=%ld\n",
// __LINE__, plugin->get_playhead_position(), frame->position);
// printf("ComprMultiFFT::signal_process %d band=%d preview_samples=%d frames size=%ld filtered_size=%ld\n",
// __LINE__, band, preview_samples, plugin->new_spectrogram_frames[band] *
//  window_size, plugin->filtered_size);
//}
	}
//printf("ComprMultiFFT::signal_process %d channel=%d band=%d frame=%p\n", __LINE__, channel, band, frame);
// apply the bandpass filter
	for( int i = 0; i < window_size / 2; i++ ) {
		double fr = freq_real[i], fi = freq_imag[i];
		double env = band_state->envelope[i];
		freq_real[i] *= env;  freq_imag[i] *= env;
		double mag = sqrt(fr*fr + fi*fi);

// update the spectrogram with the output
// neglect the true average & max spectrograms, but always use the trigger
		if( frame ) {
			int offset = band * window_size / 2 + i;
#ifndef DRAW_AFTER_BANDPASS
			frame->data[offset] = MAX(frame->data[offset], mag);
// get the maximum output in the frequency domain
			if( mag > frame->freq_max )
				frame->freq_max = mag;
#else
			mag *= env;
			frame->data[offset] = MAX(frame->data[offset], mag);
// get the maximum output in the frequency domain
			if( mag > frame->freq_max )
				frame->freq_max = mag;
#endif
		}
	}

	symmetry(window_size, freq_real, freq_imag);
	return 0;
}


int ComprMultiFFT::post_process(int band)
{
	if( frame ) {
// get the maximum output in the time domain
		double *buffer = output_real;
#ifndef DRAW_AFTER_BANDPASS
		buffer = input_buffer->get_data();
#endif
		double time_max = 0;
		for( int i = 0; i<window_size; ++i ) {
			if( fabs(buffer[i]) > time_max )
				time_max = fabs(buffer[i]);
		}
		if( time_max > frame->time_max )
			frame->time_max = time_max;
		plugin->add_gui_frame(frame);
	}
	return 0;
}


int ComprMultiFFT::read_samples(int64_t output_sample,
		int samples, Samples *buffer)
{
	int result = plugin->read_samples(buffer, channel,
			plugin->get_samplerate(), output_sample, samples);
#ifndef DRAW_AFTER_BANDPASS
// append unprocessed samples to the input_buffer
	int new_input_size = plugin->new_input_size + samples;
	plugin->allocate_input(new_input_size);
	memcpy(plugin->input_buffer[channel]->get_data() + plugin->new_input_size,
		buffer->get_data(), samples * sizeof(double));
	plugin->new_input_size = new_input_size;
#endif // !DRAW_AFTER_BANDPASS
	return result;
}

