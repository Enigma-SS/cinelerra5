/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mandelbrot.h"
#include "mandelbrotwindow.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "theme.h"

MandelbrotWindow::MandelbrotWindow(Mandelbrot *plugin)
 : PluginClientWindow(plugin, xS(180), yS(130), xS(180), yS(130), 0)
{
	this->plugin = plugin; 
	press_x = press_y = 0;
	button_no = 0;
	pending_config = 0;
}

MandelbrotWindow::~MandelbrotWindow()
{
}

void MandelbrotWindow::create_objects()
{
	int x = xS(10), y = yS(10), pad = xS(5);
	BC_Title *title;
	add_subwindow(title = new BC_Title(x,y, _("Mandelbrot:"), MEDIUMFONT, YELLOW));
	y += title->get_h() + pad;
	add_subwindow(is_julia = new MandelbrotIsJulia(this, x, y));
	y += is_julia->get_h() + pad;
	add_subwindow(drag = new MandelbrotDrag(this, x, y));
	y += drag->get_h() + pad;
	add_subwindow(reset = new MandelbrotReset(this, x, y));
	show_window();
}

void MandelbrotWindow::update_gui()
{
}


void MandelbrotWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}

int MandelbrotWindow::grab_event(XEvent *event)
{
	int ret = do_grab_event(event);
	if( pending_config && !grab_event_count() )
		send_configure_change();
	return ret;
}

int MandelbrotWindow::do_grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	default:
		return 0;
	}

	MWindow *mwindow = plugin->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cursor_x, cursor_y;
	cwindow_gui->get_relative_cursor(cursor_x, cursor_y);
	cursor_x -= canvas->view_x;
	cursor_y -= canvas->view_y;
	float output_x = cursor_x, output_y = cursor_y;
	canvas->canvas_to_output(mwindow->edl, 0, output_x, output_y);

	if( !button_no ) {
		if( cursor_x < 0 || cursor_x >= canvas->view_w ||
		    cursor_y < 0 || cursor_y >= canvas->view_h )
			return 0;
	}

	switch( event->type ) {
	case ButtonPress:
		if( button_no ) return 0;
		press_x = output_x;  press_y = output_y;
		button_no = event->xbutton.button;
		break;
	case ButtonRelease:
		if( !button_no ) return 0;
		button_no = 0;
		return 1;
	case MotionNotify: {
		if( !button_no ) return 0;
		EDL *edl = plugin->get_edl();
		double dx = 0, dy = 0, jx = 0, jy = 0, ds = 1;
		double out_w = edl->session->output_w, out_h = edl->session->output_h;
		double fx = (double)(press_x - output_x) / (2. * out_w);
		double fy = (double)(press_y - output_y) / (2. * out_h);
		press_x = output_x;  press_y = output_y;
		switch( button_no ) {
		case LEFT_BUTTON: {
			dx = fx * plugin->config.scale;
			dy = fy * plugin->config.scale;
			break; }
		case MIDDLE_BUTTON: {
			ds = fy >= 0.f ? 1-fy : 1/(1+fy);
			bclamp(ds, 1-0.05f, 1+0.05f);
			break; }
		case RIGHT_BUTTON: {
			jx = fx;
			jy = fy;
			break; }
		}
		plugin->config.x_off += dx;
		plugin->config.y_off += dy;
		plugin->config.x_julia += jx;
		plugin->config.y_julia += jy;
		plugin->config.scale *= ds;
		pending_config = 1;
		break; }
	default:
		return 0;
	}

	return 1;
}

void MandelbrotWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
}
MandelbrotDrag::MandelbrotDrag(MandelbrotWindow *gui, int x, int y)
 : BC_CheckBox(x, y, 0, _("Drag"))
{
	this->gui = gui;
}
int MandelbrotDrag::handle_event()
{
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
	if( get_value() ) {
		if( !gui->grab(cwindow_gui) ) {
			update(*value = 0);
			flicker(10,50);
		}
	}
	else
		gui->ungrab(cwindow_gui);
	return 1;
}
int MandelbrotDrag::handle_ungrab()
{
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
	int ret = gui->ungrab(cwindow_gui);
	if( ret ) update(*value = 0);
	return ret;
}

MandelbrotIsJulia::MandelbrotIsJulia(MandelbrotWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.is_julia, _("Julia"))
{
	this->gui = gui;
}
MandelbrotIsJulia::~MandelbrotIsJulia()
{
}

int MandelbrotIsJulia::handle_event()
{
	Mandelbrot *plugin = gui->plugin;
	plugin->config.is_julia = get_value();
	gui->send_configure_change();
	return 1;
}

MandelbrotReset::MandelbrotReset(MandelbrotWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}
MandelbrotReset::~MandelbrotReset()
{
}

int MandelbrotReset::handle_event()
{
	Mandelbrot *plugin = gui->plugin;
	int is_julia = plugin->config.is_julia;
	plugin->config.reset();
	if( is_julia )
		plugin->config.startJulia();
	gui->send_configure_change();
	return 1;
}



