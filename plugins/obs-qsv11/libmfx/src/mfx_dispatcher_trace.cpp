/* ****************************************************************************** *\

Copyright (C) 2012-2013 Intel Corporation.  All rights reserved.

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

File Name: mfx_dispatcher_trace.cpp

\* ****************************************************************************** */

#include <mfx_dispatcher.h>
#include "mfx_dispatcher_log.h"
#include <memory>

#include <mfx_library_iterator.h>

// static section of the file
namespace
{

#if defined(WIN64)

const
char cPlatform[] = "x64";

#else // !defined(WIN64)

const
char cPlatform[] = "win32";

#endif // defined(WIN64)

} // namespace

//external exposes
extern mfxStatus DISPATCHER_EXPOSED_PREFIX(MFXInit)(mfxIMPL impl, mfxVersion *pVer, mfxSession *session);
extern mfxStatus DISPATCHER_EXPOSED_PREFIX(MFXClose)(mfxSession session);


class sdk_library: public IMsgHandler
{
    std::string m_libPath;
public:
    sdk_library()
    {
        //library notifier
        DispatchLog::get().AttachSink(DL_SINK_IMsgHandler, this);
    }
    ~sdk_library()
    {
        DispatchLog::get().DetachSink(DL_SINK_IMsgHandler, this);
    }

    virtual void Write(int level, int /*opcode*/, const char * msg, va_list argptr)
    {
        HMODULE libModule;
        if (level == DL_LOADED_LIBRARY)
        {
            char str[32];
            vsprintf_s(str, sizeof(str)/sizeof(str[0]), msg, argptr);
            if (1 == sscanf_s(str, "%p", &libModule))
            {
                char libPath [2048];
                ::GetModuleFileNameA(libModule,libPath, sizeof(libPath)/sizeof(libPath[0]));
                m_libPath = libPath;
            }
        }
    }
    std::string& GetPath(){return m_libPath;}
};

int main(int argc, const char *argv[], bool bUsePrefix)
{
    std::auto_ptr<MFX_DISP_HANDLE> allocatedHandle;
    MFX_DISP_HANDLE *pHandle;
    mfxIMPL impl = MFX_IMPL_AUTO;
    mfxVersion ver = {0, 0};
    int i;

    if ((2 == argc) &&
        ('?' == argv[1][0]))
    {
        printf("USAGE: %s -[s|h] [-v<version>]\n", argv[0]);
        printf("Where:\n");
        printf("    -s  - use MFX_IMPL_SOFTWARE dispatching type (optional).\n");
        printf("    -h  - use MFX_IMPL_HARDWARE dispatching type (optional).\n");
        printf("    -ha - use MFX_IMPL_HARDWARE_ANY dispatching type (optional).\n");
        printf("    -h2 - use MFX_IMPL_HARDWARE2 dispatching type (optional).\n");
        printf("    -h3 - use MFX_IMPL_HARDWARE2 dispatching type (optional).\n");
        printf("    -h4 - use MFX_IMPL_HARDWARE2 dispatching type (optional).\n");
        printf("    -v - use specified API version (optional).\n");
        printf("         <version> can be required in X.Y format\n");
        return -1;
    }

    // parse parameters
    for (i = 1; i < argc; i += 1)
    {
        if ('-' == argv[i][0])
        {
            switch (argv[i][1])
            {
            // use MFX_IMPL_SOFTWARE dispatching type
            case 's':
            case 'S':
                impl = MFX_IMPL_SOFTWARE;
                break;

            // use MFX_IMPL_HARDWARE dispatching type
            case 'h':
            case 'H':
            {
                switch (argv[i][2])
                {
                    case 'a':impl = MFX_IMPL_HARDWARE_ANY;break;
                    case '2':impl = MFX_IMPL_HARDWARE2;break;
                    case '3':impl = MFX_IMPL_HARDWARE3;break;
                    case '4':impl = MFX_IMPL_HARDWARE4;break;
                    default :impl = MFX_IMPL_HARDWARE;break;
                }

                break;
            }
                
            // read specified API version
            case 'v':
            case 'V':
            {
                int x = 2;

                // read major version
                while (('0' <= argv[i][x]) &&
                       ('9' >= argv[i][x]))
                {
                    ver.Major = ver.Major * 10 + (argv[i][x] - '0');
                    x += 1;
                }
                // skip delimiter
                if (argv[i][x])
                {
                    x += 1;
                }
                // read minor version
                while (('0' <= argv[i][x]) &&
                       ('9' >= argv[i][x]))
                {
                    ver.Minor = ver.Minor * 10 + (argv[i][x] - '0');
                    x += 1;
                }
            }
            break;

            default:
                DISPATCHER_LOG_ERROR(("unknown parameter %s. Specify ? flag for help.\n", argv[i]));
                break;
            }
        }
        else
        {
            DISPATCHER_LOG_ERROR(("unknown parameter %s. Specify ? flag for help.\n", argv[i]));
        }
    }

    DISPATCHER_LOG_INFO(("current platform: %s\n", cPlatform));
    DISPATCHER_LOG_INFO(("implementation: %s\n", DispatcherLog_GetMFXImplString(impl).c_str()));

    sdk_library library;

    if (bUsePrefix)
    {
        if (MFX_ERR_NONE == DISPATCHER_EXPOSED_PREFIX(MFXInit)(impl, ver.Version?&ver:NULL, (mfxSession*)&pHandle))
        {
            DISPATCHER_EXPOSED_PREFIX(MFXClose)((mfxSession)pHandle);
        }
    }
    else
    {
        if (MFX_ERR_NONE == MFXInit(impl, ver.Version?&ver:NULL, (mfxSession*)&pHandle))
        {
            MFXClose((mfxSession)pHandle);
        }
    }

    DISPATCHER_LOG_INFO(("DISPRESULT: platform=%-5s impl=%-21s ver=%d.%d : %s\n"
        , cPlatform
        , DispatcherLog_GetMFXImplString(impl).c_str()
        , ver.Major , ver.Minor 
        , library.GetPath().empty()? "NOT FOUND" : library.GetPath().c_str()));

    return 0;

} // int main(int argc, const char *argv[])
