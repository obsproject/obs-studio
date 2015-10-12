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

File Name: mfx_dispatcher_log.h

\* ****************************************************************************** */

#if defined(MFX_DISPATCHER_LOG)

#include "mfx_dispatcher_log.h"
#include "mfxstructures.h"
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#if defined(DISPATCHER_LOG_REGISTER_EVENT_PROVIDER)
#include <evntprov.h>
#include <winmeta.h>
#endif
#endif // #if defined(_WIN32) || defined(_WIN64)
#include <stdarg.h>
#include <algorithm>
#include <string>
#include <sstream>

struct CodeStringTable
{
    int code;
    const char *string;
} LevelStrings []= 
{
    {DL_INFO,  "INFO:   "},
    {DL_WRN,   "WARNING:"},
    {DL_ERROR, "ERROR:  "}
};

#define DEFINE_CODE(code)\
    {code, #code}

static CodeStringTable StringsOfImpl[] = {
    DEFINE_CODE(MFX_IMPL_AUTO),       
    DEFINE_CODE(MFX_IMPL_SOFTWARE),
    DEFINE_CODE(MFX_IMPL_HARDWARE),     
    DEFINE_CODE(MFX_IMPL_AUTO_ANY),     
    DEFINE_CODE(MFX_IMPL_HARDWARE_ANY), 
    DEFINE_CODE(MFX_IMPL_HARDWARE2), 
    DEFINE_CODE(MFX_IMPL_HARDWARE3), 
    DEFINE_CODE(MFX_IMPL_HARDWARE4), 

    DEFINE_CODE(MFX_IMPL_UNSUPPORTED)
};

static CodeStringTable StringsOfImplVIA[] = {
    DEFINE_CODE(MFX_IMPL_VIA_ANY),
    DEFINE_CODE(MFX_IMPL_VIA_D3D9),
    DEFINE_CODE(MFX_IMPL_VIA_D3D11),
};

static CodeStringTable StringsOfStatus[] =
{
    DEFINE_CODE(MFX_ERR_NONE                    ),
    DEFINE_CODE(MFX_ERR_UNKNOWN                 ),
    DEFINE_CODE(MFX_ERR_NULL_PTR                ),
    DEFINE_CODE(MFX_ERR_UNSUPPORTED             ),
    DEFINE_CODE(MFX_ERR_MEMORY_ALLOC            ),
    DEFINE_CODE(MFX_ERR_NOT_ENOUGH_BUFFER       ),
    DEFINE_CODE(MFX_ERR_INVALID_HANDLE          ),
    DEFINE_CODE(MFX_ERR_LOCK_MEMORY             ),
    DEFINE_CODE(MFX_ERR_NOT_INITIALIZED         ),
    DEFINE_CODE(MFX_ERR_NOT_FOUND               ),
    DEFINE_CODE(MFX_ERR_MORE_DATA               ),
    DEFINE_CODE(MFX_ERR_MORE_SURFACE            ),
    DEFINE_CODE(MFX_ERR_ABORTED                 ),
    DEFINE_CODE(MFX_ERR_DEVICE_LOST             ),
    DEFINE_CODE(MFX_ERR_INCOMPATIBLE_VIDEO_PARAM),
    DEFINE_CODE(MFX_ERR_INVALID_VIDEO_PARAM     ),
    DEFINE_CODE(MFX_ERR_UNDEFINED_BEHAVIOR      ),
    DEFINE_CODE(MFX_ERR_DEVICE_FAILED           ),
    DEFINE_CODE(MFX_WRN_IN_EXECUTION            ),
    DEFINE_CODE(MFX_WRN_DEVICE_BUSY             ),
    DEFINE_CODE(MFX_WRN_VIDEO_PARAM_CHANGED     ),
    DEFINE_CODE(MFX_WRN_PARTIAL_ACCELERATION    ),
    DEFINE_CODE(MFX_WRN_INCOMPATIBLE_VIDEO_PARAM),
    DEFINE_CODE(MFX_WRN_VALUE_NOT_CHANGED       ),
    DEFINE_CODE(MFX_WRN_OUT_OF_RANGE            ),
    
};

#define CODE_TO_STRING(code,  array)\
    CodeToString(code, array, sizeof(array)/sizeof(array[0]))

const char* CodeToString(int code, CodeStringTable array[], int len )
{
    for (int i = 0 ; i < len; i++)
    {
        if (array[i].code == code)
            return array[i].string;
    }
    return "undef";
}

std::string DispatcherLog_GetMFXImplString(int impl)
{
    std::string str1 = CODE_TO_STRING(impl & ~(-MFX_IMPL_VIA_ANY), StringsOfImpl);
    std::string str2 = CODE_TO_STRING(impl & (-MFX_IMPL_VIA_ANY), StringsOfImplVIA);

    return str1 + (str2 == "undef" ? "" : "|"+str2);
}

const char *DispatcherLog_GetMFXStatusString(int sts)
{
    return CODE_TO_STRING(sts, StringsOfStatus);
}

//////////////////////////////////////////////////////////////////////////


void DispatcherLogBracketsHelper::Write(const char * str, ...)
{
    va_list argsptr;
    va_start(argsptr, str);
    DispatchLog::get().Write(m_level, m_opcode, str, argsptr);
    va_end(argsptr);
}

void DispatchLogBlockHelper::Write(const char * str, ...)
{
    va_list argsptr;
    va_start(argsptr, str);
    DispatchLog::get().Write(m_level, DL_EVENT_START, str, argsptr);
    va_end(argsptr);
}

DispatchLogBlockHelper::~DispatchLogBlockHelper()
{
    DispatchLog::get().Write(m_level, DL_EVENT_STOP, NULL, NULL);
}

//////////////////////////////////////////////////////////////////////////

DispatchLog::DispatchLog()
 : m_DispatcherLogSink(DL_SINK_PRINTF)
{

}

void   DispatchLog::SetSink(int nSink, IMsgHandler * pHandler)
{
    DetachAllSinks();
    AttachSink(nSink, pHandler);
}

void   DispatchLog::AttachSink(int nsink, IMsgHandler *pHandler)
{
    m_DispatcherLogSink |= nsink;
    if (NULL != pHandler)
        m_Recepients.push_back(pHandler);
}

void   DispatchLog::DetachSink(int nsink, IMsgHandler *pHandler)
{
    if (nsink & DL_SINK_IMsgHandler)
    {
        m_Recepients.remove(pHandler);
    }

    m_DispatcherLogSink &= ~nsink;
}

void   DispatchLog::ExchangeSink(int nsink, IMsgHandler *oldHdl, IMsgHandler *newHdl)
{
    if (nsink & DL_SINK_IMsgHandler)
    {
        std::list<IMsgHandler*> :: iterator it = std::find(m_Recepients.begin(), m_Recepients.end(), oldHdl);
        
        //cannot exchange in that case
        if (m_Recepients.end() == it)
            return;

        *it = newHdl;
    }
}


void   DispatchLog::DetachAllSinks()
{
    m_Recepients.clear();
    m_DispatcherLogSink = DL_SINK_NULL;
}

void   DispatchLog::Write(int level, int opcode, const char * msg, va_list argptr)
{
    int sinkTable[] =
    {
        DL_SINK_PRINTF,
        DL_SINK_IMsgHandler,
    };

    for (size_t i = 0; i < sizeof(sinkTable) / sizeof(sinkTable[0]); i++)
    {
        switch(m_DispatcherLogSink & sinkTable[i])
        {
            case  DL_SINK_NULL:
                break;
            
            case DL_SINK_PRINTF:
            {
                char msg_formated[8048] = {0};

                if (NULL != msg && level != DL_LOADED_LIBRARY)
                {
#if _MSC_VER >= 1400
                    vsprintf_s(msg_formated, sizeof(msg_formated)/sizeof(msg_formated[0]), msg, argptr);
#else
                    vsnprintf(msg_formated, sizeof(msg_formated)/sizeof(msg_formated[0]), msg, argptr);
#endif
                    //TODO: improve this , add opcode handling
                    printf("%s %s", CODE_TO_STRING(level, LevelStrings), msg_formated);
                }
                break;
            }

            case DL_SINK_IMsgHandler:
            {
                std::list<IMsgHandler*>::iterator it;

                for (it = m_Recepients.begin(); it != m_Recepients.end(); ++it)
                {
                    (*it)->Write(level, opcode, msg, argptr);
                }
                break;
            }
        }
    }
}

#if defined(DISPATCHER_LOG_REGISTER_EVENT_PROVIDER)
class ETWHandler : public IMsgHandler
{
public:
    ETWHandler(const wchar_t * guid_str)
      : m_bUseFormatter(DISPATCHER_LOG_USE_FORMATING)
      , m_EventHandle()
      , m_bProviderEnable()
    {
        GUID rguid = GUID_NULL;
        if (FAILED(CLSIDFromString(guid_str, &rguid)))
        {
            return;
        }
        
        EventRegister(&rguid, NULL, NULL, &m_EventHandle);

        m_bProviderEnable = 0 != EventProviderEnabled(m_EventHandle, 1,0);
    }

    ~ETWHandler()
    {
        if (m_EventHandle)
        {
            EventUnregister(m_EventHandle);
        }
    }

    virtual void Write(int level, int opcode, const char * msg, va_list argptr)
    {
        //event not registered
        if (0==m_EventHandle)
        {
            return;
        }
        if (!m_bProviderEnable)
        {
            return;
        }
        if (level == DL_LOADED_LIBRARY)
        {
            return;
        }

        char msg_formated[1024];
        EVENT_DESCRIPTOR descriptor;
        EVENT_DATA_DESCRIPTOR data_descriptor;

        EventDescZero(&descriptor);
        
        descriptor.Opcode = (UCHAR)opcode; 
        descriptor.Level  = (UCHAR)level;
        
        if (m_bUseFormatter)
        {
            if (NULL != msg)
            {
#if _MSC_VER >= 1400
                vsprintf_s(msg_formated, sizeof (msg_formated) / sizeof (msg_formated[0]), msg, argptr);
#else
                vsnprintf(msg_formated, sizeof (msg_formated) / sizeof (msg_formated[0]), msg, argptr);
#endif
                EventDataDescCreate(&data_descriptor, msg_formated, (ULONG)(strlen(msg_formated) + 1));
            }else
            {
                EventDataDescCreate(&data_descriptor, NULL, 0);
            }
        }else
        {
            //TODO: non formated events supports under zbb 
        }

        EventWrite(m_EventHandle, &descriptor, 1, &data_descriptor);
    }

protected:

    //we may not use formatter in some cases described in dispatch_log macro
    //it significantly increases performance by eliminating any vsprintf operations
    bool      m_bUseFormatter;
    //consumer is attached, dispatcher trace to reduce formating overhead 
    //submits event only if consumer attached
    bool      m_bProviderEnable;
    REGHANDLE m_EventHandle;
};
//


IMsgHandler *ETWHandlerFactory::GetSink(const wchar_t* sguid)
{
    _storage_type::iterator it;
    it = m_storage.find(sguid);
    if (it == m_storage.end())
    {
        ETWHandler * handler = new ETWHandler(sguid);
        _storage_type::_Pairib it_bool = m_storage.insert(_storage_type::value_type(sguid, handler));
        it = it_bool.first;
    }

   return it->second;
}

ETWHandlerFactory::~ETWHandlerFactory()
{
    for each(_storage_type::value_type val in m_storage)
    {
        delete val.second;
    }
}

class EventRegistrator : public IMsgHandler
{
    const wchar_t * m_sguid;
public:
    EventRegistrator(const wchar_t* sguid = DISPATCHER_LOG_EVENT_GUID)
        :m_sguid(sguid)
    {
        DispatchLog::get().AttachSink( DL_SINK_IMsgHandler
                                      , this);
    }

    virtual void Write(int level, int opcode, const char * msg, va_list argptr)
    {
        //we cannot call attach sink since we may have been called from iteration
        //we axchanging preserve that placeholding
        IMsgHandler * pSink = NULL;
        DispatchLog::get().ExchangeSink(DL_SINK_IMsgHandler,
                                        this,
                                        pSink = ETWHandlerFactory::get().GetSink(m_sguid));
        //need to call only once here all next calls will be done inside dispatcherlog
        if (NULL != pSink)
        {
            pSink->Write(level, opcode, msg, argptr);
        }
    }
};
#endif

template <class TSink>
class SinkRegistrator
{
};

#if defined(DISPATCHER_LOG_REGISTER_EVENT_PROVIDER)
template <>
class SinkRegistrator<ETWHandlerFactory>
{
public:
    SinkRegistrator(const wchar_t* sguid = DISPATCHER_LOG_EVENT_GUID)
    {
        DispatchLog::get().AttachSink( DL_SINK_IMsgHandler
                                      , ETWHandlerFactory::get().GetSink(sguid));
    }
};
#endif

#if defined(DISPATCHER_LOG_REGISTER_FILE_WRITER)
template <>
class SinkRegistrator<FileSink>
{
public:
    SinkRegistrator()
    {
        DispatchLog::get().AttachSink( DL_SINK_IMsgHandler, &FileSink::get(DISPACTHER_LOG_FW_PATH));
    }
};

void FileSink::Write(int level, int /*opcode*/, const char * msg, va_list argptr)
{
    if (NULL != m_hdl && NULL != msg)
    {
        fprintf(m_hdl, "%s", CODE_TO_STRING(level, LevelStrings));
        vfprintf(m_hdl, msg, argptr);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
//singletons initialization section


#ifdef  DISPATCHER_LOG_REGISTER_EVENT_PROVIDER
    static SinkRegistrator<ETWHandlerFactory> g_registrator1;
#endif


#ifdef DISPATCHER_LOG_REGISTER_FILE_WRITER
    static SinkRegistrator<FileSink> g_registrator2;
#endif


#endif//(MFX_DISPATCHER_LOG)