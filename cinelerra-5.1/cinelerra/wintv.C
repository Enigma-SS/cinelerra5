#ifdef HAVE_WINTV

#include "channelinfo.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "language.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "record.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "remotecontrol.h"
#include "tracks.h"
#include "wintv.h"

#include <dirent.h>
#include <linux/input.h>

WinTV::WinTV(MWindow *mwindow, int ifd)
{
	this->mwindow = mwindow;
	this->ifd = ifd;

	ev = new input_event;
	memset(ev, 0, sizeof(*ev));
	ev->code = -1;
	done = -1;
	last_code = -1;
	code = -1;
}

WinTV::~WinTV()
{
	stop();
	delete ev;
}

void WinTV::stop()
{
	if( ifd >= 0 ) {
		ioctl(ifd, EVIOCGRAB, 0);
		close(ifd);
		ifd = -1;
	}
	if( running() && !done ) {
		done = 1;
		cancel();
		join();
	}
}

void WinTV::start()
{
	ioctl(ifd, EVIOCGRAB, 1);
	done = 0;
	Thread::start();
}

int WinTV::open_usb_input(int vendor, int product, int &version)
{
	int ret = -1;
	const char *dev_input = "/dev/input";
	DIR *dir = opendir(dev_input);
	if( !dir ) return ret;

	struct dirent64 *dp;
	struct input_id dev_id;
	while( (dp = readdir64(dir)) != 0 ) {
		char *fn = dp->d_name;
		if( !strcmp(fn, ".") || !strcmp(fn, "..") ) continue;
		char path[PATH_MAX];  struct stat st;
		snprintf(path, PATH_MAX, "%s/%s", dev_input, fn);
		if( stat(path, &st) < 0 ) continue;
		if( !S_ISCHR(st.st_mode) ) continue;
		int fd = open(path, O_RDONLY);
		if( fd < 0 ) continue;
		if( !ioctl(fd, EVIOCGID, &dev_id) ) {
			if( dev_id.bustype == BUS_USB &&
			    dev_id.vendor == vendor &&
			    dev_id.product == product ) {
				version = dev_id.version;
				ret = fd;
				break;
			}
		}
		close(fd);
	}
	closedir(dir);
	return ret;
}

WinTV *WinTV::probe(MWindow *mwindow)
{
	int ver = -1;
	int ifd = open_usb_input(0x2040, 0x826d, ver);
	if( ifd < 0 ) return 0;
	printf("detected hauppauge WinTV Model 1657, ver=0x%04x\n", ver);
	return new WinTV(mwindow, ifd);
}

void WinTV::run()
{
	enable_cancel();
	while( !done ) {
		int ret = read(ifd, ev, sizeof(*ev));
		if( done ) break;
		if( ret != sizeof(*ev) ) {
			if( ret < 0 ) { perror("read event"); break; }
			fprintf(stderr, "bad read: %d\n", ret);
			break;
		}
		handle_event();
	}
}

int WinTV::check_menu_keys(int code)
{
	int result = 1;
	switch( code ) {
	case WTV_POWER:
		mwindow->quit();
		break;
	case WTV_TV: {
		Record *record = mwindow->gui->record;
		if( !record->running() )
			record->start();
		else
			record->record_gui->interrupt_thread->start(0);
		break; }
	case WTV_MENU:
#ifdef HAVE_DVB
		mwindow->gui->channel_info->toggle_scan();
#endif
		break;
	case WTV_RED: {
		RemoteControl *remote_control = mwindow->gui->remote_control;
		if( !remote_control->deactivate() )
			remote_control->activate();
		break; }
	default:
		result = 0;
	}
	return result;
}

void WinTV::handle_event()
{
	switch(ev->type) {
	case EV_KEY: {
		if( !ev->value ) break;
		this->last_code = this->code;
		this->code = ev->code;
		if( check_menu_keys(code) ) break;
		RemoteHandler *handler = mwindow->gui->remote_control->handler;
		if( handler )
			handler->process_key(ev->code);
		break; }
	case EV_SYN:
	case EV_MSC:
		break;
	default: {
		time_t t = ev->time.tv_sec;
		struct tm *tp = localtime(&t);
		printf("wintv event: %4d/%02d/%02d %02d:%02d:%02d.%03d = (%d, %d, 0x%x)\n",
			tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
			tp->tm_hour, tp->tm_min, tp->tm_sec,
			(int)ev->time.tv_usec/1000, ev->type, ev->code, ev->value);
		break; }
	}
}


int WinTVCWindowHandler::wintv_process_code(int code)
{
	MWindow *mwindow = wintv->mwindow;
	EDL *edl = mwindow->edl;
	if( !edl ) return 0;
	PlayTransport *transport = mwindow->gui->mbuttons->transport;
	if( !transport->get_edl() ) return 0;
	PlaybackEngine *engine = transport->engine;
	double position = engine->get_tracking_position();
	double length = edl->tracks->total_length();
	int next_command = -1;

	switch( code ) {
	case WTV_OK:
		break;
// select window tile config = BACK 1,2,3
	case WTV_1: case WTV_2: case WTV_3:
		if( mwindow->wintv->last_code == WTV_BACK ) {
			RemoteGUI *rgui = mwindow->gui->cwindow_remote_handler->gui;
			rgui->tile_windows(code - WTV_1);
			return 1;
		} // fall thru
// select asset program config = TEXT 1,2,3,4,5,6
	case WTV_4: case WTV_5: case WTV_6:
		if( mwindow->wintv->last_code == WTV_TEXT ) {
			mwindow->select_asset(code - WTV_1, 1);
			break;
		} // fall thru
// select position in 10 percent units
	case WTV_7: case WTV_8: case WTV_9:
	case WTV_0:
		position = length * (code - WTV_0)/10.0;
		break;
// jump +- 10/60 secs
	case WTV_LT:  position -= 10.0;  break;
	case WTV_UP:  position += 60.0;  break;
	case WTV_RT:  position += 10.0;  break;
	case WTV_DN:  position -= 60.0;  break;
	case WTV_BACK: return 1;
	case WTV_HOME: {
		CWindowCanvas *canvas = mwindow->cwindow->gui->canvas;
		int on = canvas->get_fullscreen() ? 0 : 1;
		canvas->Canvas::set_fullscreen(on, 0);
		return 1; }
	case WTV_VOLUP: return 1;
	case WTV_VOLDN: return 1;
	case WTV_CH_UP: return 1;
	case WTV_CH_DN: return 1;
	case WTV_TEXT:  return 1;
	case WTV_CC:    return 1;
	case WTV_BOX:   return 1;
	case WTV_START: next_command = SLOW_REWIND;  break;
	case WTV_REV:   next_command = FAST_REWIND;  break;
	case WTV_STOP:  next_command = STOP;         break;
	case WTV_PLAY:  next_command = NORMAL_FWD;   break;
	case WTV_FWD:   next_command = FAST_FWD;     break;
	case WTV_END:   next_command = SLOW_FWD;     break;
	case WTV_MUTE:  return 1;
	default:
		printf("wintv cwindow: unknown code: %04x\n", code);
		return -1;
	}

	if( next_command < 0 ) {
		if( position < 0 ) position = 0;
		transport->change_position(position);
	}
	else
		transport->handle_transport(next_command);
	return 0;
}

int WinTVCWindowHandler::process_key(int key)
{
	return wintv_process_code(key);
}

int WinTVRecordHandler::process_key(int key)
{
	Record *record = wintv->mwindow->gui->record;
	return record->wintv_process_code(key);
}


WinTVRecordHandler::WinTVRecordHandler(WinTV *wintv, RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, GREEN)
{
	this->wintv = wintv;
}

WinTVCWindowHandler::WinTVCWindowHandler(WinTV *wintv, RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, BLUE)
{
	this->wintv = wintv;
}

// HAVE_WINTV
#endif
