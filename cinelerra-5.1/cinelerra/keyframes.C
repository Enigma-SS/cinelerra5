
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

#include "bchash.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "localsession.h"
#include "plugin.h"
#include "track.h"
#include "transportque.inc"

KeyFrames::KeyFrames(EDL *edl, Plugin *plugin)
 : Autos(edl, plugin->track)
{
	type = Autos::AUTOMATION_TYPE_PLUGIN;
	plugin = plugin;
}

KeyFrames::~KeyFrames()
{
}


KeyFrame* KeyFrames::get_prev_keyframe(int64_t position,
	int direction)
{
	KeyFrame *current = 0;

// This doesn't work because edl->selectionstart doesn't change during
// playback at the same rate as PluginClient::source_position.
	if(position < 0)
	{
		position = track->to_units(edl->local_session->get_selectionstart(1), 0);
	}

// Get keyframe on or before current position
	for(current = (KeyFrame*)last;
		current;
		current = (KeyFrame*)PREVIOUS)
	{
		if(direction == PLAY_FORWARD && current->position <= position) break;
		else
		if(direction == PLAY_REVERSE && current->position < position) break;
	}

// Nothing before current position
	if(!current && first)
	{
		current = (KeyFrame*)first;
	}
	else
// No keyframes
	if(!current)
	{
		current = (KeyFrame*)default_auto;
	}

	return current;
}

KeyFrame* KeyFrames::get_keyframe()
{
	int64_t pos = track->to_units(edl->local_session->get_selectionstart(1), 0);
// Search for keyframe on or before selection
	KeyFrame *result = get_prev_keyframe(pos, PLAY_FORWARD);
	if( edl->session->auto_keyframes ) {
		if( !result || result->position != pos ||
		    result == (KeyFrame*)default_auto )
// generate keyframes while tweeking, and no keyframe found at pos
			result = (KeyFrame*)insert_auto(pos);
	}
	return result;
}

Auto* KeyFrames::new_auto()
{
	return new KeyFrame(edl, this);
}


void KeyFrames::dump(FILE *fp)
{
	fprintf(fp,"    DEFAULT_KEYFRAME\n");
	((KeyFrame*)default_auto)->dump(fp);
	fprintf(fp,"    KEYFRAMES total=%d\n", total());
	for(KeyFrame *current = (KeyFrame*)first;
		current;
		current = (KeyFrame*)NEXT)
	{
		current->dump(fp);
	}
}

