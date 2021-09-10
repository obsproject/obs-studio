/* SPDX-License-Identifier: MIT */
/**
	@file		windows/lockimpl.h
	@brief		Declares the AJALockImpl class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_LOCK_IMPL_H
#define AJA_LOCK_IMPL_H

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"


class AJALockImpl
{
public:

	AJALockImpl(const char* pName);
	virtual ~AJALockImpl();

	AJAStatus Lock(uint32_t uTimeout = 0xffffffff);
	AJAStatus Unlock();

	HANDLE mMutex;
};

#endif	//	AJA_LOCK_IMPL_H
