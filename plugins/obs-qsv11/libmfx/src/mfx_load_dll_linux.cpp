// Copyright (c) 2012-2019 Intel Corporation
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

#if !defined(_WIN32) && !defined(_WIN64)

#include "mfx_dispatcher.h"
#include <dlfcn.h>
#include <string.h>

#if defined(LINUX64)
const msdk_disp_char * defaultDLLName[2] = {"libmfxhw64.so",
                                            "libmfxsw64.so"};
const msdk_disp_char * defaultAudioDLLName[2] = {"libmfxaudiosw64.so",
                                                 "libmfxaudiosw64.so"};

const msdk_disp_char * defaultPluginDLLName[2] = {"libmfxplugin64_hw.so",
                                                 "libmfxplugin64_sw.so"};


#elif defined(__APPLE__)
#ifdef __i386__
const msdk_disp_char * defaultDLLName[2] = {"libmfxhw32.dylib",
                                            "libmfxsw32.dylib"};
const msdk_disp_char * defaultAudioDLLName[2] = {"libmfxaudiosw32.dylib",
                                            "libmfxaudiosw32.dylib"};

const msdk_disp_char * defaultPluginDLLName[2] = {"libmfxplugin32_hw.dylib",
                                                  "libmfxplugin32_sw.dylib"};
#else
const msdk_disp_char * defaultDLLName[2] = {"libmfxhw64.dylib",
                                            "libmfxsw64.dylib"};
const msdk_disp_char * defaultAudioDLLName[2] = {"libmfxaudiosw64.dylib",
                                            "libmfxaudiosw64.dylib"};

const msdk_disp_char * defaultPluginDLLName[2] = {"libmfxplugin64_hw.dylib",
                                                  "libmfxplugin64_sw.dylib"};

#endif // #ifdef __i386__ for __APPLE__

#else // for Linux32 and Android
const msdk_disp_char * defaultDLLName[2] = {"libmfxhw32.so",
                                            "libmfxsw32.so"};
const msdk_disp_char * defaultAudioDLLName[2] = {"libmfxaudiosw32.so",
                                            "libmfxaudiosw32.so"};

const msdk_disp_char * defaultPluginDLLName[2] = {"libmfxplugin32_hw.so",
                                                  "libmfxplugin32_sw.so"};
#endif // defined(LINUX64)

namespace MFX
{

mfxStatus mfx_get_default_dll_name(msdk_disp_char *pPath, size_t /*pathSize*/, eMfxImplType implType)
{
    strcpy(pPath, defaultDLLName[implType & 1]);

    return MFX_ERR_NONE;

} // mfxStatus GetDefaultDLLName(wchar_t *pPath, size_t pathSize, eMfxImplType implType)


mfxStatus mfx_get_default_plugin_name(msdk_disp_char *pPath, size_t pathSize, eMfxImplType implType)
{
    strcpy(pPath, defaultPluginDLLName[implType & 1]);

    return MFX_ERR_NONE;
}


mfxStatus mfx_get_default_audio_dll_name(msdk_disp_char *pPath, size_t /*pathSize*/, eMfxImplType implType)
{
    strcpy(pPath, defaultAudioDLLName[implType & 1]);

    return MFX_ERR_NONE;

} // mfxStatus GetDefaultAudioDLLName(wchar_t *pPath, size_t pathSize, eMfxImplType implType)

mfxModuleHandle mfx_dll_load(const msdk_disp_char *pFileName)
{
    mfxModuleHandle hModule = (mfxModuleHandle) 0;

    // check error(s)
    if (NULL == pFileName)
    {
        return NULL;
    }
    // load the module
    hModule = dlopen(pFileName, RTLD_LOCAL|RTLD_NOW);

    return hModule;
} // mfxModuleHandle mfx_dll_load(const wchar_t *pFileName)

mfxFunctionPointer mfx_dll_get_addr(mfxModuleHandle handle, const char *pFunctionName)
{
    if (NULL == handle)
    {
        return NULL;
    }

    mfxFunctionPointer addr = (mfxFunctionPointer) dlsym(handle, pFunctionName);
    if (!addr)
    {
        return NULL;
    }

    return addr;
} // mfxFunctionPointer mfx_dll_get_addr(mfxModuleHandle handle, const char *pFunctionName)

bool mfx_dll_free(mfxModuleHandle handle)
{
    if (NULL == handle)
    {
        return true;
    }
    dlclose(handle);

    return true;
} // bool mfx_dll_free(mfxModuleHandle handle)

mfxModuleHandle mfx_get_dll_handle(const msdk_disp_char *pFileName) {
    return mfx_dll_load(pFileName);
}

} // namespace MFX

#endif // #if !defined(_WIN32) && !defined(_WIN64)
