
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

#include "bcdisplayinfo.h"
#include "language.h"
#include "colorspacewindow.h"


const char *ColorSpaceSpace::color_space[] = {
	N_("BT601"), // COLOR_SPACE_BT601
	N_("BT709"), // COLOR_SPACE_BT709
	N_("BT2020"), // COLOR_SPACE_BT2020
};

ColorSpaceSpace::ColorSpaceSpace(ColorSpaceWindow *gui, int x, int y, int *value)
 : BC_PopupMenu(x, y, xS(120), _(color_space[*value]), 1)
{
	this->gui = gui;
	this->value = value;
}
ColorSpaceSpace::~ColorSpaceSpace()
{
}

void ColorSpaceSpace::create_objects()
{
	for( int id=0,nid=sizeof(color_space)/sizeof(color_space[0]); id<nid; ++id )
		add_item(new ColorSpaceSpaceItem(this, _(color_space[id]), id));
	update();
}

void ColorSpaceSpace::update()
{
	set_text(_(color_space[*value]));
}

int ColorSpaceSpace::handle_event()
{
	update();
	gui->plugin->send_configure_change();
	return 1;
}

ColorSpaceSpaceItem::ColorSpaceSpaceItem(ColorSpaceSpace *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int ColorSpaceSpaceItem::handle_event()
{
	*popup->value = id;
	return popup->handle_event();
}


const char *ColorSpaceRange::color_range[] = {
	N_("JPEG"), // COLOR_RANGE_JPEG
	N_("MPEG"), // COLOR_RANGE_MPEG
};

ColorSpaceRange::ColorSpaceRange(ColorSpaceWindow *gui, int x, int y, int *value)
 : BC_PopupMenu(x, y, xS(100), _(color_range[*value]), 1)
{
	this->gui = gui;
	this->value = value;
}
ColorSpaceRange::~ColorSpaceRange()
{
}

void ColorSpaceRange::create_objects()
{
	for( int id=0,nid=sizeof(color_range)/sizeof(color_range[0]); id<nid; ++id )
		add_item(new ColorSpaceRangeItem(this, _(color_range[id]), id));
	update();
}

void ColorSpaceRange::update()
{
	set_text(color_range[*value]);
}

int ColorSpaceRange::handle_event()
{
	update();
	gui->plugin->send_configure_change();
	return 1;
}

ColorSpaceRangeItem::ColorSpaceRangeItem(ColorSpaceRange *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int ColorSpaceRangeItem::handle_event()
{
	*popup->value = id;
	return popup->handle_event();
}

ColorSpaceInverse::ColorSpaceInverse(ColorSpaceWindow *gui, int x, int y, int *value)
 : BC_CheckBox(x, y, value, _("Inverse"))
{
	this->gui = gui;
}

ColorSpaceInverse::~ColorSpaceInverse()
{
}

void ColorSpaceInverse::update()
{
	set_value(*value, 1);
}

int ColorSpaceInverse::handle_event()
{
	*value = get_value();
	gui->plugin->send_configure_change();
	return 1;
}


ColorSpaceWindow::ColorSpaceWindow(ColorSpaceMain *plugin)
 : PluginClientWindow(plugin, xS(360), yS(130), xS(360), yS(130), 0)
{
	this->plugin = plugin;
}

ColorSpaceWindow::~ColorSpaceWindow()
{
}

void ColorSpaceWindow::create_objects()
{
	int ys10 = yS(10);
	int x = xS(10), y = ys10;
	int x1 = x + xS(80), x2 = x1 + xS(150);
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Color Space/Range conversion")));
	ColorSpaceConfig &config = plugin->config;
	add_subwindow(inverse = new ColorSpaceInverse(this, x2, y, &config.inverse));
	y += title->get_h() + yS(15);
	add_subwindow(title = new BC_Title(x1, y, _("Space")));
	add_subwindow(title = new BC_Title(x2, y, _("Range")));
	y += title->get_h() + ys10;
	add_subwindow(title = new BC_Title(x, y, _("Input:")));
	add_subwindow(inp_space = new ColorSpaceSpace(this, x1, y, &config.inp_colorspace));
	inp_space->create_objects();
	add_subwindow(inp_range = new ColorSpaceRange(this, x2, y, &config.inp_colorrange));
	inp_range->create_objects();
	y += title->get_h() + ys10;
	add_subwindow(title = new BC_Title(x, y, _("Output:")));
	add_subwindow(out_space = new ColorSpaceSpace(this, x1, y, &config.out_colorspace));
	out_space->create_objects();
	add_subwindow(out_range = new ColorSpaceRange(this, x2, y, &config.out_colorrange));
	out_range->create_objects();

	show_window();
	flush();
}

void ColorSpaceWindow::update()
{
	inverse->update();
	inp_space->update();
	inp_range->update();
	out_space->update();
	out_range->update();
}

