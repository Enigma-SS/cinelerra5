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
#include "language.h"
#include "nbody.h"
#include "nbodywindow.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "pluginserver.h"
#include "theme.h"

static const char *display_modes[] = {
	N_("point"),
	N_("sprite"),
	N_("color"),
};
static const int num_display_modes = sizeof(display_modes)/sizeof(*display_modes);

N_BodyWindow::N_BodyWindow(N_BodyMain *plugin)
 : PluginClientWindow(plugin, xS(240), yS(200), xS(240), yS(200), 0)
{
	this->plugin = plugin; 
	press_x = press_y = 0;
	button_no = 0;
	pending_config = 0;
}

N_BodyWindow::~N_BodyWindow()
{
}

void N_BodyWindow::create_objects()
{
	int x = xS(10), y = yS(10), pad = xS(5);
	int x1 = xS(100);
	BC_Title *title;
	add_subwindow(title = new BC_Title(x,y, _("NBody"), MEDIUMFONT, YELLOW));
	y += title->get_h() + 2*pad;
	add_subwindow(title = new BC_Title(x,y, _("set demo:")));
	set_demo = new N_BodySetDemo(this, x1,y, "0");
	set_demo->create_objects();
	y += set_demo->get_h() + pad;
	add_subwindow(title = new BC_Title(x,y, _("draw mode:")));
	set_mode = new N_BodySetMode(this, x1,y, _(display_modes[plugin->config.mode]));
	set_mode->create_objects();
	y += set_mode->get_h() + pad;
	add_subwindow(title = new BC_Title(x,y, _("numBodies:")));
	char text[BCSTRLEN];
	sprintf(text, "%d", plugin->config.numBodies);
	num_bodies = new N_BodyNumBodies(this, x1,y, text);
	num_bodies->create_objects();
	y += num_bodies->get_h() + pad;
	add_subwindow(drag = new N_BodyDrag(this, x, y));
	y += drag->get_h() + pad;
	add_subwindow(reset = new N_BodyReset(this, x, y));
	show_window();
}

void N_BodyWindow::update_gui()
{
	set_mode->update(_(display_modes[plugin->config.mode]));
	char text[BCSTRLEN];
	sprintf(text, "%d", plugin->config.numBodies);
	num_bodies->update(text);
}


void N_BodyWindow::send_configure_change()
{
	pending_config = 0;
	plugin->send_configure_change();
}

int N_BodyWindow::grab_event(XEvent *event)
{
	int ret = do_grab_event(event);
	if( pending_config && !grab_event_count() )
		send_configure_change();
	return ret;
}

int N_BodyWindow::do_grab_event(XEvent *event)
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
		N_BodyConfig &config = plugin->config;
		double dx = (double)(press_x - output_x);
		double dy = (double)(press_y - output_y);
		press_x = output_x;  press_y = output_y;
		switch( button_no ) {
		case LEFT_BUTTON: {
			config.trans[0] += dx / 5.0f;
			config.trans[1] -= dy / 5.0f;
			break; }
		case MIDDLE_BUTTON: {
			double s = 0.5f * fabs(config.trans[2]);
			if( s < 0.1 ) s = 0.1;
			config.trans[2] += (dy / 100.0f) * s;
			break; }
		case RIGHT_BUTTON: {
			config.rot[0] += dy / 5.0f;
			config.rot[1] += dx / 5.0f;
			break; }
		}
		pending_config = 1;
		break; }
	default:
		return 0;
	}

	return 1;
}

void N_BodyWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
}

N_BodyDrag::N_BodyDrag(N_BodyWindow *gui, int x, int y)
 : BC_CheckBox(x, y, 0, _("Drag"))
{
	this->gui = gui;
}
int N_BodyDrag::handle_event()
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
int N_BodyDrag::handle_ungrab()
{
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
	int ret = gui->ungrab(cwindow_gui);
	if( ret ) update(*value = 0);
	return ret;
}

N_BodySetMode::N_BodySetMode(N_BodyWindow *gui, int x, int y, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, xS(100), yS(160))
{
	this->gui = gui;
}

void N_BodySetMode::create_objects()
{
	BC_PopupTextBox::create_objects();
	for( int i=0; i<num_display_modes; ++i )
		mode_items.append(new BC_ListBoxItem(_(display_modes[i])));
	update_list(&mode_items);
}

int N_BodySetMode::handle_event()
{
	N_BodyMain *plugin = gui->plugin;
	plugin->config.mode = get_number();
	plugin->send_configure_change();
	return 1;
}


N_BodySetDemo::N_BodySetDemo(N_BodyWindow *gui, int x, int y, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, xS(100), yS(160))
{
	this->gui = gui;
}

void N_BodySetDemo::create_objects()
{
	BC_PopupTextBox::create_objects();
	for( int i=0; i<N_BodyParams::num_demos; ++i ) {
		char text[BCSTRLEN];  sprintf(text,"%d",i);
		demo_items.append(new BC_ListBoxItem(text));
	}
	update_list(&demo_items);
}

int N_BodySetDemo::handle_event()
{
	N_BodyMain *plugin = gui->plugin;
	plugin->selectDemo(get_number());
	gui->update_gui();
	plugin->send_configure_change();
	return 1;
}

N_BodyNumBodies::N_BodyNumBodies(N_BodyWindow *gui, int x, int y, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, xS(100), yS(160))
{
	this->gui = gui;
}

void N_BodyNumBodies::create_objects()
{
	BC_PopupTextBox::create_objects();
	for( int i=0; i<8; ++i ) {
		char text[BCSTRLEN];  sprintf(text,"%d",1<<(i+6));
		num_body_items.append(new BC_ListBoxItem(text));
	}
	update_list(&num_body_items);
}

int N_BodyNumBodies::handle_event()
{
	N_BodyMain *plugin = gui->plugin;
	int i = get_number();
	int n = i >= 0 ? (1 << (i+6)) : atoi(get_text());
	bclamp(n, 0x10,0x4000);
	plugin->config.numBodies = n;
	plugin->send_configure_change();
	return 1;
}

N_BodyReset::N_BodyReset(N_BodyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}
N_BodyReset::~N_BodyReset()
{
}

int N_BodyReset::handle_event()
{
	N_BodyMain *plugin = gui->plugin;
	plugin->config.reset();
	gui->update_gui();
	gui->send_configure_change();
	return 1;
}

