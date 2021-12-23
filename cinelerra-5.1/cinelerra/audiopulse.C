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

#ifdef HAVE_PULSE
#include "audiopulse.h"
#include "adeviceprefs.h"
#include "bctimer.h"
#include "cstrdup.h"
#include "language.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "recordconfig.h"

#include <pulse/pulseaudio.h>
#include <pulse/simple.h>

AudioPulse::AudioPulse(AudioDevice *device)
 : AudioLowLevel(device)
{
	buffer_position = 0;
	timer_position = 0;
	timer = new Timer;
	timer_lock = new Mutex("AudioPulse::timer_lock");
	dsp_in = 0;
	dsp_out = 0;
	period_usecs = 1000000;
	frag_usecs = period_usecs / 32;
	wr_spec = 0;
	rd_spec = 0;
}

AudioPulse::~AudioPulse()
{
	delete (pa_sample_spec *)wr_spec;
	delete (pa_sample_spec *)rd_spec;
	delete timer_lock;
	delete timer;
	close_all();
}

int AudioPulse::open_input()
{
	pa_sample_spec *ss = new pa_sample_spec();
	rd_spec = (void *)ss;

	ss->format = PA_SAMPLE_S16LE;
	ss->rate = device->in_samplerate; 
	ss->channels = device->get_ichannels();   
	device->in_bits = 16;
	return init_input();
}

int AudioPulse::init_input()
{
	pa_sample_spec *ss = (pa_sample_spec *)rd_spec;
	int error = 0;
	char *server = 0;
	if( device->in_config->pulse_in_server[0] )
		server = device->in_config->pulse_in_server;
	dsp_in = pa_simple_new(server, PROGRAM_NAME, PA_STREAM_RECORD, 
		0, "recording", ss, 0, 0, &error);
	if( !dsp_in ) {
		printf("AudioPulse::open_input %d: failed server=%s %s\n", __LINE__, 
			server, pa_strerror(error));
		return 1;
	}

	return 0;
}


int AudioPulse::open_output()
{
	pa_sample_spec *ss = new pa_sample_spec();
	wr_spec = (void *)ss;
	ss->format = PA_SAMPLE_S16LE;
	ss->rate = device->out_samplerate;
	ss->channels = device->get_ochannels();
	device->out_bits = 16;
	return init_output();
}

int AudioPulse::init_output()
{
	pa_sample_spec *ss = (pa_sample_spec *)wr_spec;
	int error = 0;
	char *server = device->out_config->pulse_out_server[0] ?
		device->out_config->pulse_out_server : 0;
	dsp_out = pa_simple_new(server, PROGRAM_NAME, PA_STREAM_PLAYBACK, 
		NULL, "playback", ss, 0, 0, &error);
	if( !dsp_out ) {
		printf("AudioPulse::open_output %d: failed server=%s %s\n", 
			__LINE__, server, pa_strerror(error));
		return 1;
	}
	timer->update();
	device->device_buffer = 0;
	buffer_position = 0;
	return 0;
}


int AudioPulse::close_all()
{
	if( dsp_out ) {
		int error = 0;
		pa_simple_flush((pa_simple*)dsp_out, &error);
		pa_simple_free((pa_simple*)dsp_out);
		dsp_out = 0;
	}

	if( dsp_in ) {
		pa_simple_free((pa_simple*)dsp_in);
		dsp_in = 0;
	}
	delete (pa_sample_spec *)wr_spec;  wr_spec = 0;
	delete (pa_sample_spec *)rd_spec;  rd_spec = 0;
	buffer_position = 0;
	timer_position = 0;
	return 0;
}

int64_t AudioPulse::device_position()
{
	timer_lock->lock("AudioPulse::device_position");
	int64_t samples = timer->get_scaled_difference(device->out_samplerate);
	int64_t result = timer_position + samples;
	timer_lock->unlock();
	return result;
}

int AudioPulse::write_buffer(char *buffer, int size)
{
	if( !dsp_out && init_output() )
		return 1;
	int error = 0;
	timer_lock->lock("AudioPulse::write_buffer");
	int64_t usecs = pa_simple_get_latency((pa_simple*)dsp_out, &error);
	int64_t delay = device->out_samplerate * usecs / 1000000;
	timer_position = buffer_position - delay;
	timer->update();
	timer_lock->unlock();

	AudioThread *audio_out = device->audio_out;
	int sample_size = (device->out_bits / 8) * device->get_ochannels();
	int samples = device->out_samplerate * frag_usecs / 1000000;
	int64_t frag_bytes = samples * sample_size;

	buffer_position += size / sample_size;
	int ret = 0;
        while( !ret && size > 0 && !device->playback_interrupted ) {
		audio_out->Thread::enable_cancel();
		usecs = pa_simple_get_latency((pa_simple*)dsp_out, &error);
		if( usecs < period_usecs ) {
			int64_t len = size;
			if( len > frag_bytes ) len = frag_bytes;
			if( pa_simple_write((pa_simple*)dsp_out, buffer, len, &error) < 0 )
				ret = 1;
			buffer += len;  size -= len;
		}
		else
			usleep(frag_usecs);
		audio_out->Thread::disable_cancel();
	}
	if( ret )
		printf("AudioPulse::write_buffer %d: %s\n", 
			__LINE__, pa_strerror(error));
	return ret;
}

int AudioPulse::read_buffer(char *buffer, int size)
{
	if( !dsp_in && init_input() )
		return 1;

	int error = 0;
	int result = pa_simple_read((pa_simple*)dsp_in, buffer, size, &error);
	if( result < 0 ) {
		printf("AudioPulse::read_buffer %d: %s\n", 
			__LINE__, pa_strerror(error));
		return 1;
	}

//printf("AudioPulse::read_buffer %d %d\n", __LINE__, size);

	return 0;
}

int AudioPulse::output_wait()
{
	int error = 0;
	pa_usec_t latency = pa_simple_get_latency((pa_simple*)dsp_out, &error);
	int64_t udelay = latency;
        while( udelay > 0 && !device->playback_interrupted ) {
                int64_t usecs = udelay;
                if( usecs > 100000 ) usecs = 100000;
                usleep(usecs);
                udelay -= usecs;
        }
        if( device->playback_interrupted &&
            !device->out_config->interrupt_workaround )
		pa_simple_flush((pa_simple*)dsp_out, &error);
        return 0;
}

int AudioPulse::flush_device()
{
	if( dsp_out ) {
		output_wait();
		int error = 0;
		pa_simple_drain((pa_simple*)dsp_out, &error);
	}
	return 0;
}

int AudioPulse::interrupt_playback()
{
	if( !dsp_out ) {
		return 1;
	}

	int error = 0;
	int result = pa_simple_flush((pa_simple*)dsp_out, &error);
	if( result < 0 ) {
		printf("AudioPulse::interrupt_playback %d: %s\n", 
			__LINE__,
			pa_strerror(error));
		return 1;
	}

	return 0;
}

#endif
