
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

#ifndef PATCHGUI_H
#define PATCHGUI_H

#include "guicast.h"
#include "mwindow.inc"
#include "patchbay.inc"
#include "patchgui.inc"
#include "intauto.inc"
#include "track.inc"


class PatchGUI
{
public:
	PatchGUI(MWindow *mwindow,
		PatchBay *patchbay,
		Track *track,
		int x,
		int y);
	virtual ~PatchGUI();

	virtual void create_objects();
	virtual int reposition(int x, int y);
	void toggle_behavior(int type,
		int value,
		BC_Toggle *toggle,
		int *output);
	virtual int update(int x, int y);
	void synchronize_faders(float change, int audio, int video, int edge, int span);
	char* calculate_nudge_text(int *changed);
	int64_t calculate_nudge(const char *string);

	MWindow *mwindow;
	PatchBay *patchbay;
	Track *track;
// Used by update routines so non-existent track doesn't need to be dereferenced
// to know it doesn't match the current EDL.
	int track_id;
	int data_type, mixer;
	int x, y;
// Don't synchronize the fader if this is true.
	int change_source;

	TitlePatch *title;
	RecordPatch *record;
	PlayPatch *play;
//	AutoPatch *automate;
	GangPatch *gang;
	DrawPatch *draw;
	MutePatch *mute;
	ZoomPatch *zoom;
	MasterPatch *master;
	ExpandPatch *expand;
	NudgePatch *nudge;
	MixPatch *mix;
	char string_return[BCTEXTLEN];
	int edge, span;
};



class PlayPatch : public BC_Toggle
{
public:
	PlayPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class RecordPatch : public BC_Toggle
{
public:
	RecordPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class TitlePatch : public BC_TextBox
{
public:
	TitlePatch(MWindow *mwindow, PatchGUI *patch, int x, int y, int w);
	int handle_event();
	void update(const char *text);
	MWindow *mwindow;
	PatchGUI *patch;
};

class AutoPatch : public BC_Toggle
{
public:
	AutoPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class GangPatch : public BC_Toggle
{
public:
	GangPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class DrawPatch : public BC_Toggle
{
public:
	DrawPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class MutePatch : public BC_Toggle
{
public:
	MutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class ZoomPatch : public BC_Tumbler
{
public:
	ZoomPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int handle_up_event();
	int handle_down_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class MasterPatch : public BC_Toggle
{
public:
	MasterPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int handle_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class ExpandPatch : public BC_Toggle
{
public:
	ExpandPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	PatchGUI *patch;
};

class NudgePatch : public BC_TextBox
{
public:
	NudgePatch(MWindow *mwindow, PatchGUI *patch, int x, int y, int w);
	int handle_event();
	int button_press_event();
	void update();
	void set_value(int64_t value);
	int64_t calculate_increment();

	MWindow *mwindow;
	PatchGUI *patch;
};

class MixPatch : public BC_Toggle
{
public:
	MixPatch(MWindow *mwindow, PatchGUI *patch, int x, int y);
	~MixPatch();
	int handle_event();
	void update(int v);

	MWindow *mwindow;
	PatchGUI *patch;
};

#endif
