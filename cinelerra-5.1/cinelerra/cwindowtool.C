/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include <stdio.h>
#include <stdint.h>

#include "automation.h"
#include "bccolors.h"
#include "bctimer.h"
#include "clip.h"
#include "condition.h"
#include "cpanel.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "gwindowgui.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "zoombar.h"


CWindowTool::CWindowTool(MWindow *mwindow, CWindowGUI *gui)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	tool_gui = 0;
	done = 0;
	current_tool = CWINDOW_NONE;
	set_synchronous(1);
	input_lock = new Condition(0, "CWindowTool::input_lock");
	output_lock = new Condition(1, "CWindowTool::output_lock");
	tool_gui_lock = new Mutex("CWindowTool::tool_gui_lock");
}

CWindowTool::~CWindowTool()
{
	done = 1;
	stop_tool();
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete output_lock;
	delete tool_gui_lock;
}

void CWindowTool::start_tool(int operation)
{
	CWindowToolGUI *new_gui = 0;
	int result = 0;

//printf("CWindowTool::start_tool 1\n");
	if(current_tool != operation)
	{
		int previous_tool = current_tool;
		current_tool = operation;
		switch(operation)
		{
			case CWINDOW_EYEDROP:
				new_gui = new CWindowEyedropGUI(mwindow, this);
				break;
			case CWINDOW_CROP:
				new_gui = new CWindowCropGUI(mwindow, this);
				break;
			case CWINDOW_CAMERA:
				new_gui = new CWindowCameraGUI(mwindow, this);
				break;
			case CWINDOW_PROJECTOR:
				new_gui = new CWindowProjectorGUI(mwindow, this);
				break;
			case CWINDOW_MASK:
				new_gui = new CWindowMaskGUI(mwindow, this);
				break;
			case CWINDOW_RULER:
				new_gui = new CWindowRulerGUI(mwindow, this);
				break;
			case CWINDOW_PROTECT:
				mwindow->edl->session->tool_window = 0;
				gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]->update(0);
				// fall thru
			default:
				result = 1;
				stop_tool();
				break;
		}

//printf("CWindowTool::start_tool 1\n");


		if(!result)
		{
			stop_tool();
// Wait for previous tool GUI to finish
			output_lock->lock("CWindowTool::start_tool");
			this->tool_gui = new_gui;
			tool_gui->create_objects();
			if( previous_tool == CWINDOW_PROTECT || previous_tool == CWINDOW_NONE ) {
				mwindow->edl->session->tool_window = 1;
				gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]->update(1);
			}
			mwindow->edl->session->tool_window = new_gui ? 1 : 0;
			update_show_window();

// Signal thread to run next tool GUI
			input_lock->unlock();
		}
//printf("CWindowTool::start_tool 1\n");
	}
	else
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::start_tool");
		tool_gui->update();
		tool_gui->unlock_window();
	}

//printf("CWindowTool::start_tool 2\n");

}


void CWindowTool::stop_tool()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::stop_tool");
		tool_gui->set_done(0);
		tool_gui->unlock_window();
	}
}

void CWindowTool::show_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->show_window();
		tool_gui->unlock_window();
	}
}

void CWindowTool::hide_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->hide_window();
		tool_gui->unlock_window();
	}
}

void CWindowTool::raise_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->raise_window();
		tool_gui->unlock_window();
	}
}


void CWindowTool::run()
{
	while(!done)
	{
		input_lock->lock("CWindowTool::run");
		if(!done)
		{
			tool_gui->run_window();
			tool_gui_lock->lock("CWindowTool::run");
			delete tool_gui;
			tool_gui = 0;
			tool_gui_lock->unlock();
		}
		output_lock->unlock();
	}
}

void CWindowTool::update_show_window()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_show_window");

		if(mwindow->edl->session->tool_window)
		{
			tool_gui->update();
			tool_gui->show_window();
		}
		else
			tool_gui->hide_window();
		tool_gui->flush();

		tool_gui->unlock_window();
	}
}

void CWindowTool::raise_window()
{
	if(tool_gui)
	{
		gui->unlock_window();
		tool_gui->lock_window("CWindowTool::raise_window");
		tool_gui->raise_window();
		tool_gui->unlock_window();
		gui->lock_window("CWindowTool::raise_window");
	}
}

void CWindowTool::update_values()
{
	tool_gui_lock->lock("CWindowTool::update_values");
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_values");
		tool_gui->update();
		tool_gui->flush();
		tool_gui->unlock_window();
	}
	tool_gui_lock->unlock();
}







CWindowToolGUI::CWindowToolGUI(MWindow *mwindow,
	CWindowTool *thread, const char *title, int w, int h)
 : BC_Window(title,
		mwindow->session->ctool_x, mwindow->session->ctool_y,
		w, h, w, h, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	current_operation = 0;
	span = 1;  edge = 0;
}

CWindowToolGUI::~CWindowToolGUI()
{
}

int CWindowToolGUI::close_event()
{
	hide_window();
	flush();
	mwindow->edl->session->tool_window = 0;
	unlock_window();
	thread->gui->lock_window("CWindowToolGUI::close_event");
	thread->gui->composite_panel->set_operation(mwindow->edl->session->cwindow_operation);
	thread->gui->flush();
	thread->gui->unlock_window();
	lock_window("CWindowToolGUI::close_event");
	return 1;
}

int CWindowToolGUI::keypress_event()
{
	int result = 0;

	switch( get_keypress() ) {
	case 'w':
	case 'W':
		return close_event();
	case KEY_F1:
	case KEY_F2:
	case KEY_F3:
	case KEY_F4:
	case KEY_F5:
	case KEY_F6:
	case KEY_F7:
	case KEY_F8:
	case KEY_F9:
	case KEY_F10:
	case KEY_F11:
	case KEY_F12:
		resend_event(thread->gui);
		result = 1;
	}

	if( result ) return result;
	return context_help_check_and_show();
}

int CWindowToolGUI::translation_event()
{
	mwindow->session->ctool_x = get_x();
	mwindow->session->ctool_y = get_y();
	return 0;
}

void CWindowToolGUI::update_auto(Track *track, int idx, CWindowCoord *vp)
{
	FloatAuto *float_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[idx], 1);
	if( !float_auto ) return;
	float v = float_auto->get_value(edge);
	float t = atof(vp->get_text());
	if( v == t ) return;
	float_auto->bump_value(t, edge, span);
	if( idx == AUTOMATION_PROJECTOR_Z || idx == AUTOMATION_CAMERA_Z ) {
		mwindow->gui->lock_window("CWindowToolGUI::update_auto");
		mwindow->gui->draw_overlays(1);
		mwindow->gui->unlock_window();
	}
	update();
	update_preview();
}

void CWindowToolGUI::update_preview(int changed_edl)
{
	unlock_window();
	draw_preview(changed_edl);
	lock_window("CWindowToolGUI::update_preview");
}

void CWindowToolGUI::draw_preview(int changed_edl)
{
	CWindowGUI *cgui = mwindow->cwindow->gui;
	cgui->lock_window("CWindowToolGUI::draw_preview");
	int change_type = !changed_edl ? CHANGE_PARAMS : CHANGE_EDL;
	cgui->sync_parameters(change_type, 0, 1);
	cgui->unlock_window();
}


CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, float value, int type)
 : BC_TumbleTextBox(gui, (float)value, (float)-65536, (float)65536, x, y, xS(70), 3)
{
	this->gui = gui;
	this->type = type;
	slider = 0;
	range = 0;
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value, int type)
 : BC_TumbleTextBox(gui, (int64_t)value, (int64_t)-65536, (int64_t)65536, x, y, xS(70))
{
	this->gui = gui;
	this->type = type;
	slider = 0;
	range = 0;
}

void CWindowCoord::create_objects()
{
	BC_TumbleTextBox::create_objects();
	if( type >= 0 ) {
		float v = atof(get_text());
		int xs10 = xS(10);
		int x1 = get_x() + BC_TumbleTextBox::get_w() + xs10, y1 = get_y();
		gui->add_subwindow(min_tumbler = new CWindowToolAutoRangeTumbler(this, x1, y1,
				0, _("Range min")));
		x1 += min_tumbler->get_w() + xs10;
		int group = Automation::autogrouptype(type, 0);
		float min = gui->mwindow->edl->local_session->automation_mins[group];
		float max = gui->mwindow->edl->local_session->automation_maxs[group];
		gui->add_subwindow(slider = new CWindowCoordSlider(this,
				x1, y1, xS(150), min, max, v));
		x1 += slider->get_w() + xS(10);
		gui->add_subwindow(max_tumbler = new CWindowToolAutoRangeTumbler(this, x1, y1,
				1, _("Range max")));
		x1 += max_tumbler->get_w() + xS(10);
		gui->add_subwindow(range_reset = new CWindowToolAutoRangeReset(this, x1, y1));
		x1 += range_reset->get_w() + xS(10);
		gui->add_subwindow(range_text = new CWindowToolAutoRangeTextBox(this, x1, y1));
		range_text->update_range();
		x1 += range_text->get_w() + xS(10);
		gui->add_subwindow(range = new CWindowCoordRangeTumbler(this, x1, y1));
	}
}

void CWindowCoord::update_gui(float value)
{
	BC_TumbleTextBox::update(value);
	if( slider ) {
		int group = Automation::autogrouptype(type, 0);
		LocalSession *local_session = gui->mwindow->edl->local_session;
		slider->update(slider->get_pointer_motion_range(), value,
			local_session->automation_mins[group],
			local_session->automation_maxs[group]);
		int x1 = range->get_x() + range->get_w() + xS(5);
		int y1 = range->get_y() + yS(5), d = xS(16);
		gui->set_color(GWindowGUI::auto_colors[type]);
		gui->draw_disc(x1, y1, d, d);
	}
}

int CWindowCoord::handle_event()
{
	if( slider )
		slider->update(atof(get_text()));
	gui->event_caller = this;
	gui->handle_event();
	return 1;
}

CWindowCoordSlider::CWindowCoordSlider(CWindowCoord *coord,
		int x, int y, int w, float mn, float mx, float value)
 : BC_FSlider(x, y, 0, w, w, mn, mx, value)
{
	this->coord = coord;
	set_precision(0.01);
}

CWindowCoordSlider::~CWindowCoordSlider()
{
}

int CWindowCoordSlider::handle_event()
{
	float value = get_value();
	coord->update(value);
	coord->gui->event_caller = coord;
	coord->gui->handle_event();
	return 1;
}

CWindowCoordRangeTumbler::CWindowCoordRangeTumbler(CWindowCoord *coord, int x, int y)
 : BC_Tumbler(x, y)
{
	this->coord = coord;
}
CWindowCoordRangeTumbler::~CWindowCoordRangeTumbler()
{
}

int CWindowCoordRangeTumbler::update(float scale)
{
	CWindowCoordSlider *slider = coord->slider;
	MWindow *mwindow = coord->gui->mwindow;
	LocalSession *local_session = mwindow->edl->local_session;
	int group = Automation::autogrouptype(coord->type, 0);
	float min = local_session->automation_mins[group];
	float max = local_session->automation_maxs[group];
	if( min >= max ) {
		switch( group ) {
		case AUTOGROUPTYPE_ZOOM: min = 0.005;  max = 5.0;   break;
		case AUTOGROUPTYPE_X:    min = -1000;  max = 1000;  break;
		case AUTOGROUPTYPE_Y:    min = -1000;  max = 1000;  break;
		}
	}
	switch( group ) {
	case AUTOGROUPTYPE_ZOOM: { // exp
		float lmin = log(min), lmax = log(max);
		float lr = (lmax - lmin) * scale;
		if( (min = exp(lmin - lr)) < ZOOM_MIN ) min = ZOOM_MIN;
		if( (max = exp(lmax + lr)) > ZOOM_MAX ) max = ZOOM_MAX;
		break; }
	case AUTOGROUPTYPE_X:
	case AUTOGROUPTYPE_Y: { // linear
		float dr = (max - min) * scale;
		if( (min -= dr) < XY_MIN ) min = XY_MIN;
		if( (max += dr) > XY_MAX ) max =  XY_MAX;
		break; }
	}
	slider->update(slider->get_pointer_motion_range(),
			slider->get_value(), min, max);
	unlock_window();
	MWindowGUI *mgui = mwindow->gui;
	mgui->lock_window("CWindowCoordRangeTumbler::update");
	local_session->zoombar_showautotype = group;
	local_session->automation_mins[group] = min;
	local_session->automation_maxs[group] = max;
	mgui->zoombar->update_autozoom();
	mgui->draw_overlays(0);
	mgui->update_patchbay();
	mgui->flash_canvas(1);
	mgui->unlock_window();
	lock_window("CWindowCoordRangeTumbler::update");
	return coord->range_text->update_range();
}

int CWindowCoordRangeTumbler::handle_up_event()
{
	return update(0.125);
}
int CWindowCoordRangeTumbler::handle_down_event()
{
	return update(-0.1);
}

CWindowCropApply::CWindowCropApply(MWindow *mwindow, CWindowCropGUI *crop_gui, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->mwindow = mwindow;
	this->crop_gui = crop_gui;
}
int CWindowCropApply::handle_event()
{
	mwindow->crop_video(crop_gui->crop_mode->mode);
	return 1;
}


int CWindowCropApply::keypress_event()
{
	if(get_keypress() == 0xd)
	{
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

const char *CWindowCropOpMode::crop_ops[] = {
	N_("Reformat"),
	N_("Resize"),
	N_("Shrink"),
};

CWindowCropOpMode::CWindowCropOpMode(MWindow *mwindow, CWindowCropGUI *crop_gui,
			int mode, int x, int y)
 : BC_PopupMenu(x, y, xS(140), _(crop_ops[mode]), 1)
{
	this->mwindow = mwindow;
	this->crop_gui = crop_gui;
	this->mode = mode;
}
CWindowCropOpMode::~CWindowCropOpMode()
{
}

void CWindowCropOpMode::create_objects()
{
	for( int id=0,nid=sizeof(crop_ops)/sizeof(crop_ops[0]); id<nid; ++id )
		add_item(new CWindowCropOpItem(this, _(crop_ops[id]), id));
	handle_event();
}

int CWindowCropOpMode::handle_event()
{
	set_text(_(crop_ops[mode]));
	return 1;
}

CWindowCropOpItem::CWindowCropOpItem(CWindowCropOpMode *popup, const char *text, int id)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->id = id;
}

int CWindowCropOpItem::handle_event()
{
	popup->set_text(get_text());
	popup->mode = id;
	return popup->handle_event();
}





CWindowCropGUI::CWindowCropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Crop"), xS(330), yS(100))
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Cropping");
}


CWindowCropGUI::~CWindowCropGUI()
{
}

void CWindowCropGUI::create_objects()
{
	int xs5 = xS(5), xs10 = xS(10);
	int ys5 = yS(5), ys10 = yS(10);
	int x = xs10, y = ys10;
	BC_Title *title;

	lock_window("CWindowCropGUI::create_objects");
	int column1 = 0;
	int pad = MAX(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1),
		BC_Title::calculate_h(this, "X")) + ys5;
	add_subwindow(title = new BC_Title(x, y, "X1:"));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("W:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(new CWindowCropApply(mwindow, this, x, y));

	x += column1 + xs5;
	y = ys10;
	x1 = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_x1);
	x1->create_objects();
	x1->set_boundaries((int64_t)0, (int64_t)65536);
	y += pad;
	width = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_x2 - mwindow->edl->session->crop_x1);
	width->create_objects();
	width->set_boundaries((int64_t)1, (int64_t)65536);


	x += x1->get_w() + xs10;
	y = ys10;
	int column2 = 0;
	add_subwindow(title = new BC_Title(x, y, "Y1:"));
	column2 = MAX(column2, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("H:")));
	column2 = MAX(column2, title->get_w());
	y += pad;

	y = ys10;
	x += column2 + xs5;
	y1 = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_y1);
	y1->create_objects();
	y1->set_boundaries((int64_t)0, (int64_t)65536);
	y += pad;

	height = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_y2 - mwindow->edl->session->crop_y1);
	height->create_objects();
	height->set_boundaries((int64_t)1, (int64_t)65536);
	y += pad;

	add_subwindow(crop_mode = new CWindowCropOpMode(mwindow, this,
				CROP_REFORMAT, x, y));
	crop_mode->create_objects();

	unlock_window();
}

void CWindowCropGUI::handle_event()
{
	int new_x1, new_y1;
	new_x1 = atol(x1->get_text());
	new_y1 = atol(y1->get_text());
	if(new_x1 != mwindow->edl->session->crop_x1)
	{
		mwindow->edl->session->crop_x2 = new_x1 +
			mwindow->edl->session->crop_x2 -
			mwindow->edl->session->crop_x1;
		mwindow->edl->session->crop_x1 = new_x1;
	}
	if(new_y1 != mwindow->edl->session->crop_y1)
	{
		mwindow->edl->session->crop_y2 = new_y1 +
			mwindow->edl->session->crop_y2 -
			mwindow->edl->session->crop_y1;
		mwindow->edl->session->crop_y1 = atol(y1->get_text());
	}
	mwindow->edl->session->crop_x2 = atol(width->get_text()) +
		mwindow->edl->session->crop_x1;
	mwindow->edl->session->crop_y2 = atol(height->get_text()) +
		mwindow->edl->session->crop_y1;
	update();
	mwindow->cwindow->gui->canvas->redraw(1);
}

void CWindowCropGUI::update()
{
	x1->update((int64_t)mwindow->edl->session->crop_x1);
	y1->update((int64_t)mwindow->edl->session->crop_y1);
	width->update((int64_t)mwindow->edl->session->crop_x2 -
		mwindow->edl->session->crop_x1);
	height->update((int64_t)mwindow->edl->session->crop_y2 -
		mwindow->edl->session->crop_y1);
}


CWindowEyedropGUI::CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Color"), xS(220), yS(290))
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Compositor Toolbar");
}

CWindowEyedropGUI::~CWindowEyedropGUI()
{
}

void CWindowEyedropGUI::create_objects()
{
	int xs10 = xS(10), ys10 = yS(10);
	int margin = mwindow->theme->widget_border;
	int x = xs10 + margin;
	int y = ys10 + margin;
	int x2 = xS(70), x3 = x2 + xS(60);
	lock_window("CWindowEyedropGUI::create_objects");
	BC_Title *title0, *title1, *title2, *title3, *title4, *title5, *title6, *title7;
	add_subwindow(title0 = new BC_Title(x, y,_("X,Y:")));
	y += title0->get_h() + margin;
	add_subwindow(title7 = new BC_Title(x, y, _("Radius:")));
	y += BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Red:")));
	y += title1->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("Green:")));
	y += title2->get_h() + margin;
	add_subwindow(title3 = new BC_Title(x, y, _("Blue:")));
	y += title3->get_h() + margin;

	add_subwindow(title4 = new BC_Title(x, y, "Y:"));
	y += title4->get_h() + margin;
	add_subwindow(title5 = new BC_Title(x, y, "U:"));
	y += title5->get_h() + margin;
	add_subwindow(title6 = new BC_Title(x, y, "V:"));

	add_subwindow(current = new BC_Title(x2, title0->get_y(), ""));

	radius = new CWindowCoord(this, x2, title7->get_y(),
		mwindow->edl->session->eyedrop_radius);
	radius->create_objects();
	radius->set_boundaries((int64_t)0, (int64_t)255);

	add_subwindow(red = new BC_Title(x2, title1->get_y(), "0"));
	add_subwindow(green = new BC_Title(x2, title2->get_y(), "0"));
	add_subwindow(blue = new BC_Title(x2, title3->get_y(), "0"));
	add_subwindow(rgb_hex = new BC_Title(x3, red->get_y(), "#000000"));

	add_subwindow(this->y = new BC_Title(x2, title4->get_y(), "0"));
	add_subwindow(this->u = new BC_Title(x2, title5->get_y(), "0"));
	add_subwindow(this->v = new BC_Title(x2, title6->get_y(), "0"));
	add_subwindow(yuv_hex = new BC_Title(x3, this->y->get_y(), "#000000"));

	y = title6->get_y() + this->v->get_h() + 2*margin;
	add_subwindow(sample = new BC_SubWindow(x, y, xS(50), yS(50)));
	y += sample->get_h() + margin;
	add_subwindow(use_max = new CWindowEyedropCheckBox(mwindow, this, x, y));
	update();
	unlock_window();
}

void CWindowEyedropGUI::update()
{
	char string[BCTEXTLEN];
	sprintf(string, "%d, %d",
		thread->gui->eyedrop_x,
		thread->gui->eyedrop_y);
	current->update(string);

	radius->update((int64_t)mwindow->edl->session->eyedrop_radius);

	LocalSession *local_session = mwindow->edl->local_session;
	int use_max = local_session->use_max;
	float r = use_max ? local_session->red_max : local_session->red;
	float g = use_max ? local_session->green_max : local_session->green;
	float b = use_max ? local_session->blue_max : local_session->blue;
	this->red->update(r);
	this->green->update(g);
	this->blue->update(b);

	int rx = 255*r + 0.5;  bclamp(rx,0,255);
	int gx = 255*g + 0.5;  bclamp(gx,0,255);
	int bx = 255*b + 0.5;  bclamp(bx,0,255);
	char rgb_text[BCSTRLEN];
	sprintf(rgb_text, "#%02x%02x%02x", rx, gx, bx);
	rgb_hex->update(rgb_text);
	
	float y, u, v;
	YUV::yuv.rgb_to_yuv_f(r, g, b, y, u, v);
	this->y->update(y);
	this->u->update(u += 0.5);
	this->v->update(v += 0.5);

	int yx = 255*y + 0.5;  bclamp(yx,0,255);
	int ux = 255*u + 0.5;  bclamp(ux,0,255);
	int vx = 255*v + 0.5;  bclamp(vx,0,255);
	char yuv_text[BCSTRLEN];
	sprintf(yuv_text, "#%02x%02x%02x", yx, ux, vx);
	yuv_hex->update(yuv_text);

	int rgb = (rx << 16) | (gx << 8) | (bx << 0);
	sample->set_color(rgb);
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}

void CWindowEyedropGUI::handle_event()
{
	int new_radius = atoi(radius->get_text());
	if(new_radius != mwindow->edl->session->eyedrop_radius)
	{
		CWindowGUI *gui = mwindow->cwindow->gui;
		if(gui->eyedrop_visible)
		{
			gui->lock_window("CWindowEyedropGUI::handle_event");
// hide it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
		}

		mwindow->edl->session->eyedrop_radius = new_radius;

		if(gui->eyedrop_visible)
		{
// draw it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
			gui->unlock_window();
		}
	}
}



/* Buttons to control Keyframe-Curve-Mode for Projector or Camera */

// Configuration for all possible Keyframe Curve Mode toggles
struct _CVD {
	FloatAuto::t_mode mode;
	bool use_camera;
	const char* icon_id;
	const char* tooltip;
};

const _CVD Camera_Crv_Smooth = { FloatAuto::SMOOTH, true, "tan_smooth",
		N_("\"smooth\" Curve on current Camera Keyframes") };
const _CVD Camera_Crv_Linear = { FloatAuto::LINEAR, true, "tan_linear",
		N_("\"linear\" Curve on current Camera Keyframes") };
const _CVD Camera_Crv_Tangent = { FloatAuto::TFREE, true, "tan_tangent",
		N_("\"tangent\" Curve on current Camera Keyframes") };
const _CVD Camera_Crv_Free  = { FloatAuto::FREE, true, "tan_free",
		N_("\"free\" Curve on current Camera Keyframes") };
const _CVD Camera_Crv_Bump = { FloatAuto::BUMP, true, "tan_bump",
		N_("\"bump\" Curve on current Camera Keyframes") };

const _CVD Projector_Crv_Smooth = { FloatAuto::SMOOTH, false, "tan_smooth",
		N_("\"smooth\" Curve on current Projector Keyframes") };
const _CVD Projector_Crv_Linear = { FloatAuto::LINEAR, false, "tan_linear",
		N_("\"linear\" Curve on current Projector Keyframes") };
const _CVD Projector_Crv_Tangent = { FloatAuto::TFREE, false, "tan_tangent",
		N_("\"tangent\" Curve on current Projector Keyframes") };
const _CVD Projector_Crv_Free  = { FloatAuto::FREE, false, "tan_free",
		N_("\"free\" Curve on current Projector Keyframes") };
const _CVD Projector_Crv_Bump = { FloatAuto::BUMP, false, "tan_bump",
		N_("\"bump\" Curve on current Projector Keyframes") };

// Implementation Class für Keyframe Curve Mode buttons
//
// This button reflects the state of the "current" keyframe
// (the nearest keyframe on the left) for all three automation
// lines together. Clicking on this button (re)sets the curve
// mode for the three "current" keyframes simultanously, but
// never creates a new keyframe.
//
class CWindowCurveToggle : public BC_Toggle
{
public:
	CWindowCurveToggle(const _CVD &mode,
			MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	void check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z);
	int handle_event();
private:
	const _CVD &cfg;
	MWindow *mwindow;
	CWindowToolGUI *gui;
};


CWindowCurveToggle::CWindowCurveToggle(const _CVD &mode,
			MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set(mode.icon_id), false),
   cfg(mode)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_(cfg.tooltip));
}

void CWindowCurveToggle::check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z)
{
// the toggle state is only set to ON if all
// three automation lines have the same curve mode.
// For mixed states the toggle stays off.
	set_value( x->curve_mode == this->cfg.mode &&
		   y->curve_mode == this->cfg.mode &&
		   z->curve_mode == this->cfg.mode
		   ,true // redraw to show new state
		);
}

int CWindowCurveToggle::handle_event()
{
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track) {
		FloatAuto *x=0, *y=0, *z=0;
		mwindow->cwindow->calculate_affected_autos(track,
			&x, &y, &z, cfg.use_camera, 0,0,0); // don't create new keyframe
		if( x ) x->change_curve_mode(cfg.mode);
		if( y ) y->change_curve_mode(cfg.mode);
		if( z ) z->change_curve_mode(cfg.mode);

		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowEyedropCheckBox::CWindowEyedropCheckBox(MWindow *mwindow,
	CWindowEyedropGUI *gui, int x, int y)
 : BC_CheckBox(x, y, mwindow->edl->local_session->use_max, _("Use maximum"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowEyedropCheckBox::handle_event()
{
	mwindow->edl->local_session->use_max = get_value();
	
	gui->update();
	return 1;
}


CWindowCameraGUI::CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Camera"), xS(580), yS(200))
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Camera and Projector");
}
CWindowCameraGUI::~CWindowCameraGUI()
{
}

void CWindowCameraGUI::create_objects()
{
	int xs5 = xS(5), xs10 = xS(10), xs15 = xS(15), xs25 = xS(25);
	int ys10 = yS(10), ys30 = yS(30);
	int x = xs10, y = ys10;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0, *y_auto = 0, *z_auto = 0;
	BC_Title *title;
	BC_Button *button;
	span = 1;  edge = 0;

	lock_window("CWindowCameraGUI::create_objects");
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 1, 0, 0, 0);
	}
	int x1 = x;
	add_subwindow(bar1 = new BC_TitleBar(x1, y, xS(340), xs10, xs10, _("Position")));
	x1 += bar1->get_w() + xS(35);
	add_subwindow(bar2 = new BC_TitleBar(x1, y, get_w()-x1-xs10, xs10, xs10, _("Range")));
	y += bar1->get_h() + ys10;

	add_subwindow(title = new BC_Title(x, y, "X:"));
	x1 = x + title->get_w() + xS(3);
	float xvalue = x_auto ? x_auto->get_value() : 0;
	this->x = new CWindowCoord(this, x1, y, xvalue, AUTOMATION_CAMERA_X);
	this->x->create_objects();
	this->x->range->set_tooltip(_("expand X range"));
	y += ys30;
	add_subwindow(title = new BC_Title(x = xs10, y, "Y:"));
	float yvalue = y_auto ? y_auto->get_value() : 0;
	this->y = new CWindowCoord(this, x1, y, yvalue, AUTOMATION_CAMERA_Y);
	this->y->create_objects();
	this->y->range->set_tooltip(_("expand Y range"));
	y += ys30;
	add_subwindow(title = new BC_Title(x = xs10, y, "Z:"));
	float zvalue = z_auto ? z_auto->get_value() : 1;
	this->z = new CWindowCoord(this, x1, y, zvalue, AUTOMATION_CAMERA_Z);
	this->z->create_objects();
	this->z->set_increment(0.01);
	this->z->range->set_tooltip(_("expand Zoom range"));
	y += ys30 + ys10;

	x1 = x;
	add_subwindow(bar3 = new BC_TitleBar(x1, y, xS(180)-x1, xs5, xs5, _("Justify")));
	x1 += bar3->get_w() + xS(35);
	add_subwindow(bar4 = new BC_TitleBar(x1, y, xS(375)-x1, xs5, xs5, _("Curve type")));
	bar4->context_help_set_keyword("Using Autos");
	x1 += bar4->get_w() + xS(25);
	add_subwindow(bar5 = new BC_TitleBar(x1, y, get_w()-xS(60)-x1, xs5, xs5, _("Keyframe")));
	bar5->context_help_set_keyword("Using Autos");
	y += bar3->get_h() + ys10;

	x1 = x;
	add_subwindow(button = new CWindowCameraLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraRight(mwindow, this, x1, y));
	x1 += button->get_w() + xs25;
	add_subwindow(button = new CWindowCameraTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraBottom(mwindow, this, x1, y));
	x1 += button->get_w() + xS(35);
	add_subwindow(t_smooth = new CWindowCurveToggle(Camera_Crv_Smooth, mwindow, this, x1, y));
	t_smooth->context_help_set_keyword("Using Autos");
	x1 += t_smooth->get_w() + xs10;
	add_subwindow(t_linear = new CWindowCurveToggle(Camera_Crv_Linear, mwindow, this, x1, y));
	t_linear->context_help_set_keyword("Using Autos");
	x1 += t_linear->get_w() + xs10;
	add_subwindow(t_tangent = new CWindowCurveToggle(Camera_Crv_Tangent, mwindow, this, x1, y));
	t_tangent->context_help_set_keyword("Using Autos");
	x1 += t_tangent->get_w() + xs10;
	add_subwindow(t_free = new CWindowCurveToggle(Camera_Crv_Free, mwindow, this, x1, y));
	t_free->context_help_set_keyword("Using Autos");
	x1 += t_free->get_w() + xs10;
	add_subwindow(t_bump = new CWindowCurveToggle(Camera_Crv_Bump, mwindow, this, x1, y));
	t_bump->context_help_set_keyword("Bump autos");
	x1 += button->get_w() + xs25;
	y += yS(5);
	add_subwindow(add_keyframe = new CWindowCameraAddKeyframe(mwindow, this, x1, y));
	add_keyframe->context_help_set_keyword("Using Autos");
	x1 += add_keyframe->get_w() + xs15;
	add_subwindow(auto_edge = new CWindowCurveAutoEdge(mwindow, this, x1, y));
	auto_edge->context_help_set_keyword("Bump autos");
	x1 += auto_edge->get_w() + xs10;
	add_subwindow(auto_span = new CWindowCurveAutoSpan(mwindow, this, x1, y));
	auto_span->context_help_set_keyword("Bump autos");
	x1 += auto_span->get_w() + xS(50);
	add_subwindow(reset = new CWindowCameraReset(mwindow, this, x1, y));

// fill in current auto keyframe values, set toggle states.
	this->update();
	unlock_window();
}

void CWindowCameraGUI::handle_event()
{
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( !track ) return;
	mwindow->undo->update_undo_before(_("camera"), this);
	if( event_caller == x )
		update_auto(track, AUTOMATION_CAMERA_X, x);
	else if( event_caller == y )
		update_auto(track, AUTOMATION_CAMERA_Y, y);
	else if( event_caller == z )
		update_auto(track, AUTOMATION_CAMERA_Z, z);
	mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
}

void CWindowCameraGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	int bg_color = get_resources()->text_background;
	int hi_color = bg_color ^ 0x444444;
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 1, 0, 0, 0);
	}

	if( x_auto ) {
		int color = (edge || span) && x_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		x->get_textbox()->set_back_color(color);
		float xvalue = x_auto->get_value(edge);
		x->update_gui(xvalue);
	}
	if( y_auto ) {
		int color = (edge || span) && y_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		y->get_textbox()->set_back_color(color);
		float yvalue = y_auto->get_value(edge);
		y->update_gui(yvalue);
	}
	if( z_auto ) {
		int color = (edge || span) && z_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		z->get_textbox()->set_back_color(color);
		float zvalue = z_auto->get_value(edge);
		z->update_gui(zvalue);
		thread->gui->lock_window("CWindowCameraGUI::update");
		thread->gui->composite_panel->cpanel_zoom->update(zvalue);
		thread->gui->unlock_window();
	}

	if( x_auto && y_auto && z_auto ) {
		t_smooth->check_toggle_state(x_auto, y_auto, z_auto);
		t_linear->check_toggle_state(x_auto, y_auto, z_auto);
		t_tangent->check_toggle_state(x_auto, y_auto, z_auto);
		t_free->check_toggle_state(x_auto, y_auto, z_auto);
		t_bump->check_toggle_state(x_auto, y_auto, z_auto);
	}
	x->range_text->update_range();
	y->range_text->update_range();
	z->range_text->update_range();
}

CWindowToolAutoRangeTumbler::CWindowToolAutoRangeTumbler(CWindowCoord *coord, int x, int y,
		int use_max, const char *tip)
 : BC_Tumbler(x, y, coord->gui->mwindow->theme->get_image_set("auto_range"),
		TUMBLER_HORZ)
{
	this->coord = coord;
	this->use_max = use_max;
	set_tooltip(tip);
}

int CWindowToolAutoRangeTumbler::handle_up_event()
{
	coord->gui->mwindow->update_autorange(coord->type, 1, use_max);
	coord->range_text->update_range();
	return 1;
}

int CWindowToolAutoRangeTumbler::handle_down_event()
{
	coord->gui->mwindow->update_autorange(coord->type, 0, use_max);
	coord->range_text->update_range();
	return 1;
}

CWindowToolAutoRangeReset::CWindowToolAutoRangeReset(CWindowCoord *coord, int x, int y)
 : BC_Button(x, y, coord->gui->mwindow->theme->get_image_set("reset_button"))
{
	this->coord = coord;
	set_tooltip(_("Reset"));
}

int CWindowToolAutoRangeReset::handle_event()
{
	float v = 0;
	int group = Automation::autogrouptype(coord->type, 0);
	MWindow *mwindow = coord->gui->mwindow;
	LocalSession *local_session = mwindow->edl->local_session;
	float min = local_session->automation_mins[group];
	float max = local_session->automation_maxs[group];
	switch( group ) {
	case AUTOGROUPTYPE_ZOOM: // exp
		min = 0.005;  max= 5.000;  v = 1;
		break;
	case AUTOGROUPTYPE_X:
		max = mwindow->edl->session->output_w;
		min = -max;
		break;
	case AUTOGROUPTYPE_Y:
		max = mwindow->edl->session->output_h;
		min = -max;
		break;
	}
	local_session->automation_mins[group] = min;
	local_session->automation_maxs[group] = max;
	coord->range_text->update_range();
	unlock_window();
	MWindowGUI *mgui = mwindow->gui;
	mgui->lock_window("CWindowToolAutoRangeReset::update");
	int color = GWindowGUI::auto_colors[coord->type];
	mgui->zoombar->update_autozoom(group, color);
	mgui->draw_overlays(0);
	mgui->update_patchbay();
	mgui->flash_canvas(1);
	mgui->unlock_window();
	mwindow->save_backup();
	lock_window("CWindowToolAutoRangeReset::update");
	CWindowCoordSlider *slider = coord->slider;
	slider->update(slider->get_pointer_motion_range(), v, min, max);
	return slider->handle_event();
}

CWindowToolAutoRangeTextBox::CWindowToolAutoRangeTextBox(CWindowCoord *coord, int x, int y)
 : BC_TextBox(x, y, xS(130), 1, "0.000 to 0.000")
{
        this->coord = coord;
        set_tooltip(_("Automation range"));
}

int CWindowToolAutoRangeTextBox::button_press_event()
{
        if (!is_event_win()) return 0;
	int use_max = get_cursor_x() < get_w()/2 ? 0 : 1;
        switch( get_buttonpress() ) {
	case WHEEL_UP:
		coord->gui->mwindow->update_autorange(coord->type, 1, use_max);
		break;
	case WHEEL_DOWN:
		coord->gui->mwindow->update_autorange(coord->type, 0, use_max);
		break;
	default:
		return BC_TextBox::button_press_event();
        }
	return coord->range_text->update_range();
}

int CWindowToolAutoRangeTextBox::handle_event()
{
        float imin, imax;
        if( sscanf(this->get_text(),"%f to%f",&imin, &imax) == 2 ) {
		MWindow *mwindow = coord->gui->mwindow;
		int group = Automation::autogrouptype(coord->type, 0);
		LocalSession *local_session = mwindow->edl->local_session;
		float min = imin, max = imax;
		switch( group ) {
		case AUTOGROUPTYPE_ZOOM:
			if( min < ZOOM_MIN ) min = ZOOM_MIN;
			if( max > ZOOM_MAX ) max = ZOOM_MAX;
			break;
		case AUTOGROUPTYPE_X:
		case AUTOGROUPTYPE_Y:
			if( min < XY_MIN ) min = XY_MIN;
			if( max > XY_MAX ) max = XY_MAX;
			break;
		}
                if( max > min ) {
			local_session->automation_mins[group] = min;
			local_session->automation_maxs[group] = max;
			if( min != imin || max != imax ) update_range();
			mwindow->gui->lock_window("CWindowToolAutoRangeTextBox::handle_event");
			int color = GWindowGUI::auto_colors[coord->type];
			mwindow->gui->zoombar->update_autozoom(group, color);
			mwindow->gui->draw_overlays(0);
			mwindow->gui->update_patchbay();
			mwindow->gui->flash_canvas(1);
			mwindow->gui->unlock_window();
		}
	}
	return 1;
}

int CWindowToolAutoRangeTextBox::update_range()
{
	char string[BCSTRLEN];
	LocalSession *local_session = coord->gui->mwindow->edl->local_session;
	int group = Automation::autogrouptype(coord->type, 0);
	float min = local_session->automation_mins[group];
	float max = local_session->automation_maxs[group];
	switch( group ) {
	case AUTOGROUPTYPE_ZOOM:
		sprintf(string, "%0.03f to %0.03f\n", min, max);
		break;
	case AUTOGROUPTYPE_X:
	case AUTOGROUPTYPE_Y:
		sprintf(string, "%0.0f to %.0f\n", min, max);
		break;
	}
	update(string);
	return 1;
}


CWindowCameraLeft::CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowCameraLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 1, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w, h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->set_value(
				(double)track->track_w / z_auto->get_value() / 2 -
				(double)w / 2);
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraCenter::CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowCameraCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_X], 1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		x_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraRight::CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowCameraRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 1, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w, h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->set_value( -((double)track->track_w / z_auto->get_value() / 2 -
				(double)w / 2));
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraTop::CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowCameraTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 1, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w, h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->set_value((double)track->track_h / z_auto->get_value() / 2 -
				(double)h / 2);
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraMiddle::CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowCameraMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		y_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraBottom::CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowCameraBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 1, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w, h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->set_value(-((double)track->track_h / z_auto->get_value() / 2 -
				(double)h / 2));
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}

CWindowCameraAddKeyframe::CWindowCameraAddKeyframe(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("keyframe_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Add Keyframe: Shift-F11"));
}

int CWindowCameraAddKeyframe::handle_event()
{
	return gui->press(&CWindowCanvas::camera_keyframe);
}

CWindowCameraReset::CWindowCameraReset(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("reset_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Reset Camera: F11"));
}

int CWindowCameraReset::handle_event()
{
	mwindow->edl->local_session->reset_view_limits();
	CWindowCameraGUI *gui = (CWindowCameraGUI *)this->gui;
	return gui->press(&CWindowCanvas::reset_camera);
}

CWindowCurveAutoEdge::CWindowCurveAutoEdge(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("bump_edge"), gui->edge)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Bump edit edge left/right"));
}

int CWindowCurveAutoEdge::handle_event()
{
	gui->edge = get_value();
	gui->update();
	return 1;
}

CWindowCurveAutoSpan::CWindowCurveAutoSpan(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("bump_span"), gui->span)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Bump spans to next/prev"));
}

int CWindowCurveAutoSpan::handle_event()
{
	gui->span = get_value();
	gui->update();
	return 1;
}


CWindowProjectorGUI::CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Projector"), xS(580), yS(200))
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Camera and Projector");
}
CWindowProjectorGUI::~CWindowProjectorGUI()
{
}
void CWindowProjectorGUI::create_objects()
{
	int xs5 = xS(5), xs10 = xS(10), xs15 = xS(15), xs25 = xS(25);
	int ys10 = yS(10), ys30 = yS(30);
	int x = xs10, y = ys10;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;
	span = 1;  edge = 0;

	lock_window("CWindowProjectorGUI::create_objects");
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 0, 0, 0, 0);
	}
	int x1 = x;
	add_subwindow(bar1 = new BC_TitleBar(x1, y, xS(340), xs10, xs10, _("Position")));
	x1 += bar1->get_w() + xS(35);
	add_subwindow(bar2 = new BC_TitleBar(x1, y, get_w()-x1-xs10, xs10, xs10, _("Range")));
	y += bar1->get_h() + ys10;
	add_subwindow(title = new BC_Title(x = xs10, y, "X:"));
	x1 = x + title->get_w() + xS(3);
	float xvalue = x_auto ? x_auto->get_value() : 0;
	this->x = new CWindowCoord(this, x1, y, xvalue, AUTOMATION_PROJECTOR_X);
	this->x->create_objects();
	this->x->range->set_tooltip(_("expand X range"));
	y += ys30;
	add_subwindow(title = new BC_Title(x = xs10, y, "Y:"));
	float yvalue = y_auto ? y_auto->get_value() : 0;
	this->y = new CWindowCoord(this, x1, y, yvalue, AUTOMATION_PROJECTOR_Y);
	this->y->create_objects();
	this->y->range->set_tooltip(_("expand Y range"));
	y += ys30;
	add_subwindow(title = new BC_Title(x = xs10, y, "Z:"));
	float zvalue = z_auto ? z_auto->get_value() : 1;
	this->z = new CWindowCoord(this, x1, y, zvalue, AUTOMATION_PROJECTOR_Z);
	this->z->create_objects();
	this->z->range->set_tooltip(_("expand Zoom range"));
	this->z->set_increment(0.01);
	y += ys30 + ys10;

	x1 = x;
	add_subwindow(bar3 = new BC_TitleBar(x1, y, xS(180)-x1, xs5, xs5, _("Justify")));
	x1 += bar3->get_w() + xS(35);
	add_subwindow(bar4 = new BC_TitleBar(x1, y, xS(375)-x1, xs5, xs5, _("Curve type")));
	bar4->context_help_set_keyword("Using Autos");
	x1 += bar4->get_w() + xS(25);
	add_subwindow(bar5 = new BC_TitleBar(x1, y, get_w()-xS(60)-x1, xs5, xs5, _("Keyframe")));
	bar5->context_help_set_keyword("Using Autos");
	y += bar3->get_h() + ys10;

	x1 = x;
	add_subwindow(button = new CWindowProjectorLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorRight(mwindow, this, x1, y));
	x1 += button->get_w() + xs25;
	add_subwindow(button = new CWindowProjectorTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorBottom(mwindow, this, x1, y));
	x1 += button->get_w() + xS(35);
	add_subwindow(t_smooth = new CWindowCurveToggle(Projector_Crv_Smooth, mwindow, this, x1, y));
	t_smooth->context_help_set_keyword("Using Autos");
	x1 += t_smooth->get_w() + xs10;
	add_subwindow(t_linear = new CWindowCurveToggle(Projector_Crv_Linear, mwindow, this, x1, y));
	t_linear->context_help_set_keyword("Using Autos");
	x1 += t_linear->get_w() + xs10;
	add_subwindow(t_tangent = new CWindowCurveToggle(Projector_Crv_Tangent, mwindow, this, x1, y));
	t_tangent->context_help_set_keyword("Using Autos");
	x1 += t_tangent->get_w() + xs10;
	add_subwindow(t_free = new CWindowCurveToggle(Projector_Crv_Free, mwindow, this, x1, y));
	t_free->context_help_set_keyword("Using Autos");
	x1 += t_free->get_w() + xs10;
	add_subwindow(t_bump = new CWindowCurveToggle(Projector_Crv_Bump, mwindow, this, x1, y));
	t_bump->context_help_set_keyword("Bump autos");
	x1 += button->get_w() + xs25;
	y += yS(5);
	add_subwindow(add_keyframe = new CWindowProjectorAddKeyframe(mwindow, this, x1, y));
	add_keyframe->context_help_set_keyword("Using Autos");
	x1 += add_keyframe->get_w() + xs15;
	add_subwindow(auto_edge = new CWindowCurveAutoEdge(mwindow, this, x1, y));
	auto_edge->context_help_set_keyword("Bump autos");
	x1 += auto_edge->get_w() + xs10;
	add_subwindow(auto_span = new CWindowCurveAutoSpan(mwindow, this, x1, y));
	auto_span->context_help_set_keyword("Bump autos");
	x1 += auto_span->get_w() + xS(50);
	add_subwindow(reset = new CWindowProjectorReset(mwindow, this, x1, y));

// fill in current auto keyframe values, set toggle states.
	this->update();
	unlock_window();
}

void CWindowProjectorGUI::handle_event()
{
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( !track ) return;
	mwindow->undo->update_undo_before(_("projector"), this);
	if( event_caller == x )
		update_auto(track, AUTOMATION_PROJECTOR_X, x);
	else if( event_caller == y )
		update_auto(track, AUTOMATION_PROJECTOR_Y, y);
	else if( event_caller == z )
		update_auto(track, AUTOMATION_PROJECTOR_Z, z);
	mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
}

void CWindowProjectorGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	int bg_color = get_resources()->text_background;
	int hi_color = bg_color ^ 0x444444;
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 0, 0, 0, 0);
	}

	if( x_auto ) {
		int color = (edge || span) && x_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		x->get_textbox()->set_back_color(color);
		float xvalue = x_auto->get_value(edge);
		x->update_gui(xvalue);
	}
	if( y_auto ) {
		int color = (edge || span) && y_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		y->get_textbox()->set_back_color(color);
		float yvalue = y_auto->get_value(edge);
		y->update_gui(yvalue);
	}
	if( z_auto ) {
		int color = (edge || span) && z_auto->curve_mode == FloatAuto::BUMP ?
			hi_color : bg_color;
		z->get_textbox()->set_back_color(color);
		float zvalue = z_auto->get_value(edge);
		z->update_gui(zvalue);
		thread->gui->lock_window("CWindowProjectorGUI::update");
		thread->gui->composite_panel->cpanel_zoom->update(zvalue);
		thread->gui->unlock_window();
	}

	if( x_auto && y_auto && z_auto ) {
		t_smooth->check_toggle_state(x_auto, y_auto, z_auto);
		t_linear->check_toggle_state(x_auto, y_auto, z_auto);
		t_tangent->check_toggle_state(x_auto, y_auto, z_auto);
		t_free->check_toggle_state(x_auto, y_auto, z_auto);
		t_bump->check_toggle_state(x_auto, y_auto, z_auto);
	}
	x->range_text->update_range();
	y->range_text->update_range();
	z->range_text->update_range();
}

CWindowProjectorLeft::CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowProjectorLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 0, 1, 0, 0);
	}
	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value( (double)track->track_w * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_w / 2 );
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorCenter::CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowProjectorCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_X], 1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorRight::CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowProjectorRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 0, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value( -((double)track->track_w * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_w / 2));
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorTop::CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowProjectorTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 0, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value( (double)track->track_h * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_h / 2 );
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorMiddle::CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowProjectorMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorBottom::CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowProjectorBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 0, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value( -((double)track->track_h * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_h / 2));
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}

CWindowProjectorAddKeyframe::CWindowProjectorAddKeyframe(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("keyframe_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Add Keyframe: Shift-F12"));
}

int CWindowProjectorAddKeyframe::handle_event()
{
	return gui->press(&CWindowCanvas::projector_keyframe);
}

CWindowProjectorReset::CWindowProjectorReset(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("reset_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Reset Projector: F12"));
}

int CWindowProjectorReset::handle_event()
{
	mwindow->edl->local_session->reset_view_limits();
	CWindowProjectorGUI *gui = (CWindowProjectorGUI *)this->gui;
	return gui->press(&CWindowCanvas::reset_projector);
}

int CWindowToolGUI::press(void (CWindowCanvas::*fn)())
{
	unlock_window();
	CWindowGUI *cw_gui = thread->gui;
	cw_gui->lock_window("CWindowGUI::press");
	(cw_gui->canvas->*fn)();
	cw_gui->unlock_window();
	lock_window("CWindowToolGUI::press");
	return 1;
}

CWindowMaskOnTrack::CWindowMaskOnTrack(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, int w, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, w, yS(120))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskOnTrack::~CWindowMaskOnTrack()
{
}

int CWindowMaskOnTrack::handle_event()
{
	CWindowMaskItem *track_item = 0;
	int k = get_number(), track_id = -1;
//printf("selected %d = %s\n", k, k<0 ? "()" : track_items[k]->get_text());
	if( k >= 0 ) {
		track_item = (CWindowMaskItem *)track_items[k];
		Track *track = track_item ? mwindow->edl->tracks->get_track_by_id(track_item->id) : 0;
		if( track && track->is_armed() ) track_id = track->get_id();
	}
	else
		track_id = mwindow->cwindow->mask_track_id;
	set_back_color(track_id >= 0 ?
		gui->get_resources()->text_background :
		gui->get_resources()->text_background_disarmed);
	if( mwindow->cwindow->mask_track_id != track_id )
		gui->mask_on_track->update(track_item ? track_item->get_text() : "");
	mwindow->cwindow->mask_track_id = track_id;
	mwindow->edl->local_session->solo_track_id = -1;
	gui->mask_solo_track->update(0);
	gui->update();
	gui->update_preview(1);
	return 1;
}

void CWindowMaskOnTrack::update_items()
{
	track_items.remove_all_objects();
	int high_color = gui->get_resources()->button_highlighted;
	for( Track *track=mwindow->edl->tracks->first; track; track=track->next ) {
		if( track->data_type != TRACK_VIDEO ) continue;
		MaskAutos *mask_autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		int color = !track->is_armed() ? RED : mask_autos->first ?  high_color : -1;
		MaskAuto *mask_auto = (MaskAuto*)mask_autos->default_auto;
		for( int i=0; color<0 && i<mask_auto->masks.size(); ++i )
			if( mask_auto->masks[i]->points.size() > 0 ) color = high_color;
		track_items.append(new CWindowMaskItem(track->title, track->get_id(), color));
	}
	update_list(&track_items);
}

CWindowMaskTrackTumbler::CWindowMaskTrackTumbler(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y)
 : BC_Tumbler(x, y)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskTrackTumbler::~CWindowMaskTrackTumbler()
{
}

int CWindowMaskTrackTumbler::handle_up_event()
{
	return do_event(1);
}

int CWindowMaskTrackTumbler::handle_down_event()
{
	return do_event(-1);
}

int CWindowMaskTrackTumbler::do_event(int dir)
{
	CWindowMaskItem *track_item = 0;
	CWindowMaskItem **items = (CWindowMaskItem**)&gui->mask_on_track->track_items[0];
	int n = gui->mask_on_track->track_items.size();
	int id = mwindow->cwindow->mask_track_id;
	if( n > 0 ) {	
		int k = n;
		while( --k >= 0 && items[k]->id != id );
		if( k >= 0 ) {
			k += dir;
			bclamp(k, 0, n-1);
			track_item = items[k];
		}
		else
			track_item = items[0];
	}
	Track *track = track_item ? mwindow->edl->tracks->get_track_by_id(track_item->id) : 0;
	int track_id = track_item && track && track->is_armed() ? track_item->id : -1;
	gui->mask_on_track->set_back_color(track_id >= 0 ?
		gui->get_resources()->text_background :
		gui->get_resources()->text_background_disarmed);
	gui->mask_on_track->update(track_item ? track_item->get_text() : "");
	mwindow->cwindow->mask_track_id = track_item ? track_item->id : -1;
	mwindow->edl->local_session->solo_track_id = -1;
	gui->mask_solo_track->update(0);
	gui->update();
	gui->update_preview(1);
	return 1;
}


CWindowMaskName::CWindowMaskName(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, xS(100), yS(160))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskName::~CWindowMaskName()
{
}

int CWindowMaskName::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
//printf("CWindowMaskGUI::update 1\n");
	gui->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		int k = get_number();
		if( k < 0 ) k = mwindow->edl->session->cwindow_mask;
		else mwindow->edl->session->cwindow_mask = k;
		if( k >= 0 && k < mask_items.size() ) {
			mask_items[k]->set_text(get_text());
			update_list(&mask_items);
		}
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		memset(submask->name, 0, sizeof(submask->name));
		strncpy(submask->name, get_text(), sizeof(submask->name)-1);
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		for(MaskAuto *current = (MaskAuto*)autos->default_auto; current; ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			memset(submask->name, 0, sizeof(submask->name));
			strncpy(submask->name, get_text(), sizeof(submask->name)-1);
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
#endif
		gui->update();
		gui->update_preview();
	}
	return 1;
}

void CWindowMaskName::update_items(MaskAuto *keyframe)
{
	mask_items.remove_all_objects();
	int sz = !keyframe ? 0 : keyframe->masks.size();
	for( int i=0; i<SUBMASKS; ++i ) {
		char text[BCSTRLEN];  memset(text, 0, sizeof(text));
		if( i < sz ) {
			SubMask *sub_mask = keyframe->masks.get(i);
			strncpy(text, sub_mask->name, sizeof(text)-1);
		}
		else
			sprintf(text, "%d", i);
		mask_items.append(new CWindowMaskItem(text));
	}
	update_list(&mask_items);
}


CWindowMaskButton::CWindowMaskButton(MWindow *mwindow, CWindowMaskGUI *gui,
		 int x, int y, int no, int v)
 : BC_CheckBox(x, y, v)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->no = no;
}

CWindowMaskButton::~CWindowMaskButton()
{
}

int CWindowMaskButton::handle_event()
{
	mwindow->edl->session->cwindow_mask = no;
	gui->mask_name->update(gui->mask_name->mask_items[no]->get_text());
	gui->update();
	gui->update_preview();
	return 1;
}

CWindowMaskThumbler::CWindowMaskThumbler(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y)
 : BC_Tumbler(x, y)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskThumbler::~CWindowMaskThumbler()
{
}

int CWindowMaskThumbler::handle_up_event()
{
	return do_event(1);
}

int CWindowMaskThumbler::handle_down_event()
{
	return do_event(-1);
}

int CWindowMaskThumbler::do_event(int dir)
{
	int k = mwindow->edl->session->cwindow_mask;
	if( (k+=dir) >= SUBMASKS ) k = 0;
	else if( k < 0 ) k = SUBMASKS-1;
	mwindow->edl->session->cwindow_mask = k;
	gui->mask_name->update(gui->mask_name->mask_items[k]->get_text());
	gui->update();
	gui->update_preview();
	return 1;
}

CWindowMaskEnable::CWindowMaskEnable(MWindow *mwindow, CWindowMaskGUI *gui,
		 int x, int y, int no, int v)
 : BC_CheckBox(x, y, v)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->no = no;
}

CWindowMaskEnable::~CWindowMaskEnable()
{
}

int CWindowMaskEnable::handle_event()
{
	Track *track = mwindow->cwindow->calculate_mask_track();
	if( track ) {
		mwindow->undo->update_undo_before(_("mask enable"), this);
		int bit = 1 << no;
		if( get_value() )
			track->masks |= bit;
		else
			track->masks &= ~bit;
		gui->update();
		gui->update_preview(1);
		mwindow->undo->update_undo_after(_("mask enable"), LOAD_PATCHES);
	}
	return 1;
}

CWindowMaskUnclear::CWindowMaskUnclear(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("unclear_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Show/Hide mask"));
}

int CWindowMaskUnclear::handle_event()
{
	Track *track = mwindow->cwindow->calculate_mask_track();
	if( track ) {
		mwindow->undo->update_undo_before(_("mask enables"), this);
		int m = (1<<SUBMASKS)-1;
		if( track->masks == m )
			track->masks = 0;
		else
			track->masks = m;
		for( int i=0; i<SUBMASKS; ++i )
			gui->mask_enables[i]->update((track->masks>>i) & 1);
		gui->update_preview(1);
		mwindow->undo->update_undo_after(_("mask enables"), LOAD_PATCHES);
	}
	return 1;
}

CWindowMaskSoloTrack::CWindowMaskSoloTrack(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int v)
 : BC_CheckBox(x, y, v, _("Solo"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Solo video track"));
}

int CWindowMaskSoloTrack::handle_event()
{
	mwindow->edl->local_session->solo_track_id =
		get_value() ? mwindow->cwindow->mask_track_id : -1;
	gui->update_preview(1);
	return 1;
}

int CWindowMaskSoloTrack::calculate_w(BC_WindowBase *gui)
{
	int w = 0, h = 0;
	calculate_extents(gui, &w, &h, _("Solo"));
	return w;
}

CWindowMaskDelMask::CWindowMaskDelMask(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete mask"));
}

int CWindowMaskDelMask::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	int total_points;

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe, mask, point, 0);

	if( track ) {
		mwindow->undo->update_undo_before(_("mask delete"), 0);

#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		submask->points.remove_all_objects();
		total_points = 0;
// Commit change to span of keyframes
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		for(MaskAuto *current = (MaskAuto*)autos->default_auto; current; ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			submask->points.remove_all_objects();
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
		total_points = 0;
#endif
		if( mwindow->cwindow->gui->affected_point >= total_points )
			mwindow->cwindow->gui->affected_point =
				total_points > 0 ? total_points-1 : 0;

		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("mask delete"), LOAD_AUTOMATION);
	}

	return 1;
}

CWindowMaskDelPoint::CWindowMaskDelPoint(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete point"));
}

int CWindowMaskDelPoint::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	int total_points;

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		mwindow->undo->update_undo_before(_("point delete"), 0);

#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		int i = mwindow->cwindow->gui->affected_point;
		for( ; i<submask->points.total-1; ++i )
			*submask->points.values[i] = *submask->points.values[i+1];
		if( submask->points.total > 0 ) {
			point = submask->points.values[submask->points.total-1];
			submask->points.remove_object(point);
		}
		total_points = submask->points.total;

// Commit change to span of keyframes
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		total_points = 0;
		MaskAuto *current = (MaskAuto*)autos->default_auto;
		while( current ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			int i = mwindow->cwindow->gui->affected_point;
			for( ; i<submask->points.total-1; ++i )
				*submask->points.values[i] = *submask->points.values[i+1];
			if( submask->points.total > 0 ) {
				point = submask->points.values[submask->points.total-1];
				submask->points.remove_object(point);
			}
			total_points = submask->points.total;
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
#endif
		if( mwindow->cwindow->gui->affected_point >= total_points )
			mwindow->cwindow->gui->affected_point =
				total_points > 0 ? total_points-1 : 0;

		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("point delete"), LOAD_AUTOMATION);
	}

	return 1;
}


CWindowMaskAffectedPoint::CWindowMaskAffectedPoint(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui,
		(int64_t)mwindow->cwindow->gui->affected_point,
		(int64_t)0, INT64_MAX, x, y, xS(70))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskAffectedPoint::~CWindowMaskAffectedPoint()
{
}

int CWindowMaskAffectedPoint::handle_event()
{
	int total_points = 0;
	int affected_point = atol(get_text());
	Track *track = mwindow->cwindow->calculate_mask_track();
	if(track) {
		MaskAutos *autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		MaskAuto *keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(autos, 0);
		if( keyframe ) {
			SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
			total_points = mask->points.size();
		}
	}
	int active_point = affected_point;
	if( affected_point >= total_points )
		affected_point = total_points - 1;
	if( affected_point < 0 )
		affected_point = 0;
	if( active_point != affected_point )
		update((int64_t)affected_point);
	mwindow->cwindow->gui->affected_point = affected_point;
	gui->update();
	gui->update_preview();
	return 1;
}


CWindowMaskFocus::CWindowMaskFocus(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->focused, _("Focus"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Center for rotate/scale"));
}

CWindowMaskFocus::~CWindowMaskFocus()
{
}

int CWindowMaskFocus::handle_event()
{
 	gui->focused = get_value();
	gui->update();
	gui->update_preview();
	return 1;
}

int CWindowMaskFocus::calculate_w(CWindowMaskGUI *gui)
{
	int w, h;
	calculate_extents(gui, &w, &h, _("Focus"));
	return w;
}

CWindowMaskScaleXY::CWindowMaskScaleXY(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, VFrame **data, int v, int id, const char *tip)
 : BC_Toggle(x, y, data, v)
{
	this->id = id;
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(tip);
}

CWindowMaskScaleXY::~CWindowMaskScaleXY()
{
}

int CWindowMaskScaleXY::handle_event()
{
	gui->scale_mode = id;
	gui->mask_scale_x->update(id == MASK_SCALE_X);
	gui->mask_scale_y->update(id == MASK_SCALE_Y);
	gui->mask_scale_xy->update(id == MASK_SCALE_XY);
	return 1;
}

CWindowMaskHelp::CWindowMaskHelp(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, 0, _("Help"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Show help text"));
}

CWindowMaskHelp::~CWindowMaskHelp()
{
}

int CWindowMaskHelp::handle_event()
{
	gui->helped = get_value();
	gui->resize_window(gui->get_w(),
		gui->helped ? gui->help_h : gui->help_y);
	gui->update();
	return 1;
}

CWindowMaskDrawMarkers::CWindowMaskDrawMarkers(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->markers, _("Markers"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip("Display points");
}

CWindowMaskDrawMarkers::~CWindowMaskDrawMarkers()
{
}

int CWindowMaskDrawMarkers::handle_event()
{
	gui->markers = get_value();
	gui->update();
	gui->update_preview();
	return 1;
}

CWindowMaskDrawBoundary::CWindowMaskDrawBoundary(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->boundary, _("Boundary"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip("Display mask outline");
}

CWindowMaskDrawBoundary::~CWindowMaskDrawBoundary()
{
}

int CWindowMaskDrawBoundary::handle_event()
{
	gui->boundary = get_value();
	gui->update();
	gui->update_preview();
	return 1;
}


CWindowMaskFeather::CWindowMaskFeather(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 0, INT_MIN, INT_MAX, x, y, xS(64), 2)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFeather::~CWindowMaskFeather()
{
}

int CWindowMaskFeather::update(float v)
{
	gui->feather_slider->update(v);
	return BC_TumbleTextBox::update(v);
}

int CWindowMaskFeather::update_value(float v)
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask feather"), this);

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
		int gang = gui->gang_feather->get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		keyframe = &temp_keyframe;
#endif
		float change = v - mask->feather;
		int k = mwindow->edl->session->cwindow_mask;
		int n = gang ? keyframe->masks.size() : k+1;
		for( int i=gang? 0 : k; i<n; ++i ) {
			if( !gui->mask_enables[i]->get_value() ) continue;
			SubMask *sub_mask = keyframe->get_submask(i);
			float feather = sub_mask->feather + change;
			sub_mask->feather = feather;
		}
#ifdef USE_KEYFRAME_SPANNING
		autos->update_parameter(keyframe);
#endif
		gui->update_preview();
	}

	mwindow->undo->update_undo_after(_("mask feather"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskFeather::handle_event()
{
	float v = atof(get_text());
	if( fabsf(v) > MAX_FEATHER )
		BC_TumbleTextBox::update((float)(v>=0 ? MAX_FEATHER : -MAX_FEATHER));
	gui->feather_slider->update(v);
	return gui->feather->update_value(v);
}

CWindowMaskFeatherSlider::CWindowMaskFeatherSlider(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y, int w, float v)
 : BC_FSlider(x, y, 0, w, w, -FEATHER_MAX-5, FEATHER_MAX+5, v)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_precision(0.01);
	timer = new Timer();
	stick = 0;
	last_v = 0;
	max = FEATHER_MAX;
}

CWindowMaskFeatherSlider::~CWindowMaskFeatherSlider()
{
	delete timer;
}

int CWindowMaskFeatherSlider::handle_event()
{
	int sticky = 0;
	float v = get_value();
	if( fabsf(v) > MAX_FEATHER )
		v = v>=0 ? MAX_FEATHER : -MAX_FEATHER;
	if( stick && timer->get_difference() >= 250 )
		stick = 0; // no events for .25 sec
	if( stick && (last_v * (v-last_v)) < 0 )
		stick = 0; // dv changed direction
	if( stick ) {
		if( --stick > 0 ) {
			timer->update();
			update(last_v);
			return 1;
		}
		if( last_v ) {
			max *= 1.25;
			if( max > MAX_FEATHER ) max = MAX_FEATHER;
			update(get_w(), v=last_v, -max-5, max+5);
			button_release_event();
		}
	}
	else if( v > max ) { v = max;  sticky = 24; }
	else if( v < -max ) { v = -max; sticky = 24; }
	else if( v>=0 ? last_v<0 : last_v>=0 ) { v = 0;  sticky = 16; }
	if( sticky ) { update(v);  stick = sticky;  timer->update(); }
	last_v = v;
	gui->feather->BC_TumbleTextBox::update(v);
	return gui->feather->update_value(v);
}

int CWindowMaskFeatherSlider::update(float v)
{
	float vv = fabsf(v);
	if( vv > MAX_FEATHER ) vv = MAX_FEATHER;
	while( max < vv ) max *= 1.25;
	return update(get_w(), v, -max-5, max+5);
}
int CWindowMaskFeatherSlider::update(int r, float v, float mn, float mx)
{
	return BC_FSlider::update(r, v, mn, mx);
}

CWindowMaskFade::CWindowMaskFade(MWindow *mwindow, CWindowMaskGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 0, -100.f, 100.f, x, y, xS(64), 2)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFade::~CWindowMaskFade()
{
}

int CWindowMaskFade::update(float v)
{
	gui->fade_slider->update(v);
	return BC_TumbleTextBox::update(v);
}

int CWindowMaskFade::update_value(float v)
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask fade"), this);

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
		int gang = gui->gang_fader->get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		keyframe = &temp_keyframe;
#endif
		float change = v - mask->fader;
		int k = mwindow->edl->session->cwindow_mask;
		int n = gang ? keyframe->masks.size() : k+1;
		for( int i=gang? 0 : k; i<n; ++i ) {
			if( !gui->mask_enables[i]->get_value() ) continue;
			SubMask *sub_mask = keyframe->get_submask(i);
			float fader = sub_mask->fader + change;
			bclamp(fader, -100.f, 100.f);
			sub_mask->fader = fader;
		}
#ifdef USE_KEYFRAME_SPANNING
		autos->update_parameter(keyframe);
#endif
		gui->update_preview();
	}

	mwindow->undo->update_undo_after(_("mask fade"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskFade::handle_event()
{
	float v = atof(get_text());
	gui->fade_slider->update(v);
	return gui->fade->update_value(v);
}

CWindowMaskFadeSlider::CWindowMaskFadeSlider(MWindow *mwindow, CWindowMaskGUI *gui,
		int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, -200, 200, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	timer = new Timer();
	stick = 0;
	last_v = 0;
}

CWindowMaskFadeSlider::~CWindowMaskFadeSlider()
{
	delete timer;
}

int CWindowMaskFadeSlider::handle_event()
{
	float v = 100*get_value()/200;
	if( stick > 0 ) {
		int64_t ms = timer->get_difference();
		if( ms < 250 && --stick > 0 ) {
			if( get_value() == 0 ) return 1;
			update(v = 0);
		}
		else {
			stick = 0;
			last_v = v;
		}
	}
	else if( (last_v>=0 && v<0) || (last_v<0 && v>=0) ) {
		stick = 16;
		v = 0;
	}
	else
		last_v = v;
	timer->update();
	gui->fade->BC_TumbleTextBox::update(v);
	return gui->fade->update_value(v);
}

int CWindowMaskFadeSlider::update(int64_t v)
{
	return BC_ISlider::update(200*v/100);
}

CWindowMaskGangFader::CWindowMaskGangFader(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("gangpatch_data"), 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Gang fader"));
}

CWindowMaskGangFader::~CWindowMaskGangFader()
{
}

int CWindowMaskGangFader::handle_event()
{
	return 1;
}

CWindowMaskGangFocus::CWindowMaskGangFocus(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("gangpatch_data"), 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Gang rotate/scale/translate"));
}

CWindowMaskGangFocus::~CWindowMaskGangFocus()
{
}

int CWindowMaskGangFocus::handle_event()
{
	return 1;
}

CWindowMaskGangPoint::CWindowMaskGangPoint(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("gangpatch_data"), 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Gang points"));
}

CWindowMaskGangPoint::~CWindowMaskGangPoint()
{
}

int CWindowMaskGangPoint::handle_event()
{
	return 1;
}


CWindowMaskSmoothButton::CWindowMaskSmoothButton(MWindow *mwindow, CWindowMaskGUI *gui,
		const char *tip, int type, int on, int x, int y, const char *images)
 : BC_Button(x, y, mwindow->theme->get_image_set(images))
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->type = type;
	this->on = on;
	set_tooltip(tip);
}

int CWindowMaskSmoothButton::handle_event()
{
	return gui->smooth_mask(type, on);
}

CWindowMaskBeforePlugins::CWindowMaskBeforePlugins(CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, 1, _("Apply mask before plugins"))
{
	this->gui = gui;
}

int CWindowMaskBeforePlugins::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
	gui->get_keyframe(track, autos, keyframe, mask, point, 1);

	if (keyframe) {
		int v = get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(gui->mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		temp_keyframe.apply_before_plugins = v;
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->apply_before_plugins = v;
#endif
		gui->update_preview();
	}
	return 1;
}


CWindowDisableOpenGLMasking::CWindowDisableOpenGLMasking(CWindowMaskGUI *gui, int x, int y)
 : BC_CheckBox(x, y, 1, _("Disable OpenGL masking"))
{
	this->gui = gui;
}

int CWindowDisableOpenGLMasking::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
	gui->get_keyframe(track, autos, keyframe, mask, point, 1);

	if( keyframe ) {
		int v = get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(gui->mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		temp_keyframe.disable_opengl_masking = v;
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->disable_opengl_masking = v;
#endif
		gui->update_preview();
	}
	return 1;
}


CWindowMaskClrMask::CWindowMaskClrMask(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("reset_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete all masks"));
}

CWindowMaskClrMask::~CWindowMaskClrMask()
{
}

int CWindowMaskClrMask::calculate_w(MWindow *mwindow)
{
	VFrame *vfrm = *mwindow->theme->get_image_set("reset_button");
	return vfrm->get_w();
}

int CWindowMaskClrMask::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe, mask, point, 0);

	if( track ) {
		mwindow->undo->update_undo_before(_("del masks"), 0);
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->clear_all();
		mwindow->undo->update_undo_after(_("del masks"), LOAD_AUTOMATION);
	}

	gui->update();
	gui->update_preview(1);
	return 1;
}

CWindowMaskGangFeather::CWindowMaskGangFeather(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("gangpatch_data"), 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Gang feather"));
}

CWindowMaskGangFeather::~CWindowMaskGangFeather()
{
}

int CWindowMaskGangFeather::handle_event()
{
	return 1;
}

CWindowMaskGUI::CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread,
	_(PROGRAM_NAME ": Mask"), xS(440), yS(700))
{
	this->mwindow = mwindow;
	this->thread = thread;
	active_point = 0;
	fade = 0;
	feather = 0;
	focused = 0;
	scale_mode = 2;
	markers = 1;
	boundary = 1;
	preset_dialog = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Masks");
}
CWindowMaskGUI::~CWindowMaskGUI()
{
	lock_window("CWindowMaskGUI::~CWindowMaskGUI");
	done_event();
	delete active_point;
	delete fade;
	delete feather;
	unlock_window();
	delete preset_dialog;
}

void CWindowMaskGUI::create_objects()
{
	int t[SUBMASKS];
	Theme *theme = mwindow->theme;
	int xs10 = xS(10), ys10 = yS(10);
	int x = xs10, y = ys10;
	int margin = theme->widget_border;
	int clr_w = CWindowMaskClrMask::calculate_w(mwindow);
	int clr_x = get_w()-x - clr_w;

	lock_window("CWindowMaskGUI::create_objects");
	BC_TitleBar *title_bar;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Masks on Track")));
	y += title_bar->get_h() + margin;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x,y, _("Track:")));
	int x1 = x + xS(90), ww = clr_x-2*margin - x1;
	for( int i=0,n=sizeof(t)/sizeof(t[0]); i<n; ++i ) t[i] = x1+(i*ww)/n;
	int del_x = t[5];
	Track *track = mwindow->cwindow->calculate_affected_track();
	const char *text = track ? track->title : "";
	mwindow->cwindow->mask_track_id = track ? track->get_id() : -1;
	mask_on_track = new CWindowMaskOnTrack(mwindow, this, x1, y, xS(100), text);
	mask_on_track->create_objects();
	mask_on_track->set_tooltip(_("Video track"));
	int x2 = x1 + mask_on_track->get_w();
	add_subwindow(mask_track_tumbler = new CWindowMaskTrackTumbler(mwindow, this, x2, y));
	mwindow->edl->local_session->solo_track_id = -1;
	add_subwindow(mask_solo_track = new CWindowMaskSoloTrack(mwindow, this, del_x, y, 0));
	y += mask_on_track->get_h() + margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Masks")));
	y += title_bar->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Mask:")));
	mask_name = new CWindowMaskName(mwindow, this, x1, y, "");
	mask_name->create_objects();
	mask_name->set_tooltip(_("Mask name"));
	add_subwindow(mask_clr = new CWindowMaskClrMask(mwindow, this, clr_x, y));
	add_subwindow(mask_del = new CWindowMaskDelMask(mwindow, this, del_x, y));
	y += mask_name->get_h() + 2*margin;
	BC_Bar *bar;
//	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
//	y += bar->get_h() + 2*margin;

	add_subwindow(title = new BC_Title(x, y, _("Select:")));
	int bw = 0, bh = 0;
	BC_CheckBox::calculate_extents(this, &bw, &bh);
	for( int i=0; i<SUBMASKS; ++i ) {
		int v = i == mwindow->edl->session->cwindow_mask ? 1 : 0;
		mask_buttons[i] = new CWindowMaskButton(mwindow, this, t[i], y, i, v);
		add_subwindow(mask_buttons[i]);
	}
	add_subwindow(mask_thumbler = new CWindowMaskThumbler(mwindow, this, clr_x, y));
	y += bh + margin;
	for( int i=0; i<SUBMASKS; ++i ) {
		char text[BCSTRLEN];  sprintf(text, "%d", i);
		int tx = (bw - get_text_width(MEDIUMFONT, text)) / 2;
		mask_blabels[i] = new BC_Title(t[i]+tx, y, text);
		add_subwindow(mask_blabels[i]);
	}
	y += mask_blabels[0]->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Enable:")));
	for( int i=0; i<SUBMASKS; ++i ) {
		mask_enables[i] = new CWindowMaskEnable(mwindow, this, t[i], y, i, 1);
		add_subwindow(mask_enables[i]);
	}
	add_subwindow(mask_unclr = new CWindowMaskUnclear(mwindow, this, clr_x, y));
	y += mask_enables[0]->get_h() + 2*margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Preset Shapes")));
	y += title_bar->get_h() + margin;
	add_subwindow(mask_shape_sqr = new CWindowMaskShape(mwindow, this,
		"mask_prst_sqr_images", MASK_SHAPE_SQUARE, t[0], y, _("Square")));
	add_subwindow(mask_shape_crc = new CWindowMaskShape(mwindow, this,
		"mask_prst_crc_images", MASK_SHAPE_CIRCLE, t[1], y, _("Circle")));
	add_subwindow(mask_shape_tri = new CWindowMaskShape(mwindow, this,
		"mask_prst_tri_images", MASK_SHAPE_TRIANGLE, t[2], y, _("Triangle")));
	add_subwindow(mask_shape_ovl = new CWindowMaskShape(mwindow, this,
		"mask_prst_ovl_images", MASK_SHAPE_OVAL, t[3], y, _("Oval")));
	add_subwindow(mask_load_list = new CWindowMaskLoadList(mwindow, this));
	add_subwindow(mask_load = new CWindowMaskLoad(mwindow, this, t[5], y, xS(80)));
	add_subwindow(mask_save = new CWindowMaskSave(mwindow, this, t[6], y, xS(80)));
	add_subwindow(mask_delete = new CWindowMaskDelete(mwindow, this, t[7], y, xS(80)));
	y += mask_load->get_h() + 2*margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Position & Scale")));
	y += title_bar->get_h() + 2*margin;
	add_subwindow(mask_center = new CWindowMaskCenter(mwindow, this, t[0], y, xS(80)));
	add_subwindow(mask_normal = new CWindowMaskNormal(mwindow, this, t[1], y, xS(80)));

	add_subwindow(mask_scale_x = new CWindowMaskScaleXY(mwindow, this,
		t[5], y, theme->get_image_set("mask_scale_x"), 0, MASK_SCALE_X, _("xlate/scale x")));
	add_subwindow(mask_scale_y = new CWindowMaskScaleXY(mwindow, this,
		t[6], y, theme->get_image_set("mask_scale_y"), 0, MASK_SCALE_Y, _("xlate/scale y")));
	add_subwindow(mask_scale_xy = new CWindowMaskScaleXY(mwindow, this,
		t[7], y, theme->get_image_set("mask_scale_xy"), 1, MASK_SCALE_XY, _("xlate/scale xy")));
	y += mask_center->get_h() + 2*margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Fade & Feather")));
	y += title_bar->get_h() + 2*margin;

	add_subwindow(title = new BC_Title(x, y, _("Fade:")));
	fade = new CWindowMaskFade(mwindow, this, x1, y);
	fade->create_objects();
	x2 = x1 + fade->get_w() + 2*margin;
	int w2 = clr_x-2*margin - x2;
	add_subwindow(fade_slider = new CWindowMaskFadeSlider(mwindow, this, x2, y, w2));
	add_subwindow(gang_fader = new CWindowMaskGangFader(mwindow, this, clr_x, y));
	y += fade->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Feather:")));
	feather = new CWindowMaskFeather(mwindow, this, x1, y);
	feather->create_objects();
	w2 = clr_x - 2*margin - x2;
	feather_slider = new CWindowMaskFeatherSlider(mwindow, this, x2, y, w2, 0);
	add_subwindow(feather_slider);
	add_subwindow(gang_feather = new CWindowMaskGangFeather(mwindow, this, clr_x, y));
	y += feather->get_h() + 2*margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Mask Points")));
	y += title_bar->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, _("Point:")));
	active_point = new CWindowMaskAffectedPoint(mwindow, this, t[0], y);
	active_point->create_objects();
// typ=0, this mask, this point
	add_subwindow(mask_pnt_linear = new CWindowMaskSmoothButton(mwindow, this,
		_("linear point"), 0, 0, t[3], y, "mask_pnt_linear_images"));
	add_subwindow(mask_pnt_smooth = new CWindowMaskSmoothButton(mwindow, this,
		_("smooth point"), 0, 1, t[4], y, "mask_pnt_smooth_images"));
	add_subwindow(del_point = new CWindowMaskDelPoint(mwindow, this, del_x, y));
	add_subwindow(gang_point = new CWindowMaskGangPoint(mwindow, this, clr_x, y));
	y += active_point->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "X:"));
	this->x = new CWindowCoord(this, t[0], y, (float)0.0);
	this->x->create_objects();
// typ>0, this mask, all points
	add_subwindow(mask_crv_linear = new CWindowMaskSmoothButton(mwindow, this,
		_("linear curve"), 1, 0, t[3], y, "mask_crv_linear_images"));
	add_subwindow(mask_crv_smooth = new CWindowMaskSmoothButton(mwindow, this,
		_("smooth curve"), 1, 1, t[4], y, "mask_crv_smooth_images"));
	add_subwindow(draw_markers = new CWindowMaskDrawMarkers(mwindow, this, del_x, y));
	y += this->x->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	this->y = new CWindowCoord(this, x1, y, (float)0.0);
	this->y->create_objects();
// typ<0, all masks, all points
	add_subwindow(mask_all_linear = new CWindowMaskSmoothButton(mwindow, this,
		_("linear all"), -1, 0, t[3], y, "mask_all_linear_images"));
	add_subwindow(mask_all_smooth = new CWindowMaskSmoothButton(mwindow, this,
		_("smooth all"), -1, 1, t[4], y, "mask_all_smooth_images"));
	add_subwindow(draw_boundary = new CWindowMaskDrawBoundary(mwindow, this, del_x, y));
	y += this->y->get_h() + 2*margin;
	add_subwindow(title_bar = new BC_TitleBar(x, y, get_w()-2*x, xS(20), xS(10),
			_("Pivot Point")));
	y += title_bar->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, "X:"));
	float cx = mwindow->edl->session->output_w / 2.f;
	focus_x = new CWindowCoord(this, x1, y, cx);
	focus_x->create_objects();
	add_subwindow(focus = new CWindowMaskFocus(mwindow, this, del_x, y));
	add_subwindow(gang_focus = new CWindowMaskGangFocus(mwindow, this, clr_x, y));
	y += focus_x->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	float cy = mwindow->edl->session->output_h / 2.f;
	focus_y = new CWindowCoord(this, x1, y, cy);
	focus_y->create_objects();
	y += focus_y->get_h() + 2*margin;
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += bar->get_h() + margin;
	add_subwindow(this->apply_before_plugins = new CWindowMaskBeforePlugins(this, x, y));
	y += this->apply_before_plugins->get_h();
	add_subwindow(this->disable_opengl_masking = new CWindowDisableOpenGLMasking(this, x, y));
	add_subwindow(help = new CWindowMaskHelp(mwindow, this, del_x, y));
	y += this->disable_opengl_masking->get_h() + 2*margin;
	help_y = y;
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += bar->get_h() + 2*margin;
	add_subwindow(title = new BC_Title(x, y, _(
		"Shift+LMB: move an end point\n"
		"Ctrl+LMB: move a control point\n"
		"Alt+LMB: to drag translate the mask\n"
		"Shift+MMB: Set Pivot Point at pointer\n"
		"Wheel: rotate around Pivot Point\n"
		"Shift+Wheel: scale around Pivot Point\n"
		"Ctrl+Wheel: rotate/scale around pointer")));
	help_h = y + title->get_h() + 2*margin;
	update();
	resize_window(get_w(), help_y);
	unlock_window();
}

int CWindowMaskGUI::close_event()
{
	done_event();
	return CWindowToolGUI::close_event();
}

void CWindowMaskGUI::done_event()
{
	if( mwindow->in_destructor ) return;
	int &solo_track_id = mwindow->edl->local_session->solo_track_id;
	if( solo_track_id >= 0 ) {
		solo_track_id = -1;
		update_preview();
	}
}

void CWindowMaskGUI::get_keyframe(Track* &track,
		MaskAutos* &autos, MaskAuto* &keyframe,
		SubMask* &mask, MaskPoint* &point, int create_it)
{
	autos = 0;
	keyframe = 0;

	track = mwindow->cwindow->calculate_mask_track();
	if( !track )
		track = mwindow->cwindow->calculate_affected_track();
		
	if(track) {
		autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(
			autos,
			create_it);
	}

	mask = !keyframe ? 0 :
		keyframe->get_submask(mwindow->edl->session->cwindow_mask);

	point = 0;
	if( keyframe ) {
		if( mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0 ) {
			point = mask->points.values[mwindow->cwindow->gui->affected_point];
		}
	}
}

void CWindowMaskGUI::update()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
//printf("CWindowMaskGUI::update 1\n");
	get_keyframe(track, autos, keyframe, mask, point, 0);
	mwindow->cwindow->mask_track_id = track ? track->get_id() : -1;
	mask_on_track->set_back_color(!track || track->is_armed() ?
		get_resources()->text_background :
		get_resources()->text_background_disarmed);
	mask_on_track->update_items();
	mask_on_track->update(!track ? "" : track->title);
	mask_name->update_items(keyframe);
	const char *text = "";
	int sz = !keyframe ? 0 : keyframe->masks.size();
	int k = mwindow->edl->session->cwindow_mask;
	if( k >= 0 && k < sz )
		text = keyframe->masks[k]->name;
	else
		k = mwindow->edl->session->cwindow_mask = 0;
	mask_name->update(text);
	update_buttons(keyframe, k);
	if( point ) {
		x->update(point->x);
		y->update(point->y);
	}
	if( track ) {
		double position = mwindow->edl->local_session->get_selectionstart(1);
		int64_t position_i = track->to_units(position, 0);
		feather->update(autos->get_feather(position_i, k, PLAY_FORWARD));
		fade->update(autos->get_fader(position_i, k, PLAY_FORWARD));
		int show_mask = track->masks;
		for( int i=0; i<SUBMASKS; ++i )
			mask_enables[i]->update((show_mask>>i) & 1);
	}
	if( keyframe ) {
		apply_before_plugins->update(keyframe->apply_before_plugins);
		disable_opengl_masking->update(keyframe->disable_opengl_masking);
	}
	active_point->update((int64_t)mwindow->cwindow->gui->affected_point);
}

void CWindowMaskGUI::handle_event()
{
	int redraw = 0;
	if( event_caller == this->focus_x ||
	    event_caller == this->focus_y ) {
		redraw = 1;
	}
	else if( event_caller == this->x ||
		 event_caller == this->y ) {
		Track *track;
		MaskAuto *keyframe;
		MaskAutos *autos;
		SubMask *mask;
		MaskPoint *point;
		get_keyframe(track, autos, keyframe, mask, point, 0);

		mwindow->undo->update_undo_before(_("mask point"), this);

		if( point ) {
			float px = atof(x->get_text());
			float py = atof(y->get_text());
			float dx = px - point->x, dy = py - point->y;
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
			MaskAuto temp_keyframe(mwindow->edl, autos);
			temp_keyframe.copy_data(keyframe);
// Get affected point in temp keyframe
			mask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
#endif
			MaskPoints &points = mask->points;
			int gang = gang_point->get_value();
			int k = mwindow->cwindow->gui->affected_point;
			int n = gang ? points.size() : k+1;
			for( int i=gang? 0 : k; i<n; ++i ) {
				if( i < 0 || i >= points.size() ) continue;
				MaskPoint *point = points[i];
				point->x += dx;  point->y += dy;
			}
#ifdef USE_KEYFRAME_SPANNING
// Commit to spanned keyframes
			autos->update_parameter(&temp_keyframe);
#endif
		}
		mwindow->undo->update_undo_after(_("mask point"), LOAD_AUTOMATION);
		redraw = 1;
	}

	if( redraw )
		update_preview();
}

void CWindowMaskGUI::set_focused(int v, float cx, float cy)
{
	CWindowGUI *cgui = mwindow->cwindow->gui;
	cgui->unlock_window();
	lock_window("CWindowMaskGUI::set_focused");
	if( focused != v )
		focus->update(focused = v);
	focus_x->update(cx);
	focus_y->update(cy);
	unlock_window();
	cgui->lock_window("CWindowCanvas::set_focused");
}

void CWindowMaskGUI::update_buttons(MaskAuto *keyframe, int k)
{
	int text_color = get_resources()->default_text_color;
	int high_color = get_resources()->button_highlighted;
	for( int i=0; i<SUBMASKS; ++i ) {
		int color = text_color;
		if( keyframe ) {
			SubMask *submask = keyframe->get_submask(i);
			if( submask && submask->points.size() )
				color = high_color;
		}
		mask_blabels[i]->set_color(color);
		mask_buttons[i]->update(i==k ? 1 : 0);
	}
}

// typ=0, this mask, this point
// typ>0, this mask, all points
// typ<0, all masks, all points
// dxy= on? pt[+1]-pt[-1] : dxy=0
int CWindowMaskGUI::smooth_mask(int typ, int on)
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask smooth"), this);

// Get existing keyframe
	get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		keyframe = &temp_keyframe;
#endif
		int k = mwindow->edl->session->cwindow_mask;
		int n = typ>=0 ? k+1 : keyframe->masks.size();
		for( int j=typ<0? 0 : k; j<n; ++j ) {
			if( !mask_enables[j]->get_value() ) continue;
			SubMask *sub_mask = keyframe->get_submask(j);
			MaskPoints &points = sub_mask->points;
			int psz = points.size();
			if( psz < 3 ) continue;
			int l = mwindow->cwindow->gui->affected_point;
			if( l > psz ) l = psz;
			int m = typ ? psz : l+1;
			for( int i=typ ? 0 : l; i<m; ++i ) {
				int i0 = i-1, i1 = i+1;
				if( i0 < 0 ) i0 = psz-1;
				if( i1 >= psz ) i1 = 0;
				MaskPoint *p0 = points[i0];
				MaskPoint *p  = points[i];
				MaskPoint *p1 = points[i1];
				float dx = !on ? 0 : p1->x - p0->x;
				float dy = !on ? 0 : p1->y - p0->y;
				p->control_x1 = -dx/4;  p->control_y1 = -dy/4;
				p->control_x2 =  dx/4;  p->control_y2 =  dy/4;
			}
		}
#ifdef USE_KEYFRAME_SPANNING
		autos->update_parameter(keyframe);
#endif
		update_preview();
	}

	mwindow->undo->update_undo_after(_("mask smooth"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskGUI::save_mask(const char *nm)
{
	int k = mwindow->edl->session->cwindow_mask;
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	get_keyframe(track, autos, keyframe, mask, point, 0);
	if( !track ) return 0;
	SubMask *sub_mask = keyframe->get_submask(k);
	ArrayList<SubMask *> masks;
	load_masks(masks);
	int i = masks.size();
	while( --i >= 0 ) {
		if( strcmp(masks[i]->name, nm) ) continue;
		masks.remove_object_number(i++);
	}
	mask = new SubMask(0, -1);
	strncpy(mask->name, nm, sizeof(mask->name)-1);
	mask->copy_from(*sub_mask, 0);
	masks.append(mask);
	save_masks(masks);
	masks.remove_all_objects();
	return 1;
}

int CWindowMaskGUI::del_mask(const char *nm)
{
	ArrayList<SubMask *> masks;
	load_masks(masks);
	int i = masks.size();
	while( --i >= 0 ) {
		if( strcmp(masks[i]->name, nm) ) continue;
		masks.remove_object_number(i++);
	}
	save_masks(masks);
	masks.remove_all_objects();
	return 1;
}

int CWindowMaskGUI::center_mask()
{
	int k = mwindow->edl->session->cwindow_mask;
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif
	get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( !track ) return 0;
	mwindow->undo->update_undo_before(_("mask center"), this);

// Get existing keyframe
#ifdef USE_KEYFRAME_SPANNING
	MaskAuto temp_keyframe(mwindow->edl, autos);
	temp_keyframe.copy_data(keyframe);
	keyframe = &temp_keyframe;
#endif
	SubMask *sub_mask = keyframe->get_submask(k);
	MaskPoints &points = sub_mask->points;
	int psz = points.size();
	if( psz > 0 ) {
		float cx = 0, cy = 0;
		for( int i=0; i<psz; ++i ) {
			MaskPoint *p  = points[i];
			cx += p->x;  cy += p->y;
		}
		cx /= psz;  cy /= psz;
		cx -= mwindow->edl->session->output_w / 2.f;
		cy -= mwindow->edl->session->output_h / 2.f;
		for( int i=0; i<psz; ++i ) {
			MaskPoint *p  = points[i];
			p->x -= cx;  p->y -= cy;
		}
	}
#ifdef USE_KEYFRAME_SPANNING
	autos->update_parameter(keyframe);
#endif
	update_preview();
	mwindow->undo->update_undo_after(_("mask center"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskGUI::normal_mask()
{
	int k = mwindow->edl->session->cwindow_mask;
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif
// Get existing keyframe
	get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( !track ) return 0;
	mwindow->undo->update_undo_before(_("mask normal"), this);

#ifdef USE_KEYFRAME_SPANNING
	MaskAuto temp_keyframe(mwindow->edl, autos);
	temp_keyframe.copy_data(keyframe);
	keyframe = &temp_keyframe;
#endif
	SubMask *sub_mask = keyframe->get_submask(k);
	MaskPoints &points = sub_mask->points;
	int psz = points.size();
	float cx = 0, cy = 0;
	double dr = 0;
	if( psz > 0 ) {
		for( int i=0; i<psz; ++i ) {
			MaskPoint *p  = points[i];
			cx += p->x;  cy += p->y;
		}
		cx /= psz;  cy /= psz;
		for( int i=0; i<psz; ++i ) {
			MaskPoint *p  = points[i];
			float dx = fabsf(p->x-cx), dy = fabsf(p->y-cy);
			double d = sqrt(dx*dx + dy*dy);
			if( dr < d ) dr = d;
		}
	}
	if( dr > 0 ) {
		float out_w = mwindow->edl->session->output_w;
		float out_h = mwindow->edl->session->output_h;
		float r = bmax(out_w, out_h);
		float s = r / (4 * dr * sqrt(2.));
		for( int i=0; i<psz; ++i ) {
			MaskPoint *p  = points[i];
			float x = p->x, y = p->y;
			p->x = (x-cx) * s + cx;
			p->y = (y-cy) * s + cy;
			p->control_x1 *= s;  p->control_y1 *= s;
			p->control_x2 *= s;  p->control_y2 *= s;
		}
	}
#ifdef USE_KEYFRAME_SPANNING
	autos->update_parameter(keyframe);
#endif
	update_preview();

	mwindow->undo->update_undo_after(_("mask normal"), LOAD_AUTOMATION);
	return 1;
}


CWindowMaskLoadList::CWindowMaskLoadList(MWindow *mwindow, CWindowMaskGUI *gui)
 : BC_ListBox(-1, -1, 1, 1, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_use_button(0);
}

CWindowMaskLoadList::~CWindowMaskLoadList()
{
}


int CWindowMaskLoadList::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask shape"), this);

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	CWindowMaskItem *item = (CWindowMaskItem *) get_selection(0, 0);
	if( track && item ) {
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		keyframe = &temp_keyframe;
		mask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
#endif
		ArrayList<SubMask *> masks;
		gui->load_masks(masks);
		mask->copy_from(*masks[item->id], 0);
		masks.remove_all_objects();
#ifdef USE_KEYFRAME_SPANNING
		autos->update_parameter(keyframe);
#endif
		gui->update();
		gui->update_preview(1);
	}
	mwindow->undo->update_undo_after(_("mask shape"), LOAD_AUTOMATION);
	return 1;
}

void CWindowMaskLoadList::create_objects()
{
	shape_items.remove_all_objects();
	ArrayList<SubMask *> masks;
	gui->load_masks(masks);
	for( int i=0; i<masks.size(); ++i )
		shape_items.append(new CWindowMaskItem(masks[i]->name, i));
	masks.remove_all_objects();
	update(&shape_items, 0, 0, 1);
}

CWindowMaskLoad::CWindowMaskLoad(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int w)
 : BC_Button(x, y, mwindow->theme->get_image_set("mask_prst_load_images"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Load preset"));
}

int CWindowMaskLoad::handle_event()
{
	gui->mask_load_list->create_objects();
	int px, py;
	get_abs_cursor(px, py);
	return gui->mask_load_list->activate(px, py, xS(120),yS(160));
}


CWindowMaskSave::CWindowMaskSave(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int w)
 : BC_Button(x, y, mwindow->theme->get_image_set("mask_prst_save_images"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Save preset"));
}

CWindowMaskSave::~CWindowMaskSave()
{
}

int CWindowMaskSave::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
	gui->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		int sx = 0, sy = 0;
		gui->get_abs_cursor(sx, sy);
		if( !gui->preset_dialog )
			gui->preset_dialog = new CWindowMaskPresetDialog(mwindow, gui);
		gui->preset_dialog->start_dialog(sx, sy, keyframe);
	}
	return 1;
}

CWindowMaskPresetDialog::CWindowMaskPresetDialog(MWindow *mwindow, CWindowMaskGUI *gui)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->gui = gui;
	pgui = 0;
}

CWindowMaskPresetDialog::~CWindowMaskPresetDialog()
{
	close_window();
}

void CWindowMaskPresetDialog::handle_close_event(int result)
{
	pgui = 0;
}

void CWindowMaskPresetDialog::handle_done_event(int result)
{
	if( result ) return;
	const char *nm = pgui->preset_text->get_text();
	if( keyframe )
		gui->save_mask(nm);
	else
		gui->del_mask(nm);
}

BC_Window* CWindowMaskPresetDialog::new_gui()
{
	pgui = new CWindowMaskPresetGUI(this, sx, sy,
		keyframe ? _(PROGRAM_NAME ": Save Mask") :
			   _(PROGRAM_NAME ": Delete Mask"));
	pgui->create_objects();
	return pgui;
}

void CWindowMaskPresetDialog::start_dialog(int sx, int sy, MaskAuto *keyframe)
{
	close_window();
	this->sx = sx;  this->sy = sy;
	this->keyframe = keyframe;
	start();
}

CWindowMaskPresetGUI::CWindowMaskPresetGUI(CWindowMaskPresetDialog *preset_dialog,
			int x, int y, const char *title)
 : BC_Window(title, x, y, xS(320), yS(100), xS(320), yS(100), 0, 0, 1)
{
	this->preset_dialog = preset_dialog;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Masks");
}

void CWindowMaskPresetGUI::create_objects()
{
	int xs8 = xS(8), xs10 = xS(10);
	int ys10 = yS(10);
	int x = xs10, y = ys10;
	lock_window("CWindowMaskPresetGUI::create_objects");
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y,
		preset_dialog->keyframe ? _("Save mask:") : _("Delete mask:")));
	int x1 = x + title->get_w() + xs8;
	int x2 = get_w() - x - xs8 - x1 -
		BC_WindowBase::get_resources()->listbox_button[0]->get_w();
	CWindowMaskGUI *gui = preset_dialog->gui;
	preset_text = new CWindowMaskPresetText(this,
		x1, y, x2, yS(120), gui->mask_name->get_text());
	preset_text->create_objects();
	preset_text->set_tooltip(_("Mask name"));
	preset_text->update_items();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	raise_window();
	unlock_window();
}

CWindowMaskPresetText::CWindowMaskPresetText(CWindowMaskPresetGUI *pgui,
		int x, int y, int w, int h, const char *text)
 : BC_PopupTextBox(pgui, 0, text, x, y, w, h)
{
	this->pgui = pgui;
}

int CWindowMaskPresetText::handle_event()
{
	int k = get_number();
	if( k >= 0 && k<mask_items.size() )
		update(mask_items[k]->get_text());
	return 1;
}

void CWindowMaskPresetText::update_items()
{
	mask_items.remove_all_objects();
	ArrayList<SubMask *> masks;
	pgui->preset_dialog->gui->load_masks(masks);
	for( int i=0; i<masks.size(); ++i ) {
		char text[BCSTRLEN];  memset(text, 0, sizeof(text));
		strncpy(text, masks[i]->name, sizeof(text)-1);
		mask_items.append(new CWindowMaskItem(text));
	}
	masks.remove_all_objects();
	update_list(&mask_items);
}


CWindowMaskDelete::CWindowMaskDelete(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int w)
 : BC_Button(x, y, mwindow->theme->get_image_set("mask_prst_trsh_images"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete preset"));
}

int CWindowMaskDelete::handle_event()
{
	int sx = 0, sy = 0;
	gui->get_abs_cursor(sx, sy);
	if( !gui->preset_dialog )
		gui->preset_dialog = new CWindowMaskPresetDialog(mwindow, gui);
	gui->preset_dialog->start_dialog(sx, sy, 0);
	return 1;
}


CWindowMaskCenter::CWindowMaskCenter(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int w)
 : BC_Button(x, y, mwindow->theme->get_image_set("mask_pstn_cen_images"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("center mask"));
}

int CWindowMaskCenter::handle_event()
{
	return gui->center_mask();
}


CWindowMaskNormal::CWindowMaskNormal(MWindow *mwindow,
	CWindowMaskGUI *gui, int x, int y, int w)
 : BC_Button(x, y, mwindow->theme->get_image_set("mask_pstn_nrm_images"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("normalize mask"));
}

int CWindowMaskNormal::handle_event()
{
	return gui->normal_mask();
}


CWindowMaskShape::CWindowMaskShape(MWindow *mwindow, CWindowMaskGUI *gui,
		const char *images, int shape, int x, int y, const char *tip)
 : BC_Button(x, y, mwindow->theme->get_image_set(images))
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->shape = shape;
	set_tooltip(tip);
}

CWindowMaskShape::~CWindowMaskShape()
{
}

void CWindowMaskShape::builtin_shape(int i, SubMask *sub_mask)
{
	int out_w = mwindow->edl->session->output_w;
	int out_h = mwindow->edl->session->output_h;
	float cx = out_w/2.f, cy = out_h/2.f;
	float r = bmax(cx, cy) / 4.f;
	double c = 4*(sqrt(2.)-1)/3; // bezier aprox circle
	float r2 = r / 2.f, rc = r*c, r4 = r / 4.f;
	MaskPoint *pt = 0;
	MaskPoints &points = sub_mask->points;
	points.remove_all_objects();
	switch( i ) {
	case MASK_SHAPE_SQUARE:
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy - r;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy - r;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy + r;
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy + r;
		break;
	case MASK_SHAPE_CIRCLE:
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy - r;
		pt->control_x1 = -rc;  pt->control_y1 =  rc;
		pt->control_x2 =  rc;  pt->control_y2 = -rc;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy - r;
		pt->control_x1 = -rc;  pt->control_y1 = -rc;
		pt->control_x2 =  rc;  pt->control_y2 =  rc;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy + r;
		pt->control_x1 =  rc;  pt->control_y1 = -rc;
		pt->control_x2 = -rc;  pt->control_y2 =  rc;
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy + r;
		pt->control_x1 =  rc;  pt->control_y1 =  rc;
		pt->control_x2 = -rc;  pt->control_y2 = -rc;
		break;
	case MASK_SHAPE_TRIANGLE:
		points.append(pt = new MaskPoint());
		pt->x = cx + 0;  pt->y = cy - r*(sqrt(3.)-1.);
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy + r;
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy + r;
		break;
	case MASK_SHAPE_OVAL:
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy - r2;
		pt->control_x1 = -r2;  pt->control_y1 =  r4;
		pt->control_x2 =  r2;  pt->control_y2 = -r4;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy - r2;
		pt->control_x1 = -r2;  pt->control_y1 = -r4;
		pt->control_x2 =  r2;  pt->control_y2 =  r4;
		points.append(pt = new MaskPoint());
		pt->x = cx + r;  pt->y = cy + r2;
		pt->control_x1 =  r2;  pt->control_y1 = -r4;
		pt->control_x2 = -r2;  pt->control_y2 =  r4;
		points.append(pt = new MaskPoint());
		pt->x = cx - r;  pt->y = cy + r2;
		pt->control_x1 =  r2;  pt->control_y1 =  r4;
		pt->control_x2 = -r2;  pt->control_y2 = -r4;
		break;
	}
}

int CWindowMaskShape::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask shape"), this);

// Get existing keyframe
	gui->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		keyframe = &temp_keyframe;
		mask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
#endif
		if( mask ) {
			builtin_shape(shape, mask);
#ifdef USE_KEYFRAME_SPANNING
			autos->update_parameter(keyframe);
#endif
			gui->update();
			gui->update_preview(1);
		}
	}
	mwindow->undo->update_undo_after(_("mask shape"), LOAD_AUTOMATION);
	return 1;
}

void CWindowMaskGUI::load_masks(ArrayList<SubMask *> &masks)
{
        char path[BCTEXTLEN];
        sprintf(path, "%s/%s", File::get_config_path(), MASKS_FILE);
        FileSystem fs;
        fs.complete_path(path);
	FileXML file;
	file.read_from_file(path, 1);

	masks.remove_all_objects();
	int result;
	while( !(result = file.read_tag()) ) {
		if( file.tag.title_is("MASK") ) {
			SubMask *sub_mask = new SubMask(0, -1);
			char name[BCTEXTLEN];  name[0] = 0;
			file.tag.get_property("NAME", name);
			strncpy(sub_mask->name, name, sizeof(sub_mask->name));
			sub_mask->load(&file);
			masks.append(sub_mask);
		}
	}
}

void CWindowMaskGUI::save_masks(ArrayList<SubMask *> &masks)
{
	FileXML file;
	for( int i=0; i<masks.size(); ++i ) {
		SubMask *sub_mask = masks[i];
		sub_mask->copy(&file);
	}
	file.terminate_string();

        char path[BCTEXTLEN];
        sprintf(path, "%s/%s", File::get_config_path(), MASKS_FILE);
        FileSystem fs;
        fs.complete_path(path);
	file.write_to_file(path);
}


CWindowRulerGUI::CWindowRulerGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Ruler"), xS(320), yS(240))
{
// *** CONTEXT_HELP ***
	context_help_set_keyword("Compositor Toolbar");
}

CWindowRulerGUI::~CWindowRulerGUI()
{
}

void CWindowRulerGUI::create_objects()
{
	int xs10 = xS(10), xs200 = xS(200);
	int ys5 = yS(5), ys10 = yS(10);
	int x = xs10, y = ys10, x1 = xS(100);
	BC_Title *title;

	lock_window("CWindowRulerGUI::create_objects");
	add_subwindow(title = new BC_Title(x, y, _("Current:")));
	add_subwindow(current = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys5;
	add_subwindow(title = new BC_Title(x, y, _("Point 1:")));
	add_subwindow(point1 = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys5;
	add_subwindow(title = new BC_Title(x, y, _("Point 2:")));
	add_subwindow(point2 = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys5;
	add_subwindow(title = new BC_Title(x, y, _("Deltas:")));
	add_subwindow(deltas = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys5;
	add_subwindow(title = new BC_Title(x, y, _("Distance:")));
	add_subwindow(distance = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys5;
	add_subwindow(title = new BC_Title(x, y, _("Angle:")));
	add_subwindow(angle = new BC_TextBox(x1, y, xs200, 1, ""));
	y += title->get_h() + ys10;
	char string[BCTEXTLEN];
	sprintf(string,
		 _("Press Ctrl to lock ruler to the\nnearest 45%c%c angle."),
		0xc2, 0xb0); // degrees utf
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	y += title->get_h() + ys10;
	sprintf(string, _("Press Alt to translate the ruler."));
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	update();
	unlock_window();
}

void CWindowRulerGUI::update()
{
	char string[BCTEXTLEN];
	int cx = mwindow->session->cwindow_output_x;
	int cy = mwindow->session->cwindow_output_y;
	sprintf(string, "%d, %d", cx, cy);
	current->update(string);
	double x1 = mwindow->edl->session->ruler_x1;
	double y1 = mwindow->edl->session->ruler_y1;
	sprintf(string, "%.0f, %.0f", x1, y1);
	point1->update(string);
	double x2 = mwindow->edl->session->ruler_x2;
	double y2 = mwindow->edl->session->ruler_y2;
	sprintf(string, "%.0f, %.0f", x2, y2);
	point2->update(string);
	double dx = x2 - x1, dy = y2 - y1;
	sprintf(string, "%s%.0f, %s%.0f", (dx>=0? "+":""), dx, (dy>=0? "+":""), dy);
	deltas->update(string);
	double d = sqrt(dx*dx + dy*dy);
	sprintf(string, _("%0.01f pixels"), d);
	distance->update(string);
	double a = d > 0 ? (atan2(-dy, dx) * 180/M_PI) : 0.;
	sprintf(string, "%0.02f %c%c", a, 0xc2, 0xb0);
	angle->update(string);
}

void CWindowRulerGUI::handle_event()
{
}

