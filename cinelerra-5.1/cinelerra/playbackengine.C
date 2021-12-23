
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
#include "bcsignals.h"
#include "cache.h"
#include "canvas.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "tracking.h"
#include "tracks.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "renderengine.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "videodevice.h"
#include "vdevicex11.h"
#include "vrender.h"


PlaybackEngine::PlaybackEngine(MWindow *mwindow, Canvas *output)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->output = output;
	is_playing_back = 0;
	tracking_position = 0;
	tracking_active = 0;
	audio_cache = 0;
	video_cache = 0;
	command = new TransportCommand(mwindow->preferences);
	command->command = STOP;
	next_command = new TransportCommand(mwindow->preferences);
	next_command->change_type = CHANGE_ALL;
	stop_command = new TransportCommand(mwindow->preferences);
	stop_command->command = STOP;
	stop_command->realtime = 1;
	sent_command = new TransportCommand(mwindow->preferences);
	sent_command->command = -1;
	send_active = 0;
	tracking_lock = new Mutex("PlaybackEngine::tracking_lock");
	renderengine_lock = new Mutex("PlaybackEngine::renderengine_lock");
	tracking_done = new Condition(1, "PlaybackEngine::tracking_done");
	pause_lock = new Condition(0, "PlaybackEngine::pause_lock");
	start_lock = new Condition(0, "PlaybackEngine::start_lock");
	cache_lock = new Mutex("PlaybackEngine::cache_lock");
	input_lock = new Condition(1, "PlaybackEngine::input_lock");
	output_lock = new Condition(0, "PlaybackEngine::output_lock", 1);

	render_engine = 0;
	debug = 0;
}

PlaybackEngine::~PlaybackEngine()
{
	done = 1;
	output_lock->unlock();
	Thread::join();
	delete_render_engine();
	delete preferences;
	if( audio_cache )
		audio_cache->remove_user();
	if( video_cache )
		video_cache->remove_user();
	delete tracking_lock;
	delete tracking_done;
	delete pause_lock;
	delete start_lock;
	delete cache_lock;
	delete renderengine_lock;
	delete command;
	delete next_command;
	delete stop_command;
	delete sent_command;
	delete input_lock;
	delete output_lock;
}

void PlaybackEngine::create_objects()
{
	preferences = new Preferences;
	preferences->copy_from(mwindow->preferences);

	done = 0;
	Thread::start();
	start_lock->lock("PlaybackEngine::create_objects");
}

ChannelDB* PlaybackEngine::get_channeldb()
{
	PlaybackConfig *config = command->get_edl()->session->playback_config;
	switch(config->vconfig->driver)
	{
		case VIDEO4LINUX2JPEG:
			return mwindow->channeldb_v4l2jpeg;
	}
	return 0;
}

int PlaybackEngine::create_render_engine()
{
// Fix playback configurations
	delete_render_engine();
	render_engine = new RenderEngine(this, preferences, output, 0);
//printf("PlaybackEngine::create_render_engine %d\n", __LINE__);
	return 0;
}

void PlaybackEngine::delete_render_engine()
{
	renderengine_lock->lock("PlaybackEngine::delete_render_engine");
	if( render_engine ) {
		render_engine->interrupt_playback();
		render_engine->wait_done();
		delete render_engine;  render_engine = 0;
	}
	renderengine_lock->unlock();
}

void PlaybackEngine::arm_render_engine()
{
	renderengine_lock->lock("PlaybackEngine::arm_render_engine");
	if( render_engine )
		render_engine->arm_command(command);
	renderengine_lock->unlock();
}

void PlaybackEngine::start_render_engine()
{
	renderengine_lock->lock("PlaybackEngine::start_render_engine");
	if( render_engine )
		render_engine->start_command();
	renderengine_lock->unlock();
}

void PlaybackEngine::wait_render_engine()
{
	if( command->realtime && render_engine ) {
		render_engine->join();
	}
}

void PlaybackEngine::create_cache()
{
	cache_lock->lock("PlaybackEngine::create_cache");
	if( audio_cache )
		audio_cache->remove_user();
	if( video_cache )
		video_cache->remove_user();
	audio_cache = new CICache(preferences);
	video_cache = new CICache(preferences);
	cache_lock->unlock();
}


void PlaybackEngine::perform_change()
{
	switch( command->change_type ) {
	case CHANGE_ALL:
		create_cache();
	case CHANGE_EDL:
		create_render_engine();
		break;
	case CHANGE_PARAMS: {
		renderengine_lock->lock("PlaybackEngine::perform_change");
		EDL *edl = render_engine ? render_engine->get_edl() : 0;
		if( edl ) edl->add_user();
		renderengine_lock->unlock();
		if( !edl ) break;
		edl->synchronize_params(command->get_edl());
		edl->remove_user();
		}
	case CHANGE_NONE:
			break;
	}
}

void PlaybackEngine::sync_parameters(EDL *edl)
{
// TODO: lock out render engine from keyframe deletions
	command->get_edl()->synchronize_params(edl);
	if( render_engine )
		render_engine->get_edl()->synchronize_params(edl);
}

void PlaybackEngine::interrupt_playback(int wait_tracking)
{
	renderengine_lock->lock("PlaybackEngine::interrupt_playback");
	if( render_engine )
		render_engine->interrupt_playback();
	renderengine_lock->unlock();

// Stop pausing
	pause_lock->unlock();

// Wait for tracking to finish if it is running
	if( wait_tracking ) {
		tracking_done->lock("PlaybackEngine::interrupt_playback");
		tracking_done->unlock();
	}
}

// Return 1 if levels exist
int PlaybackEngine::get_output_levels(double *levels, long position)
{
	int result = 0;
	if( render_engine && render_engine->do_audio ) {
		render_engine->get_output_levels(levels, position);
		result = 1;
	}
	return result;
}


int PlaybackEngine::get_module_levels(ArrayList<double> *module_levels, long position)
{
	int result = 0;
	if( render_engine && render_engine->do_audio ) {
		render_engine->get_module_levels(module_levels, position);
		result = 1;
	}
	return result;
}

int PlaybackEngine::brender_available(long position)
{
	return 0;
}

void PlaybackEngine::init_cursor(int active)
{
}

void PlaybackEngine::init_meters()
{
}

void PlaybackEngine::stop_cursor()
{
}


void PlaybackEngine::init_tracking()
{
	tracking_active = !command->single_frame() ? 1 : 0;
	tracking_position = command->playbackstart;
	tracking_done->lock("PlaybackEngine::init_tracking");
	init_cursor(tracking_active);
	init_meters();
}

void PlaybackEngine::stop_tracking(double position)
{
	tracking_position = position;
	tracking_active = 0;
	stop_cursor();
	tracking_done->unlock();
}

void PlaybackEngine::update_tracking(double position)
{
	tracking_lock->lock("PlaybackEngine::update_tracking");
	tracking_position = position;
// Signal that the timer is accurate.
	if(tracking_active) tracking_active = 2;
	tracking_timer.update();
	tracking_lock->unlock();
}

double PlaybackEngine::get_tracking_position()
{
	double result = 0;

	tracking_lock->lock("PlaybackEngine::get_tracking_position");


// Adjust for elapsed time since last update_tracking.
// But tracking timer isn't accurate until the first update_tracking
// so wait.
	if(tracking_active == 2)
	{
//printf("PlaybackEngine::get_tracking_position %d %d %d\n", command->get_direction(), tracking_position, tracking_timer.get_scaled_difference(command->get_edl()->session->sample_rate));


// Don't interpolate when every frame is played.
		if( command->get_edl()->session->video_every_frame &&
		    render_engine && render_engine->do_video ) {
			result = tracking_position;
		}
		else
// Interpolate
		{
			double loop_start, loop_end;
			int play_loop = command->loop_play ? 1 : 0;
			EDL *edl = command->get_edl();
			int loop_playback = edl->local_session->loop_playback ? 1 : 0;
			if( play_loop || !loop_playback ) {
				loop_start = command->start_position;
				loop_end = command->end_position;
			}
			else {
				loop_start = edl->local_session->loop_start;
				loop_end = edl->local_session->loop_end;
				play_loop = 1;
			}
			double loop_size = loop_end - loop_start;

			if( command->get_direction() == PLAY_FORWARD ) {
// Interpolate
				result = tracking_position +
					command->get_speed() *
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
//printf("PlaybackEngine::get_tracking_position 1 %d\n", command->get_edl()->local_session->loop_playback);
				if( play_loop && loop_size > 0 ) {
					while( result > loop_end ) result -= loop_size;
				}
			}
			else {
// Interpolate
				result = tracking_position -
					command->get_speed() *
					tracking_timer.get_difference() /
					1000.0;

// Compensate for loop
				if( play_loop && loop_size > 0 ) {
					while( result < loop_start ) result += loop_size;
				}
			}

		}
	}
	else
		result = tracking_position;

	tracking_lock->unlock();
//printf("PlaybackEngine::get_tracking_position %f %f %d\n", result, tracking_position, tracking_active);

// Adjust for loop

	return result;
}

void PlaybackEngine::update_transport(int command, int paused)
{
//	mwindow->gui->lock_window();
//	mwindow->gui->mbuttons->transport->update_gui_state(command, paused);
//	mwindow->gui->unlock_window();
}

void PlaybackEngine::run()
{
	start_lock->unlock();

	while( !done ) {
// Wait for current command to finish
		output_lock->lock("PlaybackEngine::run");
		if( done ) break;
// Read the new command
		input_lock->lock("PlaybackEngine::run");
		command->copy_from(sent_command);
//printf("sent command=%d\n", sent_command->command);
		int active = this->send_active;
		this->send_active = 0;
		input_lock->unlock();
		if( !active ) continue;

		interrupt_playback(0);
		wait_render_engine();

		switch( command->command ) {
// Parameter change only
		case COMMAND_NONE:
			perform_change();
			break;

		case PAUSE:
			init_cursor(0);
			pause_lock->lock("PlaybackEngine::run");
			stop_cursor();
			break;

		case STOP:
// No changing
			break;

		case CURRENT_FRAME:
		case LAST_FRAME:
			perform_change();
			arm_render_engine();
// Dispatch the command
			start_render_engine();
			break;
// fall through
		default:
			is_playing_back = 1;
		case REWIND:
		case GOTO_END:
			perform_change();
			arm_render_engine();

// Start tracking after arming so the tracking position doesn't change.
// The tracking for a single frame command occurs during PAUSE
			init_tracking();
			clear_borders();
// Dispatch the command
			start_render_engine();
			break;
		}
//printf("PlaybackEngine::run 100\n");
	}
}

void PlaybackEngine::clear_borders()
{
	EDL *edl = command->get_edl();
	PlaybackConfig *config = edl->session->playback_config;
	if( config->vconfig->driver == PLAYBACK_X11_GL ) {
		if( render_engine && render_engine->video ) {
			VDeviceBase *vdriver = render_engine->video->get_output_base();
			((VDeviceX11*)vdriver)->clear_output();
			return;
		}
	}
	BC_WindowBase *window = output->get_canvas();
	if( !window ) return;
	window->lock_window("PlaybackEngine::clear_output");
	output->clear_borders(edl);
	window->unlock_window();
}

void PlaybackEngine::stop_playback(int wait_tracking)
{
	transport_stop(wait_tracking);
	renderengine_lock->lock("PlaybackEngine::stop_playback");
	if( render_engine ) {
		render_engine->interrupt_playback();
		render_engine->wait_done();
	}
	renderengine_lock->unlock();
}

int PlaybackEngine::get_direction()
{
	int curr_command = is_playing_back ? this->command->command : STOP;
	return TransportCommand::get_direction(curr_command);
}

void PlaybackEngine::update_preferences(Preferences *prefs)
{
	preferences->copy_from(prefs);
	create_render_engine();
}

void PlaybackEngine::send_command(int command, EDL *edl, int wait_tracking, int use_inout)
{
//printf("PlaybackEngine::send_command 1 %d\n", command);
// Stop requires transferring the output buffer to a refresh buffer.
	int curr_command = is_playing_back ? this->command->command : STOP;
	int curr_single_frame = TransportCommand::single_frame(curr_command);
	int curr_audio = this->command->toggle_audio ?
		!curr_single_frame : curr_single_frame;
	int single_frame = TransportCommand::single_frame(command);
	int next_audio = next_command->toggle_audio ? !single_frame : single_frame;
	float next_speed = next_command->speed;
// Dispatch command
	switch( command ) {
	case STOP:
		transport_stop(wait_tracking);
		break;
	case FAST_REWIND:	// Commands that play back
	case NORMAL_REWIND:
	case SLOW_REWIND:
	case SINGLE_FRAME_REWIND:
	case SINGLE_FRAME_FWD:
	case SLOW_FWD:
	case NORMAL_FWD:
	case FAST_FWD:
	case CURRENT_FRAME:
	case LAST_FRAME:
// run shuttle as no prev command
		if( next_speed ) curr_command = COMMAND_NONE;
// Same direction pressed twice, not shuttle, and no change in audio state,  Stop
		if( curr_command == command && !curr_single_frame &&
		    curr_audio == next_audio ) {
			transport_stop(wait_tracking);
			break;
		}
// Resume or change direction
		switch( curr_command ) {
		case REWIND:
		case GOTO_END:
		case STOP:
		case COMMAND_NONE:
		case SINGLE_FRAME_FWD:
		case SINGLE_FRAME_REWIND:
		case CURRENT_FRAME:
		case LAST_FRAME:
// already stopped
			break;
		default:
			transport_stop(0);
			next_command->resume = 1;
			break;
		}
		next_command->realtime = 1;
		transport_command(command, CHANGE_NONE, edl, use_inout);
		break;
	case REWIND:
	case GOTO_END:
		transport_stop(1);
		next_command->realtime = 1;
		transport_command(command, CHANGE_NONE, edl, use_inout);
		stop_tracking(this->command->playbackstart);
		break;
	}
}

int PlaybackEngine::put_command(TransportCommand *command, int reset)
{
	input_lock->lock("PlaybackEngine::put_command");
	int prev_change_type = sent_command->change_type;
	sent_command->copy_from(command);
// run only last command, sum change type
	if( send_active )
		sent_command->change_type |= prev_change_type;
	send_active = 1;
	if( reset ) command->reset();
	output_lock->unlock();
	input_lock->unlock();
	return 0;
}

int PlaybackEngine::transport_stop(int wait_tracking)
{
	put_command(stop_command, 0);
	if( wait_tracking ) {
		tracking_done->lock("PlaybackEngine::transport_stop");
		tracking_done->unlock();
	}
//printf("send: %d (STOP) 0\n", STOP);
	return 0;
}

int PlaybackEngine::transport_command(int command, int change_type, EDL *new_edl, int use_inout)
{
	next_command->command = command;
	next_command->change_type |= change_type;
	if( new_edl ) {
// Just change the EDL if the change requires it because renderengine
// structures won't point to the new EDL otherwise and because copying the
// EDL for every cursor movement is slow.
		if( change_type == CHANGE_EDL || change_type == CHANGE_ALL )
			next_command->get_edl()->copy_all(new_edl);
		else if( change_type == CHANGE_PARAMS )
			next_command->get_edl()->synchronize_params(new_edl);
		next_command->set_playback_range(new_edl, use_inout,
				preferences->forward_render_displacement);
	}
	put_command(next_command, 1);
//static const char *types[] = { "NONE",
// "FRAME_FWD", "NORMAL_FWD", "FAST_FWD", "FRAME_REV", "NORMAL_REV", "FAST_REV",
// "STOP",  "PAUSE", "SLOW_FWD", "SLOW_REV", "REWIND", "GOTO_END", "CURRENT_FRAME",
// "LAST_FRAME" };
//printf("send= %d (%s) %d\n", sent_command->command,
// types[sent_command->command], sent_command->locked);
	return 0;
}

void PlaybackEngine::refresh_frame(int change_type, EDL *edl, int dir)
{
	int command = dir >= 0 ? CURRENT_FRAME : LAST_FRAME;
	next_command->realtime = 1;
	transport_command(command, change_type, edl);
}

