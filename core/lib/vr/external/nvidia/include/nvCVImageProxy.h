#pragma once

#if defined(linux) || defined(unix) || defined(__linux)
#warning nvCVImageProxy.cpp not ported
#else

#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
#else // !_WIN32
#include <dlfcn.h>
  typedef void* HINSTANCE;
#endif // _WIN32


extern HINSTANCE getNvCVImageLib();
extern int nvFreeLibrary(HINSTANCE handle);

#endif