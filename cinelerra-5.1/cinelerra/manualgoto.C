
/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "manualgoto.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"

#define MGT_W xS(285)
#define MGT_H yS(80)

ManualGoto::ManualGoto(MWindow *mwindow, EditPanel *panel)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->panel = panel;
	window = 0;
}

ManualGoto::~ManualGoto()
{
	close_window();
}

BC_Window *ManualGoto::new_gui()
{
	BC_DisplayInfo dpy_info;
	int x = dpy_info.get_abs_cursor_x() - MGT_W / 2;
	int y = dpy_info.get_abs_cursor_y() - MGT_H / 2;
	window = new ManualGotoWindow(this, x, y);
	window->create_objects();
	window->show_window();
	return window;
}

void ManualGoto::handle_done_event(int result)
{
	if( result ) return;
	double current_position = panel->get_position();
	const char *text = window->time_text->get_text();
	double new_position = Units::text_to_seconds(text,
			mwindow->edl->session->sample_rate,
			window->time_format,
			mwindow->edl->session->frame_rate,
			mwindow->edl->session->frames_per_foot);
	char modifier = window->direction->get_text()[0];
	switch( modifier ) {
	case '+':  new_position = current_position + new_position;  break;
	case '-':  new_position = current_position - new_position;  break;
	default: break;
	}
	panel->subwindow->lock_window("ManualGoto::handle_done_event");
	panel->set_position(new_position);
	panel->subwindow->unlock_window();
}

ManualGotoWindow::ManualGotoWindow(ManualGoto *mango, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Goto position"), x, y,
	MGT_W, MGT_H, MGT_W, MGT_H, 0, 0, 1)
{
	this->mango = mango;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transport and Buttons Bar");
}

ManualGotoWindow::~ManualGotoWindow()
{
}


ManualGotoText::ManualGotoText(ManualGotoWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, "")
{
	this->window = window;
}

int ManualGotoText::keypress_event()
{
        int key = get_keypress();
        if( key == '+' || key == '-' || key == '=' ) {
		char text[2];  text[0] = key;  text[1] = 0;
                window->direction->set_text(text);
                return 1;
        }
	if( key == RETURN ) {
		unlock_window();
		window->mango->handle_done_event(0);
		lock_window("ManualGotoText::handle_event");
                return 1;
	}
	return BC_TextBox::keypress_event();
}

void ManualGotoWindow::update(double position)
{
	MWindow *mwindow = mango->mwindow;
	format_text->update(Units::timetype_toformat(time_format));
	char string[BCSTRLEN];
	Units::totext(string, position, time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot,
		mwindow->get_timecode_offset());
	time_text->update(string);
}

void ManualGotoWindow::update()
{
	double position = mango->panel->get_position();
	update(position);
}

ManualGotoKeyItem::ManualGotoKeyItem(ManualGotoDirection *popup,
		const char *text, const char *htxt)
 : BC_MenuItem(text, htxt, htxt[0])
{
	this->popup = popup;
	this->htxt = htxt;
}

int ManualGotoKeyItem::handle_event()
{
	popup->set_text(htxt);
	return 1;
}

ManualGotoDirection::ManualGotoDirection(ManualGotoWindow *window,
	int x, int y, int w)
 : BC_PopupMenu(x, y, w, "=", -1, 0, 1)
{
	this->window = window;
}

void ManualGotoDirection::create_objects()
{
	add_item(new ManualGotoKeyItem(this, _("Forward"),  "+"));
	add_item(new ManualGotoKeyItem(this, _("Position"), "="));
	add_item(new ManualGotoKeyItem(this, _("Reverse"),  "-"));
}

ManualGotoUnitItem::ManualGotoUnitItem(ManualGotoUnits *popup, int type)
 : BC_MenuItem(Units::timetype_toformat(type))
{
	this->popup = popup;
	this->type = type;
}

int ManualGotoUnitItem::handle_event()
{
	popup->window->time_format = type;
	popup->window->update();
	return 1;
}

ManualGotoUnits::ManualGotoUnits(ManualGotoWindow *window, int x, int y, int w)
 : BC_PopupMenu(x, y, w, "", 1, 0, 0)
{
	this->window = window;
}

void ManualGotoUnits::create_objects()
{
	add_item(new ManualGotoUnitItem(this, TIME_HMS));
	add_item(new ManualGotoUnitItem(this, TIME_HMSF));
	add_item(new ManualGotoUnitItem(this, TIME_TIMECODE));
	add_item(new ManualGotoUnitItem(this, TIME_FRAMES));
	add_item(new ManualGotoUnitItem(this, TIME_SAMPLES));
	add_item(new ManualGotoUnitItem(this, TIME_SAMPLES_HEX));
	add_item(new ManualGotoUnitItem(this, TIME_SECONDS));
	add_item(new ManualGotoUnitItem(this, TIME_FEET_FRAMES));
}

void ManualGotoWindow::create_objects()
{
	lock_window("ManualGotoWindow::create_objects");
	MWindow *mwindow = mango->mwindow;
	time_format = mwindow->edl->session->time_format;
	int margin = mwindow->theme->widget_border;
	int x = xS(10) + BC_OKButton::calculate_w() + margin, y = yS(10);
	int pop_w = xS(24), x1 = x + pop_w + margin;
	add_subwindow(format_text = new BC_Title(x1, y, ""));
	y += format_text->get_h() + margin;
	add_subwindow(direction = new ManualGotoDirection(this, x, y, pop_w));
	direction->create_objects();
	int tw = get_w() - x1 - xS(10) - BC_CancelButton::calculate_w() - margin - pop_w - margin;
	add_subwindow(time_text = new ManualGotoText(this, x1, y, tw));
	x1 += time_text->get_w() + margin;
	add_subwindow(units = new ManualGotoUnits(this, x1, y, pop_w));
	units->create_objects();
	update();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	unlock_window();
}

