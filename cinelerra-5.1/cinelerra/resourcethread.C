
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "arender.h"
#include "asset.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "cache.h"
#include "clip.h"
#include "condition.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "framecache.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "renderengine.h"
#include "resourcethread.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vframe.h"
#include "vrender.h"
#include "wavecache.h"


#include <unistd.h>

ResourceThreadItem::ResourceThreadItem(ResourcePixmap *pixmap,
	int pane_number,
	Indexable *indexable,
	int data_type,
	int operation_count)
{
	this->pane_number = pane_number;
	this->data_type = data_type;
	this->pixmap = pixmap;
	this->indexable = indexable;

// Assets are garbage collected so they don't need to be replicated.
	this->operation_count = operation_count;
	indexable->Garbage::add_user();
	last = 0;
}

ResourceThreadItem::~ResourceThreadItem()
{
	indexable->Garbage::remove_user();
}


VResourceThreadItem::VResourceThreadItem(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable, int operation_count)
 : ResourceThreadItem(pixmap, pane_number,
	 	indexable, TRACK_VIDEO, operation_count)
{
	this->picon_x = picon_x;
	this->picon_y = picon_y;
	this->picon_w = picon_w;
	this->picon_h = picon_h;
	this->frame_rate = frame_rate;
	this->position = position;
	this->layer = layer;
}

VResourceThreadItem::~VResourceThreadItem()
{
}


AResourceThreadItem::AResourceThreadItem(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
		int64_t start, int64_t end, int operation_count)
 : ResourceThreadItem(pixmap, pane_number,
	 	indexable, TRACK_AUDIO, operation_count)
{
	this->x = x;
	this->channel = channel;
	this->start = start;
	this->end = end;
}

AResourceThreadItem::~AResourceThreadItem()
{
}

ResourceAudioThread::ResourceAudioThread(ResourceThread *resource_thread)
 : ResourceThreadBase(resource_thread)
{
	this->resource_thread = resource_thread;
	audio_buffer = 0;
	audio_asset = 0;
	audio_source = 0;
	for(int i = 0; i < MAXCHANNELS; i++)
		temp_buffer[i] = 0;
	timer = new Timer;
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
}

ResourceAudioThread::~ResourceAudioThread()
{
	delete audio_buffer;
	for(int i = 0; i < MAXCHANNELS; i++)
		delete temp_buffer[i];
	delete timer;
	if( audio_asset ) audio_asset->remove_user();
}

void ResourceAudioThread::start_draw()
{
	prev_x = -1;
	prev_h = 0;
	prev_l = 0;
	ResourceThreadItem *item = items.last;
// Tag last audio item to cause refresh.
	if( item ) item->last = 1;
	timer->update();
	ResourceThreadBase::start_draw();
}

File *ResourceAudioThread::get_audio_source(Asset *asset)
{
	MWindow *mwindow = resource_thread->mwindow;
	if( interrupted ) asset = 0;

	if( audio_asset && audio_asset != asset && (!asset ||
		strcmp(audio_asset->path, asset->path)) ) {
		mwindow->audio_cache->check_in(audio_asset);
		audio_source = 0;
		audio_asset->remove_user();
		audio_asset = 0;

	}
	if( !audio_asset && asset ) {
		audio_asset = asset;
		audio_asset->add_user();
		audio_source = mwindow->audio_cache->check_out(asset, mwindow->edl);
	}
	return audio_source;
}

void ResourceAudioThread::add_wave(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
		int64_t source_start, int64_t source_end)
{
	item_lock->lock("ResourceThreadBase::item_lock");
	items.append(new AResourceThreadItem(pixmap, pane_number,
			indexable, x, channel, source_start, source_end,
			resource_thread->operation_count));
	item_lock->unlock();
}


ResourceVideoThread::ResourceVideoThread(ResourceThread *resource_thread)
 : ResourceThreadBase(resource_thread)
{
	this->resource_thread = resource_thread;
	video_asset = 0;
	video_source = 0;
	temp_picon = 0;
	temp_picon2 = 0;
}
ResourceVideoThread::~ResourceVideoThread()
{
	delete temp_picon;
	delete temp_picon2;
	if( video_asset ) video_asset->remove_user();
}

File *ResourceVideoThread::get_video_source(Asset *asset)
{
	MWindow *mwindow = resource_thread->mwindow;
	if( interrupted ) asset = 0;

	if( video_asset && video_asset != asset && (!asset ||
		strcmp(video_asset->path, asset->path)) ) {
		mwindow->video_cache->check_in(video_asset);
		video_source = 0;
		video_asset->remove_user();
		video_asset = 0;

	}
	if( !video_asset && asset ) {
		video_asset = asset;
		video_asset->add_user();
		video_source = mwindow->video_cache->check_out(asset, mwindow->edl);
	}
	return video_source;
}

void ResourceVideoThread::add_picon(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable)
{
	item_lock->lock("ResourceThreadBase::item_lock");
	items.append(new VResourceThreadItem(pixmap, pane_number,
			picon_x, picon_y, picon_w, picon_h,
			frame_rate, position, layer, indexable,
			resource_thread->operation_count));
	item_lock->unlock();
}



ResourceThreadBase::ResourceThreadBase(ResourceThread *resource_thread)
 : Thread(1, 0, 0)
{
	this->resource_thread = resource_thread;
	interrupted = 1;
	done = 1;
	draw_lock = new Condition(0, "ResourceThreadBase::draw_lock", 0);
	item_lock = new Mutex("ResourceThreadBase::item_lock");
	render_engine = 0;
	render_engine_id = -1;

}

ResourceThreadBase::~ResourceThreadBase()
{
	stop();
	delete draw_lock;
	delete item_lock;
	delete render_engine;
}

void ResourceThreadBase::create_objects()
{
	done = 0;
	Thread::start();
}

void ResourceThreadBase::reset(int pane_number)
{
	item_lock->lock("ResourceThreadBase::reset");
	ResourceThreadItem *item = items.first;
	while( item ) {
		ResourceThreadItem *next_item = item->next;
		if( item->pane_number == pane_number ) delete item;
		item = next_item;
	}
	item_lock->unlock();
}

void ResourceThreadBase::stop_draw(int reset)
{
	if( !interrupted ) {
		interrupted = 1;
		item_lock->lock("ResourceThreadBase::stop_draw");
//printf("ResourceThreadBase::stop_draw %d %d\n", __LINE__, reset);
//BC_Signals::dump_stack();
		if( reset ) {
			ResourceThreadItem *item;
			while( (item=items.last) != 0 ) delete item;
		}
		item_lock->unlock();
	}
}

void ResourceThreadBase::start_draw()
{
	interrupted = 0;
	draw_lock->unlock();
}

void ResourceThreadBase::run()
{
	MWindow *mwindow = resource_thread->mwindow;
	while( !done ) {
		draw_lock->lock("ResourceThreadBase::run");
		while( !interrupted ) {
			item_lock->lock("ResourceThreadBase::run");
			ResourceThreadItem *item = items.first;
			items.remove_pointer(item);
			item_lock->unlock();
			if( !item ) break;
			draw_item(item);
			delete item;
		}
		mwindow->age_caches();
	}
}

void ResourceThreadBase::stop()
{
	if( !done ) {
		done = 1;
		interrupted = 1;
		draw_lock->unlock();
	}
	join();
}


void ResourceThreadBase::open_render_engine(EDL *nested_edl,
		int do_audio, int do_video)
{
	if( render_engine && render_engine_id != nested_edl->id ) {
		delete render_engine;  render_engine = 0;
	}

	if( !render_engine ) {
		MWindow *mwindow = resource_thread->mwindow;
		TransportCommand command(mwindow->preferences);
		command.command = do_audio ? NORMAL_FWD : CURRENT_FRAME;
		command.get_edl()->copy_all(nested_edl);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;
		render_engine = new RenderEngine(0, mwindow->preferences, 0, 0);
		render_engine_id = nested_edl->id;
		render_engine->set_vcache(mwindow->video_cache);
		render_engine->set_acache(mwindow->audio_cache);
		render_engine->arm_command(&command);
	}
}

void ResourceThreadBase::close_render_engine()
{
	delete render_engine;  render_engine = 0;
	render_engine_id = -1;
}

void ResourceVideoThread::draw_item(ResourceThreadItem *item)
{
	do_video((VResourceThreadItem *)item);
}

void ResourceVideoThread::do_video(VResourceThreadItem *item)
{
	int source_w = 0;
	int source_h = 0;
	int source_id = -1;
	int source_cmodel = -1;

	if( item->indexable->is_asset ) {
		Asset *asset = (Asset*)item->indexable;
		source_w = asset->width;
		source_h = asset->height;
		source_id = asset->id;
		source_cmodel = BC_RGB888;
	}
	else {
		EDL *nested_edl = (EDL*)item->indexable;
		source_w = nested_edl->session->output_w;
		source_h = nested_edl->session->output_h;
		source_id = nested_edl->id;
		source_cmodel = nested_edl->session->color_model;
	}
	VFrame::get_temp(temp_picon, source_w, source_h, source_cmodel);
	VFrame::get_temp(temp_picon2, item->picon_w, item->picon_h, BC_RGB888);

// Search frame cache again.

	VFrame *picon_frame = 0;
	int need_conversion = 0;
	EDL *nested_edl = 0;
	Asset *asset = 0;
	MWindow *mwindow = resource_thread->mwindow;

	picon_frame = mwindow->frame_cache->get_frame_ptr(item->position,
		item->layer, item->frame_rate, BC_RGB888,
		item->picon_w, item->picon_h, source_id);
//printf("search cache %ld,%d,%f,%dx%d,%d = %p\n",
//  item->position, item->layer, item->frame_rate,
//  item->picon_w, item->picon_h, source_id, picon_frame);
	if( picon_frame ) {
		temp_picon2->copy_from(picon_frame);
// Unlock the get_frame_ptr command
		mwindow->frame_cache->unlock();
	}
	else if( !item->indexable->is_asset ) {
		nested_edl = (EDL*)item->indexable;
		open_render_engine(nested_edl, 0, 1);

		int64_t source_position = (int64_t)(item->position *
			nested_edl->session->frame_rate /
			item->frame_rate);
		if(render_engine->vrender)
			render_engine->vrender->process_buffer(
				temp_picon,
				source_position,
				0);

		need_conversion = 1;
	}
	else {
		asset = (Asset*)item->indexable;
		File *source = get_video_source(asset);
		if(!source) return;

 		source->set_layer(item->layer);
		int64_t normalized_position = (int64_t)(item->position *
				asset->frame_rate / item->frame_rate);
		source->set_video_position(normalized_position, 0);
		source->read_frame(temp_picon);
		get_video_source(0);
		need_conversion = 1;
	}

	if( need_conversion ) {
		picon_frame = new VFrame(item->picon_w, item->picon_h, BC_RGB888, 0);
		picon_frame->transfer_from(temp_picon);
		temp_picon2->copy_from(picon_frame);
		mwindow->frame_cache->put_frame(picon_frame, item->position, item->layer,
			mwindow->edl->session->frame_rate, 0, item->indexable);
	}

// Allow escape here
	if(interrupted) return;

	MWindowGUI *gui = mwindow->gui;
// Draw the picon
	gui->lock_window("ResourceThreadBase::do_video");
// It was interrupted while waiting.
	if( interrupted ) {
		gui->unlock_window();
		return;
	}

// Test for pixmap existence first
	if( item->operation_count == resource_thread->operation_count ) {
		ArrayList<ResourcePixmap*> &resource_pixmaps = gui->resource_pixmaps;
		int i = resource_pixmaps.total;
		while( --i >= 0 && resource_pixmaps[i] != item->pixmap );
		if( i >= 0 ) {
			item->pixmap->draw_vframe(temp_picon2,
				item->picon_x, item->picon_y,
				item->picon_w, item->picon_h, 0, 0);
			TimelinePane *pane = gui->pane[item->pane_number];
		        if( pane ) pane->update(0, IGNORE_THREAD, 0, 0);
		}
	}

	gui->unlock_window();
}


#define BUFFERSIZE 65536
void ResourceAudioThread::draw_item(ResourceThreadItem *item)
{
	do_audio((AResourceThreadItem *)item);
}

void ResourceAudioThread::do_audio(AResourceThreadItem *item)
{
// Search again
	double high = 0, low = 0;
	MWindow *mwindow = resource_thread->mwindow;
	WaveCacheItem * wave_item =
		mwindow->wave_cache->get_wave(item->indexable->id,
			item->channel, item->start, item->end);
	if( wave_item ) {
		high = wave_item->high;
		low = wave_item->low;
		mwindow->wave_cache->unlock();
	}
	else {
		int first_sample = 1;
		int64_t start = item->start;
		int64_t end = item->end;
		if(start == end) end = start + 1;
		double *buffer_samples = !audio_buffer ? 0 :
			audio_buffer->get_data();

		for( int64_t sample=start; sample<end; ++sample ) {
// Get value from previous buffer
			if(audio_buffer &&
				item->channel == audio_channel &&
				item->indexable->id == audio_asset_id &&
				sample >= audio_start &&
				sample < audio_start + audio_samples) {}
			else { // Load new buffer
				if(!audio_buffer) {
					audio_buffer = new Samples(BUFFERSIZE);
					buffer_samples = audio_buffer->get_data();
				}

				int64_t total_samples = item->indexable->get_audio_samples();
				int fragment = BUFFERSIZE;
				if( fragment + sample > total_samples )
					fragment = total_samples - sample;

				if( !item->indexable->is_asset ) {
					open_render_engine((EDL*)item->indexable, 1, 0);
					if( render_engine->arender ) {
						int source_channels = item->indexable->get_audio_channels();
						for( int i=0; i<MAXCHANNELS; ++i ) {
							if( i<source_channels && !temp_buffer[i] ) {
								temp_buffer[i] = new Samples(BUFFERSIZE);
							}
							else if( i >= source_channels && temp_buffer[i] ) {
								delete temp_buffer[i];  temp_buffer[i] = 0;
							}
						}
						render_engine->arender->
							process_buffer( temp_buffer, fragment, sample);
						memcpy(buffer_samples,
							temp_buffer[item->channel]->get_data(),
							fragment * sizeof(double));
					}
					else {
						if(fragment > 0) bzero(buffer_samples, sizeof(double) * fragment);
					}
				}
				else {
					Asset *asset = (Asset*)item->indexable;
					File *source = get_audio_source(asset);
					if( !source ) return;
					source->set_channel(item->channel);
					source->set_audio_position(sample);
					source->read_samples(audio_buffer, fragment);
					get_audio_source(0);
				}
				audio_asset_id = item->indexable->id;
				audio_channel = item->channel;
				audio_start = sample;
				audio_samples = fragment;
			}
			double value = buffer_samples[sample - audio_start];
			if( first_sample ) {
				high = low = value;
				first_sample = 0;
			}
			else {
				if( value > high ) high = value;
				else if( value < low ) low = value;
			}
		}

// If it's a nested EDL, store all the channels
		mwindow->wave_cache->put_wave(item->indexable, item->channel,
				item->start, item->end, high, low);
	}

// Allow escape here
	if(interrupted) return;

	MWindowGUI *gui = mwindow->gui;
// Draw the column
	gui->lock_window("ResourceThreadBase::do_audio");
	if( interrupted ) {
		gui->unlock_window();
		return;
	}

	if( item->operation_count == resource_thread->operation_count ) {
// Test for pixmap existence first
		ArrayList<ResourcePixmap*> &resource_pixmaps = gui->resource_pixmaps;
		int i = resource_pixmaps.total;
		while( --i >= 0 && resource_pixmaps[i] != item->pixmap );
		if( i >= 0 ) {
			if( prev_x == item->x-1 ) {
				high = MAX(high, prev_l);
				low = MIN(low, prev_h);
			}
			prev_x = item->x;
			prev_h = high;
			prev_l = low;
			if( gui->pane[item->pane_number] )
				item->pixmap->draw_wave(
					gui->pane[item->pane_number]->canvas,
					item->x, high, low);
			if( timer->get_difference() > 250 || item->last ) {
				gui->update(0, IGNORE_THREAD, 0, 0, 0, 0, 0);
				timer->update();
			}
		}
	}

	gui->unlock_window();
}

ResourceThread::ResourceThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	audio_thread = 0;
	video_thread = 0;
	interrupted = 1;
	operation_count = 0;
}

ResourceThread::~ResourceThread()
{
	delete audio_thread;
	delete video_thread;
}

void ResourceThread::create_objects()
{
	audio_thread = new ResourceAudioThread(this);
	audio_thread->create_objects();
	video_thread = new ResourceVideoThread(this);
	video_thread->create_objects();
}

void ResourceThread::stop_draw(int reset)
{
	if( !interrupted ) {
		interrupted = 1;
		audio_thread->stop_draw(reset);
		video_thread->stop_draw(reset);
		++operation_count;
	}
}

void ResourceThread::start_draw()
{
	if( interrupted ) {
		interrupted = 0;
		audio_thread->start_draw();
		video_thread->start_draw();
	}
}

// Be sure to stop_draw before changing the asset table,
// closing files.
void ResourceThread::add_picon(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable)
{
	video_thread->add_picon(pixmap, pane_number,
		picon_x, picon_y, picon_w, picon_h,
		frame_rate, position, layer, indexable);
}

void ResourceThread::add_wave(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
// samples relative to asset rate
		int64_t source_start, int64_t source_end)
{
	audio_thread->add_wave(pixmap, pane_number,
		indexable, x, channel, source_start, source_end);
}

void ResourceThread::run()
{
	audio_thread->run();
	video_thread->run();
}

void ResourceThread::stop()
{
	audio_thread->stop();
	video_thread->stop();
}

void ResourceThread::reset(int pane_number, int indexes_only)
{
	audio_thread->reset(pane_number);
	if( !indexes_only )
		video_thread->reset(pane_number);
}

void ResourceThread::close_indexable(Indexable *idxbl)
{
	if( audio_thread && audio_thread->render_engine_id == idxbl->id )
		audio_thread->close_render_engine();
	if( video_thread && video_thread->render_engine_id == idxbl->id )
		video_thread->close_render_engine();
}

