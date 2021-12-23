
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
#include "brightnesswindow.h"
#include "language.h"
#include "theme.h"









BrightnessWindow::BrightnessWindow(BrightnessMain *client)
 : PluginClientWindow(client, xS(420), yS(160), xS(420), yS(160), 0)
{
	this->client = client;
}

BrightnessWindow::~BrightnessWindow()
{
}

void BrightnessWindow::create_objects()
{
	int xs10 = xS(10), xs200 = xS(200);
	int ys10 = yS(10), ys30 = yS(30), ys40 = yS(40);
	int x = xs10, y = ys10;
	int x2 = xS(80), x3 = xS(180);
	int clr_x = get_w()-x - xS(22); // note: clrBtn_w = 22

	BC_Bar *bar;

// Brightness
	y += ys10;
	add_tool(new BC_Title(x, y,_("Brightness:")));
	brightness_text = new BrightnessFText(this, client,
		0, &(client->config.brightness), (x + x2), y, -MAXVALUE, MAXVALUE);
	brightness_text->create_objects();
	brightness_slider = new BrightnessFSlider(client,
		brightness_text, &(client->config.brightness), x3, y, -MAXVALUE, MAXVALUE, xs200, 1);
	add_subwindow(brightness_slider);
	brightness_text->slider = brightness_slider;
	clr_x = x3 + brightness_slider->get_w() + x;
	add_subwindow(brightness_Clr = new BrightnessClr(client, this, clr_x, y, RESET_BRIGHTNESS));
	y += ys30;

// Contrast
	add_tool(new BC_Title(x, y, _("Contrast:")));
	contrast_text = new BrightnessFText(this, client,
		0, &(client->config.contrast), (x + x2), y, -MAXVALUE, MAXVALUE);
	contrast_text->create_objects();
	contrast_slider = new BrightnessFSlider(client,
		contrast_text, &(client->config.contrast), x3, y, -MAXVALUE, MAXVALUE, xs200, 0);
	add_subwindow(contrast_slider);
	contrast_text->slider = contrast_slider;
	add_subwindow(contrast_Clr = new BrightnessClr(client, this, clr_x, y, RESET_CONTRAST));
	y += ys30;

// Luma
	add_tool(luma = new BrightnessLuma(client, x, y));
	y += ys40;

// Reset section
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += ys10;
	add_subwindow(reset = new BrightnessReset(client, this, x, y));

	show_window();
	flush();
}

// for Reset button
void BrightnessWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_CONTRAST :
			contrast_text->update(client->config.contrast);
			contrast_slider->update(client->config.contrast);
			break;
		case RESET_BRIGHTNESS:
			brightness_text->update(client->config.brightness);
			brightness_slider->update(client->config.brightness);
			break;
		case RESET_ALL :
		default:
			brightness_text->update(client->config.brightness);
			brightness_slider->update(client->config.brightness);
			contrast_text->update(client->config.contrast);
			contrast_slider->update(client->config.contrast);
			luma->update(client->config.luma);
			break;
	}
}


/* BRIGHTNESS VALUES
  brightness is stored    from -100.00  to +100.00
  brightness_slider  goes from -100.00  to +100.00
  brightness_caption goes from   -1.000 to   +1.000
  brightness_text    goes from -100.00  to +100.00
*/

/* CONTRAST VALUES
  contrast is stored    from -100.00  to +100.00
  contrast_slider  goes from -100.00  to +100.00
  contrast_caption goes from    0.000 to   +5.000 (clear to +1.000)
  contrast_text    goes from -100.00  to +100.00
*/

BrightnessFText::BrightnessFText(BrightnessWindow *window, BrightnessMain *client,
	BrightnessFSlider *slider, float *output, int x, int y, float min, float max)
 : BC_TumbleTextBox(window, *output,
	min, max, x, y, xS(60), 2)
{
	this->window = window;
	this->client = client;
	this->output = output;
	this->slider = slider;
	this->min = min;
	this->max = max;
	set_increment(0.01);
}

BrightnessFText::~BrightnessFText()
{
}

int BrightnessFText::handle_event()
{
	*output = atof(get_text());
	if(*output > max) *output = max;
	else if(*output < min) *output = min;
	slider->update(*output);
	client->send_configure_change();
	return 1;
}

BrightnessFSlider::BrightnessFSlider(BrightnessMain *client,
	BrightnessFText *text, float *output, int x, int y,
	float min, float max, int w, int is_brightness)
 : BC_FSlider(x, y, 0, w, w, min, max, *output)
{
	this->client = client;
	this->output = output;
	this->text = text;
	this->is_brightness = is_brightness;
	enable_show_value(0); // Hide caption
}

BrightnessFSlider::~BrightnessFSlider()
{
}

int BrightnessFSlider::handle_event()
{
	*output = get_value();
	text->update(*output);
	client->send_configure_change();
	return 1;
}

char* BrightnessFSlider::get_caption()
{
	float fraction;
	if(is_brightness)
	{
		fraction = *output / 100;
	}
	else
	{
		fraction = (*output < 0) ?
			(*output + 100) / 100 :
			(*output + 25) / 25;
	}
	sprintf(string, "%0.4f", fraction);
	return string;
}


BrightnessLuma::BrightnessLuma(BrightnessMain *client,
	int x,
	int y)
 : BC_CheckBox(x,
 	y,
	client->config.luma,
	_("Boost luminance only"))
{
	this->client = client;
}
BrightnessLuma::~BrightnessLuma()
{
}
int BrightnessLuma::handle_event()
{
	client->config.luma = get_value();
	client->send_configure_change();
	return 1;
}


BrightnessReset::BrightnessReset(BrightnessMain *client, BrightnessWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->window = window;
}
BrightnessReset::~BrightnessReset()
{
}
int BrightnessReset::handle_event()
{
	client->config.reset(RESET_ALL); // clear=0 ==> reset all
	window->update_gui(RESET_ALL);
	client->send_configure_change();
	return 1;
}

BrightnessClr::BrightnessClr(BrightnessMain *client, BrightnessWindow *window, int x, int y, int clear)
 : BC_Button(x, y, client->get_theme()->get_image_set("reset_button"))
{
	this->client = client;
	this->window = window;
	this->clear = clear;
}
BrightnessClr::~BrightnessClr()
{
}
int BrightnessClr::handle_event()
{
	// clear==1 ==> Contrast text-slider
	// clear==2 ==> Brightness text-slider
	client->config.reset(clear);
	window->update_gui(clear);
	client->send_configure_change();
	return 1;
}

