
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

#include "bcmenubar.h"
#include "bcmenuitem.h"
#include "bcmenupopup.h"
#include "bcpixmap.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bccolors.h"
#include "fonts.h"
#include <string.h>
#include "vframe.h"

#define BUTTON_UP 0
#define BUTTON_HI 1
#define BUTTON_DN 2
#define TOTAL_IMAGES 3


#define TRIANGLE_W xS(10)
#define TRIANGLE_H yS(10)


BC_PopupMenu::BC_PopupMenu(int x, int y, int w, const char *text,
		int use_title, VFrame **data, int margin)
 : BC_SubWindow(x, y, 0, 0, -1)
{
	highlighted = popup_down = 0;
	menu_popup = 0;
	icon = 0;
	this->margin = margin >= 0 ? margin :
		BC_WindowBase::get_resources()->popupmenu_margin;
	this->use_title = use_title;
	strcpy(this->text, text);
	for( int i=0; i<TOTAL_IMAGES; ++i ) images[i] = 0;
	this->data = data;
	this->w_argument = w;
	status = BUTTON_UP;
	pending = 0;
}

BC_PopupMenu::BC_PopupMenu(int x, int y, const char *text,
		int use_title, VFrame **data)
 : BC_SubWindow(x, y, 0, -1, -1)
{
	highlighted = popup_down = 0;
	menu_popup = 0;
	icon = 0;
	this->margin = BC_WindowBase::get_resources()->popupmenu_margin;
	this->use_title = use_title;
	strcpy(this->text, text);
	for( int i=0; i<TOTAL_IMAGES; ++i ) images[i] = 0;
	this->data = data;
	this->w_argument = -1;
	status = BUTTON_UP;
	pending = 0;
}

BC_PopupMenu::~BC_PopupMenu()
{
	use_title = 0;
	deactivate();
	delete menu_popup;
	for( int i=0; i<TOTAL_IMAGES; ++i ) delete images[i];
}

char* BC_PopupMenu::get_text()
{
	return text;
}

void BC_PopupMenu::set_text(const char *text)
{
	if( use_title ) {
		strcpy(this->text, text);
		draw_title(1);
	}
}

void BC_PopupMenu::set_icon(BC_Pixmap *icon)
{
	if( use_title ) {
		this->icon = icon;
		if( menu_popup ) draw_title(1);
	}
}

int BC_PopupMenu::initialize()
{
	if( use_title ) {
		if( data )
			set_images(data);
		else
		if( BC_WindowBase::get_resources()->popupmenu_images )
			set_images(BC_WindowBase::get_resources()->popupmenu_images);
		else
			set_images(BC_WindowBase::get_resources()->generic_button_images);
	}
	else {
// Move outside window if no title
		x = -TRIANGLE_W;  y = -TRIANGLE_H;
		w = TRIANGLE_W;   h = TRIANGLE_H;
	}

	BC_SubWindow::initialize();

	menu_popup = new BC_MenuPopup;
	menu_popup->initialize(top_level, 0, 0, 0, this);

	if( use_title ) draw_title(0);

	return 0;
}

int BC_PopupMenu::set_images(VFrame **data)
{
	for( int i=0; i<3; ++i ) {
		delete images[i];
		images[i] = new BC_Pixmap(parent_window, data[i], PIXMAP_ALPHA);
	}

	w = w_argument > 0 ? w_argument :
		calculate_w(margin, get_text_width(MEDIUMFONT, text), use_title);
	h = images[BUTTON_UP]->get_h();
	return 0;
}

int BC_PopupMenu::calculate_w(int margin, int text_width, int use_title)
{
	BC_Resources *resources = get_resources();
	int l = margin >= 0 ? margin : resources->popupmenu_margin;
	int r = use_title < 0 ? l : l + resources->popupmenu_triangle_margin;
	return l + text_width + r;
}

int BC_PopupMenu::calculate_w(int text_width)
{
	return calculate_w(-1, text_width, 0);
}

int BC_PopupMenu::calculate_h(VFrame **data)
{
	if( !data ) data = BC_WindowBase::get_resources()->popupmenu_images ?
		BC_WindowBase::get_resources()->popupmenu_images :
		BC_WindowBase::get_resources()->generic_button_images ;

	return data[BUTTON_UP]->get_h();
}

int BC_PopupMenu::add_item(BC_MenuItem *item)
{
	menu_popup->add_item(item);
	return 0;
}

int BC_PopupMenu::remove_item(BC_MenuItem *item)
{
	menu_popup->remove_item(item);
	return 0;
}

int BC_PopupMenu::del_item(BC_MenuItem *item)
{
	menu_popup->del_item(item);
	return 0;
}

int BC_PopupMenu::total_items()
{
	return menu_popup->total_items();
}

BC_MenuItem* BC_PopupMenu::get_item(int i)
{
	return menu_popup->menu_items.values[i];
}

int BC_PopupMenu::get_margin()
{
	return margin;
}

int BC_PopupMenu::draw_face(int dx, int color)
{
	if( !use_title ) return 0;

// Background
	draw_top_background(parent_window, 0, 0, w, h);
	draw_3segmenth(0, 0, w, images[status]);

// Overlay text
	if( color < 0 ) color = get_resources()->popup_title_text;
	set_color(color);

	int offset = status == BUTTON_DN ? 1 : 0;
	int available_w = get_w() - calculate_w(margin, 0, use_title);

	if( !icon ) {
		char *truncated = get_truncated_text(MEDIUMFONT, text, available_w);
		set_font(MEDIUMFONT);
		BC_WindowBase::draw_center_text(
			dx + available_w/2 + margin + offset,
			(int)((float)get_h()/2 + get_text_ascent(MEDIUMFONT)/2 - 2) + offset,
			truncated);
		delete [] truncated;
	}

	if( icon ) {
		draw_pixmap(icon,
			available_w/ 2 + margin + offset - icon->get_w()/2 ,
			get_h()/2 - icon->get_h()/2 + offset);
	}

	if( use_title >= 0 ) {
		int tx = get_w() - margin - get_resources()->popupmenu_triangle_margin;
		int ty = get_h()/2 - TRIANGLE_H/2;
		draw_triangle_down_flat(tx, ty, TRIANGLE_W, TRIANGLE_H);
	}
	return 1;
}

int BC_PopupMenu::draw_title(int flush)
{
	draw_face(0, -1);
	flash(flush);
	return 0;
}

int BC_PopupMenu::deactivate()
{
	if( popup_down ) {
		top_level->active_popup_menu = 0;
		popup_down = 0;
		menu_popup->deactivate_menu();

		if( use_title ) draw_title(1);    // draw the title
	}
	return 0;
}

int BC_PopupMenu::activate_menu()
{
	if( !get_button_down() || !BC_WindowBase::get_resources()->popupmenu_btnup )
		return menu_activate();
	top_level->active_popup_menu = this;
	pending = 1;
	return 0;
}

int BC_PopupMenu::menu_activate()
{
	pending = 0;
	if( !popup_down ) {
		int x = this->x;
		int y = this->y;

		top_level->deactivate();
		top_level->active_popup_menu = this;
		if( !use_title ) {
			x = top_level->get_abs_cursor_x(0) - get_w();
			y = top_level->get_abs_cursor_y(0) - get_h();
			button_press_x = top_level->cursor_x;
			button_press_y = top_level->cursor_y;
		}

		if( use_title ) {
			Window tempwin;
			int new_x, new_y;
			XTranslateCoordinates(top_level->display,
				win, top_level->rootwin,
				0, 0, &new_x, &new_y, &tempwin);
			menu_popup->activate_menu(new_x, new_y,
				w, h, 0, 1);
		}
		else
			menu_popup->activate_menu(x+xS(3), y+yS(3), w, h, 0, 1);
		popup_down = 1;
		if( use_title ) draw_title(1);
	}
	else
		deactivate_menu();
	return 1;
}

int BC_PopupMenu::deactivate_menu()
{
	deactivate();
	return 0;
}


int BC_PopupMenu::reposition_window(int x, int y)
{
	BC_WindowBase::reposition_window(x, y);
	draw_title(0);
	return 0;
}

int BC_PopupMenu::focus_out_event()
{
	if( popup_down && !get_button_down() &&
	    !cursor_inside() && !menu_popup->cursor_inside() )
		deactivate();
	return 0;
}


int BC_PopupMenu::repeat_event(int64_t duration)
{
	if( status == BUTTON_HI &&
		tooltip_text && tooltip_text[0] != 0 &&
		duration == top_level->get_resources()->tooltip_delay ) {
		show_tooltip();
		return 1;
	}
	return 0;
}

int BC_PopupMenu::button_press_event()
{
	int result = 0;
	if( get_buttonpress() == 1 && is_event_win() && use_title ) {
		top_level->hide_tooltip();
		if( status == BUTTON_HI || status == BUTTON_UP ) status = BUTTON_DN;
		activate_menu();
		draw_title(1);
		return 1;
	}

	// Scrolling section
	if( is_event_win()
		&& (get_buttonpress() == 4 || get_buttonpress() == 5)
		&& menu_popup->total_items() > 1  ) {
		int theval = -1;
		for( int i=0; i<menu_popup->total_items(); ++i ) {
			if( !strcmp(menu_popup->menu_items.values[i]->get_text(),get_text()) ) {
				theval = i;
				break;
			}
		}

		if( theval == -1 ) theval = 0;
		else if( get_buttonpress() == 4 ) --theval;
		else if( get_buttonpress() == 5 ) ++theval;

		if( theval < 0 )
			theval = 0;
		else if( theval >= menu_popup->total_items() )
			theval = menu_popup->total_items() - 1;

		BC_MenuItem *tmp = menu_popup->menu_items.values[theval];
		set_text(tmp->get_text());
		result = tmp->handle_event();
		if( !result )
			result = this->handle_event();
	}
	if( popup_down ) {
// Menu is down so dispatch to popup.
		menu_popup->dispatch_button_press();
		result = 1;
	}

	return result;
}

int BC_PopupMenu::button_release_event()
{
// try the title
	int result = 0;

	if( is_event_win() && use_title ) {
		hide_tooltip();
		if( status == BUTTON_DN ) {
			status = BUTTON_HI;
			draw_title(1);
		}
	}

	if( pending )
		return menu_activate();

	if( !use_title && status == BUTTON_DN ) {
		result = 1;
	}
	else if( popup_down && menu_popup->cursor_inside() ) {
// Menu is down so dispatch to popup.
		result = menu_popup->dispatch_button_release();
	}
// released outside popup
	if( get_resources()->popupmenu_btnup && !result && popup_down ) {
		deactivate();
		result = 1;
	}
	hide_tooltip();

	return result;
}

int BC_PopupMenu::translation_event()
{
//printf("BC_PopupMenu::translation_event 1\n");
	if( popup_down ) menu_popup->dispatch_translation_event();
	return 0;
}

int BC_PopupMenu::cursor_leave_event()
{

	if( status == BUTTON_HI && use_title ) {
		status = BUTTON_UP;
		draw_title(1);
		hide_tooltip();
	}

// dispatch to popup
	if( popup_down ) {
		if( !get_button_down() && !menu_popup->cursor_inside() ) {
			status = BUTTON_UP;
//			deactivate_menu();
		}
		menu_popup->dispatch_cursor_leave();
	}

	return 0;
}


int BC_PopupMenu::cursor_enter_event()
{
	if( is_event_win() && use_title ) {
		if( top_level->button_down ) {
			status = BUTTON_DN;
		}
		else
		if( status == BUTTON_UP )
			status = BUTTON_HI;
		draw_title(1);
	}

	return 0;
}

int BC_PopupMenu::cursor_motion_event()
{
	int result = 0;

// This menu is down.
	if( popup_down ) {
		result = menu_popup->dispatch_motion_event();
	}

	if( !result && use_title && is_event_win() ) {
		if( highlighted ) {
			if( !cursor_inside() ) {
				highlighted = 0;
				draw_title(1);
			}
		}
		else {
			if( cursor_inside() ) {
				highlighted = 1;
				draw_title(1);
				result = 1;
			}
		}
	}

	return result;
}

int BC_PopupMenu::drag_start_event()
{
//printf("BC_PopupMenu::drag_start_event %d\n", popup_down);
	if( popup_down ) return 1;
	return 0;
}

int BC_PopupMenu::drag_stop_event()
{
	if( popup_down ) return 1;
	return 0;
}

int BC_PopupMenu::drag_motion_event()
{
	if( popup_down ) return 1;
	return 0;
}

