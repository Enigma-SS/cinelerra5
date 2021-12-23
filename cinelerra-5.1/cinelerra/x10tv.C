#ifdef HAVE_X10TV

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
#include "x10tv.h"

#include <dirent.h>
#include <linux/input.h>

X10TV::X10TV(MWindow *mwindow, int *fds, int nfds)
{
	this->mwindow = mwindow;
	this->ifds = new int[nfds];
	this->nfds = nfds;
	for( int i=0; i<nfds; ++i ) this->ifds[i] = fds[i];

	ev = new input_event;
	memset(ev, 0, sizeof(*ev));
	ev->code = -1;
	done = -1;
	last_code = -1;
	code = -1;
	FD_ZERO(&rfds);
	mfd = -1;
}

X10TV::~X10TV()
{
	stop();
	delete ev;
}

void X10TV::stop()
{
	done = 1;
	for( int i=nfds; --i>=0; ) {
		ioctl(ifds[i], EVIOCGRAB, 0);
		close(ifds[i]);
	}
	nfds = 0;
	if( running() ) {
		cancel();
		join();
	}
}

void X10TV::start()
{
	FD_ZERO(&rfds);
	mfd = -1;
	for( int i=nfds; --i>=0; ) {
		int fd = ifds[i];
		ioctl(fd, EVIOCGRAB, 1);
		if( fd >= mfd ) mfd = fd+1;
		FD_SET(fd, &rfds);
	}
	done = 0;
	Thread::start();
}

int X10TV::open_usb_inputs(int vendor, int product, int &version,
		int *ifds, int mxfds)
{
	int ret = -1;
	const char *dev_input = "/dev/input";
	DIR *dir = opendir(dev_input);
	if( !dir ) return ret;

	struct dirent64 *dp;
	struct input_id dev_id;
	int nfds = 0;
	while( nfds < mxfds && (dp = readdir64(dir)) != 0 ) {
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
				unsigned props = 0;
				// quirk, reports pointing_stick for keys
				unsigned mptrs =
					 (1<<INPUT_PROP_POINTING_STICK);
				int ret = ioctl(fd, EVIOCGPROP(sizeof(props)), &props);
				if( ret == sizeof(props) && (props & mptrs) ) {
					version = dev_id.version;
					ifds[nfds++] = fd;
					continue;
				}
			}
		}
		close(fd);
	}
	closedir(dir);
	return nfds;
}


X10TV *X10TV::probe(MWindow *mwindow)
{
	int ver = -1, ifds[16];
	int nfds = open_usb_inputs(0x0bc7, 0x0004, ver, ifds, 16);
	if( nfds <= 0 ) return 0;
	printf("detected ATI X10 remote, ver=0x%04x\n", ver);
	return new X10TV(mwindow, ifds, nfds);
}

void X10TV::run()
{
	enable_cancel();
	while( !done ) {
		fd_set rds = rfds;
		int ret = select(mfd, &rds, 0, 0, 0);
		if( ret < 0 ) break;
		int fd = -1, k = ret > 0 ? nfds : 0;
		while( --k >= 0 ) {
			int ifd = ifds[k];
			if( FD_ISSET(ifd, &rds) ) {
				fd = ifd;  break;
			}
		}
		if( fd < 0 ) {
			printf("select failed\n");
			usleep(100000);  continue;
		}
		ret = read(fd, ev, sizeof(*ev));
		if( done ) break;
		if( ret != sizeof(*ev) ) {
			if( ret < 0 ) { perror("read event"); break; }
			fprintf(stderr, "bad read: %d\n", ret);
			break;
		}
		handle_event(fd);
	}
}

int X10TV::check_menu_keys(int code)
{
	int result = 1;
	switch( code ) {
	case X10_POWER:
		mwindow->quit();
		break;
	case X10_TV: {
		Record *record = mwindow->gui->record;
		if( !record->running() )
			record->start();
		else
			record->record_gui->interrupt_thread->start(0);
		break; }
	case X10_BOOK:
#ifdef HAVE_DVB
		mwindow->gui->channel_info->toggle_scan();
#endif
		break;
	case X10_EDIT: {
		RemoteControl *remote_control = mwindow->gui->remote_control;
		if( !remote_control->deactivate() )
			remote_control->activate();
		break; }
	default:
		result = 0;
	}
	return result;
}

void X10TV::handle_event(int fd)
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
		printf("x10tv event %d: %4d/%02d/%02d %02d:%02d:%02d.%03d = (%d, %d, 0x%x)\n",
			fd, tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
			tp->tm_hour, tp->tm_min, tp->tm_sec,
			(int)ev->time.tv_usec/1000, ev->type, ev->code, ev->value);
		break; }
	}
}


int X10TVCWindowHandler::x10tv_process_code(int code)
{
	MWindow *mwindow = x10tv->mwindow;
	EDL *edl = mwindow->edl;
	if( !edl ) return 0;
	PlayTransport *transport = mwindow->gui->mbuttons->transport;
	if( !transport->get_edl() ) return 0;
	PlaybackEngine *engine = transport->engine;
	double position = engine->get_tracking_position();
	double length = edl->tracks->total_length();
	int next_command = -1;

	switch( code ) {
	case X10_A:		break;
	case X10_B:		break;
	case X10_POWER:		break;
	case X10_TV:		break;
	case X10_DVD:		break;
	case X10_WWW:		break;
	case X10_BOOK:		break;
	case X10_EDIT:		break;
	case X10_VOLDN:		return 1;
	case X10_VOLUP:		return 1;
	case X10_MUTE:		break;
	case X10_CH_DN:		break;
	case X10_CH_UP:		break;
// select window tile config = BACK 1,2,3
	case X10_1: case X10_2:	case X10_3:
		if( mwindow->x10tv->last_code == X10_MENU ) {
			RemoteGUI *rgui = mwindow->gui->cwindow_remote_handler->gui;
			rgui->tile_windows(code - X10_1);
			return 1;
		} // fall thru
// select asset program config = TEXT 1,2,3,4,5,6
	case X10_4: case X10_5:	case X10_6:
		if( mwindow->x10tv->last_code == X10_SETUP ) {
			mwindow->select_asset(code - X10_1, 1);
			break;
		} // fall thru
	case X10_7: case X10_8: case X10_9:
	case X10_0:
// select position in 10 percent units
		position = length * (code - X10_0)/10.0;
		break;
	case X10_MENU:		return 0;
	case X10_SETUP:		return 0;
	case X10_C:		return 1;
	case X10_UP:  position += 60.0;  break;
	case X10_D:		return 1;
	case X10_PROPS:		return 1;
	case X10_LT:  position -= 10.0;  break;
	case X10_OK:		return 1;
	case X10_RT:  position += 10.0;  break;
	case X10_SCRN: {
		CWindowCanvas *canvas = mwindow->cwindow->gui->canvas;
		int on = canvas->get_fullscreen() ? 0 : 1;
		canvas->Canvas::set_fullscreen(on, 0);
		return 1; }
	case X10_E:		return 1;
	case X10_DN:  position -= 60.0;  break;
	case X10_F:		return 1;
	case X10_REW:	next_command = FAST_REWIND;	break;
	case X10_PLAY:	next_command = NORMAL_FWD;	break;
	case X10_FWD:	next_command = FAST_FWD;	break;
	case X10_REC:	next_command = SLOW_REWIND;	break;
	case X10_STOP:	next_command = STOP;		break;
	case X10_PAUSE:	next_command = SLOW_FWD;	break;

	default:
		printf("x10tv cwindow: unknown code: %04x\n", code);
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

int X10TVCWindowHandler::process_key(int key)
{
	return x10tv_process_code(key);
}

int X10TVRecordHandler::process_key(int key)
{
	Record *record = x10tv->mwindow->gui->record;
	return record->x10tv_process_code(key);
}


X10TVRecordHandler::X10TVRecordHandler(X10TV *x10tv, RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, GREEN)
{
	this->x10tv = x10tv;
}

X10TVCWindowHandler::X10TVCWindowHandler(X10TV *x10tv, RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, BLUE)
{
	this->x10tv = x10tv;
}

// HAVE_X10TV
#endif
