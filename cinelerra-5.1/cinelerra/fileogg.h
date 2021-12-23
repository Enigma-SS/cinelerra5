
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

#ifndef FILEOGG_H
#define FILEOGG_H
#ifdef HAVE_OGG

#include "edl.inc"
#include "filebase.h"
#include "file.inc"
#include "mutex.inc"

#include <theora/theora.h>
#include <theora/theoraenc.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>


/* This code was aspired by ffmpeg2theora */
/* Special thanks for help on this code goes out to j@v2v.cc */

#define mn_pagesz 32

class sync_window_t : public ogg_sync_state
{
public:
	sync_window_t(FILE *fp, Mutex *sync_lock, int64_t begin=0, int64_t end=0);
	~sync_window_t();
	int ogg_read_locked(int buflen);
	int ogg_read_buffer(int buflen);
	int ogg_read_buffer_at(off_t filepos, int buflen);
	int ogg_sync_and_take_page_out(ogg_page *og);
	int ogg_take_page_out_autoadvance(ogg_page *og);
	int ogg_sync_and_get_next_page(long serialno, ogg_page *og);
	int ogg_prev_page_search(long serialno,ogg_page *og,
			off_t begin, off_t end);
	int ogg_get_prev_page(long serialno, ogg_page *og);
	int ogg_get_next_page(long serialno, ogg_page *og);
	int ogg_get_first_page(long serialno, ogg_page *og);
	int ogg_get_last_page(long serialno, ogg_page *og);

	int64_t filepos; // current file position
	int64_t bufpos;  // position at start of read
	int64_t pagpos;  // last search target position
	FILE *fp;
	Mutex *sync_lock;
	int64_t file_begin, file_end;
};

class OGG_PageBfr
{
public:
	int allocated, len;
	int valid, packets;
	int64_t position;
	uint8_t *page;

	OGG_PageBfr();
	~OGG_PageBfr();
	void demand(int sz);
	int write_page(FILE *fp);
	int64_t load(ogg_page *og);
};

class FileOGG : public FileBase
{
public:
	FileOGG(Asset *asset, File *file);
	~FileOGG();

	static void get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase *&format_window,
		int audio_options,int video_options,EDL *edl);
	static int check_sig(Asset *asset);
	int open_file(int rd,int wr);
	int close_file();

	int64_t get_video_position();
	int64_t get_audio_position();
	int set_video_position(int64_t pos);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset,int driver);
	int read_frame(VFrame *frame);
	int set_audio_position(int64_t pos);
	int read_samples(double *buffer,int64_t len);
	int write_samples(double **buffer,int64_t len);
	int write_frames(VFrame ***frames,int len);

private:
	void init();
	int encode_theora_init();
	int encode_vorbis_init();
	int ogg_init_encode(FILE *out);
	void close_encoder();
	int decode_theora_init();
	int decode_vorbis_init();
	int ogg_init_decode(FILE *inp);
	void close_decoder();

	int write_samples_vorbis(double **buffer, int64_t len, int e_o_s);
	int write_frames_theora(VFrame ***frames, int len, int e_o_s);
	void flush_ogg(int e_o_s);
	int write_audio_page();
	int write_video_page();

	FILE *inp, *out;
	off_t file_length;
	int audio, video;
	int64_t frame_position;
	int64_t sample_position;

	VFrame *temp_frame;
	Mutex *file_lock;
	float **pcm_history;
#ifndef HISTORY_MAX
#define HISTORY_MAX 0x100000
#endif
	int64_t history_start;
	int64_t history_size;
	int pcm_channels;
	int ach, ahz;		// audio channels, sample rate
	int amn, amx;		// audio min/max bitrate
	int abr, avbr, aqu;	// audio bitrate, variable bitrate, quality
	int asz, afrmsz;	// audio sample size, frame size

	off_t file_begin, file_end;
	sync_window_t *audiosync;
	sync_window_t *videosync;

	int64_t ogg_sample_pos(ogg_page *og);
	int64_t ogg_next_sample_pos(ogg_page *og);
	int64_t ogg_frame_pos(ogg_page *og);
	int64_t ogg_next_frame_pos(ogg_page *og);
	int ogg_read_buffer(FILE *in, sync_window_t *sy, int buflen);
	int ogg_get_video_packet(ogg_packet *op);
	int ogg_get_audio_packet(ogg_packet *op);

	int ogg_get_page_of_sample(ogg_page *og, int64_t sample);
	int ogg_seek_to_sample(int64_t sample);
	int ogg_decode_more_samples();

	int ogg_get_page_of_frame(ogg_page *og, int64_t frame);
	int ogg_seek_to_keyframe(int64_t frame, int64_t *keyframe_number);
	int move_history(int from, int to, int len);

	ogg_stream_state to;	// theora to ogg out
	ogg_stream_state vo;	// vorbis to ogg out
	int64_t ogg_sample_position, ogg_frame_position;
	int64_t next_sample_position, next_frame_position;
	int64_t start_sample, last_sample; // first and last sample inside this file
	int64_t start_frame, last_frame; // first and last frame inside this file
	int64_t audio_pos, video_pos;	// decoder last sample/frame in
	int audio_eos, video_eos;	// decoder sample/frame end of file

	th_enc_ctx *enc;	// theora video encode context
	th_dec_ctx *dec;	// theora video decode context
	th_info ti;		// theora video encoder init parameters
	th_setup_info *ts;	// theora video setup huff/quant codes
	th_comment tc;		// header init parameters
	vorbis_info vi;		// vorbis audio encoder init parameters
	vorbis_comment vc;	// header init parameters
	vorbis_dsp_state vd;	// vorbis decode audio context
	vorbis_block vb;	// vorbis decode buffering
	int force_keyframes;
	int vp3_compatible;
	int soft_target;

	int pic_x, pic_y, pic_w, pic_h;
	int frame_w, frame_h;
	int colorspace, pixfmt;
	int bitrate, quality;
	int keyframe_period, keyframe_force;
	int fps_num, fps_den;
	int aratio_num, aratio_den;

	OGG_PageBfr apage, vpage;
	double audiotime, videotime;
	int keyframe_granule_shift;
	int iframe_granule_offset;
	int theora_cmodel;
};

class OGGConfigAudio;
class OGGConfigVideo;

class OGGVorbisFixedBitrate : public BC_Radial
{
public:
	OGGVorbisFixedBitrate(int x, int y, OGGConfigAudio *gui);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisVariableBitrate : public BC_Radial
{
public:
	OGGVorbisVariableBitrate(int x, int y, OGGConfigAudio *gui);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisMinBitrate : public BC_TextBox
{
public:
	OGGVorbisMinBitrate(int x,
		int y,
		OGGConfigAudio *gui,
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisMaxBitrate : public BC_TextBox
{
public:
	OGGVorbisMaxBitrate(int x,
		int y,
		OGGConfigAudio *gui,
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};

class OGGVorbisAvgBitrate : public BC_TextBox
{
public:
	OGGVorbisAvgBitrate(int x,
		int y,
		OGGConfigAudio *gui,
		char *text);
	int handle_event();
	OGGConfigAudio *gui;
};


class OGGConfigAudio: public BC_Window
{
public:
	OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~OGGConfigAudio();

	void create_objects();
	int close_event();

	Asset *asset;
	OGGVorbisFixedBitrate *fixed_bitrate;
	OGGVorbisVariableBitrate *variable_bitrate;
private:
	BC_WindowBase *parent_window;
	char string[BCTEXTLEN];
};


class OGGTheoraBitrate : public BC_TextBox
{
public:
	OGGTheoraBitrate(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraKeyframeFrequency : public BC_TumbleTextBox
{
public:
	OGGTheoraKeyframeFrequency(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraKeyframeForceFrequency : public BC_TumbleTextBox
{
public:
	OGGTheoraKeyframeForceFrequency(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraSharpness : public BC_TumbleTextBox
{
public:
	OGGTheoraSharpness(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraFixedBitrate : public BC_Radial
{
public:
	OGGTheoraFixedBitrate(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};

class OGGTheoraFixedQuality : public BC_Radial
{
public:
	OGGTheoraFixedQuality(int x, int y, OGGConfigVideo *gui);
	int handle_event();
	OGGConfigVideo *gui;
};



class OGGConfigVideo: public BC_Window
{
public:
	OGGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~OGGConfigVideo();

	void create_objects();
	int close_event();

	OGGTheoraFixedBitrate *fixed_bitrate;
	OGGTheoraFixedQuality *fixed_quality;
	Asset *asset;
private:
	BC_WindowBase *parent_window;
};

#endif
#endif
