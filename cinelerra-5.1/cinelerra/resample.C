
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "clip.h"
#include "resample.h"
#include "samples.h"
#include "transportque.inc"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Resampling from Lame

Resample::Resample()
{
	old.allocate(BLACKSIZE, 0);
	double *old_data = old.get_data();
	memset(old_data, 0, BLACKSIZE*sizeof(*old_data));
	resample_init = 0;
	last_ratio = 0;
	output_temp = 0;
	output_size = 0;
	output_allocation = 0;
	input_size = RESAMPLE_CHUNKSIZE;
	input_position = 0;
	input = new Samples(input_size + 1);
	output_position = 0;
	itime = 0;
	direction = PLAY_FORWARD;
}


Resample::~Resample()
{
	delete [] output_temp;
	delete input;
}

int Resample::read_samples(Samples *buffer, int64_t start, int64_t len, int direction)
{
	return 0;
}

void Resample::reset()
{
	resample_init = 0;
	output_size = 0;
	output_position = 0;
	input_position = 0;
}

/* This algorithm from:
 * SIGNAL PROCESSING ALGORITHMS IN FORTRAN AND C
 * S.D. Stearns and R.A. David, Prentice-Hall, 1992 */
void Resample::blackman(double fcn, int filter_l)
{
	double wcn = M_PI * fcn;
	double ctr = filter_l / 2.0;
	double cir = 2*M_PI/filter_l;
	for( int j=0; j<=2*BPC; ++j ) {
		double offset = (j-BPC) / (2.*BPC);  // -0.5 ... 0.5
		for( int i=0; i<=filter_l; ++i ) {
			double x = i - offset;
			bclamp(x, 0,filter_l);
			double v, dx = x - ctr;
			if( fabs(dx) >= 1e-9 ) {
				double curve = sin(wcn * dx) / (M_PI * dx);
				double th = x * cir;
				double blkmn = 0.42 - 0.5 * cos(th) + 0.08 * cos(2*th);
				v = blkmn * curve;
			}
			else
				v = fcn;
			blackfilt[j][i] = v;
		}
	}
}


int Resample::get_output_size()
{
	return output_size;
}

// void Resample::read_output(double *output, int size)
// {
// 	memcpy(output, output_temp, size * sizeof(double));
// // Shift leftover forward
// 	for( int i = size; i < output_size; i++ )
// 		output_temp[i - size] = output_temp[i];
// 	output_size -= size;
// }


// starts odd = (even-1)
#define FILTER_N (BLACKSIZE-6)
#define FILTER_L (FILTER_N - (~FILTER_N & 1));

void Resample::resample_chunk(Samples *input_buffer, int64_t in_len,
	int in_rate, int out_rate)
{
//printf("Resample::resample_chunk %d in_len=%jd input_size=%d\n",
// __LINE__, in_len, input_size);
	double *input = input_buffer->get_data();
	double resample_ratio = (double)in_rate / out_rate;
	double fcn = .90 / resample_ratio;
	if( fcn > .90 ) fcn = .90;
	int filter_l = FILTER_L;
// if resample_ratio = int, filter_l should include right edge
	if( fabs(resample_ratio - floor(.5 + resample_ratio)) < .0001 )
		++filter_l;
// Blackman filter initialization must be called whenever there is a
// sampling ratio change
	if( !resample_init || last_ratio != resample_ratio ) {
		resample_init = 1;
		last_ratio = resample_ratio;
		blackman(fcn, filter_l);
		itime = 0;
	}

	double filter_l2 = filter_l/2.;
	int l2 = filter_l2;
	int64_t end_time = itime + in_len + l2;
	int64_t out_time = end_time / resample_ratio + 1;
	int64_t demand = out_time - output_position;
	if( demand >= output_allocation ) {
// demand 2**n buffer
		int64_t new_allocation = output_allocation ? output_allocation : 16384;
		while( new_allocation < demand ) new_allocation <<= 1;
		double *new_output = new double[new_allocation];
		if( output_temp ) {
			memmove(new_output, output_temp, output_allocation*sizeof(double));
			delete [] output_temp;
		}
		output_temp = new_output;
		output_allocation = new_allocation;
	}

// Main loop
	double *old_data = old.get_data();
	double ctr_pos = 0;
	int otime = 0, last_used = 0;
	while( output_size < output_allocation ) {
		double in_pos = otime * resample_ratio;
// window centered at ctr_pos
		ctr_pos = in_pos + itime;
		double pos = ctr_pos - filter_l2;
		int ipos = floor(pos);
		last_used = ipos + filter_l;
		if( last_used >= in_len ) break;
		double fraction = pos - ipos;
		int phase = floor(fraction * 2*BPC + .5);
		int i = ipos, j = filter_l;  // fir filter
		double xvalue = 0, *filt = blackfilt[phase];
		for( ; j>=0 && i<0; ++i,--j ) xvalue += *filt++ * old_data[BLACKSIZE + i];
		for( ; j>=0;        ++i,--j ) xvalue += *filt++ * input[i];
		output_temp[output_size++] = xvalue;
		++otime;
	}
// move ctr_pos backward by in_len as new itime offset
// the next read will be in the history, itime is negative
	itime = ctr_pos - in_len;
	memmove(old_data, input+in_len-BLACKSIZE, BLACKSIZE*sizeof(double));
}

void Resample::reverse_buffer(double *buffer, int64_t len)
{
	double *ap = buffer;
	double *bp = ap + len;
	while( ap < --bp ) {
		double t = *ap;
		*ap++ = *bp;
		*bp = t;
	}
}

int Resample::set_input_position(int64_t in_pos, int in_dir)
{
	reset();
	input_position = in_pos;
	direction = in_dir;
// update old, just before/after input going fwd/rev;
	int dir = direction == PLAY_FORWARD ? -1 : 1;
	in_pos += dir * BLACKSIZE;
	return read_samples(&old, in_pos, BLACKSIZE, in_dir);
}

int Resample::resample(Samples *output, int64_t out_len,
		int in_rate, int out_rate, int64_t out_position, int direction)
{
	int result = 0;
	if( this->output_position != out_position ||
	    this->direction != direction ) {
//printf("missed  %jd!=%jd\n", output_position, out_position);
// starting point in input rate.
		int64_t in_pos = out_position * in_rate / out_rate;
		set_input_position(in_pos, direction);
	}
//else
//printf("matched %jd==%jd\n", output_position, out_position);

	int dir = direction == PLAY_REVERSE ? -1 : 1;
	int remaining_len = out_len;
	double *output_ptr = output->get_data();
	while( remaining_len > 0 && !result ) {
		if( output_size ) {
			int len = bmin(output_size, remaining_len);
			memmove(output_ptr, output_temp, len*sizeof(double));
			memmove(output_temp, output_temp+len, (output_size-=len)*sizeof(double));
			output_ptr += len;  remaining_len -= len;
		}
		if( remaining_len > 0 ) {
			result = read_samples(input, input_position, input_size, direction);
			if( result ) break;
			resample_chunk(input, input_size, in_rate, out_rate);
			input_position += dir * input_size;
		}
	}
	if( !result )
		this->output_position = out_position + dir * out_len;
	return result;
}



