// Copyright (c) 2019-2020 Intel Corporation
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#ifndef __MFXDEFS_H__
#define __MFXDEFS_H__

#define MFX_VERSION_MAJOR 1
#define MFX_VERSION_MINOR 35

// MFX_VERSION_NEXT is always +1 from last public release
// may be enforced by MFX_VERSION_USE_LATEST define
// if MFX_VERSION_USE_LATEST is defined  MFX_VERSION is ignored

#define MFX_VERSION_NEXT (MFX_VERSION_MAJOR * 1000 + MFX_VERSION_MINOR + 1)

// MFX_VERSION - version of API that 'assumed' by build may be provided externally
// if it omitted then latest stable API derived from Major.Minor is assumed


#if !defined(MFX_VERSION)
  #if defined(MFX_VERSION_USE_LATEST)
    #define MFX_VERSION MFX_VERSION_NEXT
  #else
    #define MFX_VERSION (MFX_VERSION_MAJOR * 1000 + MFX_VERSION_MINOR)
  #endif
#else
  #undef MFX_VERSION_MINOR
  #define MFX_VERSION_MINOR ((MFX_VERSION) % 1000)
#endif


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* In preprocessor syntax # symbol has stringize meaning,
   so to expand some macro to preprocessor pragma we need to use
   special compiler dependent construction */

#if defined(_MSC_VER)
    #define MFX_PRAGMA_IMPL(x) __pragma(x)
#else
    #define MFX_PRAGMA_IMPL(x) _Pragma(#x)
#endif

#define MFX_PACK_BEGIN_X(x) MFX_PRAGMA_IMPL(pack(push, x))
#define MFX_PACK_END()      MFX_PRAGMA_IMPL(pack(pop))

/* The general rule for alignment is following:
   - structures with pointers have 4/8 bytes alignment on 32/64 bit systems
   - structures with fields of type mfxU64/mfxF64 (unsigned long long / double)
     have alignment 8 bytes on 64 bit and 32 bit Windows, on Linux alignment is 4 bytes
   - all the rest structures are 4 bytes aligned
   - there are several exceptions: some structs which had 4-byte alignment were extended
     with pointer / long type fields; such structs have 4-byte alignment to keep binary
     compatibility with previously release API */

#define MFX_PACK_BEGIN_USUAL_STRUCT()        MFX_PACK_BEGIN_X(4)

/* 64-bit LP64 data model */
#if defined(_WIN64) || defined(__LP64__)
    #define MFX_PACK_BEGIN_STRUCT_W_PTR()    MFX_PACK_BEGIN_X(8)
    #define MFX_PACK_BEGIN_STRUCT_W_L_TYPE() MFX_PACK_BEGIN_X(8)
/* 32-bit ILP32 data model Windows (Intel architecture) */
#elif defined(_WIN32) || defined(_M_IX86) && !defined(__linux__)
    #define MFX_PACK_BEGIN_STRUCT_W_PTR()    MFX_PACK_BEGIN_X(4)
    #define MFX_PACK_BEGIN_STRUCT_W_L_TYPE() MFX_PACK_BEGIN_X(8)
/* 32-bit ILP32 data model Linux */
#elif defined(__ILP32__)
    #define MFX_PACK_BEGIN_STRUCT_W_PTR()    MFX_PACK_BEGIN_X(4)
    #define MFX_PACK_BEGIN_STRUCT_W_L_TYPE() MFX_PACK_BEGIN_X(4)
#else
    #error Unknown packing
#endif

  #define __INT64   long long
  #define __UINT64  unsigned long long

#ifdef _WIN32
  #define MFX_CDECL __cdecl
  #define MFX_STDCALL __stdcall
#else
  #define MFX_CDECL
  #define MFX_STDCALL
#endif /* _WIN32 */

#define MFX_INFINITE 0xFFFFFFFF

#if !defined(MFX_DEPRECATED_OFF) && (MFX_VERSION >= 1034)
#define MFX_DEPRECATED_OFF
#endif

#ifndef MFX_DEPRECATED_OFF
  #if defined(__cplusplus) && __cplusplus >= 201402L
    #define MFX_DEPRECATED [[deprecated]]
    #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg [[deprecated]]
    #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
  #elif defined(__clang__)
    #define MFX_DEPRECATED __attribute__((deprecated))
    #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg __attribute__((deprecated))
    #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
  #elif defined(__INTEL_COMPILER)
    #if (defined(_WIN32) || defined(_WIN64))
      #define MFX_DEPRECATED __declspec(deprecated)
      #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg
      #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg) __pragma(deprecated(arg))
    #elif defined(__linux__)
      #define MFX_DEPRECATED __attribute__((deprecated))
      #if defined(__cplusplus)
        #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg __attribute__((deprecated))
      #else
        #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg
      #endif
      #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
    #endif
  #elif defined(_MSC_VER) && _MSC_VER > 1200 // VS 6 doesn't support deprecation
    #define MFX_DEPRECATED __declspec(deprecated)
    #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg
    #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg) __pragma(deprecated(arg))
  #elif defined(__GNUC__)
    #define MFX_DEPRECATED __attribute__((deprecated))
    #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg __attribute__((deprecated))
    #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
  #else
    #define MFX_DEPRECATED
    #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg
    #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
  #endif
#else
  #define MFX_DEPRECATED
  #define MFX_DEPRECATED_ENUM_FIELD_INSIDE(arg) arg
  #define MFX_DEPRECATED_ENUM_FIELD_OUTSIDE(arg)
#endif

typedef unsigned char       mfxU8;
typedef char                mfxI8;
typedef short               mfxI16;
typedef unsigned short      mfxU16;
typedef unsigned int        mfxU32;
typedef int                 mfxI32;
#if defined( _WIN32 ) || defined ( _WIN64 )
typedef unsigned long       mfxUL32;
typedef long                mfxL32;
#else
typedef unsigned int        mfxUL32;
typedef int                 mfxL32;
#endif
typedef float               mfxF32;
typedef double              mfxF64;
typedef __UINT64            mfxU64;
typedef __INT64             mfxI64;
typedef void*               mfxHDL;
typedef mfxHDL              mfxMemId;
typedef void*               mfxThreadTask;
typedef char                mfxChar;

typedef struct {
    mfxI16  x;
    mfxI16  y;
} mfxI16Pair;

typedef struct {
    mfxHDL first;
    mfxHDL second;
} mfxHDLPair;


/*********************************************************************************\
Error message
\*********************************************************************************/
typedef enum
{
    /* no error */
    MFX_ERR_NONE                        = 0,    /* no error */

    /* reserved for unexpected errors */
    MFX_ERR_UNKNOWN                     = -1,   /* unknown error. */

    /* error codes <0 */
    MFX_ERR_NULL_PTR                    = -2,   /* null pointer */
    MFX_ERR_UNSUPPORTED                 = -3,   /* undeveloped feature */
    MFX_ERR_MEMORY_ALLOC                = -4,   /* failed to allocate memory */
    MFX_ERR_NOT_ENOUGH_BUFFER           = -5,   /* insufficient buffer at input/output */
    MFX_ERR_INVALID_HANDLE              = -6,   /* invalid handle */
    MFX_ERR_LOCK_MEMORY                 = -7,   /* failed to lock the memory block */
    MFX_ERR_NOT_INITIALIZED             = -8,   /* member function called before initialization */
    MFX_ERR_NOT_FOUND                   = -9,   /* the specified object is not found */
    MFX_ERR_MORE_DATA                   = -10,  /* expect more data at input */
    MFX_ERR_MORE_SURFACE                = -11,  /* expect more surface at output */
    MFX_ERR_ABORTED                     = -12,  /* operation aborted */
    MFX_ERR_DEVICE_LOST                 = -13,  /* lose the HW acceleration device */
    MFX_ERR_INCOMPATIBLE_VIDEO_PARAM    = -14,  /* incompatible video parameters */
    MFX_ERR_INVALID_VIDEO_PARAM         = -15,  /* invalid video parameters */
    MFX_ERR_UNDEFINED_BEHAVIOR          = -16,  /* undefined behavior */
    MFX_ERR_DEVICE_FAILED               = -17,  /* device operation failure */
    MFX_ERR_MORE_BITSTREAM              = -18,  /* expect more bitstream buffers at output */
    MFX_ERR_INCOMPATIBLE_AUDIO_PARAM    = -19,  /* incompatible audio parameters */
    MFX_ERR_INVALID_AUDIO_PARAM         = -20,  /* invalid audio parameters */
    MFX_ERR_GPU_HANG                    = -21,  /* device operation failure caused by GPU hang */
    MFX_ERR_REALLOC_SURFACE             = -22,  /* bigger output surface required */

    /* warnings >0 */
    MFX_WRN_IN_EXECUTION                = 1,    /* the previous asynchronous operation is in execution */
    MFX_WRN_DEVICE_BUSY                 = 2,    /* the HW acceleration device is busy */
    MFX_WRN_VIDEO_PARAM_CHANGED         = 3,    /* the video parameters are changed during decoding */
    MFX_WRN_PARTIAL_ACCELERATION        = 4,    /* SW is used */
    MFX_WRN_INCOMPATIBLE_VIDEO_PARAM    = 5,    /* incompatible video parameters */
    MFX_WRN_VALUE_NOT_CHANGED           = 6,    /* the value is saturated based on its valid range */
    MFX_WRN_OUT_OF_RANGE                = 7,    /* the value is out of valid range */
    MFX_WRN_FILTER_SKIPPED              = 10,   /* one of requested filters has been skipped */
    MFX_WRN_INCOMPATIBLE_AUDIO_PARAM    = 11,   /* incompatible audio parameters */

#if MFX_VERSION >= 1031
    /* low-delay partial output */
    MFX_ERR_NONE_PARTIAL_OUTPUT         = 12,   /* frame is not ready, but bitstream contains partial output */
#endif

    /* threading statuses */
    MFX_TASK_DONE = MFX_ERR_NONE,               /* task has been completed */
    MFX_TASK_WORKING                    = 8,    /* there is some more work to do */
    MFX_TASK_BUSY                       = 9,    /* task is waiting for resources */

    /* plug-in statuses */
    MFX_ERR_MORE_DATA_SUBMIT_TASK       = -10000, /* return MFX_ERR_MORE_DATA but submit internal asynchronous task */

} mfxStatus;


// Application 
#if defined(MFX_DISPATCHER_EXPOSED_PREFIX)

#include "mfxdispatcherprefixedfunctions.h"

#endif // MFX_DISPATCHER_EXPOSED_PREFIX


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MFXDEFS_H__ */
