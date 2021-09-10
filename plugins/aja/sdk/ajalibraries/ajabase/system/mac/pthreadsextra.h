/* SPDX-License-Identifier: MIT */
/**
	@file		mac/pthreadsextra.h
	@brief		Declares extra symbols to make the Mac threads implementation look more like Unix.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PTREAD_EXTRA_H
#define AJA_PTREAD_EXTRA_H

#include <pthread.h>
#include <sys/types.h>

// defined in later XCode header

#ifndef CLOCK_REALTIME

typedef int 	clockid_t; 

/* Identifier for system-wide realtime clock.  */
#   define CLOCK_REALTIME		0
/* Monotonic system-wide clock.  */
#   define CLOCK_MONOTONIC		1
/* High-resolution timer from the CPU.  */
#   define CLOCK_PROCESS_CPUTIME_ID	2
/* Thread-specific CPU-time clock.  */
#   define CLOCK_THREAD_CPUTIME_ID	3
/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
#   define CLOCK_MONOTONIC_RAW		4
/* Identifier for system-wide realtime clock, updated only on ticks.  */
#   define CLOCK_REALTIME_COARSE	5
/* Monotonic system-wide clock, updated only on ticks.  */
#   define CLOCK_MONOTONIC_COARSE	6

#endif

//ignore the clockid_t in this implementation
int clock_gettime(clockid_t clk_id,struct timespec *tp);
int pthread_mutex_timedlock(pthread_mutex_t *__restrict mutex, const struct timespec *__restrict abs_timeout);
pid_t gettid(void);

#endif	//	AJA_PTREAD_EXTRA_H
