#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bcwindowbase.h"
#include "bcwindow.h"
#include "bcsignals.h"
#include "bccolors.h"
#include "clip.h"
#include "fonts.h"
#include "thread.h"
#include "vframe.h"

/*c++ -g -I/mnt1/build5/cinelerra-5.1/guicast x.C \
  /mnt1/build5/cinelerra-5.1/guicast/x86_64/libguicast.a \
  -DHAVE_GL -DHAVE_XFT -I/usr/include/freetype2 -lGL -lX11 -lXext \
  -lXinerama -lXv -lpng  -lfontconfig -lfreetype -lXft -pthread */

void wheel(VFrame *dst, float cx, float cy, float rad, int bg_color)
{
        int color_model = dst->get_color_model();
        int bpp = BC_CModels::calculate_pixelsize(color_model);
	int bg_r = (bg_color>>16) & 0xff;
	int bg_g = (bg_color>> 8) & 0xff;
	int bg_b = (bg_color>> 0) & 0xff;
	int w = dst->get_w(), h = dst->get_h();
	unsigned char **rows = dst->get_rows();
	for( int y=0; y<h; ++y ) {
		unsigned char *row = rows[y];
		for( int x=0; x<w; ++x,row+=bpp ) {
			int dx = cx-x, dy = cy-y;
			float d = sqrt(dx*dx + dy*dy);
			float r, g, b;
			if( d < rad ) {
			        float h = TO_DEG(atan2(cx-x, cy-y));
				if( h < 0 ) h += 360;
				float s = d / rad, v = 255;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
			}
			else {
				 r = bg_r; g = bg_g; b = bg_b;
			}
			row[0] = r; row[1] = g; row[2] = b;
		}
	}
}

class TestWindowGUI : public BC_Window
{
public:
	VFrame *wfrm;
	int bg;

	TestWindowGUI(int x, int y, int w, int h);
	~TestWindowGUI();

	void draw(int ww, int wh) {
		delete wfrm;
		wfrm = new VFrame(ww,wh,BC_RGB888);
		float wr = bmin(ww, wh)/2.25;
		int bg = get_bg_color();
		wheel(wfrm, ww/2,wh/2, wr, bg);
		draw_vframe(wfrm);
		flash();
	}
	int resize_event(int w, int h) {
		BC_WindowBase::resize_event(w, h);
		draw(w, h);
		return 0;
	}
};

TestWindowGUI::
TestWindowGUI(int x, int y, int w, int h)
 : BC_Window("test", x,y, w,h, 100,100)
{
	wfrm = 0;
	set_bg_color(0x885533);
	lock_window("init");
	clear_box(0,0,w,h);
	flash();
	unlock_window();
}

TestWindowGUI::
~TestWindowGUI()
{
}


class TestWindow : public Thread
{
	TestWindowGUI *gui;
public:
	TestWindow(int x, int y, int w, int h)
	 : Thread(1,0,0) {
		gui = new TestWindowGUI(x,y, w,h);
		gui->lock_window("init");
		gui->resize_event(w, h);
		gui->unlock_window();
		start();
	}
	~TestWindow() { delete gui; }
	void run() { gui->run_window(); }
	void close_window() { gui->close(0); }
};


int main(int ac, char **av)
{
	BC_Signals signals;
	signals.initialize();
	BC_WindowBase::init_resources(1.);
	TestWindow test_window(100, 100, 256, 256);
	test_window.join();
	return 0;
}

