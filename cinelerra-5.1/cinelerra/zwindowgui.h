
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

#ifndef __ZWINDOWGUI_H__
#define __ZWINDOWGUI_H__

#include "bcwindow.h"
#include "canvas.h"
#include "mwindow.h"
#include "playbackengine.inc"
#include "zwindow.inc"
#include "zwindowgui.inc"

class ZWindowGUI;
class ZWindowCanvas;

class ZWindowGUI : public BC_Window
{
public:
	ZWindowGUI(MWindow *mwindow, ZWindow *vwindow, Mixer *mixer);
	~ZWindowGUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int keypress_event();
	int button_press_event();
	int cursor_leave_event();
	int cursor_enter_event();
	int button_release_event();
	int cursor_motion_event();
	int select_window(int n);
	int draw_overlays();
	void set_highlighted(int v);
	void set_playable(int v);

	MWindow *mwindow;
	ZWindow *zwindow;
	ZWindowCanvas *canvas;

	PlaybackEngine *playback_engine;
	int highlighted, playable;
};

class ZWindowCanvasTileMixers : public BC_MenuItem
{
public:
	ZWindowCanvasTileMixers(ZWindowCanvas *canvas);
	int handle_event();
	ZWindowCanvas *canvas;
};

class ZWindowCanvasPlayable : public BC_MenuItem
{
public:
	ZWindowCanvasPlayable(ZWindowCanvas *canvas);
	int handle_event();
	ZWindowCanvas *canvas;
};


class ZWindowCanvas : public Canvas
{
public:
	ZWindowCanvas(MWindow *mwindow, ZWindowGUI *gui,
		int x, int y, int w, int h);

	void create_objects(EDL *edl);
	void close_source();
	void draw_refresh(int flush = 1);
	float get_auto_zoom();
	float get_zoom();
	void update_zoom(int x, int y, float zoom);
	void zoom_auto();
	void zoom_resize_window(float percentage);

	MWindow *mwindow;
	ZWindowGUI *gui;
};

#endif
