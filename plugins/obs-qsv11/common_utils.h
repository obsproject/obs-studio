#pragma once

#include <stdio.h>

// Most of this file shouldnt be accessed from C.
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum qsv_codec {
	QSV_CODEC_AVC,
	QSV_CODEC_AV1,
	QSV_CODEC_HEVC,
};

struct adapter_info {
	bool is_intel;
	bool is_dgpu;
	bool supports_av1;
	bool supports_hevc;
};

#define MAX_ADAPTERS 10
extern struct adapter_info adapters[MAX_ADAPTERS];
extern size_t adapter_count;
extern size_t adapter_index;

void util_cpuid(int cpuinfo[4], int flags);
void check_adapters(struct adapter_info *adapters, size_t *adapter_count);

#ifdef __cplusplus
}

#include <vpl/mfxvideo++.h>
#include <vpl/mfxdispatcher.h>

constexpr inline int INTEL_VENDOR_ID = 0x8086;

// =================================================================
// OS-specific definitions of types, macro, etc...
// The following should be defined:
//  - mfxTime
//  - MSDK_FOPEN
//  - MSDK_SLEEP
#if defined(_WIN32) || defined(_WIN64)
#include "bits/windows_defs.h"
#elif defined(__linux__)
#include "bits/linux_defs.h"
#endif

// =================================================================
// Helper macro definitions...
#define MSDK_PRINT_RET_MSG(ERR)                          \
	{                                                \
		PrintErrString(ERR, __FILE__, __LINE__); \
	}
#define MSDK_CHECK_RESULT(P, X, ERR)             \
	{                                        \
		if ((X) > (P)) {                 \
			MSDK_PRINT_RET_MSG(ERR); \
			return ERR;              \
		}                                \
	}
#define MSDK_CHECK_POINTER(P, ERR)               \
	{                                        \
		if (!(P)) {                      \
			MSDK_PRINT_RET_MSG(ERR); \
			return ERR;              \
		}                                \
	}
#define MSDK_CHECK_ERROR(P, X, ERR)              \
	{                                        \
		if ((X) == (P)) {                \
			MSDK_PRINT_RET_MSG(ERR); \
			return ERR;              \
		}                                \
	}
#define MSDK_IGNORE_MFX_STS(P, X)         \
	{                                 \
		if ((X) == (P)) {         \
			P = MFX_ERR_NONE; \
		}                         \
	}
#define MSDK_BREAK_ON_ERROR(P)           \
	{                                \
		if (MFX_ERR_NONE != (P)) \
			break;           \
	}
#define MSDK_SAFE_DELETE_ARRAY(P)   \
	{                           \
		if (P) {            \
			delete[] P; \
			P = NULL;   \
		}                   \
	}
#define MSDK_ALIGN32(X) (((mfxU32)((X) + 31)) & (~(mfxU32)31))
#define MSDK_ALIGN16(value) (((value + 15) >> 4) << 4)
#define MSDK_SAFE_RELEASE(X)          \
	{                             \
		if (X) {              \
			X->Release(); \
			X = NULL;     \
		}                     \
	}
#define MSDK_MAX(A, B) (((A) > (B)) ? (A) : (B))

// Usage of the following two macros are only required for certain Windows DirectX11 use cases
#define WILL_READ 0x1000
#define WILL_WRITE 0x2000

// =================================================================
// Intel VPL memory allocator entrypoints....
// Implementation of this functions is OS/Memory type specific.
mfxStatus simple_alloc(mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
mfxStatus simple_lock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
mfxStatus simple_unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr);
mfxStatus simple_gethdl(mfxHDL pthis, mfxMemId mid, mfxHDL *handle);
mfxStatus simple_free(mfxHDL pthis, mfxFrameAllocResponse *response);
mfxStatus simple_copytex(mfxHDL pthis, mfxMemId mid, void *tex, mfxU64 lock_key, mfxU64 *next_key);

// =================================================================
// Utility functions, not directly tied to VPL functionality
//

void PrintErrString(int err, const char *filestr, int line);

// LoadRawFrame: Reads raw frame from YUV file (YV12) into NV12 surface
// - YV12 is a more common format for YUV files than NV12 (therefore the conversion during read and write)
// - For the simulation case (fSource = NULL), the surface is filled with default image data
// LoadRawRGBFrame: Reads raw RGB32 frames from file into RGB32 surface
// - For the simulation case (fSource = NULL), the surface is filled with default image data

mfxStatus LoadRawFrame(mfxFrameSurface1 *pSurface, FILE *fSource);
mfxStatus LoadRawRGBFrame(mfxFrameSurface1 *pSurface, FILE *fSource);

// Write raw YUV (NV12) surface to YUV (YV12) file
mfxStatus WriteRawFrame(mfxFrameSurface1 *pSurface, FILE *fSink);

// Write bit stream data for frame to file
mfxStatus WriteBitStreamFrame(mfxBitstream *pMfxBitstream, FILE *fSink);
// Read bit stream data from file. Stream is read as large chunks (= many frames)
mfxStatus ReadBitStreamData(mfxBitstream *pBS, FILE *fSource);

void ClearYUVSurfaceSysMem(mfxFrameSurface1 *pSfc, mfxU16 width, mfxU16 height);
void ClearYUVSurfaceVMem(mfxMemId memId);
void ClearRGBSurfaceVMem(mfxMemId memId);

// Get free raw frame surface
int GetFreeSurfaceIndex(mfxFrameSurface1 **pSurfacesPool, mfxU16 nPoolSize);

// For use with asynchronous task management
typedef struct {
	mfxBitstream mfxBS;
	mfxSyncPoint syncp;
} Task;

// Get free task
int GetFreeTaskIndex(Task *pTaskPool, mfxU16 nPoolSize);

// Initialize Intel VPL Session, device/display and memory manager
mfxStatus Initialize(mfxVersion ver, mfxSession *pSession, mfxFrameAllocator *pmfxAllocator, mfxHDL *deviceHandle,
		     bool bCreateSharedHandles, enum qsv_codec codec, void **data);

// Release global shared resources (device/display)
void Release();
// Release per session resources
void ReleaseSessionData(void *data);

// Convert frame type to string
char mfxFrameTypeString(mfxU16 FrameType);

void mfxGetTime(mfxTime *timestamp);

//void mfxInitTime();  might need this for Windows
double TimeDiffMsec(mfxTime tfinish, mfxTime tstart);

#endif // __cplusplus
