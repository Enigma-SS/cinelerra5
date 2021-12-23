/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "clip.h"
#include "filexml.h"
#include "mandelbrot.h"
#include "mandelbrotwindow.h"
#include "language.h"
#include "mutex.h"

#include "cwindow.h"
#include "cwindowgui.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "playback3d.h"

REGISTER_PLUGIN(Mandelbrot)


MandelbrotConfig::MandelbrotConfig()
{
}

int MandelbrotConfig::equivalent(MandelbrotConfig &that)
{
	return is_julia == that.is_julia && scale == that.scale &&
		x_off == that.x_off && y_off == that.y_off &&
		x_julia == that.x_julia && y_julia == that.y_julia &&
		color == that.color && crunch == that.crunch &&
		step == that.step;
}

void MandelbrotConfig::copy_from(MandelbrotConfig &that)
{
	is_julia = that.is_julia;
	x_off = that.x_off;
	y_off = that.y_off;
	scale = that.scale;
	x_julia = that.x_julia;
	y_julia = that.y_julia;
	color = that.color;
	crunch = that.crunch;
	step = that.step;
}

void MandelbrotConfig::interpolate( MandelbrotConfig &prev, MandelbrotConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(prev);
	double u = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	double v = 1. - u;
	this->x_off = u*prev.x_off + v*next.x_off;
	this->y_off = u*prev.y_off + v*next.y_off;
	this->scale = u*prev.scale + v*next.scale;
	this->x_julia = u*prev.x_julia + v*next.x_julia;
	this->y_julia = u*prev.y_julia + v*next.y_julia;
}

void MandelbrotConfig::limits()
{
	bclamp(scale, -1.e4, 1.e4);
	bclamp(crunch, 1, 0x10000);
}


Mandelbrot::Mandelbrot(PluginServer *server)
 : PluginVClient(server)
{
	config.reset();
	config.startJulia();
	vfrm = 0;
	img_w = img_h = 0;
	pbo_id = -1;
	animation_frame = 0;
}

Mandelbrot::~Mandelbrot()
{
	delete vfrm;
}

const char* Mandelbrot::plugin_title() { return N_("Mandelbrot"); }
int Mandelbrot::is_realtime() { return 1; }
int Mandelbrot::is_synthesis() { return 1; }

NEW_WINDOW_MACRO(Mandelbrot, MandelbrotWindow);
LOAD_CONFIGURATION_MACRO(Mandelbrot, MandelbrotConfig)

void Mandelbrot::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("MANDELCUDA");
	output.tag.set_property("IS_JULIA", config.is_julia);
	output.tag.set_property("SCALE", config.scale);
	output.tag.set_property("X_OFF", config.x_off);
	output.tag.set_property("Y_OFF", config.y_off);
	output.tag.set_property("X_JULIA", config.x_julia);
	output.tag.set_property("Y_JULIA", config.y_julia);
	output.tag.set_property("COLOR", config.color);
	output.tag.set_property("CRUNCH", config.crunch);
	output.tag.set_property("STEP", config.step);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/MANDELCUDA");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Mandelbrot::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("MANDELCUDA") ) {
			config.is_julia = input.tag.get_property("IS_JULIA", config.is_julia);
			config.scale = input.tag.get_property("SCALE", config.scale);
			config.x_off = input.tag.get_property("X_OFF", config.x_off);
			config.y_off = input.tag.get_property("Y_OFF", config.y_off);
			config.x_julia = input.tag.get_property("X_JULIA", config.x_julia);
			config.y_julia = input.tag.get_property("Y_JULIA", config.y_julia);
			config.color = input.tag.get_property("COLOR", config.color);
			config.crunch = input.tag.get_property("CRUNCH", config.crunch);
			config.step = input.tag.get_property("STEP", config.step);
		}
	}
	config.limits();
}

void Mandelbrot::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("Mandelbrot::update_gui");
	MandelbrotWindow *window = (MandelbrotWindow*)thread->window;
	window->update_gui();
	window->flush();
	window->unlock_window();
}

void MandelbrotConfig::reset()
{
	is_julia = 0;
	x_julia = 0.0;
	y_julia = 0.0;
	x_off = -0.5;
	y_off = 0.0;
	scale = 3.2;
	color = 0x00030507;
	crunch = 512;
	step = 0;
}

void MandelbrotConfig::startJulia()
{
	is_julia = 1;
	x_julia = -0.172400;
	y_julia = -0.652693;
	x_off = -0.085760;
	y_off = 0.007040;
}

// Get a sub-pixel sample location
void Mandelbrot::GetSample(int sampleIndex, float &x, float &y)
{
	static const unsigned char pairData[128][2] = {
		{64, 64}, {0, 0}, {1, 63}, {63, 1}, {96, 32}, {97, 95}, {36, 96}, {30, 31},
		{95, 127}, {4, 97}, {33, 62}, {62, 33}, {31, 126}, {67, 99}, {99, 65}, {2, 34},
		{81, 49}, {19, 80}, {113, 17}, {112, 112}, {80, 16}, {115, 81}, {46, 15}, {82, 79},
		{48, 78}, {16, 14}, {49, 113}, {114, 48}, {45, 45}, {18, 47}, {20, 109}, {79, 115},
		{65, 82}, {52, 94}, {15, 124}, {94, 111}, {61, 18}, {47, 30}, {83, 100}, {98, 50},
		{110, 2}, {117, 98}, {50, 59}, {77, 35}, {3, 114}, {5, 77}, {17, 66}, {32, 13},
		{127, 20}, {34, 76}, {35, 110}, {100, 12}, {116, 67}, {66, 46}, {14, 28}, {23, 93},
		{102, 83}, {86, 61}, {44, 125}, {76, 3}, {109, 36}, {6, 51}, {75, 89}, {91, 21},
		{60, 117}, {29, 43}, {119, 29}, {74, 70}, {126, 87}, {93, 75}, {71, 24}, {106, 102},
		{108, 58}, {89, 9}, {103, 23}, {72, 56}, {120, 8}, {88, 40}, {11, 88}, {104, 120},
		{57, 105}, {118, 122}, {53, 6}, {125, 44}, {43, 68}, {58, 73}, {24, 22}, {22, 5},
		{40, 86}, {122, 108}, {87, 90}, {56, 42}, {70, 121}, {8, 7}, {37, 52}, {25, 55},
		{69, 11}, {10, 106}, {12, 38}, {26, 69}, {27, 116}, {38, 25}, {59, 54}, {107, 72},
		{121, 57}, {39, 37}, {73, 107}, {85, 123}, {28, 103}, {123, 74}, {55, 85}, {101, 41},
		{42, 104}, {84, 27}, {111, 91}, {9, 19}, {21, 39}, {90, 53}, {41, 60}, {54, 26},
		{92, 119}, {51, 71}, {124, 101}, {68, 92}, {78, 10}, {13, 118}, {7, 84}, {105, 4}
	};

	x = (1.0f / 128.0f) * (0.5f + (float) pairData[sampleIndex][0]);
	y = (1.0f / 128.0f) * (0.5f + (float) pairData[sampleIndex][1]);
} // GetSample

// render Mandelbrot image using CUDA
void Mandelbrot::renderImage()
{
	float xs, ys;
	// Get the anti-alias sub-pixel sample location
	GetSample(pass & 127, xs, ys);

	// Get the pixel scale and offset
	double s = config.scale / (float) img_w;
	double x  = (xs - (double) img_w * 0.5f) * s + config.x_off;
	double y  = (ys - (double) img_h * 0.5f) * s + config.y_off;

	uchar4 colors;
	colors.w = (config.color>>24) & 0xff;
	colors.x = (config.color>>16) & 0xff;
	colors.y = (config.color>>8)  & 0xff;
	colors.z = (config.color>>0)  & 0xff;

	cuda.Run(vfrm->get_data(), img_w*img_h*4, config.is_julia, config.crunch,
		x, y, config.x_julia, config.y_julia, s, colors, pass, animation_frame);
	++pass;
}

void Mandelbrot::displayFunc()
{
	if( config.step ) {
		animation_frame += config.step;
		pass = 0;
	}
	// render the Mandelbrot image
	renderImage();
}

void Mandelbrot::init()
{
	animation_frame = 0;
	pass = 0;
}

int Mandelbrot::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	int need_reconfigure = load_configuration();
	if( need_reconfigure ) pass = 0;
	output = get_output(0);
	color_model = output->get_color_model();
	img_w = output->get_w();
	img_h = output->get_h();
	if( !start_position )
		init();
	if( vfrm &&
	    (vfrm->get_w() != img_w || vfrm->get_h() != img_h) ) {
		delete vfrm;  vfrm = 0;
	}
	if( !vfrm )
		vfrm = new VFrame(img_w, img_h, BC_RGBA8888);

	if( get_use_opengl() )
		return run_opengl();
// always use_opengl
	Canvas *canvas = server->mwindow->cwindow->gui->canvas;
	return server->mwindow->playback_3d->run_plugin(canvas, this);
}

// opengl from here down

void Mandelbrot::init_cuda()
{
	cuda.init_dev();
	GLuint pbo[1];
	glGenBuffers(1, pbo);
	glBindBuffer(GL_ARRAY_BUFFER, pbo_id=*pbo);
	glBufferData(GL_ARRAY_BUFFER, img_w*img_h*4, 0, GL_STATIC_DRAW);
	cuda.init(pbo_id, img_w, img_h);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void Mandelbrot::finish_cuda()
{
	cuda.finish();
	GLuint pbo[1];  *pbo = pbo_id;
	glDeleteBuffers(1, pbo);
	pbo_id = -1;
}

int Mandelbrot::handle_opengl()
{
	vfrm->enable_opengl();
	vfrm->init_screen();
	init_cuda();
	displayFunc();
	output->transfer_from(vfrm);
	finish_cuda();
	return 0;
}

