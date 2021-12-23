
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

#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mwindow.h"
#include "patchbay.h"
#include "playabletracks.h"
#include "plugin.h"
#include "preferences.h"
#include "intauto.h"
#include "intautos.h"
#include "tracks.h"
#include "transportque.h"


PlayableTracks::PlayableTracks(EDL *edl, int64_t current_position,
		int direction, int data_type, int use_nudge)
 : ArrayList<Track*>()
{
	this->data_type = data_type;

	for( Track *track=edl->tracks->first; track; track=track->next ) {
		if( is_playable(track, current_position, direction, use_nudge) )
			append(track);
	}
}

PlayableTracks::~PlayableTracks()
{
}


int PlayableTracks::is_playable(Track *current_track, int64_t position,
		int direction, int use_nudge)
{
	int result = 1;
	if(use_nudge) position += current_track->nudge;
	if(current_track->data_type != data_type) result = 0;

// Track is off screen and not bounced to other modules
	if( result &&
		!current_track->plugin_used(position, direction) &&
		!current_track->is_playable(position, direction) )
			result = 0;
// Test play patch
	if( result &&
		!current_track->plays() )
			result = 0;
	if( result ) {
		EDL *edl = current_track->edl;
		int solo_track_id = edl->local_session->solo_track_id;
		if( solo_track_id >= 0 ) {
			int visible = 0;
			int current_id = current_track->get_id();
			Track *track = edl->tracks->first;
			while( track ) {
				int id = track->get_id();
				if( id == solo_track_id ) { visible = 1;  break; }
				if( id == current_id ) { visible = 0; break; }
 				track = track->next;
			}
			if( !track ) visible = 1;
			result = visible;
		}
	}
	if( result ) {
// Test for playable edit
		if(!current_track->playable_edit(position, direction))
		{
// Test for playable effect
			if(!current_track->is_synthesis(position,
						direction))
			{
				result = 0;
			}
		}
	}

	return result;
}


int PlayableTracks::is_listed(Track *track)
{
	for(int i = 0; i < total; i++)
	{
		if(values[i] == track) return 1;
	}
	return 0;
}
