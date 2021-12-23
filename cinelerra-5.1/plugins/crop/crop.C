
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

/*
 * 2019. Derivative by Translate plugin. This plugin works also with Proxy.
 * It uses Percent values instead of Pixel value coordinates.
*/

#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "crop.h"
#include "cropwin.h"

#include <string.h>




REGISTER_PLUGIN(CropMain)

CropConfig::CropConfig()
{
	reset(RESET_DEFAULT_SETTINGS);
}

void CropConfig::reset(int clear)
{
	switch(clear) {
		case RESET_LEFT :
			crop_l = 0.0;
			break;
		case RESET_TOP :
			crop_t = 0.0;
			break;
		case RESET_RIGHT :
			crop_r = 0.0;
			break;
		case RESET_BOTTOM :
			crop_b = 0.0;
			break;
		case RESET_POSITION_X :
			position_x = 0.0;
			break;
		case RESET_POSITION_Y :
			position_y = 0.0;
			break;
		case RESET_ALL :
		case RESET_DEFAULT_SETTINGS :
		default:
			crop_l = 0.0;
			crop_t = 0.0;
			crop_r = 0.0;
			crop_b = 0.0;

			position_x = 0.0;
			position_y = 0.0;
			break;
	}
}


int CropConfig::equivalent(CropConfig &that)
{
	return EQUIV(crop_l, that.crop_l) &&
		EQUIV(crop_t, that.crop_t) &&
		EQUIV(crop_r, that.crop_r) &&
		EQUIV(crop_b, that.crop_b) &&
		EQUIV(position_x, that.position_x) &&
		EQUIV(position_y, that.position_y);
}

void CropConfig::copy_from(CropConfig &that)
{
	crop_l = that.crop_l;
	crop_t = that.crop_t;
	crop_r = that.crop_r;
	crop_b = that.crop_b;
	position_x = that.position_x;
	position_y = that.position_y;
}

void CropConfig::interpolate(CropConfig &prev,
	CropConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->crop_l = prev.crop_l * prev_scale + next.crop_l * next_scale;
	this->crop_t = prev.crop_t * prev_scale + next.crop_t * next_scale;
	this->crop_r = prev.crop_r * prev_scale + next.crop_r * next_scale;
	this->crop_b = prev.crop_b * prev_scale + next.crop_b * next_scale;
	this->position_x = prev.position_x * prev_scale + next.position_x * next_scale;
	this->position_y = prev.position_y * prev_scale + next.position_y * next_scale;
}








CropMain::CropMain(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
	overlayer = 0;

}

CropMain::~CropMain()
{


	if(temp_frame) delete temp_frame;
	temp_frame = 0;
	if(overlayer) delete overlayer;
	overlayer = 0;
}

const char* CropMain::plugin_title() { return N_("Crop & Position"); }
int CropMain::is_realtime() { return 1; }



LOAD_CONFIGURATION_MACRO(CropMain, CropConfig)

void CropMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

// Store data
	output.tag.set_title("CROP");
	output.tag.set_property("LEFT", config.crop_l);
	output.tag.set_property("TOP", config.crop_t);
	output.tag.set_property("RIGHT", config.crop_r);
	output.tag.set_property("BOTTOM", config.crop_b);
	output.tag.set_property("POSITION_X", config.position_x);
	output.tag.set_property("POSITION_Y", config.position_y);
	output.append_tag();
	output.tag.set_title("/CROP");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void CropMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("CROP"))
			{
 				config.crop_l = input.tag.get_property("LEFT", config.crop_l);
 				config.crop_t = input.tag.get_property("TOP", config.crop_t);
 				config.crop_r = input.tag.get_property("RIGHT", config.crop_r);
 				config.crop_b = input.tag.get_property("BOTTOM", config.crop_b);
 				config.position_x = input.tag.get_property("POSITION_X", config.position_x);
 				config.position_y = input.tag.get_property("POSITION_Y", config.position_y);
			}
		}
	}
}


#define EPSILON 0.001

int CropMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	VFrame *input = input_ptr;
	VFrame *output = output_ptr;

	load_configuration();

//printf("CropMain::process_realtime 1 %p\n", input);
	if( input->get_rows()[0] == output->get_rows()[0] ) {
		if( temp_frame && (
		    temp_frame->get_w() != input_ptr->get_w() ||
		    temp_frame->get_h() != input_ptr->get_h() ||
		    temp_frame->get_color_model() != input_ptr->get_color_model() ) ) {
			delete temp_frame;
			temp_frame = 0;
		}
		if(!temp_frame)
			temp_frame = new VFrame(input_ptr->get_w(), input_ptr->get_h(),
				input->get_color_model(), 0);
		temp_frame->copy_from(input);
		input = temp_frame;
	}
//printf("CropMain::process_realtime 2 %p\n", input);


	if(!overlayer)
	{
		overlayer = new OverlayFrame(smp + 1);
	}

	output->clear_frame();

/* OLD code by "Translate" plugin
	if( config.in_w < EPSILON ) return 1;
	if( config.in_h < EPSILON ) return 1;
	if( config.out_w < EPSILON ) return 1;
	if( config.out_h < EPSILON ) return 1;
*/


/*
   *** Little description of the points ***
   Points (ix1, iy1) and (ix2, iy2) are the Camera coordinates:
     (ix1, iy1) is the point on top-left,
     (ix2, iy2) is the point on bottom-right.
   Points (ox1, oy1) and (ox2, oy2) are the Projector coordinates:
     (ox1, oy1) is the point on top-left,
     (ox2, oy2) is the point on bottom-right.
*/

// Convert from Percent to Pixel coordinate (Percent value in plugin GUI)
// so it works also for Proxy
	float ix1 = config.crop_l / 100 * output->get_w();
	float ox1 = ix1;
	ox1 += config.position_x / 100 * output->get_w();
	float ix2 = ((100.00 - config.crop_r) / 100) * output->get_w();

	if( ix1 < 0 ) {
		ox1 -= ix1;
		ix1 = 0;
	}

	if(ix2 > output->get_w())
		ix2 = output->get_w();

// Convert from Percent to Pixel coordinate (Percent value in plugin GUI)
// so it works also for Proxy
	float iy1 = config.crop_t / 100 * output->get_h();
	float oy1 = iy1;
	oy1 += config.position_y / 100 * output->get_h();
	float iy2 = ((100.00 - config.crop_b) / 100) * output->get_h();

	if( iy1 < 0 ) {
		oy1 -= iy1;
		iy1 = 0;
	}

	if( iy2 > output->get_h() )
		iy2 = output->get_h();

// Scale features: OLD code by "Translate" plugin.
// (I leave here for future development)  
// Scale_: scale_x=scale_y=1. It means NO SCALE.
	float cx = 1.00;
	float cy = cx;

	float ox2 = ox1 + (ix2 - ix1) * cx;
	float oy2 = oy1 + (iy2 - iy1) * cy;

	if( ox1 < 0 ) {
		ix1 += -ox1 / cx;
		ox1 = 0;
	}
	if( oy1 < 0 ) {
		iy1 += -oy1 / cy;
		oy1 = 0;
	}
	if( ox2 > output->get_w() ) {
		ix2 -= (ox2 - output->get_w()) / cx;
		ox2 = output->get_w();
	}
	if( oy2 > output->get_h() ) {
		iy2 -= (oy2 - output->get_h()) / cy;
		oy2 = output->get_h();
	}

	if( ix1 >= ix2 ) return 1;
	if( iy1 >= iy2 ) return 1;
	if( ox1 >= ox2 ) return 1;
	if( oy1 >= oy2 ) return 1;

	overlayer->overlay(output, input,
		ix1, iy1, ix2, iy2,
		ox1, oy1, ox2, oy2,
		1, TRANSFER_REPLACE,
		get_interpolation_type());
	return 0;
}

NEW_WINDOW_MACRO(CropMain, CropWin)

void CropMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;

	CropWin *window = (CropWin*)thread->window;
	window->lock_window();
/*
	window->crop_l->update(config.crop_l);
	window->crop_t->update(config.crop_t);
	window->crop_r->update(config.crop_r);
	window->crop_b->update(config.crop_b);
	window->position_x->update(config.position_x);
	window->position_y->update(config.position_y);
*/
	window->update(RESET_ALL);

	window->unlock_window();
}
