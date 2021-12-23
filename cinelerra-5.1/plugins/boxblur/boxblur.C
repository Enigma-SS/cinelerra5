
/*
 * CINELERRA
 * Copyright (C) 1997-2020 Adam Williams <broadcast at earthling dot net>
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

#include "guicast.h"
#include "boxblur.h"
#include "dragcheckbox.h"
#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"
#include "plugin.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "track.h"
#include "tracks.h"
#include "theme.h"

#include <stdint.h>
#include <string.h>

class BoxBlurConfig;
class BoxBlurNumISlider;
class BoxBlurNumIText;
class BoxBlurNumClear;
class BoxBlurNum;
class BoxBlurRadius;
class BoxBlurPower;
class BoxBlurDrag;
class BoxBlurReset;
class BoxBlurX;
class BoxBlurY;
class BoxBlurW;
class BoxBlurH;
class BoxBlurWindow;
class BoxBlurEffect;


class BoxBlurConfig
{
public:
	BoxBlurConfig();
	void copy_from(BoxBlurConfig &that);
	int equivalent(BoxBlurConfig &that);
	void interpolate(BoxBlurConfig &prev, BoxBlurConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	void reset();
	void preset();

	int horz_radius, vert_radius, power;
	float box_x, box_y;
	int box_w, box_h;
	int drag;
};


class BoxBlurNumISlider : public BC_ISlider
{
public:
	BoxBlurNumISlider(BoxBlurNum *num, int x, int y);
	int handle_event();
	BoxBlurNum *num;
};

class BoxBlurNumIText : public BC_TumbleTextBox
{
public:
	BoxBlurNumIText(BoxBlurNum *num, int x, int y);
	int handle_event();
	BoxBlurNum *num;
};

class BoxBlurNumClear : public BC_Button
{
public:
	BoxBlurNumClear(BoxBlurNum *num, int x, int y);
	static int calculate_w(BoxBlurNum *num);
	int handle_event();

	BoxBlurNum *num;
};

class BoxBlurNum
{
public:
	BoxBlurNum(BoxBlurWindow *gui, int x, int y, int w,
		const char *name, int *iv, int imn, int imx);
	~BoxBlurNum();
	void create_objects();
	void update(int value);
	int get_w();
	int get_h();

	BoxBlurWindow *gui;
	int x, y, w, h;
	const char *name;
	int imn, imx, *ivalue;
	int title_w, text_w, slider_w;
	BC_Title *title;
	BoxBlurNumIText *text;
	BoxBlurNumISlider *slider;
	BoxBlurNumClear *clear;
};


class BoxBlurRadius : public BoxBlurNum
{
public:
	BoxBlurRadius(BoxBlurWindow *gui, int x, int y, int w,
			const char *name, int *radius);
};

class BoxBlurPower : public BoxBlurNum
{
public:
	BoxBlurPower(BoxBlurWindow *gui, int x, int y, int w,
			const char *name, int *power);
};

class BoxBlurX : public BC_TumbleTextBox
{
public:
	BoxBlurX(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	BoxBlurWindow *gui;
};
class BoxBlurY : public BC_TumbleTextBox
{
public:
	BoxBlurY(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	BoxBlurWindow *gui;
};
class BoxBlurW : public BC_TumbleTextBox
{
public:
	BoxBlurW(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	BoxBlurWindow *gui;
};
class BoxBlurH : public BC_TumbleTextBox
{
public:
	BoxBlurH(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	BoxBlurWindow *gui;
};

class BoxBlurDrag : public DragCheckBox
{
public:
	BoxBlurDrag(BoxBlurWindow *gui, BoxBlurEffect *plugin, int x, int y);
	int handle_event();
	void update_gui();
	Track *get_drag_track();
	int64_t get_drag_position();
	static int calculate_w(BoxBlurWindow *gui);

	BoxBlurWindow *gui;
	BoxBlurEffect *plugin;
};

class BoxBlurReset : public BC_GenericButton
{
public:
	BoxBlurReset(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	static int calculate_w(BoxBlurWindow *gui);

	BoxBlurWindow *gui;
};

class BoxBlurPreset : public BC_GenericButton
{
public:
	BoxBlurPreset(BoxBlurWindow *gui, int x, int y);
	int handle_event();
	static int calculate_w(BoxBlurWindow *gui);

	BoxBlurWindow *gui;
};

class BoxBlurWindow : public PluginClientWindow
{
public:
	BoxBlurWindow(BoxBlurEffect *plugin);
	~BoxBlurWindow();
	void create_objects();
	void update_gui();
	void update_drag();

	BoxBlurEffect *plugin;
	BoxBlurReset *reset;
	BoxBlurPreset *preset;
	BoxBlurRadius *blur_horz;
	BoxBlurRadius *blur_vert;
	BoxBlurPower *blur_power;
	BoxBlurDrag *drag;
	BoxBlurX *box_x;
	BoxBlurY *box_y;
	BoxBlurW *box_w;
	BoxBlurH *box_h;
};


class BoxBlurEffect : public PluginVClient
{
public:
	BoxBlurEffect(PluginServer *server);
	~BoxBlurEffect();

	PLUGIN_CLASS_MEMBERS(BoxBlurConfig)
	void render_gui(void *data);
	int is_dragging();
	int process_realtime(VFrame *input, VFrame *output);
	void update_gui();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void draw_boundry();

	VFrame *input, *output;
	BoxBlur *box_blur;
	int drag;
};


void BoxBlurConfig::reset()
{
	horz_radius = 0;
	vert_radius = 0;
	power = 1;
	box_x = box_y = 0.0;
	box_w = box_h = 0;
}

void BoxBlurConfig::preset()
{
	horz_radius = 2;
	vert_radius = 2;
	power = 2;
	box_x = box_y = 0.0;
	box_w = box_h = 0;
}


BoxBlurConfig::BoxBlurConfig()
{
	preset();
}

void BoxBlurConfig::copy_from(BoxBlurConfig &that)
{
	horz_radius = that.horz_radius;
	vert_radius = that.vert_radius;
	power = that.power;
	box_x = that.box_x;  box_y = that.box_y;
	box_w = that.box_w;  box_h = that.box_h;
}

int BoxBlurConfig::equivalent(BoxBlurConfig &that)
{
	return horz_radius == that.horz_radius &&
		vert_radius == that.vert_radius &&
		power == that.power &&
		EQUIV(box_x, that.box_x) && EQUIV(box_y, that.box_y) &&
		box_w == that.box_w && box_h == that.box_h;
}

void BoxBlurConfig::interpolate(BoxBlurConfig &prev, BoxBlurConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double u = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	double v = 1. - u;
	this->horz_radius = u*prev.horz_radius + v*next.horz_radius;
	this->vert_radius = u*prev.vert_radius + v*next.vert_radius;
	this->power = u*prev.power + v*next.power + 1e-6; // avoid truncation jitter
	this->box_x = u*prev.box_x + v*next.box_x;
	this->box_y = u*prev.box_y + v*next.box_y;
	this->box_w = u*prev.box_w + v*next.box_w + 1e-6;
	this->box_h = u*prev.box_h + v*next.box_h + 1e-6;
}


int BoxBlurNum::get_w() { return w; }
int BoxBlurNum::get_h() { return h; }

BoxBlurNumISlider::BoxBlurNumISlider(BoxBlurNum *num, int x, int y)
 : BC_ISlider(x, y, 0, num->slider_w, num->slider_w,
		num->imn, num->imx, *num->ivalue)
{
	this->num = num;
}

int BoxBlurNumISlider::handle_event()
{
	int iv = get_value();
	num->update(iv);
	num->gui->update_drag();
	return 1;
}

BoxBlurNumIText::BoxBlurNumIText(BoxBlurNum *num, int x, int y)
 : BC_TumbleTextBox(num->gui, *num->ivalue, num->imn, num->imx,
			x, y, num->text_w)
{
	this->num = num;
}

int BoxBlurNumIText::handle_event()
{
	int iv = atoi(get_text());
	num->update(iv);
	num->gui->update_drag();
	return 1;
}

BoxBlurNumClear::BoxBlurNumClear(BoxBlurNum *num, int x, int y)
 : BC_Button(x, y, num->gui->plugin->get_theme()->get_image_set("reset_button"))
{
	this->num = num;
}

int BoxBlurNumClear::calculate_w(BoxBlurNum *num)
{
	VFrame **imgs = num->gui->plugin->get_theme()->get_image_set("reset_button");
	return imgs[0]->get_w();
}

int BoxBlurNumClear::handle_event()
{
	int v = num->imn;
	num->update(v);
	num->gui->update_drag();
	return 1;
}

BoxBlurNum::BoxBlurNum(BoxBlurWindow *gui, int x, int y, int w,
		 const char *name, int *iv, int imn, int imx)
{
	this->gui = gui;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = 0;
	this->name = name;
	this->ivalue = iv;
	this->imn = imn;
	this->imx = imx;
	int margin = gui->plugin->get_theme()->widget_border;
	int clear_w = BoxBlurNumClear::calculate_w(this);
	int tumble_w = BC_Tumbler::calculate_w();
	int len = w - 2*margin - clear_w - tumble_w;
	this->title_w = xS(60);
	this->text_w = xS(60) - tumble_w;
	this->slider_w = len - title_w - text_w - 2*margin;

	title = 0;
	text = 0;
	slider = 0;
	clear = 0;
}

BoxBlurNum::~BoxBlurNum()
{
	delete text;
}

void BoxBlurNum::create_objects()
{
	int x1 = this->x;
	gui->add_subwindow(title = new BC_Title(x1, y, name));
	int margin = gui->plugin->get_theme()->widget_border;
	x1 += title_w + margin;
	text = new BoxBlurNumIText(this, x1, y);
	text->create_objects();
	x1 += text_w + BC_Tumbler::calculate_w() + margin;
	gui->add_subwindow(slider = new BoxBlurNumISlider(this, x1, y));
	x1 += slider_w + 2*margin;
	gui->add_subwindow(clear = new BoxBlurNumClear(this, x1, y));
	h = bmax(title->get_h(), bmax(text->get_h(),
		bmax(slider->get_h(), clear->get_h())));
}

void BoxBlurNum::update(int value)
{
	text->update((int64_t)value);
	slider->update(value);
	*ivalue = value;
}


BoxBlurRadius::BoxBlurRadius(BoxBlurWindow *gui, int x, int y, int w,
		const char *name, int *radius)
 : BoxBlurNum(gui, x, y, w, name, radius, 0, 100)
{
}

BoxBlurPower::BoxBlurPower(BoxBlurWindow *gui, int x, int y, int w,
		const char *name, int *power)
 : BoxBlurNum(gui, x, y, w, name, power, 1, 10)
{
}

BoxBlurWindow::BoxBlurWindow(BoxBlurEffect *plugin)
 : PluginClientWindow(plugin, xS(360), yS(260), xS(360), yS(260), 0)
{
	this->plugin = plugin;
	blur_horz = 0;
	blur_vert = 0;
	blur_power = 0;
	box_x = 0;  box_y = 0;
	box_w = 0;  box_h = 0;
}

BoxBlurWindow::~BoxBlurWindow()
{
	delete blur_horz;
	delete blur_vert;
	delete blur_power;
	delete box_x;
	delete box_y;
	delete box_w;
	delete box_h;
}

void BoxBlurWindow::create_objects()
{
	int x = xS(10), y = yS(10);
	int ys10 = yS(10), ys20 = yS(20), ys30 = yS(30), ys40 = yS(40);
	int t1 = x, t2 = t1+xS(24), t3 = t2+xS(100), t4 = t3+xS(24);
	int ww = get_w() - 2*x, bar_o = xS(30), bar_m = xS(15);
	int margin = plugin->get_theme()->widget_border;

	BC_TitleBar *tbar;
	add_subwindow(tbar = new BC_TitleBar(x, y, ww, bar_o, bar_m, _("Position & Size")));
	y += ys20;
	int x1 = ww - BoxBlurDrag::calculate_w(this) - margin;
	if( plugin->drag && drag->drag_activate() ) {
		eprintf("drag enabled, but compositor already grabbed\n");
		plugin->drag = 0;
	}
	add_subwindow(drag = new BoxBlurDrag(this, plugin, x1, y));
	drag->create_objects();

	BC_Title *title;
	add_subwindow(title = new BC_Title(t1, y, _("X:")));
	box_x = new BoxBlurX(this, t2, y);
	box_x->create_objects();
	add_subwindow(title = new BC_Title(t3, y, _("W:")));
	box_w = new BoxBlurW(this, t4, y);
	box_w->create_objects();
	y += ys30;
	add_subwindow(title = new BC_Title(t1, y, _("Y:")));
	box_y = new BoxBlurY(this, t2, y);
	box_y->create_objects();
	add_subwindow(title = new BC_Title(t3, y, _("H:")));
	box_h = new BoxBlurH(this, t4, y);
	box_h->create_objects();
	y += ys40;

	add_subwindow(tbar = new BC_TitleBar(x, y, ww, bar_o, bar_m, _("Blur")));
	y += ys20;
	blur_horz = new BoxBlurRadius(this, x, y, ww, _("Horz:"),
			&plugin->config.horz_radius);
	blur_horz->create_objects();
	y += ys30;
	blur_vert = new BoxBlurRadius(this, x, y, ww, _("Vert:"),
			&plugin->config.vert_radius);
	blur_vert->create_objects();
	y += ys30;
	blur_power = new BoxBlurPower(this, x, y, ww, _("Power:"),
			&plugin->config.power);
	blur_power->create_objects();
	y += ys40;
	BC_Bar *bar;
	add_subwindow(bar = new BC_Bar(x, y, ww));
	y += ys10;

	add_subwindow(reset = new BoxBlurReset(this, x, y));
	x1 = x + ww - BoxBlurPreset::calculate_w(this);
	add_subwindow(preset = new BoxBlurPreset(this, x1, y));
	y += bmax(title->get_h(), reset->get_h()) + 2*margin;
	show_window(1);
}

void BoxBlurWindow::update_gui()
{
	BoxBlurConfig &config = plugin->config;
	blur_horz->update(config.horz_radius);
	blur_vert->update(config.vert_radius);
	blur_power->update(config.power);
	box_x->update(config.box_x);
	box_y->update(config.box_y);
	box_w->update((int64_t)config.box_w);
	box_h->update((int64_t)config.box_h);
	drag->drag_x = config.box_x;
	drag->drag_y = config.box_y;
	drag->drag_w = config.box_w;
	drag->drag_h = config.box_h;
}


REGISTER_PLUGIN(BoxBlurEffect)
NEW_WINDOW_MACRO(BoxBlurEffect, BoxBlurWindow)
LOAD_CONFIGURATION_MACRO(BoxBlurEffect, BoxBlurConfig)

void BoxBlurEffect::render_gui(void *data)
{
	BoxBlurEffect *box_blur = (BoxBlurEffect *)data;
	box_blur->drag = drag;
}

int BoxBlurEffect::is_dragging()
{
	drag = 0;
	send_render_gui(this);
	return drag;
}


BoxBlurEffect::BoxBlurEffect(PluginServer *server)
 : PluginVClient(server)
{
	box_blur = 0;
	drag = 0;
}

BoxBlurEffect::~BoxBlurEffect()
{
	delete box_blur;
}

const char* BoxBlurEffect::plugin_title() { return N_("BoxBlur"); }
int BoxBlurEffect::is_realtime() { return 1; }


void BoxBlurEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("BOXBLUR");
	output.tag.set_property("HORZ_RADIUS", config.horz_radius);
	output.tag.set_property("VERT_RADIUS", config.vert_radius);
	output.tag.set_property("POWER", config.power);
	output.tag.set_property("BOX_X", config.box_x);
	output.tag.set_property("BOX_Y", config.box_y);
	output.tag.set_property("BOX_W", config.box_w);
	output.tag.set_property("BOX_H", config.box_h);
	output.append_tag();
	output.tag.set_title("/BOXBLUR");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void BoxBlurEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("BOXBLUR") ) {
			config.horz_radius = input.tag.get_property("HORZ_RADIUS", config.horz_radius);
			config.vert_radius = input.tag.get_property("VERT_RADIUS", config.vert_radius);
			config.power = input.tag.get_property("POWER", config.power);
			config.box_x = input.tag.get_property("BOX_X", config.box_x);
			config.box_y = input.tag.get_property("BOX_Y", config.box_y);
			config.box_w = input.tag.get_property("BOX_W", config.box_w);
			config.box_h = input.tag.get_property("BOX_H", config.box_h);
		}
	}
}

void BoxBlurEffect::draw_boundry()
{
	if( !gui_open() ) return;
	int box_x = config.box_x, box_y = config.box_y;
	int box_w = config.box_w ? config.box_w : input->get_w();
	int box_h = config.box_h ? config.box_h : input->get_h();
	DragCheckBox::draw_boundary(output, box_x, box_y, box_w, box_h);
}

int BoxBlurEffect::process_realtime(VFrame *input, VFrame *output)
{
	this->input = input;
	this->output = output;
	load_configuration();
	int out_w = output->get_w(), out_h = output->get_h();

	if( !box_blur ) {
		int cpus = (out_w * out_h)/0x80000 + 1;
		box_blur = new BoxBlur(cpus);
	}
	int x = config.box_x, y = config.box_y;
	int ow = config.box_w ? config.box_w : out_w;
	int oh = config.box_h ? config.box_h : out_h;
	if( config.horz_radius ) {
		box_blur->hblur(output, input, config.horz_radius, config.power,
			-1, x,y, ow, oh);
		input = output;
	}
	if( config.vert_radius ) {
		box_blur->vblur(output, input, config.vert_radius, config.power,
			-1, x,y, ow, oh);
	}

	if( is_dragging() )
		draw_boundry();

	return 1;
}

void BoxBlurEffect::update_gui()
{
	if( !thread ) return;
	load_configuration();
	thread->window->lock_window("BoxBlurEffect::update_gui");
	BoxBlurWindow *gui = (BoxBlurWindow *)thread->window;
	gui->update_gui();
	thread->window->unlock_window();
}


BoxBlurX::BoxBlurX(BoxBlurWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, gui->plugin->config.box_x,
		-32767.f, 32767.f, x, y, xS(64))
{
	this->gui = gui;
	set_precision(1);
}
int BoxBlurX::handle_event()
{
	gui->plugin->config.box_x = atof(get_text());
	gui->update_drag();
	return 1;
}

BoxBlurY::BoxBlurY(BoxBlurWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, gui->plugin->config.box_y,
		-32767.f, 32767.f, x, y, xS(64))
{
	this->gui = gui;
	set_precision(1);
}
int BoxBlurY::handle_event()
{
	gui->plugin->config.box_y = atof(get_text());
	gui->update_drag();
	return 1;
}

BoxBlurW::BoxBlurW(BoxBlurWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, gui->plugin->config.box_w,
		0, 32767, x, y, xS(64))
{
	this->gui = gui;
}
int BoxBlurW::handle_event()
{
	gui->plugin->config.box_w = atol(get_text());
	gui->update_drag();
	return 1;
}

BoxBlurH::BoxBlurH(BoxBlurWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui, gui->plugin->config.box_h,
		0, 32767, x, y, xS(64))
{
	this->gui = gui;
}
int BoxBlurH::handle_event()
{
	gui->plugin->config.box_h = atol(get_text());
	gui->update_drag();
	return 1;
}

BoxBlurDrag::BoxBlurDrag(BoxBlurWindow *gui, BoxBlurEffect *plugin, int x, int y)
 : DragCheckBox(plugin->server->mwindow, x, y, _("Drag"), &plugin->drag,
		plugin->config.box_x, plugin->config.box_y,
		plugin->config.box_w, plugin->config.box_h)
{
	this->plugin = plugin;
	this->gui = gui;
}

int BoxBlurDrag::calculate_w(BoxBlurWindow *gui)
{
	int w, h;
	calculate_extents(gui, &w, &h, _("Drag"));
	return w;
}

Track *BoxBlurDrag::get_drag_track()
{
	PluginServer *server = plugin->server;
	int plugin_id = server->plugin_id;
	Plugin *plugin = server->edl->tracks->plugin_exists(plugin_id);
	return !plugin ? 0 : plugin->track;
}
int64_t BoxBlurDrag::get_drag_position()
{
	return plugin->get_source_position();
}

void BoxBlurDrag::update_gui()
{
	plugin->drag = get_value();
	plugin->config.box_x = drag_x;
	plugin->config.box_y = drag_y;
	plugin->config.box_w = drag_w+0.5;
	plugin->config.box_h = drag_h+0.5;
	gui->box_x->update((float)plugin->config.box_x);
	gui->box_y->update((float)plugin->config.box_y);
	gui->box_w->update((int64_t)plugin->config.box_w);
	gui->box_h->update((int64_t)plugin->config.box_h);
	plugin->send_configure_change();
}

int BoxBlurDrag::handle_event()
{
	int ret = DragCheckBox::handle_event();
	plugin->drag = get_value();
	plugin->send_configure_change();
	return ret;
}

void BoxBlurWindow::update_drag()
{
	drag->drag_x = plugin->config.box_x;
	drag->drag_y = plugin->config.box_y;
	drag->drag_w = plugin->config.box_w;
	drag->drag_h = plugin->config.box_h;
	plugin->send_configure_change();
}

BoxBlurReset::BoxBlurReset(BoxBlurWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}

int BoxBlurReset::calculate_w(BoxBlurWindow *gui)
{
	return BC_GenericButton::calculate_w(gui,_("Reset"));
}

int BoxBlurReset::handle_event()
{
	BoxBlurEffect *plugin = gui->plugin;
	plugin->config.reset();
	gui->drag->update(0);
	gui->drag->drag_deactivate();
	gui->update_gui();
	gui->update_drag();
	return 1;
}

BoxBlurPreset::BoxBlurPreset(BoxBlurWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Default"))
{
	this->gui = gui;
}

int BoxBlurPreset::calculate_w(BoxBlurWindow *gui)
{
	return BC_GenericButton::calculate_w(gui,_("Default"));
}

int BoxBlurPreset::handle_event()
{
	BoxBlurEffect *plugin = gui->plugin;
	plugin->config.preset();
	gui->drag->update(0);
	gui->drag->drag_deactivate();
	gui->update_gui();
	gui->update_drag();
	return 1;
}

