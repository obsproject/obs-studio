/*
Copyright (c) 2015-2016, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SEG_SERVER_H_
#define SEG_SERVER_H_

#include "Dependencies.h"
#include "SegImage.h"

#define USE_DEFAULT_PROPERTY_VALUE -1

class SegServer
{
public:
    enum ServiceStatus
    {
        COM_LIB_INIT_ERROR = -1,
        SERVICE_INIT_ERROR = -2,
        SERVICE_REINIT_ERROR = -3,
        SERVICE_FUNC_ERROR = -4,
        SHARED_MEMORY_ERROR = -5,
        ALLOCATION_FAILURE = -6,
        SERVICE_NO_ERROR = 0,
		SERVICE_NOT_READY = 1
    };

    static SegServer* CreateServer();
    static SegServer* GetServerInstance();

    virtual ServiceStatus Init() = 0;
    virtual ServiceStatus Stop() = 0;

    virtual ServiceStatus GetFrame(SegImage** image) = 0;

    virtual void SetFps(int fps) = 0;
    virtual int GetFps() = 0;
    /**
    @brief Set the IVCAM motion range trade off option, ranged from 0 (short range, better motion) to 100 (far range, long exposure). Custom property value used only with 60 fps profile
    @param[in] value		The motion range trade option. USE_DEFAULT_PROPERTY_VALUE to return default value
    */
    virtual void SetIVCAMMotionRangeTradeOff(int value) = 0;
    virtual int GetIVCAMMotionRangeTradeOff() = 0;

    virtual ~SegServer() {};
};

#endif // SEG_SERVER_H_
