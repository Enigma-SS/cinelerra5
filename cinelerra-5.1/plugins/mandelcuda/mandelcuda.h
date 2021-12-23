#ifndef __MANDELCUDA_CUH__
#define __MANDELCUDA_CUH__

class MandelCuda
{
public:
	MandelCuda();
	~MandelCuda();

	void init_dev();
	void Run(unsigned char *data, unsigned int size, int is_julia, int crunch,
		double x, double y, double jx, double jy, double scale,
		uchar4 color, int pass, int animationFrame);
	void init(int pbo, int pw, int ph);
	void finish();

	int version, numSMs;
	int pbo_id, pbo_w, pbo_h;
	struct cudaGraphicsResource *cuda_pbo;
	void *pbo_mem;
};

#endif
