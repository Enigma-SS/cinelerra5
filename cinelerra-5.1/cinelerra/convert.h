#ifndef __CONVERT_H__
#define __CONVERT_H__

/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
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

#include "arraylist.h"
#include "audiodevice.inc"
#include "asset.h"
#include "bcdialog.h"
#include "bctimer.inc"
#include "convert.inc"
#include "file.inc"
#include "formattools.h"
#include "guicast.h"
#include "mainprogress.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagerenderer.h"
#include "render.inc"

class ConvertRender : public Thread
{
public:
	ConvertRender(MWindow *mwindow);
	~ConvertRender();
	void reset();
	void to_convert_path(char *new_path, Indexable *idxbl);
	int from_convert_path(char *new_path, Indexable *idxbl);

	ArrayList<Indexable *> orig_idxbls;	// originals which match the convert assets
	ArrayList<Asset *> orig_copies;		// convert assets
	ArrayList<Indexable *> needed_idxbls;	// originals which match the needed_assets
	ArrayList<Asset *> needed_copies;	// assets which must be created

	double get_video_length(Indexable *idxbl);
	double get_audio_length(Indexable *idxbl);
	double get_length(Indexable *idxbl);
	int match_format(Asset *asset);
	EDL *convert_edl(EDL *edl, Indexable *idxbl);  // create render edl for this indexable
	int add_original(EDL *edl, Indexable *idxbl);
	void add_needed(Indexable *idxbl, Asset *convert);
	int find_convertable_assets(EDL *edl);

// if user canceled progress bar
	int is_canceled();
	void set_format(Asset *asset, const char *suffix, int to_proxy);
	void start_convert(float beep, int remove_originals);
	void run();
	void create_copy(int i);
	void start_progress();
	void stop_progress(const char *msg);

	MWindow *mwindow;
	const char *suffix;
	Asset *format_asset;
	MainProgressBar *progress;
	ConvertProgress *convert_progress;
	Timer *progress_timer;
	ConvertPackageRenderer *renderer;

	Mutex *counter_lock;
	int total_rendered, remove_originals;
	int failed, canceled, result;
	float beep;
	int to_proxy;
};

class ConvertMenuItem : public BC_MenuItem
{
public:
	ConvertMenuItem(MWindow *mwindow);
	~ConvertMenuItem();

	int handle_event();
	void create_objects();

	MWindow *mwindow;
	ConvertDialog *dialog;
};

class ConvertFormatTools : public FormatTools
{
public:
	ConvertFormatTools(MWindow *mwindow, ConvertWindow *gui, Asset *asset);

	void update_format();
	ConvertWindow *gui;
};

class ConvertSuffixText : public BC_TextBox
{
public:
	ConvertSuffixText(ConvertWindow *gui, ConvertDialog *dialog, int x, int y);
	~ConvertSuffixText();
	int handle_event();

	ConvertWindow *gui;
	ConvertDialog *dialog;
};

class ConvertRemoveOriginals : public BC_CheckBox
{
public:
	ConvertRemoveOriginals(ConvertWindow *gui, int x, int y);
        ~ConvertRemoveOriginals();

	int handle_event();

	ConvertWindow *gui;
};

class ConvertToProxyPath : public BC_CheckBox
{
public:
	ConvertToProxyPath(ConvertWindow *gui, int x, int y);
	~ConvertToProxyPath();

	int handle_event();

	ConvertWindow *gui;
};

class ConvertBeepOnDone : public BC_FPot
{
public:
	ConvertBeepOnDone(ConvertWindow *gui, int x, int y);
	void update();
	int handle_event();

	ConvertWindow *gui;
};

class ConvertPackageRenderer : public PackageRenderer
{
public:
	ConvertPackageRenderer(ConvertRender *render);
	virtual ~ConvertPackageRenderer();

	int get_master();
	int get_result();
	void set_result(int value);
	void set_progress(int64_t value);
	int progress_cancelled();

	ConvertRender *render;
};

class ConvertProgress : public Thread
{
public:
	ConvertProgress(MWindow *mwindow, ConvertRender *render);
	~ConvertProgress();

	void run();

	MWindow *mwindow;
	ConvertRender *render;
	int64_t last_value;
};

class ConvertWindow : public BC_Window
{
public:
	ConvertWindow(MWindow *mwindow, ConvertDialog *dialog,
		int x, int y);
	~ConvertWindow();

	void create_objects();

	MWindow *mwindow;
	ConvertDialog *dialog;

	ConvertSuffixText *suffix_text;
	ConvertFormatTools *format_tools;
	ConvertRemoveOriginals *remove_originals;
	ConvertToProxyPath *to_proxy_path;
	ConvertBeepOnDone *beep_on_done;
};

class ConvertDialog : public BC_DialogThread
{
public:
	ConvertDialog(MWindow *mwindow);
	~ConvertDialog();
	BC_Window* new_gui();

	void handle_close_event(int result);
	void convert();

	MWindow *mwindow;
	ConvertWindow *gui;
	Asset *asset;
	char suffix[BCTEXTLEN];
	ConvertRender *convert_render;

	int orig_scale, new_scale;
	int use_scaler, auto_scale;
	int orig_w, orig_h;
	int remove_originals;
	float beep;
	int to_proxy;
};

#endif
