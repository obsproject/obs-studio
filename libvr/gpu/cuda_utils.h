#pragma once

#include <stdint.h>

namespace libvr {

// Opaque handles to avoid including cuda.h widely
typedef struct CUexternalMemory_st *CUexternalMemory;
typedef struct CUarray_st *CUarray;
typedef struct CUstream_st *CUstream;

bool InitCudaDriver();

// Imports an Opaque FD (from Vulkan) into a CUDA External Memory Handle
// size: size in bytes
bool ImportVulkanMemoryToCuda(int fd, uint64_t size, CUexternalMemory* out_ext_mem);

// Maps the external memory to a CUDA Array (for texture sampling)
// This is required for NvCVImage to bind to it
bool MapCudaArrayFromExternalMemory(CUexternalMemory ext_mem, uint32_t width, uint32_t height, uint32_t format, CUarray* out_array);

void FreeCudaResources(CUexternalMemory ext_mem, CUarray array);

} // namespace libvr
