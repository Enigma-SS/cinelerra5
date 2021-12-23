
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bccolors.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "gradient.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "theme.h"
#include "vframe.h"

REGISTER_PLUGIN(GradientMain)

GradientConfig::GradientConfig()
{
	reset();
}

void GradientConfig::reset()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	in_r  = in_g  = in_b  = in_a  = 0xff;
	out_r = out_g = out_b = out_a = 0x00;
	shape = GradientConfig::LINEAR;
	rate = GradientConfig::LINEAR;
	center_x = 50;
	center_y = 50;
}

int GradientConfig::equivalent(GradientConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		in_r == that.in_r &&
		in_g == that.in_g &&
		in_b == that.in_b &&
		in_a == that.in_a &&
		out_r == that.out_r &&
		out_g == that.out_g &&
		out_b == that.out_b &&
		out_a == that.out_a &&
		shape == that.shape &&
		rate == that.rate &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y));
}

void GradientConfig::copy_from(GradientConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	in_r = that.in_r;
	in_g = that.in_g;
	in_b = that.in_b;
	in_a = that.in_a;
	out_r = that.out_r;
	out_g = that.out_g;
	out_b = that.out_b;
	out_a = that.out_a;
	shape = that.shape;
	rate = that.rate;
	center_x = that.center_x;
	center_y = that.center_y;
}

void GradientConfig::interpolate(GradientConfig &prev, GradientConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = (int)(prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = (int)(prev.out_radius * prev_scale + next.out_radius * next_scale);
	in_r = (int)(prev.in_r * prev_scale + next.in_r * next_scale);
	in_g = (int)(prev.in_g * prev_scale + next.in_g * next_scale);
	in_b = (int)(prev.in_b * prev_scale + next.in_b * next_scale);
	in_a = (int)(prev.in_a * prev_scale + next.in_a * next_scale);
	out_r = (int)(prev.out_r * prev_scale + next.out_r * next_scale);
	out_g = (int)(prev.out_g * prev_scale + next.out_g * next_scale);
	out_b = (int)(prev.out_b * prev_scale + next.out_b * next_scale);
	out_a = (int)(prev.out_a * prev_scale + next.out_a * next_scale);
	shape = prev.shape;
	rate = prev.rate;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
}

int GradientConfig::get_in_color()
{
	int result = (in_r << 16) | (in_g << 8) | (in_b);
	return result;
}

int GradientConfig::get_out_color()
{
	int result = (out_r << 16) | (out_g << 8) | (out_b);
	return result;
}

#define COLOR_W xS(100)
#define COLOR_H yS(30)

GradientWindow::GradientWindow(GradientMain *plugin)
 : PluginClientWindow(plugin, xS(350), yS(290), xS(350), yS(290), 0)
{
	this->plugin = plugin;
	angle = 0;
	angle_title = 0;
	center_x = 0;
	center_y = 0;
	center_x_title = 0;
	center_y_title = 0;
}
GradientWindow::~GradientWindow()
{
}


void GradientWindow::create_objects()
{
	int xs10 = xS(10);
	int ys10 = yS(10);
	int margin = plugin->get_theme()->widget_border;
	int x = xs10, y = ys10;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x, y, _("Shape:")));
	add_subwindow(shape = new GradientShape(plugin, this,
			x + title->get_w() + margin, y));
	shape->create_objects();
	y += shape->get_h() + margin;
	shape_x = x;
	shape_y = y;
	y += BC_Pot::calculate_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Rate:")));
	add_subwindow(rate = new GradientRate(plugin,
			x + title->get_w() + margin, y));
	rate->create_objects();
	y += rate->get_h() + 3*margin;

	int x1 = x, y1 = y;
	BC_Title *title1;
	add_subwindow(title1 = new BC_Title(x, y, _("Inner radius:")));
	y += BC_Slider::get_span(0) + margin;
	BC_Title *title2;
	add_subwindow(title2 = new BC_Title(x, y, _("Outer radius:")));

	y = y1;
	x += MAX(title1->get_w(), title2->get_w()) + margin;
	add_subwindow(in_radius = new GradientInRadius(plugin, x, y));
	y += in_radius->get_h() + margin;
	add_subwindow(out_radius = new GradientOutRadius(plugin, x, y));
	y += out_radius->get_h() + 3*margin;

	x = x1;
	add_subwindow(title1 = new BC_Title(x, y, _("Inner Color:")));
	y1 = y + COLOR_H+4 + 2*margin;
	add_subwindow(title2 = new BC_Title(x, y1, _("Outer Color:")));
	int x2 = x + MAX(title1->get_w(), title2->get_w()) + margin;
	int in_rgb = plugin->config.get_in_color();
	int in_a = plugin->config.in_a;
	add_subwindow(in_color = new GradientInColorButton(plugin, this, x2+2, y+2, in_rgb, in_a));
	draw_3d_border(x2,y, COLOR_W+4,COLOR_H+4, 1);
	in_color->create_objects();

	int out_rgb = plugin->config.get_out_color();
	int out_a = plugin->config.out_a;
	add_subwindow(out_color = new GradientOutColorButton(plugin, this, x2+2, y1+2, out_rgb, out_a));
	draw_3d_border(x2,y1, COLOR_W+4,COLOR_H+4, 1);
	out_color->create_objects();
	y = y1 + COLOR_H+4 + 3*margin;

	add_subwindow(reset = new GradientReset(plugin, this, x, y));
	update_shape();
	show_window();
}

void GradientWindow::update_shape()
{
	int x = shape_x, y = shape_y;

	if( plugin->config.shape == GradientConfig::LINEAR ) {
		delete center_x_title;  center_x_title = 0;
		delete center_y_title;  center_y_title = 0;
		delete center_x;  center_x = 0;
		delete center_y;  center_y = 0;
		if( !angle ) {
			add_subwindow(angle_title = new BC_Title(x, y, _("Angle:")));
			add_subwindow(angle = new GradientAngle(plugin, x + angle_title->get_w() + xS(10), y));
		}
	}
	else {
		delete angle_title;  angle_title = 0;
		delete angle;  angle = 0;
		if( !center_x ) {
			add_subwindow(center_x_title = new BC_Title(x, y, _("Center X:")));
			add_subwindow(center_x = new GradientCenterX(plugin,
				x + center_x_title->get_w() + xS(10), y));
			x += center_x_title->get_w() + xS(10) + center_x->get_w() + xS(10);
			add_subwindow(center_y_title = new BC_Title(x, y, _("Center Y:")));
			add_subwindow(center_y = new GradientCenterY(plugin,
				x + center_y_title->get_w() + xS(10), y));
		}
	}
	show_window();
}

void GradientWindow::done_event(int result)
{
	in_color->close_picker();
	out_color->close_picker();
}

GradientShape::GradientShape(GradientMain *plugin, GradientWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, xS(100), to_text(plugin->config.shape), 1)
{
	this->plugin = plugin;
	this->gui = gui;
}
void GradientShape::create_objects()
{
	add_item(new BC_MenuItem(to_text(GradientConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(GradientConfig::RADIAL)));
}
char* GradientShape::to_text(int shape)
{
	switch( shape ) {
	case GradientConfig::LINEAR:	return _("Linear");
	}
	return _("Radial");
}
int GradientShape::from_text(char *text)
{
	if( !strcmp(text, to_text(GradientConfig::LINEAR)) )
		return GradientConfig::LINEAR;
	return GradientConfig::RADIAL;
}
int GradientShape::handle_event()
{
	plugin->config.shape = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
	return 1;
}


GradientCenterX::GradientCenterX(GradientMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_x, 0, xS(100))
{
	this->plugin = plugin;
}
int GradientCenterX::handle_event()
{
	plugin->config.center_x = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientCenterY::GradientCenterY(GradientMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_y, 0, xS(100))
{
	this->plugin = plugin;
}

int GradientCenterY::handle_event()
{
	plugin->config.center_y = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientAngle::GradientAngle(GradientMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.angle, -180, 180)
{
	this->plugin = plugin;
}

int GradientAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientRate::GradientRate(GradientMain *plugin, int x, int y)
 : BC_PopupMenu(x, y, xS(100), to_text(plugin->config.rate), 1)
{
	this->plugin = plugin;
}
void GradientRate::create_objects()
{
	add_item(new BC_MenuItem(to_text(GradientConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(GradientConfig::LOG)));
	add_item(new BC_MenuItem(to_text(GradientConfig::SQUARE)));
}
char* GradientRate::to_text(int shape)
{
	switch( shape ) {
	case GradientConfig::LINEAR:	return _("Linear");
	case GradientConfig::LOG:	return _("Log");
	}
	return _("Square");
}
int GradientRate::from_text(char *text)
{
	if( !strcmp(text, to_text(GradientConfig::LINEAR)) )
		return GradientConfig::LINEAR;
	if( !strcmp(text, to_text(GradientConfig::LOG)) )
		return GradientConfig::LOG;
	return GradientConfig::SQUARE;
}
int GradientRate::handle_event()
{
	plugin->config.rate = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}


GradientInRadius::GradientInRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x, y, 0, xS(200), yS(200),
		0.f, 100.f, (float)plugin->config.in_radius)
{
	this->plugin = plugin;
}

int GradientInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientOutRadius::GradientOutRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x, y, 0, xS(200), yS(200),
		0.f, 100.f, (float)plugin->config.out_radius)
{
	this->plugin = plugin;
}

int GradientOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientInColorButton::GradientInColorButton(GradientMain *plugin, GradientWindow *gui,
		int x, int y, int color, int alpha)
 : ColorBoxButton(_("Inner color:"), x, y, COLOR_W, COLOR_H, color, alpha, 1)
{
	this->plugin = plugin;
	this->gui = gui;
}

GradientInColorButton::~GradientInColorButton()
{
}

void GradientInColorButton::handle_done_event(int result)
{
	if( result ) {
		gui->lock_window("GradientInColorButton::handle_done_event");
		update_gui(orig_color, orig_alpha);
		gui->unlock_window();
		handle_new_color(orig_color, orig_alpha);
	}
}

int GradientInColorButton::handle_new_color(int color, int alpha)
{
	plugin->config.in_r = (color & 0xff0000) >> 16;
	plugin->config.in_g = (color & 0xff00) >> 8;
	plugin->config.in_b = (color & 0xff);
	plugin->config.in_a = alpha;
	plugin->send_configure_change();
	return 1;
}

GradientOutColorButton::GradientOutColorButton(GradientMain *plugin, GradientWindow *gui,
		int x, int y, int color, int alpha)
 : ColorBoxButton(_("Outer color:"), x, y, COLOR_W, COLOR_H, color, alpha, 1)
{
	this->plugin = plugin;
	this->gui = gui;
}

GradientOutColorButton::~GradientOutColorButton()
{
}

void GradientOutColorButton::handle_done_event(int result)
{
	if( result ) {
		gui->lock_window("GradientOutColorButton::handle_done_event");
		update_gui(orig_color, orig_alpha);
		gui->unlock_window();
		handle_new_color(orig_color, orig_alpha);
	}
}

int GradientOutColorButton::handle_new_color(int color, int alpha)
{
	plugin->config.out_r = (color & 0xff0000) >> 16;
	plugin->config.out_g = (color & 0xff00) >> 8;
	plugin->config.out_b = (color & 0xff);
	plugin->config.out_a = alpha;
	plugin->send_configure_change();
	return 1;
}


GradientReset::GradientReset(GradientMain *plugin, GradientWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}

int GradientReset::handle_event()
{
	plugin->config.reset();
	window->update_gui();
	plugin->send_configure_change();
	return 1;
}


GradientMain::GradientMain(PluginServer *server)
 : PluginVClient(server)
{

	need_reconfigure = 1;
	gradient = 0;
	engine = 0;
	overlayer = 0;
	table = 0;
	table_size = 0;
}

GradientMain::~GradientMain()
{
	delete [] table;
	delete gradient;
	delete engine;
	delete overlayer;
}

const char* GradientMain::plugin_title() { return N_("Gradient"); }
int GradientMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(GradientMain, GradientWindow)

LOAD_CONFIGURATION_MACRO(GradientMain, GradientConfig)

int GradientMain::is_synthesis()
{
	return 1;
}


int GradientMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	this->input = frame;
	this->output = frame;
	float fw = input->get_w(), fh = input->get_h();
	gradient_size = hypotf(fw, fh);
	need_reconfigure = load_configuration();

	int need_alpha = config.in_a != 0xff || config.out_a != 0xff;
	if( need_alpha )
		read_frame(frame, 0, start_position, frame_rate, get_use_opengl());
	if( get_use_opengl() ) return run_opengl();

	int gradient_cmodel = input->get_color_model();
	if( need_alpha && BC_CModels::components(gradient_cmodel) == 3 ) {
		switch( gradient_cmodel ) {
		case BC_RGB888:     gradient_cmodel = BC_RGBA8888;    break;
		case BC_YUV888:     gradient_cmodel = BC_YUVA8888;    break;
		case BC_RGB_FLOAT:  gradient_cmodel = BC_RGBA_FLOAT;  break;
		}
	}

	int bpp = BC_CModels::calculate_pixelsize(gradient_cmodel);
	int comps = BC_CModels::components(gradient_cmodel);
	int grad_size1 = gradient_size + 1;
	int sz = 4 * (bpp / comps) * grad_size1;
	if( table_size < sz ) {
		delete [] table;  table = 0;
	}
	if( !table ) {
		table = new uint8_t[table_size = sz];
		need_reconfigure = 1;
	}

	if( gradient && gradient->get_color_model() != gradient_cmodel ) {
		delete gradient;
		gradient = 0;
	}

	if( !gradient )
		gradient = new VFrame(input->get_w(), input->get_h(),
			gradient_cmodel, 0);

	if( !engine )
		engine = new GradientServer(this,
			get_project_smp() + 1, get_project_smp() + 1);
	engine->process_packages();

// Use overlay routine in GradientServer if mismatched colormodels
	if( gradient->get_color_model() == output->get_color_model() ) {
		if( !overlayer ) overlayer = new OverlayFrame(get_project_smp() + 1);
		overlayer->overlay(output, gradient,
			0, 0, input->get_w(), input->get_h(),
			0, 0, input->get_w(), input->get_h(),
			1.0, TRANSFER_NORMAL, NEAREST_NEIGHBOR);
	}

	return 0;
}


void GradientMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("GradientMain::update_gui");
	if( load_configuration() ) {
		GradientWindow *window = (GradientWindow *)thread->window;
		window->update_gui();
		window->flush();
	}
	thread->window->unlock_window();
}

void GradientWindow::update_gui()
{
	GradientConfig &config = plugin->config;
	rate->set_text(GradientRate::to_text(config.rate));
	in_radius->update(config.in_radius);
	out_radius->update(config.out_radius);
	shape->set_text(GradientShape::to_text(config.shape));
	if( angle ) angle->update(config.angle);
	if( center_x ) center_x->update(config.center_x);
	if( center_y ) center_y->update(config.center_y);
	update_shape();
	in_color->update_gui(config.get_in_color(), config.in_a);
	out_color->update_gui(config.get_out_color(), config.out_a);
}


void GradientMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("GRADIENT");

	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("IN_R", config.in_r);
	output.tag.set_property("IN_G", config.in_g);
	output.tag.set_property("IN_B", config.in_b);
	output.tag.set_property("IN_A", config.in_a);
	output.tag.set_property("OUT_R", config.out_r);
	output.tag.set_property("OUT_G", config.out_g);
	output.tag.set_property("OUT_B", config.out_b);
	output.tag.set_property("OUT_A", config.out_a);
	output.tag.set_property("SHAPE", config.shape);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.append_tag();
	output.tag.set_title("/GRADIENT");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void GradientMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("GRADIENT") ) {
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
			config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
			config.in_r = input.tag.get_property("IN_R", config.in_r);
			config.in_g = input.tag.get_property("IN_G", config.in_g);
			config.in_b = input.tag.get_property("IN_B", config.in_b);
			config.in_a = input.tag.get_property("IN_A", config.in_a);
			config.out_r = input.tag.get_property("OUT_R", config.out_r);
			config.out_g = input.tag.get_property("OUT_G", config.out_g);
			config.out_b = input.tag.get_property("OUT_B", config.out_b);
			config.out_a = input.tag.get_property("OUT_A", config.out_a);
			config.shape = input.tag.get_property("SHAPE", config.shape);
			config.center_x = input.tag.get_property("CENTER_X", config.center_x);
			config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
		}
	}
}

int GradientMain::handle_opengl()
{
#ifdef HAVE_GL
	const char *head_frag =
		"uniform sampler2D tex;\n"
		"uniform vec2 tex_dimensions;\n"
		"uniform vec2 center;\n"
		"uniform vec2 angle;\n"
		"uniform float gradient_size;\n"
		"uniform vec4 out_color;\n"
		"uniform vec4 in_color;\n"
		"uniform float in_radius;\n"
		"uniform float out_radius;\n"
		"uniform float radius_diff;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 out_coord = gl_TexCoord[0].st;\n"
		"	vec2 in_coord = out_coord * tex_dimensions - center;\n";

	const char *linear_shape =
		"	float mag = 0.5 + dot(in_coord,angle)/gradient_size;\n";

	const char *radial_shape =
		"	float mag = length(in_coord)/gradient_size;\n";

// No clamp function in NVidia
	const char *linear_rate =
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = (mag - in_radius) / radius_diff;\n";

// NVidia warns about exp, but exp is in the GLSL spec.
	const char *log_rate =
		"	mag = max(mag, in_radius);\n"
		"	float opacity = 1.0 - \n"
		"		exp(1.0 * -(mag - in_radius) / radius_diff);\n";

	const char *square_rate =
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = pow((mag - in_radius) / radius_diff, 2.0);\n"
		"	opacity = min(opacity, 1.0);\n";

	const char *tail_frag =
		"	vec4 color = mix(in_color, out_color, opacity);\n"
		"	vec4 bg_color = texture2D(tex, out_coord);\n"
		"	gl_FragColor.rgb = mix(bg_color.rgb, color.rgb, color.a);\n"
		"	gl_FragColor.a = mix(bg_color.a, 1., color.a);\n"
		"}\n";


	const char *shader_stack[16];
	memset(shader_stack,0, sizeof(shader_stack));
	int current_shader = 0;

	shader_stack[current_shader++] = head_frag;

	const char *shape_frag = 0;
	switch( config.shape ) {
	case GradientConfig::LINEAR:
		shape_frag = linear_shape;
		break;
	default:
		shape_frag = radial_shape;
		break;
	}
	if( shape_frag )
		shader_stack[current_shader++] = shape_frag;

	const char *rate_frag = 0;
	switch( config.rate ) {
	case GradientConfig::LINEAR:
		rate_frag = linear_rate;
		break;
	case GradientConfig::LOG:
		rate_frag = log_rate;
		break;
	case GradientConfig::SQUARE:
		rate_frag = square_rate;
		break;
	}
	if( rate_frag )
		shader_stack[current_shader++] = rate_frag;

	shader_stack[current_shader++] = tail_frag;

// Force frame to create texture without copying to it if full alpha.
	 if( config.in_a >= 0xff && config.out_a >= 0xff )
		get_output()->set_opengl_state(VFrame::TEXTURE);
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	shader_stack[current_shader] = 0;
	unsigned int shader = VFrame::make_shader(shader_stack);
	if( shader > 0 ) {
		glUseProgram(shader);
		float w = get_output()->get_w();
		float h = get_output()->get_h();
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		float texture_w = get_output()->get_texture_w();
		float texture_h = get_output()->get_texture_h();
		glUniform2f(glGetUniformLocation(shader, "tex_dimensions"),
			texture_w, texture_h);
		float u = config.shape == GradientConfig::LINEAR ?
			0.5f : config.center_x/100.f;
		float v = config.shape == GradientConfig::LINEAR ?
			0.5f : config.center_y/100.f;
		glUniform2f(glGetUniformLocation(shader, "center"),
			u * w, v * h);
		glUniform1f(glGetUniformLocation(shader, "gradient_size"),
			gradient_size);
		glUniform2f(glGetUniformLocation(shader, "angle"),
			-sin(config.angle * (M_PI / 180)), cos(config.angle * (M_PI / 180)));

		float in_radius = (float)config.in_radius/100;
		float out_radius = (float)config.out_radius/100;
		if( in_radius > out_radius ) {
			float r = in_radius;
			in_radius = out_radius;
			out_radius = r;
		}
		glUniform1f(glGetUniformLocation(shader, "in_radius"), in_radius);
		glUniform1f(glGetUniformLocation(shader, "out_radius"), out_radius);
		glUniform1f(glGetUniformLocation(shader, "radius_diff"),
			out_radius - in_radius);

		float in_r  = (float)config.in_r  / 0xff;
		float in_g  = (float)config.in_g  / 0xff;
		float in_b  = (float)config.in_b  / 0xff;
		float in_a  = (float)config.in_a  / 0xff;
		float out_r = (float)config.out_r / 0xff;
		float out_g = (float)config.out_g / 0xff;
		float out_b = (float)config.out_b / 0xff;
		float out_a = (float)config.out_a / 0xff;
		switch( get_output()->get_color_model() ) {
		case BC_YUV888:
		case BC_YUVA8888: {
			float in1, in2, in3, in4;
			float out1, out2, out3, out4;
			YUV::yuv.rgb_to_yuv_f(in_r,in_g,in_b, in1,in2,in3);
			in2  += 0.5;  in3 += 0.5;   in4 = in_a;
			YUV::yuv.rgb_to_yuv_f(out_r,out_g,out_b, out1,out2,out3);
			out2 += 0.5;  out3 += 0.5;  out4 = out_a;
			glUniform4f(glGetUniformLocation(shader, "out_color"),
				out1, out2, out3, out4);
			glUniform4f(glGetUniformLocation(shader, "in_color"),
				in1, in2, in3, in4);
			break; }

		default: {
			glUniform4f(glGetUniformLocation(shader, "out_color"),
				out_r, out_g, out_b, out_a);
			glUniform4f(glGetUniformLocation(shader, "in_color"),
				in_r, in_g, in_b, in_a);
			break; }
		}
	}

	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);

#endif
	return 0;
}


GradientPackage::GradientPackage()
 : LoadPackage()
{
}

GradientUnit::GradientUnit(GradientServer *server, GradientMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


static float calculate_opacity(float mag,
		float in_radius, float out_radius, int rate)
{
	float mag_diff = mag - in_radius;
	float radius_diff = out_radius - in_radius;
	if( !radius_diff ) return 1.0;
	float opacity = 0.0;
	switch( rate ) {
	case GradientConfig::LINEAR:
		opacity = mag < in_radius ? 0.0 : mag >= out_radius ? 1.0 :
				mag_diff / radius_diff;
		break;
	case GradientConfig::LOG:
		opacity = mag < in_radius ? 0.0 : // Let this one decay beyond out_radius
				1 - exp(1.0 * -mag_diff / radius_diff);
		break;
	case GradientConfig::SQUARE:
		opacity = mag < in_radius ? 0.0 : mag >= out_radius ? 1.0 :
				powf(mag_diff / radius_diff, 2.);
		break;
	}
	return opacity;
}

#define CREATE_GRADIENT(type, temp, components, max) { \
/* Synthesize linear gradient for lookups */ \
	int grad_size = plugin->gradient_size; \
	int n = sizeof(type) * (grad_size+1); \
	void *r_table = (void*) (plugin->table + 0*n); \
	void *g_table = (void*) (plugin->table + 1*n); \
	void *b_table = (void*) (plugin->table + 2*n); \
	void *a_table = (void*) (plugin->table + 3*n); \
	double grad_size2 = grad_size / 2.; \
 \
	if( plugin->need_reconfigure ) { \
		for( int i = 0; i <= grad_size; i++ ) { \
			float opacity = calculate_opacity(i, in_radius, out_radius, plugin->config.rate); \
			float transparency = 1.0 - opacity; \
			((type*)r_table)[i] = (type)(out1 * opacity + in1 * transparency); \
			((type*)g_table)[i] = (type)(out2 * opacity + in2 * transparency); \
			((type*)b_table)[i] = (type)(out3 * opacity + in3 * transparency); \
			((type*)a_table)[i] = (type)(out4 * opacity + in4 * transparency); \
		} \
	} \
 \
	for( int i = pkg->y1; i < pkg->y2; i++ ) { \
		double y = half_h - i; \
		type *gradient_row = (type*)plugin->gradient->get_rows()[i]; \
		type *out_row = (type*)plugin->get_output()->get_rows()[i]; \
		switch( plugin->config.shape ) { \
		case GradientConfig::LINEAR: \
			for( int j = 0; j < w; j++ ) { \
				double x = j - half_w; \
/* Rotate by effect angle */ \
				int mag = (grad_size2 - (x * sin_angle + y * cos_angle)) + 0.5; \
/* Get gradient value from these coords */ \
				if( sizeof(type) == 4 ) { \
					float opacity = calculate_opacity(mag, \
						in_radius, out_radius, plugin->config.rate); \
					float transparency = 1.0 - opacity; \
					gradient_row[0] = (type)(out1 * opacity + in1 * transparency); \
					gradient_row[1] = (type)(out2 * opacity + in2 * transparency); \
					gradient_row[2] = (type)(out3 * opacity + in3 * transparency); \
					if( components == 4 ) \
						gradient_row[3] = (type)(out4 * opacity + in4 * transparency); \
				} \
				else if( mag < 0 ) { \
					gradient_row[0] = in1; \
					gradient_row[1] = in2; \
					gradient_row[2] = in3; \
					if( components == 4 ) \
						gradient_row[3] = in4; \
				} \
				else if( mag >= grad_size ) { \
					gradient_row[0] = out1; \
					gradient_row[1] = out2; \
					gradient_row[2] = out3; \
					if( components == 4 ) \
						gradient_row[3] = out4; \
				} \
				else { \
					gradient_row[0] = ((type*)r_table)[mag]; \
					gradient_row[1] = ((type*)g_table)[mag]; \
					gradient_row[2] = ((type*)b_table)[mag]; \
					if( components == 4 ) \
						gradient_row[3] = ((type*)a_table)[mag]; \
				} \
/* no alpha output, Overlay mixed colormodels */ \
				if( gradient_cmodel != output_cmodel ) { \
					temp opacity = gradient_row[3]; \
					temp transparency = max - opacity; \
					out_row[0] = (transparency * out_row[0] + opacity * gradient_row[0]) / max; \
					out_row[1] = (transparency * out_row[1] + opacity * gradient_row[1]) / max; \
					out_row[2] = (transparency * out_row[2] + opacity * gradient_row[2]) / max; \
					out_row += 3; \
				} \
				gradient_row += components; \
			} \
			break; \
 \
		case GradientConfig::RADIAL: \
			for( int j = 0; j < w; j++ ) { \
				double x = j - center_x, y = i - center_y; \
				double magnitude = hypot(x, y); \
				int mag = (int)magnitude; \
				if( sizeof(type) == 4 ) { \
					float opacity = calculate_opacity(mag, \
							in_radius, out_radius, plugin->config.rate); \
					float transparency = 1.0 - opacity; \
					gradient_row[0] = (type)(out1 * opacity + in1 * transparency); \
					gradient_row[1] = (type)(out2 * opacity + in2 * transparency); \
					gradient_row[2] = (type)(out3 * opacity + in3 * transparency); \
					if( components == 4 ) \
						gradient_row[3] = (type)(out4 * opacity + in4 * transparency); \
				} \
				else { \
					gradient_row[0] = ((type*)r_table)[mag]; \
					gradient_row[1] = ((type*)g_table)[mag]; \
					gradient_row[2] = ((type*)b_table)[mag]; \
					if( components == 4 ) \
						gradient_row[3] = ((type*)a_table)[mag]; \
				} \
/* Overlay mixed colormodels onto output */ \
				if( gradient_cmodel != output_cmodel ) { \
					temp opacity = gradient_row[3]; \
					temp transparency = max - opacity; \
					out_row[0] = (transparency * out_row[0] + opacity * gradient_row[0]) / max; \
					out_row[1] = (transparency * out_row[1] + opacity * gradient_row[1]) / max; \
					out_row[2] = (transparency * out_row[2] + opacity * gradient_row[2]) / max; \
					out_row += 3; \
				} \
				gradient_row += components; \
			} \
			break; \
		} \
	} \
}

void GradientUnit::process_package(LoadPackage *package)
{
	GradientPackage *pkg = (GradientPackage*)package;
	int h = plugin->input->get_h();
	int w = plugin->input->get_w();
	int grad_size = plugin->gradient_size;
	int half_w = w / 2;
	int half_h = h / 2;
	int in_radius = (int)(plugin->config.in_radius / 100 * grad_size);
	int out_radius = (int)(plugin->config.out_radius / 100 * grad_size);
	double sin_angle = sin(plugin->config.angle * (M_PI / 180));
	double cos_angle = cos(plugin->config.angle * (M_PI / 180));
	double center_x = plugin->config.center_x * w / 100;
	double center_y = plugin->config.center_y * h / 100;
	int gradient_cmodel = plugin->gradient->get_color_model();
	int output_cmodel = plugin->get_output()->get_color_model();

	if( in_radius > out_radius ) {
		int r = in_radius;
		in_radius = out_radius;
		out_radius = r;
	}

	switch( gradient_cmodel ) {
	case BC_RGB888: {
		int in1 = plugin->config.in_r;
		int in2 = plugin->config.in_g;
		int in3 = plugin->config.in_b;
		int in4 = plugin->config.in_a;
		int out1 = plugin->config.out_r;
		int out2 = plugin->config.out_g;
		int out3 = plugin->config.out_b;
		int out4 = plugin->config.out_a;
		CREATE_GRADIENT(unsigned char, int, 3, 0xff)
		break; }

	case BC_RGBA8888: {
		int in1 = plugin->config.in_r;
		int in2 = plugin->config.in_g;
		int in3 = plugin->config.in_b;
		int in4 = plugin->config.in_a;
		int out1 = plugin->config.out_r;
		int out2 = plugin->config.out_g;
		int out3 = plugin->config.out_b;
		int out4 = plugin->config.out_a;
		CREATE_GRADIENT(unsigned char, int, 4, 0xff)
		break; }

	case BC_RGB_FLOAT: {
		float in1 = (float)plugin->config.in_r / 0xff;
		float in2 = (float)plugin->config.in_g / 0xff;
		float in3 = (float)plugin->config.in_b / 0xff;
		float in4 = (float)plugin->config.in_a / 0xff;
		float out1 = (float)plugin->config.out_r / 0xff;
		float out2 = (float)plugin->config.out_g / 0xff;
		float out3 = (float)plugin->config.out_b / 0xff;
		float out4 = (float)plugin->config.out_a / 0xff;
		CREATE_GRADIENT(float, float, 3, 1.0)
		break; }

	case BC_RGBA_FLOAT: {
		float in1 = (float)plugin->config.in_r / 0xff;
		float in2 = (float)plugin->config.in_g / 0xff;
		float in3 = (float)plugin->config.in_b / 0xff;
		float in4 = (float)plugin->config.in_a / 0xff;
		float out1 = (float)plugin->config.out_r / 0xff;
		float out2 = (float)plugin->config.out_g / 0xff;
		float out3 = (float)plugin->config.out_b / 0xff;
		float out4 = (float)plugin->config.out_a / 0xff;
		CREATE_GRADIENT(float, float, 4, 1.0)
		break; }

	case BC_YUV888: {
		int in_r = plugin->config.in_r;
		int in_g = plugin->config.in_g;
		int in_b = plugin->config.in_b;
		int in1, in2, in3, in4;
		int out1, out2, out3, out4;
		YUV::yuv.rgb_to_yuv_8(in_r,in_g,in_b, in1,in2,in3);
		in4 = plugin->config.in_a;
		int out_r = plugin->config.out_r;
		int out_g = plugin->config.out_g;
		int out_b = plugin->config.out_b;
		YUV::yuv.rgb_to_yuv_8(out_r,out_g,out_b, out1,out2,out3);
		out4 = plugin->config.out_a;
		CREATE_GRADIENT(unsigned char, int, 3, 0xff)
		break; }

	case BC_YUVA8888: {
		int in_r = plugin->config.in_r;
		int in_g = plugin->config.in_g;
		int in_b = plugin->config.in_b;
		int in1, in2, in3, in4;
		int out1, out2, out3, out4;
		YUV::yuv.rgb_to_yuv_8(in_r,in_g,in_b, in1,in2,in3);
		in4 = plugin->config.in_a;
		int out_r = plugin->config.out_r;
		int out_g = plugin->config.out_g;
		int out_b = plugin->config.out_b;
		YUV::yuv.rgb_to_yuv_8(out_r,out_g,out_b, out1,out2,out3);
		out4 = plugin->config.out_a;
		CREATE_GRADIENT(unsigned char, int, 4, 0xff)
		break; }
	}
}


GradientServer::GradientServer(GradientMain *plugin,
	int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void GradientServer::init_packages()
{
	for( int i = 0; i < get_total_packages(); i++ ) {
		GradientPackage *package = (GradientPackage*)get_package(i);
		package->y1 = plugin->input->get_h() *
			i /
			get_total_packages();
		package->y2 = plugin->input->get_h() *
			(i + 1) /
			get_total_packages();
	}
}

LoadClient* GradientServer::new_client()
{
	return new GradientUnit(this, plugin);
}

LoadPackage* GradientServer::new_package()
{
	return new GradientPackage;
}

