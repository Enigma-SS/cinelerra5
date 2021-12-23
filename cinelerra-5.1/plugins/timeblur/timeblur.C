
/*
 * CINELERRA
 * Copyright (C) 2008-2012 Adam Williams <broadcast at earthling dot net>
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

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "bcprogressbox.h"
#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "loadbalance.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "timeblur.h"
#include "timeblurwindow.h"
#include "vframe.h"

REGISTER_PLUGIN(TimeBlurMain)


TimeBlurConfig::TimeBlurConfig()
{
	frames = 0;
}

int TimeBlurConfig::equivalent(TimeBlurConfig &that)
{
	return frames != that.frames ? 0 : 1;
}

void TimeBlurConfig::copy_from(TimeBlurConfig &that)
{
	frames = that.frames;
}

void TimeBlurConfig::interpolate(TimeBlurConfig &prev, TimeBlurConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	frames = prev.frames;
}


TimeBlurMain::TimeBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	stripe_engine = 0;
	input = 0;
	fframe = 0;
	last_frames = 0;
	last_position = -1;
}

TimeBlurMain::~TimeBlurMain()
{

	delete stripe_engine;
	delete fframe;
}

const char* TimeBlurMain::plugin_title() { return N_("TimeBlur"); }
int TimeBlurMain::is_realtime() { return 1; }



NEW_WINDOW_MACRO(TimeBlurMain, TimeBlurWindow)

LOAD_CONFIGURATION_MACRO(TimeBlurMain, TimeBlurConfig)


void TimeBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("TIMEBLUR");

	output.tag.set_property("FRAMES", config.frames);
	output.append_tag();
	output.tag.set_title("/TIMEBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void TimeBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("TIMEBLUR") ) {
			config.frames = input.tag.get_property("FRAMES", config.frames);
		}
	}
}

int TimeBlurMain::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	this->input = frame;
	int cpus = input->get_w() * input->get_h() / 0x80000 + 2;
	int smps = get_project_smp();
	if( cpus > smps ) cpus = smps;
	int frames = config.frames;
	int use_opengl = 0;
	MWindow *mwindow = server->mwindow;
	if( frames > 1 && (!mwindow || // dont scan during SELECT_REGION
	      mwindow->session->current_operation != SELECT_REGION ||
	      mwindow->edl->local_session->get_selectionstart() ==
		  mwindow->edl->local_session->get_selectionend() ) ) {
		if( !stripe_engine )
			stripe_engine = new TimeBlurStripeEngine(this, cpus, cpus);
		int fw = frame->get_w(), fh =frame->get_h();
		new_temp(fw, fh, BC_RGB_FLOAT);
		MWindow *mwindow = server->mwindow;
		if( (mwindow && mwindow->session->current_operation == SELECT_REGION) ||
		    ( last_frames == frames && last_position-1 == start_position &&
		      fframe && fframe->get_w() == fw && fframe->get_h() == fh ) ) {
			read_frame(temp, 0, start_position, frame_rate, use_opengl);
			stripe_engine->process_packages(ADD_FFRM);
			frame->transfer_from(temp);
		}
		else if( last_frames != frames || last_position != start_position ||
		      !fframe || fframe->get_w() != fw || fframe->get_h() != fh ) {
			last_frames = frames;
			last_position = start_position;
			VFrame::get_temp(fframe, fw, fh, BC_RGB_FLOAT);
			read_frame(fframe, 0, start_position+1, frame_rate, use_opengl);
			BC_ProgressBox *progress = 0;
			const char *progress_title = _("TimeBlur: scanning\n");
			Timer timer;
			for( int i=2; i<frames; ++i ) {
                                read_frame(temp, 0, start_position+i, frame_rate, use_opengl);
				stripe_engine->process_packages(ADD_TEMP);
				if( !progress && gui_open() && frames > 2*frame_rate ) {
					progress = new BC_ProgressBox(-1, -1, progress_title, frames);
					progress->start();
				}
				if( progress && timer.get_difference() > 100 ) {
					timer.update();
					progress->update(i, 1);
					char string[BCTEXTLEN];
					sprintf(string, "%sframe: %d", progress_title, i);
					progress->update_title(string, 1);
					if( progress->is_cancelled() ) break;
				}
				if( progress && !gui_open() ) {
					progress->stop_progress();
					delete progress;  progress = 0;
				}
			}
			read_frame(temp, 0, start_position, frame_rate, use_opengl);
			stripe_engine->process_packages(ADD_FFRMS);
			frame->transfer_from(temp);
			if( progress ) {
				progress->stop_progress();
				delete progress;
			}
			++last_position;
		}
		else {
			read_frame(temp, 0, start_position+frames-1, frame_rate, use_opengl);
			stripe_engine->process_packages(ADD_TEMPS);
			frame->transfer_from(fframe);
			read_frame(temp, 0, start_position, frame_rate, use_opengl);
			stripe_engine->process_packages(SUB_TEMPS);
			++last_position;
		}
	}
	else
		read_frame(frame, 0, start_position, frame_rate, use_opengl);
	return 0;
}


TimeBlurStripePackage::TimeBlurStripePackage()
 : LoadPackage()
{
}

TimeBlurStripeUnit::TimeBlurStripeUnit(TimeBlurStripeEngine *server, TimeBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void TimeBlurStripeUnit::process_package(LoadPackage *package)
{
	TimeBlurStripePackage *pkg = (TimeBlurStripePackage*)package;
	int frames = plugin->config.frames;
	float scale = 1. / frames;
	int iy0 = pkg->y0, iy1 = pkg->y1;
	int fw = plugin->fframe->get_w();
	uint8_t **frows = plugin->fframe->get_rows();
	uint8_t **trows = plugin->temp->get_rows();
	switch( server->operation ) {
	case ADD_TEMP:  // add temp to fframe
		for( int iy=iy0; iy<iy1; ++iy ) {
			float *trow = (float *)trows[iy];
			float *frow = (float *)frows[iy];
			for( int ix=0; ix<fw; ++ix ) {
				*frow++ += *trow++;
				*frow++ += *trow++;
				*frow++ += *trow++;
			}
		}
		break;
	case ADD_FFRM:  // add fframe to scaled temp
		for( int iy=iy0; iy<iy1; ++iy ) {
			float *trow = (float *)trows[iy];
			float *frow = (float *)frows[iy];
			for( int ix=0; ix<fw; ++ix ) {
				*trow = *trow * scale + *frow++;  ++trow;
				*trow = *trow * scale + *frow++;  ++trow;
				*trow = *trow * scale + *frow++;  ++trow;
			}
		}
		break;
	case ADD_FFRMS:  // add fframe to temp, scale temp, scale fframe
		for( int iy=iy0; iy<iy1; ++iy ) {
			float *trow = (float *)trows[iy];
			float *frow = (float *)frows[iy];
			for( int ix=0; ix<fw; ++ix ) {
				*trow += *frow;  *trow++ *= scale;  *frow++ *= scale;
				*trow += *frow;  *trow++ *= scale;  *frow++ *= scale;
				*trow += *frow;  *trow++ *= scale;  *frow++ *= scale;
			}
		}
		break;
	case ADD_TEMPS:  // add scaled temp to fframe
		for( int iy=iy0; iy<iy1; ++iy ) {
			float *trow = (float *)trows[iy];
			float *frow = (float *)frows[iy];
			for( int ix=0; ix<fw; ++ix ) {
				*frow++ += *trow++ * scale;
				*frow++ += *trow++ * scale;
				*frow++ += *trow++ * scale;
			}
		}
		break;
	case SUB_TEMPS:  // sub scaled temp from frame
		for( int iy=iy0; iy<iy1; ++iy ) {
			float *trow = (float *)trows[iy];
			float *frow = (float *)frows[iy];
			for( int ix=0; ix<fw; ++ix ) {
				*frow++ -= *trow++ * scale;
				*frow++ -= *trow++ * scale;
				*frow++ -= *trow++ * scale;
			}
		}
		break;
	}
}

TimeBlurStripeEngine::TimeBlurStripeEngine(TimeBlurMain *plugin,
	int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}
void TimeBlurStripeEngine::init_packages()
{
	int ih = plugin->input->get_h(), iy0 = 0;
	for( int i=0,n=get_total_packages(); i<n; ) {
		TimeBlurStripePackage *pkg = (TimeBlurStripePackage*)get_package(i);
		int iy1 = (ih * ++i) / n;
		pkg->y0 = iy0;  pkg->y1 = iy1;
		iy0 = iy1;
	}
}

LoadClient* TimeBlurStripeEngine::new_client()
{
	return new TimeBlurStripeUnit(this, plugin);
}

LoadPackage* TimeBlurStripeEngine::new_package()
{
	return new TimeBlurStripePackage();
}

void TimeBlurStripeEngine::process_packages(int operation)
{
	this->operation = operation;
	LoadServer::process_packages();
}

