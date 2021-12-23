/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#ifndef SCOPEWINDOW_H
#define SCOPEWINDOW_H


#include "boxblur.inc"
#include "guicast.h"
#include "loadbalance.h"
#include "mwindow.h"
#include "overlayframe.inc"
#include "pluginclient.h"
#include "recordmonitor.inc"
#include "scopewindow.inc"
#include "theme.inc"

enum {
	SCOPE_HISTOGRAM, SCOPE_HISTOGRAM_RGB,
	SCOPE_WAVEFORM, SCOPE_WAVEFORM_RGB, SCOPE_WAVEFORM_PLY,
	SCOPE_VECTORSCOPE, SCOPE_VECTORWHEEL,
	SCOPE_SMOOTH, SCOPE_REFRESH, SCOPE_RELEASE,
};

// Number of divisions in histogram.
// 65536 + min and max range to speed up the tabulation
#define TOTAL_BINS 0x13333
#define HIST_SECTIONS 4
#define FLOAT_RANGE 1.2
// Minimum value in percentage
#define HISTOGRAM_MIN -10
#define FLOAT_MIN -0.1
// Maximum value in percentage
#define HISTOGRAM_MAX 110
#define FLOAT_MAX 1.1

#define MIN_SCOPE_W xS(320)
#define MIN_SCOPE_H yS(320)


#define WAVEFORM_DIVISIONS 12
#define VECTORSCOPE_DIVISIONS 11

class ScopePackage : public LoadPackage
{
public:
	ScopePackage();
	int row1, row2;
};


class ScopeUnit : public LoadClient
{
public:
	ScopeUnit(ScopeGUI *gui, ScopeEngine *server);
	void process_package(LoadPackage *package);
	int bins[HIST_SECTIONS][TOTAL_BINS];
	ScopeGUI *gui;
};

class ScopeEngine : public LoadServer
{
public:
	ScopeEngine(ScopeGUI *gui, int cpus);
	virtual ~ScopeEngine();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	void process();
	ScopeGUI *gui;
};

class ScopePanel : public BC_SubWindow
{
public:
	ScopePanel(ScopeGUI *gui, int x, int y, int w, int h);
	void create_objects();
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int is_dragging;
	ScopeGUI *gui;
};

class ScopeWaveform : public ScopePanel
{
public:
	ScopeWaveform(ScopeGUI *gui, int x, int y, int w, int h);
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	int drag_x;
	int drag_y;
};


class ScopeVectorscope : public ScopePanel
{
public:
	ScopeVectorscope(ScopeGUI *gui, int x, int y, int w, int h);
	virtual void update_point(int x, int y);
	virtual void draw_point();
	virtual void clear_point();
	void draw_point(float th, float r, int color);
	void draw_radient(float th, float r1, float r2, int color);

	int drag_radius;
	float drag_angle;
};

class ScopeHistogram : public ScopePanel
{
public:
	ScopeHistogram(ScopeGUI *gui, int x, int y, int w, int h);
	void clear_point();
	void update_point(int x, int y);
	void draw_point();
	void draw(int flash, int flush);
	void draw_mode(int mode, int color, int y, int h);
	int drag_x;
};


class ScopeScopesOn : public BC_MenuItem
{
public:
	ScopeScopesOn(ScopeMenu *scope_menu, const char *text, int id);
	int handle_event();

	ScopeMenu *scope_menu;
	int id;
};

class ScopeMenu : public BC_PopupMenu
{
public:
	ScopeMenu(ScopeGUI *gui, int x, int y);
	void create_objects();
	void update_toggles();

	ScopeGUI *gui;
	ScopeScopesOn *hist_on;
	ScopeScopesOn *hist_rgb_on;
	ScopeScopesOn *wave_on;
	ScopeScopesOn *wave_rgb_on;
	ScopeScopesOn *wave_ply_on;
	ScopeScopesOn *vect_on;
	ScopeScopesOn *vect_wheel;
};

class ScopeSettingOn : public BC_MenuItem
{
public:
	ScopeSettingOn(ScopeSettings *settings, const char *text, int id);
	int handle_event();

	ScopeSettings *settings;
	int id;
};

class ScopeGratPaths : public ArrayList<const char *>
{
public:
	ScopeGratPaths() { set_array_delete(); }
	~ScopeGratPaths() { remove_all_objects(); }
};

class ScopeGratItem : public BC_MenuItem
{
public:
	ScopeGratItem(ScopeSettings *settings, const char *text, int idx);
	int handle_event();

	ScopeSettings *settings;
	int idx;
};

class ScopeSettings : public BC_PopupMenu
{
public:
	ScopeSettings(ScopeGUI *gui, int x, int y);
	void create_objects();
	void update_toggles();

	ScopeGUI *gui;
	ScopeSettingOn *smooth_on;
	ScopeSettingOn *refresh_on;
	ScopeSettingOn *release_on;
};


class ScopeGainReset : public BC_Button
{
public:
	ScopeGainReset(ScopeGain *gain, int x, int y);
	static int calculate_w(BC_Theme *theme);
	int handle_event();

	ScopeGain *gain;
};

class ScopeGainSlider : public BC_ISlider
{
public:
	ScopeGainSlider(ScopeGain *gain, int x, int y, int w);

	int handle_event();
	ScopeGain *gain;
};

class ScopeGain
{
public:
	ScopeGain(ScopeGUI *gui, int x, int y, int w, int *value);
	~ScopeGain();
	static int calculate_h();
	void create_objects();
	void reposition_window(int x, int y);
	int handle_event();

	ScopeGUI *gui;
	int x, y, w, *value;
	int reset_w;
	ScopeGainReset *reset;
	ScopeGainSlider *slider;

	int get_x() { return x; }
	int get_y() { return y; }
	int get_w() { return w; }
	int get_h() { return calculate_h(); }
};

class ScopeWaveSlider : public ScopeGain
{
public:
	ScopeWaveSlider(ScopeGUI *gui, int x, int y, int w);
};

class ScopeVectSlider : public ScopeGain
{
public:
	ScopeVectSlider(ScopeGUI *gui, int x, int y, int w);
};


class ScopeGUI : public PluginClientWindow
{
public:
	ScopeGUI(Theme *theme, int x, int y, int w, int h, int cpus);
	ScopeGUI(PluginClient *plugin, int w, int h);
	virtual ~ScopeGUI();

	void reset();
	virtual void create_objects();
	void create_panels();
	virtual int resize_event(int w, int h);
	virtual int translation_event();
	virtual void update_scope() {}

// Called for user storage when toggles change
	virtual void toggle_event();

// update toggles
	void update_toggles();
	void calculate_sizes(int w, int h);
	void allocate_vframes();
	void draw_overlays(int overlays, int borders, int flush);
	void update_graticule(int idx);
	void draw_colorwheel(VFrame *dst, int bg_color);
	void draw_scope();
	void process(VFrame *output_frame);
	void draw(int flash, int flush);
	void clear_points(int flash);

	Theme *theme;
	VFrame *output_frame;
	VFrame *data_frame, *temp_frame;
	ScopeEngine *engine;
	BoxBlur *box_blur;
	VFrame *waveform_vframe;
	VFrame *vector_vframe;
	VFrame *wheel_vframe;
	ScopeHistogram *histogram;
	ScopeWaveform *waveform;
	ScopeVectorscope *vectorscope;
	ScopeMenu *scope_menu;
	ScopeWaveSlider *wave_slider;
	ScopeVectSlider *vect_slider;
	ScopeSettings *settings;
	VFrame *grat_image;
	OverlayFrame *overlay;

	int x, y, w, h;
	int vector_x, vector_y, vector_w, vector_h;
	int vector_cx, vector_cy, radius;
	int wave_x, wave_y, wave_w, wave_h;
	int hist_x, hist_y, hist_w, hist_h;
	int text_color, dark_color;

	ScopeGratPaths grat_paths;
	int grat_idx, use_graticule;

	int cpus;
	int use_hist, use_wave, use_vector;
	int use_hist_parade, use_wave_parade;

	int bins[HIST_SECTIONS][TOTAL_BINS];
	int frame_w, use_smooth;
	int use_refresh, use_release;
	int use_wave_gain, use_vect_gain;
};

#endif
