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

#include "asset.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "clip.h"
#include "edit.h"
#include "file.h"
#include "fileogg.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "mwindow.inc"
#include "preferences.h"
#include "render.h"
#include "vframe.h"
#include "versioninfo.h"
#include "videodevice.inc"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* This code was aspired by ffmpeg2theora */
/* Special thanks for help on this code goes out to j@v2v.cc */


#define READ_SIZE 4*66000
#define SEEK_SIZE 2*66000

sync_window_t::sync_window_t(FILE *fp, Mutex *sync_lock, int64_t begin, int64_t end)
{
	ogg_sync_init(this);
	this->fp = fp;
	this->sync_lock = sync_lock;
	this->file_begin = begin;
	this->file_end = end;
	filepos = -1;
	bufpos = -1;
	pagpos = -1;
}

sync_window_t::~sync_window_t()
{
	ogg_sync_clear(this);
}

int sync_window_t::ogg_read_locked(int buflen)
{
	char *buffer = ogg_sync_buffer(this, buflen);
	int len = fread(buffer, 1, buflen, fp);
	ogg_sync_wrote(this, len);
	filepos += len;
	return len;
}

int sync_window_t::ogg_read_buffer(int buflen)
{
	sync_lock->lock("sync_window_t::ogg_read_buffer_at");
	fseeko(fp, filepos, SEEK_SET);
	int len = ogg_read_locked(buflen);
	sync_lock->unlock();
	return len;
}

int sync_window_t::ogg_read_buffer_at(off_t filepos, int buflen)
{
	if( bufpos == filepos && buflen == this->filepos - bufpos )
		return buflen;
	sync_lock->lock("sync_window_t::ogg_read_buffer_at");
	this->bufpos = filepos;
	fseeko(fp, filepos, SEEK_SET);
	this->filepos = filepos;
	ogg_sync_reset(this);
	int ret = ogg_read_locked(buflen);
	sync_lock->unlock();
	return ret;
}

// we never need to autoadvance when syncing, since our read chunks are larger than
// maximum page size
int sync_window_t::ogg_sync_and_take_page_out(ogg_page *og)
{
	og->header_len = 0;
	og->body_len = 0;
	og->header = 0;
	og->body = 0;
	int ret = ogg_sync_pageseek(this, og);
	bufpos += abs(ret); // can be zero
	return ret;
}

int sync_window_t::ogg_sync_and_get_next_page(long serialno, ogg_page *og)
{
	int ret = 0, retries = 1000;
	while( --retries >= 0 && (ret = ogg_sync_and_take_page_out(og)) < 0 );
	if( ret >= mn_pagesz && ogg_page_serialno(og) != serialno )
		ret = ogg_get_next_page(serialno, og);
	if( ret ) {
		pagpos = bufpos - (og->header_len + og->body_len);
		return 1;
	}
	return 0;
}

int sync_window_t::ogg_get_next_page(long serialno, ogg_page *og)
{
	int ret = 0, retries = 1000;
	while( --retries >= 0 && (ret=ogg_take_page_out_autoadvance(og)) &&
		ogg_page_serialno(og) != serialno );
	if( ret ) {
		pagpos = bufpos - (og->header_len + og->body_len);
}
	else
		printf("ogg_get_next_page missed\n");
	return ret;
}

int sync_window_t::ogg_prev_page_search(long serialno, ogg_page *og,
		off_t begin, off_t end)
{
	ogg_page page;
	int retries = 100, ret = 0;
	int64_t ppos = -1;
	while( ppos < 0 && --retries >= 0 ) {
		int64_t fpos = end;
		int read_len = SEEK_SIZE;
		fpos -= read_len;
		if( fpos < begin ) {
			read_len += fpos - begin;
			if( read_len <= 0 ) break;
			fpos = begin;
		}
		read_len = ogg_read_buffer_at(fpos, read_len);
		if( read_len <= 0 ) return 0;
		while( (ret=ogg_sync_and_take_page_out(&page)) < 0 );
		end = bufpos;
		while( ret > 0 ) {
			if( ogg_page_serialno(&page) == serialno ) {
				memcpy(og, &page, sizeof(page));
				ppos = bufpos - (page.header_len + page.body_len);
			}
			ret = ogg_sync_pageout(this, &page);
			bufpos += page.header_len + page.body_len;
		}
	}
	if( ppos >= 0 ) {
		pagpos = ppos;
		return 1;
	}
	printf("ogg_prev_page_search missed\n");
	return 0;
}

int sync_window_t::ogg_get_prev_page(long serialno, ogg_page *og)
{
	return ogg_prev_page_search(serialno, og, file_begin, pagpos);
}

int sync_window_t::ogg_get_first_page(long serialno, ogg_page *og)
{
	ogg_read_buffer_at(file_begin, SEEK_SIZE);
	return ogg_sync_and_get_next_page(serialno, og);
}

int sync_window_t::ogg_get_last_page(long serialno, ogg_page *og)
{

	ogg_page page;
	off_t filepos = file_end - READ_SIZE;
	if( filepos < 0 ) filepos = 0;
	int ret = 0, first_page_offset = 0;
	while( !ret && filepos >= 0 ) {
		int readlen = ogg_read_buffer_at(filepos, READ_SIZE);
		int page_offset = 0, page_length = 0;
		int first_page = 1; // read all pages in the buffer
		while( first_page || page_length ) {
			// if negative, skip bytes
			while( (page_length = ogg_sync_and_take_page_out(&page)) < 0 )
				page_offset -= page_length;
			if( page_length < mn_pagesz ) continue;
			if( first_page ) {
				first_page = 0;
				first_page_offset = page_offset;
			}
			if( ogg_page_serialno(&page) == serialno ) {
				// return last match page
				pagpos = bufpos - (page.header_len + page.body_len);
				memcpy(og, &page, sizeof(page));
				ret = 1;
			}
		}
		filepos -= readlen - first_page_offset;  // move backward
	}
	return ret;
}

OGG_PageBfr::OGG_PageBfr()
{
	allocated = len = 0;
	valid = packets = 0;
	position = 0;
	page = 0;
}

OGG_PageBfr::~OGG_PageBfr()
{
	delete [] page;
}

void OGG_PageBfr::demand(int sz)
{
	if( allocated >= sz ) return;
	uint8_t *new_page = new uint8_t[sz];
	memcpy(new_page, page, len);
	delete [] page;  page = new_page;
	allocated = sz;
}

int OGG_PageBfr::write_page(FILE *fp)
{
	int sz = fwrite(page, 1, len, fp);
	if( sz != len ) return -1;
	ogg_page op;  // kludgy
	op.header = page;    op.header_len = len;
	op.body = page+len;  op.body_len = 0;
	packets -= ogg_page_packets(&op);
	valid = len = 0;
	return packets;
}

int64_t OGG_PageBfr::load(ogg_page *og)
{
	int sz = og->header_len + og->body_len;
	demand(sz);
	memcpy(page, og->header, og->header_len);
	memcpy(page+og->header_len, og->body, og->body_len);
	len = sz;  valid = 1;
	position = ogg_page_granulepos(og);
	return position;
}



FileOGG::FileOGG(Asset *asset, File *file)
 : FileBase(asset, file)
{
	if( asset->format == FILE_UNKNOWN )
		asset->format = FILE_OGG;
	asset->byte_order = 0;
	init();
	file_lock = new Mutex("OGGFile::Flush lock");
}

FileOGG::~FileOGG()
{
	close_file();
	delete file_lock;
}


void FileOGG::init()
{
	inp = 0;
	out = 0;
	audio = 0;
	video = 0;
	file_length = 0;
	temp_frame = 0;
	file_lock = 0;
	ach = 0;
	ahz = 0;
	asz = 0;
	amn = 0;
	amx = 0;
	abr = 0;
	avbr = 0;
	aqu = 0;
	afrmsz = 0;
	pcm_history = 0;
	pcm_channels = 0;
	frame_position = 0;
	sample_position = 0;
	audiosync = 0;
	videosync = 0;
	file_begin = 0;
	file_end = 0;

	memset(&to, 0, sizeof(to));
	memset(&vo, 0, sizeof(vo));
	ogg_sample_position = 0;
	ogg_frame_position = 0;
	next_sample_position = 0;
	next_frame_position = 0;
	start_sample = 0;
	last_sample = 0;
	start_frame = 0;
	last_frame = 0;
	audiotime = 0;
	videotime = 0;
	audio_pos = 0;  audio_eos = 0;
	video_pos = 0;  video_eos = 0;

	keyframe_granule_shift = 0;
	iframe_granule_offset = 0;
	theora_cmodel = BC_YUV420P;
	enc = 0;
	dec = 0;
	memset(&ti, 0, sizeof(ti));
	ts = 0;
	memset(&tc, 0, sizeof(tc));
	memset(&vi, 0, sizeof(vi));
	memset(&vc, 0, sizeof(vc));
	memset(&vd, 0, sizeof(vd));
	memset(&vb, 0, sizeof(vb));
	force_keyframes = 0;
	vp3_compatible = 0;
	soft_target = 0;

	pic_x = pic_y = 0;
	pic_w = pic_h = 0;
	frame_w = frame_h = 0;
	colorspace = OC_CS_UNSPECIFIED;
	pixfmt = TH_PF_420;
	bitrate = 0;  quality = 0;
	keyframe_period = 0;
	keyframe_force = 0;
	fps_num = fps_den = 0;
	aratio_num = aratio_den = 0;
}


static int ilog(unsigned v)
{
	int ret = 0;
	while( v ) { ++ret;  v >>= 1; }
	return ret;
}

int FileOGG::encode_theora_init()
{
	ogg_stream_init(&to, rand());
	th_info_init(&ti);
	pic_w = asset->width, pic_h = asset->height;
	frame_w = (pic_w+0x0f) & ~0x0f;
	frame_h = (pic_h+0x0f) & ~0x0f;
	pic_x = ((frame_w-pic_w) >> 1) & ~1;
	pic_y = ((frame_h-pic_h) >> 1) & ~1;
	fps_num = asset->frame_rate * 1000000;
	fps_den = 1000000;
	if( asset->aspect_ratio > 0 ) {
		// Cinelerra uses frame aspect ratio, theora uses pixel aspect ratio
		float pixel_aspect = asset->aspect_ratio / asset->width * asset->height;
		aratio_num = pixel_aspect * 1000000;
		aratio_den = 1000000;
	}
	else {
		aratio_num = 1000000;
		aratio_den = 1000000;
	}
	if( EQUIV(asset->frame_rate, 25) || EQUIV(asset->frame_rate, 50) )
		colorspace = OC_CS_ITU_REC_470BG;
	else if( (asset->frame_rate > 29 && asset->frame_rate < 31) ||
		 (asset->frame_rate > 59 && asset->frame_rate < 61) )
		colorspace = OC_CS_ITU_REC_470M;
	else
		colorspace = OC_CS_UNSPECIFIED;
	pixfmt = TH_PF_420;
	if( asset->theora_fix_bitrate ) {
		bitrate = asset->theora_bitrate;
		quality = -1;
	}
	else {
		bitrate = -1;
		quality = asset->theora_quality;     // 0-63
	}
	keyframe_period = asset->theora_keyframe_frequency;
	keyframe_force = asset->theora_keyframe_force_frequency;
	vp3_compatible = 1;
	soft_target = 0;

	ti.frame_width = frame_w;
	ti.frame_height = frame_h;
	ti.pic_width = pic_w;
	ti.pic_height = pic_h;
	ti.pic_x = pic_x;
	ti.pic_y = pic_x;
	ti.colorspace = (th_colorspace)colorspace;
	ti.pixel_fmt = (th_pixel_fmt)pixfmt;
	ti.target_bitrate = bitrate;
	ti.quality = quality;
	ti.fps_numerator = fps_num;
	ti.fps_denominator = fps_den;
	ti.aspect_numerator = aratio_num;
	ti.aspect_denominator = aratio_den;
	ti.keyframe_granule_shift = ilog(keyframe_period-1);

	enc = th_encode_alloc(&ti);
	int ret =  enc ? 0 : 1;
	if( !ret && force_keyframes )
		ret = th_encode_ctl(enc,TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE,
			&keyframe_period, sizeof(keyframe_period));
	if( !ret && vp3_compatible )
		ret = th_encode_ctl(enc,TH_ENCCTL_SET_VP3_COMPATIBLE,
			&vp3_compatible, sizeof(vp3_compatible));
	if( !ret && soft_target ) {
		int arg = TH_RATECTL_CAP_UNDERFLOW;
		if( th_encode_ctl(enc, TH_ENCCTL_SET_RATE_FLAGS, &arg, sizeof(arg)) < 0 ) {
			eprintf(_("Could not set rate flags"));
			ret = 1;
		}
		int kr = keyframe_period*7>>1, fr = 5*fps_num/fps_den;
		arg = kr > fr ? kr : fr;
		if( th_encode_ctl(enc, TH_ENCCTL_SET_RATE_BUFFER, &arg, sizeof(arg)) ) {
			eprintf(_("Could not set rate buffer"));
			ret = 1;
		}
	}
	if( ret ) {
		eprintf(_("theora init context failed"));
		return 1;
	}

	th_comment_init(&tc);
	th_comment_add_tag(&tc, (char*)"ENCODER",
		(char*)PROGRAM_NAME " " CINELERRA_VERSION);
	ogg_page og;
	ogg_packet op;
	ret = th_encode_flushheader(enc, &tc, &op);
	if( ret <= 0 ) return 1;
	ogg_stream_packetin(&to, &op);
	ret = ogg_stream_pageout(&to, &og) != 1 ? 1 : 0;
	if( !ret ) {
		fwrite(og.header, 1, og.header_len, out);
		fwrite(og.body, 1, og.body_len, out);
	}
	if( ret ) {
		eprintf(_("write header out failed"));
		return 1;
	}
	while( (ret=th_encode_flushheader(enc, &tc, &op)) > 0 )
		ogg_stream_packetin(&to, &op);
	if( ret ) {
		eprintf(_("ogg_encoder_init video failed"));
		return 1;
	}
	return 0;
}

int FileOGG::encode_vorbis_init()
{
	ach = asset->channels;
	ahz = asset->sample_rate;
	amx = asset->vorbis_max_bitrate;
	amn = asset->vorbis_min_bitrate;
	abr = asset->vorbis_bitrate;
	avbr = asset->vorbis_vbr;
	asz = sizeof(short);
	afrmsz = asz * ach;
	aqu = -99;
	ogg_stream_init(&vo, rand());
	vorbis_info_init(&vi);
	int ret = 0;
	if( avbr ) {
		ret = vorbis_encode_setup_managed(&vi, ach, ahz, -1, abr, -1);
		if( !ret )
			ret = vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE_AVG, 0);
		if( !ret )
			ret = vorbis_encode_setup_init(&vi);
	}
	else
		ret = vorbis_encode_init(&vi, ach, ahz, amx, abr, amn);
	if( ret ) {
		eprintf(_("ogg_encoder_init audio init failed"));
		return 1;
	}
	vorbis_comment_init(&vc);
	vorbis_comment_add_tag(&vc, (char*)"ENCODER",
		(char*)PROGRAM_NAME " " CINELERRA_VERSION);
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);
	ogg_packet header;
	ogg_packet header_comm;
	ogg_packet header_code;
	vorbis_analysis_headerout(&vd, &vc,
		&header, &header_comm, &header_code);
	ogg_stream_packetin(&vo, &header);
	ogg_page og;
	ret = ogg_stream_pageout(&vo, &og)==1 ? 0 : -1;
	if( ret >= 0 ) {
		fwrite(og.header, 1, og.header_len, out);
		fwrite(og.body, 1, og.body_len, out);
		ogg_stream_packetin(&vo, &header_comm);
		ogg_stream_packetin(&vo, &header_code);
	}
	if( ret < 0 ) {
		eprintf(_("ogg_encoder_init audio failed"));
		return 1;
	}
	return 0;
}

int FileOGG::ogg_init_encode(FILE *out)
{
	this->out = out;
	srand(time(0));
	video = asset->video_data;
	if( video && encode_theora_init() )
		return 1;
	audio = asset->audio_data;
	if( audio && encode_vorbis_init() )
		return 1;
	ogg_page og;
	int ret = 0;
	if( !ret && video ) {
		while( (ret=ogg_stream_flush(&to, &og)) > 0 ) {
			fwrite(og.header, 1, og.header_len, out);
			fwrite(og.body, 1, og.body_len, out);
		}
	}
	if( !ret && audio ) {
		while( (ret=ogg_stream_flush(&vo, &og)) > 0 ) {
			fwrite(og.header, 1, og.header_len, out);
			fwrite(og.body, 1, og.body_len, out);
		}
	}
	if( ret < 0 ) {
		eprintf(_("render init failed"));
		return 1;
	}
	return 0;
}

int FileOGG::decode_theora_init()
{
	dec = th_decode_alloc(&ti, ts);
	if( !dec ) {
		eprintf(_("Error in probe data"));
		return 1;
	}
	keyframe_granule_shift = ti.keyframe_granule_shift;
	iframe_granule_offset = th_granule_frame(dec, 0);
	double fps = (double)ti.fps_numerator/ti.fps_denominator;

	videosync = new sync_window_t(inp, file_lock, file_begin, file_end);
	ogg_page og;
	int ret = videosync->ogg_get_first_page(to.serialno, &og);
	if( ret <= 0 ) {
		eprintf(_("cannot read video page from file"));
		return 1;
	}
	videosync->file_begin = videosync->pagpos;
	ret = videosync->ogg_get_first_page(to.serialno, &og);
	// video data starts here
	// get to the page of the finish of the first packet
	while( ret > 0 && !ogg_page_packets(&og) ) {
		if( ogg_page_granulepos(&og) != -1 ) {
			printf(_("FileOGG: Broken ogg file - broken page:"
				" ogg_page_packets == 0 and granulepos != -1\n"));
			return 1;
		}
		ret = videosync->ogg_get_next_page(to.serialno, &og);
	}
	// video frames start here
	start_frame = ogg_frame_pos(&og);
	ret = videosync->ogg_get_first_page(to.serialno, &og);
	if( ret <= 0 ) {
		printf(_("FileOGG: Cannot read data past header\n"));
		return 1;
	}
//printf("start frame = %jd, gpos %jd, begins %jd\n",
// start_frame, ogg_page_granulepos(&og), videosync->file_begin);

	ret = videosync->ogg_get_last_page(to.serialno, &og);
	while( ret > 0 && !ogg_page_packets(&og) )
		ret = videosync->ogg_get_prev_page(to.serialno, &og);
	if( ret > 0 ) {
		last_frame = ogg_next_frame_pos(&og);
		if( start_frame >= last_frame ) {
			eprintf(_("no video frames in file"));
			last_frame = start_frame = 0;
		}
		asset->video_length = last_frame - start_frame;
	}
	else {
		printf("FileOGG: Cannot find the video length\n");
		return 1;
	}
	asset->layers = 1;
	asset->width = ti.pic_width;
	asset->height = ti.pic_height;
// Don't want a user configured frame rate to get destroyed
	if( !asset->frame_rate )
		asset->frame_rate = fps;
// All theora material is noninterlaced by definition
	if( !asset->interlace_mode )
		asset->interlace_mode = ILACE_MODE_NOTINTERLACED;

	set_video_position(0); // make sure seeking is done to the first sample
	ogg_frame_position = -10;
	asset->video_data = 1;
	strncpy(asset->vcodec, "theo", 4);
//	report_colorspace(&ti);
//	dump_comments(&tc);
	return 0;
}

int FileOGG::decode_vorbis_init()
{
	ogg_stream_reset(&vo);
	vorbis_synthesis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);
	audiosync = new sync_window_t(inp, file_lock, file_begin, file_end);
	ogg_page og;
	int ret = audiosync->ogg_get_first_page(vo.serialno, &og);
	if( ret <= 0 ) {
		eprintf(_("cannot read audio page from file"));
		return 1;
	}
	// audio data starts here
	audiosync->file_begin = audiosync->pagpos;
	// audio samples starts here
	start_sample = ogg_sample_pos(&og);
//printf("start sample = %jd, gpos %jd, begins %jd\n",
// start_sample, ogg_page_granulepos(&og), audiosync->file_begin);
	ret = audiosync->ogg_get_last_page(vo.serialno, &og);
	last_sample = ret > 0 ? ogg_next_sample_pos(&og) : 0;
	asset->audio_length = last_sample - start_sample;
	if( asset->audio_length <= 0 ) {
		eprintf(_("no audio samples in file"));
		asset->audio_length = 0;
		last_sample = start_sample;
	}

	asset->channels = vi.channels;
	if( !asset->sample_rate )
		asset->sample_rate = vi.rate;
	asset->audio_data = 1;

	ogg_sample_position = -10;
	set_audio_position(0); // make sure seeking is done to the first sample
	strncpy(asset->acodec, "vorb", 4);
	return 0;
}

int FileOGG::ogg_init_decode(FILE *inp)
{
	if( !inp ) return 1;
	this->inp = inp;
	struct stat file_stat; /* get file length */
	file_end = stat(asset->path, &file_stat)>=0 ? file_stat.st_size : 0;
	if( file_end < mn_pagesz ) return 1;
	fseek(inp, 0, SEEK_SET);
	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);
	th_comment_init(&tc);
	th_info_init(&ti);
	ogg_page og;
	ogg_packet op;
	sync_window_t sy(inp, file_lock, 0, file_end);
	int ret = sy.ogg_read_buffer_at(0, READ_SIZE);
	if( ret < mn_pagesz ) return 1;
	if( !sy.ogg_sync_and_take_page_out(&og) ) return 1;
	ogg_stream_state tst;

	while( ogg_page_bos(&og) ) {
		ogg_stream_init(&tst, ogg_page_serialno(&og));
		ogg_stream_pagein(&tst, &og);
		if( ogg_stream_packetout(&tst, &op) ) {
			if( !video && th_decode_headerin(&ti, &tc, &ts, &op) >=0 ) {
				ogg_stream_init(&to, ogg_page_serialno(&og));
				video = 1;
			}
			else if( !audio && vorbis_synthesis_headerin(&vi, &vc, &op) >=0 ) {
				ogg_stream_init(&vo, ogg_page_serialno(&og));
				audio = 1;
			}
		}
		ogg_stream_clear(&tst);
		ret = sy.ogg_take_page_out_autoadvance(&og);
	}

	if( !ret || !video && !audio )
		return 1;

	// expecting more a/v header packets
	int vpkts = video ? 2 : 0;
	int apkts = audio ? 2 : 0;
	int retries = 100;
	ret = 0;
	while( --retries >= 0 && !ret && (vpkts || apkts) ) {
		if( vpkts && ogg_page_serialno(&og) == to.serialno ) {
			ogg_stream_init(&tst, to.serialno);
			ogg_stream_pagein(&tst, &og);
			while( !ret && vpkts > 0 ) {
				while( (ret=ogg_stream_packetout(&tst, &op)) < 0 );
				if( !ret ) break;
				--vpkts;
				ret = !th_decode_headerin(&ti, &tc, &ts, &op) ? 1 : 0;
			}
			if( ret )
				printf("theora header error\n");
			ogg_stream_clear(&tst);
		}
		else if( apkts && ogg_page_serialno(&og) == vo.serialno ) {
			ogg_stream_init(&tst, vo.serialno);
			ogg_stream_pagein(&tst, &og);
			while( !ret && apkts > 0 ) {
				while( (ret=ogg_stream_packetout(&tst, &op)) < 0 );
				if( !ret ) break;
				--apkts;
				ret = vorbis_synthesis_headerin(&vi, &vc, &op) ? 1 : 0;
			}
			if( ret )
				printf("vorbis header error\n");
			ogg_stream_clear(&tst);
		}
		if( !ret && !sy.ogg_take_page_out_autoadvance(&og) )
			ret = 1;
		if( ret )
			printf("incomplete headers\n");

	}
// find first start packet (not continued) with data
	int64_t start_pos = sy.bufpos - (og.header_len + og.body_len);
	if( !ret ) {
		while( --retries >= 0 && !ret && !ogg_page_packets(&og) ) {
			if( !ogg_page_continued(&og) )
				start_pos = sy.bufpos - (og.header_len + og.body_len);
			if( !sy.ogg_take_page_out_autoadvance(&og) ) ret = 1;
		}
		if( ret )
			printf("no data past headers\n");
		if( audio && apkts )
			printf("missed %d audio headers\n",apkts);
		if( video && vpkts )
			printf("missed %d video headers\n",vpkts);
	}
	if( retries < 0 || ret || (audio && apkts) || (video && vpkts) ) {
		eprintf(_("Error in headers"));
		return 1;
	}
	// headers end here
	file_begin = start_pos;

	if( video && decode_theora_init() )
		return 1;
	if( audio && decode_vorbis_init() )
		return 1;
	return 0;
}

void FileOGG::close_encoder()
{
// flush streams
	if( audio )
		write_samples_vorbis(0, 0, 1);
	if( video )
		write_frames_theora(0, 1, 1);
	flush_ogg(1);

	if( audio ) {
		vorbis_block_clear(&vb);
		vorbis_dsp_clear(&vd);
		vorbis_comment_clear(&vc);
		vorbis_info_clear(&vi);
		ogg_stream_clear(&vo);
		audio = 0;
	}
	if( video ) {
		th_comment_clear(&tc);
		ogg_stream_clear(&to);
		video = 0;
	}
	if( enc ) {
		th_encode_free(enc);
		enc = 0;
	}
	if( out ) {
		fclose(out);
		out = 0;
	}
}

void FileOGG::close_decoder()
{
	if( audio ) {
		for( int i=0; i<pcm_channels; ++i )
			delete [] pcm_history[i];
		pcm_channels = 0;
		delete [] pcm_history;  pcm_history = 0;

		vorbis_dsp_clear(&vd);
		vorbis_info_clear(&vi);
		vorbis_block_clear(&vb);
		vorbis_comment_clear(&vc);
		ogg_stream_clear(&vo);
		delete audiosync;  audiosync = 0;
		audio = 0;
	}
	if( video ) {
		th_info_clear(&ti);
		th_setup_free(ts);  ts = 0;
		th_comment_clear(&tc);
		ogg_stream_clear(&to);
		delete videosync;  videosync = 0;
		video = 0;
	}
	if( dec ) {
		th_decode_free(dec);
		dec = 0;
	}
	if( inp ) {
		fclose(inp);
		inp = 0;
	}
}



void FileOGG::get_parameters(BC_WindowBase *parent_window, Asset *asset,
	BC_WindowBase* &format_window, int audio_options,
	int video_options, EDL *edl)
{
	if(audio_options)
	{
		OGGConfigAudio *window = new OGGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
	{
		OGGConfigVideo *window = new OGGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}



int sync_window_t::ogg_take_page_out_autoadvance(ogg_page *og)
{
	for(;;) {
		int ret = ogg_sync_pageout(this, og);
		if( ret < 0 ) {
			printf("FileOGG: Lost sync reading input file\n");
			return 0;
		}
		if( ret > 0 ) {
			bufpos += og->header_len + og->body_len;
			return ret;
		}
		// need more data for page
		if( !ogg_read_buffer(READ_SIZE) ) {
			printf("FileOGG: Read past end of input file\n");
			return 0;  // No more data
		}
	}
	return 1;
}


int FileOGG::check_sig(Asset *asset)
{
	FILE *fp = fopen(asset->path, "rb");
	if( !fp ) return 0;
// Test for "OggS"
	fseek(fp, 0, SEEK_SET);
	char data[4];
	int ret = fread(data, 4, 1, fp) == 1 &&
		data[0] == 'O' && data[1] == 'g' &&
		data[2] == 'g' && data[3] == 'S' ? 1 : 0;
	fclose(fp);
	return ret;

}

int FileOGG::open_file(int rd, int wr)
{
	int ret = 1;
	if( wr ) {
		if( !(out = fopen(asset->path, "wb")) ) {
			eprintf(_("Error while opening %s for writing. %m\n"), asset->path);
			return 1;
		}
		if( (ret = ogg_init_encode(out)) && out ) {
			fclose(out);  out = 0;
		}
	}
	else if( rd ) {
		if( !(inp = fopen(asset->path, "rb")) ) {
			eprintf(_("Error while opening %s for reading. %m\n"), asset->path);
			return 1;
		}
		if( (ret = ogg_init_decode(inp)) && inp ) {
			fclose(inp);  inp = 0;
		}
	}
	return ret;
}

int FileOGG::close_file()
{
	if( file->wr )
		close_encoder();
	else if( file->rd )
		close_decoder();
	return 0;
}


int64_t FileOGG::ogg_sample_pos(ogg_page *og)
{
	ogg_packet op;
	ogg_stream_state ss;
	ogg_stream_init(&ss, vo.serialno);
	ogg_stream_pagein(&ss, og);
	int64_t bsz = 0;
	long prev = -1;
	int ret = 0;
	while( (ret=ogg_stream_packetout(&ss, &op)) ) {
		if( ret < 0 ) continue; // ignore holes
		long sz =  vorbis_packet_blocksize(&vi, &op);
		if( prev != -1 ) bsz += (prev + sz) >> 2;
		prev = sz;
	}
	ogg_stream_clear(&ss);
	return ogg_next_sample_pos(og) - bsz;
}

int64_t FileOGG::ogg_next_sample_pos(ogg_page *og)
{
	return ogg_page_granulepos(og);
}

int64_t FileOGG::ogg_frame_pos(ogg_page *og)
{
	int64_t pos = th_granule_frame(dec, ogg_page_granulepos(og)) - ogg_page_packets(og);
	if( ogg_page_continued(og) ) --pos;
	return pos;
}

int64_t FileOGG::ogg_next_frame_pos(ogg_page *og)
{
	return th_granule_frame(dec, ogg_page_granulepos(og)) + 1;
}


int FileOGG::ogg_get_page_of_sample(ogg_page *og, int64_t sample)
{
	if( sample >= asset->audio_length + start_sample ) {
		printf(_("FileOGG: Illegal seek beyond end of samples\n"));
		return 0;
	}
// guess about position
	int64_t file_length =  audiosync->file_end - audiosync->file_begin;
	off_t guess = file_length * (sample - start_sample) /
		asset->audio_length - SEEK_SIZE;
	if( guess < 0 ) guess = 0;
	guess += audiosync->file_begin;
	audiosync->ogg_read_buffer_at(guess, READ_SIZE);
	if( !audiosync->ogg_sync_and_get_next_page(vo.serialno, og) ) {
		printf(_("FileOGG: llegal seek no pages\n"));
		return 0;
	}
        int ret = 1;
	while( ret && (ogg_page_granulepos(og) == -1 || !ogg_page_packets(og)) )
		ret = videosync->ogg_get_next_page(to.serialno, og);
	if( !ret ) return 0;
	// linear seek to the sample
	int missp = 0, missm = 0;
	int64_t next_pos = ogg_next_sample_pos(og);
	if( sample >= next_pos ) { // scan forward
		while( sample >= next_pos ) {
			while( !(ret=audiosync->ogg_get_next_page(vo.serialno, og)) &&
				(ogg_page_granulepos(og) == -1 || !ogg_page_packets(og)) );
			if( !ret ) break;
			next_pos = ogg_next_sample_pos(og);
			++missp;
//printf("audio %jd next %jd %jd\n", sample, ogg_sample_pos(og), next_pos);
		}
	}
	else { // scan backward
		int64_t pos = ogg_sample_pos(og);
		while( sample < pos ) {
			while( (ret=audiosync->ogg_get_prev_page(vo.serialno, og)) &&
				(ogg_page_continued(og) && ogg_page_packets(og) == 1) );
			if( !ret ) break;
			++missm;
			pos = ogg_sample_pos(og);
//printf("audio %jd prev %jd %jd\n", sample, pos, ogg_next_sample_pos(og));
		}
	}
//printf("audio %d seek %jd, missp %d, missm %d  from %jd to %jd\n", ret,
// sample, missp, missm, ogg_sample_pos(og), ogg_next_sample_pos(og));
	return ret;
}

int FileOGG::ogg_seek_to_sample(int64_t ogg_sample)
{
	ogg_page og;
	ogg_packet op;
	if( !ogg_get_page_of_sample(&og, ogg_sample) ) {
		eprintf(_("Seeking to sample's page failed\n"));
		return 0;
	}
	int ret = 1;
	int64_t pos = ogg_sample_pos(&og);
	int64_t next_pos = pos;
	if( ogg_page_continued(&og) ) {
		while( (ret=audiosync->ogg_get_prev_page(to.serialno, &og)) &&
			(ogg_page_packets(&og) == 0 && ogg_page_continued(&og)) );
	}
	if( ret ) {
		audio_eos = 0;
		ogg_stream_reset(&vo);
		ogg_stream_pagein(&vo, &og);
		vorbis_synthesis_restart(&vd);
		ret = ogg_get_audio_packet(&op);
	}
	if( ret && !vorbis_synthesis(&vb, &op) ) {
		vorbis_synthesis_blockin(&vd, &vb);
		if( vorbis_synthesis_pcmout(&vd, 0) )
			ret = 0;
	}
	if( !ret ) {
		eprintf(_("Something wrong while trying to seek\n"));
		return 0;
	}

	while( ogg_sample > next_pos ) {
		if( !(ret=ogg_get_audio_packet(&op)) ) break;
		if( vorbis_synthesis(&vb, &op) ) continue;
		vorbis_synthesis_blockin(&vd, &vb);
		pos = next_pos;
		next_pos += vorbis_synthesis_pcmout(&vd, NULL);
		if( next_pos > ogg_sample ) break;
		// discard decoded data before current sample
		vorbis_synthesis_read(&vd, (next_pos - pos));
	}
	if( ret ) {
		audio_pos = next_pos;
		vorbis_synthesis_read(&vd, (ogg_sample - pos));
	}
	return ret;
}


int FileOGG::ogg_get_page_of_frame(ogg_page *og, int64_t frame)
{
	if( frame >= asset->video_length + start_frame ) {
		eprintf(_("Illegal seek beyond end of frames\n"));
		return 0;
	}
	if( frame < start_frame ) {
		eprintf(_("Illegal seek before start of frames\n"));
		return 0;
	}
	int64_t file_length = videosync->file_end - videosync->file_begin;
	off_t guess = file_length * (frame - start_frame) /
		 asset->video_length - SEEK_SIZE;
	if( guess < 0 ) guess = 0;
	guess += videosync->file_begin;
	videosync->ogg_read_buffer_at(guess, SEEK_SIZE);
	videosync->ogg_sync_and_get_next_page(to.serialno, og);
	// find the page with "real" ending
	int ret = 1;
	while( ret && (ogg_page_granulepos(og) == -1 || !ogg_page_packets(og)) )
	       ret = videosync->ogg_get_next_page(to.serialno, og);
	int64_t pos = ogg_next_frame_pos(og);
	// linear search
	int missp = 0, missm = 0;
// move back if continued
	if( frame >= pos ) {
		do { // scan forward
			while( (ret=videosync->ogg_get_next_page(to.serialno, og)) &&
				ogg_page_packets(og) == 0 );
			if( !ret ) break;
			missp++;
			pos = ogg_next_frame_pos(og);
//printf("video %jd next %jd %jd\n", frame, ogg_frame_pos(og), pos);
		} while( frame >= pos );
	}
	else if( (pos=ogg_frame_pos(og)) > frame ) {
		while( pos > start_frame && frame < pos ) { // scan backward
			while( (ret=videosync->ogg_get_prev_page(to.serialno, og)) &&
				ogg_page_packets(og) == 0 && ogg_page_continued(og) );
			if( !ret ) break;
			missm++;
			pos = ogg_frame_pos(og);
//printf("video %jd next %jd %jd\n", frame, pos, ogg_next_frame_pos(og));
		}
	}
//printf("video %d seek %jd, missp %d, missm %d first %jd, next %jd\n", ret,
// frame, missp, missm, ogg_frame_pos(og), ogg_next_frame_pos(og));
	return ret;
}

int FileOGG::ogg_seek_to_keyframe(int64_t frame, int64_t *keyframe_number)
{
//printf("ogg_seek_to_keyframe of === %jd\n", frame);
	ogg_page og;
	ogg_packet op;
	int64_t ipagpos = -1;
	int64_t istart = -1;
	int64_t iframe = -1;
	int ipkts = -1;
	int retries = 1000, ret = 1;
	while( --retries>=0 && frame>=start_frame ) {
		if( !ogg_get_page_of_frame(&og, frame) ) break;
		int64_t pos = ogg_frame_pos(&og);
		istart = pos;
		if( ogg_page_continued(&og) ) {
			while( (ret=videosync->ogg_get_prev_page(to.serialno, &og)) &&
				(ogg_page_packets(&og) == 0 && ogg_page_continued(&og)) );
		}
		int64_t pagpos = videosync->pagpos;
		video_eos = 0;
		ogg_stream_reset(&to);
		ogg_stream_pagein(&to, &og);
		int pkts = 0;
		while( frame >= pos && (ret=ogg_get_video_packet(&op)) ) {
			if( th_packet_iskeyframe(&op) == 1 ) {
				ipagpos = pagpos;
				iframe = pos;
				ipkts = pkts;
//printf("keyframe %jd pkts %d\n", pos, pkts);
			}
//printf("packet %jd pkts %d is a %d\n", pos, pkts,  th_packet_iskeyframe(&op));
			++pkts;  ++pos;
		}
		if( ipagpos >= 0 ) break;
		frame = istart - 1;
	}
	if( ipagpos < 0 ) {
		printf(_("Seeking to keyframe %jd search failed\n"), frame);
		return 0;
	}
	videosync->ogg_read_buffer_at(ipagpos, READ_SIZE);
	videosync->ogg_sync_and_get_next_page(to.serialno, &og);
	video_eos = 0;
	ogg_stream_reset(&to);
	ogg_stream_pagein(&to, &og);
	video_pos = ogg_next_frame_pos(&og);
// skip prev packets
//	int ipkts = iframe - ogg_frame_pos(&og);
//printf("iframe %jd, page %jd, ipkts %d\n", iframe, ogg_page_pageno(&og), ipkts);
	while( --ipkts >= 0 )
		ogg_get_video_packet(&op);
	*keyframe_number = iframe;
	return 1;
}


int64_t FileOGG::get_video_position()
{
//	printf("GVP\n");
	return next_frame_position - start_frame;
}

int64_t FileOGG::get_audio_position()
{
	return next_sample_position - start_sample;
}

int FileOGG::set_video_position(int64_t x)
{
//	x=0;
//	printf("SVP: %lli\n", x);

	next_frame_position = x + start_frame;
	return 1;
}


int FileOGG::colormodel_supported(int colormodel)
{
//	printf("CMS\n");

	if (colormodel == BC_YUV420P)
		return BC_YUV420P;
	else
		return colormodel;
}
int FileOGG::get_best_colormodel(Asset *asset, int driver)
{

	return BC_YUV420P;
}

int FileOGG::set_audio_position(int64_t x)
{
	next_sample_position = x + start_sample;
	return 0;
}


int FileOGG::ogg_get_video_packet(ogg_packet *op)
{
	int ret = 1;
	while( (ret=ogg_stream_packetout(&to, op)) <= 0 ) {
		if( video_eos ) return 0;
		ogg_page og;
		if( !videosync->ogg_get_next_page(to.serialno, &og) ) break;
		if( ogg_page_granulepos(&og) >= 0 )
			video_pos = ogg_next_frame_pos(&og);
		ogg_stream_pagein(&to, &og);
		video_eos = ogg_page_eos(&og);
	}
	if( ret <= 0 ) {
		printf("FileOGG: Cannot read video packet\n");
		return 0;
	}
	return 1;
}

int FileOGG::read_frame(VFrame *frame)
{
	if( !inp || !video ) return 1;
	// skip is cheaper than seek, do it...
	int decode_frames = 0;
	int expect_keyframe = 0;
	if( ogg_frame_position >= 0 &&
	    next_frame_position >= ogg_frame_position &&
	    next_frame_position - ogg_frame_position < 32) {
		decode_frames = next_frame_position - ogg_frame_position;
	}
	else if( next_frame_position != ogg_frame_position ) {
		if( !ogg_seek_to_keyframe(next_frame_position, &ogg_frame_position) ) {
			eprintf(_("Error while seeking to frame's keyframe"
				" (frame: %jd, keyframe: %jd)\n"),
				next_frame_position, ogg_frame_position);
			return 1;
		}
		decode_frames = next_frame_position - ogg_frame_position + 1;
		--ogg_frame_position;
		if( decode_frames <= 0 ) {
			eprintf(_("Error while seeking to keyframe,"
				" wrong keyframe number (frame: %jd, keyframe: %jd)\n"),
				next_frame_position, ogg_frame_position);
			return 1;

		}
		expect_keyframe = 1;
	}
	int ret = 0;
	ogg_packet op;
	while( decode_frames > 0 ) {
		if( !ogg_get_video_packet(&op) ) break;
		if( expect_keyframe ) {
			expect_keyframe = 0;
			if( th_packet_iskeyframe(&op) <= 0 )
				eprintf(_("FileOGG: Expecting keyframe, but didn't get it\n"));
		}
		ogg_int64_t granpos = 0;
		if( th_decode_packetin(dec, &op, &granpos) >= 0 )
			ret = 1;
		++ogg_frame_position;
		--decode_frames;
	}
//if(ret < 0 )printf("ret = %d\n", ret);
	if( ret > 0 ) {
		th_ycbcr_buffer ycbcr;
		ret = th_decode_ycbcr_out(dec, ycbcr);
		if( ret ) {
			eprintf(_("th_decode_ycbcr_out failed with code %i\n"), ret);
			ret = 0; // not always fatal
		}
		uint8_t *yp = ycbcr[0].data;
		uint8_t *up = ycbcr[1].data;
		uint8_t *vp = ycbcr[2].data;
		int yst = ycbcr[0].stride;
		int yw = ycbcr[0].width;
		int yh = ycbcr[0].height;
		VFrame temp_frame(yp, -1, 0, up-yp, vp-yp, yw,yh, BC_YUV420P, yst);
		int px = ti.pic_x, py = ti.pic_y;
		int pw = ti.pic_width, ph = ti.pic_height;
		frame->transfer_from(&temp_frame, -1, px, py, pw, ph);
	}

	next_frame_position++;
	return ret;
}


int FileOGG::ogg_get_audio_packet(ogg_packet *op)
{
	int ret = 1;
	while( (ret=ogg_stream_packetout(&vo, op)) <= 0 ) {
		if( audio_eos ) return 0;
		ogg_page og;
		if( !audiosync->ogg_get_next_page(vo.serialno, &og) ) break;
		if( ogg_page_granulepos(&og) >= 0 )
			audio_pos = ogg_next_sample_pos(&og);
		ogg_stream_pagein(&vo, &og);
		audio_eos = ogg_page_eos(&og);
	}
	if( ret <= 0 ) {
		printf("FileOGG: Cannot read audio packet\n");
		return 0;
	}
	return 1;
}

int FileOGG::ogg_decode_more_samples()
{
	ogg_packet op;
	while( ogg_get_audio_packet(&op) ) {
		if( !vorbis_synthesis(&vb, &op) ) {
			vorbis_synthesis_blockin(&vd, &vb);
			return 1;
		}
	}
	ogg_sample_position = -11;
	eprintf(_("Cannot find next page while trying to decode more samples\n"));
	return 0;
}

int FileOGG::move_history(int from, int to, int len)
{
	if( len > 0 ) {
		for( int i=0; i<asset->channels; ++i )
			memmove(pcm_history[i] + to,
				pcm_history[i] + from,
				sizeof(float) * len);
	}
	history_start = history_start + from - to;
	if( history_start < 0 ) history_start = 0;
	return 0;
}

int FileOGG::read_samples(double *buffer, int64_t len)
{
	float **vorbis_buffer;
	if( len <= 0 )
		return 0;
	if( len > HISTORY_MAX ) {
		eprintf(_("max samples=%d\n"), HISTORY_MAX);
		return 1;
	}

	if( !pcm_history ) {
		pcm_history = new float*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
			pcm_history[i] = new float[HISTORY_MAX];
		history_start = -100000000;
		history_size = 0;
	}

	int64_t hole_start = -1;
	int64_t hole_len = -1;
	int64_t hole_absstart = -1;
	int64_t hole_fill = 0;

	if( history_start < next_sample_position &&
	    history_start + history_size > next_sample_position &&
	    history_start + history_size < next_sample_position + len ) {
		hole_fill = 1;
		hole_start = history_start + history_size - next_sample_position;
		hole_len = history_size - hole_start;
		hole_absstart = next_sample_position + hole_start;
		move_history(next_sample_position - history_start, 0, hole_start);
	}
	else if( next_sample_position < history_start &&
		 history_start < next_sample_position + len ) {
		hole_fill = 1;
		hole_start = 0;
		hole_len = history_start - next_sample_position;
		hole_absstart = next_sample_position;
		move_history(0,
			history_start - next_sample_position,
			history_size - history_start + next_sample_position);

	}
	else if( next_sample_position >= history_start + history_size ||
		 next_sample_position + len <= history_start ) {
		hole_fill = 1;
		hole_start = 0;
		hole_len = HISTORY_MAX;
		hole_absstart = next_sample_position;
		history_start = hole_absstart;
		history_size = hole_len;
	}

	if( hole_fill ) {
		if( hole_start < 0 || hole_len <= 0 || hole_absstart < 0 ) {
			eprintf(_("Error in finding read file position\n"));
			return 1;
		}

		if( hole_absstart + hole_len > asset->audio_length + start_sample ) {
			hole_len = asset->audio_length + start_sample - hole_absstart;
			history_size = asset->audio_length + start_sample - history_start;
		}
		else {
			history_size = HISTORY_MAX;
		}

		int64_t samples_read = 0;
		if( ogg_sample_position != hole_absstart ) {
			ogg_sample_position = hole_absstart;
			if( !ogg_seek_to_sample(ogg_sample_position) ) {
				eprintf(_("Error while seeking to sample\n"));
				return 1;
			}
		}
		// now we have ogg_sample_positon aligned
		int64_t samples_to_read = hole_len;
		while( samples_read < hole_len ) {
			int64_t samples_waiting = vorbis_synthesis_pcmout(&vd, &vorbis_buffer);
			int64_t samples_avail = !samples_waiting && audio_eos ?
				hole_len - samples_read : // silence after eos
				samples_waiting ;
			int64_t sample_demand = samples_to_read - samples_read;
			int64_t sample_count = MIN(samples_avail, sample_demand);
			if( sample_count > 0 ) {
				int sz = sample_count*sizeof(float);
				if( samples_waiting ) {
					for( int i=0; i<asset->channels; ++i ) {
						float *input = vorbis_buffer[i];
						float *output = pcm_history[i] + hole_start;
						memcpy(output, input, sz);
					}
					vorbis_synthesis_read(&vd, sample_count);
				}
				else {
					for( int i=0; i<asset->channels; ++i ) {
						float *output = pcm_history[i] + hole_start;
						memset(output, 0, sz);
					}
				}
				ogg_sample_position += sample_count;
				hole_start += sample_count;
				samples_read += sample_count;
				if( samples_read >= hole_len ) break;
			}
			if( samples_read < hole_len && !ogg_decode_more_samples() )
				break;
		}
	}

	if( next_sample_position < history_start ||
	    next_sample_position + len > history_start + history_size ) {
		printf(_("FileOGG:: History not aligned properly \n"));
		printf(_("\tnext_sample_position: %jd, length: %jd\n"), next_sample_position, len);
		printf(_("\thistory_start: %jd, length: %jd\n"), history_start, history_size);
		return 1;
	}
	float *input = pcm_history[file->current_channel] + next_sample_position - history_start;
	for (int i = 0; i < len; i++)
		buffer[i] = input[i];

	next_sample_position += len;
	return 0;
}


int FileOGG::write_audio_page()
{
	int ret = apage.write_page(out);
	if( ret < 0 )
		eprintf(_("error writing audio page\n"));
	return ret;
}

int FileOGG::write_video_page()
{
	int ret = vpage.write_page(out);
	if( ret < 0 )
		eprintf(_("error writing video page\n"));
	return ret;
}

// flush out the ogg pages
void FileOGG::flush_ogg(int last)
{
	ogg_page og;
	file_lock->lock("FileOGG::flush_ogg");
	for(;;) {
// this way seeking is much better, (per original fileogg)
// not sure if 32 packets is a good value.
		int mx_pkts = 32;
		if( video && !vpage.valid ) {
			if( (vpage.packets > mx_pkts && ogg_stream_flush(&to, &og) > 0) ||
			    ogg_stream_pageout(&to, &og) > 0 ) {
				videotime = th_granule_time(enc, vpage.load(&og));
			}
		}
		if( audio && !apage.valid ) {
			if( (apage.packets > mx_pkts && ogg_stream_flush(&vo, &og) > 0) ||
			    ogg_stream_pageout(&vo, &og) > 0 ) {
				audiotime = vorbis_granule_time(&vd, apage.load(&og));
			}
		}
		if( !audio && vpage.valid )
			write_video_page();
		else if( !video && apage.valid )
			write_audio_page();
		else if( !vpage.valid || !apage.valid )
			break;
// output earliest page
		else if( videotime > audiotime ) // output earliest
			write_audio_page();
		else
			write_video_page();
	}
	if( last ) {  // at last
		if( vpage.valid )
			write_video_page();
		if( apage.valid )
			write_audio_page();
	}
	file_lock->unlock();
}


int FileOGG::write_samples_vorbis(double **buffer, int64_t len, int last)
{
	if( !audio || !out ) return 1;
	flush_ogg(0);
	if( !last ) {
		float **vorbis_buffer = vorbis_analysis_buffer(&vd, len);
		for( int i=0; i<asset->channels; ++i ) // double to float
			for( int j=0; j < len; ++j )
				vorbis_buffer[i][j] = buffer[i][j];
	}
	else
		len = 0;
	vorbis_analysis_wrote(&vd, len);

	while( vorbis_analysis_blockout(&vd, &vb) == 1 ) {
		vorbis_analysis(&vb, 0);
		vorbis_bitrate_addblock(&vb);
		ogg_packet op;
		while( vorbis_bitrate_flushpacket(&vd, &op) ) {
			file_lock->lock("FileOGG::write_vorbis_audio");
			ogg_stream_packetin(&vo, &op);
			++apage.packets;
			file_lock->unlock();
		}
	}
	return 0;
}

int FileOGG::write_samples(double **buffer, int64_t len)
{
	if (len > 0)
		return write_samples_vorbis(buffer, len, 0);
	return 0;
}


int FileOGG::write_frames_theora(VFrame ***frames, int len, int last)
{
	if( !video || !out ) return 1;
	for( int j=0; j<len; ++j ) {
		flush_ogg(0);
		if( temp_frame ) {
			th_ycbcr_buffer ycbcr;
			ycbcr[0].width = frame_w;
			ycbcr[0].height = frame_h;
			ycbcr[0].stride = temp_frame->get_bytes_per_line();
			ycbcr[0].data = temp_frame->get_y();
			ycbcr[1].width = frame_w/2;
			ycbcr[1].height = frame_h/2;
			ycbcr[1].stride = (temp_frame->get_bytes_per_line()+1)/2;
			ycbcr[1].data = temp_frame->get_u();
			ycbcr[2].width = frame_w/2;
			ycbcr[2].height = frame_h/2;
			ycbcr[2].stride = (temp_frame->get_bytes_per_line()+1)/2;
			ycbcr[2].data = temp_frame->get_v();
			if( th_encode_ycbcr_in(enc, ycbcr) ) {
				eprintf(_("th_encode_ycbcr_in failed"));
				return 1;
			}
			ogg_packet op;
			while( th_encode_packetout(enc, last, &op) > 0 ) {
				file_lock->lock();
				ogg_stream_packetin (&to, &op);
				++vpage.packets;
				file_lock->unlock();
			}
		}
		if( last ) return 0;
		if( !temp_frame )
			temp_frame = new VFrame (0, -1, frame_w, frame_h, theora_cmodel, -1);
		VFrame *frame = frames[0][j];
		temp_frame->transfer_from(frame);
	}
	return 0;
}

int FileOGG::write_frames(VFrame ***frames, int len)
{
	return write_frames_theora(frames, len, 0);
}



OGGConfigAudio::OGGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	xS(350), yS(250))
{
	this->parent_window = parent_window;
	this->asset = asset;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Single File Rendering");
}

OGGConfigAudio::~OGGConfigAudio()
{

}

void OGGConfigAudio::create_objects()
{
//	add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));

	int x = xS(10), y = yS(10);
	int x1 = xS(150);
	char string[BCTEXTLEN];

	lock_window("OGGConfigAudio::create_objects");
	add_tool(fixed_bitrate = new OGGVorbisFixedBitrate(x, y, this));
	add_tool(variable_bitrate = new OGGVorbisVariableBitrate(x + fixed_bitrate->get_w() + xS(5),
		y,
		this));

	y += yS(30);
	sprintf(string, "%d", asset->vorbis_min_bitrate);
	add_tool(new BC_Title(x, y, _("Min bitrate:")));
	add_tool(new OGGVorbisMinBitrate(x1, y, this, string));

	y += yS(30);
	add_tool(new BC_Title(x, y, _("Avg bitrate:")));
	sprintf(string, "%d", asset->vorbis_bitrate);
	add_tool(new OGGVorbisAvgBitrate(x1, y, this, string));

	y += yS(30);
	add_tool(new BC_Title(x, y, _("Max bitrate:")));
	sprintf(string, "%d", asset->vorbis_max_bitrate);
	add_tool(new OGGVorbisMaxBitrate(x1, y, this, string));


	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}

int OGGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}

OGGVorbisFixedBitrate::OGGVorbisFixedBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, !gui->asset->vorbis_vbr, _("Average bitrate"))
{
	this->gui = gui;
}
int OGGVorbisFixedBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 0;
	gui->variable_bitrate->update(0);
	return 1;
}

OGGVorbisVariableBitrate::OGGVorbisVariableBitrate(int x, int y, OGGConfigAudio *gui)
 : BC_Radial(x, y, gui->asset->vorbis_vbr, _("Variable bitrate"))
{
	this->gui = gui;
}
int OGGVorbisVariableBitrate::handle_event()
{
	gui->asset->vorbis_vbr = 1;
	gui->fixed_bitrate->update(0);
	return 1;
}


OGGVorbisMinBitrate::OGGVorbisMinBitrate(int x,
	int y,
	OGGConfigAudio *gui,
	char *text)
 : BC_TextBox(x, y, xS(180), 1, text)
{
	this->gui = gui;
}
int OGGVorbisMinBitrate::handle_event()
{
	gui->asset->vorbis_min_bitrate = atol(get_text());
	return 1;
}



OGGVorbisMaxBitrate::OGGVorbisMaxBitrate(int x,
	int y,
	OGGConfigAudio *gui,
	char *text)
 : BC_TextBox(x, y, xS(180), 1, text)
{
	this->gui = gui;
}
int OGGVorbisMaxBitrate::handle_event()
{
	gui->asset->vorbis_max_bitrate = atol(get_text());
	return 1;
}



OGGVorbisAvgBitrate::OGGVorbisAvgBitrate(int x, int y, OGGConfigAudio *gui, char *text)
 : BC_TextBox(x, y, xS(180), 1, text)
{
	this->gui = gui;
}
int OGGVorbisAvgBitrate::handle_event()
{
	gui->asset->vorbis_bitrate = atol(get_text());
	return 1;
}





OGGConfigVideo::OGGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	xS(450), yS(220))
{
	this->parent_window = parent_window;
	this->asset = asset;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Single File Rendering");
}

OGGConfigVideo::~OGGConfigVideo()
{

}

void OGGConfigVideo::create_objects()
{
//	add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	int x = xS(10), y = yS(10);
	int x1 = x + xS(150);
	int x2 = x + xS(300);

	lock_window("OGGConfigVideo::create_objects");
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Bitrate:")));
	add_subwindow(new OGGTheoraBitrate(x + title->get_w() + xS(5), y, this));
	add_subwindow(fixed_bitrate = new OGGTheoraFixedBitrate(x2, y, this));
	y += yS(30);

	add_subwindow(new BC_Title(x, y, _("Quality:")));
	add_subwindow(new BC_ISlider(x + xS(80), y, 0, xS(200), xS(200),
		0, 63, asset->theora_quality,
		0, 0, &asset->theora_quality));


	add_subwindow(fixed_quality = new OGGTheoraFixedQuality(x2, y, this));
	y += yS(30);

	add_subwindow(new BC_Title(x, y, _("Keyframe frequency:")));
	OGGTheoraKeyframeFrequency *keyframe_frequency =
		new OGGTheoraKeyframeFrequency(x1 + xS(60), y, this);
	keyframe_frequency->create_objects();
	y += yS(30);

	add_subwindow(new BC_Title(x, y, _("Keyframe force frequency:")));
	OGGTheoraKeyframeForceFrequency *keyframe_force_frequency =
		new OGGTheoraKeyframeForceFrequency(x1 + xS(60), y, this);
	keyframe_force_frequency->create_objects();
	y += yS(30);

	add_subwindow(new BC_Title(x, y, _("Sharpness:")));
	OGGTheoraSharpness *sharpness =
		new OGGTheoraSharpness(x1 + xS(60), y, this);
	sharpness->create_objects();
	y += yS(30);


	add_subwindow(new BC_OKButton(this));
	show_window(1);
	unlock_window();
}




int OGGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}

OGGTheoraBitrate::OGGTheoraBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_TextBox(x, y, xS(100), 1, gui->asset->theora_bitrate)
{
	this->gui = gui;
}


int OGGTheoraBitrate::handle_event()
{
	// TODO: MIN / MAX check
	gui->asset->theora_bitrate = atol(get_text());
	return 1;
};




OGGTheoraFixedBitrate::OGGTheoraFixedBitrate(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, gui->asset->theora_fix_bitrate, _("Fixed bitrate"))
{
	this->gui = gui;
}

int OGGTheoraFixedBitrate::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 1;
	gui->fixed_quality->update(0);
	return 1;
};

OGGTheoraFixedQuality::OGGTheoraFixedQuality(int x, int y, OGGConfigVideo *gui)
 : BC_Radial(x, y, !gui->asset->theora_fix_bitrate, _("Fixed quality"))
{
	this->gui = gui;
}

int OGGTheoraFixedQuality::handle_event()
{
	update(1);
	gui->asset->theora_fix_bitrate = 0;
	gui->fixed_bitrate->update(0);
	return 1;
};

OGGTheoraKeyframeFrequency::OGGTheoraKeyframeFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, (int64_t)gui->asset->theora_keyframe_frequency,
	(int64_t)1, (int64_t)500, x, y, xS(40))
{
	this->gui = gui;
}

int OGGTheoraKeyframeFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}

OGGTheoraKeyframeForceFrequency::OGGTheoraKeyframeForceFrequency(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, (int64_t)gui->asset->theora_keyframe_frequency,
	(int64_t)1, (int64_t)500, x, y, xS(40))
{
	this->gui = gui;
}

int OGGTheoraKeyframeForceFrequency::handle_event()
{
	gui->asset->theora_keyframe_frequency = atol(get_text());
	return 1;
}


OGGTheoraSharpness::OGGTheoraSharpness(int x, int y, OGGConfigVideo *gui)
 : BC_TumbleTextBox(gui, (int64_t)gui->asset->theora_sharpness,
	(int64_t)0, (int64_t)2, x, y, xS(40))
{
	this->gui = gui;
}

int OGGTheoraSharpness::handle_event()
{
	gui->asset->theora_sharpness = atol(get_text());
	return 1;
}


