/*
 * CINELERRA
 * Copyright (C) 2008-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef MANDELCUDAWINDOW_H
#define MANDELCUDAWINDOW_H


#include "guicast.h"
#include "mandelbrot.h"

class Mandelbrot;
class MandelbrotIsJulia;
class MandelbrotDrag;
class MandelbrotWindow;

class MandelbrotIsJulia : public BC_CheckBox
{
public:
	MandelbrotIsJulia(MandelbrotWindow *gui, int x, int y);
	~MandelbrotIsJulia();
	int handle_event();

	MandelbrotWindow *gui;
};

class MandelbrotReset : public BC_GenericButton
{
public:
	MandelbrotReset(MandelbrotWindow *gui, int x, int y);
	~MandelbrotReset();
	int handle_event();

	MandelbrotWindow *gui;
};

class MandelbrotDrag : public BC_CheckBox
{
public:
	MandelbrotDrag(MandelbrotWindow *gui, int x, int y);

	int handle_event();
	int handle_ungrab();
	MandelbrotWindow *gui;
};

class MandelbrotWindow : public PluginClientWindow
{
public:
	MandelbrotWindow(Mandelbrot *plugin);
	~MandelbrotWindow();
	void create_objects();
	void update_gui();

	int grab_event(XEvent *event);
	int do_grab_event(XEvent *event);
	void done_event(int result);
	void send_configure_change();

	Mandelbrot *plugin;
	MandelbrotIsJulia *is_julia;
	MandelbrotDrag *drag;
	MandelbrotReset *reset;
	int press_x, press_y;
	int button_no, pending_config;
};

#endif
