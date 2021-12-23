
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


#include "rotate.h"
#include "theme.h"

#define MAXANGLE 360.00
#define MINPIVOT   0.00
#define MAXPIVOT 100.00


REGISTER_PLUGIN(RotateEffect)




RotateConfig::RotateConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}

void RotateConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ANGLE :
			angle = 0.0;
			break;
		case RESET_PIVOT_X :
			pivot_x = 50.0;
			break;
		case RESET_PIVOT_Y :
			pivot_y = 50.0;
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			angle = 0.0;
			pivot_x = 50.0;
			pivot_y = 50.0;
			draw_pivot = 0;
			break;
	}
}

int RotateConfig::equivalent(RotateConfig &that)
{
	return EQUIV(angle, that.angle) &&
		EQUIV(pivot_x, that.pivot_y) &&
		EQUIV(pivot_y, that.pivot_y) &&
		draw_pivot == that.draw_pivot;
}

void RotateConfig::copy_from(RotateConfig &that)
{
	angle = that.angle;
	pivot_x = that.pivot_x;
	pivot_y = that.pivot_y;
	draw_pivot = that.draw_pivot;
//	bilinear = that.bilinear;
}

void RotateConfig::interpolate(RotateConfig &prev,
		RotateConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pivot_x = prev.pivot_x * prev_scale + next.pivot_x * next_scale;
	this->pivot_y = prev.pivot_y * prev_scale + next.pivot_y * next_scale;
	draw_pivot = prev.draw_pivot;
//	bilinear = prev.bilinear;
}











RotateToggle::RotateToggle(RotateWindow *window,
	RotateEffect *plugin,
	int init_value,
	int x,
	int y,
	int value,
	const char *string)
 : BC_Radial(x, y, init_value, string)
{
	this->value = value;
	this->plugin = plugin;
	this->window = window;
}

int RotateToggle::handle_event()
{
	plugin->config.angle = (float)value;
	window->update();
	plugin->send_configure_change();
	return 1;
}







RotateDrawPivot::RotateDrawPivot(RotateWindow *window,
	RotateEffect *plugin,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->config.draw_pivot, _("Draw pivot"))
{
	this->plugin = plugin;
	this->window = window;
}

int RotateDrawPivot::handle_event()
{
	plugin->config.draw_pivot = get_value();
	plugin->send_configure_change();
	return 1;
}





// RotateInterpolate::RotateInterpolate(RotateEffect *plugin, int x, int y)
//  : BC_CheckBox(x, y, plugin->config.bilinear, _("Interpolate"))
// {
// 	this->plugin = plugin;
// }
// int RotateInterpolate::handle_event()
// {
// 	plugin->config.bilinear = get_value();
// 	plugin->send_configure_change();
// 	return 1;
// }
//



RotateAngleText::RotateAngleText(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_TumbleTextBox(window, (float)plugin->config.angle,
	(float)-MAXANGLE, (float)MAXANGLE, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
}

int RotateAngleText::handle_event()
{
	plugin->config.angle = atof(get_text());
	window->update_toggles();
	window->update_sliders();
	plugin->send_configure_change();
	return 1;
}


RotateAngleSlider::RotateAngleSlider(RotateWindow *window, RotateEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, (float)-MAXANGLE, (float)MAXANGLE, (float)plugin->config.angle)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.1);
}

int RotateAngleSlider::handle_event()
{
	plugin->config.angle = get_value();
	window->update_toggles();
	window->update_texts();
	plugin->send_configure_change();
	return 1;
}



RotatePivotXText::RotatePivotXText(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_TumbleTextBox(window, (float)plugin->config.pivot_x,
	(float)MINPIVOT, (float)MAXPIVOT, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
}

int RotatePivotXText::handle_event()
{
	plugin->config.pivot_x = atof(get_text());
	window->update_sliders();
	plugin->send_configure_change();
	return 1;
}


RotatePivotXSlider::RotatePivotXSlider(RotateWindow *window, RotateEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, (float)MINPIVOT, (float)MAXPIVOT, (float)plugin->config.pivot_x)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.1);
}

int RotatePivotXSlider::handle_event()
{
	plugin->config.pivot_x = get_value();
	window->update_toggles();
	window->update_texts();
	plugin->send_configure_change();
	return 1;
}


RotatePivotYText::RotatePivotYText(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_TumbleTextBox(window, (float)plugin->config.pivot_y,
	(float)MINPIVOT, (float)MAXPIVOT, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
}

int RotatePivotYText::handle_event()
{
	plugin->config.pivot_y = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


RotatePivotYSlider::RotatePivotYSlider(RotateWindow *window, RotateEffect *plugin, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, (float)MINPIVOT, (float)MAXPIVOT, (float)plugin->config.pivot_y)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.1);
}

int RotatePivotYSlider::handle_event()
{
	plugin->config.pivot_y = get_value();
	window->update_toggles();
	window->update_texts();
	plugin->send_configure_change();
	return 1;
}



RotateClr::RotateClr(RotateWindow *window, RotateEffect *plugin, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->window = window;
	this->plugin = plugin;
	this->clear = clear;
}

RotateClr::~RotateClr()
{
}

int RotateClr::handle_event()
{
	plugin->config.reset(clear);
	window->update();
	plugin->send_configure_change();
	return 1;
}



RotateReset::RotateReset(RotateEffect *plugin, RotateWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
RotateReset::~RotateReset()
{
}
int RotateReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update();
	plugin->send_configure_change();
	return 1;
}






RotateWindow::RotateWindow(RotateEffect *plugin)
 : PluginClientWindow(plugin, xS(420), yS(260), xS(420), yS(260), 0)
{
	this->plugin = plugin;
}

#define RADIUS xS(30)

void RotateWindow::create_objects()
{
	int xs10 = xS(10), xs20 = xS(20), xs64 = xS(64), xs200 = xS(200);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30), ys40 = yS(40);
	int x2 = xS(80), x3 = xS(180);
	int x = xs10, y = ys10;
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_TitleBar *title_bar;
	BC_Bar *bar;

// Angle section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Rotation")));
	x = xs10;  y += ys20;
	add_tool(new BC_Title(x, y, _("Preset:")));
	x = x + x2; 
	add_tool(toggle180neg = new RotateToggle(this, plugin,
		plugin->config.angle == -180, x, y, -180, "-180°"));
	x += xs64;
	add_tool(toggle90neg = new RotateToggle(this, plugin,
		plugin->config.angle == -90, x, y, -90, "-90°"));
	x += xs64;
	add_tool(toggle0 = new RotateToggle(this, plugin,
		plugin->config.angle == 0, x, y, 0, "0°"));
	x += xs64;
	add_tool(toggle90 = new RotateToggle(this, plugin,
		plugin->config.angle == 90, x, y, 90, "+90°"));
	x += xs64;
	add_tool(toggle180 = new RotateToggle(this, plugin,
		plugin->config.angle == 180, x, y, 180, "+180°"));
//	add_subwindow(bilinear = new RotateInterpolate(plugin, xs10, y + ys60));
	x = xs10;  y += ys30;
	add_tool(new BC_Title(x, y, _("Angle:")));
	rotate_angle_text = new RotateAngleText(this, plugin, (x + x2), y);
	rotate_angle_text->create_objects();
	add_tool(rotate_angle_slider = new RotateAngleSlider(this, plugin, x3, y, xs200));
	add_tool(rotate_angle_clr = new RotateClr(this, plugin,
		clr_x, y, RESET_ANGLE));
	y += ys40;

// Pivot section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Pivot")));
	y += ys20;
	add_subwindow(draw_pivot = new RotateDrawPivot(this, plugin, x, y));
	y += ys30;
	add_tool(new BC_Title(x, y, _("X:")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	rotate_pivot_x_text = new RotatePivotXText(this, plugin, (x + x2), y);
	rotate_pivot_x_text->create_objects();
	add_tool(rotate_pivot_x_slider = new RotatePivotXSlider(this, plugin, x3, y, xs200));
	add_tool(rotate_pivot_x_clr = new RotateClr(this, plugin,
		clr_x, y, RESET_PIVOT_X));
	y += ys30;
	add_tool(new BC_Title(x, y, _("Y:")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	rotate_pivot_y_text = new RotatePivotYText(this, plugin, (x + x2), y);
	rotate_pivot_y_text->create_objects();
	add_tool(rotate_pivot_y_slider = new RotatePivotYSlider(this, plugin, x3, y, xs200));
	add_tool(rotate_pivot_y_clr = new RotateClr(this, plugin,
		clr_x, y, RESET_PIVOT_Y));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new RotateReset(plugin, this, x, y));

	show_window();
}



int RotateWindow::update()
{
	update_sliders();
	update_toggles();
	update_texts();
//	bilinear->update(plugin->config.bilinear);
	return 0;
}

int RotateWindow::update_sliders()
{
	rotate_angle_slider->update(plugin->config.angle);
	rotate_pivot_x_slider->update(plugin->config.pivot_x);
	rotate_pivot_y_slider->update(plugin->config.pivot_y);
	return 0;
}

int RotateWindow::update_texts()
{
	rotate_angle_text->update(plugin->config.angle);
	rotate_pivot_x_text->update(plugin->config.pivot_x);
	rotate_pivot_y_text->update(plugin->config.pivot_y);
	return 0;
}

int RotateWindow::update_toggles()
{
	toggle180neg->update(EQUIV(plugin->config.angle, -180));
	toggle90neg->update(EQUIV(plugin->config.angle, -90));
	toggle0->update(EQUIV(plugin->config.angle, 0));
	toggle90->update(EQUIV(plugin->config.angle, 90));
	toggle180->update(EQUIV(plugin->config.angle, 180));
	draw_pivot->update(plugin->config.draw_pivot);
	return 0;
}


































RotateEffect::RotateEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	need_reconfigure = 1;

}

RotateEffect::~RotateEffect()
{

	if(engine) delete engine;
}



const char* RotateEffect::plugin_title() { return N_("Rotate"); }
int RotateEffect::is_realtime() { return 1; }


NEW_WINDOW_MACRO(RotateEffect, RotateWindow)


void RotateEffect::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((RotateWindow*)thread->window)->update();
		thread->window->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(RotateEffect, RotateConfig)




void RotateEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("ROTATE");
	output.tag.set_property("ANGLE", (float)config.angle);
	output.tag.set_property("PIVOT_X", (float)config.pivot_x);
	output.tag.set_property("PIVOT_Y", (float)config.pivot_y);
	output.tag.set_property("DRAW_PIVOT", (int)config.draw_pivot);
//	output.tag.set_property("INTERPOLATE", (int)config.bilinear);
	output.append_tag();
	output.tag.set_title("/ROTATE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void RotateEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("ROTATE"))
			{
				config.angle = input.tag.get_property("ANGLE", (float)config.angle);
				config.pivot_x = input.tag.get_property("PIVOT_X", (float)config.pivot_x);
				config.pivot_y = input.tag.get_property("PIVOT_Y", (float)config.pivot_y);
				config.draw_pivot = input.tag.get_property("DRAW_PIVOT", (int)config.draw_pivot);
//				config.bilinear = input.tag.get_property("INTERPOLATE", (int)config.bilinear);
			}
		}
	}
}

int RotateEffect::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();
//printf("RotateEffect::process_buffer %d\n", __LINE__);


	if(config.angle == 0 && !config.draw_pivot)
	{
		read_frame(frame,
			0,
			start_position,
			frame_rate,
			get_use_opengl());
		return 1;
	}
//printf("RotateEffect::process_buffer %d\n", __LINE__);

	if(!engine) engine = new AffineEngine(PluginClient::smp + 1,
		PluginClient::smp + 1);
	int pivot_x = (int)(config.pivot_x * get_input()->get_w() / 100);
	int pivot_y = (int)(config.pivot_y * get_input()->get_h() / 100);
	engine->set_in_pivot(pivot_x, pivot_y);
	engine->set_out_pivot(pivot_x, pivot_y);


// Test
// engine->set_out_viewport(0, 0, 320, 240);
// engine->set_out_pivot(160, 120);

	if(get_use_opengl())
	{
		read_frame(frame,
			0,
			start_position,
			frame_rate,
			get_use_opengl());
		return run_opengl();
	}
//printf("RotateEffect::process_buffer %d\n", __LINE__);


// engine->set_viewport(50,
// 50,
// 100,
// 100);
// engine->set_pivot(100, 100);


	VFrame *temp_frame = PluginVClient::new_temp(get_input()->get_w(),
		get_input()->get_h(),
		get_input()->get_color_model());
	read_frame(temp_frame,
		0,
		start_position,
		frame_rate,
		get_use_opengl());
	frame->clear_frame();
	engine->rotate(frame,
		temp_frame,
		config.angle);

//printf("RotateEffect::process_buffer %d draw_pivot=%d\n", __LINE__, config.draw_pivot);

// Draw center
	if(config.draw_pivot) {
		VFrame *vframe = get_output();
		int w = vframe->get_w(), h = vframe->get_h();
		int mx = w > h ? w : h;
		int lw = mx/400 + 1, cxy = mx/80;
		int center_x = (int)(config.pivot_x * w/100);
		int center_y = (int)(config.pivot_y * h/100);
		int x1 = center_x - cxy, x2 = center_x + cxy;
		int y1 = center_y - cxy, y2 = center_y + cxy;
		vframe->set_pixel_color(WHITE);
		for( int i=0; i<lw; ++i )
			frame->draw_line(x1-i,center_y-i, x2-i,center_y-i);
		vframe->set_pixel_color(BLACK);
		for( int i=1; i<=lw; ++i )
			frame->draw_line(x1+i,center_y+i, x2+i,center_y+i);
		vframe->set_pixel_color(WHITE);
		for( int i=0; i<lw; ++i )
			frame->draw_line(center_x-i,y1-i, center_x-i,y2-i);
		vframe->set_pixel_color(BLACK);
		for( int i=1; i<=lw; ++i )
			frame->draw_line(center_x+i,y1+i, center_x+i,y2+i);
	}

// Conserve memory by deleting large frames
	if(get_input()->get_w() > PLUGIN_MAX_W &&
		get_input()->get_h() > PLUGIN_MAX_H)
	{
		delete engine;
		engine = 0;
	}
	return 0;
}



int RotateEffect::handle_opengl()
{
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->rotate(get_output(),
		get_output(),
		config.angle);
	engine->set_opengl(0);

	if(config.draw_pivot)
	{
		VFrame *vframe = get_output();
		int w = vframe->get_w(), h = vframe->get_h();
		int mx = w > h ? w : h;
		int lw = mx/400 + 1, cxy = mx/80;
		int center_x = (int)(config.pivot_x * w/100);
		int center_y = (int)(config.pivot_y * h/100);
		int x1 = center_x - cxy, x2 = center_x + cxy;
		int y1 = center_y - cxy, y2 = center_y + cxy;
		glDisable(GL_TEXTURE_2D);
		int is_yuv = BC_CModels::is_yuv(vframe->get_color_model());
		float rwt = 1, gwt = is_yuv? 0.5 : 1, bwt = is_yuv? 0.5 : 1;
		float rbk = 0, gbk = is_yuv? 0.5 : 0, bbk = is_yuv? 0.5 : 0;
		glBegin(GL_LINES);
		glColor4f(rwt, gwt, bwt, 1.0);
		for( int i=0; i<lw; ++i ) {
			glVertex3f(x1-i, center_y-i - h, 0.0);
			glVertex3f(x2-i, center_y-i - h, 0.0);
		}
		glColor4f(rbk, gbk, bbk, 1.0);
		for( int i=1; i<=lw; ++i ) {
			glVertex3f(x1+i, center_y+i - h, 0.0);
			glVertex3f(x2+i, center_y+i - h, 0.0);
		}
		glEnd();
		glBegin(GL_LINES);
		glColor4f(rwt, gwt, bwt, 1.0);
		for( int i=0; i<lw; ++i ) {
			glVertex3f(center_x-i, y1-i - h, 0.0);
			glVertex3f(center_x-i, y2-i - h, 0.0);
		}
		glColor4f(rbk, gbk, bbk, 1.0);
		for( int i=1; i<=lw; ++i ) {
			glVertex3f(center_x+i, y1+i - h, 0.0);
			glVertex3f(center_x+i, y2+i - h, 0.0);
		}
		glEnd();
	}
#endif
	return 0;
}


