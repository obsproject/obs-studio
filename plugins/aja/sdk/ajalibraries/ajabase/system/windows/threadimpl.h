/* SPDX-License-Identifier: MIT */
/**
	@file		windows/threadimpl.h
	@brief		Declares the AJAThreadImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_THREAD_IMPL_H
#define AJA_THREAD_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/thread.h"
#include "ajabase/system/lock.h"


class AJAThreadImpl
{
public:

	AJAThreadImpl(AJAThread* pThread);
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
    AJAStatus       SetThreadName(const char *name);

    static uint64_t	GetThreadId();
	static DWORD WINAPI ThreadProcStatic(void* pThreadImplContext);

	AJAThread* mpThread;
	HANDLE mhThreadHandle;
	DWORD mThreadID;
	AJAThreadPriority mPriority;
	AJAThreadFunction* mThreadFunc;
	void* mpUserContext;
	AJALock mLock;
	bool mTerminate;
};

#endif	//	AJA_THREAD_IMPL_H
