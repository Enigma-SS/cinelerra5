/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "mutex.h"

#include "cwindow.h"
#include "cwindowgui.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "playback3d.h"

#include "nbody.h"
#include "nbodycuda.h"
#include "nbodywindow.h"


static struct N_BodyParams demoParams[] = {
	{ 0.016f, 1.54f, 8.0f,      0.1f, 1.0f, 1.0f,     0, -2, -100},
	{ 0.016f, 0.68f, 20.0f,     0.1f, 1.0f, 0.8f,     0, -2, -30},
	{ 0.0006f, 0.16f, 1000.0f,  1.0f, 1.0f, 0.07f,    0, 0, -1.5f},
	{ 0.0006f, 0.16f, 1000.0f,  1.0f, 1.0f, 0.07f,    0, 0, -1.5f},
	{ 0.0019f, 0.32f, 276.0f,   1.0f, 1.0f, 0.07f,    0, 0, -5},
	{ 0.0016f, 0.32f, 272.0f,   0.145f, 1.0f, 0.08f,  0, 0, -5},
	{ 0.0160f, 6.04f, 0.0f,     1.0f, 1.0f, 0.76f,    0, 0, -50},
};
const int N_BodyParams::num_demos = sizeof(demoParams)/sizeof(*demoParams);


REGISTER_PLUGIN(N_BodyMain)

void N_BodyConfig::reset(int i)
{
	*(N_BodyParams*)this = demoParams[i];
	trans[0] = trans_lag[0] = m_x;
	trans[1] = trans_lag[1] = m_y;
	trans[2] = trans_lag[2] = m_z;
	rot[0] = rot_lag[0] = 0;
	rot[1] = rot_lag[1] = 0;
	rot[2] = rot_lag[2] = 0;
	mode = ParticleRenderer::PARTICLE_SPRITES_COLOR;
	numBodies = 4096;
	inertia = 0.1;
}

N_BodyConfig::N_BodyConfig()
{
	reset();
}

int N_BodyConfig::equivalent(N_BodyConfig &that)
{
	return m_timestep == that.m_timestep &&
		m_clusterScale == that.m_clusterScale &&
		m_velocityScale == that.m_velocityScale &&
		m_softening == that. m_softening &&
		m_damping == that.m_damping &&
		m_pointSize == that.m_pointSize &&
		m_x == that.m_x &&
		m_y == that.m_y &&
		m_z == that.m_z &&
		trans[0] == that.trans[0] &&
		trans[1] == that.trans[1] &&
		trans[2] == that.trans[2] &&
		trans_lag[0] == that.trans_lag[0] &&
		trans_lag[1] == that.trans_lag[1] &&
		trans_lag[2] == that.trans_lag[2] &&
		rot[0] == that.rot[0] &&
		rot[1] == that.rot[1] &&
		rot[2] == that.rot[2] &&
		rot_lag[0] == that.rot_lag[0] &&
		rot_lag[1] == that.rot_lag[1] &&
		rot_lag[2] == that.rot_lag[2] &&
		inertia == that.inertia &&
		numBodies == that.numBodies;
	return 1;
}

void N_BodyConfig::copy_from(N_BodyConfig &that)
{
	m_timestep = that.m_timestep;
	m_clusterScale = that.m_clusterScale;
	m_velocityScale = that.m_velocityScale;
	m_softening = that. m_softening;
	m_damping = that.m_damping;
	m_pointSize = that.m_pointSize;
	m_x = that.m_x;
	m_y = that.m_y;
	m_z = that.m_z;
	trans[0] = that.trans[0];
	trans[1] = that.trans[1];
	trans[2] = that.trans[2];
	trans_lag[0] = that.trans_lag[0];
	trans_lag[1] = that.trans_lag[1];
	trans_lag[2] = that.trans_lag[2];
	rot[0] = that.rot[0];
	rot[1] = that.rot[1];
	rot[2] = that.rot[2];
	rot_lag[0] = that.rot_lag[0];
	rot_lag[1] = that.rot_lag[1];
	rot_lag[2] = that.rot_lag[2];
	inertia = that.inertia;
	numBodies = that.numBodies;
}

void N_BodyConfig::interpolate( N_BodyConfig &prev, N_BodyConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(next);
}

void N_BodyConfig::limits()
{
	if( m_damping < 0.001 ) m_damping = 0.001;
	if( trans[2] < 0.005 ) trans[2] = 0.005;
	int n = 1;
	while( n < numBodies ) n <<= 1;
	bclamp(n, 0x0010, 0x4000);
	numBodies = n;
	bclamp(inertia, 0.f,1.f);
	bclamp(mode, 0, (int)ParticleRenderer::PARTICLE_NUM_MODES-1);
}


N_BodyMain::N_BodyMain(PluginServer *server)
 : PluginVClient(server)
{
	cuda = 0;
	blockSize = 256;

	m_nbody = 0;
	m_renderer = 0;
	m_hPos = 0;
	m_hVel = 0;
	m_hColor = 0;

	curr_position = -1;
	new_position = -1;
}

N_BodyMain::~N_BodyMain()
{
	delete cuda;
}

void N_BodyMain::init(int numBodies)
{
	selectDemo(0);
	delete m_nbody;      m_nbody = new N_BodySystem(numBodies, 1, blockSize);
	int sz = numBodies*4;
	delete [] m_hPos;    m_hPos = new float[sz];
	delete [] m_hVel;    m_hVel = new float[sz];
	delete [] m_hColor;  m_hColor = new float[sz];
	delete m_renderer;   m_renderer = new ParticleRenderer;
// config here
	m_nbody->setSoftening(config.m_softening);
	m_nbody->setDamping(config.m_damping);
	reset(numBodies, NBODY_CONFIG_RANDOM);
	resetRenderer();
}

void N_BodyMain::reset(int numBodies, NBodyConfig cfg)
{
	randomizeBodies(cfg, m_hPos, m_hVel, m_hColor,
		config.m_clusterScale, config.m_velocityScale,
		numBodies, true);
	setArrays(m_hPos, m_hVel);
}

void N_BodyMain::resetRenderer()
{
	float color[4] = { 1.0f, 0.6f, 0.3f, 1.0f};
	m_renderer->setBaseColor(color);
	m_renderer->setColors(m_hColor, m_nbody->getNumBodies());
	m_renderer->setSpriteSize(config.m_pointSize);
}

void N_BodyMain::selectDemo(int index)
{
	config.reset(index);
}

void N_BodyMain::finalize()
{
	delete [] m_hPos;    m_hPos = 0;
	delete [] m_hVel;    m_hVel = 0;
	delete [] m_hColor;  m_hColor = 0;
	delete m_nbody;      m_nbody = 0;
	delete m_renderer;   m_renderer = 0;
}

void N_BodyMain::draw()
{
	cuda->draw_event();
	glClearColor(0.,0.,0.,1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor4f(1.,1.,1.,1.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	float inertia = config.inertia;
	for( int c=0; c<3; ++c ) {
		config.trans_lag[c] += (config.trans[c] - config.trans_lag[c]) * inertia;
		config.rot_lag[c] += (config.rot[c] - config.rot_lag[c]) * inertia;
	}

	glTranslatef(config.trans_lag[0], config.trans_lag[1], config.trans_lag[2]);
	glRotatef(config.rot_lag[0], 1.0, 0.0, 0.0);
	glRotatef(config.rot_lag[1], 0.0, 1.0, 0.0);
	glDisable(GL_TEXTURE_2D);
	display();
}


const char* N_BodyMain::plugin_title() { return N_("N_Body"); }
int N_BodyMain::is_realtime() { return 1; }
int N_BodyMain::is_synthesis() { return 1; }

NEW_WINDOW_MACRO(N_BodyMain, N_BodyWindow);
LOAD_CONFIGURATION_MACRO(N_BodyMain, N_BodyConfig)

void N_BodyMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("NBODYCUDA");
	output.tag.set_property("TIMESTEP", config.m_timestep);
	output.tag.set_property("CLUSTER_SCALE", config.m_clusterScale);
	output.tag.set_property("VELOCITY_SCALE", config.m_velocityScale);
	output.tag.set_property("SOFTENING", config.m_softening);
	output.tag.set_property("DAMPING", config.m_damping);
	output.tag.set_property("POINT_SIZE", config.m_pointSize);
	output.tag.set_property("X", config.m_x);
	output.tag.set_property("Y", config.m_y);
	output.tag.set_property("Z", config.m_z);
	output.tag.set_property("TRANS_X",config.trans[0]);
	output.tag.set_property("TRANS_Y",config.trans[1]);
	output.tag.set_property("TRANS_Z",config.trans[2]);
	output.tag.set_property("TRANS_LAG_X",config.trans_lag[0]);
	output.tag.set_property("TRANS_LAG_Y",config.trans_lag[1]);
	output.tag.set_property("TRANS_LAG_Z",config.trans_lag[2]);
	output.tag.set_property("ROT_X",config.rot[0]);
	output.tag.set_property("ROT_Y",config.rot[1]);
	output.tag.set_property("ROT_Z",config.rot[2]);
	output.tag.set_property("ROT_LAG_X",config.rot_lag[0]);
	output.tag.set_property("ROT_LAG_Y",config.rot_lag[1]);
	output.tag.set_property("ROT_LAG_Z",config.rot_lag[2]);
	output.tag.set_property("INERTIA", config.inertia);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("NUM_BODIES", config.numBodies);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/NBODYCUDA");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void N_BodyMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("NBODYCUDA") ) {
			config.m_timestep = input.tag.get_property("TIMESTEP", config.m_timestep);
			config.m_clusterScale = input.tag.get_property("CLUSTER_SCALE", config.m_clusterScale);
			config.m_velocityScale = input.tag.get_property("VELOCITY_SCALE", config.m_velocityScale);
			config.m_softening = input.tag.get_property("SOFTENING", config.m_softening);
			config.m_damping = input.tag.get_property("DAMPING", config.m_damping);
			config.m_pointSize = input.tag.get_property("POINT_SIZE", config.m_pointSize);
			config.m_x = input.tag.get_property("X", config.m_x);
			config.m_y = input.tag.get_property("Y", config.m_y);
			config.m_z = input.tag.get_property("Z", config.m_z);
			config.trans[0] = input.tag.get_property("TRANS_X", config.trans[0]);
			config.trans[1] = input.tag.get_property("TRANS_Y", config.trans[1]);
			config.trans[2] = input.tag.get_property("TRANS_Z", config.trans[2]);
			config.trans_lag[0] = input.tag.get_property("TRANS_LAG_X", config.trans_lag[0]);
			config.trans_lag[1] = input.tag.get_property("TRANS_LAG_Y", config.trans_lag[1]);
			config.trans_lag[2] = input.tag.get_property("TRANS_LAG_Z", config.trans_lag[2]);
			config.rot[0] = input.tag.get_property("ROT_X", config.rot[0]);
			config.rot[1] = input.tag.get_property("ROT_Y", config.rot[1]);
			config.rot[2] = input.tag.get_property("ROT_Z", config.rot[2]);
			config.rot_lag[0] = input.tag.get_property("ROT_LAG_X", config.rot_lag[0]);
			config.rot_lag[1] = input.tag.get_property("ROT_LAG_Y", config.rot_lag[1]);
			config.rot_lag[2] = input.tag.get_property("ROT_LAG_Z", config.rot_lag[2]);
			config.inertia = input.tag.get_property("INERTIA", config.inertia);
			config.mode = input.tag.get_property("MODE", config.mode);
			config.numBodies = input.tag.get_property("NUM_BODIES", config.numBodies);
		}
	}
	config.limits();
}

void N_BodyMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("N_BodyMain::update_gui");
	N_BodyWindow *window = (N_BodyWindow*)thread->window;
	window->update_gui();
	window->flush();
	window->unlock_window();
}

int N_BodyMain::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{

	//int need_reconfigure =
	load_configuration();
	new_position = start_position;
	output = get_output(0);
	color_model = output->get_color_model();
	if( get_use_opengl() )
		return run_opengl();
// always use_opengl
	Canvas *canvas = server->mwindow->cwindow->gui->canvas;
	return server->mwindow->playback_3d->run_plugin(canvas, this);
}

// cuda

N_BodyCuda::N_BodyCuda()
{
	version = 0;
	numSMs = 0;
}
N_BodyCuda::~N_BodyCuda()
{
}

// opengl from here down

void N_BodyMain::init_cuda()
{
	if( !cuda ) {
		cuda = new N_BodyCuda();
		cuda->init_dev();
	}
	cuda->init();
}
void N_BodyMain::finish_cuda()
{
	cuda->finish();
}

int N_BodyMain::handle_opengl()
{
	output->enable_opengl();
	output->init_screen();
	if( !m_nbody || get_source_position() == 0 ||
	    (int)m_nbody->getNumBodies() != config.numBodies )
		init(config.numBodies);
	init_cuda();
	if( curr_position != new_position ) {
		updateSimulation();
		curr_position = new_position;
	}
	draw();
	finish_cuda();
	output->set_opengl_state(VFrame::SCREEN);
	if( !get_use_opengl() ) // rendering
		output->screen_to_ram();
	return 0;
}

