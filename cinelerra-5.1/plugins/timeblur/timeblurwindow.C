
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "cursors.h"
#include "edl.h"
#include "timeblur.h"
#include "timeblurwindow.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "theme.h"

#include <unistd.h>


TimeBlurWindow::TimeBlurWindow(TimeBlurMain *plugin)
 : PluginClientWindow(plugin, xS(250), yS(50), xS(250), yS(50), 0)
{
	this->plugin = plugin;
}

TimeBlurWindow::~TimeBlurWindow()
{
}

void TimeBlurWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = margin, y = margin;
	int x1 = x;
	add_subwindow(select = new TimeBlurSelect(plugin, this, x1, y));
	x1 += select->get_w() + margin;
	frames = new TimeBlurFrames(plugin, this, x1, y);
	frames->create_objects();
	x1 += frames->get_w() + 3*margin;
	add_subwindow(clear_frames = new TimeBlurClearFrames(plugin, this, x1, y));
	show_window();
}


TimeBlurSelect::TimeBlurSelect(TimeBlurMain *plugin, TimeBlurWindow *gui,
		int x, int y)
 : BC_GenericButton(x, y, xS(100), _("Frames"))
{
	this->plugin = plugin;
	this->gui = gui;
	set_tooltip(_("Set frames to selection duration"));
}
int TimeBlurSelect::handle_event()
{
	MWindow *mwindow = plugin->server->mwindow;
	if( mwindow ) {
		EDL *edl = mwindow->edl;
		double start = edl->local_session->get_selectionstart();
		int64_t start_pos = edl->get_frame_rate() * start;
		double end = edl->local_session->get_selectionend();
		int64_t end_pos = edl->get_frame_rate() * end;
		int64_t frames = end_pos - start_pos;
		gui->frames->update(frames);
		plugin->config.frames = frames;
		plugin->send_configure_change();
	}
	return 1;
}

TimeBlurClearFrames::TimeBlurClearFrames(TimeBlurMain *plugin, TimeBlurWindow *gui,
		int x, int y)
 : BC_Button(x, y, plugin->get_theme()->get_image_set("reset_button"))
{
	this->plugin = plugin;
	this->gui = gui;
	set_tooltip(_("Clear frames"));
}

int TimeBlurClearFrames::handle_event()
{
	plugin->config.frames = 0;
	gui->frames->update(0);
	plugin->send_configure_change();
	return 1;
}

TimeBlurFrames::TimeBlurFrames(TimeBlurMain *plugin, TimeBlurWindow *gui,
		int x, int y)
 : BC_TumbleTextBox(gui, 0, 0, 65535, x, y, xS(80))
{
	this->plugin = plugin;
	this->gui = gui;
}

int TimeBlurFrames::handle_event()
{
	plugin->config.frames = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

void TimeBlurFrames::update(int frames)
{
	BC_TumbleTextBox::update((int64_t)frames);
}

