
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bctexture.h"
#include "bcwindowbase.h"
#include "vframe.h"

#ifdef HAVE_GL
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int VFrame::get_opengl_state()
{
	return opengl_state;
}

void VFrame::set_opengl_state(int value)
{
	opengl_state = value;
}

int VFrame::get_window_id()
{
	return texture ? texture->window_id : -1;
}

int VFrame::get_texture_id()
{
	return texture ? texture->texture_id : -1;
}

int VFrame::get_texture_w()
{
	return texture ? texture->texture_w : 0;
}

int VFrame::get_texture_h()
{
	return texture ? texture->texture_h : 0;
}


int VFrame::get_texture_components()
{
	return texture ? texture->texture_components : 0;
}


void VFrame::to_texture()
{
#ifdef HAVE_GL

// Must be here so user can create textures without copying data by setting
// opengl_state to TEXTURE.
	BC_Texture::new_texture(&texture, get_w(), get_h(), get_color_model());

// Determine what to do based on state
	switch(opengl_state) {
	case VFrame::TEXTURE:
		return;

	case VFrame::SCREEN:
		if(pbuffer) {
			enable_opengl();
			screen_to_texture();
		}
		opengl_state = VFrame::TEXTURE;
		return;
	}

//printf("VFrame::to_texture %d\n", texture_id);
	GLenum type, format;
	switch(color_model) {
	case BC_RGB888:
	case BC_YUV888:
		type = GL_UNSIGNED_BYTE;
		format = GL_RGB;
		break;

	case BC_RGBA8888:
	case BC_YUVA8888:
		type = GL_UNSIGNED_BYTE;
		format = GL_RGBA;
		break;

	case BC_RGB_FLOAT:
		type = GL_FLOAT;
		format = GL_RGB;
		break;

	case BC_RGBA_FLOAT:
		format = GL_RGBA;
		type = GL_FLOAT;
		break;

	default:
		fprintf(stderr,
			"VFrame::to_texture: unsupported color model %d.\n",
			color_model);
		return;
	}

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, get_w(), get_h(),
		format, type, get_rows()[0]);
	opengl_state = VFrame::TEXTURE;
#endif
}

void VFrame::create_pbuffer()
{
#ifdef GLx4
	int ww = (get_w()+3) & ~3, hh = (get_h()+3) & ~3;
#else
	int ww = get_w(), hh = get_h();
#endif
	if( pbuffer && (pbuffer->w != ww || pbuffer->h != hh ||
	    pbuffer->window_id != BC_WindowBase::get_synchronous()->current_window->get_id() ) ) {
		delete pbuffer;
		pbuffer = 0;
	}

	if( !pbuffer ) {
		pbuffer = new BC_PBuffer(ww, hh);
	}
}

void VFrame::enable_opengl()
{
	create_pbuffer();
	if( pbuffer ) {
		pbuffer->enable_opengl();
	}
}

BC_PBuffer* VFrame::get_pbuffer()
{
	return pbuffer;
}


void VFrame::screen_to_texture(int x, int y, int w, int h)
{
#ifdef HAVE_GL
// Create texture
	BC_Texture::new_texture(&texture,
		get_w(), get_h(), get_color_model());
	if(pbuffer) {
		glEnable(GL_TEXTURE_2D);

// Read canvas into texture, use back texture for DOUBLE_BUFFER
#if 1
// According to the man page, it must be GL_BACK for the onscreen buffer
// and GL_FRONT for a single buffered PBuffer.  In reality it must be
// GL_BACK for a single buffered PBuffer if the PBuffer has alpha and using
// GL_FRONT captures the onscreen front buffer.
// 10/11/2010 is now generating "illegal operation"
		glReadBuffer(GL_BACK);
#else
		glReadBuffer(BC_WindowBase::get_synchronous()->is_pbuffer ?
			GL_FRONT : GL_BACK);
#endif
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			x >= 0 ? x : 0, y >= 0 ? y : 0,
			w >= 0 ? w : get_w(), h >= 0 ? h : get_h());
	}
#endif
}

void VFrame::screen_to_ram()
{
#ifdef HAVE_GL
	enable_opengl();
	glReadBuffer(GL_BACK);
	int type = BC_CModels::is_float(color_model) ? GL_FLOAT : GL_UNSIGNED_BYTE;
	int format = BC_CModels::has_alpha(color_model) ? GL_RGBA : GL_RGB;
	glReadPixels(0, 0, get_w(), get_h(), format, type, get_rows()[0]);
	opengl_state = VFrame::RAM;
#endif
}

void VFrame::draw_texture(
	float in_x1, float in_y1, float in_x2, float in_y2,
	float out_x1, float out_y1, float out_x2, float out_y2,
	int flip_y)
{
#ifdef HAVE_GL
	in_x1 /= get_texture_w();  in_y1 /= get_texture_h();
	in_x2 /= get_texture_w();  in_y2 /= get_texture_h();
	float ot_y1 = flip_y ? -out_y1 : -out_y2;
	float ot_y2 = flip_y ? -out_y2 : -out_y1;
	texture->draw_texture(
		in_x1,in_y1,  in_x2,in_y2,
		out_x1,ot_y1, out_x2, ot_y2);
#endif
}

void VFrame::draw_texture(int flip_y)
{
	draw_texture(0,0,  get_w(),get_h(),
		0,0, get_w(),get_h(), flip_y);
}


void VFrame::bind_texture(int texture_unit, int nearest)
{
// Bind the texture
	if(texture) {
		texture->bind(texture_unit, nearest);
	}
}


void VFrame::init_screen(int w, int h)
{
#ifdef HAVE_GL
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float near = 1;
	float far = 100;
	float frustum_ratio = near / ((near + far)/2);
 	float near_h = (float)h * frustum_ratio;
	float near_w = (float)w * frustum_ratio;
	glFrustum(-near_w/2, near_w/2, -near_h/2, near_h/2, near, far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
// Shift down and right so 0,0 is the top left corner
	glTranslatef(-w/2.f, h/2.f, 0.f);
	glTranslatef(0.0, 0.0, -(far + near) / 2);

	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
// Default for direct copy playback
	glDisable(GL_BLEND);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	glAlphaFunc(GL_ALWAYS, 0);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_LIGHTING);

	const GLfloat zero[] = { 0, 0, 0, 0 };
//	const GLfloat one[] = { 1, 1, 1, 1 };
//	const GLfloat light_position[] = { 0, 0, -1, 0 };
//	const GLfloat light_direction[] = { 0, 0, 1, 0 };

// 	glEnable(GL_LIGHT0);
// 	glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
// 	glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
// 	glLightfv(GL_LIGHT0, GL_SPECULAR, one);
// 	glLighti(GL_LIGHT0, GL_SPOT_CUTOFF, 180);
// 	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
// 	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
// 	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
// 	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0);
#ifndef GLx4
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#endif
#endif
}

void VFrame::init_screen()
{
	init_screen(get_w(), get_h());
}



#ifdef HAVE_GL

static int print_error(const char *text, unsigned int object, int is_program)
{
	char info[BCTEXTLEN];
	int len = 0;
	if( is_program )
		glGetProgramInfoLog(object, BCTEXTLEN, &len, info);
	else
		glGetShaderInfoLog(object, BCTEXTLEN, &len, info);
	if( len > 0 ) printf("Playback3D::print_error:\n%s\n%s\n", text, info);
	return !len ? 0 : 1;
}

static char *shader_segs(const char **segs, int n)
{
	if( !segs || !n ) return 0;
// concat source segs
	int ids = 0;
	char *ret = 0;
	for( int i=0; i<n; ++i ) {
		const char *text = *segs++;
		char src[strlen(text) + BCSTRLEN + 1], *sp = src;
		const char *tp = strstr(text, "main()");
		if( tp ) {
// Replace main() with a mainxxx()
			int n = tp - text;
			memcpy(sp, text, n);  sp += n;
			sp += sprintf(sp, "main%03d()", ids++);
			strcpy(sp, tp+strlen("main()"));
			text = src;
		}
		char *cp = !ret ? cstrdup(text) : cstrcat(2, ret, text);
		delete [] ret;  ret = cp;
	}

// add main() which calls mainxxx() in order
	char main_prog[BCTEXTLEN];
	char *cp = main_prog;
	cp += sprintf(cp, "\nvoid main() {\n");
	for( int i=0; i < ids; ++i )
		cp += sprintf(cp, "\tmain%03d();\n", i);
	cp += sprintf(cp, "}\n");
	if( ret ) {
		cp = cstrcat(2, ret, main_prog);
		delete [] ret;  ret = cp;
	}
	else
		ret = cstrdup(main_prog);
	return ret;
}

static int compile_shader(unsigned int &shader, int type, const GLchar *text)
{
	shader = glCreateShader(type);
	glShaderSource(shader, 1, &text, 0);
	glCompileShader(shader);
	return print_error(text, shader, 0);
}

static unsigned int build_shader(const char *vert, const char *frag)
{
	int error = 0;
	unsigned int vertex_shader = 0;
	unsigned int fragment_shader = 0;
	unsigned int program = glCreateProgram();
	if( !error && vert )
		error = compile_shader(vertex_shader, GL_VERTEX_SHADER, vert);
	if( !error && frag )
		error = compile_shader(fragment_shader, GL_FRAGMENT_SHADER, frag);
	if( !error && vert ) glAttachShader(program, vertex_shader);
	if( !error && frag ) glAttachShader(program, fragment_shader);
	if( !error ) glLinkProgram(program);
	if( !error ) error = print_error("link", program, 1);
	if( !error )
		BC_WindowBase::get_synchronous()->put_shader(program, vert, frag);
	else {
		glDeleteProgram(program);
		program = 0;
	}
	return program;
}

#endif

// call as:
//    make_shader(0, seg1, .., segn, 0);
// or make_shader(&seg);
// line 1: optional comment // vertex shader

unsigned int VFrame::make_shader(const char **segments, ...)
{
	unsigned int program = 0;
#ifdef HAVE_GL
// Construct single source file out of arguments
	int nb_segs = 0;
	if( !segments ) {  // arg list
		va_list list;  va_start(list, segments);
		while( va_arg(list, char*) != 0 ) ++nb_segs;
		va_end(list);
	}
	else { // segment list
		while( segments[nb_segs] ) ++nb_segs;
	}
	const char *segs[++nb_segs];
	if( !segments ) {
		va_list list;  va_start(list, segments);
		for( int i=0; i<nb_segs; ++i )
			segs[i] = va_arg(list, const char *);
		va_end(list);
		segments = segs;
	}

	const char *vert_shaders[nb_segs];  int vert_segs = 0;
	const char *frag_shaders[nb_segs];  int frag_segs = 0;
	for( int i=0; segments[i]!=0; ++i ) {
		const char *seg = segments[i];
		if( strstr(seg, "// vertex shader") )
			vert_shaders[vert_segs++] = seg;
		else
			frag_shaders[frag_segs++] = seg;
	}

	char *vert = shader_segs(vert_shaders, vert_segs);
	char *frag = shader_segs(frag_shaders, frag_segs);
	if( !BC_WindowBase::get_synchronous()->get_shader(&program, vert, frag) )
		program = build_shader(vert, frag);
	delete [] vert;
	delete [] frag;
#endif
	return program;
}

void VFrame::dump_shader(int shader_id)
{
	BC_WindowBase::get_synchronous()->dump_shader(shader_id);
}


void VFrame::clear_pbuffer()
{
#ifdef HAVE_GL
	int rgb = clear_color>=0 ? clear_color : 0;
	int a = clear_color>=0 ? clear_alpha : 0;
	int r = (rgb>>16) & 0xff;
	int g = (rgb>> 8) & 0xff;
	int b = (rgb>> 0) & 0xff;
	if( BC_CModels::is_yuv(get_color_model()) )
		YUV::yuv.rgb_to_yuv_8(r, g, b);
	glClearColor(r/255.f, g/255.f, b/255.f, a/255.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

