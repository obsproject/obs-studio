/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverprocamp.h
	@brief		Declares functions used in the NTV2 device driver.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.  All rights reserved.
**/
// ntv2driverprocamp.h
#ifndef NTV2DRIVERPROCAMP_H
#define NTV2DRIVERPROCAMP_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"

// This file is used by the Linux driver which is C, not C++.
#if defined(__CPLUSPLUS__) || defined(__cplusplus)
#else
#define false (0)
#define true (!false)
#endif

#ifdef AJALinux
#include "ntv2linuxpublicinterface.h"
#endif

#ifdef AJAMac
	#include <IOKit/IOTypes.h>
	#include "ntv2macpublicinterface.h"
#endif

#ifdef MSWindows
#include "ntv2winpublicinterface.h"
#endif


bool SetVirtualProcampRegister(	VirtualRegisterNum virtualRegisterNum, 
								ULWord value, 
								VirtualProcAmpRegisters *regs);

bool GetVirtualProcampRegister(	VirtualRegisterNum virtualRegisterNum, 
								ULWord *value, 
								VirtualProcAmpRegisters *regs);

#ifndef AJAMac		// note: in Mac-land these are methods in MacDriver.h

bool WriteHardwareProcampRegister(	ULWord inDeviceIndex,
									NTV2DeviceID deviceID,
									VirtualRegisterNum virtualRegisterNum, 
									ULWord value, 
									HardwareProcAmpRegisterImage *hwRegImage);
									
bool RestoreHardwareProcampRegisters(	ULWord inDeviceIndex,
										NTV2DeviceID deviceID,
										VirtualProcAmpRegisters *regs,
										HardwareProcAmpRegisterImage *hwRegImage);

bool I2CWriteDataSingle(ULWord inDeviceIndex, UByte I2CAddress, UByte I2CData);

bool I2CWriteDataDoublet(	ULWord inDeviceIndex,
						UByte I2CAddress1, UByte I2CData1,
						UByte I2CAddress2, UByte I2CData2);
						
#endif	// !AJAMac

#endif	// NTV2DRIVERPROCAMP_H
