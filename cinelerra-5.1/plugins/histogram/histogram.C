
/*
 * CINELERRA
 * Copyright (C) 2008-2012 Adam Williams <broadcast at earthling dot net>
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
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcprogressbox.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "filexml.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "playback3d.h"
#include "pluginserver.h"
#include "bccolors.h"
#include "vframe.h"
#include "workarounds.h"

#include "aggregated.h"
#include "../colorbalance/aggregated.h"
#include "../interpolate/aggregated.h"
#include "../gamma/aggregated.h"

class HistogramMain;
class HistogramEngine;
class HistogramWindow;


REGISTER_PLUGIN(HistogramMain)

HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		lookup[i] = new int[0x10000];
		preview_lookup[i] = new int[0x10000];
		accum[i] = new int64_t[HISTOGRAM_SLOTS];
		bzero(accum[i], sizeof(int64_t)*HISTOGRAM_SLOTS);
	}
	current_point = -1;
	mode = HISTOGRAM_VALUE;
	last_position = -1;
	need_reconfigure = 1;
	sum_frames = 0;
	frames = 1;
	dragging_point = 0;
	input = 0;
	output = 0;
	w = 440;
	h = 500;
	parade = 0;
}

HistogramMain::~HistogramMain()
{

	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		delete [] lookup[i];
		delete [] preview_lookup[i];
		delete [] accum[i];
	}
	delete engine;
}

const char* HistogramMain::plugin_title() { return N_("Histogram"); }
int HistogramMain::is_realtime() { return 1; }



NEW_WINDOW_MACRO(HistogramMain, HistogramWindow)

LOAD_CONFIGURATION_MACRO(HistogramMain, HistogramConfig)

void HistogramMain::render_gui(void *data)
{
	if( !thread ) return;
	HistogramWindow *window = (HistogramWindow*)thread->window;
	HistogramMain *plugin = (HistogramMain*)data;
//update gui client instance, needed for drawing
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		config.low_input[i] = plugin->config.low_input[i];
		config.high_input[i] = plugin->config.high_input[i];
		memcpy(accum[i], plugin->accum[i], HISTOGRAM_SLOTS*sizeof(*accum));
	}
	window->lock_window("HistogramMain::render_gui 2");
// draw all if reconfigure
	int reconfig = plugin->need_reconfigure;
// Always draw the histogram but don't update widgets if automatic
	int auto_rgb = reconfig ? 1 : config.automatic && mode != HISTOGRAM_VALUE ? 1 : 0;
	window->update(1, auto_rgb, auto_rgb, reconfig);
	window->unlock_window();
}

void HistogramMain::update_gui()
{
	if( thread ) {
		((HistogramWindow*)thread->window)->lock_window("HistogramMain::update_gui");
		int reconfigure = load_configuration();
		if( reconfigure )
			((HistogramWindow*)thread->window)->update(1, 1, 1, 1);
		((HistogramWindow*)thread->window)->unlock_window();
	}
}


void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];

	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("PLOT", config.plot);
	output.tag.set_property("SUM_FRAMES", config.sum_frames);
	output.tag.set_property("SPLIT", config.split);
	output.tag.set_property("LOG_SLIDER", config.log_slider);
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.tag.set_property("PARADE", parade);
	output.tag.set_property("MODE", mode);

	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		sprintf(string, "LOW_OUTPUT_%d", i);
		output.tag.set_property(string, config.low_output[i]);
		sprintf(string, "HIGH_OUTPUT_%d", i);
	   	output.tag.set_property(string, config.high_output[i]);
		sprintf(string, "LOW_INPUT_%d", i);
	   	output.tag.set_property(string, config.low_input[i]);
		sprintf(string, "HIGH_INPUT_%d", i);
	   	output.tag.set_property(string, config.high_input[i]);
		sprintf(string, "GAMMA_%d", i);
	   	output.tag.set_property(string, config.gamma[i]);
	}

	output.append_tag();
	output.tag.set_title("/HISTOGRAM");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("HISTOGRAM") ) {
			config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.plot = input.tag.get_property("PLOT", config.plot);
			config.sum_frames = input.tag.get_property("SUM_FRAMES", config.sum_frames);
			config.split = input.tag.get_property("SPLIT", config.split);
			config.log_slider = input.tag.get_property("LOG_SLIDER", config.log_slider);

			if( is_defaults() ) {
				w = input.tag.get_property("W", w);
				h = input.tag.get_property("H", h);
				parade = input.tag.get_property("PARADE", parade);
				mode = input.tag.get_property("MODE", mode);
			}

			char string[BCTEXTLEN];
			for( int i=0; i<HISTOGRAM_MODES; ++i ) {
				sprintf(string, "LOW_OUTPUT_%d", i);
				config.low_output[i] = input.tag.get_property(string, config.low_output[i]);
				sprintf(string, "HIGH_OUTPUT_%d", i);
				config.high_output[i] = input.tag.get_property(string, config.high_output[i]);
				sprintf(string, "GAMMA_%d", i);
				config.gamma[i] = input.tag.get_property(string, config.gamma[i]);

				if( i == HISTOGRAM_VALUE || !config.automatic ) {
					sprintf(string, "LOW_INPUT_%d", i);
					config.low_input[i] = input.tag.get_property(string, config.low_input[i]);
					sprintf(string, "HIGH_INPUT_%d", i);
					config.high_input[i] = input.tag.get_property(string, config.high_input[i]);
				}
			}
		}
	}

	config.boundaries();
}

float HistogramMain::calculate_level(float input, int mode, int use_value)
{
	float output = 0.0;

// Scale to input range
	if( !EQUIV(config.high_input[mode], config.low_input[mode]) ) {
		output = input < config.low_input[mode] ? 0 :
		    (input - config.low_input[mode]) /
			(config.high_input[mode] - config.low_input[mode]);
	}
	else
		output = input;

	if( !EQUIV(config.gamma[mode], 0) ) {
		output = pow(output, 1.0 / config.gamma[mode]);
		CLAMP(output, 0, 1.0);
	}

// Apply value curve
	if( use_value && mode != HISTOGRAM_VALUE )
		output = calculate_level(output, HISTOGRAM_VALUE, 0);

// scale to output range
	if( !EQUIV(config.low_output[mode], config.high_output[mode]) ) {
		output = output * (config.high_output[mode] - config.low_output[mode]) +
			config.low_output[mode];
	}

	CLAMP(output, 0, 1.0);
	return output;
}

void HistogramMain::calculate_histogram(VFrame *data, int do_value)
{
	if( !engine ) {
		int cpus = data->get_w() * data->get_h() / 0x80000 + 2;
		int smps = get_project_smp();
		if( cpus > smps ) cpus = smps;
		engine = new HistogramEngine(this, cpus, cpus);
	}

	engine->process_packages(HistogramEngine::HISTOGRAM, data, do_value);

	int k = 0;
	HistogramUnit *unit = (HistogramUnit*)engine->get_client(0);
	if( !sum_frames ) {
		frames = 0;
		for( int i=0; i<HISTOGRAM_MODES; ++i )
			memcpy(accum[i], unit->accum[i], sizeof(int64_t)*HISTOGRAM_SLOTS);
		k = 1;
	}

	for( int i=k,n=engine->get_total_clients(); i<n; ++i ) {
		unit = (HistogramUnit*)engine->get_client(i);
		for( int j=0; j<HISTOGRAM_MODES; ++j ) {
			int64_t *in = unit->accum[j], *out = accum[j];
			for( int k=HISTOGRAM_SLOTS; --k>=0; ) *out++ += *in++;
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		accum[i][0] = 0;
		accum[i][HISTOGRAM_SLOTS - 1] = 0;
	}
	++frames;
}


void HistogramMain::calculate_automatic(VFrame *data)
{
	calculate_histogram(data, 0);
	config.reset_points(1);

// Do each channel
	for( int i=0; i<3; ++i ) {
		int64_t *accum = this->accum[i];
		int64_t sz = data->get_w() * data->get_h();
		int64_t pixels = sz * frames;
		float white_fraction = 1.0 - (1.0 - config.threshold) / 2;
		int threshold = (int)(white_fraction * pixels);
		float min_level = 0.0, max_level = 1.0;

// Get histogram slot above threshold of pixels
		int64_t total = 0;
		for( int j=0; j<HISTOGRAM_SLOTS; ++j ) {
			total += accum[j];
			if( total >= threshold ) {
				max_level = (float)j / HISTOGRAM_SLOTS * FLOAT_RANGE + HIST_MIN_INPUT;
				break;
			}
		}

// Get histogram slot below threshold of pixels
		total = 0;
		for( int j=HISTOGRAM_SLOTS; --j> 0; ) {
			total += accum[j];
			if( total >= threshold ) {
				min_level = (float)j / HISTOGRAM_SLOTS * FLOAT_RANGE + HIST_MIN_INPUT;
				break;
			}
		}

		config.low_input[i] = min_level;
		config.high_input[i] = max_level;
	}
}


int HistogramMain::calculate_use_opengl()
{
// glHistogram doesn't work.
	int result = get_use_opengl() &&
		!config.automatic &&
		(!config.plot || !gui_open());
	return result;
}


int HistogramMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	need_reconfigure = load_configuration();
	int use_opengl = calculate_use_opengl();
	sum_frames = last_position == start_position ? config.sum_frames : 0;
	last_position = get_direction() == PLAY_FORWARD ?
			start_position+1 : start_position-1;
	this->input = frame;
	this->output = frame;
	int cpus = input->get_w() * input->get_h() / 0x80000 + 2;
	int smps = get_project_smp();
	if( cpus > smps ) cpus = smps;
	if( !engine )
		engine = new HistogramEngine(this, cpus, cpus);
	read_frame(frame, 0, start_position, frame_rate, use_opengl);
	if( config.automatic )
		calculate_automatic(frame);
	if( config.plot ) {
// Generate curves for value histogram
		tabulate_curve(lookup, 0);
		tabulate_curve(preview_lookup, 0, 0x10000);
// Need to get the luminance values.
		calculate_histogram(input, 1);
		send_render_gui(this);
	}

	if( need_reconfigure || config.automatic || config.plot ) {
// Calculate new curves
// Generate transfer tables with value function for integer colormodels.
		tabulate_curve(lookup, 1);
	}

// Apply histogram in hardware
	if( use_opengl )
		return run_opengl();

// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input, 0);
	return 0;
}

void HistogramMain::tabulate_curve(int **table, int idx, int use_value, int len)
{
	int len1 = len - 1;
	int *curve = table[idx];
	for( int i=0; i<len; ++i ) {
		curve[i] = calculate_level((float)i/len1, idx, use_value) * len1;
		CLAMP(curve[i], 0, len1);
	}
}

void HistogramMain::tabulate_curve(int **table, int use_value, int len)
{
// uint8 rgb is 8 bit, all others are converted to 16 bit RGB
	if( len < 0 ) {
		int color_model = input->get_color_model();
		len = color_model == BC_RGB888 || color_model == BC_RGBA8888 ?
			0x100 : 0x10000;
	}
	for( int i=0; i<3; ++i )
		tabulate_curve(table, i, use_value, len);
}

int HistogramMain::handle_opengl()
{
#ifdef HAVE_GL
// Functions to get pixel from either previous effect or texture
	static const char *histogram_get_pixel1 =
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return gl_FragColor;\n"
		"}\n";

	static const char *histogram_get_pixel2 =
		"uniform sampler2D tex;\n"
		"vec4 histogram_get_pixel()\n"
		"{\n"
		"	return texture2D(tex, gl_TexCoord[0].st);\n"
		"}\n";

	static const char *head_frag =
		"uniform vec4 low_input;\n"
		"uniform vec4 high_input;\n"
		"uniform vec4 gamma;\n"
		"uniform vec4 low_output;\n"
		"uniform vec4 output_scale;\n"
		"void main()\n"
		"{\n"
		"	float temp = 0.0;\n";

	static const char *get_rgb_frag =
		"	vec4 pixel = histogram_get_pixel();\n";

	static const char *get_yuv_frag =
		"	vec4 pixel = histogram_get_pixel();\n"
			YUV_TO_RGB_FRAG("pixel");

#define APPLY_INPUT_CURVE(PIXEL, LOW_INPUT, HIGH_INPUT, GAMMA) \
		"// apply input curve\n" \
		"	temp = (" PIXEL " - " LOW_INPUT ") / \n" \
		"		(" HIGH_INPUT " - " LOW_INPUT ");\n" \
		"	temp = max(temp, 0.0);\n" \
		"	" PIXEL " = pow(temp, 1.0 / " GAMMA ");\n"



	static const char *apply_histogram_frag =
		APPLY_INPUT_CURVE("pixel.r", "low_input.r", "high_input.r", "gamma.r")
		APPLY_INPUT_CURVE("pixel.g", "low_input.g", "high_input.g", "gamma.g")
		APPLY_INPUT_CURVE("pixel.b", "low_input.b", "high_input.b", "gamma.b")
		"// apply output curve\n"
		"	pixel.rgb *= output_scale.rgb;\n"
		"	pixel.rgb += low_output.rgb;\n"
		APPLY_INPUT_CURVE("pixel.r", "low_input.a", "high_input.a", "gamma.a")
		APPLY_INPUT_CURVE("pixel.g", "low_input.a", "high_input.a", "gamma.a")
		APPLY_INPUT_CURVE("pixel.b", "low_input.a", "high_input.a", "gamma.a")
		"// apply output curve\n"
		"	pixel.rgb *= vec3(output_scale.a, output_scale.a, output_scale.a);\n"
		"	pixel.rgb += vec3(low_output.a, low_output.a, low_output.a);\n";

	static const char *put_rgb_frag =
		"	gl_FragColor = pixel;\n"
		"}\n";

	static const char *put_yuv_frag =
			RGB_TO_YUV_FRAG("pixel")
		"	gl_FragColor = pixel;\n"
		"}\n";



	get_output()->to_texture();
	get_output()->enable_opengl();

        const char *shader_stack[16];
        memset(shader_stack,0, sizeof(shader_stack));
        int current_shader = 0;

        int need_color_matrix = BC_CModels::is_yuv(get_output()->get_color_model()) ? 1 : 0;
        if( need_color_matrix )
                shader_stack[current_shader++] = bc_gl_colors;

	int aggregate_interpolation = 0;
	int aggregate_gamma = 0;
	int aggregate_colorbalance = 0;
// All aggregation possibilities must be accounted for because unsupported
// effects can get in between the aggregation members.
	if(!strcmp(get_output()->get_prev_effect(2), _("Interpolate Pixels")) &&
		!strcmp(get_output()->get_prev_effect(1), _("Gamma")) &&
		!strcmp(get_output()->get_prev_effect(0), _("Color Balance")))
	{
		aggregate_interpolation = 1;
		aggregate_gamma = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), _("Gamma")) &&
		!strcmp(get_output()->get_prev_effect(0), _("Color Balance")))
	{
		aggregate_gamma = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), _("Interpolate Pixels")) &&
		!strcmp(get_output()->get_prev_effect(0), _("Gamma")))
	{
		aggregate_interpolation = 1;
		aggregate_gamma = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(1), _("Interpolate Pixels")) &&
		!strcmp(get_output()->get_prev_effect(0), _("Color Balance")))
	{
		aggregate_interpolation = 1;
		aggregate_colorbalance = 1;
	}
	else
	if(!strcmp(get_output()->get_prev_effect(0), _("Interpolate Pixels")))
		aggregate_interpolation = 1;
	else
	if(!strcmp(get_output()->get_prev_effect(0), _("Gamma")))
		aggregate_gamma = 1;
	else
	if(!strcmp(get_output()->get_prev_effect(0), _("Color Balance")))
		aggregate_colorbalance = 1;

// The order of processing is fixed by this sequence
	if(aggregate_interpolation)
		INTERPOLATE_COMPILE(shader_stack, current_shader);

	if(aggregate_gamma)
		GAMMA_COMPILE(shader_stack, current_shader,
			aggregate_interpolation);

	if(aggregate_colorbalance)
		COLORBALANCE_COMPILE(shader_stack, current_shader,
			aggregate_interpolation || aggregate_gamma);

	shader_stack[current_shader++] = 
		aggregate_interpolation || aggregate_gamma || aggregate_colorbalance ?
			histogram_get_pixel1 : histogram_get_pixel2;

	shader_stack[current_shader++] = head_frag;
	shader_stack[current_shader++] = BC_CModels::is_yuv(get_output()->get_color_model()) ?
			get_yuv_frag : get_rgb_frag;
	shader_stack[current_shader++] = apply_histogram_frag;
	shader_stack[current_shader++] = BC_CModels::is_yuv(get_output()->get_color_model()) ?
			put_yuv_frag : put_rgb_frag;

	shader_stack[current_shader] = 0;
	unsigned int shader = VFrame::make_shader(shader_stack);

// printf("HistogramMain::handle_opengl %d %d %d %d shader=%d\n",
// aggregate_interpolation,
// aggregate_gamma,
// aggregate_colorbalance,
// current_shader,
// shader);

	float low_input[4];
	float high_input[4];
	float gamma[4];
	float low_output[4];
	float output_scale[4];


// printf("min x    min y    max x    max y\n");
// printf("%f %f %f %f\n", input_min_r[0], input_min_r[1], input_max_r[0], input_max_r[1]);
// printf("%f %f %f %f\n", input_min_g[0], input_min_g[1], input_max_g[0], input_max_g[1]);
// printf("%f %f %f %f\n", input_min_b[0], input_min_b[1], input_max_b[0], input_max_b[1]);
// printf("%f %f %f %f\n", input_min_v[0], input_min_v[1], input_max_v[0], input_max_v[1]);

	for(int i = 0; i < HISTOGRAM_MODES; i++)
	{
		low_input[i] = config.low_input[i];
		high_input[i] = config.high_input[i];
		gamma[i] = config.gamma[i];
		low_output[i] = config.low_output[i];
		output_scale[i] = config.high_output[i] - config.low_output[i];
	}

	if(shader > 0)
	{
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "tex"), 0);
		if(aggregate_gamma) GAMMA_UNIFORMS(shader);
		if(aggregate_interpolation) INTERPOLATE_UNIFORMS(shader);
		if(aggregate_colorbalance) COLORBALANCE_UNIFORMS(shader);
		glUniform4fv(glGetUniformLocation(shader, "low_input"), 1, low_input);
		glUniform4fv(glGetUniformLocation(shader, "high_input"), 1, high_input);
		glUniform4fv(glGetUniformLocation(shader, "gamma"), 1, gamma);
		glUniform4fv(glGetUniformLocation(shader, "low_output"), 1, low_output);
		glUniform4fv(glGetUniformLocation(shader, "output_scale"), 1, output_scale);
		if( need_color_matrix ) BC_GL_COLORS(shader);
	}

	get_output()->init_screen();
	get_output()->bind_texture(0);

	glDisable(GL_BLEND);

// Draw the affected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(),
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);


		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(),
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);

		glTexCoord2f(0.0 / get_output()->get_texture_w(),
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f(0.0, -0.0, 0);


		glEnd();
	}
	else
	{
		get_output()->draw_texture();
	}

	glUseProgram(0);

// Draw the unaffected half
	if(config.split)
	{
		glBegin(GL_TRIANGLES);
		glNormal3f(0, 0, 1.0);


		glTexCoord2f(0.0 / get_output()->get_texture_w(),
			0.0 / get_output()->get_texture_h());
		glVertex3f(0.0, -(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(),
			0.0 / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(),
			-(float)get_output()->get_h(), 0);

		glTexCoord2f((float)get_output()->get_w() / get_output()->get_texture_w(),
			(float)get_output()->get_h() / get_output()->get_texture_h());
		glVertex3f((float)get_output()->get_w(), -0.0, 0);


 		glEnd();
	}

	get_output()->set_opengl_state(VFrame::SCREEN);
#endif
	return 0;
}


HistogramPackage::HistogramPackage()
 : LoadPackage()
{
}

HistogramUnit::HistogramUnit(HistogramEngine *server,
	HistogramMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		accum[i] = new int64_t[HISTOGRAM_SLOTS];
}

HistogramUnit::~HistogramUnit()
{
	for(int i = 0; i < HISTOGRAM_MODES; i++)
		delete [] accum[i];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;
	switch( server->operation ) {
	case HistogramEngine::HISTOGRAM: {
		int do_value = server->do_value;
		const int hmin = HISTOGRAM_MIN * 0xffff / 100;
		const int slots1 = HISTOGRAM_SLOTS-1;

#define HISTOGRAM_HEAD(type) { \
	type **rows = (type**)data->get_rows(); \
	for( int iy=pkg->start; iy<pkg->end; ++iy ) { \
		type *row = rows[iy]; \
		for( int ix=0; ix<w; ++ix ) {

#define HISTOGRAM_TAIL(components) \
			if( do_value ) { \
				r_out = preview_r[bclip(r, 0, 0xffff)]; \
				g_out = preview_g[bclip(g, 0, 0xffff)]; \
				b_out = preview_b[bclip(b, 0, 0xffff)]; \
/*				v = (r * 76 + g * 150 + b * 29) >> 8; */ \
/* Value takes the maximum of the output RGB values */ \
				int v = MAX(r_out, g_out); v = MAX(v, b_out); \
				++accum_v[bclip(v -= hmin, 0, slots1)]; \
			} \
 \
			++accum_r[bclip(r -= hmin, 0, slots1)]; \
			++accum_g[bclip(g -= hmin, 0, slots1)]; \
			++accum_b[bclip(b -= hmin, 0, slots1)]; \
			row += components; \
		} \
	} \
}

		VFrame *data = server->data;
		int w = data->get_w();
		//int h = data->get_h();
		int64_t *accum_r = accum[HISTOGRAM_RED];
		int64_t *accum_g = accum[HISTOGRAM_GREEN];
		int64_t *accum_b = accum[HISTOGRAM_BLUE];
		int64_t *accum_v = accum[HISTOGRAM_VALUE];
		int32_t r, g, b, y, u, v;
		int r_out, g_out, b_out;
		int *preview_r = plugin->preview_lookup[HISTOGRAM_RED];
		int *preview_g = plugin->preview_lookup[HISTOGRAM_GREEN];
		int *preview_b = plugin->preview_lookup[HISTOGRAM_BLUE];

		switch( data->get_color_model() ) {
		case BC_RGB888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGB_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA8888:
			HISTOGRAM_HEAD(unsigned char)
			r = (row[0] << 8) | row[0];
			g = (row[1] << 8) | row[1];
			b = (row[2] << 8) | row[2];
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGBA_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUVA8888:
			HISTOGRAM_HEAD(unsigned char)
			y = (row[0] << 8) | row[0];
			u = (row[1] << 8) | row[1];
			v = (row[2] << 8) | row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGB161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0];
			u = row[1];
			v = row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA16161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0];
			g = row[1];
			b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUVA16161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0];
			u = row[1];
			v = row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		}
		break; }
	case HistogramEngine::APPLY: {

#define PROCESS(type, components) { \
	type **rows = (type**)input->get_rows(); \
	for( int iy=pkg->start; iy<pkg->end; ++iy ) { \
		type *row = rows[iy]; \
		for( int ix=0; ix<w; ++ix ) { \
			if( plugin->config.split && ((ix + (iy*w)/h) < w) ) \
		    		continue; \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) { \
	type **rows = (type**)input->get_rows(); \
	for( int iy=pkg->start; iy<pkg->end; ++iy ) { \
		type *row = rows[iy]; \
		for( int ix=0; ix<w; ++ix ) { \
			if( plugin->config.split && ((ix + (iy*w)/h) < w) ) \
		    		continue; \
			if( max == 0xff ) { /* Convert to 16 bit RGB */ \
				y = (row[0] << 8) | row[0]; \
				u = (row[1] << 8) | row[1]; \
				v = (row[2] << 8) | row[2]; \
			} \
			else { \
				y = row[0]; \
				u = row[1]; \
				v = row[2]; \
			} \
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
/* Look up in RGB domain */ \
			r = lookup_r[r]; \
			g = lookup_g[g]; \
			b = lookup_b[b]; \
/* Convert to 16 bit YUV */ \
			YUV::yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
			if( max == 0xff ) { \
				row[0] = y >> 8; \
				row[1] = u >> 8; \
				row[2] = v >> 8; \
			} \
			else { \
				row[0] = y; \
				row[1] = u; \
				row[2] = v; \
			} \
			row += components; \
		} \
	} \
}

#define PROCESS_FLOAT(components) { \
	float **rows = (float**)input->get_rows(); \
	for( int iy=pkg->start; iy<pkg->end; ++iy ) { \
		float *row = rows[iy]; \
		for( int ix=0; ix<w; ++ix ) { \
			if( plugin->config.split && ((ix + (iy*w)/h) < w) ) \
		    		continue; \
			float fr = row[0]; \
			float fg = row[1]; \
			float fb = row[2]; \
			row[0]  = plugin->calculate_level(fr, HISTOGRAM_RED, 1); \
			row[1]  = plugin->calculate_level(fg, HISTOGRAM_GREEN, 1); \
			row[2]  = plugin->calculate_level(fb, HISTOGRAM_BLUE, 1); \
			row += components; \
		} \
	} \
}

		VFrame *input = plugin->input;
		//VFrame *output = plugin->output;
		int w = input->get_w();
		int h = input->get_h();
		int *lookup_r = plugin->lookup[0];
		int *lookup_g = plugin->lookup[1];
		int *lookup_b = plugin->lookup[2];
		int r, g, b, y, u, v;
		switch( input->get_color_model() ) {
		case BC_RGB888:
			PROCESS(unsigned char, 3)
			break;
		case BC_RGB_FLOAT:
			PROCESS_FLOAT(3);
			break;
		case BC_RGBA8888:
			PROCESS(unsigned char, 4)
			break;
		case BC_RGBA_FLOAT:
			PROCESS_FLOAT(4);
			break;
		case BC_RGB161616:
			PROCESS(uint16_t, 3)
			break;
		case BC_RGBA16161616:
			PROCESS(uint16_t, 4)
			break;
		case BC_YUV888:
			PROCESS_YUV(unsigned char, 3, 0xff)
			break;
		case BC_YUVA8888:
			PROCESS_YUV(unsigned char, 4, 0xff)
			break;
		case BC_YUV161616:
			PROCESS_YUV(uint16_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			PROCESS_YUV(uint16_t, 4, 0xffff)
			break;
		}
		break; }
	}
}


HistogramEngine::HistogramEngine(HistogramMain *plugin,
	int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void HistogramEngine::init_packages()
{
	switch(operation) {
	case HISTOGRAM:
		total_size = data->get_h();
		break;
	case APPLY:
		total_size = data->get_h();
		break;
	}

	int start = 0;
	for( int i=0,n=get_total_packages(); i<n; ++i ) {
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = start;
		package->end = total_size * (i+1)/n;
		start = package->end;
	}

// Initialize clients here in case some don't get run.
	for( int i=0,n=get_total_clients(); i<n; ++i ) {
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for( int j=0; j<HISTOGRAM_MODES; ++j )
			bzero(unit->accum[j], sizeof(int64_t) * HISTOGRAM_SLOTS);
	}
}

LoadClient* HistogramEngine::new_client()
{
	return new HistogramUnit(this, plugin);
}

LoadPackage* HistogramEngine::new_package()
{
	return new HistogramPackage;
}

void HistogramEngine::process_packages(int operation, VFrame *data, int do_value)
{
	this->data = data;
	this->operation = operation;
	this->do_value = do_value;
	LoadServer::process_packages();
}


