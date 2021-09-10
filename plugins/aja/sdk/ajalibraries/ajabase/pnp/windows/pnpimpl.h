/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/windows/pnpimpl.h
	@brief		Declares the AJAPnpImpl class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PNP_IMPL_H
#define AJA_PNP_IMPL_H

#include <Windows.h>
#include "ajabase/pnp/pnp.h"

class AJAPnpImpl
{
public:
	AJAPnpImpl();
	virtual ~AJAPnpImpl(void);

	AJAStatus Install(AJAPnpCallback callback, void* refCon, uint32_t devices);
	AJAStatus Uninstall(void);
	
	AJAPnpCallback GetCallback();
	void* GetRefCon();
	uint32_t GetPnpDevices();
	void AddSignaled();
	void RemoveSignaled();

private:
	
	void*				mRefCon;
	AJAPnpCallback		mCallback;
	uint32_t			mDevices;
	HANDLE				mAddEventHandle;
	HANDLE				mAddWaitHandle;
	HANDLE				mRemoveEventHandle;
	HANDLE				mRemoveWaitHandle;
	bool				mbInstalled;
};

#endif	//	AJA_PNP_IMPL_H
