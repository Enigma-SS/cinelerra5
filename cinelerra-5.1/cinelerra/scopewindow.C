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

#include "bcsignals.h"
#include "bccolors.h"
#include "boxblur.h"
#include "clip.h"
#include "cursors.h"
#include "file.h"
#include "filesystem.h"
#include "language.h"
#include "overlayframe.h"
#include "scopewindow.h"
#include "theme.h"

#include <string.h>

ScopePackage::ScopePackage()
 : LoadPackage()
{
}

ScopeUnit::ScopeUnit(ScopeGUI *gui,
	ScopeEngine *server)
 : LoadClient(server)
{
	this->gui = gui;
}

#define SCOPE_SEARCHPATH "/scopes"

#define incr_point(rows,h, iv,comp) { \
if(iy >= 0 && iy < h) { \
  uint8_t *vp = rows[iy] + ix*3 + (comp); \
  int v = *vp+(iv);  *vp = v>0xff ? 0xff : v; } \
}
#define incr_points(rows,h, rv,gv,bv, comps) { \
if(iy >= 0 && iy < h) { \
  uint8_t *vp = rows[iy] + ix*comps; \
  int v = *vp+(rv);  *vp++ = v>0xff ? 0xff : v; \
  v = *vp+(gv);  *vp++ = v>0xff ? 0xff : v; \
  v = *vp+(bv);  *vp = v>0xff ? 0xff : v; } \
}
#define decr_points(rows,h, rv,gv,bv, comps) { \
if(iy >= 0 && iy < h) { \
  uint8_t *vp = rows[iy] + ix*comps; \
  int v = *vp-(rv);  *vp++ = v<0 ? 0 : v; \
  v = *vp-(gv);  *vp++ = v<0 ? 0 : v; \
  v = *vp-(bv);  *vp = v<0 ? 0 : v; } \
}
#define clip_points(rows,h, rv,gv,bv) { \
if(iy >= 0 && iy < h) { \
  uint8_t *vp = rows[iy] + ix*3; \
  int v = *vp+(rv);  *vp++ = v>0xff ? 0xff : v<0 ? 0 : v; \
  v = *vp+(gv);  *vp++ = v>0xff ? 0xff : v<0 ? 0 : v; \
  v = *vp+(bv);  *vp = v>0xff ? 0xff : v<0 ? 0 : v; } \
}

#define PROCESS_PIXEL(column) { \
/* Calculate histogram */ \
	if(use_hist) { \
		int v_i = (intensity - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(v_i, 0, TOTAL_BINS - 1); \
		bins[3][v_i]++; \
	} \
	if(use_hist_parade) { \
		int r_i = (r - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int g_i = (g - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int b_i = (b - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(r_i, 0, TOTAL_BINS - 1); \
		CLAMP(g_i, 0, TOTAL_BINS - 1); \
		CLAMP(b_i, 0, TOTAL_BINS - 1); \
		bins[0][r_i]++; \
		bins[1][g_i]++; \
		bins[2][b_i]++; \
	} \
/* Calculate waveform */ \
	if(use_wave || use_wave_parade) { \
		int ix = column * wave_w / dat_w; \
		if(ix >= 0 && ix < wave_w-1) { \
			if(use_wave_parade > 0) { \
				ix /= 3; \
				int iy = wave_h - ((r - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc,0); \
				ix += wave_w/3; \
				iy = wave_h - ((g - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc,1); \
				ix += wave_w/3; \
				iy = wave_h - ((b - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_point(waveform_rows,wave_h, winc,2); \
			} \
			else if(use_wave_parade < 0) { \
				int iy = wave_h - ((r - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, winc,-winc,-winc); \
				iy = wave_h - ((g - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, -winc,winc,-winc); \
				iy = wave_h - ((b - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				clip_points(waveform_rows,wave_h, -winc,-winc,winc); \
			} \
			else { float yinc = 3*winc; \
				float rinc = yinc*(r-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				float ginc = yinc*(g-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				float binc = yinc*(b-FLOAT_MIN) / (FLOAT_MAX-FLOAT_MIN) + 3; \
				int iy = wave_h - ((intensity - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * wave_h); \
				incr_points(waveform_rows,wave_h, rinc, ginc, binc, 3); \
			} \
		} \
	} \
/* Calculate vectorscope */ \
	if(use_vector > 0) { \
		double t = TO_RAD(-h); \
		float adjacent = sin(t), opposite = cos(t); \
		int ix = vector_cx + adjacent * (s) / (FLOAT_MAX) * radius; \
		int iy = vector_cy - opposite * (s) / (FLOAT_MAX) * radius; \
		CLAMP(ix, 0, vector_w - 1); \
		HSV::hsv_to_rgb(r, g, b, h, s, vincf); \
		int rv = r + 3; \
		int gv = g + 3; \
		int bv = b + 3; \
		incr_points(vector_rows,vector_h, rv,gv,bv, 4); \
	} \
	else if(use_vector < 0) { \
		double t = TO_RAD(-h); \
		float adjacent = sin(t), opposite = cos(t); \
		int ix = vector_cx + adjacent * (s) / (FLOAT_MAX) * radius; \
		int iy = vector_cy - opposite * (s) / (FLOAT_MAX) * radius; \
		CLAMP(ix, 0, vector_w - 1); \
		decr_points(vector_rows,vector_h, vinc,vinc,vinc, 4); \
	} \
}

#define PROCESS_RGB_PIXEL(type,max, column) { \
	type *rp = (type *)row; \
	r = (float)rp[0] / max; \
	g = (float)rp[1] / max; \
	b = (float)rp[2] / max; \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

#define PROCESS_BGR_PIXEL(type,max, column) { \
	type *rp = (type *)row; \
	b = (float)rp[0] / max; \
	g = (float)rp[1] / max; \
	r = (float)rp[2] / max; \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

#define PROCESS_YUV_PIXEL(column, y_in, u_in, v_in) { \
	YUV::yuv.yuv_to_rgb_f(r, g, b, y_in, u_in, v_in); \
	HSV::rgb_to_hsv(r, g, b, h, s, v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

void ScopeUnit::process_package(LoadPackage *package)
{
	ScopePackage *pkg = (ScopePackage*)package;

	float r, g, b;
	float h, s, v;
	float intensity;
	int use_hist = gui->use_hist;
	int use_hist_parade = gui->use_hist_parade;
	int use_vector = gui->use_vector;
	int use_wave = gui->use_wave;
	int use_wave_parade = gui->use_wave_parade;
	int vector_cx = gui->vector_cx;
	int vector_cy = gui->vector_cy;
	int radius = gui->radius;
	VFrame *waveform_vframe = gui->waveform_vframe;
	VFrame *vector_vframe = gui->vector_vframe;
	int wave_h = waveform_vframe->get_h();
	int wave_w = waveform_vframe->get_w();
	int vector_h = vector_vframe->get_h();
	int vector_w = vector_vframe->get_w();
	int dat_w = gui->data_frame->get_w();
	int dat_h = gui->data_frame->get_h();
	int winc = (wave_w * wave_h) / (dat_w * dat_h);
	if( use_wave_parade ) winc *= 3;
	winc += 2;  winc = (winc << gui->use_wave_gain) / 4;
	int vinc = 3*(vector_w * vector_h) / (dat_w * dat_h);
	vinc += 2;  vinc = (vinc << gui->use_vect_gain) / 4;
	float vincf = vinc;
	uint8_t **waveform_rows = waveform_vframe->get_rows();
	uint8_t **vector_rows = vector_vframe->get_rows();
	uint8_t **rows = gui->data_frame->get_rows();

	switch( gui->data_frame->get_color_model() ) {
	case BC_RGB888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(uint8_t,0xff, x)
				row += 3*sizeof(uint8_t);
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(uint8_t,0xff, x)
				row += 4*sizeof(uint8_t);
			}
		}
		break;
	case BC_RGB161616:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(uint16_t,0xffff, x)
				row += 3*sizeof(uint16_t);
			}
		}
		break;
	case BC_RGBA16161616:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(uint16_t,0xffff, x)
				row += 4*sizeof(uint16_t);
			}
		}
		break;
	case BC_BGR888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_BGR_PIXEL(uint8_t,0xff, x)
				row += 3*sizeof(uint8_t);
			}
		}
		break;
	case BC_BGR8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_BGR_PIXEL(uint8_t,0xff, x)
				row += 4*sizeof(uint8_t);
			}
		}
		break;
	case BC_RGB_FLOAT:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(float,1.f, x)
				row += 3*sizeof(float);
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_RGB_PIXEL(float,1.f, x)
				row += 4*sizeof(float);
			}
		}
		break;
	case BC_YUV888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[2])
				row += 3*sizeof(uint8_t);
			}
		}
		break;

	case BC_YUVA8888:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; ++x ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[2])
				row += 4*sizeof(uint8_t);
			}
		}
		break;
	case BC_YUV420P: {
		uint8_t *yp = gui->data_frame->get_y();
		uint8_t *up = gui->data_frame->get_u();
		uint8_t *vp = gui->data_frame->get_v();
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *y_row = yp + y * dat_w;
			uint8_t *u_row = up + (y / 2) * (dat_w / 2);
			uint8_t *v_row = vp + (y / 2) * (dat_w / 2);
			for( int x=0; x<dat_w; x+=2 ) {
				PROCESS_YUV_PIXEL(x, *y_row, *u_row, *v_row);
				++y_row;
				PROCESS_YUV_PIXEL(x + 1, *y_row, *u_row, *v_row);
				++y_row;  ++u_row;  ++v_row;
			}
		}
		break; }
	case BC_YUV422:
		for( int y=pkg->row1; y<pkg->row2; ++y ) {
			uint8_t *row = rows[y];
			for( int x=0; x<dat_w; x+=2 ) {
				PROCESS_YUV_PIXEL(x, row[0], row[1], row[3]);
				PROCESS_YUV_PIXEL(x + 1, row[2], row[1], row[3]);
				row += 4*sizeof(uint8_t);
			}
		}
		break;

	default:
		printf("ScopeUnit::process_package %d: color_model=%d unrecognized\n",
			__LINE__, gui->data_frame->get_color_model());
		break;
	}
}


ScopeEngine::ScopeEngine(ScopeGUI *gui, int cpus)
 : LoadServer(cpus, cpus)
{
//printf("ScopeEngine::ScopeEngine %d cpus=%d\n", __LINE__, cpus);
	this->gui = gui;
}

ScopeEngine::~ScopeEngine()
{
}

void ScopeEngine::init_packages()
{
	int y = 0, h = gui->data_frame->get_h();
	for( int i=0,n=LoadServer::get_total_packages(); i<n; ) {
		ScopePackage *pkg = (ScopePackage*)get_package(i);
		pkg->row1 = y;
		pkg->row2 = y = (++i * h) / n;
	}

	for( int i=0,n=LoadServer::get_total_packages(); i<n; ++i ) {
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for( int j=0; j<HIST_SECTIONS; ++j )
			bzero(unit->bins[j], sizeof(int) * TOTAL_BINS);
	}
}


LoadClient* ScopeEngine::new_client()
{
	return new ScopeUnit(gui, this);
}

LoadPackage* ScopeEngine::new_package()
{
	return new ScopePackage;
}

void ScopeEngine::process()
{
	process_packages();

	for(int i = 0; i < HIST_SECTIONS; i++)
		bzero(gui->bins[i], sizeof(int) * TOTAL_BINS);

	for(int i=0,n=get_total_clients(); i<n; ++i ) {
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for( int j=0; j<HIST_SECTIONS; ++j ) {
			int *bp = gui->bins[j], *up = unit->bins[j];
			for( int k=TOTAL_BINS; --k>=0; ++bp,++up ) *bp += *up;
		}
	}
}


ScopeGUI::ScopeGUI(Theme *theme,
	int x, int y, int w, int h, int cpus)
 : PluginClientWindow(_(PROGRAM_NAME ": Scopes"),
	x, y, w, h, MIN_SCOPE_W, MIN_SCOPE_H, 1)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->theme = theme;
	this->cpus = cpus;
	reset();
}

ScopeGUI::ScopeGUI(PluginClient *plugin, int w, int h)
 : PluginClientWindow(plugin, w, h, MIN_SCOPE_W, MIN_SCOPE_H, 1)
{
	this->x = get_x();
	this->y = get_y();
	this->w = w;
	this->h = h;
	this->theme = plugin->get_theme();
	this->cpus = plugin->PluginClient::smp + 1;
	reset();
}

ScopeGUI::~ScopeGUI()
{
	delete waveform_vframe;
	delete vector_vframe;
	delete wheel_vframe;
	delete engine;
	delete box_blur;
	delete temp_frame;
	delete wave_slider;
	delete vect_slider;
	delete grat_image;
	delete overlay;
}

void ScopeGUI::reset()
{
	waveform_vframe = 0;
	vector_vframe = 0;
	wheel_vframe = 0;
	engine = 0;
	box_blur = 0;
	temp_frame = 0;
	wave_slider = 0;
	vect_slider = 0;
	grat_image = 0;
	overlay = 0;
	grat_idx = 0;
	settings = 0;

	output_frame = 0;
	data_frame = 0;
	frame_w = 1;
	use_smooth = 1;
	use_refresh = 0;
	use_release = 0;
	use_wave_gain = 5;
	use_vect_gain = 5;
	use_hist = 0;
	use_wave = 1;
	use_vector = 1;
	use_hist_parade = 0;
	use_wave_parade = 0;
	use_graticule = 0;
	waveform = 0;
	vectorscope = 0;
	histogram = 0;
	wave_w = wave_h = 0;
	vector_w = vector_h = 0;
	vector_cx = vector_cy = 0;
	radius = 0;
	text_color = GREEN;
	dark_color = (text_color>>2) & 0x3f3f3f;
	BC_Resources *resources = BC_WindowBase::get_resources();
	if( resources->bg_color == 0xffffff )
		text_color = dark_color;
}

void ScopeGUI::create_objects()
{
	if( use_hist && use_hist_parade )
		use_hist = 0;
	if( use_wave && use_wave_parade )
		use_wave = 0;
	if( !engine ) engine = new ScopeEngine(this, cpus);
	grat_idx = use_graticule; // last graticule
	use_graticule = 0;

	lock_window("ScopeGUI::create_objects");
	int margin = theme->widget_border;
	int x = margin, y = margin;
	add_subwindow(scope_menu = new ScopeMenu(this, x, y));
	scope_menu->create_objects();
	int x1 = x + scope_menu->get_w() + 2*margin;
	add_subwindow(settings = new ScopeSettings(this, x1, y));
	settings->create_objects();

	create_panels();
	update_toggles();
	show_window();
	update_scope();
	unlock_window();
}

void ScopeGUI::create_panels()
{
	calculate_sizes(get_w(), get_h());
	int slider_w = xS(100);
	if( (use_wave || use_wave_parade) ) {
		int px = wave_x + wave_w - slider_w - xS(5);
		int py = wave_y - ScopeGain::calculate_h() - yS(5);
		if( !waveform ) {
			add_subwindow(waveform = new ScopeWaveform(this,
				wave_x, wave_y, wave_w, wave_h));
			waveform->create_objects();
			wave_slider = new ScopeWaveSlider(this, px, py, slider_w);
			wave_slider->create_objects();
		}
		else {
			waveform->reposition_window(
				wave_x, wave_y, wave_w, wave_h);
			waveform->clear_box(0, 0, wave_w, wave_h);
			wave_slider->reposition_window(px, py);
		}
	}
	else if( !(use_wave || use_wave_parade) && waveform ) {
		delete waveform;     waveform = 0;
		delete wave_slider;  wave_slider = 0;
	}

	if( use_vector ) {
		int vx = vector_x + vector_w - slider_w - xS(5);
		int vy = vector_y - ScopeGain::calculate_h() - yS(5);
		if( !vectorscope ) {
			add_subwindow(vectorscope = new ScopeVectorscope(this,
				vector_x, vector_y, vector_w, vector_h));
			vectorscope->create_objects();
			vect_slider = new ScopeVectSlider(this, vx, vy, slider_w);
			vect_slider->create_objects();
		}
		else {
			vectorscope->reposition_window(
				vector_x, vector_y, vector_w, vector_h);
			vectorscope->clear_box(0, 0, vector_w, vector_h);
			vect_slider->reposition_window(vx, vy);
		}
	}
	else if( !use_vector && vectorscope ) {
		delete vectorscope;  vectorscope = 0;
		delete vect_slider;  vect_slider = 0;
	}

	if( (use_hist || use_hist_parade) ) {
		if( !histogram ) {
// printf("ScopeGUI::create_panels %d %d %d %d %d\n", __LINE__,
//  hist_x, hist_y, hist_w, hist_h);
			add_subwindow(histogram = new ScopeHistogram(this,
				hist_x, hist_y, hist_w, hist_h));
			histogram->create_objects();
		}
		else {
			histogram->reposition_window(
				hist_x, hist_y, hist_w, hist_h);
			histogram->clear_box(0, 0, hist_w, hist_h);
		}
	}
	else if( !(use_hist || use_hist_parade) ) {
		delete histogram;  histogram = 0;
	}

	allocate_vframes();
	clear_points(0);
	draw_overlays(1, 1, 0);
}

void ScopeGUI::clear_points(int flash)
{
	if( histogram )
		histogram->clear_point();
	if( waveform )
		waveform->clear_point();
	if( vectorscope )
		vectorscope->clear_point();
	if( histogram && flash )
		histogram->flash(0);
	if( waveform && flash )
		waveform->flash(0);
	if( vectorscope && flash )
		vectorscope->flash(0);
}

void ScopeGUI::toggle_event()
{
}

void ScopeGUI::calculate_sizes(int w, int h)
{
	int margin = theme->widget_border;
	int menu_h = scope_menu->get_h() + ScopeGain::calculate_h() + margin * 3;
	int text_w = get_text_width(SMALLFONT, "000") + margin * 2;
	int total_panels = ((use_hist || use_hist_parade) ? 1 : 0) +
		((use_wave || use_wave_parade) ? 1 : 0) +
		(use_vector ? 1 : 0);
	int x = margin;

	int panel_w = (w - margin) / (total_panels > 0 ? total_panels : 1);
// Vectorscope determines the size of everything else
// Always last panel
	vector_w = 0;
	if( use_vector ) {
		vector_x = w - panel_w + text_w;
		vector_w = w - margin - vector_x;
		vector_y = menu_h;
		vector_h = h - vector_y - margin;

		if( vector_w > vector_h ) {
			vector_w = vector_h;
			vector_x = w - margin - vector_w;
		}
		--total_panels;
		if(total_panels > 0)
			panel_w = (vector_x - text_w - margin) / total_panels;
	}
	vector_cx = vector_w / 2;
	vector_cy = vector_h / 2;
	radius = bmin(vector_cx, vector_cy);

// Histogram is always 1st panel
	if( use_hist || use_hist_parade ) {
		hist_x = x;
		hist_y = menu_h;
		hist_w = panel_w - margin;
		hist_h = h - hist_y - margin;

		--total_panels;
		x += panel_w;
	}

	if( use_wave || use_wave_parade ) {
		wave_x = x + text_w;
		wave_y = menu_h;
		wave_w = panel_w - margin - text_w;
		wave_h = h - wave_y - margin;
	}

}


void ScopeGUI::allocate_vframes()
{
	if(waveform_vframe) delete waveform_vframe;
	if(vector_vframe) delete vector_vframe;
	if(wheel_vframe) delete wheel_vframe;

	int w, h;
//printf("ScopeGUI::allocate_vframes %d %d %d %d %d\n", __LINE__,
// wave_w, wave_h, vector_w, vector_h);
	int xs16 = xS(16), ys16 = yS(16);
	w = MAX(wave_w, xs16);
	h = MAX(wave_h, ys16);
	waveform_vframe = new VFrame(w, h, BC_RGB888);
	w = MAX(vector_w, xs16);
	h = MAX(vector_h, ys16);
	vector_vframe = new VFrame(w, h, BC_RGBA8888);
	vector_vframe->set_clear_color(BLACK, 0xff);
	wheel_vframe = 0;
}


int ScopeGUI::resize_event(int w, int h)
{
	clear_box(0, 0, w, h);
	this->w = w;
	this->h = h;
	calculate_sizes(w, h);
	int margin = theme->widget_border;

	if( waveform ) {
		waveform->reposition_window(wave_x, wave_y, wave_w, wave_h);
		waveform->clear_box(0, 0, wave_w, wave_h);
		int px = wave_x + wave_w - wave_slider->get_w() - margin;
		int py = wave_y - wave_slider->get_h() - margin;
		wave_slider->reposition_window(px, py);
	}

	if( histogram ) {
		histogram->reposition_window(hist_x, hist_y, hist_w, hist_h);
		histogram->clear_box(0, 0, hist_w, hist_h);
	}

	if( vectorscope ) {
		vectorscope->reposition_window(vector_x, vector_y, vector_w, vector_h);
		vectorscope->clear_box(0, 0, vector_w, vector_h);
		int vx = vector_x + vector_w - vect_slider->get_w() - margin;
		int vy = vector_y - vect_slider->get_h() - margin;
		vect_slider->reposition_window(vx, vy);
	}

	allocate_vframes();
	clear_points(0);
	draw_overlays(1, 1, 1);
	update_scope();
	return 1;
}

int ScopeGUI::translation_event()
{
	x = get_x();
	y = get_y();
	PluginClientWindow::translation_event();
	return 0;
}


void ScopeGUI::draw_overlays(int overlays, int borders, int flush)
{
	int margin = theme->widget_border;
	if( overlays && borders ) {
		clear_box(0, 0, get_w(), get_h());
	}

	if( overlays ) {
		set_line_dashes(1);
		set_color(text_color);
		set_font(SMALLFONT);

		if( histogram && (use_hist || use_hist_parade) ) {
			histogram->draw_line(hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN), 0,
					hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN), hist_h);
			histogram->draw_line(hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN), 0,
					hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN), hist_h);
			set_line_dashes(0);
			histogram->draw_point();
			set_line_dashes(1);
			histogram->flash(0);
		}

// Waveform overlay
		if( waveform && (use_wave || use_wave_parade) ) {
			for( int i=0; i<=WAVEFORM_DIVISIONS; ++i ) {
				int y = wave_h * i / WAVEFORM_DIVISIONS;
				int text_y = y + wave_y + get_text_ascent(SMALLFONT) / 2;
				CLAMP(text_y, waveform->get_y() + get_text_ascent(SMALLFONT), waveform->get_y() + waveform->get_h() - 1);
				char string[BCTEXTLEN];
				sprintf(string, "%d", (int)lround((FLOAT_MAX -
					i * (FLOAT_MAX - FLOAT_MIN) / WAVEFORM_DIVISIONS) * 100));
				int text_x = wave_x - get_text_width(SMALLFONT, string) - margin;
				set_color(text_color);
				draw_text(text_x, text_y, string);
				CLAMP(y, 0, waveform->get_h() - 1);
				set_color(dark_color);
				waveform->draw_line(0, y, wave_w, y);
				waveform->draw_rectangle(0, 0, wave_w, wave_h);
			}

			int y1 = wave_h * 1.8 / WAVEFORM_DIVISIONS;
				int text_y1 = y1 + wave_y + get_text_ascent(SMALLFONT) / 2;
				CLAMP(text_y1, waveform->get_y() + get_text_ascent(SMALLFONT), waveform->get_y() + waveform->get_h() - 1);
			char string1[BCTEXTLEN];
			sprintf( string1, "%d",(int)lround((FLOAT_MAX  -
					1.8 * (FLOAT_MAX - FLOAT_MIN ) / WAVEFORM_DIVISIONS ) * 100) );
				int text_x1 = wave_x + get_text_width(SMALLFONT, string1) - margin +wave_w;
				set_color(text_color);
				draw_text(text_x1, text_y1, string1);
				CLAMP(y1, 0, waveform->get_h() - 1);
				set_color(dark_color);
				waveform->draw_line(0, y1, wave_w, y1);

			int y2 = wave_h * 10.4 / WAVEFORM_DIVISIONS;
				int text_y2 = y2 + wave_y + get_text_ascent(SMALLFONT) / 2;
				CLAMP(text_y2, waveform->get_y() + get_text_ascent(SMALLFONT), waveform->get_y() + waveform->get_h() - 1);
			char string2[BCTEXTLEN];
			sprintf( string2, "%d",(int)lround((FLOAT_MAX  -
					10.4 * (FLOAT_MAX - FLOAT_MIN ) / WAVEFORM_DIVISIONS) * 100) );
				set_color(text_color);
				draw_text(text_x1, text_y2, string2);
				CLAMP(y2, 0, waveform->get_h() - 1);
				set_color(dark_color);
				waveform->draw_line(0, y2, wave_w, y2);
			
			set_line_dashes(0);
			waveform->draw_point();
			set_line_dashes(1);
			waveform->flash(0);
		}
// Vectorscope overlay
		if( vectorscope && use_vector ) {
			set_color(text_color);
			vectorscope->draw_point();
			vectorscope->flash(0);
			set_line_dashes(1);
		}

		set_font(MEDIUMFONT);
		set_line_dashes(0);
	}

	if( borders ) {
		if( use_hist || use_hist_parade ) {
			draw_3d_border(hist_x - 2, hist_y - 2, hist_w + 4, hist_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
		if( use_wave || use_wave_parade ) {
			draw_3d_border(wave_x - 2, wave_y - 2, wave_w + 4, wave_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
		if( use_vector ) {
			draw_3d_border(vector_x - 2, vector_y - 2, vector_w + 4, vector_h + 4,
				get_bg_color(), BLACK, MDGREY, get_bg_color());
		}
	}

	flash(0);
	if(flush) this->flush();
}

void ScopeGUI::draw_scope()
{
	int graticule = use_vector < 0 ? grat_idx : 0;
	if( grat_image && use_graticule != graticule ) {
		delete grat_image;   grat_image = 0;
	}
	if( !grat_image && graticule > 0 )
		grat_image = VFramePng::vframe_png(grat_paths[graticule]);
	if( grat_image ) {
		if( !overlay )
			overlay = new OverlayFrame(1);
		int cx = vector_cx, cy = vector_cy, r = radius;
		int iw = grat_image->get_w(), ih = grat_image->get_h();
		overlay->overlay(vector_vframe, grat_image,
			0,0, iw, ih, cx-r,cy-r, cx+r, cy+r,
			1, TRANSFER_NORMAL, CUBIC_CUBIC);
	}
	use_graticule = graticule;
	vectorscope->draw_vframe(vector_vframe);
	if( use_vector > 0 ) {
		int margin = theme->widget_border;
		set_line_dashes(1);
		for( int i=1; i<=VECTORSCOPE_DIVISIONS; i+=2 ) {
			int y = vector_cy - radius * i / VECTORSCOPE_DIVISIONS;
			int text_y = y + vector_y + get_text_ascent(SMALLFONT) / 2;
			set_color(text_color);
			char string[BCTEXTLEN];
			sprintf(string, "%d",
				(int)((FLOAT_MAX / VECTORSCOPE_DIVISIONS * i) * 100));
			int text_x = vector_x - get_text_width(SMALLFONT, string) - margin;
			draw_text(text_x, text_y, string);
			int x = vector_cx - radius * i / VECTORSCOPE_DIVISIONS;
			int w = radius * i / VECTORSCOPE_DIVISIONS * 2;
			int h = radius * i / VECTORSCOPE_DIVISIONS * 2;
			if( i+2 > VECTORSCOPE_DIVISIONS )
				set_line_dashes(0);
			set_color(dark_color);
			vectorscope->draw_circle(x, y, w, h);
		}
		float th = TO_RAD(90 + 32.875);
		vectorscope->draw_radient(th, 0.1f, .75f, dark_color);
	}
}


void ScopeGUI::update_graticule(int idx)
{
	grat_idx = idx;
	update_scope();
	toggle_event();
}

void ScopeGUI::draw_colorwheel(VFrame *dst, int bg_color)
{
// downfactor radius to prevent extreme edge from showing behind graticule
	float cx = vector_cx, cy = vector_cy, rad = radius * 0.99;
        int color_model = dst->get_color_model();
        int bpp = BC_CModels::calculate_pixelsize(color_model);
	int bg_r = (bg_color>>16) & 0xff;
	int bg_g = (bg_color>> 8) & 0xff;
	int bg_b = (bg_color>> 0) & 0xff;
	int w = dst->get_w(), h = dst->get_h();
	unsigned char **rows = dst->get_rows();
	for( int y=0; y<h; ++y ) {
		unsigned char *row = rows[y];
		for( int x=0; x<w; ++x,row+=bpp ) {
			int dx = cx-x, dy = cy-y;
			float d = sqrt(dx*dx + dy*dy);
			float r, g, b;
			if( d < rad ) {
			        float h = TO_DEG(atan2(cx-x, cy-y));
				if( h < 0 ) h += 360;
				float s = d / rad, v = 255;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
			}
			else {
				 r = bg_r; g = bg_g; b = bg_b;
			}
			row[0] = r; row[1] = g; row[2] = b;  row[3] = 0xff;
		}
	}
}


void ScopeGUI::process(VFrame *output_frame)
{
	lock_window("ScopeGUI::process");
	this->output_frame = output_frame;
	int ow = output_frame->get_w(), oh = output_frame->get_h();
	if( use_smooth ) {
		VFrame::get_temp(temp_frame, ow, oh, BC_RGB161616);
		temp_frame->transfer_from(output_frame);
		if( !box_blur ) box_blur = new BoxBlur(cpus);
		box_blur->blur(temp_frame, temp_frame, 2, 2);
		data_frame = temp_frame;
	}
	else
		data_frame = output_frame;
	frame_w = data_frame->get_w();
	bzero(waveform_vframe->get_data(), waveform_vframe->get_data_size());
	if( use_vector > 0 )
		vector_vframe->clear_frame();
	else if( use_vector < 0 ) {
		if( wheel_vframe && (
		     wheel_vframe->get_w() != vector_w ||
		     wheel_vframe->get_h() != vector_h ) ) {
			delete wheel_vframe;  wheel_vframe = 0;
		}
		if( !wheel_vframe ) {
			wheel_vframe = new VFrame(vector_w, vector_h, BC_RGBA8888);
			draw_colorwheel(wheel_vframe, BLACK);
		}
		vector_vframe->copy_from(wheel_vframe);
	}
	engine->process();

	if( histogram )
		histogram->draw(0, 0);
	if( waveform )
		waveform->draw_vframe(waveform_vframe);
	if( vectorscope )
		draw_scope();

	draw_overlays(1, 0, 1);
	unlock_window();
}

void ScopeGUI::update_toggles()
{
	scope_menu->update_toggles();
	settings->update_toggles();
}

ScopePanel::ScopePanel(ScopeGUI *gui, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->gui = gui;
	is_dragging = 0;
}

void ScopePanel::create_objects()
{
	set_cursor(CROSS_CURSOR, 0, 0);
	clear_box(0, 0, get_w(), get_h());
}

void ScopePanel::update_point(int x, int y)
{
}

void ScopePanel::draw_point()
{
}

void ScopePanel::clear_point()
{
}

int ScopePanel::button_press_event()
{
	if( is_event_win() && cursor_inside() ) {
		gui->clear_points(1);
		is_dragging = 1;
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::cursor_motion_event()
{
	if( is_dragging ) {
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::button_release_event()
{
	if( is_dragging ) {
		is_dragging = 0;
		hide_tooltip();
		return 1;
	}
	return 0;
}


ScopeWaveform::ScopeWaveform(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
	drag_y = -1;
}

void ScopeWaveform::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	drag_y = y;
	int frame_x = x * gui->frame_w / get_w();

	if( gui->use_wave_parade ) {
		if( x > get_w() / 3 * 2 )
			frame_x = (x - get_w() / 3 * 2) * gui->frame_w / (get_w() / 3);
		else if( x > get_w() / 3 )
			frame_x = (x - get_w() / 3) * gui->frame_w / (get_w() / 3);
		else
			frame_x = x * gui->frame_w / (get_w() / 3);
	}

	float value = ((float)get_h() - y) / get_h() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;
	char string[BCTEXTLEN];
	sprintf(string, "X: %d Value: %.3f", frame_x, value);
	show_tooltip(string);

	draw_point();
	flash(1);
}

void ScopeWaveform::draw_point()
{
	if( drag_x >= 0 ) {
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(0, drag_y, get_w(), drag_y);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeWaveform::clear_point()
{
	draw_point();
	drag_x = -1;
	drag_y = -1;
}

ScopeVectorscope::ScopeVectorscope(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::clear_point()
{
// Hide it
	draw_point();
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::update_point(int x, int y)
{
// Hide it
	draw_point();
	int dx = x - gui->vector_cx, dy = y - gui->vector_cy;
	drag_radius = sqrt(dx*dx + dy*dy);
	if( drag_radius > gui->radius ) drag_radius = gui->radius;
	float saturation = (float)drag_radius / gui->radius * FLOAT_MAX;
	drag_angle = atan2(-dy, dx);
	float hue = TO_DEG(drag_angle) - 90;
	if( hue < 0 ) hue += 360;

	char string[BCTEXTLEN];
	sprintf(string, "Hue: %.3f Sat: %.3f", hue, saturation);
	show_tooltip(string);

// Show it
	draw_point();
	flash(1);
}

void ScopeVectorscope::draw_point()
{
	set_inverse();
	draw_point(drag_angle, drag_radius, RED);
	set_opaque();
}

void ScopeVectorscope::draw_point(float th, float r, int color)
{
	if( r > 0 ) {
		set_color(color);
		set_line_width(2);
		int cx = gui->vector_cx, cy = gui->vector_cy;
		draw_circle(cx-r, cy-r, r*2, r*2);
		set_line_width(1);
		float radius = gui->radius;
		draw_radient(th, 0.f, drag_radius/radius, color);
	}
}

void ScopeVectorscope::draw_radient(float th, float r1, float r2, int color)
{
	if( r2 > 0 && r2 >= r1 ) {
		set_color(color);
		set_line_width(2);
		int r = gui->radius;
		double cth = r*cos(th), sth = -r*sin(th);
		int cx = gui->vector_cx, cy = gui->vector_cy;
		draw_line(cx+r1*cth, cy+r1*sth, cx+r2*cth, cy+r2*sth);
		set_line_width(1);
	}
}


ScopeHistogram::ScopeHistogram(ScopeGUI *gui,
		int x, int y, int w, int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
}

void ScopeHistogram::clear_point()
{
// Hide it
	draw_point();
	drag_x = -1;
}

void ScopeHistogram::draw_point()
{
	if( drag_x >= 0 ) {
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeHistogram::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	float value = (float)x / get_w() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;

	char string[BCTEXTLEN];
	sprintf(string, "Value: %.3f", value);
	show_tooltip(string);

	draw_point();
	flash(1);
}



void ScopeHistogram::draw_mode(int mode, int color, int y, int h)
{
// Highest of all bins
	int normalize = 1, w = get_w();
	int *bin = gui->bins[mode], v;
	for( int i=0; i<TOTAL_BINS; ++i )
		if( (v=bin[i]) > normalize ) normalize = v;
	double norm = normalize>1 ? log(normalize) : 1e-4;
	double vnorm = h / norm;
	set_color(color);
	for( int x=0; x<w; ++x ) {
		int accum_start = (int)(x * TOTAL_BINS / w);
		int accum_end = (int)((x + 1) * TOTAL_BINS / w);
		CLAMP(accum_start, 0, TOTAL_BINS);
		CLAMP(accum_end, 0, TOTAL_BINS);
		int max = 0;
		for(int i=accum_start; i<accum_end; ++i )
			if( (v=bin[i]) > max ) max = v;
//		max = max * h / normalize;
		max = log(max) * vnorm;
		draw_line(x,y+h - max, x,y+h);
	}
}
void ScopeHistogram::draw(int flash, int flush)
{
	clear_box(0, 0, get_w(), get_h());
	if( gui->use_hist_parade ) {
		draw_mode(0, 0xff0000, 0, get_h() / 3);
		draw_mode(1, 0x00ff00, get_h() / 3, get_h() / 3);
		draw_mode(2, 0x0000ff, get_h() / 3 * 2, get_h() / 3);
	}
	else {
		draw_mode(3, LTGREY, 0, get_h());
	}

	if(flash) this->flash(0);
	if(flush) this->flush();
}


ScopeScopesOn::ScopeScopesOn(ScopeMenu *scope_menu, const char *text, int id)
 : BC_MenuItem(text)
{
	this->scope_menu = scope_menu;
	this->id = id;
}

int ScopeScopesOn::handle_event()
{
	int v = get_checked() ? 0 : 1;
	set_checked(v);
	ScopeGUI *gui = scope_menu->gui;
	switch( id ) {
	case SCOPE_HISTOGRAM:
		gui->use_hist = v;
		if( v ) gui->use_hist_parade = 0;
		break;
	case SCOPE_HISTOGRAM_RGB:
		gui->use_hist_parade = v;
		if( v ) gui->use_hist = 0;
		break;
	case SCOPE_WAVEFORM:
		gui->use_wave = v;
		if( v ) gui->use_wave_parade = 0;
		break;
	case SCOPE_WAVEFORM_RGB:
		gui->use_wave_parade = v;
		if( v ) gui->use_wave = 0;
		break;
	case SCOPE_WAVEFORM_PLY:
		gui->use_wave_parade = -v;
		if( v ) gui->use_wave = 0;
		break;
	case SCOPE_VECTORSCOPE:
		gui->use_vector = v;
		break;
	case SCOPE_VECTORWHEEL:
		gui->use_vector = -v;
		break;
	}
	gui->toggle_event();
	gui->update_toggles();
	gui->create_panels();
	gui->update_scope();
	gui->show_window();
	return 1;
}

ScopeMenu::ScopeMenu(ScopeGUI *gui, int x, int y)
 : BC_PopupMenu(x, y, xS(110), _("Scopes"))
{
	this->gui = gui;
}

void ScopeMenu::create_objects()
{
	add_item(hist_on =
		new ScopeScopesOn(this, _("Histogram"), SCOPE_HISTOGRAM));
	add_item(hist_rgb_on =
		new ScopeScopesOn(this, _("Histogram RGB"), SCOPE_HISTOGRAM_RGB));
	add_item(new BC_MenuItem("-"));
	add_item(wave_on =
		new ScopeScopesOn(this, _("Waveform"), SCOPE_WAVEFORM));
	add_item(wave_rgb_on =
		new ScopeScopesOn(this, _("Waveform RGB"), SCOPE_WAVEFORM_RGB));
	add_item(wave_ply_on =
		new ScopeScopesOn(this, _("Waveform ply"), SCOPE_WAVEFORM_PLY));
	add_item(new BC_MenuItem("-"));
	add_item(vect_on =
		new ScopeScopesOn(this, _("Vectorscope"), SCOPE_VECTORSCOPE));
	add_item(vect_wheel =
		new ScopeScopesOn(this, _("VectorWheel"), SCOPE_VECTORWHEEL));
}

void ScopeMenu::update_toggles()
{
	hist_on->set_checked(gui->use_hist);
	hist_rgb_on->set_checked(gui->use_hist_parade);
	wave_on->set_checked(gui->use_wave);
	wave_rgb_on->set_checked(gui->use_wave_parade>0);
	wave_ply_on->set_checked(gui->use_wave_parade<0);
	vect_on->set_checked(gui->use_vector>0);
	vect_wheel->set_checked(gui->use_vector<0);
}


ScopeSettingOn::ScopeSettingOn(ScopeSettings *settings, const char *text, int id)
 : BC_MenuItem(text)
{
	this->settings = settings;
	this->id = id;
}

int ScopeSettingOn::handle_event()
{
	int v = get_checked() ? 0 : 1;
	set_checked(v);
	ScopeGUI *gui = settings->gui;
	switch( id ) {
	case SCOPE_SMOOTH:
		gui->use_smooth = v;
		break;
	case SCOPE_REFRESH:
		gui->use_refresh = v;
		gui->use_release = 0;
		break;
	case SCOPE_RELEASE:
		gui->use_release = v;
		gui->use_refresh = 0;
	}
	gui->toggle_event();
	gui->update_toggles();
	gui->update_scope();
	gui->show_window();
	return 1;
}

ScopeSettings::ScopeSettings(ScopeGUI *gui, int x, int y)
 : BC_PopupMenu(x, y, xS(150), _("Settings"))
{
	this->gui = gui;
	refresh_on = 0;
	release_on = 0;
}

void ScopeSettings::create_objects()
{
	add_item(smooth_on =
		new ScopeSettingOn(this, _("Smooth"), SCOPE_SMOOTH));
	smooth_on->set_checked(gui->use_smooth);
	if( gui->use_refresh >= 0 ) {
		add_item(refresh_on =
			new ScopeSettingOn(this, _("Refresh on Stop"), SCOPE_REFRESH));
		add_item(release_on =
			new ScopeSettingOn(this, _("Refresh on Release"), SCOPE_RELEASE));
		refresh_on->set_checked(gui->use_refresh);
		release_on->set_checked(gui->use_release);
	}
	add_item(new BC_MenuItem(_("-VectorWheel Grids-")));

	gui->grat_paths.remove_all_objects();
	ScopeGratItem *item;
	add_item(item = new ScopeGratItem(this, _("None"), 0));
	if( item->idx == gui->grat_idx ) item->set_checked(1);
	gui->grat_paths.append(0);
	FileSystem fs;
	fs.set_filter("[*.png]");
	char scope_path[BCTEXTLEN];
	sprintf(scope_path, "%s%s", File::get_plugin_path(), SCOPE_SEARCHPATH);
	fs.update(scope_path);
	for( int i=0; i<fs.total_files(); ++i ) {
		FileItem *file_item = fs.get_entry(i);
		if( file_item->get_is_dir() ) continue;
		strcpy(scope_path, file_item->get_name());
		char *cp = strrchr(scope_path, '.');
		if( cp ) *cp = 0;
		add_item(item = new ScopeGratItem(this, scope_path, gui->grat_paths.size()));
		if( item->idx == gui->grat_idx ) item->set_checked(1);
		gui->grat_paths.append(cstrdup(file_item->get_path()));
	}
}

void ScopeSettings::update_toggles()
{
	if( refresh_on )
		refresh_on->set_checked(gui->use_refresh);
	if( release_on )
		release_on->set_checked(gui->use_release);
}

ScopeGratItem::ScopeGratItem(ScopeSettings *settings, const char *text, int idx)
 : BC_MenuItem(text)
{
	this->settings = settings;
	this->idx = idx;
}

int ScopeGratItem::handle_event()
{
	for( int i=0,n=settings->total_items(); i<n; ++i ) {
		ScopeGratItem *item = (ScopeGratItem *)settings->get_item(i);
		item->set_checked(item->idx == idx);
	}	
	settings->gui->update_graticule(idx);
	return 1;
}


ScopeGainReset::ScopeGainReset(ScopeGain *gain, int x, int y)
 : BC_Button(x, y, gain->gui->theme->get_image_set("reset_button"))
{
	this->gain = gain;
}

int ScopeGainReset::calculate_w(BC_Theme *theme)
{
	VFrame *vfrm = *theme->get_image_set("reset_button");
	return vfrm->get_w();
}

int ScopeGainReset::handle_event()
{
	gain->slider->update(5);
	return gain->handle_event();
}

ScopeGainSlider::ScopeGainSlider(ScopeGain *gain, int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, 1, 9, *gain->value)
{
	this->gain = gain;
}

int ScopeGainSlider::handle_event()
{
	return gain->handle_event();
}

ScopeGain::ScopeGain(ScopeGUI *gui, int x, int y, int w, int *value)
{
	this->gui = gui;
	this->x = x;
	this->y = y;
	this->w = w;
	this->value = value;

	slider = 0;
	reset = 0;
}
ScopeGain::~ScopeGain()
{
	delete reset;
	delete slider;
}

int ScopeGain::calculate_h()
{
	return BC_ISlider::get_span(0);
}

void ScopeGain::create_objects()
{
	reset_w = ScopeGainReset::calculate_w(gui->theme);
	gui->add_subwindow(slider = new ScopeGainSlider(this, x, y, w-reset_w-xS(5)));
	gui->add_subwindow(reset = new ScopeGainReset(this, x+w-reset_w, y));
}

int ScopeGain::handle_event()
{
	*value = slider->get_value();
	gui->update_scope();
	gui->toggle_event();
	return 1;
}

void ScopeGain::reposition_window(int x, int y)
{
	this->x = x;  this->y = y;
	slider->reposition_window(x, y);
	reset->reposition_window(x+w-reset_w, y);
}

ScopeWaveSlider::ScopeWaveSlider(ScopeGUI *gui, int x, int y, int w)
 : ScopeGain(gui, x, y, w, &gui->use_wave_gain)
{
}

ScopeVectSlider::ScopeVectSlider(ScopeGUI *gui, int x, int y, int w)
 : ScopeGain(gui, x, y, w, &gui->use_vect_gain)
{
}

