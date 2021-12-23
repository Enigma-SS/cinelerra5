
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#define GL_GLEXT_PROTOTYPES

#include "bccolors.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "canvas.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "maskautos.h"
#include "maskauto.h"
#include "mutex.h"
#include "mwindow.h"
#include "overlayframe.inc"
#include "overlayframe.h"
#include "playback3d.h"
#include "pluginclient.h"
#include "pluginvclient.h"
#include "edlsession.h"
#include "track.h"
#include "transportque.inc"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#endif

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define QQ(q)#q
#define SS(s)QQ(s)


// Shaders
// These should be passed to VFrame::make_shader to construct shaders.
// Can't hard code sampler2D


#ifdef HAVE_GL
static const char *yuv_to_rgb_frag =
	"uniform sampler2D tex;\n"
	"uniform mat3 yuv_to_rgb_matrix;\n"
	"uniform float yminf;\n"
	"void main()\n"
	"{\n"
	"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
	"	yuva.rgb -= vec3(yminf, 0.5, 0.5);\n"
	"	gl_FragColor = vec4(yuv_to_rgb_matrix * yuva.rgb, yuva.a);\n"
	"}\n";

static const char *yuva_to_yuv_frag =
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
	"	float a = yuva.a;\n"
	"	float anti_a = 1.0 - a;\n"
	"	yuva.r *= a;\n"
	"	yuva.g = yuva.g * a + 0.5 * anti_a;\n"
	"	yuva.b = yuva.b * a + 0.5 * anti_a;\n"
	"	yuva.a = 1.0;\n"
	"	gl_FragColor = yuva;\n"
	"}\n";

static const char *yuva_to_rgb_frag =
	"uniform sampler2D tex;\n"
	"uniform mat3 yuv_to_rgb_matrix;\n"
	"uniform float yminf;\n"
	"void main()\n"
	"{\n"
	"	vec4 yuva = texture2D(tex, gl_TexCoord[0].st);\n"
	"	yuva.rgb -= vec3(yminf, 0.5, 0.5);\n"
	"	yuva.rgb = yuv_to_rgb_matrix * yuva.rgb;\n"
	"	yuva.rgb *= yuva.a;\n"
	"	yuva.a = 1.0;\n"
	"	gl_FragColor = yuva;\n"
	"}\n";

static const char *rgb_to_yuv_frag =
	"uniform sampler2D tex;\n"
	"uniform mat3 rgb_to_yuv_matrix;\n"
	"uniform float yminf;\n"
	"void main()\n"
	"{\n"
	"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
	"	rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
	"	rgba.rgb += vec3(yminf, 0.5, 0.5);\n"
	"	gl_FragColor = rgba;\n"
	"}\n";


static const char *rgba_to_rgb_frag =
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
	"	rgba.rgb *= rgba.a;\n"
	"	rgba.a = 1.0;\n"
	"	gl_FragColor = rgba;\n"
	"}\n";

static const char *rgba_to_yuv_frag =
	"uniform sampler2D tex;\n"
	"uniform mat3 rgb_to_yuv_matrix;\n"
	"uniform float yminf;\n"
	"void main()\n"
	"{\n"
	"	vec4 rgba = texture2D(tex, gl_TexCoord[0].st);\n"
	"	rgba.rgb *= rgba.a;\n"
	"	rgba.a = 1.0;\n"
	"	rgba.rgb = rgb_to_yuv_matrix * rgba.rgb;\n"
	"	rgba.rgb += vec3(yminf, 0.5, 0.5);\n"
	"	gl_FragColor = rgba;\n"
	"}\n";

//static const char *rgba_to_rgb_flatten =
//	"void main() {\n"
//	"	gl_FragColor.rgb *= gl_FragColor.a;\n"
//	"	gl_FragColor.a = 1.0;\n"
//	"}\n";

#define GL_STD_BLEND(FN) \
static const char *blend_##FN##_frag = \
	"uniform sampler2D tex2;\n" \
	"uniform vec2 tex2_dimensions;\n" \
	"uniform float alpha;\n" \
	"void main() {\n" \
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n" \
	"	vec4 result;\n" \
	"	result.rgb = " SS(COLOR_##FN(1.0, gl_FragColor.rgb, gl_FragColor.a, canvas.rgb, canvas.a)) ";\n" \
	"	result.a = " SS(ALPHA_##FN(1.0, gl_FragColor.a, canvas.a)) ";\n" \
	"	gl_FragColor = mix(canvas, result, alpha);\n" \
	"}\n"

#define GL_VEC_BLEND(FN) \
static const char *blend_##FN##_frag = \
	"uniform sampler2D tex2;\n" \
	"uniform vec2 tex2_dimensions;\n" \
	"uniform float alpha;\n" \
	"void main() {\n" \
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n" \
	"	vec4 result;\n" \
	"	result.r = " SS(COLOR_##FN(1.0, gl_FragColor.r, gl_FragColor.a, canvas.r, canvas.a)) ";\n" \
	"	result.g = " SS(COLOR_##FN(1.0, gl_FragColor.g, gl_FragColor.a, canvas.g, canvas.a)) ";\n" \
	"	result.b = " SS(COLOR_##FN(1.0, gl_FragColor.b, gl_FragColor.a, canvas.b, canvas.a)) ";\n" \
	"	result.a = " SS(ALPHA_##FN(1.0, gl_FragColor.a, canvas.a)) ";\n" \
	"	result = clamp(result, 0.0, 1.0);\n" \
	"	gl_FragColor = mix(canvas, result, alpha);\n" \
	"}\n"

#undef mabs
#define mabs abs
#undef mmin
#define mmin min
#undef mmax
#define mmax max

#undef ZERO
#define ZERO 0.0
#undef ONE
#define ONE 1.0
#undef TWO
#define TWO 2.0

// NORMAL
static const char *blend_NORMAL_frag =
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"uniform float alpha;\n"
	"void main() {\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec4 result = mix(canvas, gl_FragColor, gl_FragColor.a);\n"
	"	gl_FragColor = mix(canvas, result, alpha);\n"
	"}\n";

// REPLACE
static const char *blend_REPLACE_frag =
	"uniform float alpha;\n"
	"void main() {\n"
	"}\n";

// ADDITION
static const char *blend_ADDITION_frag =
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"uniform float alpha;\n"
	"void main() {\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec4 result = clamp(gl_FragColor + canvas, 0.0, 1.0);\n"
	"	gl_FragColor = mix(canvas, result, alpha);\n"
	"}\n";

// SUBTRACT
static const char *blend_SUBTRACT_frag =
	"uniform sampler2D tex2;\n"
	"uniform vec2 tex2_dimensions;\n"
	"uniform float alpha;\n"
	"void main() {\n"
	"	vec4 canvas = texture2D(tex2, gl_FragCoord.xy / tex2_dimensions);\n"
	"	vec4 result = clamp(gl_FragColor - canvas, 0.0, 1.0);\n"
	"	gl_FragColor = mix(canvas, result, alpha);\n"
	"}\n";

GL_STD_BLEND(MULTIPLY);
GL_VEC_BLEND(DIVIDE);
GL_VEC_BLEND(MAX);
GL_VEC_BLEND(MIN);
GL_VEC_BLEND(DARKEN);
GL_VEC_BLEND(LIGHTEN);
GL_STD_BLEND(DST);
GL_STD_BLEND(DST_ATOP);
GL_STD_BLEND(DST_IN);
GL_STD_BLEND(DST_OUT);
GL_STD_BLEND(DST_OVER);
GL_STD_BLEND(SRC);
GL_STD_BLEND(SRC_ATOP);
GL_STD_BLEND(SRC_IN);
GL_STD_BLEND(SRC_OUT);
GL_STD_BLEND(SRC_OVER);
GL_STD_BLEND(AND);
GL_STD_BLEND(OR);
GL_STD_BLEND(XOR);
GL_VEC_BLEND(OVERLAY);
GL_STD_BLEND(SCREEN);
GL_VEC_BLEND(BURN);
GL_VEC_BLEND(DODGE);
GL_VEC_BLEND(HARDLIGHT);
GL_VEC_BLEND(SOFTLIGHT);
GL_VEC_BLEND(DIFFERENCE);

static const char *read_texture_frag =
	"uniform sampler2D tex;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"}\n";

static const char *in_vertex_frag =
	"#version 430 // vertex shader\n"
	"in vec3 in_pos;\n"
	"void main() {\n"
	"	gl_Position = vec4(in_pos-vec3(0.5,0.5,0.), .5);\n"
	"}\n";

static const char *feather_frag =
	"#version 430\n"
	"layout(location=0) out vec4 color;\n"
	"uniform sampler2D tex;\n"
	"const int MAX = " SS(MAX_FEATHER) "+1;\n"
	"uniform float psf[MAX];\n"
	"uniform int n;\n"
	"uniform vec2 dxy;\n"
	"uniform vec2 twh;\n"
	"\n"
	"void main() {\n"
	"	vec2 tc = gl_FragCoord.xy/textureSize(tex,0);\n"
	"	color = texture(tex, tc);\n"
	"	float c = color.r, f = c*psf[0];\n"
	"	for( int i=1; i<n; ++i ) {\n"
	"		vec2 dd = float(i)*dxy;\n"
	"		vec2 a = tc+dd, ac = min(max(vec2(0.),a), twh);\n"
	"		vec2 b = tc-dd, bc = min(max(vec2(0.),b), twh);\n"
	"		float fa = texture2D(tex, ac).r * psf[i];\n"
	"		float fb = texture2D(tex, bc).r * psf[i];\n"
	"		float m = max(fa, fb);\n"
	"		if( f < m ) f = m;\n"
	"	}\n"
	"	if( c < f ) color = vec4(f);\n"
	"}\n";

static const char *max_frag =
	"#version 430\n"
	"layout(location=0) out vec4 color;\n"
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"uniform float r;\n"
	"uniform float v;\n"
	"\n"
	"void main() {\n"
	"	vec2 tc = gl_FragCoord.xy/textureSize(tex,0);\n"
	"	color = texture2D(tex1, tc);\n"
	"	float c = texture2D(tex, tc).r;\n"
	"	float b = r<0 ? 1. : 0.;\n"
	"	if( c == b ) return;\n"
	"	float iv = v>=0. ? 1. : -1.;\n"
	"	float rr = r!=0. ? r : 1.;\n"
	"	float rv = rr*v>=0. ? 1. : -1.;\n"
	"	float vv = v>=0. ? 1.-v : 1.+v;\n"
	"	float fg = rv>0. ? vv : 1.;\n"
	"	float bg = rv>0. ? 1. : vv;\n"
	"	float a = c*fg + (1.-c)*bg;\n"
	"	if( iv*(color.a-a) > 0. ) color = vec4(a);\n"
	"}\n";

static const char *multiply_mask4_frag =
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= texture2D(tex1, gl_TexCoord[0].st).r;\n"
	"}\n";

static const char *multiply_mask3_frag =
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	float a = texture2D(tex1, gl_TexCoord[0].st).r;\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"}\n";

static const char *multiply_yuvmask3_frag =
	"uniform sampler2D tex;\n"
	"uniform sampler2D tex1;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	float a = texture2D(tex1, gl_TexCoord[0].st).r;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.rgb *= vec3(a, a, a);\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";

static const char *fade_rgba_frag =
	"uniform sampler2D tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.a *= alpha;\n"
	"}\n";

static const char *fade_yuv_frag =
	"uniform sampler2D tex;\n"
	"uniform float alpha;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D(tex, gl_TexCoord[0].st);\n"
	"	gl_FragColor.r *= alpha;\n"
	"	gl_FragColor.gb -= vec2(0.5, 0.5);\n"
	"	gl_FragColor.g *= alpha;\n"
	"	gl_FragColor.b *= alpha;\n"
	"	gl_FragColor.gb += vec2(0.5, 0.5);\n"
	"}\n";

#endif


Playback3DCommand::Playback3DCommand()
 : BC_SynchronousCommand()
{
	canvas = 0;
	is_nested = 0;
}

void Playback3DCommand::copy_from(BC_SynchronousCommand *command)
{
	Playback3DCommand *ptr = (Playback3DCommand*)command;
	this->canvas = ptr->canvas;
	this->is_cleared = ptr->is_cleared;

	this->in_x1 = ptr->in_x1;
	this->in_y1 = ptr->in_y1;
	this->in_x2 = ptr->in_x2;
	this->in_y2 = ptr->in_y2;
	this->out_x1 = ptr->out_x1;
	this->out_y1 = ptr->out_y1;
	this->out_x2 = ptr->out_x2;
	this->out_y2 = ptr->out_y2;
	this->alpha = ptr->alpha;
	this->mode = ptr->mode;
	this->interpolation_type = ptr->interpolation_type;

	this->input = ptr->input;
	this->start_position_project = ptr->start_position_project;
	this->keyframe_set = ptr->keyframe_set;
	this->keyframe = ptr->keyframe;
	this->default_auto = ptr->default_auto;
	this->plugin_client = ptr->plugin_client;
	this->want_texture = ptr->want_texture;
	this->is_nested = ptr->is_nested;
	this->dst_cmodel = ptr->dst_cmodel;

	BC_SynchronousCommand::copy_from(command);
}

Playback3D::Playback3D(MWindow *mwindow)
 : BC_Synchronous()
{
	this->mwindow = mwindow;
	temp_texture = 0;
}

Playback3D::~Playback3D()
{
}




BC_SynchronousCommand* Playback3D::new_command()
{
	return new Playback3DCommand;
}



void Playback3D::handle_command(BC_SynchronousCommand *command)
{
//printf("Playback3D::handle_command 1 %d\n", command->command);
	switch(command->command)
	{
		case Playback3DCommand::WRITE_BUFFER:
			write_buffer_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_OUTPUT:
			clear_output_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CLEAR_INPUT:
			clear_input_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_CAMERA:
			do_camera_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::OVERLAY:
			overlay_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_FADE:
			do_fade_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::DO_MASK:
			do_mask_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::PLUGIN:
			run_plugin_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::COPY_FROM:
			copy_from_sync((Playback3DCommand*)command);
			break;

		case Playback3DCommand::CONVERT_CMODEL:
			convert_cmodel_sync((Playback3DCommand*)command);
			break;

// 		case Playback3DCommand::DRAW_REFRESH:
// 			draw_refresh_sync((Playback3DCommand*)command);
// 			break;
	}
//printf("Playback3D::handle_command 10\n");
}




void Playback3D::copy_from(Canvas *canvas,
	VFrame *dst,
	VFrame *src,
	int want_texture)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::COPY_FROM;
	command.canvas = canvas;
	command.frame = dst;
	command.input = src;
	command.want_texture = want_texture;
	send_command(&command);
}

void Playback3D::copy_from_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::copy_from_sync");
	if( window ) {
		window->enable_opengl();
		glFinish();
		int copy_to_ram = 0;
		int w = command->input->get_w();
		int h = command->input->get_h();
		if( command->input->get_opengl_state() == VFrame::SCREEN &&
		    w == command->frame->get_w() && h == command->frame->get_h() ) {
// printf("Playback3D::copy_from_sync 1 %d %d %d %d %d\n",
// command->input->get_w(), command->input->get_h(),
// command->frame->get_w(), command->frame->get_h(),
// command->frame->get_color_model());
#ifdef GLx4
// With NVidia at least
			if(w % 4) {
				printf("Playback3D::copy_from_sync: w=%d not supported because it is not divisible by 4.\n", w);
			}
			else
#endif
// Copy to texture
			if( command->want_texture ) {
//printf("Playback3D::copy_from_sync 1 dst=%p src=%p\n", command->frame, command->input);
// Screen_to_texture requires the source pbuffer enabled.
				command->input->enable_opengl();
				command->frame->screen_to_texture();
				command->frame->set_opengl_state(VFrame::TEXTURE);
			}
			else {
				command->input->to_texture();
				copy_to_ram = 1;
			}
		}
		else if( command->input->get_opengl_state() == VFrame::TEXTURE &&
			w == command->frame->get_w() && h == command->frame->get_h() ) {
				copy_to_ram = 1;
		}
		else {
			printf("Playback3D::copy_from_sync: invalid formats opengl_state=%d %dx%d -> %dx%d\n",
				command->input->get_opengl_state(), w, h,
				command->frame->get_w(), command->frame->get_h());
		}

		if( copy_to_ram ) {
			command->input->bind_texture(0);
			command->frame->enable_opengl();
			command->frame->init_screen();
			unsigned int shader = BC_CModels::is_yuv(command->input->get_color_model()) ?
				VFrame::make_shader(0, yuv_to_rgb_frag, 0) : 0;
			if( shader > 0 ) {
				glUseProgram(shader);
				int variable = glGetUniformLocation(shader, "tex");
				glUniform1i(variable, 0);
				BC_GL_YUV_TO_RGB(shader);
			}
			else
				glUseProgram(0);
			command->input->draw_texture(1);
			command->frame->screen_to_ram();
			glUseProgram(0);
		}
	}
	command->canvas->unlock_canvas();
#endif
}




// void Playback3D::draw_refresh(Canvas *canvas,
// 	VFrame *frame,
// 	float in_x1,
// 	float in_y1,
// 	float in_x2,
// 	float in_y2,
// 	float out_x1,
// 	float out_y1,
// 	float out_x2,
// 	float out_y2)
// {
// 	Playback3DCommand command;
// 	command.command = Playback3DCommand::DRAW_REFRESH;
// 	command.canvas = canvas;
// 	command.frame = frame;
// 	command.in_x1 = in_x1;
// 	command.in_y1 = in_y1;
// 	command.in_x2 = in_x2;
// 	command.in_y2 = in_y2;
// 	command.out_x1 = out_x1;
// 	command.out_y1 = out_y1;
// 	command.out_x2 = out_x2;
// 	command.out_y2 = out_y2;
// 	send_command(&command);
// }
//
// void Playback3D::draw_refresh_sync(Playback3DCommand *command)
// {
// #ifdef HAVE_GL
// 	BC_WindowBase *window =
//	 	command->canvas->lock_canvas("Playback3D::draw_refresh_sync");
// 	if( window ) {
// 		window->enable_opengl();
//
// // Read output pbuffer back to RAM in project colormodel
// // RGB 8bit is fastest for OpenGL to read back.
// 		command->frame->reallocate(0,
// 			0,
// 			0,
// 			0,
// 			command->frame->get_w(),
// 			command->frame->get_h(),
// 			BC_RGB888,
// 			-1);
// 		command->frame->screen_to_ram();
//
// 		window->clear_box(0,
// 						0,
// 						window->get_w(),
// 						window->get_h());
// 		window->draw_vframe(command->frame,
// 							(int)command->out_x1,
// 							(int)command->out_y1,
// 							(int)(command->out_x2 - command->out_x1),
// 							(int)(command->out_y2 - command->out_y1),
// 							(int)command->in_x1,
// 							(int)command->in_y1,
// 							(int)(command->in_x2 - command->in_x1),
// 							(int)(command->in_y2 - command->in_y1),
// 							0);
//
// 	}
// 	command->canvas->unlock_canvas();
// #endif
// }





void Playback3D::write_buffer(Canvas *canvas,
	VFrame *frame,
	float in_x1,
	float in_y1,
	float in_x2,
	float in_y2,
	float out_x1,
	float out_y1,
	float out_x2,
	float out_y2,
	int is_cleared)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::WRITE_BUFFER;
	command.canvas = canvas;
	command.frame = frame;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.is_cleared = is_cleared;
	send_command(&command);
}


void Playback3D::write_buffer_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::write_buffer_sync");
	if( window ) {
		window->enable_opengl();
// Update hidden cursor
		window->update_video_cursor();
		command->frame->enable_opengl();
		command->frame->init_screen();
//printf("Playback3D::write_buffer_sync 1 %d\n", window->get_id());
		int frame_state = command->frame->get_opengl_state();
		if( frame_state != VFrame::TEXTURE )
			command->frame->to_texture();
		if( frame_state != VFrame::RAM ) {
			command->in_y1 = command->frame->get_h() - command->in_y1;
			command->in_y2 = command->frame->get_h() - command->in_y2;
		}
		window->enable_opengl();
		draw_output(command, 1);
		command->frame->set_opengl_state(frame_state);
	}
	command->canvas->unlock_canvas();
#endif
}



void Playback3D::draw_output(Playback3DCommand *command, int flip_y)
{
#ifdef HAVE_GL
	int texture_id = command->frame->get_texture_id();
	BC_WindowBase *window = command->canvas->get_canvas();

// printf("Playback3D::draw_output 1 texture_id=%d window=%p\n",
// texture_id,
// command->canvas->get_canvas());




// If virtual console is being used, everything in this function has
// already been done except the page flip.
	if(texture_id >= 0)
	{
		canvas_w = window->get_w();
		canvas_h = window->get_h();
		VFrame::init_screen(canvas_w, canvas_h);
		int color_model = command->frame->get_color_model();
		int is_yuv = BC_CModels::is_yuv(color_model);

		if(!command->is_cleared)
		{
// If we get here, the virtual console was not used.
			int color = command->canvas->get_clear_color();
			int r = (color>>16) & 0xff; // always rgb
			int g = (color>>8) & 0xff;
			int b = (color>>0) & 0xff;
			color_frame(command, r/255.f, g/255.f, b/255.f, 0.f);
		}

// Texture
// Undo any previous shader settings
		command->frame->bind_texture(0);

// Convert colormodel
		unsigned int shader = is_yuv ? VFrame::make_shader(0, yuv_to_rgb_frag, 0) : 0;
		if( shader > 0 ) {
			glUseProgram(shader);
// Set texture unit of the texture
			int variable = glGetUniformLocation(shader, "tex");
			glUniform1i(variable, 0);
			BC_GL_YUV_TO_RGB(shader);
		}

//		if(BC_CModels::components(color_model) == 4)
//		{
//			glEnable(GL_BLEND);
//			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		}

		command->frame->draw_texture(
			command->in_x1, command->in_y1, command->in_x2, command->in_y2,
			command->out_x1, command->out_y1, command->out_x2, command->out_y2,
			flip_y);


//printf("Playback3D::draw_output 2 %f,%f %f,%f -> %f,%f %f,%f\n",
// command->in_x1, command->in_y1, command->in_x2, command->in_y2,
// command->out_x1, command->out_y1, command->out_x2, command->out_y2);

		glUseProgram(0);

		command->canvas->get_canvas()->flip_opengl();

	}
#endif
}


void Playback3D::color_frame(Playback3DCommand *command,
		float r, float g, float b, float a)
{
#ifdef HAVE_GL
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}


void Playback3D::clear_output(Canvas *canvas, VFrame *output)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_OUTPUT;
	command.canvas = canvas;
	command.frame = output;
	send_command(&command);
}

void Playback3D::clear_output_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::clear_output_sync");
	if( window ) {
// If we get here, the virtual console is being used.
		command->canvas->get_canvas()->enable_opengl();
		int is_yuv = 0;
		int color = BLACK, alpha = 0;
// Using pbuffer for refresh frame.
		if( command->frame ) {
			command->frame->enable_opengl();
			command->frame->set_opengl_state(VFrame::SCREEN);
			color = command->frame->get_clear_color();
			alpha = command->frame->get_clear_alpha();
			int color_model = command->canvas->mwindow->edl->session->color_model;
			is_yuv = BC_CModels::is_yuv(color_model);
		}
		else
			color = command->canvas->get_clear_color();
		int a = alpha;
		int r = (color>>16) & 0xff;
		int g = (color>>8) & 0xff;
		int b = (color>>0) & 0xff;
		if( is_yuv ) YUV::yuv.rgb_to_yuv_8(r, g, b);
		color_frame(command, r/255.f, g/255.f, b/255.f, a/255.f);
	}
	command->canvas->unlock_canvas();
#endif
}


void Playback3D::clear_input(Canvas *canvas, VFrame *frame)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::CLEAR_INPUT;
	command.canvas = canvas;
	command.frame = frame;
	send_command(&command);
}

void Playback3D::clear_input_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::clear_input_sync");
	if( window ) {
		command->canvas->get_canvas()->enable_opengl();
		command->frame->enable_opengl();
		command->frame->clear_pbuffer();
		command->frame->set_opengl_state(VFrame::SCREEN);
	}
	command->canvas->unlock_canvas();
#endif
}

void Playback3D::do_camera(Canvas *canvas,
	VFrame *output,
	VFrame *input,
	float in_x1,
	float in_y1,
	float in_x2,
	float in_y2,
	float out_x1,
	float out_y1,
	float out_x2,
	float out_y2)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_CAMERA;
	command.canvas = canvas;
	command.input = input;
	command.frame = output;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	send_command(&command);
}

void Playback3D::do_camera_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::do_camera_sync");
	if( window ) {
		command->canvas->get_canvas()->enable_opengl();

		command->input->to_texture();
		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->clear_pbuffer();

		command->input->bind_texture(0);
// Must call draw_texture in input frame to get the texture coordinates right.

// printf("Playback3D::do_camera_sync 1 %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
// command->in_x1,
// command->in_y2,
// command->in_x2,
// command->in_y1,
// command->out_x1,
// (float)command->input->get_h() - command->out_y1,
// command->out_x2,
// (float)command->input->get_h() - command->out_y2);
		command->input->draw_texture(
			command->in_x1, command->in_y2,
			command->in_x2, command->in_y1,
			command->out_x1,
			(float)command->frame->get_h() - command->out_y1,
			command->out_x2,
			(float)command->frame->get_h() - command->out_y2);


		command->frame->set_opengl_state(VFrame::SCREEN);
		command->frame->screen_to_ram();
	}
	command->canvas->unlock_canvas();
#endif
}

void Playback3D::overlay(Canvas *canvas, VFrame *input,
	float in_x1, float in_y1, float in_x2, float in_y2,
	float out_x1, float out_y1, float out_x2, float out_y2,
	float alpha, int mode, int interpolation_type,
	VFrame *output, int is_nested)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::OVERLAY;
	command.canvas = canvas;
	command.frame = output;
	command.input = input;
	command.in_x1 = in_x1;
	command.in_y1 = in_y1;
	command.in_x2 = in_x2;
	command.in_y2 = in_y2;
	command.out_x1 = out_x1;
	command.out_y1 = out_y1;
	command.out_x2 = out_x2;
	command.out_y2 = out_y2;
	command.alpha = alpha;
	command.mode = mode;
	command.interpolation_type = interpolation_type;
	command.is_nested = is_nested;
	send_command(&command);
}

void Playback3D::overlay_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
// To do these operations, we need to copy the input buffer to a texture
// and blend 2 textures in a shader
	static const char * const overlay_shaders[TRANSFER_TYPES] = {
		blend_NORMAL_frag,	// TRANSFER_NORMAL
		blend_ADDITION_frag,	// TRANSFER_ADDITION
		blend_SUBTRACT_frag,	// TRANSFER_SUBTRACT
		blend_MULTIPLY_frag,	// TRANSFER_MULTIPLY
		blend_DIVIDE_frag,	// TRANSFER_DIVIDE
		blend_REPLACE_frag,	// TRANSFER_REPLACE
		blend_MAX_frag,		// TRANSFER_MAX
		blend_MIN_frag,		// TRANSFER_MIN
		blend_DARKEN_frag,	// TRANSFER_DARKEN
		blend_LIGHTEN_frag,	// TRANSFER_LIGHTEN
		blend_DST_frag,		// TRANSFER_DST
		blend_DST_ATOP_frag,	// TRANSFER_DST_ATOP
		blend_DST_IN_frag,	// TRANSFER_DST_IN
		blend_DST_OUT_frag,	// TRANSFER_DST_OUT
		blend_DST_OVER_frag,	// TRANSFER_DST_OVER
		blend_SRC_frag,		// TRANSFER_SRC
		blend_SRC_ATOP_frag,	// TRANSFER_SRC_ATOP
		blend_SRC_IN_frag,	// TRANSFER_SRC_IN
		blend_SRC_OUT_frag,	// TRANSFER_SRC_OUT
		blend_SRC_OVER_frag,	// TRANSFER_SRC_OVER
		blend_AND_frag,		// TRANSFER_AND
		blend_OR_frag,		// TRANSFER_OR
		blend_XOR_frag,		// TRANSFER_XOR
		blend_OVERLAY_frag,	// TRANSFER_OVERLAY
		blend_SCREEN_frag,	// TRANSFER_SCREEN
		blend_BURN_frag,	// TRANSFER_BURN
		blend_DODGE_frag,	// TRANSFER_DODGE
		blend_HARDLIGHT_frag,	// TRANSFER_HARDLIGHT
		blend_SOFTLIGHT_frag,	// TRANSFER_SOFTLIGHT
		blend_DIFFERENCE_frag,	// TRANSFER_DIFFERENCE
	};

	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::overlay_sync");
	if( window ) {
// Make sure OpenGL is enabled first.
		window->enable_opengl();
	 	window->update_video_cursor();

		glColor4f(1, 1, 1, 1);
		glDisable(GL_BLEND);


//printf("Playback3D::overlay_sync 1 %d\n", command->input->get_opengl_state());
		switch( command->input->get_opengl_state() ) {
// Upload texture and composite to screen
		case VFrame::RAM:
			command->input->to_texture();
			break;
// Just composite texture to screen
		case VFrame::TEXTURE:
			break;
// read from PBuffer to texture, then composite texture to screen
		case VFrame::SCREEN:
			command->input->enable_opengl();
			command->input->screen_to_texture();
			break;
		default:
			printf("Playback3D::overlay_sync unknown state\n");
			break;
		}

		if(command->frame) {
// Render to PBuffer
			command->frame->enable_opengl();
			command->frame->set_opengl_state(VFrame::SCREEN);
			canvas_w = command->frame->get_w();
			canvas_h = command->frame->get_h();
		}
		else {
// Render to canvas
			window->enable_opengl();
			canvas_w = window->get_w();
			canvas_h = window->get_h();
		}


		const char *shader_stack[16];
		memset(shader_stack,0, sizeof(shader_stack));
		int total_shaders = 0, need_matrix = 0;

		VFrame::init_screen(canvas_w, canvas_h);

// Enable texture
		command->input->bind_texture(0);

// Convert colormodel to RGB if not nested.
// The color model setting in the output frame is ignored.
//		if( command->is_nested <= 0 &&  // not nested
//		    BC_CModels::is_yuv(command->input->get_color_model()) ) {
//			need_matrix = 1;
//			shader_stack[total_shaders++] = yuv_to_rgb_frag;
//		}

// get the shaders
#define add_shader(s) \
  if(!total_shaders) shader_stack[total_shaders++] = read_texture_frag; \
  shader_stack[total_shaders++] = s

		switch(command->mode) {
		case TRANSFER_REPLACE:
// This requires overlaying an alpha multiplied image on a black screen.
			if( command->input->get_texture_components() != 4 ) break;
			add_shader(overlay_shaders[command->mode]);
			break;
		default:
			enable_overlay_texture(command);
			add_shader(overlay_shaders[command->mode]);
			break;
		}

// if to flatten alpha
//		if( command->is_nested < 0 ) {
//			switch(command->input->get_color_model()) {
//// yuv has already been converted to rgb
//			case BC_YUVA8888:
//			case BC_RGBA_FLOAT:
//			case BC_RGBA8888:
//				add_shader(rgba_to_rgb_flatten);
//				break;
//			}
//		}

// run the shaders
		add_shader(0);
		unsigned int shader = !shader_stack[0] ? 0 :
			VFrame::make_shader(shader_stack);
		if( shader > 0 ) {
			glUseProgram(shader);
			if( need_matrix ) BC_GL_YUV_TO_RGB(shader);
// Set texture unit of the texture
			glUniform1i(glGetUniformLocation(shader, "tex"), 0);
// Set texture unit of the temp texture
			glUniform1i(glGetUniformLocation(shader, "tex2"), 1);
// Set alpha
			int variable = glGetUniformLocation(shader, "alpha");
			glUniform1f(variable, command->alpha);
// Set dimensions of the temp texture
			if(temp_texture)
				glUniform2f(glGetUniformLocation(shader, "tex2_dimensions"),
					(float)temp_texture->get_texture_w(),
					(float)temp_texture->get_texture_h());
		}
		else
			glUseProgram(0);


//printf("Playback3D::overlay_sync %f %f %f %f %f %f %f %f\n",
// command->in_x1, command->in_y1, command->in_x2, command->in_y2,
// command->out_x1, command->out_y1, command->out_x2, command->out_y2);

		command->input->draw_texture(
			command->in_x1, command->in_y1, command->in_x2, command->in_y2,
			command->out_x1, command->out_y1, command->out_x2, command->out_y2,
			!command->is_nested);
		glUseProgram(0);

// Delete temp texture
		if(temp_texture) {
			delete temp_texture;
			temp_texture = 0;
			glActiveTexture(GL_TEXTURE1);
			glDisable(GL_TEXTURE_2D);
		}
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
	}
	command->canvas->unlock_canvas();
#endif
}

void Playback3D::enable_overlay_texture(Playback3DCommand *command)
{
#ifdef HAVE_GL
	glDisable(GL_BLEND);

	glActiveTexture(GL_TEXTURE1);
	BC_Texture::new_texture(&temp_texture, canvas_w, canvas_h,
		command->input->get_color_model());
	temp_texture->bind(1);

// Read canvas into texture
	glReadBuffer(GL_BACK);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, canvas_w, canvas_h);
#endif
}


void Playback3D::do_mask(Canvas *canvas,
	VFrame *output,
	int64_t start_position_project,
	MaskAutos *keyframe_set,
	MaskAuto *keyframe,
	MaskAuto *default_auto)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_MASK;
	command.canvas = canvas;
	command.frame = output;
	command.start_position_project = start_position_project;
	command.keyframe_set = keyframe_set;
	command.keyframe = keyframe;
	command.default_auto = default_auto;

	send_command(&command);
}


void Playback3D::draw_spots(MaskSpots &spots, int ix1,int iy1, int ix2,int iy2)
{
	int x1 = iy1 < iy2 ? ix1 : ix2;
	int y1 = iy1 < iy2 ? iy1 : iy2;
	int x2 = iy1 < iy2 ? ix2 : ix1;
	int y2 = iy1 < iy2 ? iy2 : iy1;

	int x = x1, y = y1;
	int dx = x2-x1, dy = y2-y1;
	int dx2 = 2*dx, dy2 = 2*dy;
	if( dx < 0 ) dx = -dx;
	int m = dx > dy ? dx : dy, n = m;
	if( dy >= dx ) {
		if( dx2 >= 0 ) do {     /* +Y, +X */
			spots.append(x, y++);
			if( (m -= dx2) < 0 ) { m += dy2;  ++x; }
		} while( --n >= 0 );
		else do {              /* +Y, -X */
			spots.append(x, y++);
			if( (m += dx2) < 0 ) { m += dy2;  --x; }
		} while( --n >= 0 );
	}
	else {
		if( dx2 >= 0 ) do {     /* +X, +Y */
			spots.append(x++, y);
			if( (m -= dy2) < 0 ) { m += dx2;  ++y; }
		} while( --n >= 0 );
		else do {              /* -X, +Y */
			spots.append(x--, y);
			if( (m -= dy2) < 0 ) { m -= dx2;  ++y; }
		} while( --n >= 0 );
	}
}

#ifdef HAVE_GL
class fb_texture : public BC_Texture
{
public:
	fb_texture(int w, int h, int colormodel);
	~fb_texture();
	void bind(int texture_unit);
	void read_screen(int x, int y, int w, int h);
	void set_output_texture();
	void unset_output_texture();
	GLuint fb, rb;
};

fb_texture::fb_texture(int w, int h, int colormodel)
 : BC_Texture(w, h, colormodel)
{
	fb = 0;  rb = 0;
//	glGenRenderbuffers(1, &rb);
//	glBindRenderbuffer(GL_RENDERBUFFER, rb);
//	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, get_texture_w(), get_texture_w());
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);
}

fb_texture::~fb_texture()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, (GLuint *)&fb);
//	glBindRenderbuffer(GL_RENDERBUFFER, 0);
//	glGenRenderbuffers(1, &rb);
}

void fb_texture::bind(int texture_unit)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
//	glBindRenderbuffer(GL_RENDERBUFFER, rb);
	BC_Texture::bind(texture_unit);
}

void fb_texture::read_screen(int x, int y, int w, int h)
{
	bind(1);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glReadBuffer(GL_BACK);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, x,y, w,h);
}

void fb_texture::set_output_texture()
{
	GLenum at = GL_COLOR_ATTACHMENT0;
	glFramebufferTexture(GL_FRAMEBUFFER, at, get_texture_id(), 0);
	GLenum dbo[1] = { at, }; // bind layout(location=0) out vec4 color;
	glDrawBuffers(1, dbo);
	int ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( ret != GL_FRAMEBUFFER_COMPLETE ) {
		printf("glDrawBuffer error 0x%04x\n", ret);
		return;
	}
}
void fb_texture::unset_output_texture()
{
	glDrawBuffers(0, 0);
	int at = GL_COLOR_ATTACHMENT0;
	glFramebufferTexture(GL_FRAMEBUFFER, at, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_TEXTURE_2D);
}


class zglTessData : public ArrayList<double *>
{
public:
	zglTessData() { set_array_delete(); }
	~zglTessData() { remove_all_objects(); }
};

static void combineData(GLdouble coords[3],
		GLdouble *vertex_data[4], GLfloat weight[4],
		GLdouble **outData, void *data)
{
	zglTessData *invented = (zglTessData *)data;
	GLdouble *vertex = new double[6];
	invented->append(vertex);
	vertex[0] = coords[0];
	vertex[1] = coords[1];
	vertex[2] = coords[2];
	for( int i=3; i<6; ++i ) {
		GLdouble v = 0;
		for( int k=0; k<4; ++k ) {
			if( !weight[k] || !vertex_data[k] ) continue;
			v += weight[k] * vertex_data[k][i];
		}
		vertex[i] = v;
	}
	*outData = vertex;
}

// dbug
static void zglBegin(GLenum mode) { glBegin(mode); }
static void zglEnd() { glEnd(); }
static void zglVertex(const GLdouble *v) { glVertex3dv(v); }

#endif

void Playback3D::do_mask_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::do_mask_sync");
	if( window ) {
		window->enable_opengl();

		switch( command->frame->get_opengl_state() ) {
		case VFrame::RAM:
// upload frame to the texture
			command->frame->to_texture();
			break;

		case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
			command->frame->enable_opengl();
			command->frame->screen_to_texture();
			break;
		}

// Initialize coordinate system
		command->frame->enable_opengl();
		command->frame->init_screen();
		int color_model = command->frame->get_color_model();
		int w = command->frame->get_w();
		int h = command->frame->get_h();
		MaskEdges edges;
		float faders[SUBMASKS], feathers[SUBMASKS], cc = 1;
		MaskPoints point_set[SUBMASKS];
// Draw every submask as a new polygon
		int total_submasks = command->keyframe_set->total_submasks(
			command->start_position_project, PLAY_FORWARD);
		int show_mask = command->keyframe_set->track->masks;

		for(int k = 0; k < total_submasks; k++) {
			MaskPoints &points = point_set[k];
			command->keyframe_set->get_points(&points,
				k, command->start_position_project, PLAY_FORWARD);
			float fader = command->keyframe_set->get_fader(
				command->start_position_project, k, PLAY_FORWARD);
			float v = fader/100.;
			faders[k] = v;
			float feather = command->keyframe_set->get_feather(
				command->start_position_project, k, PLAY_FORWARD);
			feathers[k] = feather;
			MaskEdge &edge = *edges.append(new MaskEdge());
			if( !v || !((show_mask>>k) & 1) || !points.size() ) continue;
			edge.load(point_set[k], h);
			if( v >= 0 ) continue;
			float vv = 1 + v;
			if( cc > vv ) cc = vv;
		}

		fb_texture *mask = new fb_texture(w, h, color_model);
		mask->set_output_texture();
		glClearColor(cc, cc, cc, cc);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		mask->unset_output_texture();

		unsigned int feather_shader =
			VFrame::make_shader(0, in_vertex_frag, feather_frag, 0);
		unsigned int max_shader =
			VFrame::make_shader(0, in_vertex_frag, max_frag, 0);
		if( feather_shader && max_shader ) {
			fb_texture *in = new fb_texture(w, h, color_model);
			fb_texture *out = new fb_texture(w, h, color_model);
			float tw = 1./out->get_texture_w(), th = 1./out->get_texture_h();
			float tw1 = (w-1)*tw, th1 = (h-1)*th;
			for(int k = 0; k < total_submasks; k++) {
				MaskEdge &edge = *edges[k];
				if( edge.size() < 3 ) continue;
				float r = feathers[k], v = faders[k];
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glActiveTexture(GL_TEXTURE0);
				glDisable(GL_TEXTURE_2D);
				float b = r>=0 ? 0. : 1.;
				float f = r>=0 ? 1. : 0.;
				glClearColor(b, b, b, b);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glColor4f(f, f, f, f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				int display_list = glGenLists(1);
#if 0
				glNewList(display_list, GL_COMPILE);
				glBegin(GL_POLYGON);
				MaskCoord *c = &edge[0];
				for( int i=edge.size(); --i>=0; ++c )
					glVertex2f(c->x, c->y);
				glEnd();
				glEndList();
				glCallList(display_list);
#else
				{ zglTessData invented;
				GLUtesselator *tess = gluNewTess();
				gluTessProperty(tess, GLU_TESS_TOLERANCE, 0.5);
				gluTessCallback(tess, GLU_TESS_VERTEX,(GLvoid (*)()) &zglVertex);
				gluTessCallback(tess, GLU_TESS_BEGIN,(GLvoid (*)()) &zglBegin);
				gluTessCallback(tess, GLU_TESS_END,(GLvoid (*)()) &zglEnd);
				gluTessCallback(tess, GLU_TESS_COMBINE_DATA,(GLvoid (*)()) &combineData);
				glNewList(display_list, GL_COMPILE);
				gluTessBeginPolygon(tess, &invented);
				gluTessBeginContour(tess);
				MaskCoord *c = &edge[0];
				for( int i=edge.size(); --i>=0; ++c )
					gluTessVertex(tess, (GLdouble *)c, c);
				gluTessEndContour(tess);
				gluTessEndPolygon(tess);
				glEndList();
				glCallList(display_list);
				gluDeleteTess(tess); }
#endif
				glDeleteLists(1, display_list);
				in->read_screen(0,0, w,h);
//in->write_tex("/tmp/in0.ppm");
				if( r ) {
					double sig2 = -log(255.0)/(r*r);
					int n = abs((int)r) + 1;
					if( n > MAX_FEATHER+1 ) n = MAX_FEATHER+1;
					float psf[n];  // point spot fn
					for( int i=0; i<n; ++i )
						psf[i] = exp(i*i * sig2);
					glUseProgram(feather_shader);
					glUniform1fv(glGetUniformLocation(feather_shader, "psf"), n, psf);
					glUniform1i(glGetUniformLocation(feather_shader, "n"), n);
					glUniform2f(glGetUniformLocation(feather_shader, "dxy"), tw, 0.);
					glUniform2f(glGetUniformLocation(feather_shader, "twh"), tw1, th1);
					glUniform1i(glGetUniformLocation(feather_shader, "tex"), 0);
					in->bind(0);
					out->set_output_texture();
					out->draw_texture(0,0, w,h, 0,0, w,h);
					out->unset_output_texture();
//out->write_tex("/tmp/out1.ppm");
					fb_texture *t = in;  in = out;  out = t;
					glUniform2f(glGetUniformLocation(feather_shader, "dxy"), 0., th);
					in->bind(0);
					out->set_output_texture();
					out->draw_texture(0,0, w,h, 0,0, w,h);
					out->unset_output_texture();
//out->write_tex("/tmp/out2.ppm");
					glUseProgram(0);
					t = in;  in = out;  out = t;
				}

				glUseProgram(max_shader);
				in->bind(0);
//in->write_tex("/tmp/in1.ppm");
//mask->write_tex("/tmp/mask1.ppm");
				mask->bind(1);
				glUniform1i(glGetUniformLocation(max_shader, "tex"), 0);
				glUniform1i(glGetUniformLocation(max_shader, "tex1"), 1);
				glUniform1f(glGetUniformLocation(max_shader, "r"), r);
				glUniform1f(glGetUniformLocation(max_shader, "v"), v);
				glViewport(0,0, w,h);
				out->set_output_texture();
				out->draw_texture(0,0, w,h, 0,0, w,h);
				out->unset_output_texture();
				glUseProgram(0);
				fb_texture *t = mask;  mask = out;  out = t;
//mask->write_tex("/tmp/mask2.ppm");
			}
			delete in;
			delete out;
		}

		const char *alpha_shader = BC_CModels::has_alpha(color_model) ?
				multiply_mask4_frag :
			!BC_CModels::is_yuv(color_model) ?
				multiply_mask3_frag :
				multiply_yuvmask3_frag;
		unsigned int shader = VFrame::make_shader(0, alpha_shader, 0);
		glUseProgram(shader);
		if( shader > 0 ) {
			command->frame->bind_texture(0);
			mask->BC_Texture::bind(1);
			glUniform1i(glGetUniformLocation(shader, "tex"), 0);
			glUniform1i(glGetUniformLocation(shader, "tex1"), 1);
		}
		command->frame->draw_texture();
		command->frame->set_opengl_state(VFrame::SCREEN);
		glUseProgram(0);
		delete mask;
		glColor4f(1, 1, 1, 1);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		window->enable_opengl();
	}
	command->canvas->unlock_canvas();
#endif
}


void Playback3D::convert_cmodel(Canvas *canvas,
	VFrame *output,
	int dst_cmodel)
{
// Do nothing if colormodels are equivalent in OpenGL & the image is in hardware.
	int src_cmodel = output->get_color_model();
	if(
		(output->get_opengl_state() == VFrame::TEXTURE ||
		output->get_opengl_state() == VFrame::SCREEN) &&
// OpenGL has no floating point.
		( (src_cmodel == BC_RGB888 && dst_cmodel == BC_RGB_FLOAT) ||
		  (src_cmodel == BC_RGBA8888 && dst_cmodel == BC_RGBA_FLOAT) ||
		  (src_cmodel == BC_RGB_FLOAT && dst_cmodel == BC_RGB888) ||
		  (src_cmodel == BC_RGBA_FLOAT && dst_cmodel == BC_RGBA8888) ||
// OpenGL sets alpha to 1 on import
		  (src_cmodel == BC_RGB888 && dst_cmodel == BC_RGBA8888) ||
		  (src_cmodel == BC_YUV888 && dst_cmodel == BC_YUVA8888) ||
		  (src_cmodel == BC_RGB_FLOAT && dst_cmodel == BC_RGBA_FLOAT) )
		) return;



	Playback3DCommand command;
	command.command = Playback3DCommand::CONVERT_CMODEL;
	command.canvas = canvas;
	command.frame = output;
	command.dst_cmodel = dst_cmodel;
	send_command(&command);
}

void Playback3D::convert_cmodel_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::convert_cmodel_sync");
	if( window ) {
		window->enable_opengl();

// Import into hardware
		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->to_texture();

// Colormodel permutation
		int src_cmodel = command->frame->get_color_model();
		int dst_cmodel = command->dst_cmodel;
		typedef struct {
			int src, dst, typ;
			const char *shader;
		} cmodel_shader_table_t;
		enum { rgb_to_rgb, rgb_to_yuv, yuv_to_rgb, yuv_to_yuv, };
		int type = -1;
		static cmodel_shader_table_t cmodel_shader_table[]  = {
			{ BC_RGB888,	BC_YUV888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_RGB888,	BC_YUVA8888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_RGBA8888,	BC_RGB888,	rgb_to_rgb, rgba_to_rgb_frag },
			{ BC_RGBA8888,	BC_RGB_FLOAT,	rgb_to_rgb, rgba_to_rgb_frag },
			{ BC_RGBA8888,	BC_YUV888,	rgb_to_yuv, rgba_to_yuv_frag },
			{ BC_RGBA8888,	BC_YUVA8888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_RGB_FLOAT,	BC_YUV888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_RGB_FLOAT,	BC_YUVA8888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_RGBA_FLOAT,BC_RGB888,	rgb_to_rgb, rgba_to_rgb_frag },
			{ BC_RGBA_FLOAT,BC_RGB_FLOAT,	rgb_to_rgb, rgba_to_rgb_frag },
			{ BC_RGBA_FLOAT,BC_YUV888,	rgb_to_yuv, rgba_to_yuv_frag },
			{ BC_RGBA_FLOAT,BC_YUVA8888,	rgb_to_yuv, rgb_to_yuv_frag  },
			{ BC_YUV888,	BC_RGB888,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUV888,	BC_RGBA8888,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUV888,	BC_RGB_FLOAT,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUV888,	BC_RGBA_FLOAT,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUVA8888,	BC_RGB888,	yuv_to_rgb, yuva_to_rgb_frag },
			{ BC_YUVA8888,	BC_RGBA8888,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUVA8888,	BC_RGB_FLOAT,	yuv_to_rgb, yuva_to_rgb_frag },
			{ BC_YUVA8888,	BC_RGBA_FLOAT,	yuv_to_rgb, yuv_to_rgb_frag  },
			{ BC_YUVA8888,	BC_YUV888,	yuv_to_yuv, yuva_to_yuv_frag },
		};

		const char *shader = 0;
		int table_size = sizeof(cmodel_shader_table) / sizeof(cmodel_shader_table_t);
		for( int i=0; i<table_size; ++i ) {
			if( cmodel_shader_table[i].src == src_cmodel &&
			    cmodel_shader_table[i].dst == dst_cmodel ) {
				shader = cmodel_shader_table[i].shader;
				type = cmodel_shader_table[i].typ;
				break;
			}
		}

// printf("Playback3D::convert_cmodel_sync %d %d %d shader=\n%s",
// __LINE__,
// command->frame->get_color_model(),
// command->dst_cmodel,
// shader);

		const char *shader_stack[9];
		memset(shader_stack,0, sizeof(shader_stack));
		int current_shader = 0;

		if( shader ) {
//printf("Playback3D::convert_cmodel_sync %d\n", __LINE__);
			shader_stack[current_shader++] = shader;
			shader_stack[current_shader] = 0;
			unsigned int shader_id = VFrame::make_shader(shader_stack);

			command->frame->bind_texture(0);
			glUseProgram(shader_id);

			glUniform1i(glGetUniformLocation(shader_id, "tex"), 0);
			switch( type ) {
			case rgb_to_yuv:
				BC_GL_RGB_TO_YUV(shader_id);
				break;
			case yuv_to_rgb:
				BC_GL_YUV_TO_RGB(shader_id);
				break;
			}

			command->frame->draw_texture();
			if(shader) glUseProgram(0);
			command->frame->set_opengl_state(VFrame::SCREEN);
		}
	}

	command->canvas->unlock_canvas();
#endif // HAVE_GL
}

void Playback3D::do_fade(Canvas *canvas, VFrame *frame, float fade)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::DO_FADE;
	command.canvas = canvas;
	command.frame = frame;
	command.alpha = fade;
	send_command(&command);
}

void Playback3D::do_fade_sync(Playback3DCommand *command)
{
#ifdef HAVE_GL
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::do_fade_sync");
	if( window ) {
		window->enable_opengl();
		switch( command->frame->get_opengl_state() ) {
		case VFrame::RAM:
			command->frame->to_texture();
			break;

		case VFrame::SCREEN:
// Read back from PBuffer
// Bind context to pbuffer
			command->frame->enable_opengl();
			command->frame->screen_to_texture();
			break;
		}

		command->frame->enable_opengl();
		command->frame->init_screen();
		command->frame->bind_texture(0);

//		glClearColor(0.0, 0.0, 0.0, 0.0);
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		unsigned int frag_shader = 0;
		switch(command->frame->get_color_model())
		{
// For the alpha colormodels, the native function seems to multiply the
// components by the alpha instead of just the alpha.
			case BC_RGBA8888:
			case BC_RGBA_FLOAT:
			case BC_YUVA8888:
				frag_shader = VFrame::make_shader(0, fade_rgba_frag, 0);
				break;

			case BC_RGB888:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
				glColor4f(command->alpha, command->alpha, command->alpha, 1);
				break;


			case BC_YUV888:
				frag_shader = VFrame::make_shader(0, fade_yuv_frag, 0);
				break;
		}


		if( frag_shader ) {
			glUseProgram(frag_shader);
			int variable;
			if((variable = glGetUniformLocation(frag_shader, "tex")) >= 0)
				glUniform1i(variable, 0);
			if((variable = glGetUniformLocation(frag_shader, "alpha")) >= 0)
				glUniform1f(variable, command->alpha);
		}

		command->frame->draw_texture();
		command->frame->set_opengl_state(VFrame::SCREEN);

		if(frag_shader)
		{
			glUseProgram(0);
		}

		glColor4f(1, 1, 1, 1);
		glDisable(GL_BLEND);
	}
	command->canvas->unlock_canvas();
#endif
}


int Playback3D::run_plugin(Canvas *canvas, PluginClient *client)
{
	Playback3DCommand command;
	command.command = Playback3DCommand::PLUGIN;
	command.canvas = canvas;
	command.plugin_client = client;
	return send_command(&command);
}

void Playback3D::run_plugin_sync(Playback3DCommand *command)
{
	BC_WindowBase *window =
		command->canvas->lock_canvas("Playback3D::run_plugin_sync");
	if( window ) {
		window->enable_opengl();
		command->result = ((PluginVClient*)command->plugin_client)->handle_opengl();
	}
	command->canvas->unlock_canvas();
}


