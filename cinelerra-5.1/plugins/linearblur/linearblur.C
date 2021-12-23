
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "linearblur.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"






REGISTER_PLUGIN(LinearBlurMain)




LinearBlurConfig::LinearBlurConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}

void LinearBlurConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ALL :
			radius = 0;
			angle = 0;
			steps = 1;
			r = 1;
			g = 1;
			b = 1;
			a = 1;
			break;
		case RESET_RADIUS : radius = 0;
			break;
		case RESET_ANGLE : angle = 0;
			break;
		case RESET_STEPS : steps = 1;
			break;
		case RESET_DEFAULT_SETTINGS :
		default:
			radius = 10;
			angle = 0;
			steps = 10;
			r = 1;
			g = 1;
			b = 1;
			a = 1;
			break;
	}
}

int LinearBlurConfig::equivalent(LinearBlurConfig &that)
{
	return
		radius == that.radius &&
		angle == that.angle &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void LinearBlurConfig::copy_from(LinearBlurConfig &that)
{
	radius = that.radius;
	angle = that.angle;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void LinearBlurConfig::interpolate(LinearBlurConfig &prev,
	LinearBlurConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}












LinearBlurWindow::LinearBlurWindow(LinearBlurMain *plugin)
 : PluginClientWindow(plugin,
	xS(420),
	yS(200),
	xS(420),
	yS(200),
	0)
{
	this->plugin = plugin;
}

LinearBlurWindow::~LinearBlurWindow()
{
}

void LinearBlurWindow::create_objects()
{
	int xs10 = xS(10), xs100 = xS(100), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22
	int defaultBtn_w = xs100;

	BC_Bar *bar;

	y += ys10;
	add_subwindow(new BC_Title(x, y, _("Length:")));
	radius_text = new LinearBlurIText(this, plugin,
		0, &plugin->config.radius, (x + x2), y, RADIUS_MIN, RADIUS_MAX);
	radius_text->create_objects();
	radius_slider = new LinearBlurISlider(plugin,
		radius_text, &plugin->config.radius, x3, y, RADIUS_MIN, RADIUS_MAX, xs200);
	add_subwindow(radius_slider);
	radius_text->slider = radius_slider;
	clr_x = x3 + radius_slider->get_w() + x;
	add_subwindow(radius_Clr = new LinearBlurClr(plugin, this, clr_x, y, RESET_RADIUS));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Angle:")));
	angle_text = new LinearBlurIText(this, plugin,
		0, &plugin->config.angle, (x + x2), y, -ANGLE_MAX, ANGLE_MAX);
	angle_text->create_objects();
	angle_slider = new LinearBlurISlider(plugin,
		angle_text, &plugin->config.angle, x3, y, -ANGLE_MAX, ANGLE_MAX, xs200);
	add_subwindow(angle_slider);
	angle_text->slider = angle_slider;
	add_subwindow(angle_Clr = new LinearBlurClr(plugin, this, clr_x, y, RESET_ANGLE));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Steps:")));
	steps_text = new LinearBlurIText(this, plugin,
		0, &plugin->config.steps, (x + x2), y, STEPS_MIN, STEPS_MAX);
	steps_text->create_objects();
	steps_slider = new LinearBlurISlider(plugin,
		steps_text, &plugin->config.steps, x3, y, STEPS_MIN, STEPS_MAX, xs200);
	add_subwindow(steps_slider);
	steps_text->slider = steps_slider;
	add_subwindow(steps_Clr = new LinearBlurClr(plugin, this, clr_x, y, RESET_STEPS));
	y += ys40;

	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	int x1 = x;
	int toggle_w = (get_w()-2*x) / 4;
	add_subwindow(r = new LinearBlurToggle(plugin, x1, y, &plugin->config.r, _("Red")));
	x1 += toggle_w;
	add_subwindow(g = new LinearBlurToggle(plugin, x1, y, &plugin->config.g, _("Green")));
	x1 += toggle_w;
	add_subwindow(b = new LinearBlurToggle(plugin, x1, y, &plugin->config.b, _("Blue")));
	x1 += toggle_w;
	add_subwindow(a = new LinearBlurToggle(plugin, x1, y, &plugin->config.a, _("Alpha")));
	y += ys30;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new LinearBlurReset(plugin, this, x, y));
	add_subwindow(default_settings = new LinearBlurDefaultSettings(plugin, this,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}


// for Reset button
void LinearBlurWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_RADIUS :
			radius_text->update((int64_t)plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			break;
		case RESET_ANGLE :
			angle_text->update((int64_t)plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			break;
		case RESET_STEPS :
			steps_text->update((int64_t)plugin->config.steps);
			steps_slider->update(plugin->config.steps);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			radius_text->update((int64_t)plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			angle_text->update((int64_t)plugin->config.angle);
			angle_slider->update(plugin->config.angle);
			steps_text->update((int64_t)plugin->config.steps);
			steps_slider->update(plugin->config.steps);
			r->update(plugin->config.r);
			g->update(plugin->config.g);
			b->update(plugin->config.b);
			a->update(plugin->config.a);
			break;
	}
}










LinearBlurToggle::LinearBlurToggle(LinearBlurMain *plugin,
	int x,
	int y,
	int *output,
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int LinearBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}




LinearBlurIText::LinearBlurIText(LinearBlurWindow *gui, LinearBlurMain *plugin,
	LinearBlurISlider *slider, int *output, int x, int y, int min, int max)
 : BC_TumbleTextBox(gui, *output,
	min, max, x, y, xS(60), 0)
{
	this->gui = gui;
	this->plugin = plugin;
	this->output = output;
	this->slider = slider;
	this->min = min;
	this->max = max;
	set_increment(1);
}

LinearBlurIText::~LinearBlurIText()
{
}

int LinearBlurIText::handle_event()
{
	*output = atoi(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


LinearBlurISlider::LinearBlurISlider(LinearBlurMain *plugin,
	LinearBlurIText *text, int *output, int x, int y, int min, int max, int w)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

LinearBlurISlider::~LinearBlurISlider()
{
}

int LinearBlurISlider::handle_event()
{
	*output = get_value();
	text->update((int64_t)*output);
	plugin->send_configure_change();
	return 1;
}


LinearBlurSize::LinearBlurSize(LinearBlurMain *plugin,
	int x,
	int y,
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, xS(200), yS(200), min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}
int LinearBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


LinearBlurReset::LinearBlurReset(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
LinearBlurReset::~LinearBlurReset()
{
}
int LinearBlurReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	gui->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


LinearBlurDefaultSettings::LinearBlurDefaultSettings(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->gui = gui;
}
LinearBlurDefaultSettings::~LinearBlurDefaultSettings()
{
}
int LinearBlurDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	gui->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}


LinearBlurClr::LinearBlurClr(LinearBlurMain *plugin, LinearBlurWindow *gui, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->clear = clear;
}
LinearBlurClr::~LinearBlurClr()
{
}
int LinearBlurClr::handle_event()
{
	// clear==1 ==> Radius slider
	// clear==2 ==> Angle slider
	// clear==3 ==> Steps slider
	plugin->config.reset(clear);
	gui->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}








LinearBlurMain::LinearBlurMain(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
	accum = 0;
	need_reconfigure = 1;
	temp = 0;
	layer_table = 0;
}

LinearBlurMain::~LinearBlurMain()
{

	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	if(temp) delete temp;
}

const char* LinearBlurMain::plugin_title() { return N_("Linear Blur"); }
int LinearBlurMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(LinearBlurMain, LinearBlurWindow)
LOAD_CONFIGURATION_MACRO(LinearBlurMain, LinearBlurConfig)

void LinearBlurMain::delete_tables()
{
	if(scale_x_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_x_table[i];
		delete [] scale_x_table;
	}

	if(scale_y_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_y_table[i];
		delete [] scale_y_table;
	}
	delete [] layer_table;
	layer_table = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
}

int LinearBlurMain::process_buffer(VFrame *frame,
							int64_t start_position,
							double frame_rate)
{
	need_reconfigure |= load_configuration();

	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		get_use_opengl());

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure)
	{
		int w = frame->get_w();
		int h = frame->get_h();
		int x_offset;
		int y_offset;
		int angle = config.angle;
		int radius = config.radius * MIN(w, h) / 100;

		while(angle < 0) angle += 360;
		switch(angle)
		{
			case 0:
			case 360:
				x_offset = radius;
				y_offset = 0;
				break;
			case 90:
				x_offset = 0;
				y_offset = radius;
				break;
			case 180:
				x_offset = radius;
				y_offset = 0;
				break;
			case 270:
				x_offset = 0;
				y_offset = radius;
				break;
			default:
				y_offset = (int)(sin((float)config.angle / 360 * 2 * M_PI) * radius);
				x_offset = (int)(cos((float)config.angle / 360 * 2 * M_PI) * radius);
				break;
		}


		delete_tables();
		scale_x_table = new int*[config.steps];
		scale_y_table = new int*[config.steps];
		table_entries = config.steps;
		layer_table = new LinearBlurLayer[table_entries];

//printf("LinearBlurMain::process_realtime 1 %d %d %d\n", radius, x_offset, y_offset);

		for(int i = 0; i < config.steps; i++)
		{
			float fraction = (float)(i - config.steps / 2) / config.steps;
			int x = (int)(fraction * x_offset);
			int y = (int)(fraction * y_offset);

			int *x_table;
			int *y_table;
			scale_y_table[i] = y_table = new int[h];
			scale_x_table[i] = x_table = new int[w];

			layer_table[i].x = x;
			layer_table[i].y = y;
			for(int j = 0; j < h; j++)
			{
				y_table[j] = j + y;
				CLAMP(y_table[j], 0, h - 1);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = j + x;
				CLAMP(x_table[j], 0, w - 1);
			}
		}
		need_reconfigure = 0;
	}

	if(get_use_opengl()) return run_opengl();


	if(!engine) engine = new LinearBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	if(!accum) accum = new unsigned char[frame->get_w() *
		frame->get_h() *
		BC_CModels::components(frame->get_color_model()) *
		MAX(sizeof(int), sizeof(float))];

	this->input = frame;
	this->output = frame;


	if(!temp)
		temp = new VFrame(frame->get_w(), frame->get_h(),
			frame->get_color_model(), 0);
	temp->copy_from(frame);
	this->input = temp;


	bzero(accum,
		frame->get_w() *
		frame->get_h() *
		BC_CModels::components(frame->get_color_model()) *
		MAX(sizeof(int), sizeof(float)));
	engine->process_packages();
	return 0;
}


void LinearBlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((LinearBlurWindow*)thread->window)->lock_window();
		((LinearBlurWindow*)thread->window)->radius_text->update((int64_t)config.radius);
		((LinearBlurWindow*)thread->window)->radius_slider->update(config.radius);
		((LinearBlurWindow*)thread->window)->angle_text->update((int64_t)config.angle);
		((LinearBlurWindow*)thread->window)->angle_slider->update(config.angle);
		((LinearBlurWindow*)thread->window)->steps_text->update((int64_t)config.steps);
		((LinearBlurWindow*)thread->window)->steps_slider->update(config.steps);

		((LinearBlurWindow*)thread->window)->r->update(config.r);
		((LinearBlurWindow*)thread->window)->g->update(config.g);
		((LinearBlurWindow*)thread->window)->b->update(config.b);
		((LinearBlurWindow*)thread->window)->a->update(config.a);
		((LinearBlurWindow*)thread->window)->unlock_window();
	}
}



void LinearBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("LINEARBLUR");

	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/LINEARBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void LinearBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("LINEARBLUR"))
			{
				config.radius = input.tag.get_property("RADIUS", config.radius);
				config.angle = input.tag.get_property("ANGLE", config.angle);
				config.steps = input.tag.get_property("STEPS", config.steps);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
}

#ifdef HAVE_GL
static void draw_box(float x1, float y1, float x2, float y2)
{
	glBegin(GL_QUADS);
	glVertex3f(x1, y1, 0.0);
	glVertex3f(x2, y1, 0.0);
	glVertex3f(x2, y2, 0.0);
	glVertex3f(x1, y2, 0.0);
	glEnd();
}
#endif

int LinearBlurMain::handle_opengl()
{
#ifdef HAVE_GL
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	int is_yuv = BC_CModels::is_yuv(get_output()->get_color_model());
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

// Draw unselected channels
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDrawBuffer(GL_BACK);
	if(!config.r || !config.g || !config.b || !config.a)
	{
		glColor4f(config.r ? 0 : 1,
			config.g ? 0 : 1,
			config.b ? 0 : 1,
			config.a ? 0 : 1);
		get_output()->draw_texture();
	}
	glAccum(GL_LOAD, 1.0);

// Blur selected channels
	float fraction = 1.0 / config.steps;
	for(int i = 0; i < config.steps; i++)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0,
			config.g ? 1 : 0,
			config.b ? 1 : 0,
			config.a ? 1 : 0);

		int w = get_output()->get_w();
		int h = get_output()->get_h();
		get_output()->draw_texture(0,
			0,
			w,
			h,
			layer_table[i].x,
			get_output()->get_h() - layer_table[i].y,
			layer_table[i].x + w,
			get_output()->get_h() - layer_table[i].y - h,
			1);


// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(is_yuv)
		{
			glColor4f(config.r ? 0.0 : 0,
				config.g ? 0.5 : 0,
				config.b ? 0.5 : 0,
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			float project_x1 = layer_table[i].x;
			float project_x2 = layer_table[i].x + get_output()->get_w();
			float project_y1 = layer_table[i].y;
			float project_y2 = layer_table[i].y + get_output()->get_h();
			if(project_x1 > 0)
			{
				center_x1 = project_x1;
				draw_box(0, 0, project_x1, -get_output()->get_h());
			}
			if(project_x2 < get_output()->get_w())
			{
				center_x2 = project_x2;
				draw_box(project_x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(project_y1 > 0)
			{
				draw_box(center_x1,
					-get_output()->get_h(),
					center_x2,
					-get_output()->get_h() + project_y1);
			}
			if(project_y2 < get_output()->get_h())
			{
				draw_box(center_x1,
					-get_output()->get_h() + project_y2,
					center_x2,
					0);
			}
		}




		glAccum(GL_ACCUM, fraction);
		glEnable(GL_TEXTURE_2D);
		glColor4f(config.r ? 1 : 0,
			config.g ? 1 : 0,
			config.b ? 1 : 0,
			config.a ? 1 : 0);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glReadBuffer(GL_BACK);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}






LinearBlurPackage::LinearBlurPackage()
 : LoadPackage()
{
}




LinearBlurUnit::LinearBlurUnit(LinearBlurEngine *server,
	LinearBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, TEMP, MAX, DO_YUV) \
{ \
	for(int j = pkg->y1; j < pkg->y2; j++) \
	{ \
		TEMP *out_row = (TEMP*)plugin->accum + COMPONENTS * w * j; \
		int in_y = y_table[j]; \
 \
/* Blend image */ \
		TYPE *in_row = (TYPE*)plugin->input->get_rows()[in_y]; \
		for(int k = 0; k < w; k++) \
		{ \
			int in_x = x_table[k]; \
/* Blend pixel */ \
			int in_offset = in_x * COMPONENTS; \
			*out_row++ += in_row[in_offset]; \
			if(DO_YUV) \
			{ \
				*out_row++ += in_row[in_offset + 1]; \
				*out_row++ += in_row[in_offset + 2]; \
			} \
			else \
			{ \
				*out_row++ += in_row[in_offset + 1]; \
				*out_row++ += in_row[in_offset + 2]; \
			} \
			if(COMPONENTS == 4) \
				*out_row++ += in_row[in_offset + 3]; \
		} \
	} \
 \
/* Copy to output */ \
	if(i == plugin->config.steps - 1) \
	{ \
		for(int j = pkg->y1; j < pkg->y2; j++) \
		{ \
			TEMP *in_row = (TEMP*)plugin->accum + COMPONENTS * w * j; \
			TYPE *in_backup = (TYPE*)plugin->input->get_rows()[j]; \
			TYPE *out_row = (TYPE*)plugin->output->get_rows()[j]; \
			for(int k = 0; k < w; k++) \
			{ \
				if(do_r) \
				{ \
					*out_row++ = (*in_row++ * fraction) / 0x10000; \
					in_backup++; \
				} \
				else \
				{ \
					*out_row++ = *in_backup++; \
					in_row++; \
				} \
 \
				if(DO_YUV) \
				{ \
					if(do_g) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
 \
					if(do_b) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
				else \
				{ \
					if(do_g) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
 \
					if(do_b) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
 \
				if(COMPONENTS == 4) \
				{ \
					if(do_a) \
					{ \
						*out_row++ = (*in_row++ * fraction) / 0x10000; \
						in_backup++; \
					} \
					else \
					{ \
						*out_row++ = *in_backup++; \
						in_row++; \
					} \
				} \
			} \
		} \
	} \
}

void LinearBlurUnit::process_package(LoadPackage *package)
{
	LinearBlurPackage *pkg = (LinearBlurPackage*)package;
	//int h = plugin->output->get_h();
	int w = plugin->output->get_w();

	int fraction = 0x10000 / plugin->config.steps;
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;
	for(int i = 0; i < plugin->config.steps; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
			case BC_RGB_FLOAT:
				BLEND_LAYER(3, float, float, 1, 0)
				break;
			case BC_RGB888:
				BLEND_LAYER(3, uint8_t, int, 0xff, 0)
				break;
			case BC_RGBA_FLOAT:
				BLEND_LAYER(4, float, float, 1, 0)
				break;
			case BC_RGBA8888:
				BLEND_LAYER(4, uint8_t, int, 0xff, 0)
				break;
			case BC_RGB161616:
				BLEND_LAYER(3, uint16_t, int, 0xffff, 0)
				break;
			case BC_RGBA16161616:
				BLEND_LAYER(4, uint16_t, int, 0xffff, 0)
				break;
			case BC_YUV888:
				BLEND_LAYER(3, uint8_t, int, 0xff, 1)
				break;
			case BC_YUVA8888:
				BLEND_LAYER(4, uint8_t, int, 0xff, 1)
				break;
			case BC_YUV161616:
				BLEND_LAYER(3, uint16_t, int, 0xffff, 1)
				break;
			case BC_YUVA16161616:
				BLEND_LAYER(4, uint16_t, int, 0xffff, 1)
				break;
		}
	}
}






LinearBlurEngine::LinearBlurEngine(LinearBlurMain *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void LinearBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		LinearBlurPackage *package = (LinearBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* LinearBlurEngine::new_client()
{
	return new LinearBlurUnit(this, plugin);
}

LoadPackage* LinearBlurEngine::new_package()
{
	return new LinearBlurPackage;
}





