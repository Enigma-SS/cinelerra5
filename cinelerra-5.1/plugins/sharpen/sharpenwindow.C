
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
#include "sharpenwindow.h"










SharpenWindow::SharpenWindow(SharpenMain *client)
 : PluginClientWindow(client, xS(420), yS(190), xS(420), yS(190), 0)
{
	this->client = client;
}

SharpenWindow::~SharpenWindow()
{
}

void SharpenWindow::create_objects()
{
	int xs10 = xS(10), xs100 = xS(100), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22
	int defaultBtn_w = xs100;

	BC_Bar *bar;

	y += ys10;
	add_tool(new BC_Title(x, y, _("Sharpness:")));
	sharpen_text = new SharpenText(client, this, (x + x2), y);
	sharpen_text->create_objects();
	add_tool(sharpen_slider = new SharpenSlider(client, this, x3, y, xs200));
	clr_x = x3 + sharpen_slider->get_w() + x;
	add_subwindow(sharpen_Clr = new SharpenClr(client, this, clr_x, y));

	y += ys30;
	add_tool(sharpen_interlace = new SharpenInterlace(client, x, y));
	y += ys30;
	add_tool(sharpen_horizontal = new SharpenHorizontal(client, x, y));
	y += ys30;
	add_tool(sharpen_luminance = new SharpenLuminance(client, x, y));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_tool(reset = new SharpenReset(client, this, x, y));
	add_subwindow(default_settings = new SharpenDefaultSettings(client, this,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}

void SharpenWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_SHARPEN :
			sharpen_text->update((int64_t)client->config.sharpness);
			sharpen_slider->update(client->config.sharpness);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			sharpen_text->update((int64_t)client->config.sharpness);
			sharpen_slider->update(client->config.sharpness);
			sharpen_interlace->update(client->config.interlace);
			sharpen_horizontal->update(client->config.horizontal);
			sharpen_luminance->update(client->config.luminance);
			break;
	}
}

SharpenText::SharpenText(SharpenMain *client, SharpenWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, client->config.sharpness,
	0, MAXSHARPNESS, x, y, xS(60), 0)
{
	this->client = client;
	this->gui = gui;
	set_increment(1);
}

SharpenText::~SharpenText()
{
}

int SharpenText::handle_event()
{
	client->config.sharpness = atoi(get_text());
	if(client->config.sharpness > MAXSHARPNESS)
		client->config.sharpness = MAXSHARPNESS;
	else
		if(client->config.sharpness < 0) client->config.sharpness = 0;

	gui->sharpen_slider->update((int64_t)client->config.sharpness);
	client->send_configure_change();
	return 1;
}

SharpenSlider::SharpenSlider(SharpenMain *client, SharpenWindow *gui, int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, 0, MAXSHARPNESS, client->config.sharpness, 0, 0, 0)
{
	this->client = client;
	this->gui = gui;
	enable_show_value(0); // Hide caption
}
SharpenSlider::~SharpenSlider()
{
}
int SharpenSlider::handle_event()
{
	client->config.sharpness = get_value();
	gui->sharpen_text->update(client->config.sharpness);
	client->send_configure_change();
	return 1;
}


SharpenInterlace::SharpenInterlace(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.interlace, _("Interlace"))
{
	this->client = client;
}
SharpenInterlace::~SharpenInterlace()
{
}
int SharpenInterlace::handle_event()
{
	client->config.interlace = get_value();
	client->send_configure_change();
	return 1;
}


SharpenHorizontal::SharpenHorizontal(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.horizontal, _("Horizontal only"))
{
	this->client = client;
}
SharpenHorizontal::~SharpenHorizontal()
{
}
int SharpenHorizontal::handle_event()
{
	client->config.horizontal = get_value();
	client->send_configure_change();
	return 1;
}


SharpenLuminance::SharpenLuminance(SharpenMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.luminance, _("Luminance only"))
{
	this->client = client;
}
SharpenLuminance::~SharpenLuminance()
{
}
int SharpenLuminance::handle_event()
{
	client->config.luminance = get_value();
	client->send_configure_change();
	return 1;
}


SharpenReset::SharpenReset(SharpenMain *client, SharpenWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->gui = gui;
}
SharpenReset::~SharpenReset()
{
}
int SharpenReset::handle_event()
{
	client->config.reset(RESET_ALL);
	gui->update_gui(RESET_ALL);
	client->send_configure_change();
	return 1;
}


SharpenDefaultSettings::SharpenDefaultSettings(SharpenMain *client, SharpenWindow *gui, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->client = client;
	this->gui = gui;
}
SharpenDefaultSettings::~SharpenDefaultSettings()
{
}
int SharpenDefaultSettings::handle_event()
{
	client->config.reset(RESET_DEFAULT_SETTINGS);
	gui->update_gui(RESET_DEFAULT_SETTINGS);
	client->send_configure_change();
	return 1;
}


SharpenClr::SharpenClr(SharpenMain *client, SharpenWindow *gui, int x, int y)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->gui = gui;
}
SharpenClr::~SharpenClr()
{
}
int SharpenClr::handle_event()
{
	client->config.reset(RESET_SHARPEN);
	gui->update_gui(RESET_SHARPEN);
	client->send_configure_change();
	return 1;
}

