/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/mac/pnpimpl.h
	@brief		Declares the AJAPnpImpl class.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_PNP_IMPL_H
#define AJA_PNP_IMPL_H

#include "ajabase/pnp/pnp.h"

class DeviceNotifier;


class AJAPnpImpl
{
public:
							AJAPnpImpl();
	virtual					~AJAPnpImpl(void);

	AJAStatus				Install (AJAPnpCallback callback, void* refCon, uint32_t devices);
	AJAStatus				Uninstall (void);
	
	inline AJAPnpCallback	GetCallback() const		{return mCallback;}
	inline void *			GetRefCon() const		{return mRefCon;}
	inline uint32_t			GetPnpDevices() const	{return mDevices;}

private:
	void*				mRefCon;
	AJAPnpCallback		mCallback;
	uint32_t			mDevices;

	DeviceNotifier*		mPciDevices;
};

#endif	//	AJA_PNP_IMPL_H
