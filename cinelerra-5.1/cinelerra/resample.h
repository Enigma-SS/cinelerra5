
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


// Generic resampling module

#ifndef RESAMPLE_H
#define RESAMPLE_H

#define BPC 160
#define BLACKSIZE 25

#include "resample.inc"
#include "samples.h"
#include <stdint.h>

class Resample
{
public:
	Resample();
	virtual ~Resample();

// All positions are in file sample rate
// This must reverse the buffer during reverse playback
// so the filter always renders forward.
	virtual int read_samples(Samples *buffer,
		int64_t start, int64_t len, int direction);

// Resample from the file handler and store in *output.
// Returns 1 if the input reader failed.
// Starting sample in output samplerate
// If reverse, the end of the buffer.
	int resample(Samples *samples,
		int64_t out_len, int in_rate, int out_rate,
		int64_t out_position, int direction);
	static void reverse_buffer(double *buffer, int64_t len);

// Reset after seeking
	void reset();

private:
	void blackman(double fcn, int l);
	int set_input_position(int64_t in_pos, int in_dir);
// Query output temp
	int get_output_size();
//	void read_output(Samples *output, int size);
// Resamples input and dumps it to output_temp
	void resample_chunk(Samples *input, int64_t in_len,
			int in_rate, int out_rate);

	int direction;
// Unaligned resampled output
	double *output_temp;
// Total samples in unaligned output
	int64_t output_size;
// Allocation of unaligned output
	int64_t output_allocation;
// History buffer for resampling.
	Samples old;
// input chunk
	Samples *input;
	double itime;
// Position of source in source sample rate.
	int64_t input_position;
	int64_t input_size;
	int64_t output_position;
	int resample_init;
// Last sample ratio configured to
	double last_ratio;
	double blackfilt[2 * BPC + 1][BLACKSIZE];
};

#endif
