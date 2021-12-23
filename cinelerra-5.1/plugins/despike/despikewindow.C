
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
#include "despikewindow.h"
#include "language.h"
#include <string.h>








DespikeWindow::DespikeWindow(Despike *despike)
 : PluginClientWindow(despike,
	xS(230),
	yS(110),
	xS(230),
	yS(110),
	0)
{
	this->despike = despike;
}

DespikeWindow::~DespikeWindow()
{
}

void DespikeWindow::create_objects()
{
	int xs5 = xS(5), xs10 = xS(10);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30);
	int x = xs10, y = ys10;
	add_tool(new BC_Title(xs5, y, _("Maximum level:")));
	y += ys20;
	add_tool(level = new DespikeLevel(despike, x, y));
	y += ys30;
	add_tool(new BC_Title(xs5, y, _("Maximum rate of change:")));
	y += ys20;
	add_tool(slope = new DespikeSlope(despike, x, y));
	show_window();
	flush();
}





DespikeLevel::DespikeLevel(Despike *despike, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	xS(200),
	yS(200),
	INFINITYGAIN,
	0,
	despike->config.level)
{
	this->despike = despike;
}
int DespikeLevel::handle_event()
{
	despike->config.level = get_value();
	despike->send_configure_change();
	return 1;
}

DespikeSlope::DespikeSlope(Despike *despike, int x, int y)
 : BC_FSlider(x,
 	y,
	0,
	xS(200),
	yS(200),
	INFINITYGAIN,
	0,
	despike->config.slope)
{
	this->despike = despike;
}
int DespikeSlope::handle_event()
{
	despike->config.slope = get_value();
	despike->send_configure_change();
	return 1;
}
