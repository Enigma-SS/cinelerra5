/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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


// Objects for compressors
#include "clip.h"
#include "compressortools.h"
#include "cursors.h"
#include "filexml.h"
#include "language.h"
#include "pluginclient.h"
#include "samples.h"
#include "theme.h"
#include <string.h>


BandConfig::BandConfig()
{
	reset();
}

BandConfig::~BandConfig()
{
	
}

void BandConfig::save_data(FileXML *xml, int number, int do_multiband)
{
	xml->tag.set_title("COMPRESSORBAND");
	if( do_multiband ) {
		xml->tag.set_property("NUMBER", number);
		xml->tag.set_property("FREQ", freq);
		xml->tag.set_property("BYPASS", bypass);
		xml->tag.set_property("SOLO", solo);
		xml->tag.set_property("ATTACK_LEN", attack_len);
		xml->tag.set_property("RELEASE_LEN", release_len);
		xml->tag.set_property("MKUP_GAIN", mkup_gain);
	}
	xml->append_tag();
	xml->append_newline();

	for( int i = 0; i < levels.total; i++ ) {
		xml->tag.set_title("LEVEL");
		xml->tag.set_property("X", levels[i].x);
		xml->tag.set_property("Y", levels[i].y);
		xml->append_tag();
		xml->append_newline();
	}

	xml->tag.set_title("/COMPRESSORBAND");
	xml->append_tag();
	xml->append_newline();
}

void BandConfig::read_data(FileXML *xml, int do_multiband)
{
	if( do_multiband ) {
		freq = xml->tag.get_property("FREQ", freq);
		bypass = xml->tag.get_property("BYPASS", bypass);
		solo = xml->tag.get_property("SOLO", solo);
		attack_len = xml->tag.get_property("ATTACK_LEN", attack_len);
		release_len = xml->tag.get_property("RELEASE_LEN", release_len);
		mkup_gain = xml->tag.get_property("MKUP_GAIN", mkup_gain);
	}

	levels.remove_all();
	int result = 0;
	while( !result ) {
		result = xml->read_tag();
		if( !result ) {
			if( xml->tag.title_is("LEVEL") ) {
				double x = xml->tag.get_property("X", (double)0);
				double y = xml->tag.get_property("Y", (double)0);
				compressor_point_t point = { x, y };

				levels.append(point);
			}
			else
			if( xml->tag.title_is("/COMPRESSORBAND") ) {
				break;
			}
		}
	}
}

void BandConfig::copy_from(BandConfig *src)
{
	levels.remove_all();
	for( int i = 0; i < src->levels.total; i++ ) {
		levels.append(src->levels[i]);
	}

//	readahead_len = src->readahead_len;
	attack_len = src->attack_len;
	release_len = src->release_len;
	freq = src->freq;
	solo = src->solo;
	bypass = src->bypass;
	mkup_gain = src->mkup_gain;
}

int BandConfig::equiv(BandConfig *src)
{
	if( levels.total != src->levels.total ||
		solo != src->solo ||
		bypass != src->bypass ||
		freq != src->freq ||
		mkup_gain != src->mkup_gain ||
//		!EQUIV(readahead_len, src->readahead_len) ||
		!EQUIV(attack_len, src->attack_len) ||
		!EQUIV(release_len, src->release_len) ) {
		return 0;
	}

	for( int i = 0; i < levels.total && i < src->levels.total; i++ ) {
		compressor_point_t *this_level = &levels[i];
		compressor_point_t *that_level = &src->levels[i];
		if( !EQUIV(this_level->x, that_level->x) ||
			!EQUIV(this_level->y, that_level->y) ) {
			return 0;
		}
	}
	
	return 1;
}

void BandConfig::boundaries(CompressorConfigBase *base)
{
	for( int i = 0; i < levels.size(); i++ ) {
		compressor_point_t *level = &levels[i];
		if( level->x < base->min_db ) level->x = base->min_db;
		if( level->y < base->min_db ) level->y = base->min_db;
		if( level->x > base->max_db ) level->x = base->max_db;
		if( level->y > base->max_db ) level->y = base->max_db;
	}
}

void BandConfig::reset()
{
	freq = 0;
	solo = 0;
	bypass = 0;
//	readahead_len = 1.0;
	attack_len = 1.0;
	release_len = 1.0;
	mkup_gain = 0.0;
	levels.remove_all();
}


CompressorConfigBase::CompressorConfigBase(int total_bands)
{
	this->total_bands = total_bands;
	bands = new BandConfig[total_bands];
	min_db = -78.0;
	max_db = 6.0;
	min_value = DB::fromdb(min_db) + 0.001;
//	min_x = min_db;  max_x = 0;
//	min_y = min_db;  max_y = 0;
	reset_base();
}


CompressorConfigBase::~CompressorConfigBase()
{
	delete [] bands;
}

void CompressorConfigBase::reset_base()
{
	input = CompressorConfigBase::TRIGGER;
	trigger = 0;
	smoothing_only = 0;
	for( int i=0; i<total_bands; ++i )
		bands[i].freq = Freq::tofreq((i+1) * TOTALFREQS / total_bands);
	current_band = 0;
}

void CompressorConfigBase::reset_bands()
{
	for( int i=0; i<total_bands; ++i )
		bands[i].reset();
}


void CompressorConfigBase::boundaries()
{
	for( int i=0; i<total_bands; ++i ) {
		bands[i].boundaries(this);
	}
}


void CompressorConfigBase::copy_from(CompressorConfigBase &that)
{
//	min_x = that.min_x;  max_x = that.max_x;
//	min_y = that.min_y;  max_y = that.max_y;
	trigger = that.trigger;
	input = that.input;
	smoothing_only = that.smoothing_only;

	for( int i=0; i<total_bands; ++i ) {
		BandConfig *dst = &bands[i];
		BandConfig *src = &that.bands[i];
		dst->copy_from(src);
	}
}


int CompressorConfigBase::equivalent(CompressorConfigBase &that)
{
	for( int i=0; i<total_bands; ++i ) {
		if( !bands[i].equiv(&that.bands[i]) )
			return 0;
	}

	return trigger == that.trigger &&
		input == that.input &&
		smoothing_only == that.smoothing_only;
}

double CompressorConfigBase::get_y(int band, int i)
{
	ArrayList<compressor_point_t> &levels = bands[band].levels;
	int sz = levels.size();
	if( !sz ) return 1.;
	if( i >= sz ) i = sz-1;
	return levels[i].y;
}

double CompressorConfigBase::get_x(int band, int i)
{
	ArrayList<compressor_point_t> &levels = bands[band].levels;
	int sz = levels.size();
	if( !sz ) return 0.;
	if( i >= sz ) i = sz-1;
	return levels[i].x;
}

double CompressorConfigBase::calculate_db(int band, double x)
{
	ArrayList<compressor_point_t> &levels = bands[band].levels;
	int sz = levels.size();
	if( !sz ) return x;
	compressor_point_t &point0 = levels[0];
	double px0 = point0.x, py0 = point0.y, dx0 = x - px0;
// before 1st point, use 1:1 gain
	double ret = py0 + dx0;
	if( sz > 1 ) {
// find point <= x
		int k = sz;
		while( --k >= 0 && levels[k].x > x );
		if( k >= 0 ) {
			compressor_point_t &curr = levels[k];
			double cx = curr.x, cy = curr.y, dx = x - cx;
// between 2 points.  Use slope between 2 points
// the last point.  Use slope of last 2 points
			if( k >= sz-1 ) --k;
			compressor_point_t &prev = levels[k+0];
			compressor_point_t &next = levels[k+1];
			double px = prev.x, py = prev.y;
			double nx = next.x, ny = next.y;
			ret = cy + dx * (ny - py) / (nx - px);
		}
	}
	else
// the only point.  Use slope from min_db
		ret = py0 + dx0 * (py0 - min_db) / (px0 - min_db);

	ret += bands[band].mkup_gain;
	return ret;
}


int CompressorConfigBase::set_point(int band, double x, double y)
{
	ArrayList<compressor_point_t> &levels = bands[band].levels;
	int k = levels.size(), ret = k;
	while( --k >= 0 && levels[k].x >= x ) ret = k;
	compressor_point_t new_point = { x, y };
	levels.insert(new_point, ret);
	return ret;
}

void CompressorConfigBase::remove_point(int band, int i)
{
	ArrayList<compressor_point_t> &levels = bands[band].levels;
	levels.remove_number(i);
}


double CompressorConfigBase::calculate_output(int band, double x)
{
	double x_db = DB::todb(x);
	return DB::fromdb(calculate_db(band, x_db));
}


double CompressorConfigBase::calculate_gain(int band, double input_linear)
{
	double output_linear = calculate_output(band, input_linear);
// output is below minimum.  Mute it
	return output_linear < min_value ? 0. :
// input is below minimum.  Don't change it.
		fabs(input_linear - 0.0) < min_value ? 1. :
// gain
		output_linear / input_linear;
}


CompressorCanvasBase::CompressorCanvasBase(CompressorConfigBase *config, 
		PluginClient *plugin, PluginClientWindow *window, 
		int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->config = config;
	this->plugin = plugin;
	this->window = window;
	current_operation = NONE;

	graph_x = 0;
	graph_y = 0;
	graph_w = w - graph_x;
	graph_h = h - graph_y;
	subdivisions = 6;
	divisions = (int)(config->max_db - config->min_db) / subdivisions;
}

CompressorCanvasBase::~CompressorCanvasBase()
{
}


void CompressorCanvasBase::create_objects()
{
	add_subwindow(menu = new CompressorPopup(this));
	menu->create_objects();

	set_cursor(CROSS_CURSOR, 0, 0);
	draw_scales();
	update();
}

void CompressorCanvasBase::draw_scales()
{
	int yfudge = yS(10);
	window->set_font(SMALLFONT);
	window->set_color(get_resources()->default_text_color);

// output divisions
	for( int i=0; i<=divisions; ++i ) {
		int y = get_y() + yfudge + graph_y + graph_h * i / divisions;
		int x = get_x();
		char string[BCTEXTLEN];
		sprintf(string, "%.0f", config->max_db - 
			(float)i / divisions * (config->max_db - config->min_db));
		int text_w = get_text_width(SMALLFONT, string);
		if( i >= divisions ) y -= yfudge;
		window->draw_text(x-xS(10) - text_w, y, string);
		if( i >= divisions ) break;
		
		int y1 = get_y() + graph_y + graph_h * i / divisions;
		int y2 = get_y() + graph_y + graph_h * (i + 1) / divisions;
		int x1 = get_x() - xS(10), x2 = get_x() - xS(5);
		for( int j=0; j<subdivisions; ++j,x1=x2 ) {
			y = y1 + (y2 - y1) * j / subdivisions;
			window->draw_line(x, y, x1, y);
		}
	}

// input divisions
	for( int i=0; i<=divisions; ++i ) {
		int y = get_y() + get_h();
		int x = get_x() + graph_x + graph_w * i / divisions;
		int y0 = y + window->get_text_ascent(SMALLFONT);
		char string[BCTEXTLEN];
		sprintf(string, "%.0f", (float)i / divisions * 
				(config->max_db - config->min_db) + config->min_db);
		int text_w = get_text_width(SMALLFONT, string);
		window->draw_text(x - text_w, y0 + yS(10), string);
		if( i >= divisions ) break;

		int x1 = get_x() + graph_x + graph_w * i / divisions;
		int x2 = get_x() + graph_x + graph_w * (i + 1) / divisions;
		int y1 = y + yS(10), y2 = y + yS(5);
		for( int j=0; j<subdivisions; ++j,y1=y2 ) {
			x = x1 + (x2 - x1) * j / subdivisions;
			window->draw_line(x, y, x, y1);
		}
	}


}

#define POINT_W xS(10)

// get Y from X
int CompressorCanvasBase::x_to_y(int band, int x)
{
	double min_db = config->min_db, max_db = config->max_db;
	double rng_db = max_db - min_db;
	double x_db = min_db + (double)x / graph_w * rng_db;
	double y_db = config->calculate_db(band, x_db);
	int y = graph_y + graph_h - (int)((y_db - min_db) * graph_h / rng_db);
	return y;
}

// get X from DB
int CompressorCanvasBase::db_to_x(double db)
{
	double min_db = config->min_db, max_db = config->max_db;
	double rng_db = max_db - min_db;
	int x = graph_x + (double)(db - min_db) * graph_w / rng_db;
	return x;
}

// get Y from DB
int CompressorCanvasBase::db_to_y(double db)
{
	double min_db = config->min_db, max_db = config->max_db;
	double rng_db = max_db - min_db;
	int y = graph_y + graph_h - (int)((db - min_db) * graph_h / rng_db); 
	return y;
}


double CompressorCanvasBase::x_to_db(int x)
{
	CLAMP(x, 0, get_w());
	double min_db = config->min_db, max_db = config->max_db;
	double rng_db = max_db - min_db;
	double x_db = (double)(x - graph_x) * rng_db / graph_w + min_db;
	CLAMP(x_db, min_db, max_db);
	return x_db;
}

double CompressorCanvasBase::y_to_db(int y)
{
	CLAMP(y, 0, get_h());
	double min_db = config->min_db, max_db = config->max_db;
	double rng_db = max_db - min_db;
	double y_db = (double)(graph_y - y) * rng_db / graph_h + max_db;
	CLAMP(y_db, min_db, max_db);
	return y_db;
}



void CompressorCanvasBase::update()
{
// headroom boxes
	set_color(window->get_bg_color());
	draw_box(graph_x, 0, get_w(), graph_y);
	draw_box(graph_w, graph_y, get_w() - graph_w, get_h() - graph_y);
//	 const int checker_w = DP(10);
//	 const int checker_h = DP(10);
//	 set_color(MDGREY);
//	 for( int i = 0; i < get_h(); i += checker_h )
//	 {
//		 for( int j = (i % 2) * checker_w; j < get_w(); j += checker_w * 2 )
//		 {
//			 if( !(i >= graph_y && 
//				 i + checker_h < graph_y + graph_h &&
//				 j >= graph_x &&
//				 j + checker_w < graph_x + graph_w) )
//			 {
//				 draw_box(j, i, checker_w, checker_h);
//			 }
//		 }
//	 }

// canvas boxes
	set_color(plugin->get_theme()->graph_bg_color);
	draw_box(graph_x, graph_y, graph_w, graph_h);

// graph border
	draw_3d_border(0, 0, get_w(), get_h(), window->get_bg_color(),
		plugin->get_theme()->graph_border1_color,
		plugin->get_theme()->graph_border2_color, 
		window->get_bg_color());

	set_line_dashes(1);
	set_color(plugin->get_theme()->graph_grid_color);
	
	for( int i = 1; i < divisions; i++ ) {
		int y = graph_y + graph_h * i / divisions;
		draw_line(graph_x, y, graph_x + graph_w, y);
// 0db 
		if( i == 1 ) {
			draw_line(graph_x, y + 1, graph_x + graph_w, y + 1);
		}
		
		int x = graph_x + graph_w * i / divisions;
		draw_line(x, graph_y, x, graph_y + graph_h);
// 0db 
		if( i == divisions - 1 ) {
			draw_line(x + 1, graph_y, x + 1, graph_y + graph_h);
		}
	}
	set_line_dashes(0);


	set_font(MEDIUMFONT);
	int border = plugin->get_theme()->widget_border; 
	draw_text(border, get_h() / 2, _("Output"));
	int tx = get_w() / 2 - get_text_width(MEDIUMFONT, _("Input")) / 2;
	int ty = get_h() - get_text_height(MEDIUMFONT, _("Input")) - border;
	draw_text(tx, ty, _("Input"));

	for( int pass = 0; pass < 2; pass++ ) {
		for( int band = 0; band < config->total_bands; band++ ) {
// draw the active band on top of the others
			if( band == config->current_band && pass == 0 ||
				band != config->current_band && pass == 1 ) {
				continue;
			}

			if( band == config->current_band ) {
				set_color(plugin->get_theme()->graph_active_color);
				set_line_width(2);
			}
			else {
				set_color(plugin->get_theme()->graph_inactive_color);
				set_line_width(1);
			}

// draw the line
			int x1 = graph_x, y1 = x_to_y(band, x1);
			for( int i=0; ++i <= graph_w; ) {
				int x2 = x1+1, y2 = x_to_y(band, x2);
				draw_line(x1,y1, x2,y2);
				x1 = x2;  y1 = y2;
			}

			set_line_width(1);

// draw the points
			if( band == config->current_band ) {
				ArrayList<compressor_point_t> &levels = config->bands[band].levels;
				double mkup_gain = config->bands[band].mkup_gain;
				for( int i = 0; i < levels.size(); i++ ) {
					double x_db = config->get_x(band, i);
					double y_db = config->get_y(band, i) + mkup_gain;
					int x = db_to_x(x_db);
					int y = db_to_y(y_db);

					if( i == current_point ) {
						draw_box(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
					}
					else {
						draw_rectangle(x - POINT_W / 2, y - POINT_W / 2, POINT_W, POINT_W);
					}
				}
			}
		}
	}
	
	flash();
}

int CompressorCanvasBase::button_press_event()
{
// Check existing points
	if( is_event_win() && cursor_inside() ) {
		if( get_buttonpress() == 3 ) {
			menu->activate_menu();
			return 1;
		}
		int x = get_cursor_x(), y = get_cursor_y();
		int band = config->current_band;
		ArrayList<compressor_point_t> &levels = config->bands[band].levels;
		double mkup_gain = config->bands[band].mkup_gain;
		for( int i=0; i<levels.size(); ++i ) {
			double x_db = config->get_x(config->current_band, i);
			double y_db = config->get_y(config->current_band, i) + mkup_gain;

			int px = db_to_x(x_db);
			int py = db_to_y(y_db);

			if( x <= px + POINT_W / 2 && x >= px - POINT_W / 2 &&
			    y <= py + POINT_W / 2 && y >= py - POINT_W / 2 ) {
				current_operation = DRAG;
				current_point = i;
				return 1;
			}
		}

		if( x >= graph_x && x < graph_x + graph_w &&
		    y >= graph_y && y < graph_y + graph_h ) {
// Create new point
			double x_db = x_to_db(x);
			double y_db = y_to_db(y) + mkup_gain;

			current_point = config->set_point(config->current_band, x_db, y_db);
			current_operation = DRAG;
			update_window();
			plugin->send_configure_change();
			return 1;
		}
	}
	return 0;
}

int CompressorCanvasBase::button_release_event()
{
	int band = config->current_band;
	ArrayList<compressor_point_t> &levels = config->bands[band].levels;

	if( current_operation == DRAG ) {
		if( current_point > 0 ) {
			if( levels[current_point].x < levels[current_point-1].x ) {
				config->remove_point(config->current_band, current_point);
			}
		}

		if( current_point < levels.size()-1 ) {
			if( levels[current_point].x >= levels[current_point + 1].x ) {
				config->remove_point(config->current_band, current_point);
			}
		}

		update_window();
		plugin->send_configure_change();
		current_operation = NONE;
		return 1;
	}

	return 0;
}

int CompressorCanvasBase::cursor_motion_event()
{
	int band = config->current_band;
	ArrayList<compressor_point_t> &levels = config->bands[band].levels;
	double mkup_gain = config->bands[band].mkup_gain;
	int x = get_cursor_x(), y = get_cursor_y();

	if( current_operation == DRAG ) {
		double x_db = x_to_db(x);
		double y_db = y_to_db(y) - mkup_gain;
		
		if( shift_down() ) {
			const int grid_precision = 6;
			x_db = config->max_db + (double)(grid_precision *
				(int)((x_db - config->max_db) / grid_precision - 0.5));
			y_db = config->max_db + (double)(grid_precision *
				(int)((y_db - config->max_db) / grid_precision - 0.5));
		}
		
		
//printf("CompressorCanvasBase::cursor_motion_event %d x=%d y=%d x_db=%f y_db=%f\n", 
//__LINE__, x, y, x_db, y_db);
		levels[current_point].x = x_db;
		levels[current_point].y = y_db;
		update_window();
		plugin->send_configure_change();
		return 1;
	}
	else
// Change cursor over points
	if( is_event_win() && cursor_inside() ) {
		int new_cursor = CROSS_CURSOR;

		for( int i = 0; i < levels.size(); i++ ) {
			double x_db = config->get_x(config->current_band, i);
			double y_db = config->get_y(config->current_band, i) + mkup_gain;
			int px = db_to_x(x_db);
			int py = db_to_y(y_db);

			if( x <= px + POINT_W / 2 && x >= px - POINT_W / 2 &&
				y <= py + POINT_W / 2 && y >= py - POINT_W / 2 ) {
				new_cursor = UPRIGHT_ARROW_CURSOR;
				break;
			}
		}

// out of active area
		if( x >= graph_x + graph_w || y < graph_y ) {
			new_cursor = ARROW_CURSOR;
		}
		if( new_cursor != get_cursor() ) {
			set_cursor(new_cursor, 0, 1);
		}
	}
	return 0;
}


void CompressorCanvasBase::update_window()
{
	printf("CompressorCanvasBase::update_window %d empty\n", __LINE__);
}


int CompressorCanvasBase::is_dragging()
{
	return current_operation == DRAG;
}


CompressorClientFrame::CompressorClientFrame()
{
	type = -1;
	band = 0;
}
CompressorClientFrame::~CompressorClientFrame()
{
}

CompressorFreqFrame::CompressorFreqFrame()
{
	type = FREQ_COMPRESSORFRAME;
	data = 0;      data_size = 0;
	freq_max = 0;  time_max = 0;
	nyquist = 0;
}
CompressorFreqFrame::~CompressorFreqFrame()
{
	delete [] data;
}

CompressorGainFrame::CompressorGainFrame()
{
	type = GAIN_COMPRESSORFRAME;
	gain = 0;
	level = 0;
}
CompressorGainFrame::~CompressorGainFrame()
{
}

CompressorEngine::CompressorEngine(CompressorConfigBase *config,
	int band)
{
	this->config = config;
	this->band = band;
	reset();
}

CompressorEngine::~CompressorEngine()
{
}


void CompressorEngine::reset()
{
	slope_samples = 0;
	slope_current_sample = 0;
	peak_samples = 0;
	slope_value1 = 1.0;
	slope_value2 = 1.0;
	current_value = 0.5;
	gui_frame_samples = 2048;
	gui_max_gain = 1.0;
	gui_frame_counter = 0;
}


void CompressorEngine::calculate_ranges(int *attack_samples,
	int *release_samples,
	int *preview_samples,
	int sample_rate)
{
	BandConfig *band_config = &config->bands[band];
	*attack_samples = labs(Units::round(band_config->attack_len * sample_rate));
	*release_samples = Units::round(band_config->release_len * sample_rate);
	CLAMP(*attack_samples, 1, 1000000);
	CLAMP(*release_samples, 1, 1000000);
	*preview_samples = MAX(*attack_samples, *release_samples);
}


void CompressorEngine::process(Samples **output_buffer,
	Samples **input_buffer,
	int size,
	int sample_rate,
	int channels,
	int64_t start_position)
{
	BandConfig *band_config = &config->bands[band];
	int attack_samples;
	int release_samples;
	int preview_samples;
	int trigger = CLIP(config->trigger, 0, channels - 1);

	gui_gains.remove_all();
	gui_levels.remove_all();
	gui_offsets.remove_all();
	
	calculate_ranges(&attack_samples,
		&release_samples,
		&preview_samples,
		sample_rate);
	if( slope_current_sample < 0 ) slope_current_sample = slope_samples;

	double *trigger_buffer = input_buffer[trigger]->get_data();

	for( int i = 0; i < size; i++ ) {
		double current_slope = (slope_value2 - slope_value1) /
			slope_samples;

// maximums in the 2 time ranges
		double attack_slope = -0x7fffffff;
		double attack_sample = -1;
		int attack_offset = -1;
		int have_attack_sample = 0;
		double release_slope = -0x7fffffff;
		double release_sample = -1;
		int release_offset = -1;
		int have_release_sample = 0;
		if( slope_current_sample >= slope_samples ) {
// start new line segment
			for( int j = 1; j < preview_samples; j++ ) {
				GET_TRIGGER(input_buffer[channel]->get_data(), i + j)
				double new_slope = (sample - current_value) / j;
				if( j < attack_samples && new_slope >= attack_slope ) {
					attack_slope = new_slope;
					attack_sample = sample;
					attack_offset = j;
					have_attack_sample = 1;
				}
				
				if( j < release_samples && 
					new_slope <= 0 && 
					new_slope > release_slope ) {
					release_slope = new_slope;
					release_sample = sample;
					release_offset = j;
					have_release_sample = 1;
				}
			}

			slope_current_sample = 0;
			if( have_attack_sample && attack_slope >= 0 ) {
// attack
				peak_samples = attack_offset;
				slope_samples = attack_offset;
				slope_value1 = current_value;
				slope_value2 = attack_sample;
				current_slope = attack_slope;
//printf("CompressorEngine::process %d position=%ld slope=%f samples=%d\n", 
//__LINE__, start_position + i, current_slope, slope_samples);
			}
			else
			if( have_release_sample ) {
// release
				slope_samples = release_offset;
//				slope_samples = release_samples;
				peak_samples = release_offset;
				slope_value1 = current_value;
				slope_value2 = release_sample;
				current_slope = release_slope;
//printf("CompressorEngine::process %d position=%ld slope=%f\n", 
//__LINE__, start_position + i, current_slope);
			}
			else {
static int bug = 0;
if( !bug ) {
printf("CompressorEngine::process %d have neither attack nor release position=%ld attack=%f release=%f current_value=%f\n",
__LINE__, start_position + i, attack_slope, release_slope, current_value); bug = 1;
}
			}
		}
		else {
// check for new peak after the line segment
			GET_TRIGGER(input_buffer[channel]->get_data(), i + attack_samples)
			double new_slope = (sample - current_value) /
				attack_samples;
			if( current_slope >= 0 ) {
				if( new_slope > current_slope ) {
					peak_samples = attack_samples;
					slope_samples = attack_samples;
					slope_current_sample = 0;
					slope_value1 = current_value;
					slope_value2 = sample;
					current_slope = new_slope;
//printf("CompressorEngine::process %d position=%ld slope=%f\n", 
//__LINE__, start_position + i, current_slope);
				}
			}
			else
// this strings together multiple release periods instead of 
// approaching but never reaching the ending gain
			if( current_slope < 0 ) {
				if( sample > slope_value2 ) {
					peak_samples = attack_samples;
					slope_samples = release_samples;
					slope_current_sample = 0;
					slope_value1 = current_value;
					slope_value2 = sample;
					new_slope = (sample - current_value) /
						release_samples;
					current_slope = new_slope;
//printf("CompressorEngine::process %d position=%ld slope=%f\n", 
//__LINE__, start_position + i, current_slope);
				}
//				else
//				 {
//					 GET_TRIGGER(input_buffer[channel]->get_data(), i + release_samples)
//					 new_slope = (sample - current_value) /
// 						release_samples;
//					 if( new_slope < current_slope &&
//						 slope_current_sample >= peak_samples )
//					 {
//						 peak_samples = release_samples;
//						 slope_samples = release_samples;
//						 slope_current_sample = 0;
//						 slope_value1 = current_value;
//						 slope_value2 = sample;
// 						current_slope = new_slope;
// printf("CompressorEngine::process %d position=%ld slope=%f\n", 
// __LINE__, start_position + i, current_slope);
//					 }
//				 }
			}
		}

// Update current value and multiply gain
		slope_current_sample++;
		current_value = slope_value1 +
			(slope_value2 - slope_value1) * 
			slope_current_sample / 
			slope_samples;

		double gain = 1.0;
		if( !config->smoothing_only ) {
			if( !band_config->bypass )
				gain = config->calculate_gain(band, current_value);
// update the GUI frames
			if( fabs(gain - 1.0) > fabs(gui_max_gain - 1.0) ) {
				gui_max_gain = gain;
			}
// calculate the input level to draw.  Should it be the trigger or a channel?
			GET_TRIGGER(input_buffer[channel]->get_data(), i);
			if( sample > gui_max_level ) {
				gui_max_level = sample;
			}
			gui_frame_counter++;
			if( gui_frame_counter > gui_frame_samples ) {
//if( !EQUIV(gui_frame_max, 1.0) ) printf("CompressorEngine::process %d offset=%d gui_frame_max=%f\n", __LINE__, i, gui_frame_max);
				gui_gains.append(gui_max_gain);
				gui_levels.append(gui_max_level);
				gui_offsets.append((double)i / sample_rate);
				gui_max_gain = 1.0;
				gui_max_level = 0.0;
				gui_frame_counter = 0;
			}
		}
		else {
			gain = current_value > 0.01 ? 0.5 / current_value : 50.;
		}
		for( int j = 0; j < channels; j++ ) {
			output_buffer[j]->get_data()[i] = input_buffer[j]->get_data()[i] * gain;
		}
	}
}


CompressorPopup::CompressorPopup(CompressorCanvasBase *canvas)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->canvas = canvas;
}

CompressorPopup::~CompressorPopup()
{
}

	
void CompressorPopup::create_objects()
{
	add_item(new CompressorCopy(this));
	add_item(new CompressorPaste(this));
	add_item(new CompressorClearGraph(this));
}


CompressorCopy::CompressorCopy(CompressorPopup *popup)
 : BC_MenuItem(_("Copy graph"))
{
	this->popup = popup;
}


CompressorCopy::~CompressorCopy()
{
}

int CompressorCopy::handle_event()
{
	FileXML output;
	CompressorConfigBase *config = popup->canvas->config;
	config->bands[config->current_band].save_data(&output, 0, 0);
	output.terminate_string();
	char *cp = output.string(); 
	popup->to_clipboard(cp, strlen(cp), SECONDARY_SELECTION);
	return 1;
}


CompressorPaste::CompressorPaste(CompressorPopup *popup)
 : BC_MenuItem(_("Paste graph"))
{
	this->popup = popup;
}


CompressorPaste::~CompressorPaste()
{
}

int CompressorPaste::handle_event()
{
	int len = popup->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	if( len ) {
		CompressorConfigBase *config = popup->canvas->config;
		char *string = new char[len + 1];
		popup->get_clipboard()->from_clipboard(string, len, SECONDARY_SELECTION);
		
		FileXML xml;
		xml.read_from_string(string);
		delete [] string;
		int result = 0, got_it = 0;
		while( !(result = xml.read_tag()) ) {
			if( xml.tag.title_is("COMPRESSORBAND") ) {
				int band = config->current_band;
				BandConfig *band_config = &config->bands[band];
				band_config->read_data(&xml, 0);
				got_it = 1;
				break;
			}
		}
		
		if( got_it ) {
			popup->canvas->update();
			PluginClient *plugin = popup->canvas->plugin;
			plugin->send_configure_change();
		}
	}
	return 1;
}


CompressorClearGraph::CompressorClearGraph(CompressorPopup *popup)
 : BC_MenuItem(_("Clear graph"))
{
	this->popup = popup;
}


CompressorClearGraph::~CompressorClearGraph()
{
}

int CompressorClearGraph::handle_event()
{
	CompressorConfigBase *config = popup->canvas->config;
	config->bands[config->current_band].levels.remove_all();
	popup->canvas->update();
	PluginClient *plugin = popup->canvas->plugin;
	plugin->send_configure_change();
	return 1;
}

