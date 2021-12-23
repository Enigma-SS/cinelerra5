
/*
 * CINELERRA
 * Copyright (C) 2009-2013 Adam Williams <broadcast at earthling dot net>
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

#include "aattachmentpoint.h"
#include "aedit.h"
#include "amodule.h"
#include "aplugin.h"
#include "arender.h"
#include "asset.h"
#include "atrack.h"
#include "automation.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "language.h"
#include "mainerror.h"
#include "module.h"
#include "patch.h"
#include "plugin.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "samples.h"
#include "sharedlocation.h"
#include "theme.h"
#include "transition.h"
#include "transportque.h"
#include "tracks.h"
#include <string.h>








AModuleResample::AModuleResample(AModule *module)
 : Resample()
{
	this->module = module;
	bzero(nested_output, sizeof(Samples*) * MAX_CHANNELS);
	nested_allocation = 0;
}

AModuleResample::~AModuleResample()
{
	for(int i = 0; i < MAX_CHANNELS; i++)
		delete nested_output[i];
}

int AModuleResample::read_samples(Samples *buffer,
		int64_t start, int64_t len, int direction)
{
	return module->read_samples(buffer, start, len, direction);
}

AModule::AModule(RenderEngine *renderengine,
	CommonRender *commonrender,
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_AUDIO;
	channel = 0;
	transition_temp = 0;
	speed_temp = 0;
	bzero(nested_output, sizeof(Samples*) * MAX_CHANNELS);
	meter_history = new MeterHistory();
	nested_allocation = 0;
	resample = 0;
	asset = 0;
	file = 0;
}




AModule::~AModule()
{
	delete transition_temp;
	delete speed_temp;
	delete meter_history;
	for(int i = 0; i < MAX_CHANNELS; i++)
		delete nested_output[i];
	delete resample;
}

int AModule::read_samples(Samples *buffer, int64_t start, int64_t len, int direction)
{
	if( len < 0 ) return 1;
	double *buffer_data = buffer->get_data();
// if start < 0, zero fill prefix.  if error, zero fill buffer
	int64_t zeros = len;
	int result = 0;
	if( asset ) {
// Files only read going forward.
		if( direction == PLAY_REVERSE ) start -= len;
		int64_t sz = start >= 0 ? len : len + start;
		if( start < 0 ) start = 0;
		if( sz > 0 ) {
			file->set_audio_position(start);
			file->set_channel(channel);
			result = file->read_samples(buffer, sz);
			if( !result && (zeros-=sz) > 0 ) {
				double *top_data = buffer_data + zeros;
				memmove(top_data, buffer_data, sz*sizeof(*buffer_data));
			}
		}
		if( !result && direction == PLAY_REVERSE )
			Resample::reverse_buffer(buffer_data, len);
	}
	else if( nested_edl ) {
		if( nested_allocation < len ) {
			nested_allocation = len;
			for( int i=0; i<nested_edl->session->audio_channels; ++i ) {
				delete nested_output[i];
				nested_output[i] = new Samples(nested_allocation);
			}
		}
		result = nested_renderengine->arender->
			process_buffer(nested_output, len, start);
		if( !result ) {
			double *sample_data = nested_output[channel]->get_data();
			int buffer_size = len * sizeof(*buffer_data);
			memcpy(buffer_data, sample_data, buffer_size);
			zeros = 0;
		}
	}
	if( zeros > 0 )
		memset(buffer_data, 0, zeros*sizeof(*buffer_data));
	return result;
}

AttachmentPoint* AModule::new_attachment(Plugin *plugin)
{
	return new AAttachmentPoint(renderengine, plugin);
}


void AModule::create_objects()
{
	Module::create_objects();
// Not needed in pluginarray
	if( commonrender ) {
		meter_history->init(1, ((ARender*)commonrender)->total_peaks);
		meter_history->reset_channel(0);
	}
}

int AModule::get_buffer_size()
{
	if(renderengine)
		return renderengine->fragment_len;
	else
		return plugin_array->get_bufsize();
}


CICache* AModule::get_cache()
{
	if(renderengine)
		return renderengine->get_acache();
	else
		return cache;
}


int AModule::import_samples(AEdit *edit,
	int64_t start_project, int64_t edit_startproject, int64_t edit_startsource,
	int direction, int sample_rate, Samples *buffer, int64_t fragment_len)
{
	int result = 0;
	if( fragment_len <= 0 )
		result = 1;
	if( nested_edl && edit->channel >= nested_edl->session->audio_channels )
		result = 1;
	double *buffer_data = buffer->get_data();
// buffer fragment adjusted for speed curve
	Samples *speed_buffer = buffer;
	double *speed_data = speed_buffer->get_data();
	int64_t speed_fragment_len = fragment_len;
	int dir = direction == PLAY_FORWARD ? 1 : -1;
// normal speed source boundaries in EDL samplerate
	int64_t start_source = start_project - edit_startproject + edit_startsource;
	double end_source = start_source + dir*speed_fragment_len;
	double start_position = start_source;
//	double end_position = end_source;
// normal speed playback boundaries
	double min_source = bmin(start_source, end_source);
	double max_source = bmax(start_source, end_source);

	this->channel = edit->channel;
	int have_speed = track->has_speed();

// apply speed curve to source position so the timeline agrees with the playback
	if( !result && have_speed ) {
// get speed adjusted start position from start of edit.
		FloatAuto *previous = 0, *next = 0;
		FloatAutos *speed_autos = (FloatAutos*)track->automation->autos[AUTOMATION_SPEED];
		double source_position = edit_startsource +
			speed_autos->automation_integral(edit_startproject,
				start_project-edit_startproject, PLAY_FORWARD);
		min_source = source_position;
		max_source = source_position;
// calculate boundaries of input fragment required for speed curve
		int64_t pos = start_project;
		start_position = source_position;
		for( int64_t i=fragment_len; --i>=0; pos+=dir ) {
			double speed = speed_autos->get_value(pos, direction, previous, next);
			source_position += dir*speed;
			if( source_position > max_source ) max_source = source_position;
			if( source_position < min_source ) min_source = source_position;
		}
//		end_position = source_position;
		speed_fragment_len = (int64_t)(max_source - min_source);
		start_source = direction == PLAY_FORWARD ? min_source : max_source;
		if( speed_fragment_len > 0 ) {
// swap in the temp buffer
			if( speed_temp && speed_temp->get_allocated() < speed_fragment_len ) {
				delete speed_temp;  speed_temp = 0;
			}
			if( !speed_temp )
				speed_temp = new Samples(speed_fragment_len);
			speed_buffer = speed_temp;
			speed_data = speed_buffer->get_data();
		}
	}

	int edit_sample_rate = 0;
	if( speed_fragment_len <= 0 )
		result = 1;

	if( !result && edit->asset ) {
		nested_edl = 0;
		if( nested_renderengine ) {
			delete nested_renderengine;  nested_renderengine = 0;
		}
// Source is an asset
		asset = edit->asset;
		edit_sample_rate = asset->sample_rate;
		get_cache()->age();
		file = get_cache()->check_out(asset, get_edl());
		if( !file ) {
			printf(_("AModule::import_samples Couldn't open %s.\n"), asset->path);
			result = 1;
		}
	}
	else if( !result && edit->nested_edl ) {
		asset = 0;
// Source is a nested EDL
		if( !nested_edl || nested_edl->id != edit->nested_edl->id ) {
			nested_edl = edit->nested_edl;
			delete nested_renderengine;
			nested_renderengine = 0;
		}
		edit_sample_rate = nested_edl->session->sample_rate;
		int command = direction == PLAY_REVERSE ?
			NORMAL_REWIND : NORMAL_FWD;
		if( !nested_command )
			nested_command = new TransportCommand(get_preferences());
		nested_command->command = command;
		nested_command->get_edl()->copy_all(nested_edl);
		nested_command->change_type = CHANGE_ALL;
		nested_command->realtime = renderengine->command->realtime;
		if( !nested_renderengine ) {
			nested_renderengine = new RenderEngine(0, get_preferences(), 0, 1);
			nested_renderengine->set_acache(get_cache());
			nested_renderengine->arm_command(nested_command);
		}
		nested_renderengine->command->command = command;
		result = 0;
	}
	if( edit_sample_rate <= 0 )
		result = 1;

	if( !result ) {
// speed_buffer is (have_speed ? speed_temp : buffer)
		if( sample_rate != edit_sample_rate ) {
			if( !resample )
				resample = new AModuleResample(this);
			result = resample->resample(speed_buffer,
				speed_fragment_len, edit_sample_rate,
				sample_rate, start_source, direction);
		}
		else {
			result = read_samples(speed_buffer,
				start_source, speed_fragment_len, direction);
		}
	}
	if( asset && file ) {
		file = 0;
		get_cache()->check_in(asset);
	}
// Stretch it to fit the speed curve
// Need overlapping buffers to get the interpolation to work, but this
// screws up sequential effects.
	if( !result && have_speed ) {
		FloatAuto *previous = 0, *next = 0;
		FloatAutos *speed_autos = (FloatAutos*)track->automation->autos[AUTOMATION_SPEED];
		int len1 = speed_fragment_len-1;
		double speed_position = start_position;
		double pos = start_project;
// speed		gnuplot> plot "/tmp/x.dat" using($1) with lines
// speed_position	gnuplot> plot "/tmp/x.dat" using($2) with lines
//FILE *fp = 0;
//if( !channel ) { fp = fopen("/tmp/x.dat", "a"); fprintf(fp," %f %f\n",0.,0.); }
		for( int64_t i=0; i<fragment_len; ++i,pos+=dir ) {
			int64_t speed_pos = speed_position;
			double speed = speed_autos->get_value(pos,
				direction, previous, next);
//if(fp) fprintf(fp," %f %f\n", speed, speed_position);
			double next_position = speed_position + dir*speed;
			int64_t next_pos = next_position;
			int total = abs(next_pos - speed_pos);
			int k = speed_pos - min_source;
			if( dir < 0 ) k = len1 - k; // if buffer reversed
			double sample = speed_data[bclip(k, 0,len1)];
			if( total > 1 ) {
				int d = next_pos >= speed_pos ? 1 : -1;
				for( int j=total; --j>0; ) {
					k += d;
					sample += speed_data[bclip(k, 0,len1)];
				}
				sample /= total;
			}
#if 0
			else if( total < 1 ) {
				int d = next_pos >= speed_pos ? 1 : -1;
				k += d;
				double next_sample = speed_data[bclip(k, 0,len1)];
				double v = speed_position - speed_pos;
				sample = (1.-v) * sample + v * next_sample;
			}
#endif
			buffer_data[i] = sample;
			speed_position = next_position;
		}
//if(fp) fclose(fp);
	}

	if( result )
		bzero(buffer_data, fragment_len*sizeof(*buffer_data));
	return result;
}


int AModule::render(Samples *buffer,
	int64_t input_len,
	int64_t start_position,
	int direction,
	int sample_rate,
	int use_nudge)
{
	int64_t edl_rate = get_edl()->session->sample_rate;
	const int debug = 0;

if(debug) printf("AModule::render %d\n", __LINE__);

	if(use_nudge)
		start_position += track->nudge *
			sample_rate /
			edl_rate;
	AEdit *playable_edit;
	int64_t end_position;
	if(direction == PLAY_FORWARD)
		end_position = start_position + input_len;
	else
		end_position = start_position - input_len;
	int buffer_offset = 0;
	int result = 0;


// // Flip range around so the source is always read forward.
// 	if(direction == PLAY_REVERSE)
// 	{
// 		start_project -= input_len;
// 		end_position -= input_len;
// 	}


// Clear buffer
	bzero(buffer->get_data(), input_len * sizeof(double));

// The EDL is normalized to the requested sample rate because
// the requested rate may be the project sample rate and a sample rate
// might as well be directly from the source rate to the requested rate.
// Get first edit containing the range
	if(direction == PLAY_FORWARD)
		playable_edit = (AEdit*)track->edits->first;
	else
		playable_edit = (AEdit*)track->edits->last;
if(debug) printf("AModule::render %d\n", __LINE__);

	while(playable_edit)
	{
		int64_t edit_start = playable_edit->startproject;
		int64_t edit_end = playable_edit->startproject + playable_edit->length;

// Normalize to requested rate
		edit_start = edit_start * sample_rate / edl_rate;
		edit_end = edit_end * sample_rate / edl_rate;

		if(direction == PLAY_FORWARD)
		{
			if(start_position < edit_end && end_position > edit_start)
			{
				break;
			}
			playable_edit = (AEdit*)playable_edit->next;
		}
		else
		{
			if(end_position < edit_end && start_position > edit_start)
			{
				break;
			}
			playable_edit = (AEdit*)playable_edit->previous;
		}
	}


if(debug) printf("AModule::render %d\n", __LINE__);





// Fill output one fragment at a time
	while(start_position != end_position)
	{
		int64_t fragment_len = input_len;

if(debug) printf("AModule::render %d %jd %jd\n", __LINE__, start_position, end_position);
// Clamp fragment to end of input
		if(direction == PLAY_FORWARD &&
			start_position + fragment_len > end_position)
			fragment_len = end_position - start_position;
		else
		if(direction == PLAY_REVERSE &&
			start_position - fragment_len < end_position)
			fragment_len = start_position - end_position;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

// Normalize position here since update_transition is a boolean operation.
		update_transition(start_position *
				edl_rate /
				sample_rate,
			PLAY_FORWARD);

		if(playable_edit)
		{
			AEdit *previous_edit = (AEdit*)playable_edit->previous;

// Normalize EDL positions to requested rate
			int64_t edit_startproject = playable_edit->startproject;
			int64_t edit_endproject = playable_edit->startproject + playable_edit->length;
			int64_t edit_startsource = playable_edit->startsource;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

			edit_startproject = edit_startproject * sample_rate / edl_rate;
			edit_endproject = edit_endproject * sample_rate / edl_rate;
			edit_startsource = edit_startsource * sample_rate / edl_rate;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);



// Clamp fragment to end of edit
			if(direction == PLAY_FORWARD &&
				start_position + fragment_len > edit_endproject)
				fragment_len = edit_endproject - start_position;
			else
			if(direction == PLAY_REVERSE &&
				start_position - fragment_len < edit_startproject)
				fragment_len = start_position - edit_startproject;
if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

// Clamp to end of transition
			int64_t transition_len = 0;
			Plugin *transition = get_edl()->tracks->plugin_exists(transition_id);
			if( transition && transition->on && previous_edit ) {
				transition_len = transition->length * sample_rate / edl_rate;
				if(direction == PLAY_FORWARD &&
					start_position < edit_startproject + transition_len &&
					start_position + fragment_len > edit_startproject + transition_len)
					fragment_len = edit_startproject + transition_len - start_position;
				else
				if(direction == PLAY_REVERSE &&
					start_position > edit_startproject + transition_len &&
					start_position - fragment_len < edit_startproject + transition_len)
					fragment_len = start_position - edit_startproject - transition_len;
			}
if(debug) printf("AModule::render %d buffer_offset=%d fragment_len=%jd\n",
__LINE__,
buffer_offset,
fragment_len);

			Samples output(buffer);
			output.set_offset(output.get_offset() + buffer_offset);
			if(import_samples(playable_edit,
				start_position,
				edit_startproject,
				edit_startsource,
				direction,
				sample_rate,
				&output,
				fragment_len)) result = 1;

if(debug) printf("AModule::render %d\n", __LINE__);


// Read transition into temp and render
			if(transition && transition->on && previous_edit)
			{
				int64_t previous_startproject = previous_edit->startproject *
					sample_rate /
					edl_rate;
				int64_t previous_startsource = previous_edit->startsource *
					sample_rate /
					edl_rate;

// Allocate transition temp size
				int transition_fragment_len = fragment_len;
				if(direction == PLAY_FORWARD &&
					fragment_len + start_position > edit_startproject + transition_len)
					fragment_len = edit_startproject + transition_len - start_position;


// Read into temp buffers
// Temp + master or temp + temp ? temp + master
				if(transition_temp &&
					transition_temp->get_allocated() < fragment_len)
				{
					delete transition_temp;
					transition_temp = 0;
				}

				if(!transition_temp)
				{
					transition_temp = new Samples(fragment_len);
				}

if(debug) printf("AModule::render %d %jd\n", __LINE__, fragment_len);

				if(transition_fragment_len > 0)
				{
// Previous_edit is always the outgoing segment, regardless of direction
					import_samples(previous_edit,
						start_position,
						previous_startproject,
						previous_startsource,
						direction,
						sample_rate,
						transition_temp,
						transition_fragment_len);
					int64_t current_position;

// Reverse buffers here so transitions always render forward.
					if(direction == PLAY_REVERSE)
					{
						Resample::reverse_buffer(output.get_data(), transition_fragment_len);
						Resample::reverse_buffer(transition_temp->get_data(), transition_fragment_len);
						current_position = start_position -
							transition_fragment_len -
							edit_startproject;
					}
					else
					{
						current_position = start_position - edit_startproject;
					}
					if( transition_server ) {
						transition_server->process_transition(
							transition_temp, &output, current_position,
							transition_fragment_len, transition->length);
					}
					else
						eprintf("missing transition plugin: %s\n", transition->title);

// Reverse output buffer here so transitions always render forward.
					if(direction == PLAY_REVERSE)
						Resample::reverse_buffer(output.get_data(),
							transition_fragment_len);
				}
			}
if(debug) printf("AModule::render %d start_position=%jd end_position=%jd fragment_len=%jd\n",
 __LINE__, start_position, end_position, fragment_len);

			if(direction == PLAY_REVERSE)
			{
				if(playable_edit && start_position - fragment_len <= edit_startproject)
					playable_edit = (AEdit*)playable_edit->previous;
			}
			else
			{
				if(playable_edit && start_position + fragment_len >= edit_endproject)
					playable_edit = (AEdit*)playable_edit->next;
			}
		}

		if(fragment_len > 0)
		{
			buffer_offset += fragment_len;
			if(direction == PLAY_FORWARD)
				start_position += fragment_len;
			else
				start_position -= fragment_len;
		}
	}

if(debug) printf("AModule::render %d\n", __LINE__);

	return result;
}









