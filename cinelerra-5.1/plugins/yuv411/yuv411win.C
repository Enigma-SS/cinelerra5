#include "bcdisplayinfo.h"
#include "edl.h"
#include "edlsession.h"
#include "yuv411win.h"
#include "language.h"

yuv411Window::yuv411Window(yuv411Main *client)
 : PluginClientWindow(client, xS(260), yS(250), xS(260), yS(250), 0)
{
	this->client = client;
	colormodel = -1;
}

yuv411Window::~yuv411Window()
{
}

void yuv411Window::create_objects()
{
	int xs10 = xS(10), xs90 = xS(90);
	int ys10 = yS(10), ys30 = yS(30);
	int x = xs10, y = ys10, x1=xs90;
	add_tool(avg_vertical = new yuv411Toggle(client,
		&(client->config.avg_vertical),
		_("Vertical average"),
		x,
		y));
	y += ys30;
	add_tool(int_horizontal = new yuv411Toggle(client,
		&(client->config.int_horizontal),
		_("Horizontal interpolate"),
		x,
		y));
	y += ys30;
	add_tool(inpainting = new yuv411Toggle(client,
		&(client->config.inpainting),
		_("Inpainting method"),
		x,
		y));
	y += ys30;
	add_subwindow(new BC_Title(x, y, _("Offset:")));
	add_subwindow(offset=new yuv411Offset(client,x1,y));
	y += ys30;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	add_subwindow(thresh=new yuv411Thresh(client,x1,y));
	y += ys30;
	add_subwindow(new BC_Title(x, y, _("Bias:")));
	add_subwindow(bias=new yuv411Bias(client,x1,y));
	y += ys30;
	int y1 = get_h() - yuv411Reset::calculate_h() - yS(20);
	add_subwindow(reset = new yuv411Reset(client, this, x, y1+ys10));
	show_window();
	flush();

	yuv_warning = new BC_Title(x, y, _("Warning: colormodel not YUV"),MEDIUMFONT,RED);
	add_subwindow(yuv_warning);
	EDL *edl = client->get_edl();
	colormodel = edl->session->color_model;
	switch( colormodel ) {
	case BC_YUV888:
	case BC_YUVA8888:
		yuv_warning->hide_window();
		break;
	}
}

void yuv411Window::update()
{
	avg_vertical->update(client->config.avg_vertical);
	int_horizontal->update(client->config.int_horizontal);
	inpainting->update(client->config.inpainting);
	offset->update(client->config.offset);
	thresh->update(client->config.thresh);
	bias->update(client->config.bias);
}

int yuv411Window::close_event()
{
	set_done(1);
	return 1;
}

yuv411Toggle::yuv411Toggle(yuv411Main *client, int *output, char *string, int x, int y)
 : BC_CheckBox(x, y, *output, string)
{
	this->client = client;
	this->output = output;
}
yuv411Toggle::~yuv411Toggle()
{
}
int yuv411Toggle::handle_event()
{
	*output = get_value();
	((yuv411Window*)client->thread->window)->update_enables();
	client->send_configure_change();
	return 1;
}

yuv411Offset::yuv411Offset(yuv411Main *client, int x, int y)
 :  BC_FSlider(x+xS(60), y, 0, xS(100), yS(100), (float)0, (float)2,
            (float)client->config.offset)
{
	this->client = client;
	set_precision(1.0);
}
int yuv411Offset::handle_event()
{
	client->config.offset = get_value();
	client->send_configure_change();
	return 1;
}

yuv411Thresh::yuv411Thresh(yuv411Main *client, int x, int y)
 :  BC_FSlider(x+xS(60), y, 0, xS(100), yS(100), (float)1, (float)100,
            (float)client->config.thresh)
{
	this->client = client;
	set_precision(1.0);
}
int yuv411Thresh::handle_event()
{
	client->config.thresh = get_value();
	client->send_configure_change();
	return 1;
}

yuv411Bias::yuv411Bias(yuv411Main *client, int x, int y)
 :  BC_FSlider(x+xS(60), y, 0, xS(100), yS(100), (float)0, (float)25,
            (float)client->config.bias)
{
	this->client = client;
	set_precision(1.0);
}

int yuv411Bias::handle_event()
{
	client->config.bias = get_value();
	client->send_configure_change();
	return 1;
}

yuv411Reset::yuv411Reset(yuv411Main *client, yuv411Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->client = client;
	this->window = window;
}
yuv411Reset::~yuv411Reset()
{
}
int yuv411Reset::handle_event()
{
	client->config.reset();
	window->update();
	window->update_enables();
	client->send_configure_change();
	return 1;
}


void yuv411Window::update_enables()
{
	yuv411Config &config = client->config;
	if( !config.int_horizontal && config.inpainting ) {
		config.inpainting = 0;
		inpainting->set_value(0);
	}
	if( config.int_horizontal ) {
		inpainting->enable();
		offset->show_window();
	}
	else {
		inpainting->disable();
		offset->hide_window();
	}
	if( config.int_horizontal && config.inpainting ) {
		thresh->show_window();
		bias->show_window();
	}
	else {
		thresh->hide_window();
		bias->hide_window();
	}
}

void yuv411Window::show_warning(int warn)
{
	if( warn )
		yuv_warning->show_window();
	else
		yuv_warning->hide_window();
}

