
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

#include "cdripwindow.h"
#include "language.h"
#include "mwindow.inc"

#include <string.h>

CDRipWindow::CDRipWindow(CDRipMain *cdripper, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": CD Ripper"),
	x, y, xS(450), yS(230), xS(450), yS(230), 0, 0, 1)
{
	this->cdripper = cdripper;
// *** CONTEXT_HELP ***
	if(cdripper) context_help_set_keyword(cdripper->plugin_title());
	else 	     context_help_set_keyword("Rendered Audio Effects");
}

CDRipWindow::~CDRipWindow()
{
}

void CDRipWindow::create_objects()
{
	int xs10 = xS(10), xs70 = xS(70), xs100 = xS(100);
	int ys10 = yS(10), ys25 = yS(25), ys30 = yS(30), ys35 = yS(35);
	int y = ys10, x = xs10;
	add_tool(new BC_Title(x, y, _("Select the range to transfer:"))); y += ys25;
	add_tool(new BC_Title(x, y, _("Track:"))); x += xs70;
	add_tool(new BC_Title(x, y, _("Min."))); x += xs70;
	add_tool(new BC_Title(x, y, _("Sec."))); x += xs100;

	add_tool(new BC_Title(x, y, _("Track:"))); x += xs70;
	add_tool(new BC_Title(x, y, _("Min."))); x += xs70;
	add_tool(new BC_Title(x, y, _("Sec."))); x += xs100;

	x = xs10;  y += ys25;
	add_tool(track1 = new CDRipTextValue(this, &(cdripper->track1), x, y, 50));
	x += xs70;
	add_tool(min1 = new CDRipTextValue(this, &(cdripper->min1), x, y, 50));
	x += xs70;
	add_tool(sec1 = new CDRipTextValue(this, &(cdripper->sec1), x, y, 50));
	x += xs100;

	add_tool(track2 = new CDRipTextValue(this, &(cdripper->track2), x, y, 50));
	x += xs70;
	add_tool(min2 = new CDRipTextValue(this, &(cdripper->min2), x, y, 50));
	x += xs70;
	add_tool(sec2 = new CDRipTextValue(this, &(cdripper->sec2), x, y, 50));

	x = xs10;   y += ys30;
	add_tool(new BC_Title(x, y, _("From"), LARGEFONT, RED));
	x += xS(240);
	add_tool(new BC_Title(x, y, _("To"), LARGEFONT, RED));

	x = xs10;   y += ys35;
	add_tool(new BC_Title(x, y, _("CD Device:")));
	x += xs100;
	add_tool(device = new CDRipWindowDevice(this, cdripper->device, x, y, 200));

	x = xs10;   y += ys35;
	add_tool(new BC_OKButton(this));
	x += xS(300);
	add_tool(new BC_CancelButton(this));
	show_window();
	flush();
}








CDRipTextValue::CDRipTextValue(CDRipWindow *window, int *output, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, *output)
{
	this->output = output;
	this->window = window;
}

CDRipTextValue::~CDRipTextValue()
{
}

int CDRipTextValue::handle_event()
{
	*output = atol(get_text());
	return 1;
}

CDRipWindowDevice::CDRipWindowDevice(CDRipWindow *window, char *device, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, device)
{
	this->window = window;
	this->device = device;
}

CDRipWindowDevice::~CDRipWindowDevice()
{
}

int CDRipWindowDevice::handle_event()
{
	strcpy(device, get_text());
	return 1;
}
