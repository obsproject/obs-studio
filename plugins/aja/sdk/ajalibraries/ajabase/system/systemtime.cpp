/* SPDX-License-Identifier: MIT */
/**
	@file		systemtime.cpp
	@brief		Implements the AJATime class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/systemtime.h"

#if defined(AJA_MAC)
	#include <mach/mach_time.h>
	#include <CoreServices/CoreServices.h>
	static int64_t s_PerformanceFrequency;
	static bool s_bPerformanceInit = false;
#endif


#if defined(AJA_WINDOWS)
    #include "timeapi.h"
	static LARGE_INTEGER s_PerformanceFrequency;
	static bool s_bPerformanceInit = false;
#endif

#if defined(AJA_LINUX)
	#include <unistd.h>
	#if (_POSIX_TIMERS > 0)
		#ifdef _POSIX_MONOTONIC_CLOCK
			#define AJA_USE_CLOCK_GETTIME
		#else
			#undef AJA_USE_CLOCK_GETTIME
		#endif
	#endif
	
	#ifdef AJA_USE_CLOCK_GETTIME
		#include <time.h>
	#else
		// Use gettimeofday - this is not really desirable
		#include <sys/time.h>
	#endif
#endif


int64_t 
AJATime::GetSystemTime()
{
	// system dependent time function
#if defined(AJA_WINDOWS)
	return (int64_t)::timeGetTime();
#endif

#if defined(AJA_MAC)
    static mach_timebase_info_data_t    sTimebaseInfo;
	uint64_t ticks = mach_absolute_time();
    
    if ( sTimebaseInfo.denom == 0 )
    {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    
    // Do the maths. We hope that the multiplication doesn't
    // overflow; the price you pay for working in fixed point.
    int64_t nanoSeconds = ticks * sTimebaseInfo.numer / sTimebaseInfo.denom;
    
    return nanoSeconds;
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * ((int64_t)1000)) + (ts.tv_nsec / (int64_t)1000000);
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday( &tv, &tz );
	return (int64_t)((tv.tv_sec * ((int64_t)1000)) + (int64_t)(tv.tv_usec / 1000));
#endif
#endif
}


int64_t 
AJATime::GetSystemCounter()
{
#if defined(AJA_WINDOWS)
	LARGE_INTEGER performanceCounter;

	performanceCounter.QuadPart = 0;
	if (!QueryPerformanceCounter(&performanceCounter))
	{
		return 0;
	}

	return (int64_t)performanceCounter.QuadPart;
#endif

#if defined(AJA_MAC)
	return (int64_t) mach_absolute_time();
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * ((int64_t)1000000000)) + (ts.tv_nsec);
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday( &tv, &tz );
	return (int64_t)((int64_t)tv.tv_sec * (int64_t)1000000 + tv.tv_usec);
#endif
#endif
}


int64_t 
AJATime::GetSystemFrequency()
{
#if defined(AJA_WINDOWS)
	if (!s_bPerformanceInit)
	{
		QueryPerformanceFrequency(&s_PerformanceFrequency);
		s_bPerformanceInit = true;
	}

	return (int64_t)s_PerformanceFrequency.QuadPart;
#endif

#if defined(AJA_MAC)
	if (!s_bPerformanceInit)
	{
		// 1 billion ticks approximately equals 1 sec on a Mac
        static mach_timebase_info_data_t    sTimebaseInfo;
        uint64_t ticks = 1000000000;
        
        if ( sTimebaseInfo.denom == 0 )
        {
            (void) mach_timebase_info(&sTimebaseInfo);
        }
        
        // Do the maths. We hope that the multiplication doesn't
        // overflow; the price you pay for working in fixed point.
        int64_t nanoSeconds = ticks * sTimebaseInfo.numer / sTimebaseInfo.denom;
       
		// system frequency - ticks per second units
		s_PerformanceFrequency = ticks * 1000000000 / nanoSeconds;	
		s_bPerformanceInit = true;
	}
	
	return s_PerformanceFrequency;
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
    return 1000000000;
#else
    return 1000000;
#endif
#endif
}

uint64_t 
AJATime::GetSystemMilliseconds()
{				
	uint64_t ticks          = GetSystemCounter();
	uint64_t ticksPerSecond = GetSystemFrequency();
	uint64_t ms             = 0;
	if (ticksPerSecond)
	{
		// floats are being used here to avoid the issue of overflow
		// or inaccuracy when chosing where to apply the '1000' correction
		ms = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000.);
	}
	return ms;
}

uint64_t 
AJATime::GetSystemMicroseconds()
{
	uint64_t ticks          = GetSystemCounter();
	uint64_t ticksPerSecond = GetSystemFrequency();
	uint64_t us             = 0;
	if (ticksPerSecond)
	{
		// floats are being used here to avoid the issue of overflow
		// or inaccuracy when chosing where to apply the '1000000' correction
        us = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000000.);
	}
	return us;
}

uint64_t
AJATime::GetSystemNanoseconds()
{
    uint64_t ticks          = GetSystemCounter();
    uint64_t ticksPerSecond = GetSystemFrequency();
    uint64_t us             = 0;
    if (ticksPerSecond)
    {
        // floats are being used here to avoid the issue of overflow
        // or inaccuracy when chosing where to apply the '1000000000' correction
        us = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000000000.);
    }
    return us;
}

// sleep time in milliseconds
void 
AJATime::Sleep(int32_t interval)
{
	if(interval < 0)
	{
		interval = 0;
	}

	// system dependent sleep function
#if defined(AJA_WINDOWS)
	::Sleep((DWORD)interval);
#endif

#if defined(AJA_MAC)
	usleep(interval * 1000);
#endif

#if defined(AJA_LINUX)
	usleep(interval * 1000);  // milliseconds to microseconds
#endif
}

// sleep time in microseconds
void 
AJATime::SleepInMicroseconds(int32_t interval)
{
	if(interval < 0)
	{
		interval = 0;
	}

	// system dependent sleep function
#if defined(AJA_WINDOWS)
	::Sleep((DWORD)interval / 1000);
#endif

#if defined(AJA_MAC)
	usleep(interval);
#endif

#if defined(AJA_LINUX)
	usleep(interval);  
#endif
}
