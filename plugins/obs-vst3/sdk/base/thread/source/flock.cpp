//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/thread/source/flock.cpp
// Created by  : Steinberg, 1995
// Description : Locks
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#define DEBUG_LOCK 0

#include "base/thread/include/flock.h"
#include "base/source/fdebug.h"

//------------------------------------------------------------------------
#if SMTG_OS_WINDOWS
//------------------------------------------------------------------------
#ifndef WINVER
#define WINVER 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#include <windows.h>
#include <objbase.h>
#define INIT_CS(cs) \
	InitializeCriticalSection ((LPCRITICAL_SECTION)&cs);

#endif // SMTG_OS_WINDOWS

namespace Steinberg {
namespace Base {
namespace Thread {


//------------------------------------------------------------------------
//	FLock implementation
//------------------------------------------------------------------------
FLock::FLock (const char8* /*name*/)
{
#if SMTG_PTHREADS
	pthread_mutexattr_t mutexAttr;
	pthread_mutexattr_init (&mutexAttr);
	pthread_mutexattr_settype (&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
	if (pthread_mutex_init (&mutex, &mutexAttr) != 0)
		{SMTG_WARNING("mutex_init failed")}
	pthread_mutexattr_destroy (&mutexAttr);

#elif SMTG_OS_WINDOWS
	INIT_CS (section)
#else
#warning implement FLock!
#endif
}

//------------------------------------------------------------------------
FLock::~FLock ()
{
#if SMTG_PTHREADS
	pthread_mutex_destroy (&mutex);

#elif SMTG_OS_WINDOWS
	DeleteCriticalSection ((LPCRITICAL_SECTION)&section);
		
#endif
}

//------------------------------------------------------------------------
void FLock::lock ()
{
#if DEBUG_LOCK
	FDebugPrint ("FLock::lock () %x\n", this);
#endif

#if SMTG_PTHREADS
	pthread_mutex_lock (&mutex);

#elif SMTG_OS_WINDOWS
	EnterCriticalSection ((LPCRITICAL_SECTION)&section);

#endif
}

//------------------------------------------------------------------------
void FLock::unlock ()
{
#if DEBUG_LOCK
	FDebugPrint ("FLock::unlock () %x\n", this);
#endif
	
#if SMTG_PTHREADS
	pthread_mutex_unlock (&mutex);

#elif SMTG_OS_WINDOWS
	LeaveCriticalSection ((LPCRITICAL_SECTION)&section);

#endif 
}

//------------------------------------------------------------------------
bool FLock::trylock ()
{
#if SMTG_PTHREADS
	return pthread_mutex_trylock (&mutex) == 0;

#elif SMTG_OS_WINDOWS
	return TryEnterCriticalSection ((LPCRITICAL_SECTION)&section) != 0 ? true : false;

#else
	return false;
#endif 
}

//------------------------------------------------------------------------
} // namespace Thread
} // namespace Base
} // namespace Steinberg
