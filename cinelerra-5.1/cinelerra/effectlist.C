/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "awindow.h"
#include "awindowgui.h"
#include "clip.h"
#include "cstrdup.h"
#include "effectlist.h"
#include "edl.h"
#include "guicast.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "pluginserver.h"


EffectTipItem::EffectTipItem(AWindowGUI *gui)
 : BC_MenuItem("","i",'i')
{
	this->gui = gui;
	update();
}
EffectTipItem::~EffectTipItem()
{
}


int EffectTipItem::handle_event()
{
	int v = !gui->tip_info ? 1 : 0;
	update(v);
	return 1;
}

void EffectTipItem::update(int v)
{
	if( v >= 0 ) gui->tip_info = v;
	else v = gui->tip_info;
	const char *text = v ?  _("Info off") : _("Info on");
	set_text(text);
}


EffectListMenu::EffectListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

EffectListMenu:: ~EffectListMenu()
{
}

void EffectListMenu::create_objects()
{
	add_item(info = new EffectTipItem(gui));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new AWindowListSort(mwindow, gui));
}

void EffectListMenu::update()
{
	info->update();
	format->update();
}

