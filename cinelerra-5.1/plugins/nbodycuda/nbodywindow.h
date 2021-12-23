/*
 * CINELERRA
 * Copyright (C) 2008-2014 Adam Williams <broadcast at earthling dot net>
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

#ifndef NBODYCUDAWINDOW_H
#define NBODYCUDAWINDOW_H


#include "guicast.h"
#include "nbody.h"

class N_BodyDrag;
class N_BodyReset;
class NBodyModeItems;
class N_BodySetMode;
class NBodyDemoItems;
class N_BodySetDemo;
class NBodyNumBodyItems;
class N_BodyNumBodies;
class N_BodyWindow;

class N_BodyDrag : public BC_CheckBox
{
public:
	N_BodyDrag(N_BodyWindow *gui, int x, int y);

	int handle_event();
	int handle_ungrab();
	N_BodyWindow *gui;
};

class N_BodyReset : public BC_GenericButton
{
public:
	N_BodyReset(N_BodyWindow *gui, int x, int y);
	~N_BodyReset();
	int handle_event();

	N_BodyWindow *gui;
};

class NBodyModeItems : public ArrayList<BC_ListBoxItem*>
{
public:
	NBodyModeItems() {} 
	~NBodyModeItems() { remove_all_objects(); }
};

class N_BodySetMode : public BC_PopupTextBox
{
public:
	N_BodySetMode(N_BodyWindow *gui, int x, int y, const char *text);
	void create_objects();
	int handle_event();

	N_BodyWindow *gui;
	NBodyModeItems mode_items;
};

class NBodyDemoItems : public ArrayList<BC_ListBoxItem*>
{
public:
	NBodyDemoItems() {} 
	~NBodyDemoItems() { remove_all_objects(); }
};

class N_BodySetDemo : public BC_PopupTextBox
{
public:
	N_BodySetDemo(N_BodyWindow *gui, int x, int y, const char *text);
	void create_objects();
	int handle_event();

	N_BodyWindow *gui;
	NBodyDemoItems demo_items;
};

class NBodyNumBodyItems : public ArrayList<BC_ListBoxItem*>
{
public:
	NBodyNumBodyItems() {} 
	~NBodyNumBodyItems() { remove_all_objects(); }
};

class N_BodyNumBodies : public BC_PopupTextBox
{
public:
	N_BodyNumBodies(N_BodyWindow *gui, int x, int y, const char *text);
	void create_objects();
	int handle_event();

	N_BodyWindow *gui;
	NBodyNumBodyItems num_body_items;
};


class N_BodyWindow : public PluginClientWindow
{
public:
	N_BodyWindow(N_BodyMain *plugin);
	~N_BodyWindow();

	void create_objects();
	void update_gui();

	int grab_event(XEvent *event);
	int do_grab_event(XEvent *event);
	void done_event(int result);
	void send_configure_change();

	N_BodyMain *plugin;
	N_BodySetMode *set_mode;
	N_BodySetDemo *set_demo;
	N_BodyNumBodies *num_bodies;
	N_BodyDrag *drag;
	N_BodyReset *reset;

	int press_x, press_y;
	int button_no, pending_config;
};

#endif
