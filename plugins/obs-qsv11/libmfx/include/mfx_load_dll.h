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

#if !defined(__MFX_LOAD_DLL_H)
#define __MFX_LOAD_DLL_H

#include "mfx_dispatcher.h"

namespace MFX
{


    //
    // declare DLL loading routines
    //

    mfxStatus mfx_get_rt_dll_name(wchar_t *pPath, size_t pathSize);
    mfxStatus mfx_get_default_dll_name(wchar_t *pPath, size_t pathSize, eMfxImplType implType);
    mfxStatus mfx_get_default_plugin_name(wchar_t *pPath, size_t pathSize, eMfxImplType implType);
#if defined(MEDIASDK_UWP_DISPATCHER)
    mfxStatus mfx_get_default_intel_gfx_api_dll_name(wchar_t *pPath, size_t pathSize);
#endif

    mfxStatus mfx_get_default_audio_dll_name(wchar_t *pPath, size_t pathSize, eMfxImplType implType);
    

    mfxModuleHandle mfx_dll_load(const wchar_t *file_name);
    //increments reference counter
    mfxModuleHandle mfx_get_dll_handle(const wchar_t *file_name);
    mfxFunctionPointer mfx_dll_get_addr(mfxModuleHandle handle, const char *func_name);
    bool mfx_dll_free(mfxModuleHandle handle);

} // namespace MFX

#endif  // __MFX_LOAD_DLL_H
