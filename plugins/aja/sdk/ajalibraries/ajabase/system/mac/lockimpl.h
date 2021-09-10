/* SPDX-License-Identifier: MIT */
/**
	@file		mac/lockimpl.h
	@brief		Declares the AJALockImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_LOCK_IMPL_H
#define AJA_LOCK_IMPL_H

#include <pthread.h>
#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"

class AJALockImpl
{
public:
					
					AJALockImpl(const char* pName);
	virtual			~AJALockImpl();

	AJAStatus		Lock(uint32_t uTimeout = 0xffffffff);
	AJAStatus		Unlock();
	
private:
	const char*		mName;
	pthread_t		mOwner;
	int				mRefCount;
	pthread_mutex_t mMutex;		// For protecting the member variables
};

#endif	//	AJA_LOCK_IMPL_H
