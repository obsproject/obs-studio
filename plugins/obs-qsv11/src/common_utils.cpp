/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "common_utils.h"

#if defined(_WIN32) || defined(_WIN64)
#include <new>

MSDKMutex::MSDKMutex(void)
{
	InitializeCriticalSection(&m_CritSec);
}

MSDKMutex::~MSDKMutex(void)
{
	DeleteCriticalSection(&m_CritSec);
}

mfxStatus MSDKMutex::Lock(void)
{
	EnterCriticalSection(&m_CritSec);
	return MFX_ERR_NONE;
}

mfxStatus MSDKMutex::Unlock(void)
{
	LeaveCriticalSection(&m_CritSec);
	return MFX_ERR_NONE;
}

int MSDKMutex::Try(void)
{
	return TryEnterCriticalSection(&m_CritSec);
}
#else

#include <new> // std::bad_alloc
#include <stdio.h> // setrlimit
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "vm/thread_defs.h"
#include "sample_utils.h"

MSDKMutex::MSDKMutex(void)
{
	int res = pthread_mutex_init(&m_mutex, NULL);
	if (res) throw std::bad_alloc();
}

MSDKMutex::~MSDKMutex(void)
{
	pthread_mutex_destroy(&m_mutex);
}

mfxStatus MSDKMutex::Lock(void)
{
	return (pthread_mutex_lock(&m_mutex)) ? MFX_ERR_UNKNOWN : MFX_ERR_NONE;
}

mfxStatus MSDKMutex::Unlock(void)
{
	return (pthread_mutex_unlock(&m_mutex)) ? MFX_ERR_UNKNOWN : MFX_ERR_NONE;
}

int MSDKMutex::Try(void)
{
	return (pthread_mutex_trylock(&m_mutex)) ? 0 : 1;
}
#endif

AutomaticMutex::AutomaticMutex(MSDKMutex& mutex) :
	m_rMutex(mutex),
	m_bLocked(false)
{
	if (MFX_ERR_NONE != Lock()) throw std::bad_alloc();
};
AutomaticMutex::~AutomaticMutex(void)
{
	Unlock();
}

mfxStatus AutomaticMutex::Lock(void)
{
	mfxStatus sts = MFX_ERR_NONE;
	if (!m_bLocked)
	{
		if (!m_rMutex.Try())
		{
			// add time measurement here to estimate how long you sleep on mutex...
			sts = m_rMutex.Lock();
		}
		m_bLocked = true;
	}
	return sts;
}

mfxStatus AutomaticMutex::Unlock(void)
{
	mfxStatus sts = MFX_ERR_NONE;
	if (m_bLocked)
	{
		sts = m_rMutex.Unlock();
		m_bLocked = false;
	}
	return sts;
}

mfxU16 GetFreeSurfaceIndex(mfxFrameSurface1* pSurfacesPool, mfxU16 nPoolSize)
{
	if (pSurfacesPool)
	{
		for (mfxU16 i = 0; i < nPoolSize; i++)
		{
			if (0 == pSurfacesPool[i].Data.Locked)
			{
				return i;
			}
		}
	}

	return MSDK_INVALID_SURF_IDX;
}

msdk_string StatusToString(mfxStatus sts)
{
	switch (sts)
	{
	case MFX_ERR_NONE:
		return msdk_string(MSDK_STRING("MFX_ERR_NONE"));
	case MFX_ERR_UNKNOWN:
		return msdk_string(MSDK_STRING("MFX_ERR_UNKNOWN"));
	case MFX_ERR_NULL_PTR:
		return msdk_string(MSDK_STRING("MFX_ERR_NULL_PTR"));
	case MFX_ERR_UNSUPPORTED:
		return msdk_string(MSDK_STRING("MFX_ERR_UNSUPPORTED"));
	case MFX_ERR_MEMORY_ALLOC:
		return msdk_string(MSDK_STRING("MFX_ERR_MEMORY_ALLOC"));
	case MFX_ERR_NOT_ENOUGH_BUFFER:
		return msdk_string(MSDK_STRING("MFX_ERR_NOT_ENOUGH_BUFFER"));
	case MFX_ERR_INVALID_HANDLE:
		return msdk_string(MSDK_STRING("MFX_ERR_INVALID_HANDLE"));
	case MFX_ERR_LOCK_MEMORY:
		return msdk_string(MSDK_STRING("MFX_ERR_LOCK_MEMORY"));
	case MFX_ERR_NOT_INITIALIZED:
		return msdk_string(MSDK_STRING("MFX_ERR_NOT_INITIALIZED"));
	case MFX_ERR_NOT_FOUND:
		return msdk_string(MSDK_STRING("MFX_ERR_NOT_FOUND"));
	case MFX_ERR_MORE_DATA:
		return msdk_string(MSDK_STRING("MFX_ERR_MORE_DATA"));
	case MFX_ERR_MORE_SURFACE:
		return msdk_string(MSDK_STRING("MFX_ERR_MORE_SURFACE"));
	case MFX_ERR_ABORTED:
		return msdk_string(MSDK_STRING("MFX_ERR_ABORTED"));
	case MFX_ERR_DEVICE_LOST:
		return msdk_string(MSDK_STRING("MFX_ERR_DEVICE_LOST"));
	case MFX_ERR_INCOMPATIBLE_VIDEO_PARAM:
		return msdk_string(MSDK_STRING("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM"));
	case MFX_ERR_INVALID_VIDEO_PARAM:
		return msdk_string(MSDK_STRING("MFX_ERR_INVALID_VIDEO_PARAM"));
	case MFX_ERR_UNDEFINED_BEHAVIOR:
		return msdk_string(MSDK_STRING("MFX_ERR_UNDEFINED_BEHAVIOR"));
	case MFX_ERR_DEVICE_FAILED:
		return msdk_string(MSDK_STRING("MFX_ERR_DEVICE_FAILED"));
	case MFX_ERR_MORE_BITSTREAM:
		return msdk_string(MSDK_STRING("MFX_ERR_MORE_BITSTREAM"));
	case MFX_ERR_INCOMPATIBLE_AUDIO_PARAM:
		return msdk_string(MSDK_STRING("MFX_ERR_INCOMPATIBLE_AUDIO_PARAM"));
	case MFX_ERR_INVALID_AUDIO_PARAM:
		return msdk_string(MSDK_STRING("MFX_ERR_INVALID_AUDIO_PARAM"));
	case MFX_ERR_GPU_HANG:
		return msdk_string(MSDK_STRING("MFX_ERR_GPU_HANG"));
	case MFX_ERR_REALLOC_SURFACE:
		return msdk_string(MSDK_STRING("MFX_ERR_REALLOC_SURFACE"));
	case MFX_WRN_IN_EXECUTION:
		return msdk_string(MSDK_STRING("MFX_WRN_IN_EXECUTION"));
	case MFX_WRN_DEVICE_BUSY:
		return msdk_string(MSDK_STRING("MFX_WRN_DEVICE_BUSY"));
	case MFX_WRN_VIDEO_PARAM_CHANGED:
		return msdk_string(MSDK_STRING("MFX_WRN_VIDEO_PARAM_CHANGED"));
	case MFX_WRN_PARTIAL_ACCELERATION:
		return msdk_string(MSDK_STRING("MFX_WRN_PARTIAL_ACCELERATION"));
	case MFX_WRN_INCOMPATIBLE_VIDEO_PARAM:
		return msdk_string(MSDK_STRING("MFX_WRN_INCOMPATIBLE_VIDEO_PARAM"));
	case MFX_WRN_VALUE_NOT_CHANGED:
		return msdk_string(MSDK_STRING("MFX_WRN_VALUE_NOT_CHANGED"));
	case MFX_WRN_OUT_OF_RANGE:
		return msdk_string(MSDK_STRING("MFX_WRN_OUT_OF_RANGE"));
	case MFX_WRN_FILTER_SKIPPED:
		return msdk_string(MSDK_STRING("MFX_WRN_FILTER_SKIPPED"));
	case MFX_WRN_INCOMPATIBLE_AUDIO_PARAM:
		return msdk_string(MSDK_STRING("MFX_WRN_INCOMPATIBLE_AUDIO_PARAM"));
	case MFX_TASK_WORKING:
		return msdk_string(MSDK_STRING("MFX_TASK_WORKING"));
	case MFX_TASK_BUSY:
		return msdk_string(MSDK_STRING("MFX_TASK_BUSY"));
	case MFX_ERR_MORE_DATA_SUBMIT_TASK:
		return msdk_string(MSDK_STRING("MFX_ERR_MORE_DATA_SUBMIT_TASK"));
	default:
		return msdk_string(MSDK_STRING("[Unknown status]"));
	}
}