/* ****************************************************************************** *\

Copyright (C) 2012-2014 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: mfx_load_dll.h

\* ****************************************************************************** */

#if !defined(__MFX_LOAD_DLL_H)
#define __MFX_LOAD_DLL_H

#include "mfx_dispatcher.h"

namespace MFX
{


    //
    // declare DLL loading routines
    //

    mfxStatus mfx_get_rt_dll_name(msdk_disp_char *pPath, size_t pathSize);
    mfxStatus mfx_get_default_dll_name(msdk_disp_char *pPath, size_t pathSize, eMfxImplType implType);
    mfxStatus mfx_get_default_plugin_name(msdk_disp_char *pPath, size_t pathSize, eMfxImplType implType);

    mfxStatus mfx_get_default_audio_dll_name(msdk_disp_char *pPath, size_t pathSize, eMfxImplType implType);
    

    mfxModuleHandle mfx_dll_load(const msdk_disp_char *file_name);
    //increments reference counter
    mfxModuleHandle mfx_get_dll_handle(const msdk_disp_char *file_name);
    mfxFunctionPointer mfx_dll_get_addr(mfxModuleHandle handle, const char *func_name);
    bool mfx_dll_free(mfxModuleHandle handle);

} // namespace MFX

#endif  // __MFX_LOAD_DLL_H
