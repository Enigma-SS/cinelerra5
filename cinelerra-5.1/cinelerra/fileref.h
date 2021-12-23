#ifndef __FILEREF_H__
#define __FILEREF_H__
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

#include "asset.inc"
#include "cache.inc"
#include "edl.inc"
#include "filebase.h"
#include "file.inc"
#include "maxchannels.h"
#include "renderengine.inc"
#include "samples.inc"
#include "transportque.inc"
#include "vframe.inc"


class FileREF : public FileBase
{
public:
	FileREF(Asset *asset, File *file);
	~FileREF();

	int open_file(int rd, int wr);
	int64_t get_video_position();
	int64_t get_audio_position();
	int set_video_position(int64_t pos);
	int set_layer(int layer);
	int set_audio_position(int64_t pos);
	int set_channel(int channel);
	int read_samples(double *buffer, int64_t len);
	int read_frame(VFrame *frame);
	int colormodel_supported(int colormodel);
	static int get_best_colormodel(Asset *asset, int driver);
	int close_file();

	EDL *ref;
	TransportCommand *command;
	Samples *samples[MAX_CHANNELS];
	int64_t samples_position, samples_length;
	int64_t audio_position;
	int64_t video_position;
	int channel, layer;
	RenderEngine *render_engine;
	CICache *acache, *vcache;
	VFrame *temp;
	int is_open;
};

#endif
