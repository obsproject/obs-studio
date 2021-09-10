/* SPDX-License-Identifier: MIT */
/**
	@file		mac/pthreadsextra.cpp
	@brief		Declares extra symbols to make the Mac threads implementation look more like Unix.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "pthreadsextra.h"
#include <sys/time.h>  
#include "ajabase/system/mac/threadimpl.h"

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    AJA_UNUSED(clk_id);
    
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tp->tv_sec = tv.tv_sec;
	tp->tv_nsec = tv.tv_usec*1000;
	
	return 0;
	
}

static bool times_up(const struct timespec *timeout)
{
	// get the current time of day
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);	
	
	if(ts.tv_sec >= timeout->tv_sec &&
	   ts.tv_nsec >= timeout->tv_nsec)
	{
		return true;
	}
	else 
	{
		return false;
	}
}

// see: http://lists.apple.com/archives/xcode-users/2007/Apr/msg00331.html
int pthread_mutex_timedlock(pthread_mutex_t *__restrict mutex, const struct timespec *__restrict abs_timeout)
{
	int result;
	do
	{			
		result = pthread_mutex_trylock(mutex);
		if (result == EBUSY)
		{
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 10000000;
			
			/* Sleep for 10,000,000 nanoseconds before trying again. */
			int status = -1;
			while (status == -1)
				status = nanosleep(&ts, &ts);
		}
		else
			break;
		
		if(times_up(abs_timeout))
		{
			result = ETIMEDOUT;
		}		
	}
	while (result != 0);
	
	return result;
}

pid_t gettid(void)
{
	return getpid();
}
