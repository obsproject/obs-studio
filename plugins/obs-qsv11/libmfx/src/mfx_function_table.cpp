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

File Name: mfx_function_table.cpp

\* ****************************************************************************** */

#include "mfx_dispatcher.h"

//
// implement a table with functions names
//

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
    {#func_name, API_VERSION},

const
FUNCTION_DESCRIPTION APIFunc[eVideoFuncTotal] =
{
    {"MFXInit", {{0, 1}}},
    {"MFXClose", {{0, 1}}},
    {"MFXQueryIMPL", {{0, 1}}},
    {"MFXQueryVersion", {{0, 1}}},

    {"MFXJoinSession", {{1, 1}}},
    {"MFXDisjoinSession", {{1, 1}}},
    {"MFXCloneSession", {{1, 1}}},
    {"MFXSetPriority", {{1, 1}}},
    {"MFXGetPriority", {{1, 1}}},
    
    {"MFXInitEx", {{1, 14}}},

#include "mfx_exposed_functions_list.h"
};

const
FUNCTION_DESCRIPTION APIAudioFunc[eAudioFuncTotal] =
{
    {"MFXInit", {{8, 1}}},
    {"MFXClose", {{8, 1}}},
    {"MFXQueryIMPL", {{8, 1}}},
    {"MFXQueryVersion", {{8, 1}}},

    {"MFXJoinSession", {{8, 1}}},
    {"MFXDisjoinSession", {{8, 1}}},
    {"MFXCloneSession", {{8, 1}}},
    {"MFXSetPriority", {{8, 1}}},
    {"MFXGetPriority", {{8, 1}}},

#include "mfxaudio_exposed_functions_list.h"
};

// static section of the file
namespace
{

//
// declare pseudo-functions.
// they are used as default values for call-tables.
//

mfxStatus pseudoMFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)
{
    // touch unreferenced parameters
    impl = impl;
    ver = ver;
    session = session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)

mfxStatus pseudoMFXClose(mfxSession session)
{
    // touch unreferenced parameters
    session = session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXClose(mfxSession session)

mfxStatus pseudoMFXJoinSession(mfxSession session, mfxSession child_session)
{
    // touch unreferenced parameters
    session = session;
    child_session = child_session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXJoinSession(mfxSession session, mfxSession child_session)

mfxStatus pseudoMFXCloneSession(mfxSession session, mfxSession *clone)
{
    // touch unreferenced parameters
    session = session;
    clone = clone;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXCloneSession(mfxSession session, mfxSession *clone)

void SuppressWarnings(...)
{
    // this functions is suppose to suppress warnings.
    // Actually it does nothing.

} // void SuppressWarnings(...)

#undef FUNCTION
#define FUNCTION(return_value, func_name, formal_param_list, actual_param_list) \
return_value pseudo##func_name formal_param_list \
{ \
    SuppressWarnings actual_param_list; \
    return MFX_ERR_UNKNOWN; \
}

#include "mfx_exposed_functions_list.h"

} // namespace
