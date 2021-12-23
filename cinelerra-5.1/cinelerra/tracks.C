/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "atrack.h"
#include "automation.h"
#include "clip.h"
#include "bchash.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "intauto.h"
#include "intautos.h"
#include "localsession.h"
#include "module.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "plugin.h"
#include "mainsession.h"
#include "strack.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.inc"
#include "vtrack.h"
#include <string.h>

Tracks::Tracks(EDL *edl)
 : List<Track>()
{
	this->edl = edl;
}

Tracks::Tracks()
 : List<Track>()
{
}


Tracks::~Tracks()
{
	delete_all_tracks();
}





void Tracks::equivalent_output(Tracks *tracks, double *result)
{
	if(total_playable_vtracks() != tracks->total_playable_vtracks())
	{
		*result = 0;
	}
	else
	{
		Track *current = first;
		Track *that_current = tracks->first;
		while(current || that_current)
		{
// Get next video track
			while(current && current->data_type != TRACK_VIDEO)
				current = NEXT;

			while(that_current && that_current->data_type != TRACK_VIDEO)
				that_current = that_current->next;

// One doesn't exist but the other does
			if((!current && that_current) ||
				(current && !that_current))
			{
				*result = 0;
				break;
			}
			else
// Both exist
			if(current && that_current)
			{
				current->equivalent_output(that_current, result);
				current = NEXT;
				that_current = that_current->next;
			}
		}
	}
}


void Tracks::clear_selected_edits()
{
	for( Track *track=first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next )
			edit->is_selected = 0;
	}
}

void Tracks::get_selected_edits(ArrayList<Edit*> *drag_edits)
{
	drag_edits->remove_all();
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->is_selected ) continue;
			drag_edits->append(edit);
		}
	}
}

void Tracks::select_edits(double start, double end, int v)
{
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		int64_t start_pos = track->to_units(start, 0);
		int64_t end_pos = track->to_units(end, 0);
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( start_pos >= edit->startproject+edit->length ) continue;
			if( edit->startproject >= end_pos ) continue;
			edit->is_selected = v > 1 ? 1 : v < 0 ? 0 : !edit->is_selected ;
		}
	}
}

void Tracks::get_automation_extents(float *min,
	float *max,
	double start,
	double end,
	int autogrouptype)
{
	*min = 0;
	*max = 0;
	int coords_undefined = 1;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->is_armed())
		{
			current->automation->get_extents(min,
				max,
				&coords_undefined,
				current->to_units(start, 0),
				current->to_units(end, 1),
				autogrouptype);
		}
	}
}


void Tracks::copy_from(Tracks *tracks)
{
	Track *new_track = 0;

	delete_all_tracks();
	int solo_track_id = tracks->edl->local_session->solo_track_id;

	for(Track *current = tracks->first; current; current = NEXT)
	{
		switch(current->data_type)
		{
		case TRACK_AUDIO:
			new_track = add_audio_track(0, 0);
			break;
		case TRACK_VIDEO:
			new_track = add_video_track(0, 0);
			break;
		case TRACK_SUBTITLE:
			new_track = add_subttl_track(0, 0);
			break;
		default:
			continue;
		}
		new_track->copy_from(current);

		if( current->get_id() == solo_track_id )
			edl->local_session->solo_track_id = new_track->get_id();
	}
}

Tracks& Tracks::operator=(Tracks &tracks)
{
printf("Tracks::operator= 1\n");
	copy_from(&tracks);
	return *this;
}

int Tracks::load(FileXML *xml,
	int &track_offset,
	uint32_t load_flags)
{
// add the appropriate type of track
	char string[BCTEXTLEN];
	Track *track = 0;
	string[0] = 0;

	xml->tag.get_property("TYPE", string);

	if((load_flags & LOAD_ALL) == LOAD_ALL ||
		(load_flags & LOAD_EDITS)) {
		if(!strcmp(string, "VIDEO")) {
			track = add_video_track(0, 0);
		}
		else if(!strcmp(string, "SUBTTL")) {
			track = add_subttl_track(0, 0);
		}
		else {
			track = add_audio_track(0, 0);    // default to audio
		}
	}
	else {
		track = get_item_number(track_offset++);
	}

// load it
	if( track ) track->load(xml, track_offset, load_flags);

	return 0;
}

Track* Tracks::add_audio_track(int above, Track *dst_track)
{
	ATrack* new_track = new ATrack(edl, this);
	if(!dst_track)
	{
		dst_track = (above ? first : last);
	}

	if(above)
	{
		insert_before(dst_track, (Track*)new_track);
	}
	else
	{
		insert_after(dst_track, (Track*)new_track);
// Shift effects referenced below the destination track
	}

// Shift effects referenced below the new track
	for(Track *track = last;
		track && track != new_track;
		track = track->previous)
	{
		change_modules(number_of(track) - 1, number_of(track), 0);
	}


	new_track->create_objects();
	new_track->set_default_title();

	int current_pan = 0;
	for(Track *current = first;
		current != (Track*)new_track;
		current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO) current_pan++;
		if(current_pan >= edl->session->audio_channels) current_pan = 0;
	}



	PanAuto* pan_auto =
		(PanAuto*)new_track->automation->autos[AUTOMATION_PAN]->default_auto;
	pan_auto->values[current_pan] = 1.0;

	BC_Pan::calculate_stick_position(edl->session->audio_channels,
		edl->session->achannel_positions,
		pan_auto->values,
		MAX_PAN,
		PAN_RADIUS,
		pan_auto->handle_x,
		pan_auto->handle_y);
	return new_track;
}

Track* Tracks::add_video_track(int above, Track *dst_track)
{
	VTrack* new_track = new VTrack(edl, this);
	if(!dst_track)
		dst_track = (above ? first : last);
	if(above)
		insert_before(dst_track, (Track*)new_track);
	else
		insert_after(dst_track, (Track*)new_track);

	for(Track *track = last; track && track != new_track; track = track->previous)
		change_modules(number_of(track) - 1, number_of(track), 0);

	new_track->create_objects();
	new_track->set_default_title();
	return new_track;
}


Track* Tracks::add_subttl_track(int above, Track *dst_track)
{
	STrack* new_track = new STrack(edl, this);
	if(!dst_track)
		dst_track = (above ? first : last);

	if(above)
		insert_before(dst_track, (Track*)new_track);
	else
		insert_after(dst_track, (Track*)new_track);

	for(Track *track = last; track && track != new_track; track = track->previous)
		change_modules(number_of(track) - 1, number_of(track), 0);

	new_track->create_objects();
	new_track->set_default_title();
//	new_track->paste_silence(0,total_length(),0);
	return new_track;
}


int Tracks::delete_track(Track *track, int gang)
{
	if( !track ) return 0;
	if( gang < 0 )
		gang = edl->local_session->gang_tracks != GANG_NONE ? 1 : 0;
	Track *nxt = track->next;
	if( gang ) {
		track = track->gang_master();
		while( nxt && !nxt->master )
			nxt = nxt->next;
	}
	Track *current = track;
	int old_location = number_of(current);
	for( Track *next_track=0; current!=nxt; current=next_track ) {
		next_track = current->next;
		detach_shared_effects(old_location);
		for( Track *curr=current; curr; curr=curr->next ) {
// Shift effects referencing effects below the deleted track
			change_modules(number_of(curr), number_of(curr)-1, 0);
		}
		delete current;
	}
	return 0;
}

int Tracks::detach_shared_effects(int module)
{
	for( Track *current=first; current; current=NEXT ) {
		current->detach_shared_effects(module);
	}
 	return 0;
} 
int Tracks::detach_ganged_effects(Plugin *plugin)
{
	if( edl->local_session->gang_tracks == GANG_NONE ) return 1;
	for( Track *current=first; current; current=NEXT ) {
		if( current == plugin->track ) continue;
		if( !current->armed_gang(plugin->track) ) continue;
		current->detach_ganged_effects(plugin);
	}
 	return 0;
}

int Tracks::total_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		long unit_start = current->to_units(edl->local_session->get_selectionstart(1), 0);
		Auto *mute_keyframe = 0;
		current->automation->autos[AUTOMATION_MUTE]->
			get_prev_auto(unit_start, PLAY_FORWARD, mute_keyframe);
		IntAuto *mute_auto = (IntAuto *)mute_keyframe;

		result +=
			(current->plays() && type == PLAY) ||
			(current->is_armed() && type == RECORD) ||
			(current->is_ganged() && type == GANG) ||
			(current->draw && type == DRAW) ||
			(mute_auto->value && type == MUTE) ||
			(current->expand_view && type == EXPAND);
	}
	return result;
}

int Tracks::recordable_audio_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_AUDIO && current->is_armed()) result++;
	return result;
}

int Tracks::recordable_video_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->is_armed()) result++;
	}
	return result;
}


int Tracks::playable_audio_tracks()
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO && current->plays())
		{
			result++;
		}
	}

	return result;
}

int Tracks::playable_video_tracks()
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->plays())
		{
			result++;
		}
	}
	return result;
}

int Tracks::total_audio_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_AUDIO) result++;
	return result;
}

int Tracks::total_video_tracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
		if(current->data_type == TRACK_VIDEO) result++;
	return result;
}

double Tracks::total_playable_length()
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if( current->plays() )
		{
			double length = current->get_length();
			if(length > total) total = length;
		}
	}
	return total;
}

double Tracks::total_recordable_length()
{
	double total = -1;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->is_armed())
		{
			double length = current->get_length();
			if(length > total) total = length;
		}
	}
	return total;
}

double Tracks::total_length()
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		double length = current->get_length();
		if(length > total) total = length;
	}
	return total;
}

double Tracks::total_audio_length()
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_AUDIO &&
			current->get_length() > total) total = current->get_length();
	}
	return total;
}

double Tracks::total_video_length()
{
	double total = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO &&
			current->get_length() > total) total = current->get_length();
	}
	return total;
}

double Tracks::total_length_framealigned(double fps)
{
	if (total_audio_tracks() && total_video_tracks())
		return MIN(floor(total_audio_length() * fps), floor(total_video_length() * fps)) / fps;

	if (total_audio_tracks())
		return floor(total_audio_length() * fps) / fps;

	if (total_video_tracks())
		return floor(total_video_length() * fps) / fps;

	return 0;
}

void Tracks::translate_fauto_xy(int fauto, float dx, float dy, int all)
{
	Track *track = first;
	for( ; track; track=track->next ) {
		if( !all && !track->is_armed() ) continue;
		if( track->data_type != TRACK_VIDEO ) continue;
		((VTrack*)track)->translate(fauto, dx, dy, all);
	}
}

void Tracks::translate_projector(float dx, float dy, int all)
{
	translate_fauto_xy(AUTOMATION_PROJECTOR_X, dx, dy, all);
}

void Tracks::translate_camera(float dx, float dy, int all)
{
	translate_fauto_xy(AUTOMATION_CAMERA_X, dx, dy, all);
}

void Tracks::crop_resize(float x, float y, float z)
{
	float ctr_x = edl->session->output_w / 2.;
	float ctr_y = edl->session->output_h / 2.;
	Track *track = first;
	for( ; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		if( track->data_type != TRACK_VIDEO ) continue;
		float px, py, pz;
		track->get_projector(px, py, pz);
		float old_x = px + ctr_x;
		float old_y = py + ctr_y;
		float nx = (old_x - x) * z;
		float ny = (old_y - y) * z;
		track->set_projector(nx, ny, pz * z);
	}
}

void Tracks::crop_shrink(float x, float y, float z)
{
	float ctr_x = edl->session->output_w / 2.;
	float ctr_y = edl->session->output_h / 2.;
	Track *track = first;
	for( ; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		if( track->data_type != TRACK_VIDEO ) continue;
		float cx, cy, cz, px, py, pz;
		track->get_camera(cx, cy, cz);
		track->get_projector(px, py, pz);
		float dx = x - (px + ctr_x);
		float dy = y - (py + ctr_y);
		cz *= pz;
		cx += dx / cz;  cy += dy / cz;
		track->set_camera(cx, cy, cz * z);
		px += dx;  py += dy;
		track->set_projector(px, py, 1 / z);
	}
}

void Tracks::update_y_pixels(Theme *theme)
{
//	int y = -edl->local_session->track_start;
	int y = 0;
	for(Track *current = first; current; current = NEXT)
	{
//printf("Tracks::update_y_pixels %d\n", y);
		current->y_pixel = y;
		if( current->is_hidden() ) continue;
		y += current->vertical_span(theme);
	}
}

int Tracks::dump(FILE *fp)
{
	for(Track* current = first; current; current = NEXT)
	{
		fprintf(fp,"  Track: %p\n", current);
		current->dump(fp);
		fprintf(fp,"\n");
	}
	return 0;
}

void Tracks::select_all(int type,
		int value)
{
	for(Track* current = first; current; current = NEXT)
	{
		double position = edl->local_session->get_selectionstart(1);

		if(type == PLAY) current->play = value;
		if(type == RECORD) current->armed = value;
		if(type == GANG) current->ganged = value;
		if(type == DRAW) current->draw = value;

		if(type == MUTE)
		{
			((IntAuto*)current->automation->autos[AUTOMATION_MUTE]->get_auto_for_editing(position))->value = value;
		}

		if(type == EXPAND) current->expand_view = value;
	}
}

// ===================================== file operations

int Tracks::popup_transition(int cursor_x, int cursor_y)
{
	int result = 0;
	for(Track* current = first; current && !result; current = NEXT)
	{
		result = current->popup_transition(cursor_x, cursor_y);
	}
	return result;
}


int Tracks::change_channels(int oldchannels, int newchannels)
{
	for(Track *current = first; current; current = NEXT)
	{ current->change_channels(oldchannels, newchannels); }
	return 0;
}



int Tracks::totalpixels()
{
	int result = 0;
	for(Track* current = first; current; current = NEXT)
	{
		result += current->data_h;
	}
	return result;
}

int Tracks::number_of(Track *track)
{
	int i = 0;
	for(Track *current = first; current && current != track; current = NEXT)
	{
		i++;
	}
	return i;
}

Track* Tracks::number(int number)
{
	Track *current;
	int i = 0;
	for(current = first; current && i < number; current = NEXT)
	{
		i++;
	}
	return current;
}

Track* Tracks::get_track_by_id(int id)
{
	Track *track = edl->tracks->first;
	while( track && track->get_id() != id ) track = track->next;
	return track;
}

int Tracks::total_playable_vtracks()
{
	int result = 0;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO && current->play) result++;
	}
	return result;
}

Plugin *Tracks::plugin_exists(int plugin_id)
{
	if( plugin_id < 0 ) return 0;
	Plugin *plugin = 0;
	for( Track *track=first; !plugin && track; track=track->next ) {
		plugin = track->plugin_exists(plugin_id);
	}
	return plugin;
}

int Tracks::track_exists(Track *track)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current == track) return 1;
	}
	return 0;
}

int Tracks::new_group(int id)
{
	int count = 0;
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( edit->group_id > 0 ) continue;
			if( !edit->is_selected ) continue;
			edit->group_id = id;
			++count;
		}
	}
	return count;
}

int Tracks::set_group_selected(int id, int v)
{
	int count = 0;
	int gang = edl->local_session->gang_tracks != GANG_NONE ? 1 : 0;
	for( Track *track=first; track; track=track->next ) {
		if( track->is_hidden() ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( edit->group_id != id ) continue;
			if( v < 0 ) v = !edit->is_selected ? 1 : 0;
			edit->select_affected_edits(v, gang);
			++count;
		}
	}
	return count;
}

int Tracks::del_group(int id)
{
	int count = 0;
	for( Track *track=first; track; track=track->next ) {
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( edit->group_id != id ) continue;
			edit->is_selected = 1;
			edit->group_id = 0;
			++count;
		}
	}
	return count;
}

Track *Tracks::get(int idx, int data_type)
{
	for(Track *current = first; current; current = NEXT)
	{
		if( current->data_type != data_type ) continue;
		if( --idx < 0 ) return current;
	}
	return 0;
}

void Tracks::roll_tracks(Track *src, Track *dst, int n)
{
	if( src == dst ) return;
	while( --n >= 0 && src ) {
		Track *nxt = src->next;
		change_modules(number_of(src), total(), 0);
		for( Track *track=nxt; track; track=track->next )
			change_modules(number_of(track), number_of(track)-1, 0);
		remove_pointer(src);
		int ndst = dst ? number_of(dst) : total();
		insert_before(dst, src);
		for( Track *track=last; track && track!=src; track=track->previous ) 
			change_modules(number_of(track)-1, number_of(track), 0);
		change_modules(total(), ndst, 0);
		src = nxt;
	}
}

double Tracks::align_timecodes()
{
	double offset = -1;
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		double early_offset = track->edits->early_timecode();
		if( offset < 0 || offset > early_offset )
			offset = early_offset;
	}
	if( offset >= 0 ) {
		for( Track *track=first; track; track=track->next ) {
			if( !track->is_armed() ) continue;
			track->edits->align_timecodes(offset);
		}
	}
	return offset;
}

void Tracks::update_idxbl_length(int id, double dt)
{
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		int64_t du = track->to_units(dt,0);
		track->edits->update_idxbl_length(id, du);
		track->optimize();
	}
}

void Tracks::create_keyframes(double position, int mask, int mode)
{
	for( Track *track=first; track; track=track->next ) {
		if( !track->is_armed() ) continue;
		track->create_keyframes(position, mask, mode);
	}
}

