/*
 * This copyright notice applies to this header file only:
 *
 * Copyright (c) 2016
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined(FFNV_DYNLINK_CUDA_H) && !defined(CUDA_VERSION)
#define FFNV_DYNLINK_CUDA_H

#include <stddef.h>
#include <stdint.h>

#define CUDA_VERSION 7050

#if defined(_WIN32) || defined(__CYGWIN__)
#define CUDAAPI __stdcall
#else
#define CUDAAPI
#endif

#define CU_CTX_SCHED_BLOCKING_SYNC 4

typedef int CUdevice;
#if defined(__x86_64) || defined(AMD64) || defined(_M_AMD64) || defined(__LP64__) || defined(__aarch64__)
typedef unsigned long long CUdeviceptr;
#else
typedef unsigned int CUdeviceptr;
#endif
typedef unsigned long long CUtexObject;

typedef struct CUarray_st            *CUarray;
typedef struct CUctx_st              *CUcontext;
typedef struct CUstream_st           *CUstream;
typedef struct CUevent_st            *CUevent;
typedef struct CUfunc_st             *CUfunction;
typedef struct CUmod_st              *CUmodule;
typedef struct CUmipmappedArray_st   *CUmipmappedArray;
typedef struct CUgraphicsResource_st *CUgraphicsResource;
typedef struct CUextMemory_st        *CUexternalMemory;
typedef struct CUextSemaphore_st     *CUexternalSemaphore;
typedef struct CUeglStreamConnection_st *CUeglStreamConnection;

typedef struct CUlinkState_st *CUlinkState;

typedef enum cudaError_enum {
    CUDA_SUCCESS = 0,
    CUDA_ERROR_NOT_READY = 600,
    CUDA_ERROR_LAUNCH_TIMEOUT = 702,
    CUDA_ERROR_UNKNOWN = 999
} CUresult;

/**
 * Device properties (subset)
 */
typedef enum CUdevice_attribute_enum {
    CU_DEVICE_ATTRIBUTE_CLOCK_RATE = 13,
    CU_DEVICE_ATTRIBUTE_TEXTURE_ALIGNMENT = 14,
    CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = 16,
    CU_DEVICE_ATTRIBUTE_INTEGRATED = 18,
    CU_DEVICE_ATTRIBUTE_CAN_MAP_HOST_MEMORY = 19,
    CU_DEVICE_ATTRIBUTE_COMPUTE_MODE = 20,
    CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS = 31,
    CU_DEVICE_ATTRIBUTE_PCI_BUS_ID = 33,
    CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 34,
    CU_DEVICE_ATTRIBUTE_TCC_DRIVER = 35,
    CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE = 36,
    CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH = 37,
    CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT = 40,
    CU_DEVICE_ATTRIBUTE_UNIFIED_ADDRESSING = 41,
    CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID = 50,
    CU_DEVICE_ATTRIBUTE_TEXTURE_PITCH_ALIGNMENT = 51,
    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 75,
    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 76,
    CU_DEVICE_ATTRIBUTE_MANAGED_MEMORY = 83,
    CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD = 84,
    CU_DEVICE_ATTRIBUTE_MULTI_GPU_BOARD_GROUP_ID = 85,
} CUdevice_attribute;

typedef enum CUarray_format_enum {
    CU_AD_FORMAT_UNSIGNED_INT8  = 0x01,
    CU_AD_FORMAT_UNSIGNED_INT16 = 0x02,
    CU_AD_FORMAT_UNSIGNED_INT32 = 0x03,
    CU_AD_FORMAT_SIGNED_INT8    = 0x08,
    CU_AD_FORMAT_SIGNED_INT16   = 0x09,
    CU_AD_FORMAT_SIGNED_INT32   = 0x0a,
    CU_AD_FORMAT_HALF           = 0x10,
    CU_AD_FORMAT_FLOAT          = 0x20
} CUarray_format;

typedef enum CUmemorytype_enum {
    CU_MEMORYTYPE_HOST = 1,
    CU_MEMORYTYPE_DEVICE = 2,
    CU_MEMORYTYPE_ARRAY = 3
} CUmemorytype;

typedef enum CUlimit_enum {
    CU_LIMIT_STACK_SIZE = 0,
    CU_LIMIT_PRINTF_FIFO_SIZE = 1,
    CU_LIMIT_MALLOC_HEAP_SIZE = 2,
    CU_LIMIT_DEV_RUNTIME_SYNC_DEPTH = 3,
    CU_LIMIT_DEV_RUNTIME_PENDING_LAUNCH_COUNT = 4
} CUlimit;

typedef enum CUresourcetype_enum {
    CU_RESOURCE_TYPE_ARRAY           = 0x00,
    CU_RESOURCE_TYPE_MIPMAPPED_ARRAY = 0x01,
    CU_RESOURCE_TYPE_LINEAR          = 0x02,
    CU_RESOURCE_TYPE_PITCH2D         = 0x03
} CUresourcetype;

typedef enum CUaddress_mode_enum {
    CU_TR_ADDRESS_MODE_WRAP = 0,
    CU_TR_ADDRESS_MODE_CLAMP = 1,
    CU_TR_ADDRESS_MODE_MIRROR = 2,
    CU_TR_ADDRESS_MODE_BORDER = 3
} CUaddress_mode;

typedef enum CUfilter_mode_enum {
    CU_TR_FILTER_MODE_POINT = 0,
    CU_TR_FILTER_MODE_LINEAR = 1
} CUfilter_mode;

typedef enum CUgraphicsRegisterFlags_enum {
    CU_GRAPHICS_REGISTER_FLAGS_NONE = 0,
    CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY = 1,
    CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD = 2,
    CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST = 4,
    CU_GRAPHICS_REGISTER_FLAGS_TEXTURE_GATHER = 8
} CUgraphicsRegisterFlags;

typedef enum CUexternalMemoryHandleType_enum {
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD        = 1,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32     = 2,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT = 3,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP       = 4,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE   = 5,
} CUexternalMemoryHandleType;

typedef enum CUexternalSemaphoreHandleType_enum {
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD                = 1,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32             = 2,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT         = 3,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE              = 4,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_FD    = 9,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_TIMELINE_SEMAPHORE_WIN32 = 10,
} CUexternalSemaphoreHandleType;

typedef enum CUjit_option_enum
{
    CU_JIT_MAX_REGISTERS               = 0,
    CU_JIT_THREADS_PER_BLOCK           = 1,
    CU_JIT_WALL_TIME                   = 2,
    CU_JIT_INFO_LOG_BUFFER             = 3,
    CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES  = 4,
    CU_JIT_ERROR_LOG_BUFFER            = 5,
    CU_JIT_ERROR_LOG_BUFFER_SIZE_BYTES = 6,
    CU_JIT_OPTIMIZATION_LEVEL          = 7,
    CU_JIT_TARGET_FROM_CUCONTEXT       = 8,
    CU_JIT_TARGET                      = 9,
    CU_JIT_FALLBACK_STRATEGY           = 10,
    CU_JIT_GENERATE_DEBUG_INFO         = 11,
    CU_JIT_LOG_VERBOSE                 = 12,
    CU_JIT_GENERATE_LINE_INFO          = 13,
    CU_JIT_CACHE_MODE                  = 14,
    CU_JIT_NEW_SM3X_OPT                = 15,
    CU_JIT_FAST_COMPILE                = 16,
    CU_JIT_GLOBAL_SYMBOL_NAMES         = 17,
    CU_JIT_GLOBAL_SYMBOL_ADDRESSES     = 18,
    CU_JIT_GLOBAL_SYMBOL_COUNT         = 19,
    CU_JIT_NUM_OPTIONS
} CUjit_option;

typedef enum CUjitInputType_enum
{
    CU_JIT_INPUT_CUBIN     = 0,
    CU_JIT_INPUT_PTX       = 1,
    CU_JIT_INPUT_FATBINARY = 2,
    CU_JIT_INPUT_OBJECT    = 3,
    CU_JIT_INPUT_LIBRARY   = 4,
    CU_JIT_NUM_INPUT_TYPES
} CUjitInputType;

typedef enum CUeglFrameType
{
    CU_EGL_FRAME_TYPE_ARRAY = 0,
    CU_EGL_FRAME_TYPE_PITCH = 1,
} CUeglFrameType;

typedef enum CUeglColorFormat
{
    CU_EGL_COLOR_FORMAT_YUV420_PLANAR = 0x00,
    CU_EGL_COLOR_FORMAT_YUV420_SEMIPLANAR = 0x01,
    CU_EGL_COLOR_FORMAT_YVU420_SEMIPLANAR = 0x15,
    CU_EGL_COLOR_FORMAT_Y10V10U10_420_SEMIPLANAR = 0x17,
    CU_EGL_COLOR_FORMAT_Y12V12U12_420_SEMIPLANAR = 0x19,
} CUeglColorFormat;

#ifndef CU_UUID_HAS_BEEN_DEFINED
#define CU_UUID_HAS_BEEN_DEFINED
typedef struct CUuuid_st {
    char bytes[16];
} CUuuid;
#endif

typedef struct CUDA_MEMCPY2D_st {
    size_t srcXInBytes;
    size_t srcY;
    CUmemorytype srcMemoryType;
    const void *srcHost;
    CUdeviceptr srcDevice;
    CUarray srcArray;
    size_t srcPitch;

    size_t dstXInBytes;
    size_t dstY;
    CUmemorytype dstMemoryType;
    void *dstHost;
    CUdeviceptr dstDevice;
    CUarray dstArray;
    size_t dstPitch;

    size_t WidthInBytes;
    size_t Height;
} CUDA_MEMCPY2D;

typedef struct CUDA_RESOURCE_DESC_st {
    CUresourcetype resType;
    union {
        struct {
            CUarray hArray;
        } array;
        struct {
            CUmipmappedArray hMipmappedArray;
        } mipmap;
        struct {
            CUdeviceptr devPtr;
            CUarray_format format;
            unsigned int numChannels;
            size_t sizeInBytes;
        } linear;
        struct {
            CUdeviceptr devPtr;
            CUarray_format format;
            unsigned int numChannels;
            size_t width;
            size_t height;
            size_t pitchInBytes;
        } pitch2D;
        struct {
            int reserved[32];
        } reserved;
    } res;
    unsigned int flags;
} CUDA_RESOURCE_DESC;

typedef struct CUDA_TEXTURE_DESC_st {
    CUaddress_mode addressMode[3];
    CUfilter_mode filterMode;
    unsigned int flags;
    unsigned int maxAnisotropy;
    CUfilter_mode mipmapFilterMode;
    float mipmapLevelBias;
    float minMipmapLevelClamp;
    float maxMipmapLevelClamp;
    float borderColor[4];
    int reserved[12];
} CUDA_TEXTURE_DESC;

/* Unused type */
typedef struct CUDA_RESOURCE_VIEW_DESC_st CUDA_RESOURCE_VIEW_DESC;

typedef unsigned int GLenum;
typedef unsigned int GLuint;
/*
 * Prefix type name to avoid collisions. Clients using these types
 * will include the real headers with real definitions.
 */
typedef int32_t ffnv_EGLint;
typedef void *ffnv_EGLStreamKHR;

typedef enum CUGLDeviceList_enum {
    CU_GL_DEVICE_LIST_ALL = 1,
    CU_GL_DEVICE_LIST_CURRENT_FRAME = 2,
    CU_GL_DEVICE_LIST_NEXT_FRAME = 3,
} CUGLDeviceList;

typedef struct CUDA_EXTERNAL_MEMORY_HANDLE_DESC_st {
    CUexternalMemoryHandleType type;
    union {
        int fd;
        struct {
            void *handle;
            const void *name;
        } win32;
    } handle;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_HANDLE_DESC;

typedef struct CUDA_EXTERNAL_MEMORY_BUFFER_DESC_st {
    unsigned long long offset;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_BUFFER_DESC;

typedef struct CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC_st {
    CUexternalSemaphoreHandleType type;
    union {
        int fd;
        struct {
            void *handle;
            const void *name;
        } win32;
    } handle;
    unsigned int flags;
    unsigned int reserved[16];
} CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC;

typedef struct CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_st {
    struct {
        struct {
            unsigned long long value;
        } fence;
        unsigned int reserved[16];
    } params;
    unsigned int flags;
    unsigned int reserved[16];
} CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS;

typedef CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS;

typedef struct CUDA_ARRAY3D_DESCRIPTOR_st {
    size_t Width;
    size_t Height;
    size_t Depth;

    CUarray_format Format;
    unsigned int NumChannels;
    unsigned int Flags;
} CUDA_ARRAY3D_DESCRIPTOR;

typedef struct CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC_st {
    unsigned long long offset;
    CUDA_ARRAY3D_DESCRIPTOR arrayDesc;
    unsigned int numLevels;
    unsigned int reserved[16];
} CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC;

#define CU_EGL_FRAME_MAX_PLANES 3
typedef struct CUeglFrame_st {
    union {
        CUarray pArray[CU_EGL_FRAME_MAX_PLANES];
        void* pPitch[CU_EGL_FRAME_MAX_PLANES];
    } frame;
    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int pitch;
    unsigned int planeCount;
    unsigned int numChannels;
    CUeglFrameType frameType;
    CUeglColorFormat eglColorFormat;
    CUarray_format cuFormat;
} CUeglFrame;

#define CU_STREAM_NON_BLOCKING 1
#define CU_EVENT_BLOCKING_SYNC 1
#define CU_EVENT_DISABLE_TIMING 2
#define CU_TRSF_READ_AS_INTEGER 1

typedef void CUDAAPI CUstreamCallback(CUstream hStream, CUresult status, void *userdata);

typedef CUresult CUDAAPI tcuInit(unsigned int Flags);
typedef CUresult CUDAAPI tcuDeviceGetCount(int *count);
typedef CUresult CUDAAPI tcuDeviceGet(CUdevice *device, int ordinal);
typedef CUresult CUDAAPI tcuDeviceGetAttribute(int *pi, CUdevice_attribute attrib, CUdevice dev);
typedef CUresult CUDAAPI tcuDeviceGetName(char *name, int len, CUdevice dev);
typedef CUresult CUDAAPI tcuDeviceGetUuid(CUuuid *uuid, CUdevice dev);
typedef CUresult CUDAAPI tcuDeviceComputeCapability(int *major, int *minor, CUdevice dev);
typedef CUresult CUDAAPI tcuCtxCreate_v2(CUcontext *pctx, unsigned int flags, CUdevice dev);
typedef CUresult CUDAAPI tcuCtxSetLimit(CUlimit limit, size_t value);
typedef CUresult CUDAAPI tcuCtxPushCurrent_v2(CUcontext pctx);
typedef CUresult CUDAAPI tcuCtxPopCurrent_v2(CUcontext *pctx);
typedef CUresult CUDAAPI tcuCtxDestroy_v2(CUcontext ctx);
typedef CUresult CUDAAPI tcuMemAlloc_v2(CUdeviceptr *dptr, size_t bytesize);
typedef CUresult CUDAAPI tcuMemAllocPitch_v2(CUdeviceptr *dptr, size_t *pPitch, size_t WidthInBytes, size_t Height, unsigned int ElementSizeBytes);
typedef CUresult CUDAAPI tcuMemAllocManaged(CUdeviceptr *dptr, size_t bytesize, unsigned int flags);
typedef CUresult CUDAAPI tcuMemsetD8Async(CUdeviceptr dstDevice, unsigned char uc, size_t N, CUstream hStream);
typedef CUresult CUDAAPI tcuMemFree_v2(CUdeviceptr dptr);
typedef CUresult CUDAAPI tcuMemcpy(CUdeviceptr dst, CUdeviceptr src, size_t bytesize);
typedef CUresult CUDAAPI tcuMemcpyAsync(CUdeviceptr dst, CUdeviceptr src, size_t bytesize, CUstream hStream);
typedef CUresult CUDAAPI tcuMemcpy2D_v2(const CUDA_MEMCPY2D *pcopy);
typedef CUresult CUDAAPI tcuMemcpy2DAsync_v2(const CUDA_MEMCPY2D *pcopy, CUstream hStream);
typedef CUresult CUDAAPI tcuMemcpyHtoD_v2(CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount);
typedef CUresult CUDAAPI tcuMemcpyHtoDAsync_v2(CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI tcuMemcpyDtoH_v2(void *dstHost, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult CUDAAPI tcuMemcpyDtoHAsync_v2(void *dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI tcuMemcpyDtoD_v2(CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult CUDAAPI tcuMemcpyDtoDAsync_v2(CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream);
typedef CUresult CUDAAPI tcuGetErrorName(CUresult error, const char** pstr);
typedef CUresult CUDAAPI tcuGetErrorString(CUresult error, const char** pstr);
typedef CUresult CUDAAPI tcuCtxGetDevice(CUdevice *device);

typedef CUresult CUDAAPI tcuDevicePrimaryCtxRetain(CUcontext *pctx, CUdevice dev);
typedef CUresult CUDAAPI tcuDevicePrimaryCtxRelease(CUdevice dev);
typedef CUresult CUDAAPI tcuDevicePrimaryCtxSetFlags(CUdevice dev, unsigned int flags);
typedef CUresult CUDAAPI tcuDevicePrimaryCtxGetState(CUdevice dev, unsigned int *flags, int *active);
typedef CUresult CUDAAPI tcuDevicePrimaryCtxReset(CUdevice dev);

typedef CUresult CUDAAPI tcuStreamCreate(CUstream *phStream, unsigned int flags);
typedef CUresult CUDAAPI tcuStreamQuery(CUstream hStream);
typedef CUresult CUDAAPI tcuStreamSynchronize(CUstream hStream);
typedef CUresult CUDAAPI tcuStreamDestroy_v2(CUstream hStream);
typedef CUresult CUDAAPI tcuStreamAddCallback(CUstream hStream, CUstreamCallback *callback, void *userdata, unsigned int flags);
typedef CUresult CUDAAPI tcuEventCreate(CUevent *phEvent, unsigned int flags);
typedef CUresult CUDAAPI tcuEventDestroy_v2(CUevent hEvent);
typedef CUresult CUDAAPI tcuEventSynchronize(CUevent hEvent);
typedef CUresult CUDAAPI tcuEventQuery(CUevent hEvent);
typedef CUresult CUDAAPI tcuEventRecord(CUevent hEvent, CUstream hStream);

typedef CUresult CUDAAPI tcuLaunchKernel(CUfunction f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ, unsigned int sharedMemBytes, CUstream hStream, void** kernelParams, void** extra);
typedef CUresult CUDAAPI tcuLinkCreate(unsigned int  numOptions, CUjit_option* options, void** optionValues, CUlinkState* stateOut);
typedef CUresult CUDAAPI tcuLinkAddData(CUlinkState state, CUjitInputType type, void* data, size_t size, const char* name, unsigned int numOptions, CUjit_option* options, void** optionValues);
typedef CUresult CUDAAPI tcuLinkComplete(CUlinkState state, void** cubinOut, size_t* sizeOut);
typedef CUresult CUDAAPI tcuLinkDestroy(CUlinkState state);
typedef CUresult CUDAAPI tcuModuleLoadData(CUmodule* module, const void* image);
typedef CUresult CUDAAPI tcuModuleUnload(CUmodule hmod);
typedef CUresult CUDAAPI tcuModuleGetFunction(CUfunction* hfunc, CUmodule hmod, const char* name);
typedef CUresult CUDAAPI tcuModuleGetGlobal(CUdeviceptr *dptr, size_t *bytes, CUmodule hmod, const char* name);
typedef CUresult CUDAAPI tcuTexObjectCreate(CUtexObject* pTexObject, const CUDA_RESOURCE_DESC* pResDesc, const CUDA_TEXTURE_DESC* pTexDesc, const CUDA_RESOURCE_VIEW_DESC* pResViewDesc);
typedef CUresult CUDAAPI tcuTexObjectDestroy(CUtexObject texObject);

typedef CUresult CUDAAPI tcuGLGetDevices_v2(unsigned int* pCudaDeviceCount, CUdevice* pCudaDevices, unsigned int cudaDeviceCount, CUGLDeviceList deviceList);
typedef CUresult CUDAAPI tcuGraphicsGLRegisterImage(CUgraphicsResource* pCudaResource, GLuint image, GLenum target, unsigned int Flags);
typedef CUresult CUDAAPI tcuGraphicsUnregisterResource(CUgraphicsResource resource);
typedef CUresult CUDAAPI tcuGraphicsMapResources(unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI tcuGraphicsUnmapResources(unsigned int count, CUgraphicsResource* resources, CUstream hStream);
typedef CUresult CUDAAPI tcuGraphicsSubResourceGetMappedArray(CUarray* pArray, CUgraphicsResource resource, unsigned int arrayIndex, unsigned int mipLevel);

typedef CUresult CUDAAPI tcuImportExternalMemory(CUexternalMemory* extMem_out, const CUDA_EXTERNAL_MEMORY_HANDLE_DESC* memHandleDesc);
typedef CUresult CUDAAPI tcuDestroyExternalMemory(CUexternalMemory extMem);
typedef CUresult CUDAAPI tcuExternalMemoryGetMappedBuffer(CUdeviceptr* devPtr, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_BUFFER_DESC* bufferDesc);
typedef CUresult CUDAAPI tcuExternalMemoryGetMappedMipmappedArray(CUmipmappedArray* mipmap, CUexternalMemory extMem, const CUDA_EXTERNAL_MEMORY_MIPMAPPED_ARRAY_DESC* mipmapDesc);
typedef CUresult CUDAAPI tcuMipmappedArrayGetLevel(CUarray* pLevelArray, CUmipmappedArray hMipmappedArray, unsigned int level);
typedef CUresult CUDAAPI tcuMipmappedArrayDestroy(CUmipmappedArray hMipmappedArray);

typedef CUresult CUDAAPI tcuImportExternalSemaphore(CUexternalSemaphore* extSem_out, const CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC* semHandleDesc);
typedef CUresult CUDAAPI tcuDestroyExternalSemaphore(CUexternalSemaphore extSem);
typedef CUresult CUDAAPI tcuSignalExternalSemaphoresAsync(const CUexternalSemaphore* extSemArray, const CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS* paramsArray, unsigned int numExtSems, CUstream stream);
typedef CUresult CUDAAPI tcuWaitExternalSemaphoresAsync(const CUexternalSemaphore* extSemArray, const CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS* paramsArray, unsigned int numExtSems, CUstream stream);

typedef CUresult CUDAAPI tcuArray3DCreate(CUarray *pHandle, const CUDA_ARRAY3D_DESCRIPTOR* pAllocateArray);
typedef CUresult CUDAAPI tcuArrayDestroy(CUarray hArray);

typedef CUresult CUDAAPI tcuEGLStreamProducerConnect (CUeglStreamConnection* conn, ffnv_EGLStreamKHR stream, ffnv_EGLint width, ffnv_EGLint height);
typedef CUresult CUDAAPI tcuEGLStreamProducerDisconnect (CUeglStreamConnection* conn);
typedef CUresult CUDAAPI tcuEGLStreamConsumerDisconnect (CUeglStreamConnection* conn);
typedef CUresult CUDAAPI tcuEGLStreamProducerPresentFrame (CUeglStreamConnection* conn, CUeglFrame eglframe, CUstream* pStream);
typedef CUresult CUDAAPI tcuEGLStreamProducerReturnFrame (CUeglStreamConnection* conn, CUeglFrame* eglframe, CUstream* pStream);
#endif
