#ifndef __X10TV_H__
#define __X10TV_H__
#ifdef HAVE_X10TV

#include "remotecontrol.h"
#include "thread.h"

#define X10_A		0x001e
#define X10_B		0x0030
#define X10_POWER	0x0074
#define X10_TV		0x0179
#define X10_DVD		0x0185
#define X10_WWW		0x0096
#define X10_BOOK	0x009c
#define X10_EDIT	0x00b0
#define X10_VOLDN	0x0072
#define X10_VOLUP	0x0073
#define X10_MUTE	0x0071
#define X10_CH_DN	0x0193
#define X10_CH_UP	0x0192
#define X10_1		0x0201
#define X10_2		0x0202
#define X10_3		0x0203
#define X10_4		0x0204
#define X10_5		0x0205
#define X10_6		0x0206
#define X10_7		0x0207
#define X10_8		0x0208
#define X10_9		0x0209
#define X10_MENU	0x008b
#define X10_0		0x0200
#define X10_SETUP	0x008d
#define X10_C		0x002e
#define X10_UP		0x0067
#define X10_D		0x0020
#define X10_PROPS	0x0082
#define X10_LT		0x0069
#define X10_OK		0x0160
#define X10_RT		0x006a
#define X10_SCRN	0x0177
#define X10_E		0x0012
#define X10_DN		0x006c
#define X10_F		0x0021
#define X10_REW		0x00a8
#define X10_PLAY	0x00cf
#define X10_FWD		0x00d0
#define X10_REC		0x00a7
#define X10_STOP	0x00a6
#define X10_PAUSE	0x0077

// unknown keysyms
//#define X10_NEXT	0x0000
//#define X10_PREV	0x0000
//#define X10_INFO	0x0000
//#define X10_HOME	0x0000
//#define X10_END	0x0000
//#define X10_SELECT	0x0000

struct input_event;

class X10TV : public Thread
{
public:
	X10TV(MWindow *mwindow, int *ifd, int nfds);
	~X10TV();

	void stop();
	void start();
	static int open_usb_inputs(int vendor, int product, int &version,
			int *ifds, int nfds);
	static X10TV *probe(MWindow *mwindow);
	void run();
	void handle_event(int fd);
	int check_menu_keys(int code);
	virtual int process_code() { return 1; }

	MWindow *mwindow;
	input_event *ev;
	int done;
	int *ifds, nfds;
	int last_code, code;
	fd_set rfds;
	int mfd;
};

class X10TVCWindowHandler : public RemoteHandler
{
public:
	X10TVCWindowHandler(X10TV *wintv, RemoteControl *remote_control);
	int x10tv_process_code(int code);
	int process_key(int key);
	int is_x10tv() { return 1; }

	X10TV *x10tv;
};

class X10TVRecordHandler : public RemoteHandler
{
public:
	X10TVRecordHandler(X10TV *wintv, RemoteControl *remote_control);
	int wintv_process_code(int code);
	int process_key(int key);
	int is_x10tv() { return 1; }

	X10TV *x10tv;
};

#endif
#endif
