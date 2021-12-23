
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

/*
 * 2019. Derivative by Translate plugin. This plugin works also with Proxy.
 * It uses Percent values instead of Pixel value coordinates.
*/

#include "bcdisplayinfo.h"
#include "clip.h"
#include "language.h"
#include "theme.h"
#include "crop.h"
#include "cropwin.h"












CropWin::CropWin(CropMain *client)
 : PluginClientWindow(client,
	xS(420),
	yS(290),
	xS(420),
	yS(290),
	0)
{
	this->client = client;
}

CropWin::~CropWin()
{
}

void CropWin::create_objects()
{
	int xs10 = xS(10), xs20 = xS(20), xs200 = xS(200);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30), ys40 = yS(40);
	int x2 = xS(80), x3 = xS(180);
	int x = xs10, y = ys10;
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_TitleBar *title_bar;
	BC_Bar *bar;

// Crop section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Crop")));
	y += ys20;
	add_tool(new BC_Title(x, y, _("Left")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_left_text = new CropLeftText(this, client, (x + x2), y);
	crop_left_text->create_objects();
	crop_left_slider = new CropLeftSlider(this, client, x3, y, xs200);
	add_subwindow(crop_left_slider);
	clr_x = x3 + crop_left_slider->get_w() + x;
	add_subwindow(crop_left_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_LEFT));
	y += ys30;
	add_tool(new BC_Title(x, y, _("Top")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_top_text = new CropTopText(this, client, (x + x2), y);
	crop_top_text->create_objects();
	crop_top_slider = new CropTopSlider(this, client, x3, y, xs200);
	add_subwindow(crop_top_slider);
	add_subwindow(crop_top_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_TOP));
	y += ys30;
	add_tool(new BC_Title(x, y, _("Right")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_right_text = new CropRightText(this, client, (x + x2), y);
	crop_right_text->create_objects();
	crop_right_slider = new CropRightSlider(this, client, x3, y, xs200);
	add_subwindow(crop_right_slider);
	add_subwindow(crop_right_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_RIGHT));
	y += ys30;
	add_tool(new BC_Title(x, y, _("Bottom")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_bottom_text = new CropBottomText(this, client, (x + x2), y);
	crop_bottom_text->create_objects();
	crop_bottom_slider = new CropBottomSlider(this, client,	x3, y, xs200);
	add_subwindow(crop_bottom_slider);
	add_subwindow(crop_bottom_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_BOTTOM));
	y += ys40;

// Position section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Position")));
	y += ys20;
	add_tool(new BC_Title(x, y, _("X")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_position_x_text = new CropPositionXText(this, client, (x + x2), y);
	crop_position_x_text->create_objects();
	crop_position_x_slider = new CropPositionXSlider(this, client, x3, y, xs200);
	add_subwindow(crop_position_x_slider);
	add_subwindow(crop_position_x_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_POSITION_X));
	y += ys30;
	add_tool(new BC_Title(x, y, _("Y")));
	add_tool(new BC_Title((x2-x), y, _("%")));
	crop_position_y_text = new CropPositionYText(this, client, (x + x2), y);
	crop_position_y_text->create_objects();
	crop_position_y_slider = new CropPositionYSlider(this, client, x3, y, xs200);
	add_subwindow(crop_position_y_slider);
	add_subwindow(crop_position_y_clr = new CropEdgesClr(this, client,
		clr_x, y, RESET_POSITION_Y));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_tool(reset = new CropReset(client, this, x, y));

	show_window();
	flush();
}

void CropWin::update(int clear)
{
	switch(clear) {
		case RESET_LEFT :
			crop_left_text->update((float)client->config.crop_l);
			crop_left_slider->update((float)client->config.crop_l);
			break;
		case RESET_TOP :
			crop_top_text->update((float)client->config.crop_t);
			crop_top_slider->update((float)client->config.crop_t);
			break;
		case RESET_RIGHT :
			crop_right_text->update((float)client->config.crop_r);
			crop_right_slider->update((float)client->config.crop_r);
			break;
		case RESET_BOTTOM :
			crop_bottom_text->update((float)client->config.crop_b);
			crop_bottom_slider->update((float)client->config.crop_b);
			break;
		case RESET_POSITION_X :
			crop_position_x_text->update((float)client->config.position_x);
			crop_position_x_slider->update((float)client->config.position_x);
			break;
		case RESET_POSITION_Y :
			crop_position_y_text->update((float)client->config.position_y);
			crop_position_y_slider->update((float)client->config.position_y);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			crop_left_text->update((float)client->config.crop_l);
			crop_left_slider->update((float)client->config.crop_l);
			crop_top_text->update((float)client->config.crop_t);
			crop_top_slider->update((float)client->config.crop_t);
			crop_right_text->update((float)client->config.crop_r);
			crop_right_slider->update((float)client->config.crop_r);
			crop_bottom_text->update((float)client->config.crop_b);
			crop_bottom_slider->update((float)client->config.crop_b);

			crop_position_x_text->update((float)client->config.position_x);
			crop_position_x_slider->update((float)client->config.position_x);
			crop_position_y_text->update((float)client->config.position_y);
			crop_position_y_slider->update((float)client->config.position_y);
			break;
	}
}







/* *********************************** */
/* **** CROP LEFT ******************** */
CropLeftText::CropLeftText(CropWin *win,
	CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.crop_l,
	(float)0.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropLeftText::~CropLeftText()
{
}

int CropLeftText::handle_event()
{
	client->config.crop_l = atof(get_text());
	win->crop_left_slider->update(client->config.crop_l);
	client->send_configure_change();
	return 1;
}

CropLeftSlider::CropLeftSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0.00, 100.00, client->config.crop_l)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropLeftSlider::~CropLeftSlider()
{
}

int CropLeftSlider::handle_event()
{
	client->config.crop_l = get_value();
	win->crop_left_text->update(client->config.crop_l);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */

/* *********************************** */
/* **** CROP TOP  ******************** */
CropTopText::CropTopText(CropWin *win,
	CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.crop_t,
	(float)0.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropTopText::~CropTopText()
{
}

int CropTopText::handle_event()
{
	client->config.crop_t = atof(get_text());
	win->crop_top_slider->update(client->config.crop_t);
	client->send_configure_change();
	return 1;
}

CropTopSlider::CropTopSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0.00, 100.00, client->config.crop_t)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropTopSlider::~CropTopSlider()
{
}

int CropTopSlider::handle_event()
{
	client->config.crop_t = get_value();
	win->crop_top_text->update(client->config.crop_t);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */

/* *********************************** */
/* **** CROP RIGHT ******************* */
CropRightText::CropRightText(CropWin *win, CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.crop_r,
	(float)0.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropRightText::~CropRightText()
{
}

int CropRightText::handle_event()
{
	client->config.crop_r = atof(get_text());
	win->crop_right_slider->update(client->config.crop_r);
	client->send_configure_change();
	return 1;
}

CropRightSlider::CropRightSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0.00, 100.00, client->config.crop_r)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropRightSlider::~CropRightSlider()
{
}

int CropRightSlider::handle_event()
{
	client->config.crop_r = get_value();
	win->crop_right_text->update(client->config.crop_r);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */

/* *********************************** */
/* **** CROP BOTTOM ****************** */
CropBottomText::CropBottomText(CropWin *win, CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.crop_b,
	(float)0.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropBottomText::~CropBottomText()
{
}

int CropBottomText::handle_event()
{
	client->config.crop_b = atof(get_text());
	win->crop_bottom_slider->update(client->config.crop_b);
	client->send_configure_change();
	return 1;
}

CropBottomSlider::CropBottomSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0.00, 100.00, client->config.crop_b)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropBottomSlider::~CropBottomSlider()
{
}

int CropBottomSlider::handle_event()
{
	client->config.crop_b = get_value();
	win->crop_bottom_text->update(client->config.crop_b);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */

/* *********************************** */
/* **** CROP POSITION X ************** */
CropPositionXText::CropPositionXText(CropWin *win, CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.position_x,
	(float)-100.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropPositionXText::~CropPositionXText()
{
}

int CropPositionXText::handle_event()
{
	client->config.position_x = atof(get_text());
	win->crop_position_x_slider->update(client->config.position_x);
	client->send_configure_change();
	return 1;
}

CropPositionXSlider::CropPositionXSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, -100.00, 100.00, client->config.position_x)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropPositionXSlider::~CropPositionXSlider()
{
}

int CropPositionXSlider::handle_event()
{
	client->config.position_x = get_value();
	win->crop_position_x_text->update(client->config.position_x);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */

/* *********************************** */
/* **** CROP POSITION Y ************** */
CropPositionYText::CropPositionYText(CropWin *win, CropMain *client,
	int x,
	int y)
 : BC_TumbleTextBox(win, client->config.position_y,
	(float)-100.00, (float)100.00, x, y, xS(60), 2)
{
	this->win = win;
	this->client = client;
}

CropPositionYText::~CropPositionYText()
{
}

int CropPositionYText::handle_event()
{
	client->config.position_y = atof(get_text());
	win->crop_position_y_slider->update(client->config.position_y);
	client->send_configure_change();
	return 1;
}

CropPositionYSlider::CropPositionYSlider(CropWin *win, CropMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, -100.00, 100.00, client->config.position_y)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

CropPositionYSlider::~CropPositionYSlider()
{
}

int CropPositionYSlider::handle_event()
{
	client->config.position_y = get_value();
	win->crop_position_y_text->update(client->config.position_y);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
/* *********************************** */


CropEdgesClr::CropEdgesClr(CropWin *win, CropMain *client, int x, int y, int clear)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->win = win;
	this->client = client;
	this->clear = clear;
}
CropEdgesClr::~CropEdgesClr()
{
}
int CropEdgesClr::handle_event()
{
	client->config.reset(clear);
	win->update(clear);
	client->send_configure_change();
	return 1;
}

CropReset::CropReset(CropMain *client, CropWin *win, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->win = win;
}

CropReset::~CropReset()
{
}

int CropReset::handle_event()
{
	client->config.reset(RESET_ALL);
	win->update(RESET_ALL);
	client->send_configure_change();
	return 1;
}
