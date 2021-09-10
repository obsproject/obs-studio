/* SPDX-License-Identifier: MIT */
/**
	@file		windows/threadimpl.cpp
	@brief		Implements the AJAThreadImpl class for the Windows platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/windows/threadimpl.h"
#include "ajabase/common/timer.h"


AJAThreadImpl::AJAThreadImpl(AJAThread* pThread)
{
	mpThread = pThread;
	mhThreadHandle = 0;
	mThreadID = 0;
	mPriority = AJA_ThreadPriority_Normal;
	mThreadFunc = NULL;
	mpUserContext = NULL;
	mTerminate = false;
}


AJAThreadImpl::~AJAThreadImpl()
{
	Stop();
}


AJAStatus
AJAThreadImpl::Start()
{
	AJAAutoLock lock(&mLock);

	// return success if thread is already running
	if (Active())
	{
		return AJA_STATUS_SUCCESS;
	}

	// create the thread
	mTerminate = false;
	mhThreadHandle = CreateThread(NULL, 0, ThreadProcStatic, this, 0, &mThreadID);
	if (mhThreadHandle == 0)
	{
		mThreadID = 0;
		return AJA_STATUS_FAIL;
	}

	// set the thread priority
	SetPriority(mPriority);

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJAThreadImpl::Stop(uint32_t timeout)
{
	AJAAutoLock lock(&mLock);
	AJAStatus status = AJA_STATUS_SUCCESS;

	// return success if the thread is already stopped
	if (!Active())
	{
		return AJA_STATUS_SUCCESS;
	}

	// check for infinite timeout
	if (timeout == 0xffffffff)
	{
		timeout = INFINITE;
	}

	// signal thread to stop
	mTerminate = true;

	if (timeout > 0)
	{
		// wait for thread to stop
		DWORD returnCode = WaitForSingleObject(mhThreadHandle, (DWORD)timeout);
		// the wait timed out
		if (returnCode == WAIT_TIMEOUT)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Warning, "AJAThread::Stop  timeout waiting to stop");
			status = AJA_STATUS_TIMEOUT;
		}
		else if (returnCode != WAIT_OBJECT_0)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Warning, "AJAThread::Stop  thread failed to stop");
			status = AJA_STATUS_FAIL;
		}
		else
		{
			status = AJA_STATUS_SUCCESS;
		}
	}

	if(AJA_STATUS_SUCCESS == status)
	{
	// close thread handle
		CloseHandle(mhThreadHandle);
		mhThreadHandle = 0;
		mThreadID = 0;
	}

	return status;
}

AJAStatus 
AJAThreadImpl::Kill(uint32_t exitCode)
{
	AJAAutoLock lock(&mLock);
	AJAStatus status = AJA_STATUS_SUCCESS;

	// return success if the thread is already stopped
	if (!Active())
	{
		return AJA_STATUS_SUCCESS;
	}
	if(TerminateThread(mhThreadHandle, exitCode))
		return AJA_STATUS_SUCCESS;
	return AJA_STATUS_FAIL;
}

bool
AJAThreadImpl::Active()
{
	// if no handle thread is not active
	if (mhThreadHandle == 0)
	{
		return false;
	}

	// if wait succeeds then thread has exited
	DWORD returnCode = WaitForSingleObject(mhThreadHandle, 0);
	if (returnCode == WAIT_OBJECT_0)
	{
		// the thread has terminated, so clear the handle
		CloseHandle(mhThreadHandle);
		mhThreadHandle = 0;

		return false;
	}

	return true;
}

bool
AJAThreadImpl::IsCurrentThread()
{
	if(mThreadID == 0)
	{
		return false;
	}

	if(mThreadID == GetCurrentThreadId())
	{
		return true;
	}
	
	return false;
}


AJAStatus
AJAThreadImpl::SetPriority(AJAThreadPriority threadPriority)
{
	AJAAutoLock lock(&mLock);
	bool success = true;

	// save priority for starts
	mPriority = threadPriority;

	// if thread is running dynamically set the priority
	if (Active())
	{
		switch (threadPriority)
		{
		case AJA_ThreadPriority_Normal:
			success = SetThreadPriority(mhThreadHandle, THREAD_PRIORITY_NORMAL)? true : false;
			break;
		case AJA_ThreadPriority_Low:
			success = SetThreadPriority(mhThreadHandle, THREAD_PRIORITY_LOWEST)? true : false;
			break;
		case AJA_ThreadPriority_High:
			success = SetThreadPriority(mhThreadHandle, THREAD_PRIORITY_HIGHEST)? true : false;
			break;
		case AJA_ThreadPriority_TimeCritical:
			success = SetThreadPriority(mhThreadHandle, THREAD_PRIORITY_TIME_CRITICAL)? true : false;
			break;
		case AJA_ThreadPriority_AboveNormal:
			success = SetThreadPriority(mhThreadHandle, THREAD_PRIORITY_ABOVE_NORMAL)? true : false;
			break;
		case AJA_ThreadPriority_Unknown:
		default:
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority:  bad thread priority %d", mpThread, threadPriority);
			return AJA_STATUS_RANGE;
		}
	}

	return success? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL;
}


AJAStatus
AJAThreadImpl::GetPriority(AJAThreadPriority* pTheadPPriority)
{
	if(pTheadPPriority == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	*pTheadPPriority = mPriority;

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAThreadImpl::SetRealTime(AJAThreadRealTimePolicy policy, int priority)
{
    return AJA_STATUS_FAIL;
}


AJAStatus
AJAThreadImpl::Attach(AJAThreadFunction* pThreadFunction, void* pUserContext)
{
	// remember the users thread function
	mThreadFunc = pThreadFunction;
	mpUserContext = pUserContext;

	return AJA_STATUS_SUCCESS;
}


DWORD WINAPI 
AJAThreadImpl::ThreadProcStatic(void* pThreadImplContext)
{
	// this function is called when the thread starts
	AJAThreadImpl* pThreadImpl = static_cast<AJAThreadImpl*>(pThreadImplContext);
	AJA_ASSERT(pThreadImpl != NULL);
	if (pThreadImpl == NULL)
	{
		return 0;
	}

	// call the thread worker function
	try
	{
		// if user specified function call it
		if (pThreadImpl->mThreadFunc != NULL)
		{
			(*pThreadImpl->mThreadFunc)(pThreadImpl->mpThread, pThreadImpl->mpUserContext);
		}
		// otherwise call the virtual function
		else
		{
			pThreadImpl->mpThread->ThreadRun();
		}
	}
	catch (...)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic exception in thread function", pThreadImpl->mpThread);
		return 0;
	}

	return 1;
}

AJAStatus AJAThreadImpl::SetThreadName(const char *) {
	// FIXME: This feature is not supported on windows yet.
	return AJA_STATUS_FAIL;
}

uint64_t AJAThreadImpl::GetThreadId()
{
    return uint64_t(GetCurrentThreadId());
}
