#include "mandelcuda.h"
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "helper_cuda.h"
#include "helper_gl.h"

// The dimensions of the thread block
#define BLOCKDIM_X 16
#define BLOCKDIM_Y 16
#define ABS(n) ((n) < 0 ? -(n) : (n))

void MandelCuda::init_dev()
{
	if( numSMs ) return;
//	int dev_id = findCudaDevice(argc, (const char **)argv);
	int dev_id = gpuGetMaxGflopsDeviceId();
	checkCudaErrors(cudaSetDevice(dev_id));
	cudaDeviceProp deviceProp;
	checkCudaErrors(cudaGetDeviceProperties(&deviceProp, dev_id));
printf("GPU Device %d: \"%s\" with compute capability %d.%d\n",
  dev_id, deviceProp.name, deviceProp.major, deviceProp.minor);
	version = deviceProp.major * 10 + deviceProp.minor;
	numSMs = deviceProp.multiProcessorCount;
	if( !numSMs ) numSMs = -1;
}

void MandelCuda::init(int pbo, int pw, int ph)
{
	if( pbo_id >= 0 ) return;
	pbo_id = pbo;  pbo_w = pw;  pbo_h = ph;
	checkCudaErrors(cudaGraphicsGLRegisterBuffer(&cuda_pbo, pbo_id, cudaGraphicsMapFlagsNone));
	checkCudaErrors(cudaGraphicsMapResources(1, &cuda_pbo, 0));
	size_t pbo_bytes = 0;
	checkCudaErrors(cudaGraphicsResourceGetMappedPointer(&pbo_mem, &pbo_bytes, cuda_pbo));
}

void MandelCuda::finish()
{
	pbo_id = -1;
	pbo_w = pbo_h = 0;
	checkCudaErrors(cudaGraphicsUnmapResources(1, &cuda_pbo));
	pbo_mem = 0;
	cudaGraphicsUnregisterResource(cuda_pbo);  cuda_pbo = 0;
}


MandelCuda::MandelCuda()
{
	version = 0;
	numSMs = 0;
	pbo_id = -1;
	pbo_w = pbo_h = 0;
	cuda_pbo = 0;
	pbo_mem = 0;
}
MandelCuda::~MandelCuda()
{
}

static inline int iDivUp(int a, int b)
{
    int v = a / b;
    return a % b ? v+1 : v;
}

// Determine if two pixel colors are within tolerance
__device__ inline int CheckColors(const uchar4 &color0, const uchar4 &color1)
{
	int x = color1.x - color0.x;
	if( ABS(x) > 10 ) return 1;
	int y = color1.y - color0.y;
        if( ABS(y) > 10 ) return 1;
	int z = color1.z - color0.z;
        if( ABS(z) > 10 ) return 1;
	return 0;
}


// The core MandelCuda calculation function template
template<class T> __device__
inline int CalcCore(const int n, T ix, T iy, T xC, T yC)
{
    T x = ix, y = iy;
    T xx = x * x, yy = y * y;
    int i = n;
    while( --i && (xx + yy < 4.0f) ) {
    	y = x * y +  x * y + yC ;  // 2*x*y + yC
        x = xx - yy + xC ;
        yy = y * y;
        xx = x * x;
    }

    return i;
}

template<class T> __global__
void Calc(uchar4 *dst, const int img_w, const int img_h, const int is_julia,
		const int crunch, const int gridWidth, const int numBlocks,
		const T x_off, const T y_off, const T x_julia, const T y_julia, const T scale,
		const uchar4 colors, const int frame, const int animationFrame)
{
	// loop until all blocks completed
	for( unsigned int bidx=blockIdx.x; bidx<numBlocks; bidx+=gridDim.x ) {
		unsigned int blockX = bidx % gridWidth;
		unsigned int blockY = bidx / gridWidth;
		const int x = blockDim.x * blockX + threadIdx.x;
		const int y = blockDim.y * blockY + threadIdx.y;
		if( x >= img_w || y >= img_h ) continue;
		int pi = img_w*y + x, n = !frame ? 1 : 0;
		uchar4 pixel = dst[pi];
		if( !n && x > 0 )
			n += CheckColors(pixel, dst[pi-1]);
		if( !n && x+1 < img_w )
			n += CheckColors(pixel, dst[pi+1]);
		if( !n && y > 0 )
			n += CheckColors(pixel, dst[pi-img_w]);
		if( !n && y+1 < img_h )
			n += CheckColors(pixel, dst[pi+img_w]);
		if( !n ) continue;

		const T tx = T(x) * scale + x_off;
		const T ty = T(y) * scale + y_off;
		const T ix = is_julia ? tx : 0;
		const T iy = is_julia ? ty : 0;
		const T xC = is_julia ? x_julia : tx;
		const T yC = is_julia ? y_julia : ty;
		int m = CalcCore(crunch, ix,iy, xC,yC);
		m = m > 0 ? crunch - m : 0;
		if( m ) m += animationFrame;

		uchar4 color;
		color.x = m * colors.x;
		color.y = m * colors.y;
		color.z = m * colors.z;
		color.w = 0;

		int frame1 = frame+1, frame2 = frame1/2;
		color.x = (pixel.x * frame + color.x + frame2) / frame1;
		color.y = (pixel.y * frame + color.y + frame2) / frame1;
		color.z = (pixel.z * frame + color.z + frame2) / frame1;
		dst[pi] = color; // Output the pixel
	}
}


void MandelCuda::Run(unsigned char *data, unsigned int size, int is_julia, int crunch,
		double x_off, double y_off, double x_julia, double y_julia, double scale,
		uchar4 colors, int pass, int animationFrame)
{
	if( numSMs < 0 ) return;
	checkCudaErrors(cudaMemcpy(pbo_mem, data, size, cudaMemcpyHostToDevice));
	dim3 threads(BLOCKDIM_X, BLOCKDIM_Y);
	dim3 grid(iDivUp(pbo_w, BLOCKDIM_X), iDivUp(pbo_h, BLOCKDIM_Y));
	Calc<float><<<numSMs, threads>>>((uchar4 *)pbo_mem, pbo_w, pbo_h,
			is_julia, crunch, grid.x, grid.x*grid.y,
			float(x_off), float(y_off), float(x_julia), float(y_julia), float(scale),
			colors, pass, animationFrame);
	checkCudaErrors(cudaMemcpy(data, pbo_mem, size, cudaMemcpyDeviceToHost));
}

