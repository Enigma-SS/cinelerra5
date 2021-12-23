
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


#include "zoomblur.h"



REGISTER_PLUGIN(ZoomBlurMain)



ZoomBlurConfig::ZoomBlurConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}

void ZoomBlurConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ALL :
			x = 50;
			y = 50;
			radius = 0;
			steps = 1;
			r = 1;
			g = 1;
			b = 1;
			a = 1;
			break;
		case RESET_XSLIDER : x = 50;
			break;
		case RESET_YSLIDER : y = 50;
			break;
		case RESET_RADIUS : radius = 0;
			break;
		case RESET_STEPS : steps = 1;
			break;
		case RESET_DEFAULT_SETTINGS :
		default:
			x = 50;
			y = 50;
			radius = 10;
			steps = 10;
			r = 1;
			g = 1;
			b = 1;
			a = 1;
			break;
	}
}

int ZoomBlurConfig::equivalent(ZoomBlurConfig &that)
{
	return
		x == that.x &&
		y == that.y &&
		radius == that.radius &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void ZoomBlurConfig::copy_from(ZoomBlurConfig &that)
{
	x = that.x;
	y = that.y;
	radius = that.radius;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void ZoomBlurConfig::interpolate(ZoomBlurConfig &prev,
	ZoomBlurConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}












ZoomBlurWindow::ZoomBlurWindow(ZoomBlurMain *plugin)
 : PluginClientWindow(plugin,
	xS(420),
	yS(230),
	xS(420),
	yS(230),
	0)
{
	this->plugin = plugin;
}

ZoomBlurWindow::~ZoomBlurWindow()
{
}

void ZoomBlurWindow::create_objects()
{
	int xs10 = xS(10), xs100 = xS(100), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22
	int defaultBtn_w = xs100;

	BC_Bar *bar;

	y += ys10;
	add_subwindow(new BC_Title(x, y, _("X:")));
	x_text = new ZoomBlurIText(this, plugin,
		0, &plugin->config.x, (x + x2), y, XY_MIN, XY_MAX);
	x_text->create_objects();
	x_slider = new ZoomBlurISlider(plugin,
		x_text, &plugin->config.x, x3, y, XY_MIN, XY_MAX, xs200);
	add_subwindow(x_slider);
	x_text->slider = x_slider;
	clr_x = x3 + x_slider->get_w() + x;
	add_subwindow(x_Clr = new ZoomBlurClr(plugin, this, clr_x, y, RESET_XSLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Y:")));
	y_text = new ZoomBlurIText(this, plugin,
		0, &plugin->config.y, (x + x2), y, XY_MIN, XY_MAX);
	y_text->create_objects();
	y_slider = new ZoomBlurISlider(plugin,
		y_text, &plugin->config.y, x3, y, XY_MIN, XY_MAX, xs200);
	add_subwindow(y_slider);
	y_text->slider = y_slider;
	add_subwindow(y_Clr = new ZoomBlurClr(plugin, this, clr_x, y, RESET_YSLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Radius:")));
	radius_text = new ZoomBlurIText(this, plugin,
		0, &plugin->config.radius, (x + x2), y, -RADIUS_MAX, RADIUS_MAX);
	radius_text->create_objects();
	radius_slider = new ZoomBlurISlider(plugin,
		radius_text, &plugin->config.radius, x3, y, -RADIUS_MAX, RADIUS_MAX, xs200);
	add_subwindow(radius_slider);
	radius_text->slider = radius_slider;
	add_subwindow(radius_Clr = new ZoomBlurClr(plugin, this, clr_x, y, RESET_RADIUS));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Steps:")));
	steps_text = new ZoomBlurIText(this, plugin,
		0, &plugin->config.steps, (x + x2), y, STEPS_MIN, STEPS_MAX);
	steps_text->create_objects();
	steps_slider = new ZoomBlurISlider(plugin,
		steps_text, &plugin->config.steps, x3, y, STEPS_MIN, STEPS_MAX, xs200);
	add_subwindow(steps_slider);
	steps_text->slider = steps_slider;
	add_subwindow(steps_Clr = new ZoomBlurClr(plugin, this, clr_x, y, RESET_STEPS));
	y += ys40;

	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	int x1 = x;
	int toggle_w = (get_w()-2*x) / 4;
	add_subwindow(r = new ZoomBlurToggle(plugin, x1, y, &plugin->config.r, _("Red")));
	x1 += toggle_w;
	add_subwindow(g = new ZoomBlurToggle(plugin, x1, y, &plugin->config.g, _("Green")));
	x1 += toggle_w;
	add_subwindow(b = new ZoomBlurToggle(plugin, x1, y, &plugin->config.b, _("Blue")));
	x1 += toggle_w;
	add_subwindow(a = new ZoomBlurToggle(plugin, x1, y, &plugin->config.a, _("Alpha")));
	y += ys30;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new ZoomBlurReset(plugin, this, x, y));
	add_subwindow(default_settings = new ZoomBlurDefaultSettings(plugin, this,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}

// for Reset button
void ZoomBlurWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_XSLIDER :
			x_text->update((int64_t)plugin->config.x);
			x_slider->update(plugin->config.x);
			break;
		case RESET_YSLIDER :
			y_text->update((int64_t)plugin->config.y);
			y_slider->update(plugin->config.y);
			break;
		case RESET_RADIUS :
			radius_text->update((int64_t)plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			break;
		case RESET_STEPS :
			steps_text->update((int64_t)plugin->config.steps);
			steps_slider->update(plugin->config.steps);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			x_text->update((int64_t)plugin->config.x);
			x_slider->update(plugin->config.x);
			y_text->update((int64_t)plugin->config.y);
			y_slider->update(plugin->config.y);
			radius_text->update((int64_t)plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			steps_text->update((int64_t)plugin->config.steps);
			steps_slider->update(plugin->config.steps);
			r->update(plugin->config.r);
			g->update(plugin->config.g);
			b->update(plugin->config.b);
			a->update(plugin->config.a);
			break;
	}
}










ZoomBlurToggle::ZoomBlurToggle(ZoomBlurMain *plugin,
	int x,
	int y,
	int *output,
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int ZoomBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


ZoomBlurIText::ZoomBlurIText(ZoomBlurWindow *window, ZoomBlurMain *plugin,
	ZoomBlurISlider *slider, int *output, int x, int y, int min, int max)
 : BC_TumbleTextBox(window, *output,
	min, max, x, y, xS(60), 0)
{
	this->window = window;
	this->plugin = plugin;
	this->output = output;
	this->slider = slider;
	this->min = min;
	this->max = max;
	set_increment(1);
}

ZoomBlurIText::~ZoomBlurIText()
{
}

int ZoomBlurIText::handle_event()
{
	*output = atoi(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


ZoomBlurISlider::ZoomBlurISlider(ZoomBlurMain *plugin,
	ZoomBlurIText *text, int *output, int x, int y, int min, int max, int w)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

ZoomBlurISlider::~ZoomBlurISlider()
{
}

int ZoomBlurISlider::handle_event()
{
	*output = get_value();
	text->update((int64_t)*output);
	plugin->send_configure_change();
	return 1;
}


ZoomBlurReset::ZoomBlurReset(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
ZoomBlurReset::~ZoomBlurReset()
{
}
int ZoomBlurReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


ZoomBlurDefaultSettings::ZoomBlurDefaultSettings(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->window = window;
}
ZoomBlurDefaultSettings::~ZoomBlurDefaultSettings()
{
}
int ZoomBlurDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	window->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}


ZoomBlurClr::ZoomBlurClr(ZoomBlurMain *plugin, ZoomBlurWindow *window, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
ZoomBlurClr::~ZoomBlurClr()
{
}
int ZoomBlurClr::handle_event()
{
	// clear==1 ==> X slider
	// clear==2 ==> Y slider
	// clear==3 ==> Radius slider
	// clear==4 ==> Steps slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}




ZoomBlurMain::ZoomBlurMain(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
	accum = 0;
	need_reconfigure = 1;
	temp = 0;
}

ZoomBlurMain::~ZoomBlurMain()
{

	if(engine) delete engine;
	delete_tables();
	if(accum) delete [] accum;
	if(temp) delete temp;
}

const char* ZoomBlurMain::plugin_title() { return N_("Zoom Blur"); }
int ZoomBlurMain::is_realtime() { return 1; }



NEW_WINDOW_MACRO(ZoomBlurMain, ZoomBlurWindow)

LOAD_CONFIGURATION_MACRO(ZoomBlurMain, ZoomBlurConfig)

void ZoomBlurMain::delete_tables()
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
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
}

int ZoomBlurMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	need_reconfigure |= load_configuration();


SET_TRACE
	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
		get_use_opengl());

SET_TRACE

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(need_reconfigure)
	{
SET_TRACE
		float w = frame->get_w();
		float h = frame->get_h();
		float center_x = (float)config.x / 100 * w;
		float center_y = (float)config.y / 100 * h;
		float radius = (float)(100 + config.radius) / 100;
		float min_w, min_h;
		//float max_w, max_h;
		int steps = config.steps ? config.steps : 1;
		float min_x1;
		float min_y1;
		float min_x2;
		float min_y2;
		float max_x1;
		float max_y1;
		float max_x2;
		float max_y2;

SET_TRACE

// printf("ZoomBlurMain::process_realtime 1 %d %d\n",
// config.x,
// config.y);

		center_x = (center_x - w / 2) * (1.0 - radius) + w / 2;
		center_y = (center_y - h / 2) * (1.0 - radius) + h / 2;
		min_w = w * radius;
		min_h = h * radius;
		//max_w = w;
		//max_h = h;
		min_x1 = center_x - min_w / 2;
		min_y1 = center_y - min_h / 2;
		min_x2 = center_x + min_w / 2;
		min_y2 = center_y + min_h / 2;
		max_x1 = 0;
		max_y1 = 0;
		max_x2 = w;
		max_y2 = h;

SET_TRACE
// printf("ZoomBlurMain::process_realtime 2 w=%f radius=%f center_x=%f\n",
// w,
// radius,
// center_x);


// Dimensions of outermost rectangle

		delete_tables();
		table_entries = steps;
		scale_x_table = new int*[steps];
		scale_y_table = new int*[steps];
		layer_table = new ZoomBlurLayer[table_entries];

SET_TRACE
		for(int i = 0; i < steps; i++)
		{
			float fraction = (float)i / steps;
			float inv_fraction = 1.0 - fraction;
			float out_x1 = min_x1 * fraction + max_x1 * inv_fraction;
			float out_x2 = min_x2 * fraction + max_x2 * inv_fraction;
			float out_y1 = min_y1 * fraction + max_y1 * inv_fraction;
			float out_y2 = min_y2 * fraction + max_y2 * inv_fraction;
			float out_w = out_x2 - out_x1;
			float out_h = out_y2 - out_y1;
			if(out_w < 0) out_w = 0;
			if(out_h < 0) out_h = 0;
			float scale_x = (float)w / out_w;
			float scale_y = (float)h / out_h;

			int *x_table;
			int *y_table;
			scale_y_table[i] = y_table = new int[(int)(h + 1)];
			scale_x_table[i] = x_table = new int[(int)(w + 1)];
SET_TRACE
			layer_table[i].x1 = out_x1;
			layer_table[i].y1 = out_y1;
			layer_table[i].x2 = out_x2;
			layer_table[i].y2 = out_y2;
SET_TRACE

			for(int j = 0; j < h; j++)
			{
				y_table[j] = (int)((j - out_y1) * scale_y);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = (int)((j - out_x1) * scale_x);
//printf("ZoomBlurMain::process_realtime %d %d\n", j, x_table[j]);
			}
		}
SET_TRACE
		need_reconfigure = 0;
	}

SET_TRACE
	if(get_use_opengl()) return run_opengl();

SET_TRACE



	if(!engine) engine = new ZoomBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);
	if(!accum) accum = new unsigned char[frame->get_w() *
		frame->get_h() *
		BC_CModels::components(frame->get_color_model()) *
		MAX(sizeof(int), sizeof(float))];

	this->input = frame;
	this->output = frame;


	if(!temp) temp = new VFrame(frame->get_w(), frame->get_h(),
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


void ZoomBlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((ZoomBlurWindow*)thread->window)->x_text->update((int64_t)config.x);
		((ZoomBlurWindow*)thread->window)->x_slider->update(config.x);
		((ZoomBlurWindow*)thread->window)->y_text->update((int64_t)config.y);
		((ZoomBlurWindow*)thread->window)->y_slider->update(config.y);
		((ZoomBlurWindow*)thread->window)->radius_text->update((int64_t)config.radius);
		((ZoomBlurWindow*)thread->window)->radius_slider->update(config.radius);
		((ZoomBlurWindow*)thread->window)->steps_text->update((int64_t)config.steps);
		((ZoomBlurWindow*)thread->window)->steps_slider->update(config.steps);
		((ZoomBlurWindow*)thread->window)->r->update(config.r);
		((ZoomBlurWindow*)thread->window)->g->update(config.g);
		((ZoomBlurWindow*)thread->window)->b->update(config.b);
		((ZoomBlurWindow*)thread->window)->a->update(config.a);
		thread->window->unlock_window();
	}
}





void ZoomBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("ZOOMBLUR");

	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/ZOOMBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void ZoomBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ZOOMBLUR"))
			{
				config.x = input.tag.get_property("X", config.x);
				config.y = input.tag.get_property("Y", config.y);
				config.radius = input.tag.get_property("RADIUS", config.radius);
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

int ZoomBlurMain::handle_opengl()
{
#ifdef HAVE_GL
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	//int is_yuv = BC_CModels::is_yuv(get_output()->get_color_model());
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

		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			layer_table[i].x1,
			get_output()->get_h() - layer_table[i].y1,
			layer_table[i].x2,
			get_output()->get_h() - layer_table[i].y2,
			1);

// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(BC_CModels::is_yuv(get_output()->get_color_model()))
		{
			glColor4f(config.r ? 0.0 : 0,
				config.g ? 0.5 : 0,
				config.b ? 0.5 : 0,
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			if(layer_table[i].x1 > 0)
			{
				center_x1 = layer_table[i].x1;
				draw_box(0, 0, layer_table[i].x1, -get_output()->get_h());
			}
			if(layer_table[i].x2 < get_output()->get_w())
			{
				center_x2 = layer_table[i].x2;
				draw_box(layer_table[i].x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(layer_table[i].y1 > 0)
			{
				draw_box(center_x1,
					-get_output()->get_h(),
					center_x2,
					-get_output()->get_h() + layer_table[i].y1);
			}
			if(layer_table[i].y2 < get_output()->get_h())
			{
				draw_box(center_x1,
					-get_output()->get_h() + layer_table[i].y2,
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
	glReadBuffer(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}














ZoomBlurPackage::ZoomBlurPackage()
 : LoadPackage()
{
}




ZoomBlurUnit::ZoomBlurUnit(ZoomBlurEngine *server,
	ZoomBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, TEMP_TYPE, MAX, DO_YUV) \
{ \
	const int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	for(int j = pkg->y1; j < pkg->y2; j++) \
	{ \
		TEMP_TYPE *out_row = (TEMP_TYPE*)plugin->accum + COMPONENTS * w * j; \
		int in_y = y_table[j]; \
 \
/* Blend image */ \
		if(in_y >= 0 && in_y < h) \
		{ \
			TYPE *in_row = (TYPE*)plugin->input->get_rows()[in_y]; \
			for(int k = 0; k < w; k++) \
			{ \
				int in_x = x_table[k]; \
/* Blend pixel */ \
				if(in_x >= 0 && in_x < w) \
				{ \
					int in_offset = in_x * COMPONENTS; \
					*out_row++ += in_row[in_offset]; \
					if(DO_YUV) \
					{ \
						*out_row++ += in_row[in_offset + 1]; \
						*out_row++ += in_row[in_offset + 2]; \
					} \
					else \
					{ \
						*out_row++ += (TEMP_TYPE)in_row[in_offset + 1]; \
						*out_row++ += (TEMP_TYPE)in_row[in_offset + 2]; \
					} \
					if(COMPONENTS == 4) \
						*out_row++ += in_row[in_offset + 3]; \
				} \
/* Blend nothing */ \
				else \
				{ \
					out_row++; \
					if(DO_YUV) \
					{ \
						*out_row++ += chroma_offset; \
						*out_row++ += chroma_offset; \
					} \
					else \
					{ \
						out_row += 2; \
					} \
					if(COMPONENTS == 4) out_row++; \
				} \
			} \
		} \
		else \
		if(DO_YUV) \
		{ \
			for(int k = 0; k < w; k++) \
			{ \
				out_row++; \
				*out_row++ += chroma_offset; \
				*out_row++ += chroma_offset; \
				if(COMPONENTS == 4) out_row++; \
			} \
		} \
	} \
 \
/* Copy just selected blurred channels to output and combine with original \
	unblurred channels */ \
	if(i == plugin->config.steps - 1) \
	{ \
		for(int j = pkg->y1; j < pkg->y2; j++) \
		{ \
			TEMP_TYPE *in_row = (TEMP_TYPE*)plugin->accum + COMPONENTS * w * j; \
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
						*out_row++ = ((*in_row++ * fraction) / 0x10000); \
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
						*out_row++ = ((*in_row++ * fraction) / 0x10000); \
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

void ZoomBlurUnit::process_package(LoadPackage *package)
{
	ZoomBlurPackage *pkg = (ZoomBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;

	int fraction = 0x10000 / plugin->config.steps;
	for(int i = 0; i < plugin->config.steps; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
			case BC_RGB888:
				BLEND_LAYER(3, uint8_t, int, 0xff, 0)
				break;
			case BC_RGB_FLOAT:
				BLEND_LAYER(3, float, float, 1, 0)
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






ZoomBlurEngine::ZoomBlurEngine(ZoomBlurMain *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void ZoomBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ZoomBlurPackage *package = (ZoomBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ZoomBlurEngine::new_client()
{
	return new ZoomBlurUnit(this, plugin);
}

LoadPackage* ZoomBlurEngine::new_package()
{
	return new ZoomBlurPackage;
}





