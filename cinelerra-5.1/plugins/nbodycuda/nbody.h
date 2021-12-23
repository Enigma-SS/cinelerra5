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
 */

#ifndef NBODYCUDA_H
#define NBODYCUDA_H

#include "pluginvclient.h"

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include <helper_cuda.h>
#include <helper_functions.h>

#include <5_Simulations/nbody/bodysystemcuda.h>
#include <5_Simulations/nbody/render_particles.h>
#include "nbodycuda.h"

class N_BodyConfig;
class N_BodyMain;
class N_BodyCuda;


class N_BodyParams
{
public: 
	float m_timestep, m_clusterScale, m_velocityScale;
	float m_softening, m_damping, m_pointSize;
	float m_x, m_y, m_z;
	static const int num_demos;
};
class N_BodyCamera
{
public:
	float trans[3];
	float rot[3];
	float trans_lag[3];
	float rot_lag[3];
};

class N_BodyConfig : public N_BodyParams, public N_BodyCamera
{
public:
	N_BodyConfig();
	void reset(int i=0);
	int mode;
	float inertia;
	int numBodies;

	int equivalent(N_BodyConfig &that);
	void copy_from(N_BodyConfig &that);
	void interpolate(N_BodyConfig &prev, N_BodyConfig &next, 
		long prev_frame, long next_frame, long current_frame);
	void limits();
};

class N_BodyMain : public PluginVClient
{
public:
	N_BodyMain(PluginServer *server);
	~N_BodyMain();
	PLUGIN_CLASS_MEMBERS2(N_BodyConfig)
	int is_realtime();
	int is_synthesis();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void initData();
	void reset();
	int handle_opengl();

	int color_model, pass;
	VFrame *output;
	N_BodyCuda *cuda;

	void init_cuda();
	void finish_cuda();

	BodySystem<float> *m_nbody;
	ParticleRenderer *m_renderer;

	float *m_hPos, *m_hVel;
	float *m_hColor;

	char deviceName[100];
	enum { M_VIEW = 0, M_MOVE };
	int blockSize;

	int activeDemo;
	int64_t curr_position, new_position;

	void init(int numBodies);
	void reset(int numBodies, NBodyConfig cfg);
	void resetRenderer();
	void selectDemo(int index);
	void updateParams() {
		m_nbody->setSoftening(config.m_softening);
		m_nbody->setDamping(config.m_damping);
	}

	void updateSimulation() {
		m_nbody->update(config.m_timestep);
	}

	void display() { // display particles
		m_renderer->setSpriteSize(config.m_pointSize);
		m_renderer->setPBO(m_nbody->getCurrentReadBuffer(),
			m_nbody->getNumBodies(), (sizeof(float) > 4));
		m_renderer->display((ParticleRenderer::DisplayMode)config.mode);
	}
	void draw();

	void getArrays(float *pos, float *vel) {
		float *_pos = m_nbody->getArray(BODYSYSTEM_POSITION);
		float *_vel = m_nbody->getArray(BODYSYSTEM_VELOCITY);
		memcpy(pos, _pos, m_nbody->getNumBodies() * 4 * sizeof(float));
		memcpy(vel, _vel, m_nbody->getNumBodies() * 4 * sizeof(float));
	}

	void setArrays(const float *pos, const float *vel) {
		int sz = config.numBodies * 4 * sizeof(float);
		if (pos != m_hPos)
			memcpy(m_hPos, pos, sz);
		if (vel != m_hVel)
			memcpy(m_hVel, vel, sz);
		m_nbody->setArray(BODYSYSTEM_POSITION, m_hPos);
		m_nbody->setArray(BODYSYSTEM_VELOCITY, m_hVel);
		resetRenderer();
	}

	void finalize();
};

#endif
