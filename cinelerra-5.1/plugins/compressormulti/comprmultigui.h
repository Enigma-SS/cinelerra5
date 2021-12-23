/*
 * CINELERRA
 * Copyright (C) 2008-2019 Adam Williams <broadcast at earthling dot net>
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



#ifndef COMPRMULTIGUI_H
#define COMPRMULTIGUI_H

#include "bchash.inc"
#include "comprmulti.h"
#include "compressortools.h"
#include "eqcanvas.inc"
#include "guicast.h"

class ComprMultiWindow;


class ComprMultiCanvas : public CompressorCanvasBase
{
public:
	ComprMultiCanvas(ComprMultiEffect *plugin,
		ComprMultiWindow *window, int x, int y, int w, int h);
	void update_window();
};

class ComprMultiBand : public BC_Radial
{
public:
	ComprMultiBand(ComprMultiWindow *window,
		ComprMultiEffect *plugin, int x, int y, int number, char *text);
	int handle_event();

	ComprMultiWindow *window;
	ComprMultiEffect *plugin;
// 0 - (TOTAL_BANDS-1)
	int number;
};


class ComprMultiReaction : public BC_TumbleTextBox
{
public:
	ComprMultiReaction(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};

class ComprMultiX : public BC_TumbleTextBox
{
public:
	ComprMultiX(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
	ComprMultiWindow *window;
};

class ComprMultiY : public BC_TumbleTextBox
{
public:
	ComprMultiY(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
	ComprMultiWindow *window;
};

class ComprMultiTrigger : public BC_TumbleTextBox
{
public:
	ComprMultiTrigger(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};

class ComprMultiDecay : public BC_TumbleTextBox
{
public:
	ComprMultiDecay(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};


class ComprMultiClear : public BC_GenericButton
{
public:
	ComprMultiClear(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
	ComprMultiWindow *window;
};

class ComprMultiReset : public BC_GenericButton
{
public:
	ComprMultiReset(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
	ComprMultiWindow *window;
};

class ComprMultiSmooth : public BC_CheckBox
{
public:
	ComprMultiSmooth(ComprMultiEffect *plugin, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};

class ComprMultiSolo : public BC_CheckBox
{
public:
	ComprMultiSolo(ComprMultiEffect *plugin, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};

class ComprMultiBypass : public BC_CheckBox
{
public:
	ComprMultiBypass(ComprMultiEffect *plugin, int x, int y);
	int handle_event();
	ComprMultiEffect *plugin;
};

class ComprMultiInput : public BC_PopupMenu
{
public:
	ComprMultiInput(ComprMultiEffect *plugin,
			ComprMultiWindow *window, int x, int y);
	void create_objects();
	int handle_event();
	static const char* value_to_text(int value);
	static int text_to_value(char *text);
	ComprMultiEffect *plugin;
	ComprMultiWindow *window;
};


class ComprMultiFPot : public BC_FPot
{
public:
	ComprMultiFPot(ComprMultiWindow *gui, ComprMultiEffect *plugin,
			int x, int y, double *output, double min, double max);
	int handle_event();
	ComprMultiWindow *gui;
	ComprMultiEffect *plugin;
	double *output;
};

class ComprMultiQPot : public BC_QPot
{
public:
	ComprMultiQPot(ComprMultiWindow *gui, ComprMultiEffect *plugin,
		int x, int y, int *output);
	int handle_event();
	ComprMultiWindow *gui;
	ComprMultiEffect *plugin;
	int *output;
};


class ComprMultiSize : public BC_PopupMenu
{
public:
	ComprMultiSize(ComprMultiWindow *gui,
			ComprMultiEffect *plugin, int x, int y);
	int handle_event();
	void create_objects();		 // add initial items
	void update(int size);
	ComprMultiWindow *gui;
	ComprMultiEffect *plugin;	
};

class ComprMultiMkupGain : public BC_FPot
{
public:
	ComprMultiMkupGain(ComprMultiEffect *plugin, ComprMultiWindow *window,
			int x, int y, double *output, double min, double max);
	int handle_event();
	ComprMultiWindow *window;
	ComprMultiEffect *plugin;
	double *output;
};


class ComprMultiWindow : public PluginClientWindow
{
public:
	ComprMultiWindow(ComprMultiEffect *plugin);
	~ComprMultiWindow();
	void create_objects();
	void update();
// draw the dynamic range canvas
	void update_canvas();
// draw the bandpass canvas
	void update_eqcanvas();
	int resize_event(int w, int h);	

	ComprMultiCanvas *canvas;
	ComprMultiReaction *reaction;
	ComprMultiClear *clear;
	ComprMultiReset *reset;
	ComprMultiX *x_text;
	ComprMultiY *y_text;
	ComprMultiTrigger *trigger;
	ComprMultiDecay *decay;
	ComprMultiSmooth *smooth;
	ComprMultiSolo *solo;
	ComprMultiBypass *bypass;
	ComprMultiInput *input;
	BC_Meter *in;
	BC_Meter *gain_change;
	ComprMultiBand *band[TOTAL_BANDS];
	CompressorGainFrame *gain_frame;
	CompressorFreqFrame *freq_frame;

	ComprMultiQPot *freq1;
	ComprMultiQPot *freq2;
	ComprMultiFPot *q;
	ComprMultiMkupGain *mkup_gain;
	ComprMultiSize *size;
	EQCanvas *eqcanvas;

	ComprMultiEffect *plugin;
	BC_Hash *defaults;
};

#endif

