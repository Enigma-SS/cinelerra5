
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "cstrdup.h"
#include "edl.inc"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "theme.h"
#include "vframe.h"
#include "shapewipe.h"

#include <png.h>
#include <math.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>

#define SHAPE_SEARCHPATH "/shapes"
#define DEFAULT_SHAPE "circle"
// feather slider range log2 = -10 .. -1 == 9.8e-4 .. 0.5
#define SHAPE_FLOG_MIN -10.
#define SHAPE_FLOG_MAX -1.
#define SHAPE_FMIN expf(M_LN2*SHAPE_FLOG_MIN)
#define SHAPE_FMAX expf(M_LN2*SHAPE_FLOG_MAX)

REGISTER_PLUGIN(ShapeWipeMain)

ShapeWipeConfig::ShapeWipeConfig()
{
	direction = 0;
	feather = SHAPE_FMIN;
	preserve_aspect = 0;
	strcpy(shape_name, DEFAULT_SHAPE);
}
ShapeWipeConfig::~ShapeWipeConfig()
{
}

void ShapeWipeConfig::read_xml(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while( !input.read_tag() ) {
		if( input.tag.title_is("SHAPEWIPE") ) {
			direction = input.tag.get_property("DIRECTION", direction);
			feather = input.tag.get_property("FEATHER", feather);
			preserve_aspect = input.tag.get_property("PRESERVE_ASPECT", preserve_aspect);
			input.tag.get_property("SHAPE_NAME", shape_name);
		}
	}
}
void ShapeWipeConfig::save_xml(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("SHAPEWIPE");
	output.tag.set_property("DIRECTION", direction);
	output.tag.set_property("FEATHER", feather);
	output.tag.set_property("PRESERVE_ASPECT", preserve_aspect);
	output.tag.set_property("SHAPE_NAME", shape_name);
	output.append_tag();
	output.tag.set_title("/SHAPEWIPE");
	output.append_tag();
	output.terminate_string();
}


ShapeWipeW2B::ShapeWipeW2B(ShapeWipeMain *plugin,
	ShapeWipeWindow *window, int x, int y)
 : BC_Radial(x, y, plugin->config.direction == 0, _("White to Black"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeW2B::handle_event()
{
	update(1);
	plugin->config.direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

ShapeWipeB2W::ShapeWipeB2W(ShapeWipeMain *plugin,
	ShapeWipeWindow *window, int x, int y)
 : BC_Radial(x, y, plugin->config.direction == 1, _("Black to White"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeB2W::handle_event()
{
	update(1);
	plugin->config.direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}


ShapeWipePreserveAspectRatio::ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
	ShapeWipeWindow *window, int x, int y)
 : BC_CheckBox (x, y, plugin->config.preserve_aspect, _("Preserve shape aspect ratio"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipePreserveAspectRatio::handle_event()
{
	plugin->config.preserve_aspect = get_value();
	plugin->send_configure_change();
	return 0;
}


ShapeWipeTumble::ShapeWipeTumble(ShapeWipeMain *client,
	ShapeWipeWindow *window, int x, int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
	set_increment(0.01);
}

int ShapeWipeTumble::handle_up_event()
{
	window->prev_shape();
	return 1;
}

int ShapeWipeTumble::handle_down_event()
{
	window->next_shape();
	return 0;
}


ShapeWipeFeather::ShapeWipeFeather(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y)
 : BC_TumbleTextBox(window,
		bclip(client->config.feather, SHAPE_FMIN, SHAPE_FMAX),
		SHAPE_FMIN, SHAPE_FMAX, x, y, xS(64), yS(3))
{
	this->client = client;
	this->window = window;
}

int ShapeWipeFeather::handle_event()
{
	float v = atof(get_text());
	bclamp(v, SHAPE_FMIN, SHAPE_FMAX);
	client->config.feather = v;
	float sv = log(v)/M_LN2;
	window->shape_fslider->update(sv);
	client->send_configure_change();
	return 1;
}

ShapeWipeFSlider::ShapeWipeFSlider(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, SHAPE_FLOG_MIN, SHAPE_FLOG_MAX,
	log(bclip(client->config.feather, SHAPE_FMIN, SHAPE_FMAX))/M_LN2)
{
	this->client = client;
	this->window = window;
	set_precision(0.001);
	set_pagination(0.01, 0.1);
	enable_show_value(0);
}

int ShapeWipeFSlider::handle_event()
{
	float v = get_value();
	float vv = exp(M_LN2*v);
	client->config.feather = vv;
	window->shape_feather->update(vv);
	client->send_configure_change();
	return 1;
}

ShapeWipeReset::ShapeWipeReset(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->window = window;
	set_tooltip(_("Reset feather"));
}

int ShapeWipeReset::handle_event()
{
	window->shape_fslider->update(SHAPE_FLOG_MIN);
	float v = SHAPE_FMIN;
	window->shape_feather->update(v);
	client->config.feather = v;
	client->send_configure_change();
	return 1;
}

ShapeWipeShape::ShapeWipeShape(ShapeWipeMain *client,
		ShapeWipeWindow *window, int x, int y,
		int text_w, int list_h)
 : BC_PopupTextBox(window, &window->shapes, client->config.shape_name,
	x, y, text_w, list_h)
{
	this->client = client;
	this->window = window;
}

int ShapeWipeShape::handle_event()
{
	strcpy(client->config.shape_name, get_text());
	client->send_configure_change();
	return 1;
}


ShapeWipeWindow::ShapeWipeWindow(ShapeWipeMain *plugin)
 : PluginClientWindow(plugin, xS(425), yS(215), xS(425), yS(215), 0)
{
	this->plugin = plugin;
	shape_feather = 0;
}

ShapeWipeWindow::~ShapeWipeWindow()
{
	shapes.remove_all_objects();
	delete shape_feather;
}


void ShapeWipeWindow::create_objects()
{
	BC_Title *title = 0;
	lock_window("ShapeWipeWindow::create_objects");
	int pad = xS(10), margin = xS(10);
	int x = margin, y = margin;
	int ww = get_w() - 2*margin;

	plugin->init_shapes();
	for( int i=0; i<plugin->shape_titles.size(); ++i ) {
		shapes.append(new BC_ListBoxItem(plugin->shape_titles.get(i)));
	}

	BC_TitleBar *bar;
	add_subwindow(bar = new BC_TitleBar(x, y, ww, xS(20), yS(10),
		_("Wipe"), MEDIUMFONT));
	y += bar->get_h() + pad;

	add_subwindow(title = new BC_Title(x, y, _("Shape:")));
	int x1 = xS(85), x2 = xS(355), x3 = xS(386);
	shape_text = new ShapeWipeShape(plugin, this, x1, y, x2-x1, yS(200));
	shape_text->create_objects();
	add_subwindow(new ShapeWipeTumble(plugin, this, x3, y));
	y += shape_text->get_h() + pad;

	x = margin;
	add_subwindow(title = new BC_Title(x, y, _("Feather:")));
	x = x1;
	shape_feather = new ShapeWipeFeather(plugin, this, x, y);
	shape_feather->create_objects();
	shape_feather->set_log_floatincrement(1);
	x += shape_feather->get_w() + 2*pad;
	add_subwindow(shape_fslider = new ShapeWipeFSlider(plugin, this, x, y, x2-x));
	add_subwindow(shape_reset = new ShapeWipeReset(plugin, this, x3, y));
	y += shape_fslider->get_h() + pad;

	x = margin;
	ShapeWipePreserveAspectRatio *aspect_ratio;
	add_subwindow(aspect_ratio = new ShapeWipePreserveAspectRatio(
		plugin, this, x, y));
	y += aspect_ratio->get_h() + pad;

	add_subwindow(bar = new BC_TitleBar(x, y, ww, xS(20), yS(10),
		_("Direction"), MEDIUMFONT));
	y += bar->get_h() + pad;
	x = margin;
	add_subwindow(left = new ShapeWipeW2B(plugin, this, x, y));
	y += left->get_h();
	add_subwindow(right = new ShapeWipeB2W(plugin, this, x, y));

	show_window();
	unlock_window();
}

void ShapeWipeWindow::next_shape()
{
	ShapeWipeConfig &config = plugin->config;
	int k = plugin->shape_titles.size();
	while( --k>=0 && strcmp(plugin->shape_titles.get(k),config.shape_name) );

	if( k >= 0 ) {
		if( ++k >= plugin->shape_titles.size() ) k = 0;
		strcpy(config.shape_name, plugin->shape_titles.get(k));
		shape_text->update(config.shape_name);
	}
	client->send_configure_change();
}

void ShapeWipeWindow::prev_shape()
{
	ShapeWipeConfig &config = plugin->config;
	int k = plugin->shape_titles.size();
	while( --k>=0 && strcmp(plugin->shape_titles.get(k),config.shape_name) );

	if( k >= 0 ) {
		if( --k < 0 ) k = plugin->shape_titles.size()-1;
		strcpy(config.shape_name, plugin->shape_titles.get(k));
		shape_text->update(config.shape_name);
	}
	client->send_configure_change();
}


ShapeWipeMain::ShapeWipeMain(PluginServer *server)
 : PluginVClient(server)
{
	input = 0;
	output = 0;
	engine = 0;
	current_filename[0] = '\0';
	current_name[0] = 0;
	pattern_image = 0;
	last_preserve_aspect = 0;
	shapes_initialized = 0;
	shape_paths.set_array_delete();
	shape_titles.set_array_delete();
}

ShapeWipeMain::~ShapeWipeMain()
{
	reset_pattern_image();
	shape_paths.remove_all_objects();
	shape_titles.remove_all_objects();
	delete engine;
}

const char* ShapeWipeMain::plugin_title() { return N_("Shape Wipe"); }
int ShapeWipeMain::is_transition() { return 1; }
int ShapeWipeMain::uses_gui() { return 1; }

NEW_WINDOW_MACRO(ShapeWipeMain, ShapeWipeWindow);

void ShapeWipeMain::read_data(KeyFrame *keyframe)
{
	config.read_xml(keyframe);
}
void ShapeWipeMain::save_data(KeyFrame *keyframe)
{
	config.save_xml(keyframe);
}

void ShapeWipeMain::init_shapes()
{
	if( !shapes_initialized ) {
		FileSystem fs;
		fs.set_filter("[*.png][*.jpg]");
		char shape_path[BCTEXTLEN];
		sprintf(shape_path, "%s%s", get_plugin_dir(), SHAPE_SEARCHPATH);
		fs.update(shape_path);

		for( int i=0; i<fs.total_files(); ++i ) {
			FileItem *file_item = fs.get_entry(i);
			if( !file_item->get_is_dir() ) {
				shape_paths.append(cstrdup(file_item->get_path()));
				char *ptr = cstrdup(file_item->get_name());
				char *ptr2 = strrchr(ptr, '.');
				if(ptr2) *ptr2 = 0;
				shape_titles.append(ptr);
			}
		}

		shapes_initialized = 1;
	}
}

int ShapeWipeMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
	return 1;
}

int ShapeWipeMain::read_pattern_image(char *shape_name,
		int frame_width, int frame_height)
{
	VFrame *pattern = 0;
	int ret = 0, fd = -1;
	int is_png = 0;
	unsigned char header[10];
// Convert name to filename
	int k = shape_paths.size(), hsz = sizeof(header);
	while( --k>=0 && strcmp(shape_titles[k], shape_name) );
	if( k < 0 ) ret = 1;
	if( !ret ) {
		strcpy(current_filename, shape_paths[k]);
		fd = ::open(current_filename, O_RDONLY);
		if( fd < 0 || read(fd,header,hsz) != hsz ) ret = 1;
	}
	if( !ret ) {
		is_png = !png_sig_cmp(header, 0, hsz);
		if( !is_png && strncmp("JFIF", (char*)header+6, 4) ) ret = 1;
	}
	if( !ret ) {
		lseek(fd, 0, SEEK_SET);
		pattern = is_png ?
			VFramePng::vframe_png(fd, 1, 1) :
			VFrameJpeg::vframe_jpeg(fd, 1, 1, BC_GREY8);
		if( !pattern ) ret = 1;
	}
	if( fd >= 0 ) {
		close(fd);  fd = -1;
	}
	if( !ret ) {
		int width = pattern->get_w(), height = pattern->get_h();
		double row_factor, col_factor;
		double row_offset = 0.5, col_offset = 0.5;	// for rounding

		if( config.preserve_aspect && aspect_w && aspect_h ) {
			row_factor = (height-1)/aspect_h;
			col_factor = (width-1)/aspect_w;
			if( row_factor < col_factor )
				col_factor = row_factor;
			else
				row_factor = col_factor;
			row_factor *= aspect_h/(double)(frame_height-1);
			col_factor *= aspect_w/(double)(frame_width-1);

			// center the pattern over the frame
			row_offset += (height-1-(frame_height-1)*row_factor)/2;
			col_offset += (width-1-(frame_width-1)*col_factor)/2;
		}
		else {
			// Stretch (or shrink) the pattern image to fill the frame
			row_factor = (double)(height-1)/(double)(frame_height-1);
			col_factor = (double)(width-1)/(double)(frame_width-1);
		}
		int out_w = width * col_factor, out_h = height * row_factor;
		if( out_w != width || out_h != height ) {
			VFrame *new_pattern = new VFrame(frame_width, frame_height, BC_GREY8);
			new_pattern->transfer_from(pattern, 0, col_offset,row_offset,
				frame_width*col_factor, frame_height*row_factor);
			delete pattern;  pattern = new_pattern;
		}
		unsigned char **rows = pattern->get_rows();
		unsigned char min = 0xff, max = 0x00;
		// first, determine range min..max
		for( int y=0; y<frame_height; ++y ) {
			unsigned char *row = rows[y];
			for( int x=0; x<frame_width; ++x ) {
				unsigned char value = row[x];
				if( value < min ) min = value;
				if( value > max ) max = value;
			}
		}
		if( min > max ) min = max;
		int range = max - min;
		if( !range ) range = 1;
		pattern_image = new  unsigned char*[frame_height];
		// scale to fade normalized pattern_image
		for( int y=0; y<frame_height; ++y ) {
			unsigned char *row = rows[y];
			pattern_image[y] = new unsigned char[frame_width];
			for( int x=0; x<frame_width; ++x ) {
				unsigned char value = row[x];
				pattern_image[y][x] = 0xff*(value-min) / range;
			}
		}
		this->frame_width = frame_width;
		this->frame_height = frame_height;
	}

	return ret;
}

void ShapeWipeMain::reset_pattern_image()
{
	if( pattern_image ) {
		for( int y=0; y<frame_height; ++y )
			delete [] pattern_image[y];
		delete [] pattern_image;  pattern_image = 0;
	}
}

#define SHAPEBLEND(type, components, tmp_type) { \
	float scale = feather ? 1/feather : 0xff; \
	type  **in_rows = (type**)input->get_rows(); \
	type **out_rows = (type**)output->get_rows(); \
	if( !dir ) { \
		for( int y=y1; y<y2; ++y ) { \
			type *in_row = (type*) in_rows[y]; \
			type *out_row = (type*)out_rows[y]; \
			unsigned char *pattern_row = pattern_image[y]; \
			for( int x=0; x<w; ++x ) { \
				tmp_type d = (pattern_row[x] - threshold) * scale; \
				if( d > 0xff ) d = 0xff; \
				else if( d < -0xff ) d = -0xff; \
				tmp_type a = (d + 0xff) / 2, b = 0xff - a; \
				for( int i=0; i<components; ++i ) { \
					type ic = in_row[i], oc = out_row[i]; \
					out_row[i] = (ic * a + oc * b) / 0xff; \
				} \
				in_row += components; out_row += components; \
			} \
		} \
	} \
	else { \
		for( int y=y1; y<y2; ++y ) { \
			type *in_row = (type*) in_rows[y]; \
			type *out_row = (type*)out_rows[y]; \
			unsigned char *pattern_row = pattern_image[y]; \
			for( int x=0; x<w; ++x ) { \
				tmp_type d = (pattern_row[x] - threshold) * scale; \
				if( d > 0xff ) d = 0xff; \
				else if( d < -0xff ) d = -0xff; \
				tmp_type b = (d + 0xff) / 2, a = 0xff - b; \
				for( int i=0; i<components; ++i ) { \
					type ic = in_row[i], oc = out_row[i]; \
					out_row[i] = (ic * a + oc * b) / 0xff; \
				} \
				in_row += components; out_row += components; \
			} \
		} \
	} \
}

int ShapeWipeMain::process_realtime(VFrame *input, VFrame *output)
{
	this->input = input;
	this->output = output;
	int w = input->get_w();
	int h = input->get_h();
	init_shapes();
	load_configuration();

	if( strncmp(config.shape_name, current_name, BCTEXTLEN) ||
	    config.preserve_aspect != last_preserve_aspect ) {
		reset_pattern_image();
	}
	if ( !pattern_image ) {
		if( read_pattern_image(config.shape_name, w, h) ) {
			fprintf(stderr, _("Shape Wipe: cannot load shape %s\n"),
				current_filename);
			current_filename[0] = 0;
			return 0;
		}
		strncpy(current_name, config.shape_name, BCTEXTLEN);
		last_preserve_aspect = config.preserve_aspect;
	}

	float fade = (float)PluginClient::get_source_position() /
		(float)PluginClient::get_total_len();
	if( !config.direction ) fade = 1 - fade;
	threshold = fade * 0xff;

	int slices = w*h/0x40000+1;
	int max_slices = BC_Resources::machine_cpus/2;
	if( slices > max_slices ) slices = max_slices;
	if( slices < 1 ) slices = 1;
	if( engine && engine->get_total_clients() != slices ) {
		delete engine;  engine = 0;
	}
	if( !engine )
		engine = new ShapeEngine(this, slices, slices);

	engine->process_packages();
	return 0;
}


ShapePackage::ShapePackage()
 : LoadPackage()
{
}

ShapeUnit::ShapeUnit(ShapeEngine *server) : LoadClient(server)
{
	this->server = server;
}

ShapeUnit::~ShapeUnit()
{
}

void ShapeUnit::process_package(LoadPackage *package)
{
	VFrame *input = server->plugin->input;
	VFrame *output = server->plugin->output;
	int w = input->get_w();
	int dir = server->plugin->config.direction;

	unsigned char **pattern_image = server->plugin->pattern_image;
	unsigned char threshold = server->plugin->threshold;
	float feather = server->plugin->config.feather;
	ShapePackage *pkg = (ShapePackage*)package;
	int y1 = pkg->y1, y2 = pkg->y2;

	switch(input->get_color_model()) {
	case BC_RGB_FLOAT:
		SHAPEBLEND(float, 3, float)
		break;
	case BC_RGB888:
	case BC_YUV888:
		SHAPEBLEND(unsigned char, 3, int)
		break;
	case BC_RGBA_FLOAT:
		SHAPEBLEND(float, 4, float)
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		SHAPEBLEND(unsigned char, 4, int)
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		SHAPEBLEND(uint16_t, 3, int64_t)
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		SHAPEBLEND(uint16_t, 4, int64_t)
		break;
	}
}


ShapeEngine::ShapeEngine(ShapeWipeMain *plugin,
	int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

ShapeEngine::~ShapeEngine()
{
}


void ShapeEngine::init_packages()
{
	int y = 0, h1 = plugin->input->get_h()-1;
	int total_packages = get_total_packages();
	for(int i = 0; i<total_packages; ) {
		ShapePackage *pkg = (ShapePackage*)get_package(i++);
		pkg->y1 = y;
		y = h1 * i / total_packages;
		pkg->y2 = y;
	}
}

LoadClient* ShapeEngine::new_client()
{
	return new ShapeUnit(this);
}

LoadPackage* ShapeEngine::new_package()
{
	return new ShapePackage;
}

