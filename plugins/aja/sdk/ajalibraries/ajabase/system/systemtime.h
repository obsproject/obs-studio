/* SPDX-License-Identifier: MIT */
/**
	@file		systemtime.h
	@brief		Declares the AJATime class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIME_H
#define AJA_TIME_H

#include "ajabase/common/public.h"

/** 
	@brief		Collection of platform-independent host system clock time functions.
	@ingroup	AJAGroupSystem
**/
class AJA_EXPORT AJATime
{
public:

	/**
		@brief		Returns the current value of the host system's low-resolution clock, in milliseconds.
		@return		The current value of the host system's low-resolution clock, in milliseconds.
	**/
	static int64_t	GetSystemTime (void);

	/**
		@brief		Returns the current value of the host system's high-resolution time counter.
		@return		The current value of the host system's high-resolution time counter.
	**/
	static int64_t	GetSystemCounter (void);

	/**
		@brief		Returns the frequency of the host system's high-resolution time counter.
		@return		The high resolution counter frequency in ticks per second.
	**/
	static int64_t	GetSystemFrequency (void);

	/**
		@brief		Returns the current value of the host's high-resolution clock, in milliseconds.
		@return		Current value of the host's clock, in milliseconds, based on GetSystemCounter() and GetSystemFrequency().
	**/
	static uint64_t	GetSystemMilliseconds (void);

	/**
		@brief		Returns the current value of the host's high-resolution clock, in microseconds.
		@return		Current value of the host's clock, in microseconds, based on GetSystemCounter() and GetSystemFrequency().
	**/
	static uint64_t	GetSystemMicroseconds (void);

    /**
        @brief		Returns the current value of the host's high-resolution clock, in nanoseconds.
        @return		Current value of the host's clock, in nanoseconds, based on GetSystemCounter() and GetSystemFrequency().
    **/
    static uint64_t	GetSystemNanoseconds (void);

	/**
		@brief		Suspends execution of the current thread for a given number of milliseconds.
		@param		inMilliseconds		Specifies the sleep time, in milliseconds.
	**/
	static void		Sleep (const int32_t inMilliseconds);

	/**
		@brief		Suspends execution of the current thread for a given number of microseconds.
		@param		inMicroseconds		Time to sleep (microseconds).
	**/
	static void		SleepInMicroseconds (const int32_t inMicroseconds);

};	//	AJATime

#endif	//	AJA_TIME_H
