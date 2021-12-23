
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

#include "clip.h"
#include "language.h"
#include "loadmode.h"
#include "mwindow.h"
#include "theme.h"

#define LOADMODE_LOAD_TEXT _("Load strategy:")
#define LOADMODE_EDL_TEXT _("EDL strategy:")

// Must match macros
static const char *mode_images[] =
{
	"loadmode_none",
	"loadmode_new",
	"loadmode_newcat",
	"loadmode_newtracks",
	"loadmode_cat",
	"loadmode_paste",
	"loadmode_resource",
	"loadmode_edl_clip",
	"loadmode_edl_nested",
	"loadmode_edl_fileref",
};

static const char *mode_text[] =
{
	N_("Insert nothing"),
	N_("Replace current project"),
	N_("Replace current project and concatenate tracks"),
	N_("Append in new tracks"),
	N_("Concatenate to existing tracks"),
	N_("Paste over selection/at insertion point"),
	N_("Create new resources only"),
	N_("EDL"),
	N_("Nested"),
	N_("Reference"),
};


LoadModeItem::LoadModeItem(const char *text, int value)
 : BC_ListBoxItem(text)
{
	this->value = value;
}


LoadModeToggle::LoadModeToggle(int x, int y, LoadMode *window, int id,
		int *output, const char *images, const char *tooltip)
 : BC_Toggle(x, y, window->mwindow->theme->get_image_set(images), *output)
{
	this->window = window;
	this->id = id;
	this->output = output;
	set_tooltip(tooltip);
}

int LoadModeToggle::handle_event()
{
	*output = id;
	window->update();
	return 1;
}



LoadMode::LoadMode(MWindow *mwindow, BC_WindowBase *window,
		int x, int y, int *load_mode, int *edl_mode,
		int use_nothing, int line_wrap)
{
	this->mwindow = mwindow;
	this->window = window;
	this->x = x;
	this->y = y;
	this->load_mode = load_mode;
	this->edl_mode = edl_mode;
	this->use_nothing = use_nothing;
	this->line_wrap = line_wrap;
	for( int i=0; i<TOTAL_LOADMODES; ++i ) mode[i] = 0;
	load_title = 0;
	edl_title = 0;
}

LoadMode::~LoadMode()
{
	load_modes.remove_all_objects();
	for( int i=0; i<TOTAL_LOADMODES; ++i ) delete mode[i];
}

const char *LoadMode::mode_to_text(int mode)
{
	for( int i=0; i<load_modes.total; ++i ) {
		if( load_modes[i]->value == mode )
			return load_modes[i]->get_text();
	}
	return _("Unknown");
}

void LoadMode::load_mode_geometry(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap,
		int *pw, int *ph)
{
	int pad = 5;
	const char *load_text = LOADMODE_LOAD_TEXT;
	int mw = BC_Title::calculate_w(gui, load_text);
	int mh = BC_Title::calculate_h(gui, load_text);
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = theme->loadmode_w + 24;
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		switch( i ) {
		case LOADMODE_NOTHING:
			if( !use_nothing) continue;
			break;
		case LOADMODE_EDL_CLIP:
		case LOADMODE_EDL_NESTED:
		case LOADMODE_EDL_FILEREF:
			if( !use_nested ) continue;
			if( iy ) break;
			ix = 0;  iy = mh + pad;
			const char *edl_text = LOADMODE_EDL_TEXT;
			ix += bmax(BC_Title::calculate_w(gui, load_text),
				   BC_Title::calculate_w(gui, edl_text)) + 2*pad;
			break;
		}
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(gui,
			theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = 0;  iy = mh+pad;
	mh = iy + BC_TextBox::calculate_h(gui, MEDIUMFONT, 1, 1);
	if( pw ) *pw = mw;
	if( ph ) *ph = mh;
}

int LoadMode::calculate_w(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap)
{
	int result = 0;
	load_mode_geometry(gui, theme, use_nothing, use_nested, line_wrap,
			&result, 0);
	return result;
}

int LoadMode::calculate_h(BC_WindowBase *gui, Theme *theme,
		int use_nothing, int use_nested, int line_wrap)
{
	int result = 0;
	load_mode_geometry(gui, theme, use_nothing, use_nested, line_wrap,
			0, &result);
	return result;
}

void LoadMode::create_objects()
{
	int pad = 5;
	load_title = new BC_Title(x, y, LOADMODE_LOAD_TEXT);
	window->add_subwindow(load_title);
	int mw = load_title->get_w(), mh = load_title->get_h();
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = mwindow->theme->loadmode_w + 24;
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		int *mode_set = load_mode;
		switch( i ) {
		case LOADMODE_NOTHING:
			if( !use_nothing) continue;
			break;
		case LOADMODE_EDL_CLIP:
		case LOADMODE_EDL_NESTED:
		case LOADMODE_EDL_FILEREF:
			if( !edl_mode ) continue;
			mode_set = edl_mode;
			if( iy ) break;
			ix = 0;  iy = mh + pad;
			edl_title = new BC_Title(x+ix, y+iy, LOADMODE_EDL_TEXT);
			window->add_subwindow(edl_title);
			ix += bmax(load_title->get_w(), edl_title->get_w()) + 2*pad;
			break;
		}
		load_modes.append(new LoadModeItem(_(mode_text[i]), i));
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(window,
			mwindow->theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		mode[i] = new LoadModeToggle(x+ix, y+iy, this, i,
			mode_set, mode_images[i], _(mode_text[i]));
		window->add_subwindow(mode[i]);
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = xS(25);  iy = mh+pad;
	const char *mode_text = mode_to_text(*load_mode);
	textbox = new BC_TextBox(x+ix, y+iy,
		mwindow->theme->loadmode_w-2*ix, 1, mode_text);
	window->add_subwindow(textbox);
	ix += textbox->get_w();
	listbox = new LoadModeListBox(window, this, x+ix, y+iy);
	window->add_subwindow(listbox);
	mh = iy + textbox->get_h();
	update();
}

int LoadMode::reposition_window(int x, int y)
{
	this->x = x;  this->y = y;
	load_title->reposition_window(x, y);
	int mw = load_title->get_w(), mh = load_title->get_h();
	int pad = xS(5);
	int ix = mw + 2*pad, iy = 0, x1 = ix;
	int ww = mwindow->theme->loadmode_w + xS(24);
	if( mw < ww ) mw = ww;

	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		switch( i ) {
		case LOADMODE_NOTHING:
			if( !use_nothing) continue;
			break;
		case LOADMODE_EDL_CLIP:
		case LOADMODE_EDL_NESTED:
		case LOADMODE_EDL_FILEREF:
			if( !edl_mode ) continue;
			if( iy ) break;
			ix = 0;  iy = mh + pad;
			edl_title->reposition_window(x+ix, y+iy);
			ix += bmax(load_title->get_w(), edl_title->get_w()) + 2*pad;
			break;
		}
		int text_line, w, h, toggle_x, toggle_y;
		int text_x, text_y, text_w, text_h;
		BC_Toggle::calculate_extents(window,
			mwindow->theme->get_image_set(mode_images[i]), 0,
			&text_line, &w, &h, &toggle_x, &toggle_y,
			&text_x, &text_y, &text_w, &text_h, 0, MEDIUMFONT);
		if( line_wrap && ix+w > ww ) { ix = x1;  iy += h+pad; }
		mode[i]->reposition_window(x+ix, y+iy);
		if( (ix+=w) > mw ) mw = ix;
		if( (h+=iy) > mh ) mh = h;
		ix += pad;
	}

	ix = xS(25);  iy = mh+pad;
	textbox->reposition_window(x+ix, y+iy);
	ix += textbox->get_w();
	listbox->reposition_window(x+ix, y+iy);
	return 0;
}

int LoadMode::get_h()
{
	int result = 0;
	load_mode_geometry(window, mwindow->theme,
			use_nothing, edl_mode!=0, line_wrap, 0, &result);
	return result;
}

int LoadMode::get_x()
{
	return x;
}

int LoadMode::get_y()
{
	return y;
}

void LoadMode::update()
{
	for( int i=0; i<TOTAL_LOADMODES; ++i ) {
		if( !mode[i] ) continue;
		int v = 0;
		if( *load_mode == i ) v = 1;
		if( edl_mode && *edl_mode == i ) v = 1;
		mode[i]->set_value(v);
	}
	for( int k=0; k<load_modes.total; ++k ) {
		int i = load_modes[k]->value, v = 0;
		if( *load_mode == i ) v = 1;
		if( edl_mode && *edl_mode == i ) v = 1;
		load_modes[k]->set_selected(v);
	}
	textbox->update(mode_to_text(*load_mode));
}

int LoadMode::set_line_wrap(int v)
{
	int ret = line_wrap;
	line_wrap = v;
	return ret;
}

LoadModeListBox::LoadModeListBox(BC_WindowBase *window, LoadMode *loadmode,
		int x, int y)
 : BC_ListBox(x, y, loadmode->mwindow->theme->loadmode_w, yS(150), LISTBOX_TEXT,
	(ArrayList<BC_ListBoxItem *>*)&loadmode->load_modes, 0, 0, 1, 0, 1)
{
	this->window = window;
	this->loadmode = loadmode;
}

LoadModeListBox::~LoadModeListBox()
{
}

int LoadModeListBox::handle_event()
{
	LoadModeItem *item = (LoadModeItem *)get_selection(0, 0);
	if( !item ) return 1;
	int mode = item->value;
	if( mode < LOADMODE_EDL_CLIP )
		*loadmode->load_mode = mode;
	else if( loadmode->edl_mode )
		*loadmode->edl_mode = mode;
	loadmode->update();
	return 1;
}

