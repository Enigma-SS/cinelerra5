
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

#include "bcresources.h"
#include "bctitle.h"
#include "bcpixmap.h"
#include "vframe.h"
#include <string.h>
#include <unistd.h>

BC_Title::BC_Title(int x,
		int y,
		const char *text,
		int font,
		int color,
		int centered,
		int fixed_w)
 : BC_SubWindow(x, y, -1, -1, -1)
{
	this->font = font;
	if(color < 0)
		this->color = get_resources()->default_text_color;
	else
		this->color = color;
	this->centered = centered;
	this->fixed_w = fixed_w;
	strcpy(this->text, text);
}

BC_Title::~BC_Title()
{
}


int BC_Title::initialize()
{
	if(w <= 0 || h <= 0)
		get_size(this, font, text, fixed_w, w, h);

	if(centered) x -= w / 2;

	BC_SubWindow::initialize();
	draw(0);
	show_window(0);
	return 0;
}

int BC_Title::set_color(int color)
{
	this->color = color;
	draw(0);
	return 0;
}

int BC_Title::resize(int w, int h)
{
	resize_window(w, h);
	draw(0);
	return 0;
}

int BC_Title::reposition(int x, int y)
{
	reposition_window(x, y, w, h);
	draw(0);
	return 0;
}


int BC_Title::update(const char *text, int flush)
{
	int new_w, new_h;

	strcpy(this->text, text);
	get_size(this, font, text, fixed_w, new_w, new_h);
	if(new_w > w || new_h > h)
	{
		resize_window(new_w, new_h);
	}
	draw(flush);
	return 0;
}

void BC_Title::update(float value)
{
	char string[BCTEXTLEN];
	sprintf(string, "%.04f", value);
	update(string);
}

char* BC_Title::get_text()
{
	return text;
}

int BC_Title::draw(int flush)
{
// Fix background for block fonts.
// This should eventually be included in a BC_WindowBase::is_blocked_font()

 	if(font == MEDIUM_7SEGMENT)
 	{
		//leave it up to the theme to decide if we need a background or not.
		if (top_level->get_resources()->draw_clock_background) {
			BC_WindowBase::set_color(get_bg_color());
			draw_box(0, 0, w, h);
		}
 	}
	else
 		draw_top_background(parent_window, 0, 0, w, h);

	set_font(font);
	BC_WindowBase::set_color(color);
	return draw(flush, 0, 0);
}

int BC_Title::draw(int flush, int x, int y)
{
	y += get_text_ascent(font);
	int len = strlen(text);
	for( int i=0,j=0; i<=len; ++i ) {
		if( text[i] && text[i] != '\n' ) continue;
		if( centered )
			draw_center_text(get_w()/2, y, &text[j], i - j);
		else
			draw_text(x, y, &text[j], i - j);
		j = i + 1;
		y += get_text_height(font);
	}
	set_font(MEDIUMFONT);    // reset
	flash(flush);
	return 0;
}

int BC_Title::calculate_w(BC_WindowBase *gui, const char *text, int font)
{
	int temp_w, temp_h;
	get_size(gui, font, text, 0, temp_w, temp_h);
	return temp_w;
}

int BC_Title::calculate_h(BC_WindowBase *gui, const char *text, int font)
{
	int temp_w, temp_h;
	get_size(gui, font, text, 0, temp_w, temp_h);
	return temp_h;
}



void BC_Title::get_size(BC_WindowBase *gui, int font, const char *text, int fixed_w, int &w, int &h)
{
	int i, j;
	w = 0;
	h = 0;
	j = 0;

	int text_len = strlen(text);
	for(i = 0; i <= text_len; i++)
	{
		int line_w = 0;
		if(text[i] == '\n')
		{
			h++;
			line_w = gui->get_text_width(font, &text[j], i - j);
			j = i + 1;
		}
		else
		if(text[i] == 0)
		{
			h++;
			line_w = gui->get_text_width(font, &text[j]);
		}
		if(line_w > w) w = line_w;
	}

	h *= gui->get_text_height(font);
	w += xS(5);
	if(fixed_w > 0) w = fixed_w;
}


BC_TitleBar::BC_TitleBar(int x, int y, int w, int offset, int margin,
		const char *text, int font, int color, VFrame *data)
: BC_Title(x, y, text, font, color, 0, w)
{
	this->offset = offset;
	this->margin = margin;
	this->data = data;
	image = 0;
}

BC_TitleBar::~BC_TitleBar()
{
	delete image;
}

void BC_TitleBar::set_image(VFrame *data)
{
	delete image;
	image = new BC_Pixmap(get_parent_window(), data, PIXMAP_ALPHA);
}

int BC_TitleBar::initialize()
{
	if(data)
		set_image(data);
	else
		set_image(get_resources()->bar_data);
	BC_Title::initialize();
	draw(0);
	return 0;
}

int BC_TitleBar::draw(int flush)
{
	int w = get_w(), h = get_h(), h2 = h/2;
	draw_top_background(get_parent_window(), 0, 0,w, h);
	draw_3segmenth(0,h2, offset, 0, offset, image);
	int tx = offset + margin, tw, th;
	set_font(font);
	BC_WindowBase::set_color(color);
	BC_Title::draw(flush, tx, 0);
	get_size(get_parent_window(), font, text, 0, tw, th);
	tx += tw + margin;
	draw_3segmenth(tx,h2, w-tx, tx,w-tx, image);
	flash(flush);
	return 0;
}

