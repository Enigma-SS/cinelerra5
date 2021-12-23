
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
#include "bcpopup.h"
#include "bcpopupmenu.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcwindowbase.h"
#include "bccolors.h"
#include "cstrdup.h"

#include <string.h>


#define MENUITEM_UP 0
#define MENUITEM_HI 1
#define MENUITEM_DN 2


#define MENUITEM_MARGIN 2

// ================================ Menu Item ==================================

BC_MenuItem::BC_MenuItem(const char *text, const char *hotkey_text, int hotkey)
{
	this->text = 0;
	this->hotkey_text = 0;
	reset();

	if(text) set_text(text);
	if(hotkey_text) set_hotkey_text(hotkey_text);

	this->hotkey = hotkey;
	checked = 0;
	highlighted = 0;
	down = 0;
	submenu = 0;
	shift_hotkey = 0;
	alt_hotkey = 0;
	ctrl_hotkey = 0;
	menu_popup = 0;
	enabled = 1;
}

BC_MenuItem::~BC_MenuItem()
{
	if(text) delete [] text;
	text = 0;
	if(hotkey_text) delete [] hotkey_text;
	hotkey_text = 0;
	if(submenu) delete submenu;
	submenu = 0;
	if(menu_popup)
		menu_popup->remove_item(this);
}

void BC_MenuItem::reset()
{
	set_text("");
	set_hotkey_text("");
	icon = 0;
}

int BC_MenuItem::initialize(BC_WindowBase *top_level, BC_MenuBar *menu_bar, BC_MenuPopup *menu_popup)
{
	this->top_level = top_level;
	this->menu_popup = menu_popup;
	this->menu_bar = menu_bar;
	return 0;
}

int BC_MenuItem::set_checked(int value)
{
	this->checked = value;
	return 0;
}

int BC_MenuItem::get_checked()
{
	return checked;
}

void BC_MenuItem::set_icon(BC_Pixmap *icon)
{
	this->icon = icon;
}

BC_Pixmap* BC_MenuItem::get_icon()
{
	return icon;
}

void BC_MenuItem::set_text(const char *text)
{
	delete [] this->text;
	this->text = cstrdup(text);
}

void BC_MenuItem::set_hotkey_text(const char *text)
{
	delete [] this->hotkey_text;
	this->hotkey_text = cstrdup(text);
}

int BC_MenuItem::deactivate_submenus(BC_MenuPopup *exclude)
{
	if(submenu && submenu != exclude)
	{
		submenu->deactivate_submenus(exclude);
		submenu->deactivate_menu();
		submenu->popup_menu = 0;
		highlighted = 0;
	}
	return 0;
}

int BC_MenuItem::get_enabled()
{
	return enabled;
}
void BC_MenuItem::set_enabled(int v)
{
	enabled = v;
}


int BC_MenuItem::activate_submenu()
{
	int new_x, new_y;
	if(menu_popup->popup && submenu && !submenu->popup)
	{
		Window tempwin;
		XTranslateCoordinates(top_level->display,
			menu_popup->get_popup()->win,
			top_level->rootwin,
			0,
			y,
			&new_x,
			&new_y,
			&tempwin);
		submenu->popup_menu = menu_popup->popup_menu;
		submenu->activate_menu(new_x + xS(5), new_y, menu_popup->w - xS(10), h, 0, 0);
		highlighted = 1;
	}
	return 0;
}


int BC_MenuItem::dispatch_button_press()
{
	int result = 0;

	if(submenu)
	{
		result = submenu->dispatch_button_press();
	}

	if(!result && menu_popup->get_popup()->is_event_win())
	{
		if(top_level->cursor_x >= 0 && top_level->cursor_x < menu_popup->get_w() &&
			top_level->cursor_y >= y && top_level->cursor_y < y + h)
		{
			if(!highlighted)
			{
				highlighted = 1;
			}
			result = 1;
		}
		else
		if(highlighted)
		{
			highlighted = 0;
			result = 1;
		}
	}

	return result;
}

int BC_MenuItem::dispatch_button_release(int &redraw)
{
	int len = strlen(text);
	if( len > 0 && text[0] == '-' && text[len-1] == '-' ) return 0;

	int result = 0;
	if(submenu)
	{
		result = submenu->dispatch_button_release();
	}

	if( !result && menu_popup->cursor_inside() ) {
		int cursor_x, cursor_y;
		menu_popup->get_popup()->get_relative_cursor(cursor_x, cursor_y);
		if( cursor_x >= 0 && cursor_x < menu_popup->get_w() &&
			cursor_y >= y && cursor_y < y + h ) {
			if(menu_bar)
				menu_bar->deactivate();
			else
				menu_popup->popup_menu->deactivate();

			if(!handle_event() && menu_popup && menu_popup->popup_menu)
			{
				menu_popup->popup_menu->set_text(text);
				menu_popup->popup_menu->handle_event();
			}
			return 1;
		}
	}
	return 0;
}

int BC_MenuItem::dispatch_motion_event(int &redraw)
{
	int result = 0;

	if(submenu)
	{
		result = submenu->dispatch_motion_event();
	}

	if( !result && menu_popup->cursor_inside() ) {
		int cursor_x, cursor_y;
		menu_popup->get_popup()->get_relative_cursor(cursor_x, cursor_y);
		if( cursor_x >= 0 && cursor_x < menu_popup->get_w() &&
			cursor_y >= y && cursor_y < y + h) {
// Highlight the item
			if(!highlighted)
			{
// Deactivate submenus in the parent menu excluding this one.
				menu_popup->deactivate_submenus(submenu);
				highlighted = 1;
				if(submenu) activate_submenu();
				redraw = 1;
			}
			result = 1;
		}
		else
		if(highlighted)
		{
			highlighted = 0;
			result = 1;
			redraw = 1;
		}
	}
	return result;
}

int BC_MenuItem::dispatch_translation_event()
{
	if(submenu)
		submenu->dispatch_translation_event();
	return 0;
}

int BC_MenuItem::dispatch_cursor_leave()
{
	int result = 0;
	if(submenu)
	{
		result = submenu->dispatch_cursor_leave();
	}

	if(!result && highlighted && menu_popup->get_popup()->is_event_win())
	{
		highlighted = 0;
		return 1;
	}
	return 0;
}

int BC_MenuItem::dispatch_key_press()
{
	int result = 0;
	if(submenu)
	{
		result = submenu->dispatch_key_press();
	}

	if(!result)
	{

		if(top_level->get_keypress() == hotkey &&
			shift_hotkey == top_level->shift_down() &&
			alt_hotkey == top_level->alt_down() &&
			ctrl_hotkey == top_level->ctrl_down())
		{
			result = 1;
			handle_event();
		}
	}
	return result;
}


int BC_MenuItem::draw()
{
	int text_line = top_level->get_text_descent(MEDIUMFONT);
	BC_Resources *resources = top_level->get_resources();

	if(!strcmp(text, "-")) {
		int bx = xS(5), by = y+h/2, bw = menu_popup->get_w()-xS(10);
		draw_bar(bx, by, bw);
	}
	else if( text[0] == '-' && text[strlen(text)-1] == '-' ) {
		draw_title_bar();
	}
	else {
		int xoffset = 0, yoffset = 0;
		if(highlighted)
		{
			int y = this->y;
			//int w = menu_popup->get_w() - 4;
			int h = this->h;

// Button down
			if(top_level->get_button_down() && !submenu)
			{
				if(menu_popup->item_bg[MENUITEM_DN])
				{
// 					menu_popup->get_popup()->draw_9segment(MENUITEM_MARGIN,
// 						y,
// 						menu_popup->get_w() - MENUITEM_MARGIN * 2,
// 						h,
// 						menu_popup->item_bg[MENUITEM_DN]);
					menu_popup->get_popup()->draw_3segmenth(MENUITEM_MARGIN,
						y,
						menu_popup->get_w() - MENUITEM_MARGIN * 2,
						menu_popup->item_bg[MENUITEM_DN]);
				}
				else
				{
					menu_popup->get_popup()->draw_3d_box(MENUITEM_MARGIN,
						y,
						menu_popup->get_w() - MENUITEM_MARGIN * 2,
						h,
						resources->menu_shadow,
						BLACK,
						resources->menu_down,
						resources->menu_down,
						resources->menu_light);
				}
				xoffset = xS(1);  yoffset = yS(1);
			}
			else
// Highlighted
			{
				if(menu_popup->item_bg[MENUITEM_HI])
				{
// 					menu_popup->get_popup()->draw_9segment(MENUITEM_MARGIN,
// 						y,
// 						menu_popup->get_w() - MENUITEM_MARGIN * 2,
// 						h,
// 						menu_popup->item_bg[MENUITEM_HI]);
					menu_popup->get_popup()->draw_3segmenth(MENUITEM_MARGIN,
						y,
						menu_popup->get_w() - MENUITEM_MARGIN * 2,
						menu_popup->item_bg[MENUITEM_HI]);
				}
				else
				{
					menu_popup->get_popup()->set_color(resources->menu_highlighted);
					menu_popup->get_popup()->draw_box(MENUITEM_MARGIN,
						y,
						menu_popup->get_w() - MENUITEM_MARGIN * 2,
						h);
				}
			}
			menu_popup->get_popup()->set_color(resources->menu_highlighted_fontcolor);
		}
		else
		  {
		menu_popup->get_popup()->set_color(resources->menu_item_text);
		  }
		if(checked)
		{
//			menu_popup->get_popup()->draw_check(xS(10) + xoffset, y + 2 + yoffset);
			menu_popup->get_popup()->draw_pixmap(menu_popup->check,
				xoffset,
				y + (this->h - menu_popup->check->get_h()) / 2 + yoffset);
			menu_popup->get_popup()->set_font(MEDIUMFONT);
			menu_popup->get_popup()->draw_text(menu_popup->check->get_w() + xoffset,
				y + h - text_line - 2 + yoffset,
				text);
			menu_popup->get_popup()->draw_text(menu_popup->get_key_x() + xoffset,
				y + h - text_line - 2 + yoffset,
				hotkey_text);
		}
		else
		{
			menu_popup->get_popup()->set_font(MEDIUMFONT);
			menu_popup->get_popup()->draw_text(xS(10) + xoffset, y + h - text_line - 2 + yoffset, text);
			menu_popup->get_popup()->draw_text(menu_popup->get_key_x() + xoffset, y + h - text_line - 2 + yoffset, hotkey_text);
		}
	}
	return 0;
}

void BC_MenuItem::draw_bar(int bx, int by, int bw)
{
	BC_Popup *popup = menu_popup->get_popup();
	popup->set_color(DKGREY);
	popup->draw_line(bx, by, bx+bw, by);
	popup->set_color(LTGREY);  ++by;
	popup->draw_line(bx, by, bx+bw, by);
}

void BC_MenuItem::draw_title_bar()
{
	BC_Popup *popup = menu_popup->get_popup();
	int len = strlen(text)-2;
	if( len <= 0 ) return;
	int tw = popup->get_text_width(MEDIUMFONT, text+1, len);
	int th = popup->get_text_ascent(MEDIUMFONT);
	int mw = menu_popup->get_w(), lw = mw - tw;
	int x1 = xS(5), y1 = y+h/2;
	int tx = lw/4, ty = y1 + th/2;
	int w1 = tx - x1 - xS(5);
	if( w1 > 0 ) draw_bar(x1, y1, w1);
	BC_Resources *resources = top_level->get_resources();
	popup->set_color(resources->text_background_hi);
	popup->draw_text(tx, ty, text+1, len);
	int x2 = tx + tw + xS(5), w2 = mw - xS(5) - x2;
	if( w2 > 0 ) draw_bar(x2, y1, w2);
}

int BC_MenuItem::add_submenu(BC_SubMenu *submenu)
{
	this->submenu = submenu;
	submenu->initialize(top_level, menu_bar, 0, this, 0);
	return 0;
}

BC_SubMenu* BC_MenuItem::get_submenu()
{
	return submenu;
}

char* BC_MenuItem::get_text()
{
	return text;
}

BC_WindowBase* BC_MenuItem::get_top_level()
{
	return top_level;
}

BC_PopupMenu* BC_MenuItem::get_popup_menu()
{
	return menu_popup->popup_menu;
}

int BC_MenuItem::set_shift(int value)
{
	shift_hotkey = value;
	return 0;
}

int BC_MenuItem::set_alt(int value)
{
	alt_hotkey = value;
	return 0;
}

void BC_MenuItem::set_ctrl(int value)
{
	ctrl_hotkey = value;
}

