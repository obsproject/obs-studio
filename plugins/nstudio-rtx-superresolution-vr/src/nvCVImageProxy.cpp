#if defined(linux) || defined(unix) || defined(__linux)
#warning nvCVImageProxy.cpp not ported
#else
/*###############################################################################
#
# Copyright 2020 NVIDIA Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
###############################################################################*/
#include <string>
#include "include/nvCVImage.h"

#ifdef _WIN32
  #define _WINSOCKAPI_
  #include <windows.h>
  #include <tchar.h>
  #include "include/nvTransferD3D.h"
  #include "include/nvTransferD3D11.h"
#else // !_WIN32
  #include <dlfcn.h>
  typedef void* HMODULE;
  typedef void* HANDLE;
  typedef void* HINSTANCE;
#endif // _WIN32

// Parameter string does not include the file extension
#ifdef _WIN32
#define nvLoadLibrary(library) LoadLibrary(TEXT(library ".dll"))
#else // !_WIN32
#define nvLoadLibrary(library) dlopen("lib" library ".so", RTLD_LAZY)
#endif // _WIN32


inline void* nvGetProcAddress(HINSTANCE handle, const char* proc) {
  if (nullptr == handle) return nullptr;
#ifdef _WIN32
  return GetProcAddress(handle, proc);
#else // !_WIN32
  return dlsym(handle, proc);
#endif // _WIN32
}

inline int nvFreeLibrary(HINSTANCE handle) {
#ifdef _WIN32
  return FreeLibrary(handle);
#else
  return dlclose(handle);
#endif
}

HINSTANCE getNvCVImageLib() {
  TCHAR path[MAX_PATH], tmpPath[MAX_PATH], fullPath[MAX_PATH];
  static HINSTANCE nvCVImageLib = NULL;
  static bool bSDKPathSet = false;
  if (!bSDKPathSet) {
    nvCVImageLib = nvLoadLibrary("NVCVImage");
    if (nvCVImageLib)  bSDKPathSet = true;
  }
  if (!bSDKPathSet) {
    // There can be multiple apps on the system,
    // some might include the SDK in the app package and
    // others might expect the SDK to be installed in Program Files
    GetEnvironmentVariable(TEXT("NV_VIDEO_EFFECTS_PATH"), path, MAX_PATH);
    GetEnvironmentVariable(TEXT("NV_AR_SDK_PATH"), tmpPath, MAX_PATH);
    if (_tcscmp(path, TEXT("USE_APP_PATH")) && _tcscmp(tmpPath, TEXT("USE_APP_PATH"))) {
      // App has not set environment variable to "USE_APP_PATH"
      // So pick up the SDK dll and dependencies from Program Files
      GetEnvironmentVariable(TEXT("ProgramFiles"), path, MAX_PATH);
      size_t max_len = sizeof(fullPath) / sizeof(TCHAR);
      _stprintf_s(fullPath, max_len, TEXT("%s\\NVIDIA Corporation\\NVIDIA Video Effects\\"), path);
      SetDllDirectory(fullPath);
      nvCVImageLib = nvLoadLibrary("NVCVImage");
      if (!nvCVImageLib) {
        _stprintf_s(fullPath, max_len, TEXT("%s\\NVIDIA Corporation\\NVIDIA AR SDK\\"), path);
        SetDllDirectory(fullPath);
        nvCVImageLib = nvLoadLibrary("NVCVImage");
      }
    }
    bSDKPathSet = true;
  }
  return nvCVImageLib;
}
 
NvCV_Status NvCV_API NvCVImage_Init(NvCVImage* im, unsigned width, unsigned height, int pitch, void* pixels,
                                       NvCVImage_PixelFormat format, NvCVImage_ComponentType type, unsigned isPlanar,
                                       unsigned onGPU) {
  static const auto funcPtr = (decltype(NvCVImage_Init)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Init");
  
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, width, height, pitch, pixels, format, type, isPlanar, onGPU);
}

void NvCV_API NvCVImage_InitView(NvCVImage* subImg, NvCVImage* fullImg, int x, int y, unsigned width,
                                   unsigned height) {
  static const auto funcPtr = (decltype(NvCVImage_InitView)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_InitView");
 
  if (nullptr != funcPtr) funcPtr(subImg, fullImg, x, y, width, height);
}

NvCV_Status NvCV_API NvCVImage_Alloc(NvCVImage* im, unsigned width, unsigned height, NvCVImage_PixelFormat format,
                              NvCVImage_ComponentType type, unsigned isPlanar, unsigned onGPU, unsigned alignment) {
  static const auto funcPtr = (decltype(NvCVImage_Alloc)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Alloc");
  
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, width, height, format, type, isPlanar, onGPU, alignment);
}

NvCV_Status NvCV_API NvCVImage_Realloc(NvCVImage* im, unsigned width, unsigned height,
                                          NvCVImage_PixelFormat format, NvCVImage_ComponentType type,
                                          unsigned isPlanar, unsigned onGPU, unsigned alignment) {
  static const auto funcPtr = (decltype(NvCVImage_Realloc)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Realloc");
  
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, width, height, format, type, isPlanar, onGPU, alignment);
}

void NvCV_API NvCVImage_Dealloc(NvCVImage* im) {
  static const auto funcPtr = (decltype(NvCVImage_Dealloc)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Dealloc");

  if (nullptr != funcPtr) funcPtr(im);
}

void NvCV_API NvCVImage_DeallocAsync(NvCVImage* im,  CUstream_st* stream) {
  static const auto funcPtr = (decltype(NvCVImage_DeallocAsync)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_DeallocAsync");

  if (nullptr != funcPtr) funcPtr(im, stream);
}

NvCV_Status NvCV_API NvCVImage_Create(unsigned width, unsigned height, NvCVImage_PixelFormat format,
                                         NvCVImage_ComponentType type, unsigned isPlanar, unsigned onGPU,
                                         unsigned alignment, NvCVImage** out) {
  static const auto funcPtr = (decltype(NvCVImage_Create)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Create");
  
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(width, height, format, type, isPlanar, onGPU, alignment, out);
}

void NvCV_API NvCVImage_Destroy(NvCVImage* im) {
  static const auto funcPtr = (decltype(NvCVImage_Destroy)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Destroy");
  
  if (nullptr != funcPtr) funcPtr(im);
}

void NvCV_API NvCVImage_ComponentOffsets(NvCVImage_PixelFormat format, int* rOff, int* gOff, int* bOff, int* aOff,
                                           int* yOff) {
  static const auto funcPtr =
      (decltype(NvCVImage_ComponentOffsets)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_ComponentOffsets");
  
  if (nullptr != funcPtr) funcPtr(format, rOff, gOff, bOff, aOff, yOff);
}

NvCV_Status NvCV_API NvCVImage_Transfer(const NvCVImage* src, NvCVImage* dst, float scale, CUstream_st* stream,
                                           NvCVImage* tmp) {
  static const auto funcPtr = (decltype(NvCVImage_Transfer)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Transfer");
  
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, dst, scale, stream, tmp);
}

NvCV_Status NvCV_API NvCVImage_TransferRect(const NvCVImage *src, const NvCVRect2i *srcRect, NvCVImage *dst,
  const NvCVPoint2i *dstPt, float scale, struct CUstream_st *stream, NvCVImage *tmp) {
  static const auto funcPtr = (decltype(NvCVImage_TransferRect)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_TransferRect");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, srcRect, dst, dstPt, scale, stream, tmp);
}

NvCV_Status NvCV_API NvCVImage_TransferFromYUV(const void *y, int yPixBytes, int yPitch, const void *u, const void *v,
  int uvPixBytes, int uvPitch, NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType, unsigned yuvColorSpace,
  unsigned yuvMemSpace, NvCVImage *dst, const NvCVRect2i *dstRect, float scale, struct CUstream_st *stream, NvCVImage *tmp) {
  static const auto funcPtr = (decltype(NvCVImage_TransferFromYUV)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_TransferFromYUV");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(y, yPixBytes, yPitch, u, v, uvPixBytes, uvPitch, yuvFormat, yuvType, yuvColorSpace, yuvMemSpace, dst,
    dstRect, scale, stream, tmp);
}

NvCV_Status NvCV_API NvCVImage_TransferToYUV(const NvCVImage *src, const NvCVRect2i *srcRect, 
  const void *y, int yPixBytes, int yPitch, const void *u, const void *v, int uvPixBytes, int uvPitch,
  NvCVImage_PixelFormat yuvFormat, NvCVImage_ComponentType yuvType, unsigned yuvColorSpace, unsigned yuvMemSpace,
  float scale, struct CUstream_st *stream, NvCVImage *tmp) {
  static const auto funcPtr = (decltype(NvCVImage_TransferToYUV)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_TransferToYUV");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, srcRect, y, yPixBytes, yPitch, u, v, uvPixBytes, uvPitch, yuvFormat, yuvType, yuvColorSpace, yuvMemSpace, scale, stream, tmp);
}

NvCV_Status NvCV_API NvCVImage_MapResource(NvCVImage *im, struct CUstream_st *stream) {
  static const auto funcPtr = (decltype(NvCVImage_MapResource)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_MapResource");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, stream);
}

NvCV_Status NvCV_API NvCVImage_UnmapResource(NvCVImage *im, struct CUstream_st *stream) {
  static const auto funcPtr = (decltype(NvCVImage_UnmapResource)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_UnmapResource");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, stream);
}

#if RTX_CAMERA_IMAGE == 0
NvCV_Status NvCV_API NvCVImage_Composite(const NvCVImage* fg, const NvCVImage* bg, const NvCVImage* mat, NvCVImage* dst,
    struct CUstream_st *stream) {
  static const auto funcPtr = (decltype(NvCVImage_Composite)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Composite");
   
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(fg, bg, mat, dst, stream);
}
#else //  RTX_CAMERA_IMAGE == 1
NvCV_Status NvCV_API NvCVImage_Composite(const NvCVImage* fg, const NvCVImage* bg, const NvCVImage* mat, NvCVImage* dst) {
  static const auto funcPtr = (decltype(NvCVImage_Composite)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Composite");
   
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(fg, bg, mat, dst);
}
#endif //  RTX_CAMERA_IMAGE

NvCV_Status NvCV_API NvCVImage_CompositeRect(
      const NvCVImage *fg,  const NvCVPoint2i *fgOrg,
      const NvCVImage *bg,  const NvCVPoint2i *bgOrg,
      const NvCVImage *mat, unsigned mode,
      NvCVImage       *dst, const NvCVPoint2i *dstOrg,
      struct CUstream_st *stream) {
  static const auto funcPtr = (decltype(NvCVImage_CompositeRect)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_CompositeRect");
   
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(fg, fgOrg, bg, bgOrg, mat, mode, dst, dstOrg, stream);
}

#if RTX_CAMERA_IMAGE == 0
NvCV_Status NvCV_API NvCVImage_CompositeOverConstant(const NvCVImage *src, const NvCVImage *mat,
  const void *bgColor, NvCVImage *dst, struct CUstream_st *stream) {
  static const auto funcPtr =
    (decltype(NvCVImage_CompositeOverConstant)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_CompositeOverConstant");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, mat, bgColor, dst, stream);
}
#else // RTX_CAMERA_IMAGE == 1
NvCV_Status NvCV_API NvCVImage_CompositeOverConstant(const NvCVImage *src, const NvCVImage *mat,
                                                     const unsigned char bgColor[3], NvCVImage *dst) {
  static const auto funcPtr =
      (decltype(NvCVImage_CompositeOverConstant)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_CompositeOverConstant");
   
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, mat, bgColor, dst);
}
#endif // RTX_CAMERA_IMAGE


NvCV_Status NvCV_API NvCVImage_FlipY(const NvCVImage *src, NvCVImage *dst) {
  static const auto funcPtr = (decltype(NvCVImage_FlipY)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_FlipY");
   
  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(src, dst);
}

NvCV_Status NvCV_API NvCVImage_Sharpen(float sharpness, const NvCVImage *src, NvCVImage *dst,
    struct CUstream_st *stream, NvCVImage *tmp) {
  static const auto funcPtr = (decltype(NvCVImage_Sharpen)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_Sharpen");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(sharpness, src, dst, stream, tmp);
}

#ifdef _WIN32
__declspec(dllexport) const char* __cdecl
#else
const char*
#endif  // _WIN32 or linux
    NvCV_GetErrorStringFromCode(NvCV_Status code) {
  static const auto funcPtr =
      (decltype(NvCV_GetErrorStringFromCode)*)nvGetProcAddress(getNvCVImageLib(), "NvCV_GetErrorStringFromCode");
  
  if (nullptr == funcPtr) return "Cannot find nvCVImage DLL or its dependencies";
  return funcPtr(code);
}



#ifdef _WIN32 // Direct 3D

NvCV_Status NvCV_API NvCVImage_InitFromD3D11Texture(NvCVImage *im, struct ID3D11Texture2D *tx) {
  static const auto funcPtr = (decltype(NvCVImage_InitFromD3D11Texture)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_InitFromD3D11Texture");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(im, tx);
}

NvCV_Status NvCV_API NvCVImage_ToD3DFormat(NvCVImage_PixelFormat format, NvCVImage_ComponentType type, unsigned layout, DXGI_FORMAT *d3dFormat) {
  static const auto funcPtr = (decltype(NvCVImage_ToD3DFormat)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_ToD3DFormat");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(format, type, layout, d3dFormat);
}

NvCV_Status NvCV_API NvCVImage_FromD3DFormat(DXGI_FORMAT d3dFormat, NvCVImage_PixelFormat *format, NvCVImage_ComponentType *type, unsigned char *layout) {
  static const auto funcPtr = (decltype(NvCVImage_FromD3DFormat)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_FromD3DFormat");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(d3dFormat, format, type, layout);
}

#ifdef __dxgicommon_h__

NvCV_Status NvCV_API NvCVImage_ToD3DColorSpace(unsigned char nvcvColorSpace, DXGI_COLOR_SPACE_TYPE *pD3dColorSpace) {
  static const auto funcPtr = (decltype(NvCVImage_ToD3DColorSpace)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_ToD3DColorSpace");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(nvcvColorSpace, pD3dColorSpace);
}

NvCV_Status NvCV_API NvCVImage_FromD3DColorSpace(DXGI_COLOR_SPACE_TYPE d3dColorSpace, unsigned char *pNvcvColorSpace) {
  static const auto funcPtr = (decltype(NvCVImage_FromD3DColorSpace)*)nvGetProcAddress(getNvCVImageLib(), "NvCVImage_FromD3DColorSpace");

  if (nullptr == funcPtr) return NVCV_ERR_LIBRARY;
  return funcPtr(d3dColorSpace, pNvcvColorSpace);
}

#endif // __dxgicommon_h__

#endif // _WIN32 Direct 3D

#endif // enabling for this file
