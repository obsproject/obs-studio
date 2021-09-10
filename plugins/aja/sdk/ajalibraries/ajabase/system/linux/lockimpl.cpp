/* SPDX-License-Identifier: MIT */
/**
	@file		linux/lockimpl.cpp
	@brief		Implements the AJALockImpl class on the Linux platform.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include <errno.h>
#include "ajabase/system/linux/lockimpl.h"

// For converting milliseconds to nanoseconds
static const int MIL_2_NSEC = 1000000;

// Number of nanoseconds in a second
static const int MAX_NSEC = 1000000000;

// class AJALockImpl

AJALockImpl::AJALockImpl(const char* pName) :
	mName(pName),
	mOwner(0),
	mRefCount(0)
{
	bool okSoFar = true;
	bool freeAttr = false;

	// Set up the thread attributes
	pthread_mutexattr_t attr;
	int rc = pthread_mutexattr_init(&attr);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALockImpl(%s) attr init reported error %d", mName, rc);
		okSoFar = false;
	}
	freeAttr = true;

	if (okSoFar)
	{
		rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		if (rc)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALockImpl(%s) attr settype reported error %d", mName, rc);
			okSoFar = false;
		}
	}

	if (okSoFar)
	{
		rc = pthread_mutex_init(&mMutex, &attr);
		if (rc)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALockImpl(%s) mutex init reported error %d", mName, rc);
			okSoFar = false;
		}
	}

	// Clean up the attribute memory
	if (freeAttr)
	{
		rc = pthread_mutexattr_destroy(&attr);
		if (rc)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALockImpl(%s) attr destroy reported error %d", mName, rc);
		}
	}
}


AJALockImpl::~AJALockImpl()
{
	int rc;

	rc = pthread_mutex_destroy(&mMutex);
	if (rc)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "~AJALockImpl(%s) mutex destroy reported error %d", mName, rc);
	}
}


AJAStatus
AJALockImpl::Lock(uint32_t timeout)
{
	int rc;

	// Allow multiple locks by the same thread
	if (mOwner && (mOwner == pthread_self()))
	{
		mRefCount++;
		return AJA_STATUS_SUCCESS;
	}

	// get the current time of day
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	// calculate how long to wait
	if (timeout == 0xffffffff)
	{
		ts.tv_sec  += 60 * 60 * 24 * 365;  // A year is infinite enough
		ts.tv_nsec = 0;
	}
	else
	{
		uint64_t bigtimeout = (uint64_t) timeout * MIL_2_NSEC;
		ts.tv_sec  += bigtimeout / MAX_NSEC;
		ts.tv_nsec += bigtimeout % MAX_NSEC;
		if (ts.tv_nsec >= MAX_NSEC)
		{
			ts.tv_sec++;
			ts.tv_nsec -= MAX_NSEC;
		}
	}

	// Wait for the lock
	rc = pthread_mutex_timedlock(&mMutex, &ts);
	if (rc)
	{
		if (rc == ETIMEDOUT)
			return AJA_STATUS_TIMEOUT;
		
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJALockImpl::Lock(%s) mutex lock reported error %d", mName, rc);
		return AJA_STATUS_FAIL;
	}

	mOwner = pthread_self();
	mRefCount = 1;

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJALockImpl::Unlock()
{
	if (mOwner != pthread_self())
	{
		return AJA_STATUS_FAIL;
	}

	// Allow multiple unlocks by the same thread
	mRefCount--;
	if (mRefCount)
	{
		return AJA_STATUS_SUCCESS;
	}

	mOwner = 0;
	mRefCount = 0;
	pthread_mutex_unlock(&mMutex);

	return AJA_STATUS_SUCCESS;
}

