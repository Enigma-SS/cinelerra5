
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


#include "radialblur.h"



REGISTER_PLUGIN(RadialBlurMain)



RadialBlurConfig::RadialBlurConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}


void RadialBlurConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ALL :
			x = 50;
			y = 50;
			angle = 0;
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
		case RESET_ANGLE : angle = 0;
			break;
		case RESET_STEPS : steps = 1;
			break;
		case RESET_DEFAULT_SETTINGS :
		default:
			x = 50;
			y = 50;
			angle = 33;
			steps = 10;
			r = 1;
			g = 1;
			b = 1;
			a = 1;
			break;
	}
}

int RadialBlurConfig::equivalent(RadialBlurConfig &that)
{
	return
		angle == that.angle &&
		x == that.x &&
		y == that.y &&
		steps == that.steps &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void RadialBlurConfig::copy_from(RadialBlurConfig &that)
{
	x = that.x;
	y = that.y;
	angle = that.angle;
	steps = that.steps;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void RadialBlurConfig::interpolate(RadialBlurConfig &prev,
	RadialBlurConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}













RadialBlurWindow::RadialBlurWindow(RadialBlurMain *plugin)
 : PluginClientWindow(plugin,
	xS(420),
	yS(230),
	xS(420),
	yS(230),
	0)
{
	this->plugin = plugin;
}

RadialBlurWindow::~RadialBlurWindow()
{
}

void RadialBlurWindow::create_objects()
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
	x_text = new RadialBlurIText(this, plugin,
		0, &plugin->config.x, (x + x2), y, XY_MIN, XY_MAX);
	x_text->create_objects();
	x_slider = new RadialBlurISlider(plugin,
		x_text, &plugin->config.x, x3, y, XY_MIN, XY_MAX, xs200);
	add_subwindow(x_slider);
	x_text->slider = x_slider;
	clr_x = x3 + x_slider->get_w() + x;
	add_subwindow(x_Clr = new RadialBlurClr(plugin, this, clr_x, y, RESET_XSLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Y:")));
	y_text = new RadialBlurIText(this, plugin,
		0, &plugin->config.y, (x + x2), y, XY_MIN, XY_MAX);
	y_text->create_objects();
	y_slider = new RadialBlurISlider(plugin,
		y_text, &plugin->config.y, x3, y, XY_MIN, XY_MAX, xs200);
	add_subwindow(y_slider);
	y_text->slider = y_slider;
	add_subwindow(y_Clr = new RadialBlurClr(plugin, this, clr_x, y, RESET_YSLIDER));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Angle:")));
	angle_text = new RadialBlurIText(this, plugin,
		0, &plugin->config.angle, (x + x2), y, ANGLE_MIN, ANGLE_MAX);
	angle_text->create_objects();
	angle_slider = new RadialBlurISlider(plugin,
		angle_text, &plugin->config.angle, x3, y, ANGLE_MIN, ANGLE_MAX, xs200);
	add_subwindow(angle_slider);
	angle_text->slider = angle_slider;
	add_subwindow(angle_Clr = new RadialBlurClr(plugin, this, clr_x, y, RESET_ANGLE));
	y += ys30;

	add_subwindow(new BC_Title(x, y, _("Steps:")));
	steps_text = new RadialBlurIText(this, plugin,
		0, &plugin->config.steps, (x + x2), y, STEPS_MIN, STEPS_MAX);
	steps_text->create_objects();
	steps_slider = new RadialBlurISlider(plugin,
		steps_text, &plugin->config.steps, x3, y, STEPS_MIN, STEPS_MAX, xs200);
	add_subwindow(steps_slider);
	steps_text->slider = steps_slider;
	add_subwindow(steps_Clr = new RadialBlurClr(plugin, this, clr_x, y, RESET_STEPS));
	y += ys40;

	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	int x1 = x;
	int toggle_w = (get_w()-2*x) / 4;
	add_subwindow(r = new RadialBlurToggle(plugin, x1, y, &plugin->config.r, _("Red")));
	x1 += toggle_w;
	add_subwindow(g = new RadialBlurToggle(plugin, x1, y, &plugin->config.g, _("Green")));
	x1 += toggle_w;
	add_subwindow(b = new RadialBlurToggle(plugin, x1, y, &plugin->config.b, _("Blue")));
	x1 += toggle_w;
	add_subwindow(a = new RadialBlurToggle(plugin, x1, y, &plugin->config.a, _("Alpha")));
	y += ys30;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new RadialBlurReset(plugin, this, x, y));
	add_subwindow(default_settings = new RadialBlurDefaultSettings(plugin, this,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}

// for Reset button
void RadialBlurWindow::update_gui(int clear)
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
			x_text->update((int64_t)plugin->config.x);
			x_slider->update(plugin->config.x);
			y_text->update((int64_t)plugin->config.y);
			y_slider->update(plugin->config.y);
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









RadialBlurToggle::RadialBlurToggle(RadialBlurMain *plugin,
	int x,
	int y,
	int *output,
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int RadialBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


RadialBlurIText::RadialBlurIText(RadialBlurWindow *gui, RadialBlurMain *plugin,
	RadialBlurISlider *slider, int *output, int x, int y, int min, int max)
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

RadialBlurIText::~RadialBlurIText()
{
}

int RadialBlurIText::handle_event()
{
	*output = atoi(get_text());
	if(*output > max) *output = max;
	if(*output < min) *output = min;
	slider->update(*output);
	plugin->send_configure_change();
	return 1;
}


RadialBlurISlider::RadialBlurISlider(RadialBlurMain *plugin,
	RadialBlurIText *text, int *output, int x, int y, int min, int max, int w)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
	this->text = text;
	enable_show_value(0); // Hide caption
}

RadialBlurISlider::~RadialBlurISlider()
{
}

int RadialBlurISlider::handle_event()
{
	*output = get_value();
	text->update((int64_t)*output);
	plugin->send_configure_change();
	return 1;
}


RadialBlurReset::RadialBlurReset(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
RadialBlurReset::~RadialBlurReset()
{
}
int RadialBlurReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	gui->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


RadialBlurDefaultSettings::RadialBlurDefaultSettings(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->gui = gui;
}
RadialBlurDefaultSettings::~RadialBlurDefaultSettings()
{
}
int RadialBlurDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	gui->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}


RadialBlurClr::RadialBlurClr(RadialBlurMain *plugin, RadialBlurWindow *gui, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->clear = clear;
}
RadialBlurClr::~RadialBlurClr()
{
}
int RadialBlurClr::handle_event()
{
	// clear==1 ==> X slider
	// clear==2 ==> Y slider
	// clear==3 ==> Angle slider
	// clear==4 ==> Steps slider
	plugin->config.reset(clear);
	gui->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}




RadialBlurMain::RadialBlurMain(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
	temp = 0;
	rotate = 0;
}

RadialBlurMain::~RadialBlurMain()
{

	if(engine) delete engine;
	if(temp) delete temp;
	delete rotate;
}

const char* RadialBlurMain::plugin_title() { return N_("Radial Blur"); }
int RadialBlurMain::is_realtime() { return 1; }



NEW_WINDOW_MACRO(RadialBlurMain, RadialBlurWindow)

LOAD_CONFIGURATION_MACRO(RadialBlurMain, RadialBlurConfig)

int RadialBlurMain::process_buffer(VFrame *frame,
							int64_t start_position,
							double frame_rate)
{
	load_configuration();


	read_frame(frame,
		0,
		get_source_position(),
		get_framerate(),
//		0);
		get_use_opengl());

	if(get_use_opengl()) return run_opengl();

	if(!engine) engine = new RadialBlurEngine(this,
		get_project_smp() + 1,
		get_project_smp() + 1);

	this->input = frame;
	this->output = frame;


	if(!temp)
		temp = new VFrame(frame->get_w(), frame->get_h(),
			frame->get_color_model(), 0);
	temp->copy_from(frame);
	this->input = temp;

	engine->process_packages();
	return 0;
}


void RadialBlurMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((RadialBlurWindow*)thread->window)->x_text->update((int64_t)config.x);
		((RadialBlurWindow*)thread->window)->x_slider->update(config.x);
		((RadialBlurWindow*)thread->window)->y_text->update((int64_t)config.y);
		((RadialBlurWindow*)thread->window)->y_slider->update(config.y);
		((RadialBlurWindow*)thread->window)->angle_text->update((int64_t)config.angle);
		((RadialBlurWindow*)thread->window)->angle_slider->update(config.angle);
		((RadialBlurWindow*)thread->window)->steps_text->update((int64_t)config.steps);
		((RadialBlurWindow*)thread->window)->steps_slider->update(config.steps);

		((RadialBlurWindow*)thread->window)->r->update(config.r);
		((RadialBlurWindow*)thread->window)->g->update(config.g);
		((RadialBlurWindow*)thread->window)->b->update(config.b);
		((RadialBlurWindow*)thread->window)->a->update(config.a);
		thread->window->unlock_window();
	}
}




void RadialBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("RADIALBLUR");

	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/RADIALBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void RadialBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("RADIALBLUR"))
			{
				config.x = input.tag.get_property("X", config.x);
				config.y = input.tag.get_property("Y", config.y);
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

int RadialBlurMain::handle_opengl()
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
		get_output()->set_opengl_state(VFrame::TEXTURE);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0,
			config.g ? 1 : 0,
			config.b ? 1 : 0,
			config.a ? 1 : 0);

		float w = get_output()->get_w();
		float h = get_output()->get_h();



		double current_angle = (double)config.angle *
			i /
			config.steps -
			(double)config.angle / 2;

		if(!rotate) rotate = new AffineEngine(PluginClient::smp + 1,
			PluginClient::smp + 1);
		rotate->set_in_pivot((int)(config.x * w / 100),
			(int)(config.y * h / 100));
		rotate->set_out_pivot((int)(config.x * w / 100),
			(int)(config.y * h / 100));
		rotate->set_opengl(1);
		rotate->rotate(get_output(),
			get_output(),
			current_angle);

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











RadialBlurPackage::RadialBlurPackage()
 : LoadPackage()
{
}


RadialBlurUnit::RadialBlurUnit(RadialBlurEngine *server,
	RadialBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}


#define BLEND_LAYER(COMPONENTS, TYPE, TEMP_TYPE, MAX, DO_YUV) \
{ \
	int chroma_offset = (DO_YUV ? ((MAX + 1) / 2) : 0); \
	TYPE **in_rows = (TYPE**)plugin->input->get_rows(); \
	TYPE **out_rows = (TYPE**)plugin->output->get_rows(); \
	int steps = plugin->config.steps; \
	double step = (double)plugin->config.angle / 360 * 2 * M_PI / steps; \
 \
	for(int i = pkg->y1, out_y = pkg->y1 - center_y; \
		i < pkg->y2; \
		i++, out_y++) \
	{ \
		TYPE *out_row = out_rows[i]; \
		TYPE *in_row = in_rows[i]; \
		int y_square = out_y * out_y; \
 \
		for(int j = 0, out_x = -center_x; j < w; j++, out_x++) \
		{ \
			TEMP_TYPE accum_r = 0; \
			TEMP_TYPE accum_g = 0; \
			TEMP_TYPE accum_b = 0; \
			TEMP_TYPE accum_a = 0; \
 \
/* Output coordinate to polar */ \
			double magnitude = sqrt(y_square + out_x * out_x); \
			double angle; \
			if(out_y < 0) \
				angle = atan((double)out_x / out_y) + M_PI; \
			else \
			if(out_y > 0) \
				angle = atan((double)out_x / out_y); \
			else \
			if(out_x > 0) \
				angle = M_PI / 2; \
			else \
				angle = M_PI * 1.5; \
 \
/* Overlay all steps on this pixel*/ \
			angle -= (double)plugin->config.angle / 360 * M_PI; \
			for(int k = 0; k < steps; k++, angle += step) \
			{ \
/* Polar to input coordinate */ \
				int in_x = (int)(magnitude * sin(angle)) + center_x; \
				int in_y = (int)(magnitude * cos(angle)) + center_y; \
 \
/* Accumulate input coordinate */ \
				if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h) \
				{ \
					accum_r += in_rows[in_y][in_x * COMPONENTS]; \
					if(DO_YUV) \
					{ \
						accum_g += (int)in_rows[in_y][in_x * COMPONENTS + 1]; \
						accum_b += (int)in_rows[in_y][in_x * COMPONENTS + 2]; \
					} \
					else \
					{ \
						accum_g += in_rows[in_y][in_x * COMPONENTS + 1]; \
						accum_b += in_rows[in_y][in_x * COMPONENTS + 2]; \
					} \
					if(COMPONENTS == 4) \
						accum_a += in_rows[in_y][in_x * COMPONENTS + 3]; \
				} \
				else \
				{ \
					accum_g += chroma_offset; \
					accum_b += chroma_offset; \
				} \
			} \
 \
/* Accumulation to output */ \
			if(do_r) \
			{ \
				*out_row++ = (accum_r * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_g) \
			{ \
				if(DO_YUV) \
					*out_row++ = ((accum_g * fraction) / 0x10000); \
				else \
					*out_row++ = (accum_g * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(do_b) \
			{ \
				if(DO_YUV) \
					*out_row++ = (accum_b * fraction) / 0x10000; \
				else \
					*out_row++ = (accum_b * fraction) / 0x10000; \
				in_row++; \
			} \
			else \
			{ \
				*out_row++ = *in_row++; \
			} \
 \
			if(COMPONENTS == 4) \
			{ \
				if(do_a) \
				{ \
					*out_row++ = (accum_a * fraction) / 0x10000; \
					in_row++; \
				} \
				else \
				{ \
					*out_row++ = *in_row++; \
				} \
			} \
		} \
	} \
}

void RadialBlurUnit::process_package(LoadPackage *package)
{
	RadialBlurPackage *pkg = (RadialBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do_r = plugin->config.r;
	int do_g = plugin->config.g;
	int do_b = plugin->config.b;
	int do_a = plugin->config.a;
	int fraction = 0x10000 / plugin->config.steps;
	int center_x = plugin->config.x * w / 100;
	int center_y = plugin->config.y * h / 100;

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			BLEND_LAYER(3, uint8_t, int, 0xff, 0)
			break;
		case BC_RGBA8888:
			BLEND_LAYER(4, uint8_t, int, 0xff, 0)
			break;
		case BC_RGB_FLOAT:
			BLEND_LAYER(3, float, float, 1, 0)
			break;
		case BC_RGBA_FLOAT:
			BLEND_LAYER(4, float, float, 1, 0)
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






RadialBlurEngine::RadialBlurEngine(RadialBlurMain *plugin,
	int total_clients,
	int total_packages)
 : LoadServer(total_clients, total_packages)
// : LoadServer(1, 1)
{
	this->plugin = plugin;
}

void RadialBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		RadialBlurPackage *package = (RadialBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* RadialBlurEngine::new_client()
{
	return new RadialBlurUnit(this, plugin);
}

LoadPackage* RadialBlurEngine::new_package()
{
	return new RadialBlurPackage;
}





