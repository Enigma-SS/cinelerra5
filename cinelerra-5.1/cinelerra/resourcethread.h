
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

#ifndef RESOURCETHREAD_H
#define RESOURCETHREAD_H

// This thread tries to draw picons into the timeline, asynchronous
// of the navigation.

// TrackCanvas draws the picons which are in the cache and makes a table of
// picons and locations which need to be decompressed.  Then ResourceThread
// decompresses the picons and draws them one by one, refreshing the
// entire trackcanvas in the process.


#include "arraylist.h"
#include "linklist.h"
#include "bctimer.inc"
#include "condition.inc"
#include "file.inc"
#include "indexable.inc"
#include "maxchannels.h"
#include "mwindow.inc"
#include "renderengine.inc"
#include "samples.inc"
#include "thread.h"
#include "vframe.inc"


class ResourceThreadItem : public ListItem<ResourceThreadItem>
{
public:
	ResourceThreadItem(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int data_type, int operation_count);
	virtual ~ResourceThreadItem();

	ResourcePixmap *pixmap;
	Indexable *indexable;
	int data_type, pane_number;
	int operation_count, last;
};


class AResourceThreadItem : public ResourceThreadItem
{
public:
	AResourceThreadItem(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
		int64_t start, int64_t end, int operation_count);
	~AResourceThreadItem();
	int x, channel;
	int64_t start, end;
};

class VResourceThreadItem : public ResourceThreadItem
{
public:
	VResourceThreadItem(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable, int operation_count);
	~VResourceThreadItem();

	int picon_x, picon_y;
	int picon_w, picon_h;
	double frame_rate;
	int64_t position;
	int layer;
};


class ResourceThreadBase : public Thread
{
public:
	ResourceThreadBase(ResourceThread *resource_thread);
	~ResourceThreadBase();

	void create_objects();
	void stop_draw(int reset);
	virtual void start_draw();
	virtual void draw_item(ResourceThreadItem *item) = 0;
	void close_render_engine();

// Be sure to stop_draw before changing the asset table,
// closing files.
	void run();
	void stop();
	void reset(int pane_number);

	void open_render_engine(EDL *nested_edl,
		int do_audio, int do_video);

	ResourceThread *resource_thread;
	Condition *draw_lock;
	Mutex *item_lock;
	List<ResourceThreadItem> items;
	int interrupted;
	int done;
// Render engine for nested EDL
	RenderEngine *render_engine;
// ID of nested EDL being rendered
	int render_engine_id;
};

class ResourceAudioThread : public ResourceThreadBase
{
public:
	ResourceAudioThread(ResourceThread *resource_thread);
	~ResourceAudioThread();
	void start_draw();
	File *get_audio_source(Asset *asset);
	void draw_item(ResourceThreadItem *item);
	void do_audio(AResourceThreadItem *item);

	void add_wave(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
 // samples relative to asset rate
		int64_t source_start, int64_t source_end);

	ResourceThread *resource_thread;
	Asset *audio_asset;
	File *audio_source;

// Current audio buffer for spanning multiple pixels
	Samples *audio_buffer;
// Temporary for nested EDL
	Samples *temp_buffer[MAX_CHANNELS];
	int audio_channel;
	int64_t audio_start;
	int audio_samples;
	int audio_asset_id;
// Timer for waveform refreshes
	Timer *timer;
// Waveform state
	int prev_x;
	double prev_h;
	double prev_l;
};

class ResourceVideoThread : public ResourceThreadBase
{
public:
	ResourceVideoThread(ResourceThread *resource_thread);
	~ResourceVideoThread();
	File *get_video_source(Asset *asset);
	void draw_item(ResourceThreadItem *item);
	void do_video(VResourceThreadItem *item);

// Be sure to stop_draw before changing the asset table,
// closing files.
	void add_picon(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable);

	ResourceThread *resource_thread;
	Asset *video_asset;
	File *video_source;

	VFrame *temp_picon;
	VFrame *temp_picon2;
};


class ResourceThread
{
public:
	ResourceThread(MWindow *mwindow);
	~ResourceThread();

	void create_objects();
// reset - delete all picons.  Used for index building.
	void stop_draw(int reset);
	void start_draw();

// Be sure to stop_draw before changing the asset table,
// closing files.
	void add_picon(ResourcePixmap *pixmap, int pane_number,
		int picon_x, int picon_y, int picon_w, int picon_h,
		double frame_rate, int64_t position, int layer,
		Indexable *indexable);

	void add_wave(ResourcePixmap *pixmap, int pane_number,
		Indexable *indexable, int x, int channel,
// samples relative to asset rate
		int64_t source_start, int64_t source_end);

	void run();
	void stop();
	void reset(int pane_number, int indexes_only);
	void close_indexable(Indexable*);

	MWindow *mwindow;
	ResourceAudioThread *audio_thread;
	ResourceVideoThread *video_thread;
	int operation_count;
	int interrupted;
};

#endif
