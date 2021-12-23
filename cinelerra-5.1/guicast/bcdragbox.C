#include "bcdragbox.h"
#include "bcmenuitem.h"
#include "bctimer.h"
#include "bcwindowbase.h"
#include "colors.h"

BC_DragBox::BC_DragBox(BC_WindowBase *parent)
 : Thread(1, 0, 0)
{
	this->parent = parent;
	popup = 0;
	done = -1;
}
BC_DragBox::~BC_DragBox()
{
	if( running() ) {
		done = 1;
		cancel();
	}
	join();
	delete popup;
}

void BC_DragBox::start_drag()
{
	popup = new BC_DragBoxPopup(this);
	popup->lock_window("BC_DragBox::start");
	for( int i=0; i<4; ++i )
		edge[i] = new BC_Popup(parent, 0,0, 1,1, ORANGE, 1);
	parent->grab_buttons();
	parent->grab_cursor();
	popup->grab(parent);
	popup->create_objects();
	popup->show_window();
	popup->unlock_window();
	done = 0;
	Thread::start();
}

void BC_DragBox::run()
{
	popup->lock_window("BC_DragBox::run 0");
	while( !done ) {
		popup->update();
		popup->unlock_window();
		enable_cancel();
		Timer::delay(200);
		disable_cancel();
		popup->lock_window("BC_DragBox::run 1");
	}
	int x0 = popup->lx0, y0 = popup->ly0;
	int x1 = popup->lx1, y1 = popup->ly1;
	parent->ungrab_cursor();
	parent->ungrab_buttons();
	popup->ungrab(parent);
	for( int i=0; i<4; ++i ) delete edge[i];
	popup->unlock_window();
	delete popup;  popup = 0;
	handle_done_event(x0, y0, x1, y1);
}

BC_DragBoxPopup::BC_DragBoxPopup(BC_DragBox *grab_thread)
 : BC_Popup(grab_thread->parent, 0,0, 16,16, -1,1)
{
	this->grab_thread = grab_thread;
	dragging = -1;
	grab_color = ORANGE;
	x0 = y0 = x1 = y1 = -1;
	lx0 = ly0 = lx1 = ly1 = -1;
}

BC_DragBoxPopup::~BC_DragBoxPopup()
{
}

int BC_DragBoxPopup::grab_event(XEvent *event)
{
	int cur_drag = dragging;
	switch( event->type ) {
	case ButtonPress: {
		if( cur_drag > 0 ) return 1;
		int x0 = event->xbutton.x_root;
		int y0 = event->xbutton.y_root;
		if( !cur_drag ) {
			draw_selection(-1);
			if( event->xbutton.button == RIGHT_BUTTON ) break;
			if( x0>=get_x() && x0<get_x()+get_w() &&
			    y0>=get_y() && y0<get_y()+get_h() ) break;
		}
		this->x0 = this->x1 = x0;
		this->y0 = this->y1 = y0;
		draw_selection(1);
		dragging = 1;
		return 1; }
	case ButtonRelease:
		dragging = 0;
	case MotionNotify:
		if( cur_drag > 0 ) {
			this->x1 = event->xbutton.x_root;
			this->y1 = event->xbutton.y_root;
			draw_selection(0);
		}
		return 1;
	default:
		return 0;
	}

	hide_window();
	sync_display();
	grab_thread->done = 1;
	return 1;
}

void BC_DragBoxPopup::update()
{
	set_color(grab_color ^= GREEN);
	draw_box(0,0, get_w(),get_h());
	flash(1);
}

void BC_DragBoxPopup::draw_selection(int show)
{
	if( show < 0 ) {
		for( int i=0; i<4; ++i ) hide_window(0);
		flush();
		return;
	}

	int nx0 = x0 < x1 ? x0 : x1;
	int nx1 = x0 < x1 ? x1 : x0;
	int ny0 = y0 < y1 ? y0 : y1;
	int ny1 = y0 < y1 ? y1 : y0;
	lx0 = nx0;  lx1 = nx1;
	ly0 = ny0;  ly1 = ny1;

	--nx0;  --ny0;
	BC_Popup **edge = grab_thread->edge;
	edge[0]->reposition_window(nx0,ny0, nx1-nx0, 1);
	edge[1]->reposition_window(nx1,ny0, 1, ny1-ny0);
	edge[2]->reposition_window(nx0,ny1, nx1-nx0, 1);
	edge[3]->reposition_window(nx0,ny0, 1, ny1-ny0);

	if( show > 0 ) {
		for( int i=0; i<4; ++i ) edge[i]->show_window(0);
	}
	flush();
}

