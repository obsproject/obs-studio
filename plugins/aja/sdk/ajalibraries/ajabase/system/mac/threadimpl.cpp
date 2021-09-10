/* SPDX-License-Identifier: MIT */
/**
	@file		mac/threadimpl.cpp
	@brief		Implements the AJAThreadImpl class on the Mac platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include "ajabase/system/mac/pthreadsextra.h"
#include "ajabase/system/mac/threadimpl.h"
#include "ajabase/common/timer.h"

#include <mach/mach_init.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#include <mach/mach_time.h>

static const size_t STACK_SIZE = 1024 * 1024;

AJAThreadImpl::AJAThreadImpl(AJAThread* pThreadContext) :
	mpThreadContext(pThreadContext),
	mThread(0),
	mPriority(AJA_ThreadPriority_Normal),
	mThreadFunc(0),
	mpUserContext(0),
	mTerminate(false),
	mExiting(false)
{
	int rc = pthread_mutex_init(&mExitMutex, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) mutex init reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_init(&mExitCond, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) cond init reported error %d", mpThreadContext, rc);
	}
}


AJAThreadImpl::~AJAThreadImpl()
{
	Stop();

	int rc = pthread_mutex_destroy(&mExitMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) mutex destroy reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_destroy(&mExitCond);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) cond destroy reported error %d", mpThreadContext, rc);
	}
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

	// Windows version currently uses the default stack size
	// The docs say this is 1MB, so give our threads the same stack size
	pthread_attr_t attr;
	int rc = 0;
	rc |= pthread_attr_init(&attr);
	rc |= pthread_attr_setstacksize(&attr, STACK_SIZE);
	rc |= pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl::Start(%p) error setting thread attributes", mpThreadContext);
		mThread = 0;
		return AJA_STATUS_FAIL;
	}

	// create the thread
	mTerminate = false;
	mExiting = false;
	rc = pthread_create(&mThread, &attr, ThreadProcStatic, this);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl::Start(%p) error %d creating thread", mpThreadContext, rc);
		mThread = 0;
		return AJA_STATUS_FAIL;
	}

	// set the thread priority
	return SetPriority(mPriority);
}


AJAStatus 
AJAThreadImpl::Stop(uint32_t timeout)
{
	AJAAutoLock lock(&mLock);
	AJAStatus returnStatus = AJA_STATUS_SUCCESS;

	// return success if the thread is already stopped
	if (!Active())
	{
		return AJA_STATUS_SUCCESS;
	}

	// wait for thread to stop
	int rc = pthread_mutex_lock(&mExitMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl::Stop(%p) error %d locking exit mutex", mpThreadContext, rc);
		return AJA_STATUS_FAIL;
	}

	// calculate how long to wait
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	if (timeout == 0xffffffff)
	{
		ts.tv_sec += 60 * 60 * 24 * 365;  // A year is infinite enough
	}
	else
	{
		// Calculates the non-second portion of the timeout
		// to nanoseconds.  
		uint32_t nsec = ((timeout % 1000) * 1000000);

		// Add the current nanosecond count to the result
		nsec += ts.tv_nsec;

		struct timespec ts1;
		ts1 = ts;

		// The addition of the two nanosecond values may wrap to
		// another second, so note that; but our nsec portion will
		// always be less than a second.  Finally, create the
		// absolute time during which the timeout will lapse.
		ts.tv_sec += (timeout / 1000) + (nsec / 1000000000);
		ts.tv_nsec = (nsec % 1000000000);
	}

	// signal thread to stop
	mTerminate = true;

		
	// loop until signaled or timed out
	do
	{
		if (mExiting == false)
		{
			rc = pthread_cond_timedwait(&mExitCond, &mExitMutex, &ts);
			if (rc)
			{
				returnStatus = AJA_STATUS_FAIL;
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Error,
					"AJAThread(%p)::Stop pthread_cond_timedwait returned error %d",
					mpThreadContext, rc);

				// non-timeout errors release the mutex by themselves
				if (rc == ETIMEDOUT)
				{
					rc = pthread_mutex_unlock(&mExitMutex);
					if (rc)
					{
						AJA_REPORT(
							0,
							AJA_DebugSeverity_Error,
							"AJAThread(%p)::Stop error %d unlocking timeout mutex",
							mpThreadContext, rc);
					}
				}
				break;
			}
		}

		// deal with spurious wakeups here
		if (!mExiting)
			continue;  // the thread hasn't left its loop, so wait some more
		else
		{
			rc = pthread_mutex_unlock(&mExitMutex);
			if (rc)
			{
				returnStatus = AJA_STATUS_FAIL;
				AJA_REPORT(
					0,
					AJA_DebugSeverity_Error,
					"AJAThread(%p)::Stop error %d unlocking exit mutex",
					mpThreadContext, rc);
			}
			break;
		}
	} while (rc == 0);

	// close thread handle
	void* exitValue;
	rc = pthread_join(mThread, &exitValue);
	if( rc )
	{
		returnStatus = AJA_STATUS_FAIL;
		AJA_REPORT(
			0,
			AJA_DebugSeverity_Error,
			"AJAThread(%p)::Stop error %d from pthread_join",
			mpThreadContext, rc);
	}
	mThread = 0;

	return returnStatus;
}


AJAStatus
AJAThreadImpl::Kill(uint32_t exitCode)
{
    AJA_UNUSED(exitCode);
    
	AJAAutoLock lock(&mLock);
	AJAStatus returnStatus = AJA_STATUS_SUCCESS;

	// If the thread doesn't exist, consider the Kill successful
	// with zero, no signal sent to thread, it's only checked for validity
	if (!pthread_kill(mThread, 0))
		return returnStatus;

	// Try to make the thread as killable as possible
	int rc = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if( rc )
	{
		returnStatus = AJA_STATUS_FAIL;
		AJA_REPORT(
			0,
			AJA_DebugSeverity_Error,
			"AJAThread(%p)::Kill error %d from pthread_setcancelstate",
			mpThreadContext, rc);
	}
	rc = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if( rc )
	{
		returnStatus = AJA_STATUS_FAIL;
		AJA_REPORT(
			0,
			AJA_DebugSeverity_Error,
			"AJAThread(%p)::Kill error %d from pthread_setcanceltype",
			mpThreadContext, rc);
	}

	// This should kill the thread, but there are no guarantees 
	rc = pthread_cancel(mThread);
	if( rc )
	{
		returnStatus = AJA_STATUS_FAIL;
		AJA_REPORT(
			0,
			AJA_DebugSeverity_Error,
			"AJAThread(%p)::Kill error %d from pthread_cancel",
			mpThreadContext, rc);
	}

	return returnStatus;
}


bool
AJAThreadImpl::Active()
{
	// if no handle thread is not active
	if (mThread == 0)
	{
		return false;
	}

	// with zero, no signal sent to thread, it's only checked for validity
	if (!pthread_kill(mThread, 0))
		return true;

	// the thread has terminated, so clear the handle
	mThread = 0;

	return false;
}

bool
AJAThreadImpl::IsCurrentThread()
{
	if(mThread == 0)
	{
		return false;
	}

	if( pthread_equal(mThread, pthread_self()) )
	{
		return true;
	}
	
	return false;
}


AJAStatus
AJAThreadImpl::SetPriority(AJAThreadPriority threadPriority)
{
	AJAAutoLock lock(&mLock);
	int nice = 0;

	switch (threadPriority)
	{
	case AJA_ThreadPriority_Normal:
		nice = 0;
		break;
	case AJA_ThreadPriority_Low:
		nice = -10;
		break;
	case AJA_ThreadPriority_High:
		nice = 10;
		break;
	case AJA_ThreadPriority_TimeCritical:
		nice = 19;
		break;
	case AJA_ThreadPriority_AboveNormal:
		nice = 5;
		break;
	case AJA_ThreadPriority_Unknown:
	default:
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: bad thread priority %d", mpThreadContext, threadPriority);
		return AJA_STATUS_RANGE;
	}

	// save priority for starts
	mPriority = threadPriority;

	// If thread isn't running, we're done (but we've logged the desired priority for later)
	if (!Active())
		return AJA_STATUS_SUCCESS;

	int policy;
	struct sched_param param;
	int rc = pthread_getschedparam(mThread, &policy, &param);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: error %d getting sched param", mpThreadContext, threadPriority);
		return AJA_STATUS_FAIL;
	}

	param.sched_priority = nice;
	rc = pthread_setschedparam(mThread, policy, &param);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: error %d setting sched param", mpThreadContext, threadPriority);
		return AJA_STATUS_FAIL;
	}
	
	if (threadPriority == AJA_ThreadPriority_TimeCritical)
	{
		mach_timebase_info_data_t info;
		IOReturn ioReturn =  mach_timebase_info(&info);
	
		uint32_t period			= 60000000;
		uint32_t computation	=  2000000;
		
		thread_time_constraint_policy timeConstraint;
		timeConstraint.period		= uint32_t(static_cast<long long>(period)      * info.denom / info.numer);
		timeConstraint.computation	= uint32_t(static_cast<long long>(computation) * info.denom / info.numer);
		timeConstraint.constraint	= timeConstraint.computation * 2;
		timeConstraint.preemptible	= true;
		
		mach_port_t machThread = pthread_mach_thread_np(mThread);
		while (MACH_PORT_NULL == machThread)
			machThread = pthread_mach_thread_np(mThread);

		ioReturn = thread_policy_set(machThread, THREAD_TIME_CONSTRAINT_POLICY,
				reinterpret_cast<thread_policy_t>(&timeConstraint), THREAD_TIME_CONSTRAINT_POLICY_COUNT);
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAThreadImpl::GetPriority(AJAThreadPriority* pThreadPriority)
{
	if (pThreadPriority == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	*pThreadPriority = mPriority;

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


void*
AJAThreadImpl::ThreadProcStatic(void* pThreadImplContext)
{
	// this function is called when the thread starts
	AJAThreadImpl* pThreadImpl = static_cast<AJAThreadImpl*>(pThreadImplContext);
	AJA_ASSERT(pThreadImpl != NULL);
	if (pThreadImpl == NULL)
	{
		return (void*)0;
	}

	// call the thread worker function
	try
	{
		// if user specified function call it
		if (pThreadImpl->mThreadFunc != NULL)
		{
			(*pThreadImpl->mThreadFunc)(pThreadImpl->mpThreadContext, pThreadImpl->mpUserContext);
		}
		// otherwise call the virtual function
		else
		{
			pThreadImpl->mpThreadContext->ThreadRun();
		}
	}
	catch(...)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic exception in thread function", pThreadImpl->mpThreadContext);
		return (void*)0;
    }
	// signal parent we're exiting
	pThreadImpl->mExiting = true;

	int rc = pthread_mutex_lock(&pThreadImpl->mExitMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d locking exit mutex", pThreadImpl->mpThreadContext, rc);
		return (void*)0;
	}
	rc = pthread_cond_signal(&pThreadImpl->mExitCond);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d signaling cond variable", pThreadImpl->mpThreadContext, rc);
		return (void*)0;
	}
	rc = pthread_mutex_unlock(&pThreadImpl->mExitMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d unlocking exit mutex", pThreadImpl->mpThreadContext, rc);
		return (void*)0;
	}

	return (void*)1;
}

AJAStatus AJAThreadImpl::SetThreadName(const char *name) {
    int ret = pthread_setname_np(name);
    if (ret != 0)
    {
        return AJA_STATUS_FAIL;
    }
    return AJA_STATUS_SUCCESS;
}

uint64_t AJAThreadImpl::GetThreadId()
{
    uint64_t tid=0;
    pthread_threadid_np(NULL, &tid);
    return tid;
}
