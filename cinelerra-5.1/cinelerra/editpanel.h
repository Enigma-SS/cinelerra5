
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

#ifndef EDITPANEL_H
#define EDITPANEL_H

#include "bcdialog.h"
#include "guicast.h"
#include "editpanel.inc"
#include "localsession.inc"
#include "meterpanel.inc"
#include "mwindow.inc"
#include "manualgoto.inc"
#include "scopewindow.h"



class EditPanel;


class EditInPoint : public BC_Button
{
public:
	EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditInPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditOutPoint : public BC_Button
{
public:
	EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditOutPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditDelInPoint : public BC_Button
{
public:
	EditDelInPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditDelInPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditDelOutPoint : public BC_Button
{
public:
	EditDelOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditDelOutPoint();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditSplice : public BC_Button
{
public:
	EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditSplice();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditOverwrite : public BC_Button
{
public:
	EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditOverwrite();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditToClip : public BC_Button
{
public:
	EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditToClip();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditManualGoto : public BC_Button
{
public:
	EditManualGoto(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditManualGoto();
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
	ManualGoto *mangoto;
};

class EditCut : public BC_Button
{
public:
	EditCut(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditCut();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditCommercial : public BC_Button
{
public:
	EditCommercial(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditCommercial();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditClick2Play : public BC_Toggle
{
public:
	EditClick2Play(MWindow *mwindow, EditPanel *panel, int x, int y);
        EditClick2Play();

        int handle_event();
	int keypress_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditCopy : public BC_Button
{
public:
	EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditCopy();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditPaste : public BC_Button
{
public:
	EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPaste();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditUndo : public BC_Button
{
public:
	EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditUndo();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditRedo : public BC_Button
{
public:
	EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditRedo();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditLabelbutton : public BC_Button
{
public:
	EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditLabelbutton();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditFit : public BC_Button
{
public:
	EditFit(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditFit();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class EditFitAutos : public BC_Button
{
public:
	EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditFitAutos();
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};


class EditPrevLabel : public BC_Button
{
public:
	EditPrevLabel(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPrevLabel();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditNextLabel : public BC_Button
{
public:
	EditNextLabel(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditNextLabel();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditPrevEdit : public BC_Button
{
public:
	EditPrevEdit(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPrevEdit();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};

class EditNextEdit : public BC_Button
{
public:
	EditNextEdit(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditNextEdit();

	int keypress_event();
	int handle_event();

	MWindow *mwindow;
	EditPanel *panel;
};


class ArrowButton : public BC_Toggle
{
public:
	ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y);
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class IBeamButton : public BC_Toggle
{
public:
	IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y);
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class KeyFrameButton : public BC_Toggle
{
public:
	KeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y);
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class SpanKeyFrameButton : public BC_Toggle
{
public:
	SpanKeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y);
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};

class LockLabelsButton : public BC_Toggle
{
public:
	LockLabelsButton(MWindow *mwindow, EditPanel *panel, int x, int y);
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
};


class EditPanelScopeGUI : public ScopeGUI
{
public:
	EditPanelScopeGUI(MWindow *mwindow, EditPanelScopeDialog *dialog);
	~EditPanelScopeGUI();

	void create_objects();
	void toggle_event();
	int translation_event();
	int resize_event(int w, int h);
	void update_scope();

	MWindow *mwindow;
	EditPanelScopeDialog *dialog;
};

class EditPanelScopeDialog : public BC_DialogThread
{
public:
	EditPanelScopeDialog(MWindow *mwindow, EditPanel *panel);
	~EditPanelScopeDialog();

	void handle_close_event(int result);
	void handle_done_event(int result);
	BC_Window* new_gui();
	void process(VFrame *output_frame);

	MWindow *mwindow;
	EditPanel *panel;
	EditPanelScopeGUI *scope_gui;
	Mutex *gui_lock;
	VFrame *output_frame;
};

class EditPanelGangTracks : public BC_Button
{
	static VFrame **gang_images[TOTAL_GANGS];
	static const char *gang_tips[TOTAL_GANGS];
public:
	EditPanelGangTracks(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPanelGangTracks();
	VFrame **get_images(MWindow *mwindow);
	void update(int gang);
	int handle_event();
	EditPanel *panel;
	MWindow *mwindow;
};


class EditPanelScope : public BC_Toggle
{
public:
	EditPanelScope(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPanelScope();
	int handle_event();
	EditPanel *panel;
	MWindow *mwindow;
};

class EditPanelTimecode : public BC_Button
{
public:
	EditPanelTimecode(MWindow *mwindow, EditPanel *panel, int x, int y);
	~EditPanelTimecode();
	int handle_event();
	MWindow *mwindow;
	EditPanel *panel;
	EditPanelTcDialog *tc_dialog;
};

class EditPanelTcDialog : public BC_DialogThread
{
public:
	EditPanelTcDialog(MWindow *mwindow, EditPanel *panel);
	~EditPanelTcDialog();
	BC_Window *new_gui();
	void start_dialog(int px, int py);
	void handle_done_event(int result);

	MWindow *mwindow;
	EditPanel *panel;
	EditPanelTcWindow *tc_gui;
	int px, py;
};

class EditPanelTcWindow : public BC_Window
{
public:
	EditPanelTcWindow(EditPanelTcDialog *tc_dialog, int x, int y);
	~EditPanelTcWindow();
	void create_objects();
	double get_timecode();
	void update(double timecode);

	EditPanelTcDialog *tc_dialog;
	EditPanelTcInt *hours;
	EditPanelTcInt *minutes;
	EditPanelTcInt *seconds;
	EditPanelTcInt *frames;
};

class EditPanelTcInt : public BC_TextBox
{
public:
	EditPanelTcInt(EditPanelTcWindow *window, int x, int y, int w,
		int max, const char *format);
	~EditPanelTcInt();
	int handle_event();
	int keypress_event();
	void update(int v);

	EditPanelTcWindow *window;
	int max, digits;
	const char *format;
};

class EditPanelTcReset : public BC_Button
{
public:
	EditPanelTcReset(EditPanelTcWindow *window, int x, int y);
	int handle_event();

	EditPanelTcWindow *window;
};

class EditPanel
{
public:
	EditPanel(MWindow *mwindow, BC_WindowBase *subwindow,
		int window_id, int x, int y,
		int editing_mode,   // From edl.inc
		int use_editing_mode,
		int use_keyframe,
		int use_splice,   // Extra buttons
		int use_overwrite,
		int use_copy,  // Use copy when in EDITING_ARROW
		int use_paste,
		int use_undo,
		int use_fit,
		int use_locklabels,
		int use_labels,
		int use_toclip,
		int use_meters,
		int use_cut,
		int use_commerical,
		int use_goto,
		int use_clk2play,
		int use_scope,
		int use_gang_tracks,
		int use_timecode);
	~EditPanel();

	void set_meters(MeterPanel *meter_panel);
	static int calculate_w(MWindow *mwindow, int use_keyframe, int total_buttons);
	static int calculate_h(MWindow *mwindow);
	void update();
	void create_buttons();
	void stop_transport(const char *lock_msg);
	void reposition_buttons(int x, int y);
	void create_objects();
	int get_w();

	virtual double get_position() = 0;
	virtual void set_position(double position) = 0;
	virtual void set_click_to_play(int v) = 0;

	virtual void panel_stop_transport() = 0;
	virtual void panel_toggle_label() = 0;
	virtual void panel_next_label(int cut) = 0;
	virtual void panel_prev_label(int cut) = 0;
	virtual void panel_prev_edit(int cut) = 0;
	virtual void panel_next_edit(int cut) = 0;
	virtual void panel_copy_selection() = 0;
	virtual void panel_overwrite_selection() = 0;
	virtual void panel_splice_selection() = 0;
	virtual void panel_set_inpoint() = 0;
	virtual void panel_set_outpoint() = 0;
	virtual void panel_unset_inoutpoint() = 0;
	virtual void panel_to_clip() = 0;
	virtual void panel_cut() = 0;
	virtual void panel_paste() = 0;
	virtual void panel_fit_selection() = 0;
	virtual void panel_fit_autos(int all) = 0;
	virtual void panel_set_editing_mode(int mode) = 0;
	virtual void panel_set_auto_keyframes(int v) = 0;
	virtual void panel_set_span_keyframes(int v) = 0;
	virtual void panel_set_labels_follow_edits(int v) = 0;
	virtual void panel_set_gang_tracks(int mode) = 0;

	MWindow *mwindow;
	BC_WindowBase *subwindow;
	MeterPanel *meter_panel;

	int window_id;
	int x, y, x1, y1;
	int editing_mode;
	int use_editing_mode;
	int use_keyframe;
	int use_splice;
	int use_overwrite;
	int use_paste;
	int use_undo;
	int use_fit;
	int use_copy;
	int use_locklabels;
	int use_labels;
	int use_toclip;
	int use_meters;
	int use_cut;
	int use_commercial;
	int use_goto;
	int use_clk2play;
	int use_scope;
	int use_gang_tracks;
	int use_timecode;

	EditFit *fit;
	EditFitAutos *fit_autos;
	EditInPoint *inpoint;
	EditOutPoint *outpoint;
//	EditDelInPoint *delinpoint;
//	EditDelOutPoint *deloutpoint;
	EditSplice *splice;
	EditOverwrite *overwrite;
	EditToClip *clip;
	EditCut *cut;
	EditCommercial *commercial;
	EditManualGoto *mangoto;
	EditClick2Play *click2play;
	EditPanelScope *scope;
	EditPanelTimecode *timecode;
	EditPanelScopeDialog *scope_dialog;
	EditCopy *copy;
	EditPaste *paste;
	EditLabelbutton *labelbutton;
	EditPrevLabel *prevlabel;
	EditNextLabel *nextlabel;
	EditPrevEdit *prevedit;
	EditNextEdit *nextedit;
	EditPanelGangTracks *gang_tracks;
	EditUndo *undo;
	EditRedo *redo;
	MeterShow *meters;
	ArrowButton *arrow;
	IBeamButton *ibeam;
	KeyFrameButton *keyframe;
	SpanKeyFrameButton *span_keyframe;
	LockLabelsButton *locklabels;

	int is_mwindow() { return window_id == MWINDOW_ID; }
	int is_cwindow() { return window_id == CWINDOW_ID; }
	int is_vwindow() { return window_id == VWINDOW_ID; }
};

#endif
