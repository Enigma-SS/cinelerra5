
/*
 * CINELERRA
 * Copyright (C) 2004 Andraz Tori
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

#ifndef MANUALGOTO_H
#define MANUALGOTO_H

#include "bcwindowbase.h"
#include "bcdialog.h"
#include "mwindow.inc"
#include "editpanel.inc"

class ManualGoto;
class ManualGotoKeyItem;
class ManualGotoDirection;
class ManualGotoUnitItem;
class ManualGotoUnits;
class ManualGotoText;
class ManualGotoWindow;

class ManualGoto : public BC_DialogThread
{
public:
	ManualGoto(MWindow *mwindow, EditPanel *panel);
	~ManualGoto();

	EditPanel *panel;
	MWindow *mwindow;
	ManualGotoWindow *window;

	BC_Window *new_gui();
	void handle_done_event(int result);

};

class ManualGotoKeyItem : public BC_MenuItem
{
public:
	ManualGotoKeyItem(ManualGotoDirection *popup,
		const char *text, const char *htxt);
	int handle_event();

	ManualGotoDirection *popup;
	const char *htxt;
};

class ManualGotoDirection : public BC_PopupMenu
{
public:
	ManualGotoDirection(ManualGotoWindow *window, int x, int y, int w);
	void create_objects();

	ManualGotoWindow *window;
};

class ManualGotoUnitItem : public BC_MenuItem
{
public:
	ManualGotoUnitItem(ManualGotoUnits *popup, int type);
	int handle_event();

	ManualGotoUnits *popup;
	int type;
};

class ManualGotoUnits : public BC_PopupMenu
{
public:
	ManualGotoUnits(ManualGotoWindow *window, int x, int y, int w);
	void create_objects();

	ManualGotoWindow *window;
};


class ManualGotoText : public BC_TextBox
{
public:
	ManualGotoText(ManualGotoWindow *window, int x, int y, int w);
	int keypress_event();
	ManualGotoWindow *window;
};

class ManualGotoWindow : public BC_Window
{
public:
	ManualGotoWindow(ManualGoto *mango, int x, int y);
	~ManualGotoWindow();

	void create_objects();
	void update(double position);
	void update();

	ManualGoto *mango;
	int time_format;
	BC_Title *format_text;
	ManualGotoDirection *direction;
	ManualGotoUnits *units;
	ManualGotoText *time_text;
};

#endif
