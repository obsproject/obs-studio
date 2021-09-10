/* SPDX-License-Identifier: MIT */
/**
	@file		lock.cpp
	@brief		Implements the AJALock class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#if defined(AJA_USE_CPLUSPLUS11)
	#include <chrono>
#else
	// include the system dependent implementation class
	#if defined(AJA_WINDOWS)
		#include "ajabase/system/windows/lockimpl.h"
	#endif
	#if defined(AJA_LINUX)
		#include "ajabase/system/linux/lockimpl.h"
	#endif
	#if defined(AJA_MAC)
		#include "ajabase/system/mac/lockimpl.h"
	#endif
#endif


AJALock::AJALock(const char* pName)
{
#if defined(AJA_USE_CPLUSPLUS11)
	mpMutex = new recursive_timed_mutex;
	if (pName != nullptr)
		name = pName;
#else
	mpImpl = NULL;
	mpImpl = new AJALockImpl(pName);
#endif
}


AJALock::~AJALock()
{
#if defined(AJA_USE_CPLUSPLUS11)
	delete mpMutex;
	mpMutex = nullptr;
#else
	if(mpImpl)
		delete mpImpl;
#endif
}

// interface to the implementation class

AJAStatus
AJALock::Lock(uint32_t timeout)
{
#if defined(AJA_USE_CPLUSPLUS11)
	if (timeout != LOCK_TIME_INFINITE)
	{
		bool success = mpMutex->try_lock_for(std::chrono::milliseconds(timeout));
		return success ? AJA_STATUS_SUCCESS : AJA_STATUS_TIMEOUT;
	}
	else
	{
		mpMutex->lock();
		return AJA_STATUS_SUCCESS;
	}
#else
	return mpImpl->Lock(timeout);
#endif
}


AJAStatus
AJALock::Unlock()
{
#if defined(AJA_USE_CPLUSPLUS11)
	mpMutex->unlock();
	return AJA_STATUS_SUCCESS;
#else
	return mpImpl->Unlock();
#endif
}


AJAAutoLock::AJAAutoLock(AJALock* pLock)
{
	mpLock = pLock;
	if (mpLock)
	{
		mpLock->Lock();
	}
}


AJAAutoLock::~AJAAutoLock()
{
	if (mpLock)
	{
		mpLock->Unlock();
	}
}
