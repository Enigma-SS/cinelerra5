#ifndef __BC_DRAGRECT_H__
#define __BC_DRAGRECT_H__

#include "bcwindowbase.inc"
#include "bcdragbox.inc"
#include "bcpopup.h"
#include "thread.h"


class BC_DragBox : public Thread
{
public:
	BC_DragBox(BC_WindowBase *parent);
	~BC_DragBox();
	void start_drag();
	void run();
	virtual int handle_done_event(int x0, int y0, int x1, int y1) { return 0; }

	BC_Popup *edge[4];
	BC_WindowBase *parent;
	BC_DragBoxPopup *popup;
	int done;
};

class BC_DragBoxPopup : public BC_Popup
{
public:
	BC_DragBoxPopup(BC_DragBox *grab_thread);
	~BC_DragBoxPopup();
	int grab_event(XEvent *event);
	void update();
	void draw_selection(int show);

	BC_DragBox *grab_thread;
	int dragging;
	int grab_color;
	int x0, y0, x1, y1;
	int lx0, ly0, lx1, ly1;
};

#endif
