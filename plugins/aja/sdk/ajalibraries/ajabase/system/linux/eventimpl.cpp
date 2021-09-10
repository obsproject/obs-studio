/* SPDX-License-Identifier: MIT */
/**
	@file		linux/eventimpl.cpp
	@brief		Implements the AJAEventImpl class on the Linux platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/event.h"
#include "ajabase/system/linux/eventimpl.h"
#include <errno.h>

#define MAX_EVENTS 64

using std::string;

// event implementation class (linux)
AJAEventImpl::AJAEventImpl(bool manualReset, const std::string& name)
	: mManualReset(manualReset)
{
    AJA_UNUSED(name);
	pthread_mutex_init(&mMutex, NULL);
	pthread_cond_init(&mCondVar, NULL);
	Clear();
}


AJAEventImpl::~AJAEventImpl(void)
{
	pthread_cond_destroy(&mCondVar);
	pthread_mutex_destroy(&mMutex);
}


AJAStatus 
AJAEventImpl::Signal(void)
{
	// check for open
	int theError;

	pthread_mutex_lock(&mMutex);
	mSignaled = true;
	theError  = pthread_cond_broadcast(&mCondVar);
	pthread_mutex_unlock(&mMutex);

	if (theError)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAEventImpl::Signal() returns error %08x", theError);
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAEventImpl::Clear(void)
{
	// check for open
	mSignaled = false;
	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAEventImpl::SetState(bool signaled)
{
	AJAStatus status;

	if (signaled)
	{
		status = Signal();
	}
	else
	{
		status = Clear();
	}

	return status;
}


AJAStatus
AJAEventImpl::GetState(bool* pSignaled)
{
    AJA_UNUSED(pSignaled);

	// wait with timeout 0 returns immediately
	return WaitForSignal(0);
}


AJAStatus
AJAEventImpl::SetManualReset(bool manualReset)
{
	mManualReset = manualReset;
	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAEventImpl::GetManualReset(bool* pManualReset)
{
    if (pManualReset)
	{
		*pManualReset = mManualReset;
		return AJA_STATUS_SUCCESS;
	}
	else
		return AJA_STATUS_FAIL;
}


AJAStatus
AJAEventImpl::WaitForSignal(uint32_t timeout)
{
	static const long long	NS_PER_SEC = 1000000000ULL;

	// check for open
	int64_t nanos = timeout;
	nanos *= 1000000;

	struct timespec ts;
	int32_t         result;
	AJAStatus       status = AJA_STATUS_SUCCESS;
	
	clock_gettime(CLOCK_REALTIME, &ts);

	nanos     += ts.tv_nsec;
	ts.tv_sec += (nanos / NS_PER_SEC);
	ts.tv_nsec = (nanos % NS_PER_SEC);

	pthread_mutex_lock(&mMutex);

	if (false == mSignaled)
	{
		result = pthread_cond_timedwait(&mCondVar, &mMutex, &ts);

		if (0 != result)
		{
			if (result == ETIMEDOUT) 
			{
				AJA_REPORT(0, AJA_DebugSeverity_Info,
					"AJAEventImpl::WaitForSignal() timeout", result);
				status = AJA_STATUS_TIMEOUT;
			}
			else
			{
				AJA_REPORT(0, AJA_DebugSeverity_Error,
					"AJAEventImpl::WaitForSignal() "
					"pthread_cond_timedwait returns error %08x", result);
				status = AJA_STATUS_FAIL;
			}
		}
	}
	if (!mManualReset)
	{
		mSignaled = false;
	}
	pthread_mutex_unlock(&mMutex);
	return (status);
}


AJAStatus
AJAEventImpl::GetEventObject(uint64_t* pEventObject)
{
    AJA_UNUSED(pEventObject);

	return AJA_STATUS_SUCCESS;
}


/*
AJAStatus
AJAWaitForEvents(AJAEventPtr* pEventList, uint32_t numEvents, bool all, uint32_t timeout)
{
	
	HANDLE eventArray[MAX_EVENTS];
	
	if (pEventList == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list is NULL");
		return AJA_STATUS_INITIALIZE;
	}

	if (numEvents == 0)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list is empty");
		return AJA_STATUS_RANGE;
	}

	if (numEvents > MAX_EVENTS)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list larger that MAX_EVENTS");
		return AJA_STATUS_RANGE;
	}

	uint32_t i;

	// build the array of handles from the AJAEvent(s)
	for (i = 0; i < numEvents; i++)
	{
		AJAEventPtr eventPtr = pEventList[i];
		AJA_ASSERT(eventPtr->mImplPtr->mEvent != NULL);
		if(eventPtr->mImplPtr->mEvent == NULL)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event not initialized");
			return AJA_STATUS_INITIALIZE;
		}
		eventArray[i] = eventPtr->mImplPtr->mEvent;
	}

	// check for infinite timeout
	if (timeout == 0xffffffff)
	{
		timeout = INFINITE;
	}

	// wait for event(s) to be signaled
	DWORD retCode = WaitForMultipleObjects(numEvents, eventArray, all, (DWORD)timeout);

	// an event was signaled
	if ((retCode >= WAIT_OBJECT_0) && (retCode < (WAIT_OBJECT_0 + numEvents)))
	{
		return AJA_STATUS_SUCCESS;
	}
	// the wait timed out
	else if (retCode == WAIT_TIMEOUT)
	{
		return AJA_STATUS_TIMEOUT;
	}

	// some type of error occurred
	AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAEvent()::WaitForEvents WaitForMultipleObjects() returns error %08x", retCode);

	return AJA_STATUS_FAIL;
}
*/
