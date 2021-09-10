/* SPDX-License-Identifier: MIT */
/**
	@file		mac/threadimpl.h
	@brief		Declares the AJAThreadImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_THREAD_IMPL_H
#define AJA_THREAD_IMPL_H

#include <pthread.h>
#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/lock.h"


class AJAThreadImpl
{
public:

	AJAThreadImpl(AJAThread* pThreadContext);
	virtual ~AJAThreadImpl();

    AJAStatus       Start();
    AJAStatus       Stop(uint32_t timeout = 0xffffffff);

    AJAStatus       Kill(uint32_t exitCode);

    bool            Active();
    bool            IsCurrentThread();

    AJAStatus       SetPriority(AJAThreadPriority threadPriority);
    AJAStatus       GetPriority(AJAThreadPriority* pThreadPriority);

    AJAStatus       SetRealTime(AJAThreadRealTimePolicy policy, int priority);

    AJAStatus       Attach(AJAThreadFunction* pThreadFunction, void* pUserContext);

    static uint64_t	GetThreadId();
    static void*    ThreadProcStatic(void* pThreadImplContext);
    AJAStatus       SetThreadName(const char *name);

	AJAThread* mpThreadContext;
	pthread_t mThread;
	AJAThreadPriority mPriority;
	AJAThreadFunction* mThreadFunc;
	void* mpUserContext;
	AJALock mLock;
	bool mTerminate;
    bool mExiting;
	pthread_mutex_t mExitMutex;
	pthread_cond_t mExitCond;
};

#endif	//	AJA_THREAD_IMPL_H
