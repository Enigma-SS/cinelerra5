
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

#ifndef BCDISPLAYINFO_H
#define BCDISPLAYINFO_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#ifdef HAVE_GL
#define GL_GLEXT_PROTOTYPES
#include <GL/glx.h>
#endif

class BC_DisplayInfo
{
public:
	BC_DisplayInfo(const char *display_name = "", int show_error = 1);
	~BC_DisplayInfo();

	friend class BC_WindowBase;
	friend class BC_Window;

	int get_root_w();
	int get_root_h();
	int get_abs_cursor_x();
	int get_abs_cursor_y();
	static void parse_geometry(char *geom, int *x, int *y, int *width, int *height);
// Get window border size created by window manager
	static int get_top_border();
	static int get_left_border();
	static int get_right_border();
	static int get_bottom_border();
	static const char *get_gl_shader_version();
	static int get_gl_max_texture_size();
	int get_screen_count();
	void test_window(int &x_out, int &y_out, int &x_out2, int &y_out2, int x_in, int y_in);
	static const char *host_display_name(const char *name);
	void init_window(const char *display_name, int show_error=0);
	int get_screen() { return scrnum; }
	int get_xinerama_screens();
	int xinerama_geometry(int screen, int &x, int &y, int &w, int &h);
	int xinerama_big_screen();
#ifdef HAVE_GL
	int gl_fb_config();
	int gl_probe(Window win);
	int ncfgs;
	GLXFBConfig *fb_cfgs, cfg;
	XVisualInfo *vis_info;
#endif

private:
	static void init_borders();
	Display* display;
	Window rootwin;
	Visual *vis;
	int scrnum;
	static int top_border;
	static int left_border;
	static int bottom_border;
	static int right_border;
	static int auto_reposition_x;
	static int auto_reposition_y;
	static int gl_max_texture_size;
	static char gl_shader_version[64];
	int depth;
	char *display_name;
	XineramaScreenInfo *xinerama_info;
	int xinerama_screens;
};

#endif
