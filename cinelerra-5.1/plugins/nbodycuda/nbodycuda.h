#ifndef __NBODYCUDA_H__
#define __NBODYCUDA_H__

#include <vector>
template <typename T>
void read_tipsy_file(std::vector<T> &bodyPositions, std::vector<T> &bodyVelocities,
		std::vector<int> &bodiesIDs, const std::string &fileName,
		int &NTotal, int &NFirst, int &NSecond, int &NThird)
{
}

#include <5_Simulations/nbody/bodysystem.h>
#include <5_Simulations/nbody/bodysystemcuda.h>

class N_BodyCuda
{
public:
	N_BodyCuda();
	~N_BodyCuda();
	void init_dev();
	void init();
	void finish();

	int version, numSMs;

	cudaEvent_t startEvent, stopEvent, hostMemSyncEvent;
	void start_event() { cudaEventRecord(startEvent, 0); }
	void stop_event() { cudaEventRecord(stopEvent, 0); }
	void draw_event() { cudaEventRecord(hostMemSyncEvent, 0); }
};

class N_BodySystem : public BodySystemCUDA<float>
{
public:
	N_BodySystem(int n_bodies, int n_devs, int blockSize)
	 : BodySystemCUDA<float>(n_bodies, n_devs, blockSize, 1) {}
	N_BodySystem() {};
	virtual void loadTipsyFile(const std::string &filename) {}
};

#endif
