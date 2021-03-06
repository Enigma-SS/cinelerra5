
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

#include "bccolors.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "huesaturation.h"
#include "guicast.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "playback3d.h"
#include "pluginvclient.h"
#include "theme.h"
#include "vframe.h"

#include <stdint.h>
#include <string.h>



REGISTER_PLUGIN(HueEffect)





HueConfig::HueConfig()
{
	reset(RESET_ALL);
}

void HueConfig::reset(int clear)
{
	switch(clear) {
		case RESET_HUV : hue = 0;
			break;
		case RESET_SAT : saturation = 0;
			break;
		case RESET_VAL : value = 0;
			break;
		case RESET_ALL :
		default:
			hue = saturation = value = 0;
			break;
	}
}

void HueConfig::copy_from(HueConfig &src)
{
	hue = src.hue;
	saturation = src.saturation;
	value = src.value;
}
int HueConfig::equivalent(HueConfig &src)
{
	return EQUIV(hue, src.hue) &&
		EQUIV(saturation, src.saturation) &&
		EQUIV(value, src.value);
}
void HueConfig::interpolate(HueConfig &prev,
	HueConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->hue = prev.hue * prev_scale + next.hue * next_scale;
	this->saturation = prev.saturation * prev_scale + next.saturation * next_scale;
	this->value = prev.value * prev_scale + next.value * next_scale;
}






/* SATURATION VALUES
  saturation is stored    from -100.00  to +100.00
  saturation_slider  goes from -100.00  to +100.00
  saturation_caption goes from    0.000 to   +2.000 (clear to +1.000)
  saturation_text    goes from -100.00  to +100.00
*/
/* VALUE VALUES
  value is stored    from -100.00  to +100.00
  value_slider  goes from -100.00  to +100.00
  value_caption goes from    0.000 to   +2.000 (clear to +1.000)
  value_text    goes from -100.00  to +100.00
*/

HueText::HueText(HueEffect *plugin, HueWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, plugin->config.hue,
	(float)MINHUE, (float)MAXHUE, x, y, xS(60), 2)
{
	this->gui = gui;
	this->plugin = plugin;
	set_increment(0.01);
}

HueText::~HueText()
{
}

int HueText::handle_event()
{
	float min = MINHUE, max = MAXHUE;
	float output = atof(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.hue = output;
	gui->hue_slider->update(plugin->config.hue);
	plugin->send_configure_change();
	return 1;
}

HueSlider::HueSlider(HueEffect *plugin, HueWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w,
	(float)MINHUE, (float)MAXHUE,
	plugin->config.hue)
{
	this->plugin = plugin;
	this->gui = gui;
	enable_show_value(0); // Hide caption
}
int HueSlider::handle_event()
{
	plugin->config.hue = get_value();
	gui->hue_text->update(plugin->config.hue);
	plugin->send_configure_change();
	return 1;
}


SaturationText::SaturationText(HueEffect *plugin, HueWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, plugin->config.saturation,
	(float)MINSATURATION, (float)MAXSATURATION, x, y, xS(60), 2)
{
	this->gui = gui;
	this->plugin = plugin;
	set_increment(0.01);
}

SaturationText::~SaturationText()
{
}

int SaturationText::handle_event()
{
	float min = MINSATURATION, max = MAXSATURATION;
	float output = atof(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.saturation = output;
	gui->sat_slider->update(plugin->config.saturation);
	plugin->send_configure_change();
	return 1;
}

SaturationSlider::SaturationSlider(HueEffect *plugin, HueWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w,
	(float)MINSATURATION, (float)MAXSATURATION,
	plugin->config.saturation)
{
	this->plugin = plugin;
	this->gui = gui;
	enable_show_value(0); // Hide caption
}
int SaturationSlider::handle_event()
{
	plugin->config.saturation = get_value();
	gui->sat_text->update(plugin->config.saturation);
	plugin->send_configure_change();
	return 1;
}

char* SaturationSlider::get_caption()
{
	float fraction = ((float)plugin->config.saturation - MINSATURATION) /
		MAXSATURATION;
	sprintf(string, "%0.4f", fraction);
	return string;
}


ValueText::ValueText(HueEffect *plugin, HueWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, plugin->config.value,
	(float)MINVALUE, (float)MAXVALUE, x, y, xS(60), 2)
{
	this->gui = gui;
	this->plugin = plugin;
	set_increment(0.01);
}

ValueText::~ValueText()
{
}

int ValueText::handle_event()
{
	float min = MINVALUE, max = MAXVALUE;
	float output = atof(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.value = output;
	gui->value_slider->update(plugin->config.value);
	plugin->send_configure_change();
	return 1;
}

ValueSlider::ValueSlider(HueEffect *plugin, HueWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w,
	(float)MINVALUE, (float)MAXVALUE,
	plugin->config.value)
{
	this->plugin = plugin;
	this->gui = gui;
	enable_show_value(0); // Hide caption
}
int ValueSlider::handle_event()
{
	plugin->config.value = get_value();
	gui->value_text->update(plugin->config.value);
	plugin->send_configure_change();
	return 1;
}

char* ValueSlider::get_caption()
{
	float fraction = ((float)plugin->config.value - MINVALUE) / MAXVALUE;
	sprintf(string, "%0.4f", fraction);
	return string;
}


HueReset::HueReset(HueEffect *plugin, HueWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
HueReset::~HueReset()
{
}
int HueReset::handle_event()
{
	plugin->config.reset(RESET_ALL); // clear=0 ==> reset all
	gui->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


HueClr::HueClr(HueEffect *plugin, HueWindow *gui, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->clear = clear;
}
HueClr::~HueClr()
{
}
int HueClr::handle_event()
{
	// clear==1 ==> Hue
	// clear==2 ==> Saturation
	// clear==3 ==> Value
	plugin->config.reset(clear);
	gui->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}




HueWindow::HueWindow(HueEffect *plugin)
 : PluginClientWindow(plugin, xS(420), yS(160), xS(420), yS(160), 0)
{
	this->plugin = plugin;
}
void HueWindow::create_objects()
{
	int xs10 = xS(10), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Bar *bar;

// Hue
	y += ys10;
	add_subwindow(new BC_Title(x, y, _("Hue:")));
	hue_text = new HueText(plugin, this, (x + x2), y);
	hue_text->create_objects();
	add_subwindow(hue_slider = new HueSlider(plugin, this, x3, y, xs200));
	clr_x = x3 + hue_slider->get_w() + x;
	add_subwindow(hue_clr = new HueClr(plugin, this, clr_x, y, RESET_HUV));
	y += ys30;

// Saturation
	add_subwindow(new BC_Title(x, y, _("Saturation:")));
	sat_text = new SaturationText(plugin, this, (x + x2), y);
	sat_text->create_objects();
	add_subwindow(sat_slider = new SaturationSlider(plugin, this, x3, y, xs200));
	add_subwindow(sat_clr = new HueClr(plugin, this, clr_x, y, RESET_SAT));
	y += ys30;

// Value
	add_subwindow(new BC_Title(x, y, _("Value:")));
	value_text = new ValueText(plugin, this, (x + x2), y);
	value_text->create_objects();
	add_subwindow(value_slider = new ValueSlider(plugin, this, x3, y, xs200));
	add_subwindow(value_clr = new HueClr(plugin, this, clr_x, y, RESET_VAL));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new HueReset(plugin, this, x, y));
	show_window();
	flush();
}


// for Reset button
void HueWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_HUV :
			hue_text->update(plugin->config.hue);
			hue_slider->update(plugin->config.hue);
			break;
		case RESET_SAT :
			sat_text->update(plugin->config.saturation);
			sat_slider->update(plugin->config.saturation);
			break;
		case RESET_VAL :
			value_text->update(plugin->config.value);
			value_slider->update(plugin->config.value);
			break;
		case RESET_ALL :
		default:
			hue_text->update(plugin->config.hue);
			hue_slider->update(plugin->config.hue);
			sat_text->update(plugin->config.saturation);
			sat_slider->update(plugin->config.saturation);
			value_text->update(plugin->config.value);
			value_slider->update(plugin->config.value);
			break;
	}
}








HueEngine::HueEngine(HueEffect *plugin, int cpus)
 : LoadServer(cpus, cpus)
{
	this->plugin = plugin;
}
void HueEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		HuePackage *pkg = (HuePackage*)get_package(i);
		pkg->row1 = plugin->input->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = plugin->input->get_h() * (i + 1) / LoadServer::get_total_packages();
	}
}
LoadClient* HueEngine::new_client()
{
	return new HueUnit(plugin, this);
}
LoadPackage* HueEngine::new_package()
{
	return new HuePackage;
}








HuePackage::HuePackage()
 : LoadPackage()
{
}

HueUnit::HueUnit(HueEffect *plugin, HueEngine *server)
 : LoadClient(server)
{
	this->plugin = plugin;
}







#define HUESATURATION(type, max, components, use_yuv) \
{ \
	float h_offset = plugin->config.hue; \
	float s_offset = ((float)plugin->config.saturation - MINSATURATION) / MAXSATURATION; \
	float v_offset = ((float)plugin->config.value - MINVALUE) / MAXVALUE; \
	for(int i = pkg->row1; i < pkg->row2; i++) \
	{ \
		type* in_row = (type*)plugin->input->get_rows()[i]; \
		type* out_row = (type*)plugin->output->get_rows()[i]; \
 \
		for(int j = 0; j < w; j++) \
		{ \
			float h, s, va; \
			int y, u, v; \
			float r, g, b; \
			int r_i, g_i, b_i; \
 \
			if(use_yuv) \
			{ \
				y = (int)in_row[0]; \
				u = (int)in_row[1]; \
				v = (int)in_row[2]; \
				if(max == 0xffff) \
					YUV::yuv.yuv_to_rgb_16(r_i, g_i, b_i, y, u, v); \
				else \
					YUV::yuv.yuv_to_rgb_8(r_i, g_i, b_i, y, u, v); \
				HSV::rgb_to_hsv((float)r_i / max, \
					(float)g_i / max, \
					(float)b_i / max, \
					h, \
					s, \
					va); \
			} \
			else \
			{ \
				r = (float)in_row[0] / max; \
				g = (float)in_row[1] / max; \
				b = (float)in_row[2] / max; \
				HSV::rgb_to_hsv(r, g, b, h, s, va); \
			} \
 \
 \
			h += h_offset; \
			s *= s_offset; \
			va *= v_offset; \
 \
			if(h >= 360) h -= 360; \
			if(h < 0) h += 360; \
			if(sizeof(type) < 4) \
			{ \
				if(s > 1) s = 1; \
				if(va > 1) va = 1; \
				if(s < 0) s = 0; \
				if(va < 0) va = 0; \
			} \
 \
			if(use_yuv) \
			{ \
				HSV::hsv_to_yuv(y, u, v, h, s, va, max); \
				out_row[0] = y; \
				out_row[1] = u; \
				out_row[2] = v; \
			} \
			else \
			{ \
				HSV::hsv_to_rgb(r, g, b, h, s, va); \
				if(sizeof(type) < 4) \
				{ \
					r *= max; \
					g *= max; \
					b *= max; \
					out_row[0] = (type)CLIP(r, 0, max); \
					out_row[1] = (type)CLIP(g, 0, max); \
					out_row[2] = (type)CLIP(b, 0, max); \
				} \
				else \
				{ \
					out_row[0] = (type)r; \
					out_row[1] = (type)g; \
					out_row[2] = (type)b; \
				} \
			} \
 \
			in_row += components; \
			out_row += components; \
		} \
	} \
}


void HueUnit::process_package(LoadPackage *package)
{
	HuePackage *pkg = (HuePackage*)package;
	int w = plugin->input->get_w();

	switch(plugin->input->get_color_model())
	{
		case BC_RGB888:
			HUESATURATION(unsigned char, 0xff, 3, 0)
			break;

		case BC_RGB_FLOAT:
			HUESATURATION(float, 1, 3, 0)
			break;

		case BC_YUV888:
			HUESATURATION(unsigned char, 0xff, 3, 1)
			break;

		case BC_RGB161616:
			HUESATURATION(uint16_t, 0xffff, 3, 0)
			break;

		case BC_YUV161616:
			HUESATURATION(uint16_t, 0xffff, 3, 1)
			break;

		case BC_RGBA_FLOAT:
			HUESATURATION(float, 1, 4, 0)
			break;

		case BC_RGBA8888:
			HUESATURATION(unsigned char, 0xff, 4, 0)
			break;

		case BC_YUVA8888:
			HUESATURATION(unsigned char, 0xff, 4, 1)
			break;

		case BC_RGBA16161616:
			HUESATURATION(uint16_t, 0xffff, 4, 0)
			break;

		case BC_YUVA16161616:
			HUESATURATION(uint16_t, 0xffff, 4, 1)
			break;

	}
}






HueEffect::HueEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;

}
HueEffect::~HueEffect()
{

	if(engine) delete engine;
}

int HueEffect::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	read_frame(frame,
		0,
		start_position,
		frame_rate,
		get_use_opengl());


	this->input = frame;
	this->output = frame;
	if(EQUIV(config.hue, 0) && EQUIV(config.saturation, 0) && EQUIV(config.value, 0))
	{
		return 0;
	}
	else
	{
		if(get_use_opengl())
		{
			run_opengl();
			return 0;
		}

		if(!engine) engine = new HueEngine(this, PluginClient::smp + 1);

		engine->process_packages();
	}
	return 0;
}

const char* HueEffect::plugin_title() { return N_("Hue saturation"); }
int HueEffect::is_realtime() { return 1; }

NEW_WINDOW_MACRO(HueEffect, HueWindow)
LOAD_CONFIGURATION_MACRO(HueEffect, HueConfig)


void HueEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("HUESATURATION");
	output.tag.set_property("HUE", config.hue);
	output.tag.set_property("SATURATION", config.saturation);
	output.tag.set_property("VALUE", config.value);
	output.append_tag();
	output.tag.set_title("/HUESATURATION");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}
void HueEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	while(!input.read_tag())
	{
		if(input.tag.title_is("HUESATURATION"))
		{
			config.hue = input.tag.get_property("HUE", config.hue);
			config.saturation = input.tag.get_property("SATURATION", config.saturation);
			config.value = input.tag.get_property("VALUE", config.value);
		}
	}
}
void HueEffect::update_gui()
{
	if(thread)
	{
		((HueWindow*)thread->window)->lock_window();
		load_configuration();
		((HueWindow*)thread->window)->hue_text->update(config.hue);
		((HueWindow*)thread->window)->hue_slider->update(config.hue);
		((HueWindow*)thread->window)->sat_text->update(config.saturation);
		((HueWindow*)thread->window)->sat_slider->update(config.saturation);
		((HueWindow*)thread->window)->value_text->update(config.value);
		((HueWindow*)thread->window)->value_slider->update(config.value);
		((HueWindow*)thread->window)->unlock_window();
	}
}

int HueEffect::handle_opengl()
{
#ifdef HAVE_GL
	const char *yuv_saturation_frag =
		"uniform sampler2D tex;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
		"	pixel.r *= v_offset;\n"
		"	pixel.gb -= vec2(0.5, 0.5);\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= s_offset;\n"
		"	pixel.gb += vec2(0.5, 0.5);\n"
		"	gl_FragColor = pixel;\n"
		"}\n";


	const char *yuv_frag =
		"uniform sampler2D tex;\n"
		"uniform float h_offset;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
			YUV_TO_RGB_FRAG("pixel")
			RGB_TO_HSV_FRAG("pixel")
		"	pixel.r += h_offset;\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= v_offset;\n"
		"	if(pixel.r >= 360.0) pixel.r -= 360.0;\n"
		"	if(pixel.r < 0.0) pixel.r += 360.0;\n"
			HSV_TO_RGB_FRAG("pixel")
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";

	const char *rgb_frag =
		"uniform sampler2D tex;\n"
		"uniform float h_offset;\n"
		"uniform float s_offset;\n"
		"uniform float v_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 pixel = texture2D(tex, gl_TexCoord[0].st);\n"
			RGB_TO_HSV_FRAG("pixel")
		"	pixel.r += h_offset;\n"
		"	pixel.g *= s_offset;\n"
		"	pixel.b *= v_offset;\n"
		"	if(pixel.r >= 360.0) pixel.r -= 360.0;\n"
		"	if(pixel.r < 0.0) pixel.r += 360.0;\n"
			HSV_TO_RGB_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";


	get_output()->to_texture();
	get_output()->enable_opengl();

	const char *shader_stack[16];
	memset(shader_stack,0, sizeof(shader_stack));
	int current_shader = 0;

	int need_color_matrix = BC_CModels::is_yuv(get_output()->get_color_model()) ? 1 : 0;
	if( need_color_matrix ) shader_stack[current_shader++] = bc_gl_colors;
	shader_stack[current_shader++] = !need_color_matrix ? rgb_frag :
		EQUIV(config.hue, 0) ? yuv_saturation_frag: yuv_frag ;

	shader_stack[current_shader] = 0;
        unsigned int shader = VFrame::make_shader(shader_stack);
	if(shader > 0) {
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		glUniform1f(glGetUniformLocation(shader, "h_offset"), config.hue);
		glUniform1f(glGetUniformLocation(shader, "s_offset"),
			((float)config.saturation - MINSATURATION) / MAXSATURATION);
		glUniform1f(glGetUniformLocation(shader, "v_offset"),
			((float)config.value - MINVALUE) / MAXVALUE);
		if( need_color_matrix ) BC_GL_COLORS(shader);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);
	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}





