/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/windows/pnpimpl.cpp
	@brief		Implements the AJAPnpImpl class on the Windows platform.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/pnp/windows/pnpimpl.h"
#include "ajabase/system/systemtime.h"

void
CALLBACK SignaledAddRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	AJAPnpImpl *context = (AJAPnpImpl*)lpParam;
	context->AddSignaled();

}

void
CALLBACK SignaledRemoveRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	AJAPnpImpl *context = (AJAPnpImpl*)lpParam;
	context->RemoveSignaled();

}

AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0), mAddEventHandle(NULL), mAddWaitHandle(NULL), mRemoveEventHandle(NULL), mRemoveWaitHandle(NULL)
{
}


AJAPnpImpl::~AJAPnpImpl()
{
	Uninstall();
}


AJAStatus 
AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon = refCon;
	mDevices = devices;

	
 	mAddEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPAddEvent");
 	RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)&SignaledAddRoutine, this, INFINITE, NULL);
 
 	mRemoveEventHandle = CreateEventW(NULL, FALSE, FALSE, L"Global\\AJAPNPRemoveEvent");
 	RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)&SignaledRemoveRoutine, this, INFINITE, NULL);
	
	return AJA_STATUS_SUCCESS;
}
	
AJAStatus 
AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	
	if(mAddWaitHandle != NULL)
	{
		UnregisterWait(mAddWaitHandle);
		mAddWaitHandle = NULL;
	}
	if(mRemoveWaitHandle != NULL)
	{
		UnregisterWait(mRemoveWaitHandle);
		mRemoveWaitHandle = NULL;
	}
	
	return AJA_STATUS_SUCCESS;
}

AJAPnpCallback 
AJAPnpImpl::GetCallback()
{
	return mCallback;
}

void* 
AJAPnpImpl::GetRefCon()
{
	return mRefCon;
}

uint32_t
AJAPnpImpl::GetPnpDevices()
{
	return mDevices;
}

void
AJAPnpImpl::AddSignaled()
{
	AJATime::Sleep(1000);
 	if (mCallback)
 		(*(mCallback))(AJA_Pnp_DeviceAdded, mRefCon);

	RegisterWaitForSingleObject(&mAddWaitHandle, mAddEventHandle, (WAITORTIMERCALLBACK)SignaledAddRoutine, this, INFINITE, NULL);
}

void
AJAPnpImpl::RemoveSignaled()
{
	if (mCallback)
		(*(mCallback))(AJA_Pnp_DeviceRemoved, mRefCon);

	RegisterWaitForSingleObject(&mRemoveWaitHandle, mRemoveEventHandle, (WAITORTIMERCALLBACK)SignaledRemoveRoutine, this, INFINITE, NULL);
}
