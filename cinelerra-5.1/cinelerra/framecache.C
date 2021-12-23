
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

#include "bcsignals.h"
#include "clip.h"
#include "cstrdup.h"
#include "edl.h"
#include "framecache.h"
#include "indexable.h"
#include "mutex.h"
#include "vframe.h"


#include <limits.h>
#include <math.h>
#include <string.h>
#include <unistd.h>



FrameCacheItem::FrameCacheItem()
 : CacheItemBase()
{
	data = 0;
	position = 0;
	frame_rate = (double)30000.0 / 1001;
}

FrameCacheItem::~FrameCacheItem()
{
	delete data;
}

int FrameCacheItem::get_size()
{
	if(data) return data->get_data_size() + (path ? strlen(path)+1 : 0);
	return 0;
}















FrameCache::FrameCache()
 : CacheBase()
{
}

FrameCache::~FrameCache()
{
}


VFrame* FrameCache::get_frame_ptr(int64_t position, int layer, double frame_rate,
		int color_model, int w, int h, int source_id)
{
	lock->lock("FrameCache::get_frame_ptr");
	VFrame *vframe = get_vframe(position, w, h, color_model,
			layer, frame_rate, source_id);
	if( vframe ) return vframe;  // not unlocked
	lock->unlock();
	return 0;
}

VFrame *FrameCache::get_vframe(int64_t position, int w, int h,
		int color_model, int layer, double frame_rate,
		int source_id)
{
	FrameCacheItem *item = 0;
	int ret = frame_exists(position, layer, frame_rate,
			w, h, color_model, &item, source_id);
	if( ret && position >= 0 && item )
		item->age = get_age();
	return ret && item ? item->data : 0;
}

VFrame *FrameCache::get_frame(int64_t position, int w, int h,
		int color_model, int layer, double frame_rate,
		int source_id)
{
	lock->lock("FrameCache::get_frame");
	VFrame *frame = get_vframe(position, w, h,
			color_model, layer, frame_rate, source_id);
	lock->unlock();
	return frame;
}

// Returns 1 if frame exists in cache and copies it to the frame argument.
int FrameCache::get_frame(VFrame *frame, int64_t position,
		int layer, double frame_rate, int source_id)
{
	lock->lock("FrameCache::get_frame");
	VFrame *vframe = get_vframe(position,
			frame->get_w(), frame->get_h(), frame->get_color_model(),
			layer, frame_rate, source_id);
	if( vframe )
		frame->copy_from(vframe);
	lock->unlock();
	return vframe ? 1 : 0;
}


void FrameCache::put_vframe(VFrame *frame, int64_t position,
		int layer, double frame_rate, int source_id)
{
	FrameCacheItem *item = new FrameCacheItem;
	item->data = frame;
	item->position = position;
	item->layer = layer;
	item->frame_rate = frame_rate;
	item->source_id = source_id;
	item->age = position < 0 ? INT_MAX : get_age();
	put_item(item);
}

// Puts frame in cache if the frame doesn't already exist.
void FrameCache::put_frame(VFrame *frame, int64_t position,
		int layer, double frame_rate, int use_copy, Indexable *idxbl)
{
	int source_id = idxbl ? idxbl->id : -1;
	lock->lock("FrameCache::put_frame");
	VFrame *vframe = get_vframe(position,
			frame->get_w(), frame->get_h(), frame->get_color_model(),
			layer, frame_rate, source_id);
	if( !vframe ) {
		if( use_copy ) frame = new VFrame(*frame);
		put_vframe(frame, position, layer, frame_rate, source_id);
	}
	lock->unlock();
}

// get vframe for keys, overwrite frame if found
int FrameCache::get_cache_frame(VFrame *frame, int64_t position,
		int layer, double frame_rate)
{
	lock->lock("FrameCache::get_cache_frame");
	VFrame *vframe = get_vframe(position,
		frame->get_w(), frame->get_h(), frame->get_color_model(),
		layer, frame_rate, -1);
	if( vframe )
		frame->copy_from(vframe);
	lock->unlock();
	return vframe ? 1 : 0;
}

// adds or replaces vframe, consumes frame if not use_copy
void FrameCache::put_cache_frame(VFrame *frame, int64_t position,
		int layer, double frame_rate, int use_copy)
{
	lock->lock("FrameCache::put_cache_frame");
	FrameCacheItem *item = 0;
	int w = frame->get_w(), h = frame->get_h(); 
	int color_model = frame->get_color_model();
	int ret = frame_exists(position, layer, frame_rate,
			w, h, color_model, &item, -1);
	if( use_copy ) {
// do not use shm here, puts too much pressure on 32bit systems
		VFrame *vframe = new VFrame(w, h, color_model, 0);
		vframe->copy_from(frame);
		frame = vframe;
	}
	if( ret ) {
		delete item->data;
		item->data = frame;
	}
	else
		put_vframe(frame, position, layer, frame_rate, -1);
	lock->unlock();
}


int FrameCache::frame_exists(VFrame *format, int64_t position,
	int layer, double frame_rate, FrameCacheItem **item_return, int source_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	for( ; item && item->position == position; item = (FrameCacheItem*)item->next ) {
		if( !EQUIV(item->frame_rate, frame_rate) ) continue;
		if( layer != item->layer ) continue;
		if( !format->equivalent(item->data, 0) ) continue;
		if( source_id == -1 || item->source_id == -1 ||
		    source_id == item->source_id ) {
			*item_return = item;
			return 1;
		}
	}
	return 0;
}

int FrameCache::frame_exists(int64_t position, int layer, double frame_rate,
		int w, int h, int color_model, FrameCacheItem **item_return, int source_id)
{
	FrameCacheItem *item = (FrameCacheItem*)get_item(position);
	for( ; item && item->position == position ; item = (FrameCacheItem*)item->next ) {
		if( !EQUIV(item->frame_rate, frame_rate) ) continue;
		if( layer != item->layer ) continue;
		if( color_model != item->data->get_color_model() ) continue;
		if( w != item->data->get_w() ) continue;
		if( h != item->data->get_h() ) continue;
		if( source_id == -1 || item->source_id == -1 ||
		    source_id == item->source_id ) {
			*item_return = item;
			return 1;
		}
	}
	return 0;
}


void FrameCache::dump()
{
// 	lock->lock("FrameCache::dump");
 	printf("FrameCache::dump 1 %d\n", total());
 	FrameCacheItem *item = (FrameCacheItem *)first;
	while( item ) {
 		printf("  position=%jd frame_rate=%f age=%d size=%ld\n",
 			item->position, item->frame_rate, item->age,
 			item->data->get_data_size());
		item = (FrameCacheItem*)item->next;
 	}
// 	lock->unlock();
}




