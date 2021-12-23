#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "helper_cuda.h"
#include "helper_gl.h"

#include "nbodycuda.h"

void N_BodyCuda::init()
{
	checkCudaErrors(cudaEventCreate(&startEvent));
	checkCudaErrors(cudaEventCreate(&stopEvent));
	checkCudaErrors(cudaEventCreate(&hostMemSyncEvent));
}

void N_BodyCuda::init_dev()
{
//	int dev_id = findCudaDevice(argc, (const char **)argv);
	int dev_id = gpuGetMaxGflopsDeviceId();
	checkCudaErrors(cudaSetDevice(dev_id));
	cudaDeviceProp deviceProp;
	checkCudaErrors(cudaGetDeviceProperties(&deviceProp, dev_id));
printf("GPU Device %d: \"%s\" with compute capability %d.%d\n",
  dev_id, deviceProp.name, deviceProp.major, deviceProp.minor);
	version = deviceProp.major * 10 + deviceProp.minor;
	numSMs = deviceProp.multiProcessorCount;
}


void N_BodyCuda::finish()
{
	checkCudaErrors(cudaEventDestroy(startEvent));
	checkCudaErrors(cudaEventDestroy(stopEvent));
	checkCudaErrors(cudaEventDestroy(hostMemSyncEvent));
}

