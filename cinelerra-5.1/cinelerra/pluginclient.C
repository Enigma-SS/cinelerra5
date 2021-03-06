
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "attachmentpoint.h"
#include "clip.h"
#include "condition.h"
#include "edits.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderengine.h"
#include "track.h"
#include "tracks.h"
#include "transportque.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

PluginClientFrame::PluginClientFrame()
{
	position = -1;
}

PluginClientFrame::~PluginClientFrame()
{
}


PluginClientThread::PluginClientThread(PluginClient *client)
 : Thread(1, 0, 0)
{
	this->client = client;
	window = 0;
	init_complete = new Condition(0, "PluginClientThread::init_complete");
}

PluginClientThread::~PluginClientThread()
{
	delete window;
	delete init_complete;
}

void PluginClientThread::run()
{
	BC_DisplayInfo info;
	int result = 0;
	if(client->window_x < 0) client->window_x = info.get_abs_cursor_x();
	if(client->window_y < 0) client->window_y = info.get_abs_cursor_y();
	if(!window)
		window = (PluginClientWindow*)client->new_window();

	if(window) {
		window->lock_window("PluginClientThread::run");
		window->create_objects();
		VFrame *picon = client->server->get_picon();
		if( picon ) window->set_icon(picon);
		window->unlock_window();

/* Only set it here so tracking doesn't update it until everything is created. */
 		client->thread = this;
		init_complete->unlock();

		result = window->run_window();
		window->lock_window("PluginClientThread::run");
//printf("PluginClientThread::run %p %d\n", this, __LINE__);
		window->hide_window(1);
		client->save_defaults_xml(); // needs window lock
		window->unlock_window();
		window->done_event(result);
/* This is needed when the GUI is closed from itself */
		if(result) client->client_side_close();
	}
	else
// No window
	{
 		client->thread = this;
		init_complete->unlock();
	}
}

BC_WindowBase* PluginClientThread::get_window()
{
	return window;
}

PluginClient* PluginClientThread::get_client()
{
	return client;
}


PluginClientWindow::PluginClientWindow(PluginClient *client,
	int w, int h, int min_w, int min_h, int allow_resize)
 : BC_Window(client->gui_string,
	client->window_x /* - w / 2 */, client->window_y /* - h / 2 */,
	w, h, min_w, min_h, allow_resize, 0, 1)
{
	char title[BCTEXTLEN];

	this->client = client;

// *** CONTEXT_HELP ***
	if(client) {
		strcpy(title, client->plugin_title());
		if(! strcmp(title, "Overlay")) {
			// "Overlay" plugin title is ambiguous
			if(client->is_audio()) strcat(title, " \\(Audio\\)");
			if(client->is_video()) strcat(title, " \\(Video\\)");
		}
		if(client->server->is_ffmpeg()) {
			// FFmpeg plugins can be audio or video
			if(client->is_audio())
				strcpy(title, "FFmpeg Audio Plugins");
			if(client->is_video())
				strcpy(title, "FFmpeg Video Plugins");
		}
		context_help_set_keyword(title);
	}
}

PluginClientWindow::PluginClientWindow(const char *title,
	int x, int y, int w, int h, int min_w, int min_h, int allow_resize)
 : BC_Window(title, x, y, w, h, min_w, min_h, allow_resize, 0, 1)
{
	this->client = 0;
// *** CONTEXT_HELP ***
	context_help_set_keyword(title);
}

PluginClientWindow::~PluginClientWindow()
{
}


int PluginClientWindow::translation_event()
{
	if(client)
	{
		client->window_x = get_x();
		client->window_y = get_y();
	}

	return 1;
}

int PluginClientWindow::close_event()
{
/* Set result to 1 to indicate a client side close */
	set_done(1);
	return 1;
}

void PluginClientWindow::param_updated()
{
    printf("PluginClientWindow::param_updated %d undefined\n", __LINE__);
}

//phyllis
PluginParam::PluginParam(PluginClient *plugin, PluginClientWindow *gui,
    int x1, int x2, int x3, int y, int text_w,
    int *output_i, float *output_f, int *output_q,
    const char *title, float min, float max)
{
    this->output_i = output_i;
    this->output_f = output_f;
    this->output_q = output_q;
    this->title = cstrdup(title);
    this->plugin = plugin;
    this->gui = gui;
    this->x1 = x1;
    this->x2 = x2;
    this->x3 = x3;
    this->text_w = text_w;
    this->y = y;
    this->min = min;
    this->max = max;
    fpot = 0;
    ipot = 0;
    qpot = 0;
    text = 0;
    precision = 2;
}
PluginParam::~PluginParam()
{
    delete fpot;
    delete ipot;
    delete qpot;
    delete text;
    delete title;
}


void PluginParam::initialize()
{
    BC_Title *title_;
    int y2 = y +
        (BC_Pot::calculate_h() -
        BC_Title::calculate_h(gui, _(title), MEDIUMFONT)) / 2;
    gui->add_tool(title_ = new BC_Title(x1, y2, _(title)));

    if(output_f)
    {
        gui->add_tool(fpot = new PluginFPot(this, x2, y));
    }

    if(output_i)
    {
        gui->add_tool(ipot = new PluginIPot(this, x2, y));
    }

    if(output_q)
    {
        gui->add_tool(qpot = new PluginQPot(this, x2, y));
    }

    int y3 = y +
        (BC_Pot::calculate_h() -
        BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1)) / 2;
    if(output_i)
    {
        gui->add_tool(text = new PluginText(this, x3, y3, *output_i));
    }
    if(output_f)
    {
        gui->add_tool(text = new PluginText(this, x3, y3, *output_f));
    }
    if(output_q)
    {
        gui->add_tool(text = new PluginText(this, x3, y3, *output_q));
    }

    set_precision(precision);
}

void PluginParam::update(int skip_text, int skip_pot)
{
    if(!skip_text)
    {
        if(output_i)
        {
            text->update((int64_t)*output_i);
        }
        if(output_q)
        {
            text->update((int64_t)*output_q);
        }
        if(output_f)
        {
            text->update((float)*output_f);
        }
    }

    if(!skip_pot)
    {
        if(ipot)
        {
            ipot->update((int64_t)*output_i);
        }
        if(qpot)
        {
            qpot->update((int64_t)*output_q);
        }
        if(fpot)
        {
            fpot->update((float)*output_f);
        }
    }
}

void PluginParam::set_precision(int digits)
{
    this->precision = digits;
    if(fpot)
    {
        if(text)
        {
            text->set_precision(digits);
        }

        fpot->set_precision(1.0f / pow(10, digits));
    }
}


PluginFPot::PluginFPot(PluginParam *param, int x, int y)
 : BC_FPot(x,
        y,
        *param->output_f,
        param->min,
        param->max)
{
    this->param = param;
    set_use_caption(0);
}

int PluginFPot::handle_event()
{
        *param->output_f = get_value();
    param->update(0, 1);
        param->plugin->send_configure_change();
    param->gui->param_updated();
    return 1;
}

PluginIPot::PluginIPot(PluginParam *param, int x, int y)
 : BC_IPot(x,
        y,
        *param->output_i,
        (int)param->min,
        (int)param->max)
{
    this->param = param;
    set_use_caption(0);
}

int PluginIPot::handle_event()
{
        *param->output_i = get_value();
    param->update(0, 1);
        param->plugin->send_configure_change();
    param->gui->param_updated();
    return 1;
}


PluginQPot::PluginQPot(PluginParam *param, int x, int y)
 : BC_QPot(x,
        y,
        *param->output_q)
{
    this->param = param;
    set_use_caption(0);
}

int PluginQPot::handle_event()
{
        *param->output_q = get_value();
    param->update(0, 1);
        param->plugin->send_configure_change();
    param->gui->param_updated();
    return 1;
}

PluginText::PluginText(PluginParam *param, int x, int y, int value)
 : BC_TextBox(x,
    y,
    param->text_w,
    1,
    (int64_t)value,
    1,
    MEDIUMFONT)
{
    this->param = param;
}

PluginText::PluginText(PluginParam *param, int x, int y, float value)
 : BC_TextBox(x,
    y,
    param->text_w,
    1,
    (float)value,
    1,
    MEDIUMFONT,
    param->precision)
{
    this->param = param;
}

int PluginText::handle_event()
{
    if(param->output_i)
    {
        *param->output_i = atoi(get_text());
    }

    if(param->output_f)
    {
        *param->output_f = atof(get_text());
    }

    if(param->output_q)
    {
        *param->output_q = atoi(get_text());
    }
    param->update(1, 0);
    param->plugin->send_configure_change();
    param->gui->param_updated();
    return 1;
}


PluginClient::PluginClient(PluginServer *server)
{
	reset();
	this->server = server;
	smp = server->preferences->project_smp;
	defaults = 0;
	update_timer = new Timer;
// Virtual functions don't work here.
}

PluginClient::~PluginClient()
{
	if( thread ) {
		hide_gui();
		thread->join();
		delete thread;
	}

// Virtual functions don't work here.
	if(defaults) delete defaults;
	delete update_timer;
}

int PluginClient::reset()
{
	window_x = -1;
	window_y = -1;
	interactive = 0;
	show_initially = 0;
	wr = rd = 0;
	master_gui_on = 0;
	client_gui_on = 0;
	realtime_priority = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	total_out_buffers = 0;
	source_position = 0;
	source_start = 0;
	total_len = 0;
	direction = PLAY_FORWARD;
	thread = 0;
	using_defaults = 0;
	return 0;
}


void PluginClient::hide_gui()
{
	if(thread && thread->window)
	{
		thread->window->lock_window("PluginClient::hide_gui");
		thread->window->set_done(0);
		thread->window->unlock_window();
	}
}

// For realtime plugins initialize buffers
int PluginClient::plugin_init_realtime(int realtime_priority,
	int total_in_buffers,
	int buffer_size)
{

// Get parameters for all
	master_gui_on = get_gui_status();



// get parameters depending on video or audio
	init_realtime_parameters();

	this->realtime_priority = realtime_priority;
	this->total_in_buffers = this->total_out_buffers = total_in_buffers;
	this->out_buffer_size = this->in_buffer_size = buffer_size;
	return 0;
}

int PluginClient::plugin_start_loop(int64_t start,
	int64_t end,
	int64_t buffer_size,
	int total_buffers)
{
//printf("PluginClient::plugin_start_loop %d %ld %ld %ld %d\n",
// __LINE__, start, end, buffer_size, total_buffers);
	this->source_start = start;
	this->total_len = end - start;
	this->start = start;
	this->end = end;
	this->in_buffer_size = this->out_buffer_size = buffer_size;
	this->total_in_buffers = this->total_out_buffers = total_buffers;
	start_loop();
	return 0;
}

int PluginClient::plugin_process_loop()
{
	return process_loop();
}

int PluginClient::plugin_stop_loop()
{
	return stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, int64_t length)
{
	return server->start_progress(string, length);
}


// Non realtime parameters
int PluginClient::plugin_get_parameters()
{
	int result = get_parameters();
	if(defaults) save_defaults();
	return result;
}

// ========================= main loop

int PluginClient::is_multichannel() { return 0; }
int PluginClient::is_synthesis() { return 0; }
int PluginClient::is_realtime() { return 0; }
int PluginClient::is_fileio() { return 0; }
const char* PluginClient::plugin_title() { return _("Untitled"); }

Theme* PluginClient::new_theme() { return 0; }

int PluginClient::load_configuration()
{
	return 0;
}

Theme* PluginClient::get_theme()
{
	return server->get_theme();
}

int PluginClient::show_gui()
{
	load_configuration();
	thread = new PluginClientThread(this);
	thread->start();
	thread->init_complete->lock("PluginClient::show_gui");
// Must wait before sending any hide_gui
	if( !thread->window ) return 1;
	thread->window->init_wait();
	return 0;
}

void PluginClient::raise_window()
{
	if(thread && thread->window)
	{
		thread->window->lock_window("PluginClient::raise_window");
		thread->window->raise_window();
		thread->window->flush();
		thread->window->unlock_window();
	}
}

int PluginClient::set_string()
{
	if(thread)
	{
		thread->window->lock_window("PluginClient::set_string");
		thread->window->put_title(gui_string);
		thread->window->unlock_window();
	}
	return 0;
}





PluginClientFrames::PluginClientFrames()
{
	count = 0;
}
PluginClientFrames::~PluginClientFrames()
{
}

int PluginClientFrames::fwd_cmpr(PluginClientFrame *a, PluginClientFrame *b)
{
	double d = a->position - b->position;
	return d < 0 ? -1 : !d ? 0 : 1;
}

int PluginClientFrames::rev_cmpr(PluginClientFrame *a, PluginClientFrame *b)
{
	double d = b->position - a->position;
	return d < 0 ? -1 : !d ? 0 : 1;
}

void PluginClientFrames::reset()
{
	destroy();
	count = 0;
}

void PluginClientFrames::add_gui_frame(PluginClientFrame *frame)
{
	append(frame);
	++count;
}

void PluginClientFrames::concatenate(PluginClientFrames *frames)
{
	concat(*frames);
	count += frames->count;
	frames->count = 0;
}

void PluginClientFrames::sort_position(int dir)
{
// enforce order
	if( dir == PLAY_REVERSE )
		rev_sort();
	else
		fwd_sort();
}

// pop frames until buffer passes position=pos in direction=dir
// dir==0, pop frame; pos<0, pop all frames
// delete past frames, return last popped frame
PluginClientFrame* PluginClientFrames::get_gui_frame(double pos, int dir)
{
	if( dir ) {
		while( first != last ) {
			if( pos >= 0 && dir*(first->next->position - pos) > 0 ) break;
			delete first;  --count;
		}
	}
	PluginClientFrame *frame = first;
	if( frame ) { remove_pointer(frame);  --count; }
	return frame;
}

PluginClientFrame* PluginClient::get_gui_frame(double pos, int dir)
{
	return client_frames.get_gui_frame(pos, dir);
}
PluginClientFrame* PluginClient::next_gui_frame()
{
	return client_frames.first;
}


void PluginClient::plugin_update_gui()
{
	update_gui();
}

void PluginClient::update_gui()
{
}

int PluginClient::pending_gui_frame()
{
	PluginClientFrame *frame = client_frames.first;
	if( !frame ) return 0;
	double tracking_position = get_tracking_position();
	int direction = get_tracking_direction();
	int ret = !(direction == PLAY_REVERSE ?
		frame->position < tracking_position :
		frame->position > tracking_position);
	return ret;
}

int PluginClient::pending_gui_frames()
{
	PluginClientFrame *frame = client_frames.first;
	if( !frame ) return 0;
	double tracking_position = get_tracking_position();
	int direction = get_tracking_direction();
	int count = 0;
	while( frame && !(direction == PLAY_REVERSE ?
	    frame->position < tracking_position :
	    frame->position > tracking_position) ) {
		++count;  frame=frame->next;
	}
	return count;
}

void PluginClient::add_gui_frame(PluginClientFrame *frame)
{
	client_frames.add_gui_frame(frame);
}
int PluginClient::get_gui_frames()
{
	return client_frames.total();
}

double PluginClient::get_tracking_position()
{
	return server->mwindow->get_tracking_position();
}

int PluginClient::get_tracking_direction()
{
	return server->mwindow->get_tracking_direction();
}

void PluginClient::send_render_gui()
{
	server->send_render_gui(&client_frames);
}

void PluginClient::send_render_gui(void *data)
{
	server->send_render_gui(data);
}

void PluginClient::send_render_gui(void *data, int size)
{
	server->send_render_gui(data, size);
}


void PluginClient::plugin_reset_gui_frames()
{
	if( !thread ) return;
	BC_WindowBase *window = thread->get_window();
	if( !window ) return;
	window->lock_window("PluginClient::plugin_reset_gui_frames");
	client_frames.reset();
	window->unlock_window();
}

void PluginClient::plugin_render_gui_frames(PluginClientFrames *frames)
{
	if( !thread ) return;
	BC_WindowBase *window = thread->get_window();
	if( !window ) return;
	window->lock_window("PluginClient::render_gui");
	while( client_frames.count > MAX_FRAME_BUFFER )
		delete get_gui_frame(0, 0);
// append client frames to gui client_frames, consumes frames
	client_frames.concatenate(frames);
	client_frames.sort_position(get_tracking_direction());
	update_timer->update();
	window->unlock_window();
}

void PluginClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginClient::plugin_render_gui(void *data, int size)
{
	render_gui(data, size);
}

void PluginClient::render_gui(void *data)
{
        printf("PluginClient::render_gui %d\n", __LINE__);
}

void PluginClient::render_gui(void *data, int size)
{
        printf("PluginClient::render_gui %d\n", __LINE__);
}

void PluginClient::reset_gui_frames()
{
	server->reset_gui_frames();
}

int PluginClient::is_audio() { return 0; }
int PluginClient::is_video() { return 0; }
int PluginClient::is_theme() { return 0; }
int PluginClient::uses_gui() { return 1; }
int PluginClient::is_transition() { return 0; }
int PluginClient::load_defaults()
{
//	printf("PluginClient::load_defaults undefined in %s.\n", plugin_title());
	return 0;
}

int PluginClient::save_defaults()
{
	save_defaults_xml();
//	printf("PluginClient::save_defaults undefined in %s.\n", plugin_title());
	return 0;
}

void PluginClient::load_defaults_xml()
{
	char path[BCTEXTLEN];
	server->get_defaults_path(path);
	FileSystem fs;
	fs.complete_path(path);
	using_defaults = 1;
//printf("PluginClient::load_defaults_xml %d %s\n", __LINE__, path);

	char *data = 0;
	int64_t len = -1;
	struct stat st;
	int fd = open(path, O_RDONLY);
	if( fd >= 0 && !fstat(fd, &st) ) {
		int64_t sz = st.st_size;
		data = new char[sz+1];
		len = read(fd, data, sz);
		close(fd);
	}
	if( data && len >= 0 ) {
		data[len] = 0;
// Get window extents
		int i = 0;
		for( int state=0; i<len && state>=0; ++i ) {
			if( !data[i] || data[i] == '<' ) break;
			if( !isdigit(data[i]) ) continue;
			if( !state ) {
				window_x = atoi(data+i);
				state = 1;
			}
			else {
				window_y = atoi(data+i);
				state = -1;
			}
			while( i<len && isdigit(data[i]) ) ++i;
		}
		KeyFrame keyframe(data+i, len-i);
		read_data(&keyframe);
	}
	delete [] data;

	using_defaults = 0;
//printf("PluginClient::load_defaults_xml %d %s\n", __LINE__, path);
}

void PluginClient::save_defaults_xml()
{
	char path[BCTEXTLEN];
	server->get_defaults_path(path);
	FileSystem fs;
	fs.complete_path(path);
	using_defaults = 1;

	KeyFrame temp_keyframe;
	save_data(&temp_keyframe);

	const char *data = temp_keyframe.get_data();
	int len = strlen(data);
	FILE *fp = fopen(path, "w");

	if( fp ) {
		fprintf(fp, "%d\n%d\n", window_x, window_y);
		if( len > 0 && !fwrite(data, len, 1, fp) ) {
			fprintf(stderr, "PluginClient::save_defaults_xml %d \"%s\" %d bytes: %s\n",
				__LINE__, path, len, strerror(errno));
		}
		fclose(fp);
	}

	using_defaults = 0;
}

int PluginClient::is_defaults()
{
	return using_defaults;
}

BC_Hash* PluginClient::get_defaults()
{
	return defaults;
}
PluginClientThread* PluginClient::get_thread()
{
	return thread;
}

BC_WindowBase* PluginClient::new_window()
{
	printf("PluginClient::new_window undefined in %s.\n", plugin_title());
	return 0;
}
int PluginClient::get_parameters() { return 0; }
int PluginClient::get_samplerate() { return get_project_samplerate(); }
double PluginClient::get_framerate() { return get_project_framerate(); }
int PluginClient::init_realtime_parameters() { return 0; }
int PluginClient::delete_nonrealtime_parameters() { return 0; }
int PluginClient::start_loop() { return 0; };
int PluginClient::process_loop() { return 0; };
int PluginClient::stop_loop() { return 0; };

void PluginClient::set_interactive()
{
	interactive = 1;
}

int64_t PluginClient::get_in_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int64_t PluginClient::get_out_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int PluginClient::get_gui_status()
{
	return server->get_gui_status();
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

int PluginClient::stop_gui_client()
{
	if(!client_gui_on) return 0;
	client_gui_on = 0;
	return 0;
}

int PluginClient::get_project_samplerate()
{
	return server->get_project_samplerate();
}

double PluginClient::get_project_framerate()
{
	return server->get_project_framerate();
}

const char *PluginClient::get_source_path()
{
	Plugin *plugin = server->edl->tracks->plugin_exists(server->plugin_id);
	int64_t source_position = plugin->startproject;
	Edit *edit = plugin->track->edits->editof(source_position,PLAY_FORWARD,0);
	Indexable *indexable = edit ? edit->get_source() : 0;
	return indexable ? indexable->path : 0;
}


void PluginClient::update_display_title()
{
	server->generate_display_title(gui_string);
	set_string();
}

char* PluginClient::get_gui_string()
{
	return gui_string;
}


char* PluginClient::get_path()
{
	return server->path;
}

char* PluginClient::get_plugin_dir()
{
	return server->preferences->plugin_dir;
}

int PluginClient::set_string_client(char *string)
{
	strcpy(gui_string, string);
	set_string();
	return 0;
}


int PluginClient::get_interpolation_type()
{
	return server->get_interpolation_type();
}


float PluginClient::get_red()
{
	EDL *edl = get_edl();
	return edl->local_session->use_max ?
		edl->local_session->red_max :
		edl->local_session->red;
}

float PluginClient::get_green()
{
	EDL *edl = get_edl();
	return edl->local_session->use_max ?
		edl->local_session->green_max :
		edl->local_session->green;
}

float PluginClient::get_blue()
{
	EDL *edl = get_edl();
	return edl->local_session->use_max ?
		edl->local_session->blue_max :
		edl->local_session->blue;
}


int64_t PluginClient::get_source_position()
{
	return source_position;
}

int64_t PluginClient::get_source_start()
{
	return source_start;
}

int64_t PluginClient::get_total_len()
{
	return total_len;
}

int PluginClient::get_direction()
{
	return direction;
}

int64_t PluginClient::local_to_edl(int64_t position)
{
	return position;
}

int64_t PluginClient::edl_to_local(int64_t position)
{
	return position;
}

int PluginClient::get_use_opengl()
{
	return server->get_use_opengl();
}

int PluginClient::to_ram(VFrame *vframe)
{
	return server->to_ram(vframe);
}

int PluginClient::get_total_buffers()
{
	return total_in_buffers;
}

int PluginClient::get_buffer_size()
{
	return in_buffer_size;
}

int PluginClient::get_project_smp()
{
//printf("PluginClient::get_project_smp %d %d\n", __LINE__, smp);
	return smp;
}

const char* PluginClient::get_defaultdir()
{
	return File::get_plugin_path();
}


int PluginClient::send_hide_gui()
{
// Stop the GUI server and delete GUI messages
	client_gui_on = 0;
	return 0;
}

int PluginClient::send_configure_change()
{
	if(server->mwindow)
		server->mwindow->undo->update_undo_before(_("tweek"), this);
#ifdef USE_KEYFRAME_SPANNING
	EDL *edl = server->edl;
        Plugin *plugin = edl->tracks->plugin_exists(server->plugin_id);
	KeyFrames *keyframes = plugin ? plugin->keyframes : 0;
	KeyFrame keyframe(edl, keyframes);
	save_data(&keyframe);
	server->apply_keyframe(plugin, &keyframe);
#else
	KeyFrame* keyframe = server->get_keyframe();
// Call save routine in plugin
	save_data(keyframe);
#endif
	if(server->mwindow)
		server->mwindow->undo->update_undo_after(_("tweek"), LOAD_AUTOMATION);
	server->sync_parameters();
	return 0;
}

// virtual default spanning keyframe update.  If a range is selected,
// then changed parameters are copied to (prev + selected) keyframes.
// redefine per client for custom keyframe updates, see tracer, sketcher, crikey
void PluginClient::span_keyframes(KeyFrame *src, int64_t start, int64_t end)
{
	src->span_keyframes(start, end);
}


KeyFrame* PluginClient::get_prev_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_prev_keyframe(position);
}

KeyFrame* PluginClient::get_next_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_next_keyframe(position);
}

void PluginClient::get_camera(float *x, float *y, float *z, int64_t position)
{
	server->get_camera(x, y, z, position, direction);
}

void PluginClient::get_projector(float *x, float *y, float *z, int64_t position)
{
	server->get_projector(x, y, z, position, direction);
}


void PluginClient::output_to_track(float ox, float oy, float &tx, float &ty)
{
	float projector_x, projector_y, projector_z;
	int64_t position = get_source_position();
	get_projector(&projector_x, &projector_y, &projector_z, position);
	EDL *edl = get_edl();
	projector_x += edl->session->output_w / 2;
	projector_y += edl->session->output_h / 2;
	Plugin *plugin = edl->tracks->plugin_exists(server->plugin_id);
	Track *track = plugin ? plugin->track : 0;
	int track_w = track ? track->track_w : edl->session->output_w;
	int track_h = track ? track->track_h : edl->session->output_h;
	tx = (ox - projector_x) / projector_z + track_w / 2;
	ty = (oy - projector_y) / projector_z + track_h / 2;
}

void PluginClient::track_to_output(float tx, float ty, float &ox, float &oy)
{
	float projector_x, projector_y, projector_z;
	int64_t position = get_source_position();
	get_projector(&projector_x, &projector_y, &projector_z, position);
	EDL *edl = get_edl();
	projector_x += edl->session->output_w / 2;
	projector_y += edl->session->output_h / 2;
	Plugin *plugin = edl->tracks->plugin_exists(server->plugin_id);
	Track *track = plugin ? plugin->track : 0;
	int track_w = track ? track->track_w : edl->session->output_w;
	int track_h = track ? track->track_h : edl->session->output_h;
	ox = (tx - track_w / 2) * projector_z + projector_x;
	oy = (ty - track_h / 2) * projector_z + projector_y;
}


EDL *PluginClient::get_edl()
{
	return server->mwindow ? server->mwindow->edl : server->edl;
}

int PluginClient::gui_open()
{
	return server->gui_open();
}


