/* SPDX-License-Identifier: MIT */
/**
	@file		pnp/mac/pnpimpl.cpp
	@brief		Implements the AJAPnpImpl class on the Mac platform.
	@copyright	(C) 2011-2021 AJA Video Systems, Inc.  All rights reserved.
**/
#include "ajabase/pnp/mac/pnpimpl.h"
#include "ajabase/common/common.h"
#include "devicenotifier.h"		//	For now use NTV2 DeviceNotifier facility
#include <sstream>
#include <iostream>
#include <iomanip>


using namespace std;

//	Logging Macros

#define	HEX2(__x__)				"0x" << hex << setw(2)  << setfill('0') << (0x00FF     & uint16_t(__x__)) << dec
#define	HEX4(__x__)				"0x" << hex << setw(4)  << setfill('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)				"0x" << hex << setw(8)  << setfill('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)			"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define KR(_kr_)				"kernErr=" << HEX8(_kr_) << "(" << ::GetKernErrStr(_kr_) << ")"
#define INST(__p__)				"Ins-" << hex << setw(16) << setfill('0') << uint64_t(__p__) << dec
#define THRD(__t__)				"Thr-" << hex << setw(16) << setfill('0') << uint64_t(__t__) << dec

#define	PNPLOGS(__lvl__, __x__)	AJA_sREPORT(AJA_DebugUnit_PnP, (__lvl__),	__func__ << ": " << __x__)


// static
bool sOnline = false;
void PCIDeviceNotifierCallback (unsigned long message, void *refcon);



AJAPnpImpl::AJAPnpImpl() : mRefCon(NULL), mCallback(NULL), mDevices(0)
{
	mPciDevices = new KonaNotifier(PCIDeviceNotifierCallback, this);
}


AJAPnpImpl::~AJAPnpImpl()
{
	Uninstall();

	delete mPciDevices;
	mPciDevices = NULL;
}


AJAStatus AJAPnpImpl::Install(AJAPnpCallback callback, void* refCon, uint32_t devices)
{
	mCallback = callback;
	mRefCon = refCon;
	mDevices = devices;
	
	// pci devices
	if ((mDevices & AJA_Pnp_PciVideoDevices) != 0)
	{
		mPciDevices->Install();
	}
	return AJA_STATUS_SUCCESS;
}
	

AJAStatus AJAPnpImpl::Uninstall(void)
{
	mCallback = NULL;
	mRefCon = NULL;
	mDevices = 0;
	return AJA_STATUS_SUCCESS;
}


// static - translate a PCIDeviceNotifierCallback/message to a AJAPnpCallback/message
void PCIDeviceNotifierCallback  (unsigned long message, void *refcon)
{
	PNPLOGS(AJA_DebugSeverity_Debug, "msg=" << HEX8(message));

	AJAPnpImpl* pnpObj = (AJAPnpImpl*) refcon;
	if (pnpObj == NULL)
	{
		PNPLOGS(AJA_DebugSeverity_Error, "NULL refcon, msg=" << HEX8(message));
		return;
	}
	
	AJAPnpCallback callback = pnpObj->GetCallback();
	if (callback == NULL)
	{
		PNPLOGS(AJA_DebugSeverity_Error, "NULL callback, msg=" << HEX8(message));
		return;
	}

	switch (message)
	{
		case kAJADeviceInitialOpen:
			PNPLOGS(AJA_DebugSeverity_Info, "kAJADeviceInitialOpen, AJA_Pnp_DeviceAdded");
			(callback)(AJA_Pnp_DeviceAdded, pnpObj->GetRefCon());
			break;
			
		case kAJADeviceTerminate:
			PNPLOGS(AJA_DebugSeverity_Info, "kAJADeviceTerminate, AJA_Pnp_DeviceRemoved");
			(callback)(AJA_Pnp_DeviceRemoved, pnpObj->GetRefCon());
			break;
			
		default:
			break;
	}
}
