
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainsession.h"
#include "mainundo.h"
#include "manualgoto.h"
#include "mbuttons.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "preferences.h"
#include "scopewindow.h"
#include "theme.h"
#include "timebar.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vwindowgui.h"
#include "zoombar.h"



EditPanel::EditPanel(MWindow *mwindow,
	BC_WindowBase *subwindow,
	int window_id,
	int x,
	int y,
	int editing_mode,
	int use_editing_mode,
	int use_keyframe,
	int use_splice,   // Extra buttons
	int use_overwrite,
	int use_copy,
	int use_paste,
	int use_undo,
	int use_fit,
	int use_locklabels,
	int use_labels,
	int use_toclip,
	int use_meters,
	int use_cut,
	int use_commercial,
	int use_goto,
	int use_clk2play,
	int use_scope,
	int use_gang_tracks,
	int use_timecode)
{
	this->window_id = window_id;
	this->editing_mode = editing_mode;
	this->use_editing_mode = use_editing_mode;
	this->use_keyframe = use_keyframe;
	this->use_splice = use_splice;
	this->use_overwrite = use_overwrite;
	this->use_copy = use_copy;
	this->use_paste = use_paste;
	this->use_undo = use_undo;
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->use_fit = use_fit;
	this->use_labels = use_labels;
	this->use_locklabels = use_locklabels;
	this->use_toclip = use_toclip;
	this->use_meters = use_meters;
	this->use_cut = use_cut;
	this->use_commercial = use_commercial;
	this->use_goto = use_goto;
	this->use_clk2play = use_clk2play;
	this->use_scope = use_scope;
	this->use_gang_tracks = use_gang_tracks;
	this->use_timecode = use_timecode;

	this->x = x;
	this->y = y;
	this->fit = 0;
	this->fit_autos = 0;
	this->inpoint = 0;
	this->outpoint = 0;
	this->splice = 0;
	this->overwrite = 0;
	this->clip = 0;
	this->cut = 0;
	this->commercial = 0;
	this->copy = 0;
	this->paste = 0;
	this->labelbutton = 0;
	this->prevlabel = 0;
	this->nextlabel = 0;
	this->prevedit = 0;
	this->nextedit = 0;
	this->gang_tracks = 0;
	this->undo = 0;
	this->redo = 0;
	this->meter_panel = 0;
	this->meters = 0;
	this->arrow = 0;
	this->ibeam = 0;
	this->keyframe = 0;
	this->span_keyframe = 0;
	this->mangoto = 0;
	this->click2play = 0;
	this->scope = 0;
	this->scope_dialog = 0;
	locklabels = 0;
}

EditPanel::~EditPanel()
{
	delete scope_dialog;
}

void EditPanel::set_meters(MeterPanel *meter_panel)
{
	this->meter_panel = meter_panel;
}


void EditPanel::update()
{
	int new_editing_mode = mwindow->edl->session->editing_mode;
	if( arrow ) arrow->update(new_editing_mode == EDITING_ARROW);
	if( ibeam ) ibeam->update(new_editing_mode == EDITING_IBEAM);
	if( keyframe ) keyframe->update(mwindow->edl->session->auto_keyframes);
	if( span_keyframe ) span_keyframe->update(mwindow->edl->session->span_keyframes);
	if( locklabels ) locklabels->set_value(mwindow->edl->session->labels_follow_edits);
	if( click2play ) {
		int value = !is_vwindow() ?
			mwindow->edl->session->cwindow_click2play :
			mwindow->edl->session->vwindow_click2play ;
		click2play->set_value(value);
	}
	if( gang_tracks ) gang_tracks->update(mwindow->edl->local_session->gang_tracks);
	if( meters ) {
		if( is_cwindow() ) {
			meters->update(mwindow->edl->session->cwindow_meter);
			mwindow->cwindow->gui->update_meters();
		}
		else {
			meters->update(mwindow->edl->session->vwindow_meter);
		}
	}
	subwindow->flush();
}

int EditPanel::calculate_w(MWindow *mwindow, int use_keyframe, int total_buttons)
{
	int button_w = xS(24); // mwindow->theme->get_image_set("meters")[0]->get_w();
	int result = button_w * total_buttons;
	if( use_keyframe )
		result += 2*(button_w + mwindow->theme->toggle_margin);
	return result;
}

int EditPanel::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("meters")[0]->get_h();
}

void EditPanel::create_buttons()
{
	x1 = x, y1 = y;

	if( use_editing_mode ) {
		arrow = new ArrowButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(arrow);
		x1 += arrow->get_w();
		ibeam = new IBeamButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(ibeam);
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if( use_keyframe ) {
		keyframe = new KeyFrameButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(keyframe);
		x1 += keyframe->get_w();
		span_keyframe = new SpanKeyFrameButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(span_keyframe);
		x1 += span_keyframe->get_w();
	}

	if( use_locklabels ) {
		locklabels = new LockLabelsButton(mwindow, this, x1, y1);
		subwindow->add_subwindow(locklabels);
		x1 += locklabels->get_w();
	}

	if( use_keyframe || use_locklabels )
		x1 += mwindow->theme->toggle_margin;

// Mandatory
	inpoint = new EditInPoint(mwindow, this, x1, y1);
	subwindow->add_subwindow(inpoint);
	x1 += inpoint->get_w();
	outpoint = new EditOutPoint(mwindow, this, x1, y1);
	subwindow->add_subwindow(outpoint);
	x1 += outpoint->get_w();

	if( use_splice ) {
		splice = new EditSplice(mwindow, this, x1, y1);
		subwindow->add_subwindow(splice);
		x1 += splice->get_w();
	}

	if( use_overwrite ) {
		overwrite = new EditOverwrite(mwindow, this, x1, y1);
		subwindow->add_subwindow(overwrite);
		x1 += overwrite->get_w();
	}

	if( use_toclip ) {
		clip = new EditToClip(mwindow, this, x1, y1);
		subwindow->add_subwindow(clip);
		x1 += clip->get_w();
	}

	if( use_cut ) {
		cut = new EditCut(mwindow, this, x1, y1);
		subwindow->add_subwindow(cut);
		x1 += cut->get_w();
	}

	if( use_copy ) {
		copy = new EditCopy(mwindow, this, x1, y1);
		subwindow->add_subwindow(copy);
		x1 += copy->get_w();
	}

	if( use_paste ) {
		paste = new EditPaste(mwindow, this, x1, y1);
		subwindow->add_subwindow(paste);
		x1 += paste->get_w();
	}

	if( use_labels ) {
		labelbutton = new EditLabelbutton(mwindow, this, x1, y1);
		subwindow->add_subwindow(labelbutton);
		x1 += labelbutton->get_w();
		prevlabel = new EditPrevLabel(mwindow, this, x1, y1);
		subwindow->add_subwindow(prevlabel);
		x1 += prevlabel->get_w();
		nextlabel = new EditNextLabel(mwindow, this, x1, y1);
		subwindow->add_subwindow(nextlabel);
		x1 += nextlabel->get_w();
	}

// all windows except VWindow since it's only implemented in MWindow.
	if( use_cut ) {
		prevedit = new EditPrevEdit(mwindow, this, x1, y1);
		subwindow->add_subwindow(prevedit);
		x1 += prevedit->get_w();
		nextedit = new EditNextEdit(mwindow, this, x1, y1);
		subwindow->add_subwindow(nextedit);
		x1 += nextedit->get_w();
	}

	if( use_fit ) {
		fit = new EditFit(mwindow, this, x1, y1);
		subwindow->add_subwindow(fit);
		x1 += fit->get_w();
		fit_autos = new EditFitAutos(mwindow, this, x1, y1);
		subwindow->add_subwindow(fit_autos);
		x1 += fit_autos->get_w();
	}

	if( use_undo ) {
		undo = new EditUndo(mwindow, this, x1, y1);
		subwindow->add_subwindow(undo);
		x1 += undo->get_w();
		redo = new EditRedo(mwindow, this, x1, y1);
		subwindow->add_subwindow(redo);
		x1 += redo->get_w();
	}

	if( use_goto ) {
		mangoto = new EditManualGoto(mwindow, this, x1, y1);
		subwindow->add_subwindow(mangoto);
		x1 += mangoto->get_w();
	}

	if( use_clk2play ) {
		click2play = new EditClick2Play(mwindow, this, x1, y1+yS(3));
		subwindow->add_subwindow(click2play);
		x1 += click2play->get_w();
	}

	if( use_scope ) {
		scope = new EditPanelScope(mwindow, this, x1, y1-yS(1));
		subwindow->add_subwindow(scope);
		x1 += scope->get_w();
		scope_dialog = new EditPanelScopeDialog(mwindow, this);
	}

	if( use_timecode ) {
		timecode = new EditPanelTimecode(mwindow, this, x1, y1);
		subwindow->add_subwindow(timecode);
		x1 += timecode->get_w();
	}

	if( use_gang_tracks ) {
		gang_tracks = new EditPanelGangTracks(mwindow, this, x1, y1-yS(1));
		subwindow->add_subwindow(gang_tracks);
		x1 += gang_tracks->get_w();
	}

	if( use_meters ) {
		if( meter_panel ) {
			meters = new MeterShow(mwindow, meter_panel, x1, y1);
			subwindow->add_subwindow(meters);
			x1 += meters->get_w();
		}
		else
			printf("EditPanel::create_objects: meter_panel == 0\n");
	}

	if( use_commercial ) {
		commercial = new EditCommercial(mwindow, this, x1, y1);
		subwindow->add_subwindow(commercial);
		x1 += commercial->get_w();
	}
}

void EditPanel::reposition_buttons(int x, int y)
{
	this->x = x;
	this->y = y;
	x1 = x, y1 = y;

	if( use_editing_mode ) {
		arrow->reposition_window(x1, y1);
		x1 += arrow->get_w();
		ibeam->reposition_window(x1, y1);
		x1 += ibeam->get_w();
		x1 += mwindow->theme->toggle_margin;
	}

	if( use_keyframe ) {
		keyframe->reposition_window(x1, y1);
		x1 += keyframe->get_w();
		span_keyframe->reposition_window(x1, y1);
		x1 += span_keyframe->get_w();
	}

	if( use_locklabels ) {
		locklabels->reposition_window(x1,y1);
		x1 += locklabels->get_w();
	}

	if( use_keyframe || use_locklabels )
		x1 += mwindow->theme->toggle_margin;

	inpoint->reposition_window(x1, y1);
	x1 += inpoint->get_w();
	outpoint->reposition_window(x1, y1);
	x1 += outpoint->get_w();
	if( use_splice ) {
		splice->reposition_window(x1, y1);
		x1 += splice->get_w();
	}
	if( use_overwrite ) {
		overwrite->reposition_window(x1, y1);
		x1 += overwrite->get_w();
	}
	if( use_toclip ) {
		clip->reposition_window(x1, y1);
		x1 += clip->get_w();
	}
	if( use_cut ) {
		cut->reposition_window(x1, y1);
		x1 += cut->get_w();
	}
	if( use_copy ) {
		copy->reposition_window(x1, y1);
		x1 += copy->get_w();
	}
	if( use_paste ) {
		paste->reposition_window(x1, y1);
		x1 += paste->get_w();
	}

	if( use_labels ) {
		labelbutton->reposition_window(x1, y1);
		x1 += labelbutton->get_w();
		prevlabel->reposition_window(x1, y1);
		x1 += prevlabel->get_w();
		nextlabel->reposition_window(x1, y1);
		x1 += nextlabel->get_w();
	}

	if( prevedit ) {
		prevedit->reposition_window(x1, y1);
		x1 += prevedit->get_w();
	}

	if( nextedit ) {
		nextedit->reposition_window(x1, y1);
		x1 += nextedit->get_w();
	}

	if( use_fit ) {
		fit->reposition_window(x1, y1);
		x1 += fit->get_w();
		fit_autos->reposition_window(x1, y1);
		x1 += fit_autos->get_w();
	}

	if( use_undo ) {
		undo->reposition_window(x1, y1);
		x1 += undo->get_w();
		redo->reposition_window(x1, y1);
		x1 += redo->get_w();
	}

	if( use_goto ) {
		mangoto->reposition_window(x1, y1);
		x1 += mangoto->get_w();
	}
	if( use_clk2play ) {
		click2play->reposition_window(x1, y1+yS(3));
		x1 += click2play->get_w();
	}
	if( use_scope ) {
		scope->reposition_window(x1, y1-yS(1));
		x1 += scope->get_w();
	}
	if( use_timecode ) {
		timecode->reposition_window(x1, y1);
		x1 += timecode->get_w();
	}

	if( use_meters ) {
		meters->reposition_window(x1, y1);
		x1 += meters->get_w();
	}
}

void EditPanel::create_objects()
{
	create_buttons();
}

int EditPanel::get_w()
{
	return x1 - x;
}

// toggle_label
EditLabelbutton::EditLabelbutton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("labelbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Toggle label at current position ( l )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Labels");
}

EditLabelbutton::~EditLabelbutton()
{
}
int EditLabelbutton::keypress_event()
{
	if( get_keypress() == 'l' && !alt_down() )
		return handle_event();
	return context_help_check_and_show();
}
int EditLabelbutton::handle_event()
{
	panel->panel_toggle_label();
	return 1;
}

//next_label
EditNextLabel::EditNextLabel(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Next label ( ctrl -> )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Labels");
}
EditNextLabel::~EditNextLabel()
{
}
int EditNextLabel::keypress_event()
{
	if( ctrl_down() ) {
		int key = get_keypress();
		if( (key == RIGHT || key == '.') && !alt_down() ) {
			panel->panel_next_label(0);
			return 1;
		}
		if( key == '>' && alt_down() ) {
			panel->panel_next_label(1);
			return 1;
		}
	}
	return context_help_check_and_show();
}
int EditNextLabel::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_next_label(cut);
	return 1;
}

//prev_label
EditPrevLabel::EditPrevLabel(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevlabel"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Previous label ( ctrl <- )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Labels");
}
EditPrevLabel::~EditPrevLabel()
{
}
int EditPrevLabel::keypress_event()
{
	if( ctrl_down() ) {
		int key = get_keypress();
		if( (key == LEFT || key == ',') && !alt_down() ) {
			panel->panel_prev_label(0);
			return 1;
		}
		if( key == '<' && alt_down() ) {
			panel->panel_prev_label(1);
			return 1;
		}
	}
	return context_help_check_and_show();
}
int EditPrevLabel::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_prev_label(cut);
	return 1;
}

//prev_edit
EditPrevEdit::EditPrevEdit(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("prevedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Previous edit (alt <- )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Snapping while Cutting and Dragging");
}
EditPrevEdit::~EditPrevEdit()
{
}
int EditPrevEdit::keypress_event()
{
	if( alt_down() ) {
		int key = get_keypress();
		if( (key == LEFT || key == ',') && !ctrl_down() ) {
			panel->panel_prev_edit(0);
			return 1;
		}
		if( key == ',' && ctrl_down() ) {
			panel->panel_prev_edit(1);
			return 1;
		}
	}
	return context_help_check_and_show();
}
int EditPrevEdit::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_prev_edit(cut);
	return 1;
}

//next_edit
EditNextEdit::EditNextEdit(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("nextedit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Next edit ( alt -> )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Snapping while Cutting and Dragging");
}
EditNextEdit::~EditNextEdit()
{
}
int EditNextEdit::keypress_event()
{
	if( alt_down() ) {
		int key = get_keypress();
		if( (key == RIGHT || key == '.') && !ctrl_down() ) {
			panel->panel_next_edit(0);
			return 1;
		}
		if( key == '.' && ctrl_down() ) {
			panel->panel_next_edit(1);
			return 1;
		}
	}
	return context_help_check_and_show();
}
int EditNextEdit::handle_event()
{
	int cut = ctrl_down() && alt_down();
	panel->panel_next_edit(cut);
	return 1;
}

//copy_selection
EditCopy::EditCopy(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("copy"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Copy ( c )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Cut and Paste Editing");
}
EditCopy::~EditCopy()
{
}

int EditCopy::keypress_event()
{
	if( alt_down() ) return context_help_check_and_show();
	if( (get_keypress() == 'c' && !ctrl_down()) ||
	    (panel->is_vwindow() && get_keypress() == 'C') ) {
		return handle_event();
	}
	return context_help_check_and_show();
}
int EditCopy::handle_event()
{
	panel->panel_copy_selection();
	return 1;
}

//overwrite_selection
EditOverwrite::EditOverwrite(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->overwrite_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Overwrite ( b )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Two Screen Editing");
}
EditOverwrite::~EditOverwrite()
{
}
int EditOverwrite::handle_event()
{
	panel->panel_overwrite_selection();
	return 1;
}
int EditOverwrite::keypress_event()
{
	if( alt_down() ) return context_help_check_and_show();
	if( get_keypress() == 'b' ||
	    (panel->is_vwindow() && get_keypress() == 'B') ) {
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

//set_inpoint
//unset_inoutpoint
EditInPoint::EditInPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("inbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("In point ( [ or < )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("In\\/Out Points");
}
EditInPoint::~EditInPoint()
{
}
int EditInPoint::handle_event()
{
	panel->panel_set_inpoint();
	return 1;
}
int EditInPoint::keypress_event()
{
	int key = get_keypress();
	if( ctrl_down() ) {
		if( key == 't' ) {
			panel->panel_unset_inoutpoint();
			return 1;
		}
	}
	else if( !alt_down() ) {
		if( key == '[' || key == '<' ) {
			panel->panel_set_inpoint();
			return 1;
		}
	}
	return context_help_check_and_show();
}

//set_outpoint
//unset_inoutpoint
EditOutPoint::EditOutPoint(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("outbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Out point ( ] or > )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("In\\/Out Points");
}
EditOutPoint::~EditOutPoint()
{
}
int EditOutPoint::handle_event()
{
	panel->panel_set_outpoint();
	return 1;
}
int EditOutPoint::keypress_event()
{
	int key = get_keypress();
	if( ctrl_down() ) {
		if(  key == 't' ) {
			panel->panel_unset_inoutpoint();
			return 1;
		}
	}
	else if( !alt_down() ) {
		if( key == ']' || key == '>' ) {
			panel->panel_set_outpoint();
			return 1;
		}
	}
	return context_help_check_and_show();
}

//splice_selection
EditSplice::EditSplice(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->splice_data)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Splice ( v )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Two Screen Editing");
}
EditSplice::~EditSplice()
{
}
int EditSplice::handle_event()
{
	panel->panel_splice_selection();
	return 1;
}
int EditSplice::keypress_event()
{
	if( alt_down() ) return context_help_check_and_show();
	if( (get_keypress() == 'v' && !ctrl_down()) ||
	    (panel->is_vwindow() && get_keypress() == 'V') ) {
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

//to_clip
EditToClip::EditToClip(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("toclip"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("To clip ( i )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Two Screen Editing");
}
EditToClip::~EditToClip()
{
}
int EditToClip::handle_event()
{
	panel->panel_to_clip();
	return 1;
}

int EditToClip::keypress_event()
{
	if( alt_down() ) return context_help_check_and_show();
	if( get_keypress() == 'i' ||
	    (panel->is_vwindow() && get_keypress() == 'I') ) {
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}

//cut
EditCut::EditCut(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("cut"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Split | Cut ( x )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Cut and Paste Editing");
}
EditCut::~EditCut()
{
}
int EditCut::keypress_event()
{
	if( ctrl_down() || shift_down() || alt_down() )
		return context_help_check_and_show();
	if( get_keypress() == 'x' )
		return handle_event();
	return context_help_check_and_show();
}

int EditCut::handle_event()
{
	panel->panel_cut();
	return 1;
}

//paste
EditPaste::EditPaste(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("paste"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Paste ( v )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Cut and Paste Editing");
}
EditPaste::~EditPaste()
{
}

int EditPaste::keypress_event()
{
	if( get_keypress() == 'v' && !ctrl_down() )
		return handle_event();
	return context_help_check_and_show();
}
int EditPaste::handle_event()
{
	panel->panel_paste();
	return 1;
}

//fit_selection
EditFit::EditFit(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fit"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit selection to display ( f )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transport and Buttons Bar");
}
EditFit::~EditFit()
{
}
int EditFit::keypress_event()
{
	if( !alt_down() && get_keypress() == 'f' ) {
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}
int EditFit::handle_event()
{
	panel->panel_fit_selection();
	return 1;
}

//fit_autos
EditFitAutos::EditFitAutos(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fitautos"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Fit all autos to display ( Alt + f )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Using Autos");
}
EditFitAutos::~EditFitAutos()
{
}
int EditFitAutos::keypress_event()
{
	if( get_keypress() == 'f' && alt_down() ) {
		panel->panel_fit_autos(!ctrl_down() ? 1 : 0);
		return 1;
	}
	return context_help_check_and_show();
}
int EditFitAutos::handle_event()
{
	panel->panel_fit_autos(1);
	return 1;
}

//set_editing_mode
ArrowButton::ArrowButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("arrow"),
	mwindow->edl->session->editing_mode == EDITING_ARROW,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Drag and drop editing mode"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Drag and Drop Editing");
}

int ArrowButton::handle_event()
{
	update(1);
	panel->ibeam->update(0);
	panel->panel_set_editing_mode(EDITING_ARROW);
// Nothing after this
	return 1;
}

IBeamButton::IBeamButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("ibeam"),
	mwindow->edl->session->editing_mode == EDITING_IBEAM,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Cut and paste editing mode"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Cut and Paste Editing");
}

int IBeamButton::handle_event()
{
	update(1);
	panel->arrow->update(0);
	panel->panel_set_editing_mode(EDITING_IBEAM);
// Nothing after this
	return 1;
}

//set_auto_keyframes
KeyFrameButton::KeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("autokeyframe"),
	mwindow->edl->session->auto_keyframes,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Generate keyframes while tweeking (j)"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Generate Keyframes while Tweaking");
}

int KeyFrameButton::handle_event()
{
	panel->panel_set_auto_keyframes(get_value());
	return 1;
}

int KeyFrameButton::keypress_event()
{
	int key = get_keypress();
	if( key == 'j' && !ctrl_down() && !shift_down() && !alt_down() ) {
		int value = get_value() ? 0 : 1;
		update(value);
		panel->panel_set_auto_keyframes(value);
		return 1;
	}
	return context_help_check_and_show();
}

//set_span_keyframes
SpanKeyFrameButton::SpanKeyFrameButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("spankeyframe"),
	mwindow->edl->session->span_keyframes,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Allow keyframe spanning"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Allow Keyframe Spanning");
}

int SpanKeyFrameButton::handle_event()
{
	panel->panel_set_span_keyframes(get_value());
	return 1;
}

//set_labels_follow_edits
LockLabelsButton::LockLabelsButton(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y,
	mwindow->theme->get_image_set("locklabels"),
	mwindow->edl->session->labels_follow_edits,
	"", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Lock labels from moving with edits"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Labels");
}

int LockLabelsButton::handle_event()
{
	panel->panel_set_labels_follow_edits(get_value());
	return 1;
}



EditManualGoto::EditManualGoto(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("goto"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	mangoto = new ManualGoto(mwindow, panel);
	set_tooltip(_("Manual goto ( g )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transport and Buttons Bar");
}
EditManualGoto::~EditManualGoto()
{
	delete mangoto;
}
int EditManualGoto::handle_event()
{
	mangoto->start();
	return 1;
}

int EditManualGoto::keypress_event()
{
	if( get_keypress() == 'g' ) {
		handle_event();
		return 1;
	}
	return context_help_check_and_show();
}


EditClick2Play::EditClick2Play(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("playpatch_data"),
    !panel->is_vwindow() ?
	mwindow->edl->session->cwindow_click2play :
	mwindow->edl->session->vwindow_click2play)
{
        this->mwindow = mwindow;
        this->panel = panel;
        set_tooltip(_("Click to play (p)"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Click to Play in Viewer and Compositor");
}
int EditClick2Play::handle_event()
{
	int value = get_value();
	panel->set_click_to_play(value);
	return 1;
}
int EditClick2Play::keypress_event()
{
	int key = get_keypress();
	if( key == 'p' && !ctrl_down() && !shift_down() && !alt_down() ) {
		int value = get_value() ? 0 : 1;
		update(value);
		panel->set_click_to_play(value);
		return 1;
	}
	return context_help_check_and_show();
}


EditCommercial::EditCommercial(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("commercial"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Commercial ( shift A )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("The commercial DB");
}
EditCommercial::~EditCommercial()
{
}
int EditCommercial::keypress_event()
{
	if( ctrl_down() || !shift_down() || alt_down() )
		return context_help_check_and_show();
	if( get_keypress() == 'A' )
		return handle_event();
	return context_help_check_and_show();
}

int EditCommercial::handle_event()
{
	int have_mwindow_lock = mwindow->gui->get_window_lock();
	if( have_mwindow_lock )
		mwindow->gui->unlock_window();
	mwindow->commit_commercial();
	if( !mwindow->put_commercial() ) {
		mwindow->gui->lock_window("EditCommercial::handle_event 1");
		mwindow->cut();
		if( !have_mwindow_lock )
			mwindow->gui->unlock_window();
		mwindow->activate_commercial();
		return 1;
	}
	mwindow->undo_commercial();
	if( have_mwindow_lock )
		mwindow->gui->lock_window("EditCommercial::handle_event 2");
	return 1;
}


EditUndo::EditUndo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("undo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Undo ( z or Ctrl-z)"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transport and Buttons Bar");
}
EditUndo::~EditUndo()
{
}
int EditUndo::keypress_event()
{
	if( ctrl_down() || shift_down() || alt_down() )
		return context_help_check_and_show();
	if( get_keypress() == 'z' )
		return handle_event();
	return context_help_check_and_show();
}
int EditUndo::handle_event()
{
	mwindow->undo_entry(panel->subwindow);
	return 1;
}

EditRedo::EditRedo(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("redo"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("Redo ( shift Z )"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Transport and Buttons Bar");
}
EditRedo::~EditRedo()
{
}
int EditRedo::keypress_event()
{
	if( ctrl_down() || !shift_down() || alt_down() )
		return context_help_check_and_show();
	if( get_keypress() == 'Z' )
		return handle_event();
	return context_help_check_and_show();
}
int EditRedo::handle_event()
{
	mwindow->redo_entry(panel->subwindow);
	return 1;
}


EditPanelScopeDialog::EditPanelScopeDialog(MWindow *mwindow, EditPanel *panel)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->panel = panel;
	scope_gui = 0;
	gui_lock = new Mutex("EditPanelScopeDialog::gui_lock");
}

EditPanelScopeDialog::~EditPanelScopeDialog()
{
	close_window();
	delete gui_lock;
}

void EditPanelScopeDialog::handle_close_event(int result)
{
	scope_gui = 0;
}
void EditPanelScopeDialog::handle_done_event(int result)
{
	gui_lock->lock("EditPanelScopeDialog::handle_done_event");
	scope_gui = 0;
	gui_lock->unlock();

	panel->subwindow->lock_window("EditPanelScopeDialog::handle_done_event");
	panel->scope->update(0);
	panel->subwindow->unlock_window();
}

BC_Window* EditPanelScopeDialog::new_gui()
{
	EditPanelScopeGUI *gui = new EditPanelScopeGUI(mwindow, this);
	gui->create_objects();
	scope_gui = gui;
	return gui;
}

void EditPanelScopeDialog::process(VFrame *output_frame)
{
	if( panel->scope_dialog ) {
		panel->scope_dialog->gui_lock->lock("EditPanelScopeDialog::process");
		if( panel->scope_dialog->scope_gui ) {
			EditPanelScopeGUI *gui = panel->scope_dialog->scope_gui;
			gui->process(output_frame);
		}
		panel->scope_dialog->gui_lock->unlock();
	}
}

EditPanelScopeGUI::EditPanelScopeGUI(MWindow *mwindow, EditPanelScopeDialog *dialog)
 : ScopeGUI(mwindow->theme,
	mwindow->session->scope_x, mwindow->session->scope_y,
	mwindow->session->scope_w, mwindow->session->scope_h,
	mwindow->get_cpus())
{
	this->mwindow = mwindow;
	this->dialog = dialog;
}

EditPanelScopeGUI::~EditPanelScopeGUI()
{
}

void EditPanelScopeGUI::create_objects()
{
	MainSession *session = mwindow->session;
	use_hist = session->use_hist;
	use_wave = session->use_wave;
	use_vector = session->use_vector;
	use_hist_parade = session->use_hist_parade;
	use_wave_parade = session->use_wave_parade;
	use_wave_gain = session->use_wave_gain;
	use_vect_gain = session->use_vect_gain;
	use_smooth = session->use_smooth;
	use_refresh = session->use_refresh;
	use_release = session->use_release;
	use_graticule = session->use_graticule;
	ScopeGUI::create_objects();
}

void EditPanelScopeGUI::toggle_event()
{
	MainSession *session = mwindow->session;
	session->use_hist = use_hist;
	session->use_wave = use_wave;
	session->use_vector = use_vector;
	session->use_hist_parade = use_hist_parade;
	session->use_wave_parade = use_wave_parade;
	session->use_wave_gain = use_wave_gain;
	session->use_vect_gain = use_vect_gain;
	session->use_smooth = use_smooth;
	session->use_refresh = use_refresh;
	session->use_release = use_release;
	session->use_graticule = use_graticule;
}

int EditPanelScopeGUI::translation_event()
{
	ScopeGUI::translation_event();
	MainSession *session = mwindow->session;
	session->scope_x = get_x();
	session->scope_y = get_y();
	return 0;
}

int EditPanelScopeGUI::resize_event(int w, int h)
{
	ScopeGUI::resize_event(w, h);
	MainSession *session = mwindow->session;
	session->scope_w = w;
	session->scope_h = h;
	return 0;
}

EditPanelScope::EditPanelScope(MWindow *mwindow, EditPanel *panel, int x, int y)
 : BC_Toggle(x, y, mwindow->theme ?
		mwindow->theme->get_image_set("scope_toggle") : 0, 0)
{
	this->mwindow = mwindow;
	this->panel = panel;
	set_tooltip(_("View scope"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Videoscope");
}

EditPanelScope::~EditPanelScope()
{
}

void EditPanelScopeGUI::update_scope()
{
	Canvas *canvas = 0;
	if( dialog->panel->is_cwindow() ) {
		CWindowGUI *cgui = (CWindowGUI *)dialog->panel->subwindow;
		canvas = cgui->canvas;
	}
	else if( dialog->panel->is_vwindow() ) {
		VWindowGUI *vgui = (VWindowGUI *)dialog->panel->subwindow;
		canvas = vgui->canvas;
	}
	if( canvas && canvas->refresh_frame )
		process(canvas->refresh_frame);
}

int EditPanelScope::handle_event()
{
	unlock_window();
	int v = get_value();
	if( v )
		panel->scope_dialog->start();
	else
		panel->scope_dialog->close_window();
	lock_window("EditPanelScope::handle_event");
	return 1;
}

const char *EditPanelGangTracks::gang_tips[TOTAL_GANGS] = {
	N_("Currently: Gang None\n  Click to: Gang Channels"),
	N_("Currently: Gang Channels\n  Click to: Gang Media"),
	N_("Currently: Gang Media\n  Click to: Gang None"),
};

EditPanelGangTracks::EditPanelGangTracks(MWindow *mwindow, EditPanel *panel,
		int x, int y)
 : BC_Button(x, y, get_images(mwindow))
{
	this->mwindow = mwindow;
	this->panel = panel;
	int gang = mwindow->edl->local_session->gang_tracks;
	set_tooltip(_(gang_tips[gang]));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Displaying tracks: Ganged mode");
}

EditPanelGangTracks::~EditPanelGangTracks()
{
}

VFrame **EditPanelGangTracks::gang_images[TOTAL_GANGS];

VFrame **EditPanelGangTracks::get_images(MWindow *mwindow)
{
	gang_images[GANG_NONE] = mwindow->theme->get_image_set("gang0");
	gang_images[GANG_MEDIA] = mwindow->theme->get_image_set("gang1");
	gang_images[GANG_CHANNELS] = mwindow->theme->get_image_set("gang2");
	int gang = mwindow->edl->local_session->gang_tracks;
	return gang_images[gang];
}

void EditPanelGangTracks::update(int gang)
{
	set_images(gang_images[gang]);
	draw_face();
	set_tooltip(_(gang_tips[gang]));
}

int EditPanelGangTracks::handle_event()
{
	int gang = mwindow->edl->local_session->gang_tracks;
	if( !shift_down() ) {
		if( ++gang > GANG_MEDIA ) gang = GANG_NONE;
	}
	else {
		if( --gang < GANG_NONE ) gang = GANG_MEDIA;
	}
	update(gang);
	panel->panel_set_gang_tracks(gang);
	return 1;
}


EditPanelTimecode::EditPanelTimecode(MWindow *mwindow,
	EditPanel *panel, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("clapperbutton"))
{
	this->mwindow = mwindow;
	this->panel = panel;
	tc_dialog = 0;
	set_tooltip(_("Set Timecode"));
// *** CONTEXT_HELP ***
	context_help_set_keyword("Align Timecodes");
}

EditPanelTimecode::~EditPanelTimecode()
{
	delete tc_dialog;
}

int EditPanelTimecode::handle_event()
{
	if( !tc_dialog )
		tc_dialog = new EditPanelTcDialog(mwindow, panel);
	int px, py;
	get_pop_cursor(px, py, 0);
	tc_dialog->start_dialog(px, py);
	return 1;
}

EditPanelTcDialog::EditPanelTcDialog(MWindow *mwindow, EditPanel *panel)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->panel = panel;
	tc_gui = 0;
	px = py = 0;
}

EditPanelTcDialog::~EditPanelTcDialog()
{
	close_window();
}

#define TCW_W xS(200)
#define TCW_H yS(120)

void EditPanelTcDialog::start_dialog(int px, int py)
{
	this->px = px - TCW_W/2;
	this->py = py - TCW_H/2;
	start();
}

BC_Window *EditPanelTcDialog::new_gui()
{
	tc_gui = new EditPanelTcWindow(this, px, py);
	tc_gui->create_objects();
	double timecode = mwindow->get_timecode_offset();
	tc_gui->update(timecode);
	tc_gui->show_window();
	return tc_gui;
}

void EditPanelTcDialog::handle_done_event(int result)
{
	if( result ) return;
	double ofs = tc_gui->get_timecode();
	mwindow->set_timecode_offset(ofs);
}

EditPanelTcWindow::EditPanelTcWindow(EditPanelTcDialog *tc_dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Timecode"), x, y,
	TCW_W, TCW_H, TCW_W, TCW_H, 0, 0, 1)
{
	this->tc_dialog = tc_dialog;
// *** CONTEXT_HELP ***
	context_help_set_keyword("Align Timecodes");
}

EditPanelTcWindow::~EditPanelTcWindow()
{
}

double EditPanelTcWindow::get_timecode()
{
	int hrs = atoi(hours->get_text());
	int mins = atoi(minutes->get_text());
	int secs = atoi(seconds->get_text());
	int frms = atoi(frames->get_text());
	double frame_rate = tc_dialog->mwindow->edl->session->frame_rate;
	double timecode = hrs*3600 + mins*60 + secs + frms/frame_rate;
	return timecode;
}

void EditPanelTcWindow::update(double timecode)
{
	if( timecode < 0 ) timecode = 0;
	int64_t pos = timecode;
	int hrs = pos/3600;
	int mins = pos/60 - hrs*60;
	int secs = pos - hrs*3600 - mins*60;
	double frame_rate = tc_dialog->mwindow->edl->session->frame_rate;
	int frms = (timecode-pos) * frame_rate;
	hours->update(hrs);
	minutes->update(mins);
	seconds->update(secs);
	frames->update(frms);
}

void EditPanelTcWindow::create_objects()
{
	lock_window("EditPanelTcWindow::create_objects");
	int x = xS(20), y = yS(5);
	BC_Title *title = new BC_Title(x - 2, y, _("hour  min   sec   frms"), SMALLFONT);
	add_subwindow(title);  y += title->get_h() + xS(3);
	hours = new EditPanelTcInt(this, x, y, xS(26), 99, "%02i");
	add_subwindow(hours);    x += hours->get_w() + xS(4);
	minutes = new EditPanelTcInt(this, x, y, xS(26), 59, "%02i");
	add_subwindow(minutes);  x += minutes->get_w() + xS(4);
	seconds = new EditPanelTcInt(this, x, y, xS(26), 60, "%02i");
	add_subwindow(seconds);  x += seconds->get_w() + xS(4);
	frames = new EditPanelTcInt(this, x, y, xS(34), 999, "%03i");
	add_subwindow(frames);   x += frames->get_w() + xS(16);
	add_subwindow(new EditPanelTcReset(this, x, y));
	double timecode = tc_dialog->mwindow->get_timecode_offset();
	update(timecode);
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	unlock_window();
}

EditPanelTcReset::EditPanelTcReset(EditPanelTcWindow *window, int x, int y)
 : BC_Button(x, y, window->tc_dialog->mwindow->theme->get_image_set("reset_button"))
{
	this->window = window;
}

int EditPanelTcReset::handle_event()
{
	window->update(0);
	return 1;
}


EditPanelTcInt::EditPanelTcInt(EditPanelTcWindow *window, int x, int y, int w,
	int max, const char *format)
 : BC_TextBox(x, y, w, 1, "")
{
	this->window = window;
	this->max = max;
	this->format = format;
	digits = 1;
	for( int m=max; (m/=10)>0; ++digits );
}

EditPanelTcInt::~EditPanelTcInt()
{
}

int EditPanelTcInt::handle_event()
{
	int v = atoi(get_text());
	if( v > max ) {
		v = v % (max+1);
		char string[BCSTRLEN];
		sprintf(string, format, v);
		BC_TextBox::update(string);
	}
	return 1;
}

void EditPanelTcInt::update(int v)
{
	char text[BCTEXTLEN];
	if( v > max ) v = max;
	sprintf(text, format, v);
	BC_TextBox::update(text);
}

int EditPanelTcInt::keypress_event()
{
	if( get_keypress() == 'h' && alt_down() ) {
		context_help_show("Align Timecodes");
		return 1;
	}

	if( (int)strlen(get_text()) >= digits )
		BC_TextBox::update("");
	int key = get_keypress();
	switch( key ) {
	case TAB:   case LEFTTAB:
	case LEFT:  case RIGHT:
	case HOME:  case END:
	case BACKSPACE:
	case DELETE:
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return BC_TextBox::keypress_event();
	}
	return 1;
}

