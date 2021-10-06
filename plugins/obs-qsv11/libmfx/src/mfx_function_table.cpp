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
    (void) impl;
    (void) ver;
    (void) session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXInit(mfxIMPL impl, mfxVersion *ver, mfxSession *session)

mfxStatus pseudoMFXClose(mfxSession session)
{
    // touch unreferenced parameters
    (void) session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXClose(mfxSession session)

mfxStatus pseudoMFXJoinSession(mfxSession session, mfxSession child_session)
{
    // touch unreferenced parameters
    (void) session;
    (void) child_session;

    return MFX_ERR_UNKNOWN;

} // mfxStatus pseudoMFXJoinSession(mfxSession session, mfxSession child_session)

mfxStatus pseudoMFXCloneSession(mfxSession session, mfxSession *clone)
{
    // touch unreferenced parameters
    (void) session;
    (void) clone;

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
