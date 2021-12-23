#ifndef __WINTV_H__
#define __WINTV_H__
// as of 2020/01/06 using kernel version 5.3.16
//  a patch is needed for: drivers/media/usb/em28xx/em28xx-rc.ko
//  prototype in thirdparty/src/em28xx-input.patch1
//#define HAVE_WINTV
#ifdef HAVE_WINTV

#include "remotecontrol.h"
#include "thread.h"

#define WTV_OK          0x0160
#define WTV_LT          0x0069
#define WTV_UP          0x0067
#define WTV_RT          0x006a
#define WTV_DN          0x006c
#define WTV_HOME        0x0162
#define WTV_BACK        0x00ae
#define WTV_MENU        0x008b
#define WTV_TV          0x0179
#define WTV_POWER       0x0074
#define WTV_VOLUP       0x0073
#define WTV_VOLDN       0x0072
#define WTV_CH_UP       0x0192
#define WTV_CH_DN       0x0193
#define WTV_1           0x0201
#define WTV_2           0x0202
#define WTV_3           0x0203
#define WTV_4           0x0204
#define WTV_5           0x0205
#define WTV_6           0x0206
#define WTV_7           0x0207
#define WTV_8           0x0208
#define WTV_9           0x0209
#define WTV_TEXT        0x0184
#define WTV_0           0x0200
#define WTV_CC          0x0172
#define WTV_BOX         0x0080
#define WTV_START       0x0195
#define WTV_REV         0x00a8
#define WTV_STOP        0x0077
#define WTV_PLAY        0x00cf
#define WTV_FWD         0x00d0
#define WTV_END         0x0197
#define WTV_MUTE        0x0071
#define WTV_PREV        0x019c
#define WTV_RED         0x00a7

struct input_event;

class WinTV : public Thread
{
public:
	WinTV(MWindow *mwindow, int ifd);
	~WinTV();

	void stop();
	void start();
	static int open_usb_input(int vendor, int product, int &version);
	static WinTV *probe(MWindow *mwindow);
	void run();
	void handle_event();
	int check_menu_keys(int code);
	virtual int process_code() { return 1; }

	MWindow *mwindow;
	input_event *ev;
	int done, ifd;
	int last_code, code;
};

class WinTVCWindowHandler : public RemoteHandler
{
public:
	WinTVCWindowHandler(WinTV *wintv, RemoteControl *remote_control);
	int wintv_process_code(int code);
	int process_key(int key);
	int is_wintv() { return 1; }

	WinTV *wintv;
};

class WinTVRecordHandler : public RemoteHandler
{
public:
	WinTVRecordHandler(WinTV *wintv, RemoteControl *remote_control);
	int wintv_process_code(int code);
	int process_key(int key);
	int is_wintv() { return 1; }

	WinTV *wintv;
};

#endif
#endif
