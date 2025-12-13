#include "cuda_utils.h"
#include <dlfcn.h>
#include <iostream>
#include <vector>

namespace libvr {

// CUDA Driver API defines (Subset)
#define CUDA_SUCCESS 0

typedef int CUresult;
typedef int CUdevice;
typedef struct CUctx_st *CUcontext;

typedef enum {
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD = 1,
} CUexternalMemoryHandleType;

typedef struct CUDA_EXTERNAL_MEMORY_HANDLE_DESC_st {
    CUexternalMemoryHandleType type;
    union {
        int fd;
        void* handle;
    } handle;
    unsigned long long size;
    unsigned int flags;
} CUDA_EXTERNAL_MEMORY_HANDLE_DESC;

typedef struct CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC_st {
    unsigned long long offset;
    struct { // CUDA_ARRAY3D_DESCRIPTOR
        unsigned int Width;
        unsigned int Height;
        unsigned int Depth;
        unsigned int Format; // CUarray_format
        unsigned int NumChannels;
        unsigned int Flags;
    } arrayDesc;
    unsigned int numLevels;
    unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC;

typedef struct CUmipmappedArray_st *CUmipmappedArray;

// Function Pointers
typedef CUresult (*t_cuInit)(unsigned int Flags);
typedef CUresult (*t_cuDeviceGet)(CUdevice *device, int ordinal);
typedef CUresult (*t_cuCtxCreate)(CUcontext *pctx, unsigned int flags, CUdevice dev);
typedef CUresult (*t_cuImportExternalMemory)(CUexternalMemory *extMem_out, const CUDA_EXTERNAL_MEMORY_HANDLE_DESC *memHandleDesc);
typedef CUresult (*t_cuExternalMemoryGetMappedMipmappedArray)(CUmipmappedArray *mipmap_out, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC *mipmapDesc);
typedef CUresult (*t_cuMipmappedArrayGetLevel)(CUarray *pLevelArray, CUmipmappedArray hMipmappedArray, unsigned int level);
typedef CUresult (*t_cuDestroyExternalMemory)(CUexternalMemory extMem);
typedef CUresult (*t_cuMipmappedArrayDestroy)(CUmipmappedArray hMipmappedArray);

static t_cuInit cuInit = nullptr;
static t_cuImportExternalMemory cuImportExternalMemory = nullptr;
static t_cuExternalMemoryGetMappedMipmappedArray cuExternalMemoryGetMappedMipmappedArray = nullptr;
static t_cuMipmappedArrayGetLevel cuMipmappedArrayGetLevel = nullptr;
static t_cuDestroyExternalMemory cuDestroyExternalMemory = nullptr;
static t_cuMipmappedArrayDestroy cuMipmappedArrayDestroy = nullptr;

static void* libcuda = nullptr;

bool InitCudaDriver() {
    if (libcuda) return true;
    
    libcuda = dlopen("libcuda.so.1", RTLD_LAZY | RTLD_GLOBAL);
    if (!libcuda) {
        // Fallback
        libcuda = dlopen("libcuda.so", RTLD_LAZY | RTLD_GLOBAL);
    }
    
    if (!libcuda) {
        std::cerr << "[CUDA] Failed to load libcuda.so" << std::endl;
        return false;
    }
    
    cuInit = (t_cuInit)dlsym(libcuda, "cuInit");
    cuImportExternalMemory = (t_cuImportExternalMemory)dlsym(libcuda, "cuImportExternalMemory");
    cuExternalMemoryGetMappedMipmappedArray = (t_cuExternalMemoryGetMappedMipmappedArray)dlsym(libcuda, "cuExternalMemoryGetMappedMipmappedArray");
    cuMipmappedArrayGetLevel = (t_cuMipmappedArrayGetLevel)dlsym(libcuda, "cuMipmappedArrayGetLevel");
    cuDestroyExternalMemory = (t_cuDestroyExternalMemory)dlsym(libcuda, "cuDestroyExternalMemory");
    cuMipmappedArrayDestroy = (t_cuMipmappedArrayDestroy)dlsym(libcuda, "cuMipmappedArrayDestroy");

    if (!cuInit || !cuImportExternalMemory) {
        std::cerr << "[CUDA] Missing required symbols in libcuda" << std::endl;
        return false;
    }

    CUresult res = cuInit(0);
    if (res != CUDA_SUCCESS) {
        std::cerr << "[CUDA] cuInit failed: " << res << std::endl;
        return false;
    }
    
    return true;
}

bool ImportVulkanMemoryToCuda(int fd, uint64_t size, CUexternalMemory* out_ext_mem) {
    if (!InitCudaDriver()) return false;
    
    CUDA_EXTERNAL_MEMORY_HANDLE_DESC desc = {};
    desc.type = CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD;
    desc.handle.fd = fd;
    desc.size = size;
    
    CUresult res = cuImportExternalMemory(out_ext_mem, &desc);
    if (res != CUDA_SUCCESS) {
        std::cerr << "[CUDA] cuImportExternalMemory failed: " << res << std::endl;
        return false;
    }
    
    return true;
}

bool MapCudaArrayFromExternalMemory(CUexternalMemory ext_mem, uint32_t width, uint32_t height, uint32_t format, CUarray* out_array) {
    // Assumption: VK_FORMAT_R8G8B8A8_UNORM maps to CU_AD_FORMAT_UNSIGNED_INT8 with 4 channels
    // Real implementation needs a robust switch/case
    
    CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC desc = {};
    desc.offset = 0;
    desc.arrayDesc.Width = width;
    desc.arrayDesc.Height = height;
    desc.arrayDesc.Depth = 0;
    desc.arrayDesc.Format = 0x01; // CU_AD_FORMAT_UNSIGNED_INT8
    desc.arrayDesc.NumChannels = 4;
    desc.arrayDesc.Flags = 0; // CUDA_ARRAY3D_SURFACE_LDST
    desc.numLevels = 1;

    CUmipmappedArray mipmap;
    CUresult res = cuExternalMemoryGetMappedMipmappedArray(&mipmap, ext_mem, &desc);
    if (res != CUDA_SUCCESS) {
        std::cerr << "[CUDA] GetMappedMipmappedArray failed: " << res << std::endl;
        return false;
    }
    
    // Get level 0
    res = cuMipmappedArrayGetLevel(out_array, mipmap, 0);
     if (res != CUDA_SUCCESS) {
        std::cerr << "[CUDA] GetLevel failed: " << res << std::endl;
        return false;
    }
    
    // Note: CUmipmappedArray conceptually owns the array, but for interop often we keep it alive?
    // Simplified lifecycle here. A robust impl would store CUmipmappedArray handle too.
    
    return true;
}

void FreeCudaResources(CUexternalMemory ext_mem, CUarray array) {
    if (ext_mem) cuDestroyExternalMemory(ext_mem);
    // array is destroyed if mipmap is destroyed, need to track that. Simple leak here for skeleton.
}

} // namespace libvr
