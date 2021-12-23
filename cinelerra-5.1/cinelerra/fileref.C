
/*
 * CINELERRA
 * Copyright (C) 2020 William Morrow
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
#include "arender.h"
#include "cache.h"
#include "edl.h"
#include "filebase.h"
#include "file.h"
#include "fileref.h"
#include "language.h"
#include "mainerror.h"
#include "renderengine.h"
#include "samples.h"
#include "edlsession.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"
#include "vrender.h"
#include "filexml.h"


FileREF::FileREF(Asset *asset, File *file)
 : FileBase(asset, file)
{
	is_open = 0;
	audio_position = 0;
	video_position = 0;
	samples_position = -1;
	samples_length = -1;
	channel = 0;
	layer = 0;
	ref = 0;
	command = 0;
	render_engine = 0;
	acache = 0;
	vcache = 0;
	temp = 0;
	for( int i=0; i<MAX_CHANNELS; ++i ) samples[i] = 0;
}

FileREF::~FileREF()
{
	close_file();
}

int FileREF::open_file(int rd, int wr)
{
	if( is_open ) return 1;
	if(wr) {
		eprintf(_("Reference files cant be created by rendering\n"));
		return 1;
	}
	if(rd) {
		FileXML file_xml;
		if( file_xml.read_from_file(asset->path) ) return 1;
//		file_xml.check_version();
		if( ref ) ref->remove_user();
		ref = new EDL;
		ref->create_objects();
		if( ref->load_xml(&file_xml, LOAD_ALL) ) {
			ref->remove_user();
			ref = 0;
			eprintf(_("Error loading Reference file:\n%s"), asset->path);
			return 1;
		}
		if( !asset->layers ) asset->layers = ref->get_video_layers();
		asset->video_data = asset->layers > 0 ? 1 : 0;
		asset->video_length = asset->video_data ? ref->get_video_frames() : 0;
		asset->actual_width = asset->video_data ? ref->get_w() : 0;
		asset->actual_height = asset->video_data ? ref->get_h() : 0;
		if( !asset->width ) asset->width = asset->actual_width;
		if( !asset->height ) asset->height = asset->actual_height;
		if( !asset->frame_rate ) asset->frame_rate = ref->get_frame_rate();
		strcpy(asset->vcodec, "REF");
		asset->channels = ref->get_audio_channels();
		asset->audio_data = asset->channels > 0 ? 1 : 0;
		asset->sample_rate = ref->get_sample_rate();
		asset->audio_length = asset->audio_data ? ref->get_audio_samples() : 0;
		strcpy(asset->acodec, "REF");
		command = new TransportCommand(file->preferences);
		command->reset();
		command->get_edl()->copy_all(ref);
		command->command = NORMAL_FWD;
		command->change_type = CHANGE_ALL;
		command->realtime = 0;
		samples_position = -1;
		samples_length = -1;
		audio_position = 0;
		render_engine = new RenderEngine(0, file->preferences, 0, 0);
		render_engine->set_acache(acache = new CICache(file->preferences));
		render_engine->set_vcache(vcache = new CICache(file->preferences));
		render_engine->arm_command(command);
		is_open = 1;
	}
	return 0;
}

int FileREF::close_file()
{
	if( !is_open ) return 1;
	if( ref ) ref->remove_user();
	ref = 0;
	delete render_engine;  render_engine = 0;
	delete command;  command = 0;
	if( acache ) { acache->remove_user();  acache = 0; }
	if( vcache ) { vcache->remove_user();  vcache = 0; }
	delete temp;     temp = 0;
	for( int i=0; i<MAX_CHANNELS; ++i ) {
		delete samples[i];  samples[i] = 0;
	}
	audio_position = 0;
	video_position = 0;
	channel = 0;
	samples_position = -1;
	samples_length = -1;
	layer = 0;
	is_open = 0;
	return 0;
}

int64_t FileREF::get_video_position()
{
	return video_position;
}

int64_t FileREF::get_audio_position()
{
	return audio_position;
}

int FileREF::set_video_position(int64_t pos)
{
	this->video_position = pos;
	return 0;
}
int FileREF::set_layer(int layer)
{
	this->layer = layer;
	return 0;
}

int FileREF::set_audio_position(int64_t pos)
{
	this->audio_position = pos;
	return 0;
}
int FileREF::set_channel(int channel)
{
	this->channel = channel;
	return 0;
}

int FileREF::read_samples(double *buffer, int64_t len)
{
	int result = len > 0 ? 0 : 1;
	if( !render_engine || !render_engine->arender ) result = 1;
	if( !result ) {
		if( samples_length != len ) {
			samples_length = -1;
			for( int i=0; i<MAX_CHANNELS; ++i ) {
				delete samples[i];  samples[i] = 0;
			}
		}
		if( samples_length < 0 ) {
			samples_length = len;
			int ch = 0, channels = asset->channels;
			while( ch < channels ) samples[ch++] = new Samples(samples_length);
			samples_position = -1;
		}
		if( samples_position != audio_position ) {
			result = render_engine->arender->process_buffer(samples, len, audio_position);
			samples_position = audio_position;
		}
	}
	Samples *cbfr = samples[channel];
	double *data = cbfr ? cbfr->get_data() : 0;
	if( !data ) result = 1;
	int64_t sz = len*(sizeof(*buffer));
	if( !result )
		memcpy(buffer, data, sz);
	else
		memset(buffer, 0, sz);
	return result;
}

int FileREF::read_frame(VFrame *frame)
{
        int result = render_engine && render_engine->vrender ? 0 : 1;
	EDLSession *render_session = render_engine->get_edl()->session;
	int color_model = render_session->color_model;
	int out_w = render_session->output_w, out_h = render_session->output_h;
	VFrame *vframe = frame;
	if( color_model != frame->get_color_model() ||
	    out_w != frame->get_w() || out_h != frame->get_h() ) {
		VFrame::get_temp(temp, out_w, out_h, color_model);
		vframe = temp;
	}
	if( !result )
		result = render_engine->vrender->process_buffer(vframe, video_position++, 0);
	if( vframe != frame )
		frame->transfer_from(vframe);
	return result;
}

int FileREF::colormodel_supported(int colormodel)
{
	return colormodel;
}


int FileREF::get_best_colormodel(Asset *asset, int driver)
{
	return BC_RGBA_FLOAT;
}

