
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

#include "bcsignals.h"
#include "guicast.h"

/*
c++ x.C -I/mnt1/build5/cinelerra-5.1/guicast \
  -L/mnt1/build5/cinelerra-5.1/guicast/x86_64 -lguicast \
  -DHAVE_GL -DHAVE_XFT -I/usr/include/freetype2 -lGL -lX11 -lXext \
  -lXinerama -lXv -lpng  -lfontconfig -lfreetype -lXft -pthread
*/

class TestWindow : public BC_Window
{
public:
	TestWindow() : BC_Window("Test", 0, 0, 320, 240) {};

	void create_objects()
	{
		lock_window("TestWindow::create_objects");
		set_color(BLACK);
		set_font(LARGEFONT);
		draw_text(10, 50, "Hello world");
		flash();
		flush();
		unlock_window();
	};
};


int main()
{
	BC_Signals signals;
	BC_WindowBase::init_resources(1.);
	TestWindow window;
	window.create_objects();
	window.run_window();
}
