
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

#include "asset.h"
#include "assets.h"
#include "awindow.h"
#include "awindowgui.h"
#include "edit.h"
#include "edits.h"
#include "editpopup.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"

#include <string.h>

EditPopup::EditPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;

	track = 0;
	edit = 0;
	plugin = 0;
	pluginset = 0;
	position = 0;
	open_edl = 0;
}

EditPopup::~EditPopup()
{
}

void EditPopup::create_objects()
{
	add_item(open_edl = new EditPopupOpenEDL(mwindow, this));
	add_item(new EditPopupClearSelect(mwindow, this));
	add_item(new EditPopupSelectEdits(mwindow, this));
	add_item(new EditPopupDeselectEdits(mwindow, this));
	add_item(new EditPopupCopy(mwindow, this));
	add_item(new EditPopupCut(mwindow, this));
	add_item(new EditPopupMute(mwindow, this));
	add_item(new EditPopupCopyPack(mwindow, this));
	add_item(new EditPopupCutPack(mwindow, this));
	add_item(new EditPopupMutePack(mwindow, this));
	add_item(new EditPopupPaste(mwindow, this));
	add_item(new EditPopupOverwrite(mwindow, this));
	add_item(new BC_MenuItem("-"));
	add_item(new EditPopupOverwritePlugins(mwindow, this));
	add_item(new EditCollectEffects(mwindow, this));
	add_item(new EditPasteEffects(mwindow, this));
	add_item(new EditPopupTimecode(mwindow, this));
}

int EditPopup::activate_menu(Track *track, Edit *edit,
		PluginSet *pluginset, Plugin *plugin, double position)
{
	this->track = track;
	this->edit = edit;
	this->pluginset = pluginset;
	this->plugin = plugin;
	this->position = position;
	int enable = !edit ? 0 :
		edit->nested_edl ? 1 :
		!edit->asset ? 0 :
		edit->asset->format == FILE_REF ? 1 : 0;
	open_edl->set_enabled(enable);
	return BC_PopupMenu::activate_menu();
}

EditPopupOpenEDL::EditPopupOpenEDL(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Open EDL"))
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupOpenEDL::handle_event()
{
	Edit *edit = popup->edit;
	if( !edit ) return 1;
	EDL *edl = 0;
	Indexable *idxbl = 0;
	if( edit->asset && edit->asset->format == FILE_REF ) {
		FileXML xml_file;
		const char *filename = edit->asset->path;
		if( xml_file.read_from_file(filename, 1) ) {
			eprintf(_("Error: unable to open:\n  %s"), filename);
			return 1;
		}
		edl = new EDL;
		edl->create_objects();
		if( edl->load_xml(&xml_file, LOAD_ALL) ) {
			eprintf(_("Error: unable to load:\n  %s"), filename);
			edl->remove_user();
			return 1;
		}
		idxbl = edit->asset;
	}
	else if( edit->nested_edl ) {
		edl = edit->nested_edl;
		edl->add_user();
		idxbl = edl;
	}
	else {
		char edit_title[BCTEXTLEN];
		edit->get_title(edit_title);
		eprintf(_("Edit is not EDL: %s"), edit_title);
		return 1;
	}
	mwindow->stack_push(edl, idxbl);
	return 1;
}

EditPopupClearSelect::EditPopupClearSelect(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Clear Select"),_("Ctrl-Shift-A"),'A')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupClearSelect::handle_event()
{
	mwindow->clear_select();
	return 1;
}

EditPopupSelectEdits::EditPopupSelectEdits(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Select Edits"),_("Ctrl-Alt-'"),'\'')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_alt(1);
}

int EditPopupSelectEdits::handle_event()
{
	mwindow->select_edits(1);
	return 1;
}

EditPopupDeselectEdits::EditPopupDeselectEdits(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Deselect Edits"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupDeselectEdits::handle_event()
{
	mwindow->select_edits(0);
	return 1;
}

EditPopupCopy::EditPopupCopy(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Copy"),_("Ctrl-c"),'c')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupCopy::handle_event()
{
	mwindow->selected_edits_to_clipboard(0);
	return 1;
}

EditPopupCopyPack::EditPopupCopyPack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Copy pack"),_("Ctrl-Shift-C"),'C')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupCopyPack::handle_event()
{
	mwindow->selected_edits_to_clipboard(1);
	return 1;
}

EditPopupCut::EditPopupCut(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Cut"),_("Ctrl-x"),'x')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupCut::handle_event()
{
	mwindow->cut_selected_edits(1, 0);
	return 1;
}

EditPopupCutPack::EditPopupCutPack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Cut pack"),_("Ctrl-Alt-z"),'z')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_alt();
}

int EditPopupCutPack::handle_event()
{
	mwindow->cut_selected_edits(1, 1);
	return 1;
}

EditPopupMute::EditPopupMute(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(C_("Mute"),_("Ctrl-m"),'m')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupMute::handle_event()
{
	mwindow->cut_selected_edits(0, 0);
	return 1;
}

EditPopupMutePack::EditPopupMutePack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Mute pack"),_("Ctrl-Shift-M"),'M')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupMutePack::handle_event()
{
	mwindow->cut_selected_edits(0, 1);
	return 1;
}

EditPopupPaste::EditPopupPaste(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Paste"),_("Ctrl-v"),'v')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupPaste::handle_event()
{
	mwindow->paste(popup->position, popup->track, 0, 0);
	mwindow->edl->tracks->clear_selected_edits();
	popup->gui->draw_overlays(1);
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}

EditPopupOverwrite::EditPopupOverwrite(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Overwrite"),_("Ctrl-b"),'b')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupOverwrite::handle_event()
{
	mwindow->paste(popup->position, popup->track, 0, -1);
	mwindow->edl->tracks->clear_selected_edits();
	popup->gui->draw_overlays(1);
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}

EditPopupOverwritePlugins::EditPopupOverwritePlugins(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Overwrite Plugins"),_("Ctrl-Shift-P"),'P')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupOverwritePlugins::handle_event()
{
	mwindow->paste_clipboard(popup->track, popup->position, 1, 0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->autos_follow_edits,
			mwindow->edl->session->plugins_follow_edits);
	mwindow->edl->tracks->clear_selected_edits();
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}


EditCollectEffects::EditCollectEffects(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Collect Effects"))
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditCollectEffects::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->collect_effects();
	return 1;
}

EditPasteEffects::EditPasteEffects(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Paste Effects"))
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPasteEffects::handle_event()
{
	if( mwindow->session->current_operation == NO_OPERATION )
		mwindow->paste_effects();
	return 1;
}

EditPopupTimecode::EditPopupTimecode(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Timecode"),_("Ctrl-!"),'!')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupTimecode::handle_event()
{
	if( mwindow->session->current_operation != NO_OPERATION ) return 1;
	Edit *edit = popup->edit;
	if( !edit || !edit->asset ) return 1;
	Asset *asset = edit->asset;
	double timecode = asset->timecode != -2 ? asset->timecode :
		FFMPEG::get_timecode(asset->path,
			edit->track->data_type, edit->channel,
			mwindow->edl->session->frame_rate);
	asset->timecode = timecode;
	if( timecode >= 0 ) {
		int64_t pos = edit->startproject + edit->startsource;
		double position = edit->track->from_units(pos);
		mwindow->set_timecode_offset(timecode - position);
	}
	else
		mwindow->set_timecode_offset(0);
	return 1;
}

