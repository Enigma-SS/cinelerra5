
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
#include "theme.h"
#include "unsharp.h"
#include "unsharpwindow.h"


UnsharpWindow::UnsharpWindow(UnsharpMain *plugin)
 : PluginClientWindow(plugin, xS(420), yS(160), xS(420), yS(160), 0)
{
	this->plugin = plugin;
}

UnsharpWindow::~UnsharpWindow()
{
}

void UnsharpWindow::create_objects()
{
	int xs10 = xS(10), xs100 = xS(100), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22
	int defaultBtn_w = xs100;

	BC_Title *title;
	BC_Bar *bar;

// Radius
	y += ys10;
	add_subwindow(title = new BC_Title(x, y, _("Radius:")));
	radius_text = new UnsharpRadiusText(this, plugin, (x + x2), y);
	radius_text->create_objects();
	radius_slider = new UnsharpRadiusSlider(this, plugin, x3, y, xs200);
	add_subwindow(radius_slider);
	clr_x = x3 + radius_slider->get_w() + x;
	add_subwindow(radius_clr = new UnsharpClr(this, plugin,
		clr_x, y, RESET_RADIUS));
	y += ys30;
// Amount
	add_subwindow(title = new BC_Title(x, y, _("Amount:")));
	amount_text = new UnsharpAmountText(this, plugin, (x + x2), y);
	amount_text->create_objects();
	amount_slider = new UnsharpAmountSlider(this, plugin, x3, y, xs200);
	add_subwindow(amount_slider);
	add_subwindow(amount_clr = new UnsharpClr(this, plugin,
		clr_x, y, RESET_AMOUNT));
	y += ys30;
// Threshold
	add_subwindow(title = new BC_Title(x, y, _("Threshold:")));
	threshold_text = new UnsharpThresholdText(this, plugin, (x + x2), y);
	threshold_text->create_objects();
	threshold_slider = new UnsharpThresholdSlider(this, plugin, x3, y, xs200);
	add_subwindow(threshold_slider);
	add_subwindow(threshold_clr = new UnsharpClr(this, plugin,
		clr_x, y, RESET_THRESHOLD));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new UnsharpReset(this, plugin, x, y));
	add_subwindow(default_settings = new UnsharpDefaultSettings(this,plugin,
		(get_w() - xs10 - defaultBtn_w), y, defaultBtn_w));

	show_window();
	flush();
}


void UnsharpWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_RADIUS : 
			radius_text->update(plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			break;
		case RESET_AMOUNT : 
			amount_text->update(plugin->config.amount);
			amount_slider->update(plugin->config.amount);
			break;
		case RESET_THRESHOLD : 
			threshold_text->update((int64_t)plugin->config.threshold);
			threshold_slider->update((int64_t)plugin->config.threshold);
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			radius_text->update(plugin->config.radius);
			radius_slider->update(plugin->config.radius);
			amount_text->update(plugin->config.amount);
			amount_slider->update(plugin->config.amount);
			threshold_text->update((int64_t)plugin->config.threshold);
			threshold_slider->update((int64_t)plugin->config.threshold);
			break;
	}
}



/* *********************************** */
/* **** UNSHARP RADIUS *************** */
UnsharpRadiusText::UnsharpRadiusText(UnsharpWindow *window, UnsharpMain *plugin, int x, int y)
 : BC_TumbleTextBox(window, plugin->config.radius,
	(float)RADIUS_MIN, (float)RADIUS_MAX, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
	set_increment(0.1);
}

UnsharpRadiusText::~UnsharpRadiusText()
{
}

int UnsharpRadiusText::handle_event()
{
	float min = RADIUS_MIN, max = RADIUS_MAX;
	float output = atof(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.radius = output;
	window->radius_slider->update(plugin->config.radius);
	window->radius_text->update(plugin->config.radius);
	plugin->send_configure_change();
	return 1;
}

UnsharpRadiusSlider::UnsharpRadiusSlider(UnsharpWindow *window, UnsharpMain *plugin,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, RADIUS_MIN, RADIUS_MAX, plugin->config.radius)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

UnsharpRadiusSlider::~UnsharpRadiusSlider()
{
}

int UnsharpRadiusSlider::handle_event()
{
	plugin->config.radius = get_value();
	window->radius_text->update(plugin->config.radius);
	plugin->send_configure_change();
	return 1;
}
/* *********************************** */


/* *********************************** */
/* **** UNSHARP AMOUNT *************** */
UnsharpAmountText::UnsharpAmountText(UnsharpWindow *window, UnsharpMain *plugin, int x, int y)
 : BC_TumbleTextBox(window, plugin->config.amount,
	(float)AMOUNT_MIN, (float)AMOUNT_MAX, x, y, xS(60), 2)
{
	this->window = window;
	this->plugin = plugin;
	set_increment(0.1);
}

UnsharpAmountText::~UnsharpAmountText()
{
}

int UnsharpAmountText::handle_event()
{
	float min = AMOUNT_MIN, max = AMOUNT_MAX;
	float output = atof(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.amount = output;
	window->amount_slider->update(plugin->config.amount);
	window->amount_text->update(plugin->config.amount);
	plugin->send_configure_change();
	return 1;
}

UnsharpAmountSlider::UnsharpAmountSlider(UnsharpWindow *window, UnsharpMain *plugin,
	int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, AMOUNT_MIN, AMOUNT_MAX, plugin->config.amount)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
	set_precision(0.01);
}

UnsharpAmountSlider::~UnsharpAmountSlider()
{
}

int UnsharpAmountSlider::handle_event()
{
	plugin->config.amount = get_value();
	window->amount_text->update(plugin->config.amount);
	plugin->send_configure_change();
	return 1;
}
/* *********************************** */


/* *********************************** */
/* **** UNSHARP THRESHOLD ************ */
UnsharpThresholdText::UnsharpThresholdText(UnsharpWindow *window, UnsharpMain *plugin, int x, int y)
 : BC_TumbleTextBox(window, plugin->config.threshold,
	THRESHOLD_MIN, THRESHOLD_MAX, x, y, xS(60))
{
	this->window = window;
	this->plugin = plugin;
	set_increment(1);
}

UnsharpThresholdText::~UnsharpThresholdText()
{
}

int UnsharpThresholdText::handle_event()
{
	int min = THRESHOLD_MIN, max = THRESHOLD_MAX;
	int output = atoi(get_text());
	if(output > max) output = max;
	if(output < min) output = min;
	plugin->config.threshold = output;
	window->threshold_slider->update(plugin->config.threshold);
	window->threshold_text->update((int64_t)plugin->config.threshold);
	plugin->send_configure_change();
	return 1;
}

UnsharpThresholdSlider::UnsharpThresholdSlider(UnsharpWindow *window, UnsharpMain *plugin,
	int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, THRESHOLD_MIN, THRESHOLD_MAX, plugin->config.threshold)
{
	this->window = window;
	this->plugin = plugin;
	enable_show_value(0); // Hide caption
}

UnsharpThresholdSlider::~UnsharpThresholdSlider()
{
}

int UnsharpThresholdSlider::handle_event()
{
	plugin->config.threshold = get_value();
	window->threshold_text->update((int64_t)plugin->config.threshold);
	plugin->send_configure_change();
	return 1;
}
/* *********************************** */


UnsharpReset::UnsharpReset(UnsharpWindow *window, UnsharpMain *plugin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
UnsharpReset::~UnsharpReset()
{
}
int UnsharpReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}

UnsharpDefaultSettings::UnsharpDefaultSettings(UnsharpWindow *window, UnsharpMain *plugin, int x, int y, int w)
 : BC_GenericButton(x, y, w, _("Default"))
{
	this->plugin = plugin;
	this->window = window;
}
UnsharpDefaultSettings::~UnsharpDefaultSettings()
{
}
int UnsharpDefaultSettings::handle_event()
{
	plugin->config.reset(RESET_DEFAULT_SETTINGS);
	window->update_gui(RESET_DEFAULT_SETTINGS);
	plugin->send_configure_change();
	return 1;
}

UnsharpClr::UnsharpClr(UnsharpWindow *window, UnsharpMain *plugin, int x, int y, int clear)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->window = window;
	this->plugin = plugin;
	this->clear = clear;
}
UnsharpClr::~UnsharpClr()
{
}
int UnsharpClr::handle_event()
{
	// clear==1 ==> Radius slider
	// clear==2 ==> Amount slider
	// clear==3 ==> Threshold slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}
