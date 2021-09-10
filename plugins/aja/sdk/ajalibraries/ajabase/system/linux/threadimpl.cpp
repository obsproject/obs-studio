/* SPDX-License-Identifier: MIT */
/**
	@file		linux/threadimpl.cpp
	@brief		Implements the AJAThreadImpl class on the Linux platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "ajabase/system/linux/threadimpl.h"
#include "ajabase/common/timer.h"
#include <string.h>

static const size_t STACK_SIZE = 1024 * 1024;

AJAThreadImpl::AJAThreadImpl(AJAThread* pThreadContext) :
	mpThreadContext(pThreadContext),
	mThread(0),
	mTid(0),
	mPriority(AJA_ThreadPriority_Normal),
	mThreadFunc(0),
	mpUserContext(0),
	mThreadStarted(false),
	mTerminate(false),
	mExiting(false)
{
	int rc = pthread_mutex_init(&mStartMutex, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) start mutex init reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_init(&mStartCond, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) start cond init reported error %d", mpThreadContext, rc);
	}

	rc = pthread_mutex_init(&mExitMutex, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) exit mutex init reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_init(&mExitCond, 0);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl(%p) exit cond init reported error %d", mpThreadContext, rc);
	}
}


AJAThreadImpl::~AJAThreadImpl()
{
	Stop();

	int rc = pthread_mutex_destroy(&mStartMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) start mutex destroy reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_destroy(&mStartCond);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) start cond destroy reported error %d", mpThreadContext, rc);
	}

	rc = pthread_mutex_destroy(&mExitMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) exit mutex destroy reported error %d", mpThreadContext, rc);
	}

	rc = pthread_cond_destroy(&mExitCond);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJAThreadImpl(%p) exit cond destroy reported error %d", mpThreadContext, rc);
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

	mTerminate = false;
	mExiting   = false;

	// we're going to create the thread, then block until it tells us it's alive
	rc = pthread_mutex_lock(&mStartMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl::Start(%p) error %d locking start mutex", mpThreadContext, rc);
		return AJA_STATUS_FAIL;
	}

	// create the thread
	mThreadStarted = false;

	rc = pthread_create(&mThread, &attr, ThreadProcStatic, this);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThreadImpl::Start(%p) error %d creating thread", mpThreadContext, rc);
		mThread = 0;
		return AJA_STATUS_FAIL;
	}

	// wait until the new thread signals us that it's alive (and has logged its thread ID)
	// ToDo: add a timeout? how do we respond if the thread DOESN'T come up?
	AJAStatus status = AJA_STATUS_SUCCESS;

	while (!mThreadStarted)
	{
		rc = pthread_cond_wait(&mStartCond, &mStartMutex);

		if (rc)
		{
			status = AJA_STATUS_FAIL;
			AJA_REPORT(0, AJA_DebugSeverity_Error,
				"AJAThread(%p)::Start pthread_cond_wait returned error %d",
				mpThreadContext, rc);
			break;
		}
	}

	rc = pthread_mutex_unlock(&mStartMutex);
	if (rc)
	{
		status = AJA_STATUS_FAIL;
		AJA_REPORT(0, AJA_DebugSeverity_Error,
			"AJAThread(%p)::Start error %d unlocking start mutex",
			mpThreadContext, rc);
	}


#if 0 // This should be set within the running thread itself if a dynamic change
      // in priority is desired.  This will fail in a daemon invocation.

	// Now that the new thread is up and running and has reported its
	// thread ID, set the thread priority
	if ( AJA_SUCCESS(status) )
		status = SetPriority(mPriority);
#endif

	return status;
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

	// save priority for starts
	mPriority = threadPriority;

	// If thread isn't running, we're done (but we've logged the desired priority for later)
	if (!Active())
		return AJA_STATUS_SUCCESS;

	// If we haven't been able to get a thread ID (thread not running yet?), bail
	if (mTid == 0)							// note: we're assuming PID == 0 never happens in nature...?
		return AJA_STATUS_FAIL;


	// If thread is already running, tweak its priority
	// We're going to mix Realtime priorities (policy == SCHED_FIFO/SCHED_RR) and "standard" round-robin priorities
	// (policy == SCHED_OTHER) in one API...
	bool bRTPriority = false;				//
	int  newPriority = 0;					// "nice" priority used by setpriority() when bRTPriority = false, or RT priority used by sched_setscheduler() when bRTPriority = true

	switch (threadPriority)
	{
		case AJA_ThreadPriority_Normal:
			bRTPriority = false;			// use setpriority()
			newPriority = 0;				// -20 to +19, standard "nice" priority
			break;
		case AJA_ThreadPriority_Low:
			bRTPriority = false;			// use setpriority()
			newPriority = 10;				// positive "nice" values have LOWER priority
			break;
		case AJA_ThreadPriority_High:
			bRTPriority = false;			// use setpriority()
			newPriority = -10;				// negative "nice" values have HIGHER priority
			break;
		case AJA_ThreadPriority_AboveNormal:
			bRTPriority = false;			// use setpriority()
			newPriority = -5;				// negative "nice" values have HIGHER priority
			break;
		case AJA_ThreadPriority_TimeCritical:
			bRTPriority = true;				// use sched_setscheduler()
            newPriority = 90;				// 1 - 99 (higher values = higher priority)
			break;
		case AJA_ThreadPriority_Unknown:
		default:
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: bad thread priority %d", mpThreadContext, threadPriority);
			return AJA_STATUS_RANGE;
	}

	// first, set (or unset) the RT priority
	struct sched_param newParam;
	newParam.sched_priority = (bRTPriority ? newPriority : 0);		// if we're not using RT Priority, reset to SCHED_OTHER and sched_priority = 0
	int newPolicy = (bRTPriority ? SCHED_RR : SCHED_OTHER);
	int rc = pthread_setschedparam(mThread, newPolicy, &newParam);
	if (rc != 0)
	{
        AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: error %d setting sched param: policy = %d, priority = %d\n", mpThreadContext, rc, newPolicy, newParam.sched_priority);
        return AJA_STATUS_FAIL;
	}

	// now set the standard ("nice") priority
	int newNice = (bRTPriority ? 0 : newPriority);		// if we're using RT Priority (SCHED_RR or SCHED_FF), reset "nice" level to zero
	rc = setpriority(PRIO_PROCESS, mTid, newNice);
	if (errno != 0)
	{
        AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetPriority: error %d setting nice level: %d\n", mpThreadContext, rc, newNice);
        return AJA_STATUS_FAIL;
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
    int pval = 0;
    switch(policy)
    {
        case AJA_ThreadRealTimePolicyFIFO:
            pval = SCHED_FIFO;
            break;
        case AJA_ThreadRealTimePolicyRoundRobin:
            pval = SCHED_RR;
            break;
        default:
            AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetRealTime: bad thread policy %d", mpThreadContext, policy);
            return AJA_STATUS_RANGE;
    }

    for(int i = 0; i < 30; i++)
    {
        if(!Active())
        {
            usleep(1000);
            continue;
        }
        struct sched_param newParam;
        memset(&newParam, 0, sizeof(newParam));
        newParam.sched_priority = priority;
        int rc = pthread_setschedparam(mThread, pval, &newParam);
        if (rc != 0)
        {
            AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetRealTime: error %d setting sched param: policy = %d, priority = %d\n", mpThreadContext, rc, pval, newParam.sched_priority);
            return AJA_STATUS_FAIL;
        }
        return AJA_STATUS_SUCCESS;
    }
    AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::SetRealTime: Failed to set realtime thread is not running\n", mpThreadContext);
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

	// Who am I? We need the thread ID (TID) in order to set thread scheduler priorities
//	pid_t myTid = gettid();					// in theory we could make this call, but Glibc doesn't support it
	errno = 0;								// so instead we do this (ick...)
	pid_t myTid = syscall( SYS_gettid );
	if (errno == 0)							// theoretically gettid() cannot fail, so by extension syscall(SYS_gettid) can't either...?
		pThreadImpl->mTid = myTid;


	// signal parent we've started
	int rc = pthread_mutex_lock(&pThreadImpl->mStartMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d locking start mutex", pThreadImpl->mpThreadContext, rc);
		return (void*)0;
	}

	pThreadImpl->mThreadStarted = true;

	rc = pthread_cond_signal(&pThreadImpl->mStartCond);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d signaling start cond variable", pThreadImpl->mpThreadContext, rc);
		return (void*)0;
	}
	rc = pthread_mutex_unlock(&pThreadImpl->mStartMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAThread(%p)::ThreadProcStatic error %d unlocking start mutex", pThreadImpl->mpThreadContext, rc);
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

	rc = pthread_mutex_lock(&pThreadImpl->mExitMutex);
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
	int ret = prctl(PR_SET_NAME, (unsigned long)name, 0, 0);
	if(ret == -1) {
		AJA_REPORT(0, AJA_DebugSeverity_Error, "Failed to set thread name to %s", name);
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

uint64_t AJAThreadImpl::GetThreadId()
{
    errno = 0;
    pid_t tid = syscall(SYS_gettid);
    if (errno == 0)
        return uint64_t(tid);
    else
        return 0;
}
