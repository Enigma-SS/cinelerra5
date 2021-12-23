
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

#ifndef CWINDOWTOOL_H
#define CWINDOWTOOL_H

#include "condition.inc"
#include "cwindowgui.inc"
#include "cwindowtool.inc"
#include "guicast.h"
#include "keyframe.inc"
#include "maskauto.inc"
#include "maskautos.inc"
#include "mwindow.inc"

enum {
	MASK_SHAPE_SQUARE,
	MASK_SHAPE_CIRCLE,
	MASK_SHAPE_TRIANGLE,
	MASK_SHAPE_OVAL,
};
enum {
	MASK_SCALE_X,
	MASK_SCALE_Y,
	MASK_SCALE_XY,
};

// This common thread supports all the tool GUI's.
class CWindowTool : public Thread
{
public:
	CWindowTool(MWindow *mwindow, CWindowGUI *gui);
	~CWindowTool();

// Called depending on state of toggle button
	void start_tool(int operation);
	void stop_tool();

// Called when window is visible
	void show_tool();
	void hide_tool();
	void raise_tool();

	void run();
	void update_show_window();
	void raise_window();
	void update_values();

	MWindow *mwindow;
	CWindowGUI *gui;
	CWindowToolGUI *tool_gui;
	int done;
	int current_tool;
	Condition *input_lock;
	Condition *output_lock;
// Lock run and update_values
	Mutex *tool_gui_lock;
};

class CWindowToolGUI : public BC_Window
{
public:
	CWindowToolGUI(MWindow *mwindow,
		CWindowTool *thread,
		const char *title,
		int w,
		int h);
	~CWindowToolGUI();

	virtual void create_objects() {};
// Update the keyframe from text boxes
	virtual void handle_event() {};
// Update text boxes from keyframe here
	virtual void update() {};

// Update EDL and preview only
	void update_preview(int changed_edl=0);
	void update_auto(Track *track, int idx, CWindowCoord *vp);
	void draw_preview(int changed_edl);
	int current_operation;
	virtual int close_event();
	int keypress_event();
	int translation_event();
	int press(void (CWindowCanvas::*fn)());

	MWindow *mwindow;
	CWindowTool *thread;
	CWindowCoord *event_caller;
	int edge, span;
};

class CWindowCoord : public BC_TumbleTextBox
{
public:
	CWindowCoord(CWindowToolGUI *gui, int x, int y, float value, int group=-1);
	CWindowCoord(CWindowToolGUI *gui, int x, int y, int value, int group=-1);
	void create_objects();
	void update_gui(float value);
// Calls the window's handle_event
	int handle_event();

	CWindowToolGUI *gui;
	int type;
	CWindowToolAutoRangeTumbler *min_tumbler;
	CWindowCoordSlider *slider;
	CWindowToolAutoRangeTumbler *max_tumbler;
	CWindowToolAutoRangeReset *range_reset;
	CWindowToolAutoRangeTextBox *range_text;
	CWindowCoordRangeTumbler *range;
};

class CWindowCoordSlider : public BC_FSlider
{
public:
	CWindowCoordSlider(CWindowCoord *coord, int x, int y, int w,
		float mn, float mx, float value);
	~CWindowCoordSlider();
	int handle_event();

	CWindowCoord *coord;
};

class CWindowCoordRangeTumbler : public BC_Tumbler
{
public:
	CWindowCoordRangeTumbler(CWindowCoord *coord, int x, int y);
	~CWindowCoordRangeTumbler();
	int update(float scale);
	int handle_up_event();
	int handle_down_event();

	CWindowCoord *coord;
};

class CWindowCurveAutoEdge : public BC_Toggle
{
public:
	CWindowCurveAutoEdge(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowCurveAutoSpan : public BC_Toggle
{
public:
	CWindowCurveAutoSpan(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);

	int handle_event();
	MWindow *mwindow;
	CWindowToolGUI *gui;
};


class CWindowCropApply : public BC_GenericButton
{
public:
	CWindowCropApply(MWindow *mwindow, CWindowCropGUI *crop_gui,
			int x, int y);
// Perform the cropping operation
	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	CWindowCropGUI *crop_gui;
};

class CWindowCropOpMode : public BC_PopupMenu
{
	static const char *crop_ops[CROP_MODES];
public:
	CWindowCropOpMode(MWindow *mwindow, CWindowCropGUI *crop_gui,
			int mode, int x, int y);
	~CWindowCropOpMode();
	void create_objects();
	int handle_event();

	MWindow *mwindow;
	CWindowCropGUI *crop_gui;
	int mode;
};

class CWindowCropOpItem : public BC_MenuItem
{
public:
	CWindowCropOpItem(CWindowCropOpMode *popup, const char *text, int id);
	int handle_event();

	CWindowCropOpMode *popup;
	int id;
};


class CWindowCropGUI : public CWindowToolGUI
{
public:
	CWindowCropGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCropGUI();
	void create_objects();
	void update();
// Update the gui
	void handle_event();
	CWindowCoord *x1, *y1, *width, *height;
	CWindowCropOpMode *crop_mode;
};

class CWindowMaskItem : public BC_ListBoxItem
{
public:
	CWindowMaskItem(const char *text, int id=-1, int color=-1)
	 : BC_ListBoxItem(text, color), id(id) {}
	~CWindowMaskItem() {}
	int id;
};

class CWindowMaskItems : public ArrayList<BC_ListBoxItem*>
{
public:
	CWindowMaskItems() {}
	~CWindowMaskItems() { remove_all_objects(); }
};

class CWindowMaskOnTrack : public BC_PopupTextBox
{
public:
	CWindowMaskOnTrack(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w, const char *text);
	~CWindowMaskOnTrack();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskItems track_items;

	int handle_event();
	void update_items();
};

class CWindowMaskTrackTumbler : public BC_Tumbler
{
public:
	CWindowMaskTrackTumbler(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskTrackTumbler();
	int handle_up_event();
	int handle_down_event();
	int do_event(int dir);

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};


class CWindowMaskName : public BC_PopupTextBox
{
public:
	CWindowMaskName(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, const char *text);
	~CWindowMaskName();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskItems mask_items;

	int handle_event();
	void update_items(MaskAuto *keyframe);
};

class CWindowMaskUnclear : public BC_Button
{
public:
	CWindowMaskUnclear(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskSoloTrack : public BC_CheckBox
{
public:
	CWindowMaskSoloTrack(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int v);
	static int calculate_w(BC_WindowBase *gui);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskDelMask : public BC_GenericButton
{
public:
	CWindowMaskDelMask(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskClrMask : public BC_Button
{
public:
	CWindowMaskClrMask(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskClrMask();
	static int calculate_w(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskButton : public BC_CheckBox
{
public:
	CWindowMaskButton(MWindow *mwindow, CWindowMaskGUI *gui,
			 int x, int y, int no, int v);
	~CWindowMaskButton();

	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int no;
};

class CWindowMaskThumbler : public BC_Tumbler
{
public:
	CWindowMaskThumbler(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskThumbler();
	int handle_up_event();
	int handle_down_event();
	int do_event(int dir);

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskEnable : public BC_CheckBox
{
public:
	CWindowMaskEnable(MWindow *mwindow, CWindowMaskGUI *gui,
			 int x, int y, int no, int v);
	~CWindowMaskEnable();

	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int no;
};

class CWindowMaskFade : public BC_TumbleTextBox
{
public:
	CWindowMaskFade(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskFade();
	int update(float v);
	int update_value(float v);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskFadeSlider : public BC_ISlider
{
public:
	CWindowMaskFadeSlider(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, int w);
	~CWindowMaskFadeSlider();
	int handle_event();
	int update(int64_t v);
	char *get_caption() { return 0; }
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int stick;
	float last_v;
	Timer *timer;
};

class CWindowMaskGangFader : public BC_Toggle
{
public:
	CWindowMaskGangFader(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskGangFader();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskGangFocus : public BC_Toggle
{
public:
	CWindowMaskGangFocus(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskGangFocus();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskGangPoint : public BC_Toggle
{
public:
	CWindowMaskGangPoint(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskGangPoint();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskSmoothButton : public BC_Button
{
public:
	CWindowMaskSmoothButton(MWindow *mwindow, CWindowMaskGUI *gui,
		const char *tip, int type, int on, int x, int y, const char *images);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int type, on;
};

class CWindowMaskAffectedPoint : public BC_TumbleTextBox
{
public:
	CWindowMaskAffectedPoint(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskAffectedPoint();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskFocus : public BC_CheckBox
{
public:
	CWindowMaskFocus(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskFocus();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	static int calculate_w(CWindowMaskGUI *gui);
};

class CWindowMaskScaleXY : public BC_Toggle
{
public:
	CWindowMaskScaleXY(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, VFrame **data, int v,
			int id, const char *tip);
	~CWindowMaskScaleXY();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int id;
};

class CWindowMaskHelp : public BC_CheckBox
{
public:
	CWindowMaskHelp(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskHelp();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskDrawMarkers : public BC_CheckBox
{
public:
	CWindowMaskDrawMarkers(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskDrawMarkers();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskDrawBoundary : public BC_CheckBox
{
public:
	CWindowMaskDrawBoundary(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskDrawBoundary();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskDelPoint : public BC_GenericButton
{
public:
	CWindowMaskDelPoint(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskFeather : public BC_TumbleTextBox
{
public:
	CWindowMaskFeather(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskFeather();
	int update(float v);
	int update_value(float v);
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskFeatherSlider : public BC_FSlider
{
public:
	CWindowMaskFeatherSlider(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, int w, float v);
	~CWindowMaskFeatherSlider();
	int handle_event();
	int update(float v);
	int update(int r, float v, float mn, float mx);
	char *get_caption() { return 0; }
	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int stick;
	float last_v;
	float max;
	Timer *timer;
};

class CWindowMaskGangFeather : public BC_Toggle
{
public:
	CWindowMaskGangFeather(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y);
	~CWindowMaskGangFeather();
	int handle_event();
	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskBeforePlugins : public BC_CheckBox
{
public:
	CWindowMaskBeforePlugins(CWindowMaskGUI *gui,
			int x, int y);
	int handle_event();
	CWindowMaskGUI *gui;
};

class CWindowMaskLoadList : public BC_ListBox
{
public:
	CWindowMaskLoadList(MWindow *mwindow, CWindowMaskGUI *gui);
	~CWindowMaskLoadList();
	void create_objects();
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskItems shape_items;
};

class CWindowMaskLoad : public BC_Button
{
public:
	CWindowMaskLoad(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w);
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskSave : public BC_Button
{
public:
	CWindowMaskSave(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w);
	~CWindowMaskSave();
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskPresetDialog *preset_dialog;
};

class CWindowMaskPresetDialog : public BC_DialogThread
{
public:
	CWindowMaskPresetDialog(MWindow *mwindow, CWindowMaskGUI *gui);
	~CWindowMaskPresetDialog();
	void handle_close_event(int result);
	void handle_done_event(int result);
	BC_Window* new_gui();
	void start_dialog(int sx, int sy, MaskAuto *keyframe);

	MWindow *mwindow;
	CWindowMaskGUI *gui;
	CWindowMaskPresetGUI *pgui;
	int sx, sy;
	MaskAuto *keyframe;
};

class CWindowMaskPresetGUI : public BC_Window
{
public:
	CWindowMaskPresetGUI(CWindowMaskPresetDialog *preset_dialog,
			int x, int y, const char *title);
	void create_objects();

	CWindowMaskPresetDialog *preset_dialog;
	CWindowMaskPresetText *preset_text;
};

class CWindowMaskPresetText : public BC_PopupTextBox
{
public:
	CWindowMaskPresetText(CWindowMaskPresetGUI *pgui,
		int x, int y, int w, int h, const char *text);
	int handle_event();
	void update_items();

	CWindowMaskPresetGUI *pgui;
	CWindowMaskItems mask_items;
};

class CWindowMaskDelete : public BC_Button
{
public:
	CWindowMaskDelete(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w);
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskCenter : public BC_Button
{
public:
	CWindowMaskCenter(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w);
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskNormal : public BC_Button
{
public:
	CWindowMaskNormal(MWindow *mwindow, CWindowMaskGUI *gui,
			int x, int y, int w);
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
};

class CWindowMaskShape : public BC_Button
{
public:
	CWindowMaskShape(MWindow *mwindow, CWindowMaskGUI *gui,
		const char *images, int shape, int x, int y, const char *tip);
	~CWindowMaskShape();
	void builtin_shape(int i, SubMask *sub_mask);
	int handle_event();

	MWindow *mwindow;
	CWindowMaskGUI *gui;
	int shape;
	CWindowMaskItems shape_items;
};

class CWindowDisableOpenGLMasking : public BC_CheckBox
{
public:
	CWindowDisableOpenGLMasking(CWindowMaskGUI *gui,
			int x, int y);
	int handle_event();
	CWindowMaskGUI *gui;
};

class CWindowMaskGUI : public CWindowToolGUI
{
public:
	CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowMaskGUI();
	void create_objects();
	void update();
	int close_event();
	void done_event();
	void handle_event();
	void set_focused(int v, float cx, float cy);
	void update_buttons(MaskAuto *keyframe, int k);
	void get_keyframe(Track* &track, MaskAutos* &autos, MaskAuto* &keyframe,
		SubMask* &mask, MaskPoint* &point, int create_it);
	void load_masks(ArrayList<SubMask *> &masks);
	void save_masks(ArrayList<SubMask *> &masks);
	int smooth_mask(int typ, int on);
	int save_mask(const char *nm);
	int del_mask(const char *nm);
	int center_mask();
	int normal_mask();

	CWindowMaskOnTrack *mask_on_track;
	CWindowMaskTrackTumbler *mask_track_tumbler;
	CWindowMaskName *mask_name;
	CWindowMaskButton *mask_buttons[SUBMASKS];
	CWindowMaskThumbler *mask_thumbler;
	BC_Title *mask_blabels[SUBMASKS];
	CWindowMaskEnable *mask_enables[SUBMASKS];
	CWindowMaskSoloTrack *mask_solo_track;
	CWindowMaskDelMask *mask_del;
	CWindowMaskUnclear *mask_unclr;
	CWindowMaskClrMask *mask_clr;
	CWindowMaskShape *mask_shape_sqr;
	CWindowMaskShape *mask_shape_crc;
	CWindowMaskShape *mask_shape_tri;
	CWindowMaskShape *mask_shape_ovl;
	CWindowMaskLoadList *mask_load_list;
	CWindowMaskLoad *mask_load;
	CWindowMaskSave *mask_save;
	CWindowMaskDelete *mask_delete;
	CWindowMaskPresetDialog *preset_dialog;
	CWindowMaskCenter *mask_center;
	CWindowMaskNormal *mask_normal;
	CWindowMaskFade *fade;
	CWindowMaskFadeSlider *fade_slider;
	CWindowMaskGangFader *gang_fader;
	CWindowMaskAffectedPoint *active_point;
	CWindowMaskDelPoint *del_point;
	CWindowMaskGangPoint *gang_point;
	CWindowMaskSmoothButton *mask_pnt_linear, *mask_pnt_smooth;
	CWindowMaskSmoothButton *mask_crv_linear, *mask_crv_smooth;
	CWindowMaskSmoothButton *mask_all_linear, *mask_all_smooth;
	CWindowCoord *x, *y;
	CWindowMaskScaleXY *mask_scale_x, *mask_scale_y, *mask_scale_xy;
	int scale_mode;
	CWindowMaskFocus *focus;
	int focused;
	CWindowMaskGangFocus *gang_focus;
	CWindowMaskHelp *help;
	int helped, help_y, help_h;
	CWindowMaskDrawMarkers *draw_markers;
	int markers;
	CWindowMaskDrawBoundary *draw_boundary;
	int boundary;
	CWindowCoord *focus_x, *focus_y;
	CWindowMaskFeather *feather;
	CWindowMaskFeatherSlider *feather_slider;
	CWindowMaskGangFeather *gang_feather;
	CWindowMaskBeforePlugins *apply_before_plugins;
	CWindowDisableOpenGLMasking *disable_opengl_masking;
};


class CWindowEyedropGUI : public CWindowToolGUI
{
public:
	CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowEyedropGUI();

	void handle_event();
	void create_objects();
	void update();

	BC_Title *current;
	CWindowCoord *radius;
	CWindowEyedropCheckBox *use_max;
	BC_Title *red, *green, *blue, *y, *u, *v;
	BC_Title *rgb_hex, *yuv_hex;
	BC_SubWindow *sample;
};


class CWindowEyedropCheckBox : public BC_CheckBox
{
public:
	CWindowEyedropCheckBox(MWindow *mwindow, 
		CWindowEyedropGUI *gui,
		int x, 
		int y);

	int handle_event();
	MWindow *mwindow;
	CWindowEyedropGUI *gui;
};


class CWindowToolAutoRangeTumbler : public BC_Tumbler
{
public:
	CWindowToolAutoRangeTumbler(CWindowCoord *coord, int x, int y,
			int use_max, const char *tip);
	int handle_up_event();
	int handle_down_event();

	CWindowCoord *coord;
	int use_max;
};

class CWindowToolAutoRangeReset : public BC_Button
{
public:
	CWindowToolAutoRangeReset(CWindowCoord *coord, int x, int y);
	int handle_event();

	CWindowCoord *coord;
};

class CWindowToolAutoRangeTextBox : public BC_TextBox
{
public:
	CWindowToolAutoRangeTextBox(CWindowCoord *coord, int x, int y);
	int button_press_event();
	int handle_event();
	int update_range();

	CWindowCoord *coord;
};


class CWindowCameraGUI : public CWindowToolGUI
{
public:
	CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowCameraGUI();
	void handle_event();
	void create_objects();
	void update();

	BC_TitleBar *bar1, *bar2;
	CWindowCoord *x, *y, *z;
	CWindowCameraAddKeyframe *add_keyframe;
	CWindowCameraReset *reset;
	BC_TitleBar *bar3, *bar4, *bar5;
	CWindowCurveToggle *t_smooth, *t_linear, *t_tangent, *t_free, *t_bump;
	CWindowCurveAutoSpan *auto_span;
	CWindowCurveAutoEdge *auto_edge;
};

class CWindowCameraLeft : public BC_Button
{
public:
	CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraCenter : public BC_Button
{
public:
	CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraRight : public BC_Button
{
public:
	CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraTop : public BC_Button
{
public:
	CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraMiddle : public BC_Button
{
public:
	CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraBottom : public BC_Button
{
public:
	CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowCameraGUI *gui;
};

class CWindowCameraAddKeyframe : public BC_Button
{
public:
	CWindowCameraAddKeyframe(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowCameraReset : public BC_Button
{
public:
	CWindowCameraReset(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};


class CWindowProjectorGUI : public CWindowToolGUI
{
public:
	CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowProjectorGUI();
	void handle_event();
	void create_objects();
	void update();

	BC_TitleBar *bar1, *bar2;
	CWindowCoord *x, *y, *z;
	CWindowProjectorAddKeyframe *add_keyframe;
	CWindowProjectorReset *reset;
	BC_TitleBar *bar3, *bar4, *bar5;
	CWindowCurveToggle *t_smooth, *t_linear, *t_tangent, *t_free, *t_bump;
	CWindowCurveAutoSpan *auto_span;
	CWindowCurveAutoEdge *auto_edge;
};

class CWindowProjectorLeft : public BC_Button
{
public:
	CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorCenter : public BC_Button
{
public:
	CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorRight : public BC_Button
{
public:
	CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorTop : public BC_Button
{
public:
	CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorMiddle : public BC_Button
{
public:
	CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorBottom : public BC_Button
{
public:
	CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui,
			int x, int y);
	int handle_event();
	MWindow *mwindow;
	CWindowProjectorGUI *gui;
};

class CWindowProjectorAddKeyframe : public BC_Button
{
public:
	CWindowProjectorAddKeyframe(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};

class CWindowProjectorReset : public BC_Button
{
public:
	CWindowProjectorReset(MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	int handle_event();

	MWindow *mwindow;
	CWindowToolGUI *gui;
};




class CWindowRulerGUI : public CWindowToolGUI
{
public:
	CWindowRulerGUI(MWindow *mwindow, CWindowTool *thread);
	~CWindowRulerGUI();
	void create_objects();
	void update();
// Update the gui
	void handle_event();

	BC_TextBox *current;
	BC_TextBox *point1;
	BC_TextBox *point2;
	BC_TextBox *deltas;
	BC_TextBox *distance;
	BC_TextBox *angle;
};


#endif
