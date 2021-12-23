
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
#include "clip.h"
#include "language.h"
#include "theme.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "scale.h"



ScaleWin::ScaleWin(ScaleMain *client)
 : PluginClientWindow(client, xS(420), yS(260), xS(420), yS(260), 0)
{
	this->client = client;
}

ScaleWin::~ScaleWin()
{
	delete x_factor_text;
	delete x_factor_slider;
	delete x_factor_clr;
	delete y_factor_text;
	delete y_factor_slider;
	delete y_factor_clr;
	delete width_text;
	delete width_slider;
	delete width_clr;
	delete height_text;
	delete height_slider;
	delete height_clr;
}

void ScaleWin::create_objects()
{
	int xs10 = xS(10), xs20 = xS(20), xs200 = xS(200);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30), ys40 = yS(40);
	int x2 = xS(60), x3 = xS(180);
	int x = xs10, y = ys10;
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_TitleBar *title_bar;
	BC_Bar *bar;


// Scale section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Scale")));
	y += ys20;
	add_tool(use_scale = new ScaleUseScale(this, client, x, y));
	int xa= x*2 + use_scale->get_w();
	add_tool(new BC_Title(xa, y, _("X:")));
	x_factor_text = new ScaleXFactorText(this, client, (x + x2), y);
	x_factor_text->create_objects();
	x_factor_slider = new ScaleXFactorSlider(this, client, x3, y, xs200);
	add_subwindow(x_factor_slider);
	clr_x = x3 + x_factor_slider->get_w() + x;
	add_subwindow(x_factor_clr = new ScaleClr(this, client, clr_x, y, RESET_X_FACTOR));
	y += ys30;
	add_tool(new BC_Title(xa, y, _("Y:")));
	y_factor_text = new ScaleYFactorText(this, client, (x + x2), y);
	y_factor_text->create_objects();
	y_factor_slider = new ScaleYFactorSlider(this, client, x3, y, xs200);
	add_subwindow(y_factor_slider);
	add_subwindow(y_factor_clr = new ScaleClr(this, client, clr_x, y, RESET_Y_FACTOR));
	y += ys30;
	add_tool(constrain = new ScaleConstrain(this, client, (x + x2), y));
	y += ys40;

// Size section
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xs20, xs10, _("Size")));
	y += ys20;
	add_tool(use_size = new ScaleUseSize(this, client, x, y));
	add_tool(new BC_Title(xa, y, _("W:")));
	width_text = new ScaleWidthText(this, client, (x + x2), y);
	width_text->create_objects();
	width_slider = new ScaleWidthSlider(this, client, x3, y, xs200);
	add_subwindow(width_slider);
	add_subwindow(width_clr = new ScaleClr(this, client, clr_x, y, RESET_WIDTH));
	int ya = y;
	y += ys30;
	add_tool(new BC_Title(xa, y, _("H:")));
	height_text = new ScaleHeightText(this, client, (x + x2), y);
	height_text->create_objects();
	height_slider = new ScaleHeightSlider(this, client, x3, y, xs200);
	add_subwindow(height_slider);
	add_subwindow(height_clr = new ScaleClr(this, client, clr_x, y, RESET_HEIGHT));

	int x4 = x + x2 + height_text->get_w();
	add_tool(pulldown = new FrameSizePulldown(client->server->mwindow->theme,
			width_text->get_textbox(), height_text->get_textbox(), x4, ya));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_tool(reset = new ScaleReset(this, client, x, y));

	show_window();
	flush();

	// Needed to update Enable-Disable GUI when "Show controls" is pressed.
	update_scale_size_enable();
}


void ScaleWin::update_scale_size_enable()
{
	if(use_scale->get_value()==1) 
	{
		width_text->disable();
		width_slider->disable();
		width_clr->disable();
		height_text->disable();
		height_slider->disable();
		height_clr->disable();
		pulldown->hide_window();

		x_factor_text->enable();
		x_factor_slider->enable();
		x_factor_clr->enable();
		if(client->config.constrain)
		{
			y_factor_text->disable();
			y_factor_slider->disable();
			y_factor_clr->disable();
		}
		else
		{
			y_factor_text->enable();
			y_factor_slider->enable();
			y_factor_clr->enable();
		}
		constrain->enable();
	}

	if(use_size->get_value()==1)
	{
		x_factor_text->disable();
		x_factor_slider->disable();
		x_factor_clr->disable();
		y_factor_text->disable();
		y_factor_slider->disable();
		y_factor_clr->disable();
		constrain->disable();

		width_text->enable();
		width_slider->enable();
		width_clr->enable();
		height_text->enable();
		height_slider->enable();
		height_clr->enable();
		pulldown->show_window();
	}
}


void ScaleWin::update(int clear)
{
	switch(clear) {
		case RESET_X_FACTOR :
			x_factor_text->update((float)client->config.x_factor);
			x_factor_slider->update((float)client->config.x_factor);
			break;
		case RESET_Y_FACTOR :
			y_factor_text->update((float)client->config.y_factor);
			y_factor_slider->update((float)client->config.y_factor);
			break;
		case RESET_WIDTH :
			width_text->update((int64_t)client->config.width);
			width_slider->update((int64_t)client->config.width);
			break;
		case RESET_HEIGHT :
			height_text->update((int64_t)client->config.height);
			height_slider->update((int64_t)client->config.height);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			x_factor_text->update((float)client->config.x_factor);
			x_factor_slider->update((float)client->config.x_factor);
			y_factor_text->update((float)client->config.y_factor);
			y_factor_slider->update((float)client->config.y_factor);
			constrain->update(client->config.constrain);

			width_text->update((int64_t)client->config.width);
			width_slider->update((int64_t)client->config.width);
			height_text->update((int64_t)client->config.height);
			height_slider->update((int64_t)client->config.height);

			use_scale->update((int)!client->config.type);
			use_size->update((int)client->config.type);
			break;
	}
}





ScaleUseScale::ScaleUseScale(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_Radial(x, y, client->config.type == FIXED_SCALE, "")
{
        this->win = win;
        this->client = client;
	set_tooltip(_("Use fixed scale"));
}
ScaleUseScale::~ScaleUseScale()
{
}
int ScaleUseScale::handle_event()
{
	client->set_type(FIXED_SCALE);
	win->update_scale_size_enable();
	return 1;
}

ScaleUseSize::ScaleUseSize(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_Radial(x, y, client->config.type == FIXED_SIZE, "")
{
        this->win = win;
        this->client = client;
	set_tooltip(_("Use fixed size"));
}
ScaleUseSize::~ScaleUseSize()
{
}
int ScaleUseSize::handle_event()
{
	client->set_type(FIXED_SIZE);
	win->update_scale_size_enable();
	return 1;
}



ScaleConstrain::ScaleConstrain(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.constrain, _("Constrain ratio"))
{
        this->win = win;
	this->client = client;
}
ScaleConstrain::~ScaleConstrain()
{
}
int ScaleConstrain::handle_event()
{
	client->config.constrain = get_value();

	if(client->config.constrain)
	{
		win->y_factor_text->disable();
		win->y_factor_slider->disable();
		win->y_factor_clr->disable();

		client->config.y_factor = client->config.x_factor;
		win->y_factor_text->update(client->config.y_factor);
		win->y_factor_slider->update(client->config.y_factor);
	}
	else
	{
		win->y_factor_text->enable();
		win->y_factor_slider->enable();
		win->y_factor_clr->enable();
	}
	client->send_configure_change();
	return 1;
}



/* *********************************** */
/* **** SCALE X FACTOR  ************** */
ScaleXFactorText::ScaleXFactorText(ScaleWin *win, ScaleMain *client,
	int x, int y)
 : BC_TumbleTextBox(win, (float)client->config.x_factor,
	MIN_FACTOR, MAX_FACTOR, x, y, xS(60), 2)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
	enabled = 1;
}

ScaleXFactorText::~ScaleXFactorText()
{
}

int ScaleXFactorText::handle_event()
{
	client->config.x_factor = atof(get_text());
	CLAMP(client->config.x_factor, MIN_FACTOR, MAX_FACTOR);

	if(client->config.constrain)
	{
		client->config.y_factor = client->config.x_factor;
		win->y_factor_text->update(client->config.y_factor);
		win->y_factor_slider->update(client->config.y_factor);
	}
	win->x_factor_slider->update(client->config.x_factor);
	client->send_configure_change();
	return 1;
}

ScaleXFactorSlider::ScaleXFactorSlider(ScaleWin *win, ScaleMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, MIN_FACTOR, MAX_FACTOR, (float)client->config.x_factor)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

ScaleXFactorSlider::~ScaleXFactorSlider()
{
}

int ScaleXFactorSlider::handle_event()
{
	client->config.x_factor = get_value();
	if(client->config.constrain)
	{
		client->config.y_factor = client->config.x_factor;
		win->y_factor_text->update(client->config.y_factor);
		win->y_factor_slider->update(client->config.y_factor);
	}
	win->x_factor_text->update(client->config.x_factor);
	client->send_configure_change();
	return 1;
}
/* *********************************** */


/* *********************************** */
/* **** SCALE Y FACTOR  ************** */
ScaleYFactorText::ScaleYFactorText(ScaleWin *win, ScaleMain *client,
	int x, int y)
 : BC_TumbleTextBox(win, (float)client->config.y_factor,
	MIN_FACTOR, MAX_FACTOR, x, y, xS(60), 2)
{
	this->client = client;
	this->win = win;
	set_increment(0.1);
	enabled = 1;
}

ScaleYFactorText::~ScaleYFactorText()
{
}

int ScaleYFactorText::handle_event()
{
	client->config.y_factor = atof(get_text());
	CLAMP(client->config.y_factor, MIN_FACTOR, MAX_FACTOR);

	if(client->config.constrain)
	{
		client->config.x_factor = client->config.y_factor;
		win->x_factor_text->update(client->config.x_factor);
		win->x_factor_slider->update(client->config.x_factor);
	}
	win->y_factor_slider->update(client->config.y_factor);
	client->send_configure_change();
	return 1;
}

ScaleYFactorSlider::ScaleYFactorSlider(ScaleWin *win, ScaleMain *client,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, MIN_FACTOR, MAX_FACTOR, (float)client->config.y_factor)
{
	this->win = win;
	this->client = client;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

ScaleYFactorSlider::~ScaleYFactorSlider()
{
}

int ScaleYFactorSlider::handle_event()
{
	client->config.y_factor = get_value();
	if(client->config.constrain)
	{
		client->config.x_factor = client->config.y_factor;
		win->x_factor_text->update(client->config.x_factor);
		win->x_factor_slider->update(client->config.x_factor);
	}
	win->y_factor_text->update(client->config.y_factor);
	client->send_configure_change();
	return 1;
}
/* *********************************** */


/* *********************************** */
/* **** SCALE WIDTH     ************** */
ScaleWidthText::ScaleWidthText(ScaleWin *win, ScaleMain *client,
	int x, int y)
 : BC_TumbleTextBox(win, client->config.width,
	0, MAX_WIDTH, x, y, xS(60))
{
	this->client = client;
	this->win = win;
	set_increment(10);
	enabled = 1;
}

ScaleWidthText::~ScaleWidthText()
{
}

int ScaleWidthText::handle_event()
{
	client->config.width = atoi(get_text());
	win->width_slider->update(client->config.width);
	client->send_configure_change();
	return 1;
}

ScaleWidthSlider::ScaleWidthSlider(ScaleWin *win, ScaleMain *client,
	int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, 0, MAX_WIDTH, client->config.width)
{
	this->client = client;
	this->win = win;
	enable_show_value(0); // Hide caption
}

ScaleWidthSlider::~ScaleWidthSlider()
{
}

int ScaleWidthSlider::handle_event()
{
	client->config.width = get_value();
	win->width_text->update((int64_t)client->config.width);
	client->send_configure_change();
	return 1;
}
/* *********************************** */


/* *********************************** */
/* **** SCALE HEIGHT    ************** */
ScaleHeightText::ScaleHeightText(ScaleWin *win, ScaleMain *client,
	int x, int y)
 : BC_TumbleTextBox(win, client->config.height,
	0, MAX_HEIGHT, x, y, xS(60))
{
	this->client = client;
	this->win = win;
	set_increment(10);
	enabled = 1;
}

ScaleHeightText::~ScaleHeightText()
{
}

int ScaleHeightText::handle_event()
{
	client->config.height = atoi(get_text());
	win->height_slider->update(client->config.height);
	client->send_configure_change();
	return 1;
}

ScaleHeightSlider::ScaleHeightSlider(ScaleWin *win, ScaleMain *client,
	int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, 0, MAX_HEIGHT, client->config.height)
{
	this->client = client;
	this->win = win;
	enable_show_value(0); // Hide caption
}

ScaleHeightSlider::~ScaleHeightSlider()
{
}

int ScaleHeightSlider::handle_event()
{
	client->config.height = get_value();
	win->height_text->update((int64_t)client->config.height);
	client->send_configure_change();
	return 1;
}
/* *********************************** */


ScaleClr::ScaleClr(ScaleWin *win, ScaleMain *client, int x, int y, int clear)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->win = win;
	this->client = client;
	this->clear = clear;
}
ScaleClr::~ScaleClr()
{
}
int ScaleClr::handle_event()
{
	client->config.reset(clear);
	win->update(clear);
	if( client->config.constrain && win->use_scale->get_value() )
	{
		client->config.reset(RESET_X_FACTOR);
		win->update(RESET_X_FACTOR);

		client->config.reset(RESET_Y_FACTOR);
		win->update(RESET_Y_FACTOR);
	}
	client->send_configure_change();
	return 1;
}

ScaleReset::ScaleReset(ScaleWin *win, ScaleMain *client, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->win = win;
	this->client = client;
}

ScaleReset::~ScaleReset()
{
}

int ScaleReset::handle_event()
{
	client->config.reset(RESET_ALL);
	win->update(RESET_ALL);
	win->update_scale_size_enable();
	client->send_configure_change();
	return 1;
}
