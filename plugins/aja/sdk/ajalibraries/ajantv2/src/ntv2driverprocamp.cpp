/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2driverprocamp.cpp
	@brief		Implementation of driver procamp utility functions.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
	@note		Because this module is compiled into the driver, it must remain straight ANSI 'C' -- no C++ or STL.
				Since the Linux driver is not C++ based, a devicenumber is required in some functions in place of "this" pointer.
**/

#include "ntv2driverprocamp.h"
#if defined(NTV2_BUILDING_DRIVER)

#ifdef AJALinux
#include <linux/jiffies.h>
#include "registerio.h"
#include "driverdbg.h"
#include "ntv2driverdbgmsgctl.h"
#define ENABLE_DEBUG_PRINT
#endif

#ifdef AJAMac
	#include <IOKit/IOLib.h>
	#include "MacDriver.h"
	
	#define MSG IOLog
	#define MsgsEnabled(x)	(false)		// set this to 'true' to enable debug IOLogs
	#define ENABLE_DEBUG_PRINT
#endif



typedef enum
{
	isSDProc,
	isHDProc
} SDIProcType;

#ifndef AJAMac		// see MacDriver.h
	static bool I2CWriteData(ULWord deviceNumber, ULWord value);
	static bool I2CWriteControl(ULWord deviceNumber, ULWord value);
	static bool WaitForI2CReady(ULWord deviceNumber);
	static bool I2CInhibitHardwareReads(ULWord deviceNumber);
	static bool I2CEnableHardwareReads(ULWord deviceNumber);
#endif	



/*****************************************************************************************
 *	SetVirtualProcampRegister
 *****************************************************************************************/
bool SetVirtualProcampRegister(	VirtualRegisterNum virtualRegisterNum, 
								ULWord value, 
								VirtualProcAmpRegisters *regs)
{

#ifdef ENABLE_DEBUG_PRINT
	if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
		MSG("Got set of procamp vreg %d, value 0x%x\n", virtualRegisterNum, value);
#endif

	switch (virtualRegisterNum) 
	{
		case kVRegProcAmpSDRegsInitialized:
			regs->SD.initialized = value;
			break;

		case kVRegProcAmpStandardDefBrightness:
			regs->SD.brightness = value;
			break;

		case kVRegProcAmpStandardDefContrast:
			regs->SD.contrast = value;
			break;

		case kVRegProcAmpStandardDefSaturation:
			regs->SD.saturationCb = value;
			regs->SD.saturationCr = value;
			break;

		case kVRegProcAmpStandardDefHue:
			regs->SD.hue = value;
			break;

		case kVRegProcAmpStandardDefCbOffset:
			regs->SD.CbOffset = value;
			break;

		case kVRegProcAmpStandardDefCrOffset:
			regs->SD.CrOffset = value;
			break;

		case kVRegProcAmpHDRegsInitialized:
			regs->HD.initialized = value;
			break;

		case kVRegProcAmpHighDefContrast:
			regs->HD.contrast = value;
			break;

		case kVRegProcAmpHighDefSaturationCb:
			regs->HD.saturationCb = value;
			break;

		case kVRegProcAmpHighDefSaturationCr:
			regs->HD.saturationCr = value;
			break;

		case kVRegProcAmpHighDefBrightness:
			regs->HD.brightness = value;
			break;

		case kVRegProcAmpHighDefCbOffset:
			regs->HD.CbOffset = value;
			break;

		case kVRegProcAmpHighDefCrOffset:
			regs->HD.CrOffset = value;
			break;

		case kVRegProcAmpHighDefHue:	// Not supported in hardware
			return false;	

		default:
			return false;	
	}

	return true;
}


/*****************************************************************************************
 *	GetVirtualProcampRegister
 *****************************************************************************************/
bool GetVirtualProcampRegister(	VirtualRegisterNum virtualRegisterNum, 
								ULWord *value, 
								VirtualProcAmpRegisters *regs)
{
#ifdef ENABLE_DEBUG_PRINT
	if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
		MSG("Got get of procamp vreg %d, fetching value\n", virtualRegisterNum);
#endif

	switch (virtualRegisterNum) 
	{
		case kVRegProcAmpSDRegsInitialized:
			*value = regs->SD.initialized;
			break;

		case kVRegProcAmpStandardDefBrightness:
			*value = regs->SD.brightness;
			break;

		case kVRegProcAmpStandardDefContrast:
			*value = regs->SD.contrast;
			break;

		case kVRegProcAmpStandardDefSaturation:
			if (regs->SD.saturationCb != regs->SD.saturationCr)
				return false;
			*value = regs->SD.saturationCb;
			break;

		case kVRegProcAmpStandardDefHue:
			*value = regs->SD.hue;
			break;

		case kVRegProcAmpStandardDefCbOffset:
			*value = regs->SD.CbOffset;
			break;

		case kVRegProcAmpStandardDefCrOffset:
			*value = regs->SD.CrOffset;
			break;

		case kVRegProcAmpHDRegsInitialized:
			*value = regs->HD.initialized;
			break;

		case kVRegProcAmpHighDefContrast:
			*value = regs->HD.contrast;
			break;

		case kVRegProcAmpHighDefSaturationCb:
			*value = regs->HD.saturationCb;
			break;

		case kVRegProcAmpHighDefSaturationCr:
			*value = regs->HD.saturationCr;
			break;

		case kVRegProcAmpHighDefBrightness:
			*value = regs->HD.brightness;
			break;

		case kVRegProcAmpHighDefCbOffset:
			*value = regs->HD.CbOffset;
			break;

		case kVRegProcAmpHighDefCrOffset:
			*value = regs->HD.CrOffset;
			break;

		case kVRegProcAmpHighDefHue:
			return false;	// Not supported

		default:
			return false;	// Not supported
	}

#ifdef ENABLE_DEBUG_PRINT
	if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
		MSG("Got get of procamp vreg %d, value 0x%x\n", virtualRegisterNum, *value);
#endif
	return true;
}


/*****************************************************************************************
 *	RestoreHardwareProcampRegisters
 *****************************************************************************************/
// Call this function to restore ProcAmp registers (from virtual register)


#ifdef AJAMac  
bool MacDriver::RestoreHardwareProcampRegisters(	ULWord deviceNumber,
													NTV2DeviceID deviceID,
													VirtualProcAmpRegisters *regs,
													HardwareProcAmpRegisterImage *hwRegImage)
#else
bool RestoreHardwareProcampRegisters(	ULWord deviceNumber,
										NTV2DeviceID deviceID,
										VirtualProcAmpRegisters *regs,
										HardwareProcAmpRegisterImage *hwRegImage)
#endif
{
	ULWord regVal;
	ULWord regNum;
	
	// If SD controls initialized
	if ( GetVirtualProcampRegister( (VirtualRegisterNum)kVRegProcAmpSDRegsInitialized, &regVal, regs ) && regVal != 0)
	{
		for ( regNum = (ULWord)kVRegProcAmpStandardDefBrightness; regNum < (ULWord)kVRegProcAmpEndStandardDefRange; regNum++ )
		{
			// Fetch cached value and skip unsupported registers
			if ( GetVirtualProcampRegister( (VirtualRegisterNum)regNum, &regVal, regs ) )
			{
				if (!WriteHardwareProcampRegister( deviceNumber, deviceID, (VirtualRegisterNum)regNum, regVal, hwRegImage ))
				{
					return false;
				}
			}
		}
	}

	// If HD controls initialized
	if ( GetVirtualProcampRegister( (VirtualRegisterNum)kVRegProcAmpHDRegsInitialized, &regVal, regs ) && regVal != 0)
	{
		for ( regNum = (ULWord)kVRegProcAmpHighDefBrightness; regNum < (ULWord)kVRegProcAmpEndHighDefRange; regNum++ )
		{
			// Fetch cached value and skip unsupported registers
			if ( GetVirtualProcampRegister( (VirtualRegisterNum)regNum, &regVal, regs ) )
			{
				if (!WriteHardwareProcampRegister( deviceNumber, deviceID, (VirtualRegisterNum)regNum, regVal, hwRegImage ))
				{
					return false;
				}
			}
		}
	}
	return true;
}



/*****************************************************************************************
 *	WriteHardwareProcampRegister
 *****************************************************************************************/
// This function is necessary because:
// 1) We can't read the I2C registers and
// 2) The HD registers are spread across various I2C addresses
//  
#ifdef AJAMac  
bool MacDriver::WriteHardwareProcampRegister(	ULWord deviceNumber,
												NTV2DeviceID deviceID,
												VirtualRegisterNum virtualRegisterNum, 
												ULWord value, 
												HardwareProcAmpRegisterImage *hwRegImage)
#else
bool WriteHardwareProcampRegister(	ULWord deviceNumber,
									NTV2DeviceID deviceID,
									VirtualRegisterNum virtualRegisterNum, 
									ULWord value, 
									HardwareProcAmpRegisterImage *hwRegImage)
#endif
{
	ULWord mask;

#ifdef ENABLE_DEBUG_PRINT
	if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
		MSG("Got write to hw reg of procamp vreg %d, value 0x%x\n", virtualRegisterNum, value);
#endif

	// Fetch mask.
	switch (virtualRegisterNum) 
	{
		case kVRegProcAmpStandardDefBrightness:
		case kVRegProcAmpStandardDefContrast:
		case kVRegProcAmpStandardDefSaturation:
		case kVRegProcAmpStandardDefHue:
		case kVRegProcAmpStandardDefCbOffset:
		case kVRegProcAmpStandardDefCrOffset:
			return false;
			break;

		case kVRegProcAmpHighDefContrast:
		case kVRegProcAmpHighDefSaturationCb:
		case kVRegProcAmpHighDefSaturationCr:
		case kVRegProcAmpHighDefBrightness:
		case kVRegProcAmpHighDefCbOffset:
		case kVRegProcAmpHighDefCrOffset:
			return false;
			break;

		case kVRegProcAmpHighDefHue:		// Not supported in hardware.
			return false;

		default:						// Other non-procamp virtual regs
			return false;
	}

	value &= mask;	// Note: We may be nuking some high bits from a previously signed value to shoehorn it into a hw register

	switch (virtualRegisterNum) 
	{
		case kVRegProcAmpStandardDefBrightness:
			hwRegImage->SD.brightness = value;
			return I2CWriteDataSingle(deviceNumber, kRegADV7189BBrightness, hwRegImage->SD.brightness);

		case kVRegProcAmpStandardDefContrast:
			hwRegImage->SD.contrast= value;
			return I2CWriteDataSingle(deviceNumber, kRegADV7189BContrast, hwRegImage->SD.contrast);

		case kVRegProcAmpStandardDefSaturation:
			hwRegImage->SD.saturationCb = value;
			hwRegImage->SD.saturationCr = value;
			return I2CWriteDataDoublet(	deviceNumber, 
									kRegADV7189BSaturationCb, hwRegImage->SD.saturationCb, 
									kRegADV7189BSaturationCr, hwRegImage->SD.saturationCr);

		case kVRegProcAmpStandardDefHue:
			hwRegImage->SD.hue = value;
			return I2CWriteDataSingle(deviceNumber, kRegADV7189BHue, hwRegImage->SD.hue);

		case kVRegProcAmpStandardDefCbOffset:
			hwRegImage->SD.CbOffset = value;
			return I2CWriteDataSingle(deviceNumber, kRegADV7189BCbOffset, hwRegImage->SD.CbOffset);

		case kVRegProcAmpStandardDefCrOffset:
			hwRegImage->SD.CrOffset = value;
			return I2CWriteDataSingle(deviceNumber, kRegADV7189BCrOffset, hwRegImage->SD.CrOffset);

		// The high def regs are scattered across various I2C addresses.
		// Messy.
		// TODO: If performance response is sluggish, a possible optimization is 
		// to write only register(s) that change.  I2C writes are slow, 100uSec.
		// Maybe fast enough... 
		case kVRegProcAmpHighDefContrast:
			hwRegImage->HD.hex73 = BIT_7 | BIT_6 | (value >> 4); 
			hwRegImage->HD.hex74 &= 0x0f;		// Preserve saturation Cb portion	
			hwRegImage->HD.hex74 |= (value & 0x0f) << 4;
			return I2CWriteDataDoublet(	deviceNumber, 
									0x73, hwRegImage->HD.hex73, 
									0x74, hwRegImage->HD.hex74);

		case kVRegProcAmpHighDefSaturationCb:
			hwRegImage->HD.hex74 &= 0xf0;		// Preserve contrast portion
			hwRegImage->HD.hex74 |= value >> 6;
			hwRegImage->HD.hex75 &= 0x03;		// Preserve saturation Cr portion
			hwRegImage->HD.hex75 |= (value & 0x3f) <<2;
			return I2CWriteDataDoublet(	deviceNumber,
									0x74, hwRegImage->HD.hex74, 
									0x75, hwRegImage->HD.hex75);

		case kVRegProcAmpHighDefSaturationCr:
			hwRegImage->HD.hex75 &= 0xfc;		// Preserve Saturation Cb portion
			hwRegImage->HD.hex75 |= value >> 8;
			hwRegImage->HD.hex76 = value & 0xff;	
			return I2CWriteDataDoublet(	deviceNumber,
									0x75, hwRegImage->HD.hex75, 
									0x76, hwRegImage->HD.hex76);

		case kVRegProcAmpHighDefBrightness:
			hwRegImage->HD.hex77 = value  >> 4; // Bits 7, 6 should be clear
			hwRegImage->HD.hex78 &= 0x0f;		// Preserve Cb Offset portion
			hwRegImage->HD.hex78 |= (value & 0x0f) << 4;
			return I2CWriteDataDoublet(	deviceNumber,
									0x77, hwRegImage->HD.hex77, 
									0x78, hwRegImage->HD.hex78);

		case kVRegProcAmpHighDefCbOffset:
			hwRegImage->HD.hex78 &= 0xf0; 		// Preserve brightness portion
			hwRegImage->HD.hex78 |= value  >> 6; 
			hwRegImage->HD.hex79 &= 0x03;		// Preserve Cr Offset portion
			hwRegImage->HD.hex79 |= (value & 0x3f) << 2;
			return I2CWriteDataDoublet(	deviceNumber,
									0x78, hwRegImage->HD.hex78, 
									0x79, hwRegImage->HD.hex79);

		case kVRegProcAmpHighDefCrOffset:
			hwRegImage->HD.hex79 &= 0xfc;		// Preserve Cb offset portion 
			hwRegImage->HD.hex79 |= value  >> 8; 
			hwRegImage->HD.hex7A = value & 0xff;
			return I2CWriteDataDoublet(	deviceNumber,
									0x79, hwRegImage->HD.hex79, 
									0x7A, hwRegImage->HD.hex7A);

		case kVRegProcAmpHighDefHue: 			// Hardware doesn't support HD hue adjust
			return false;

		default:								// Other non-procamp virtual regs
			return false;
	}

	return true;
}


/*****************************************************************************************
 *	I2CWriteDataSingle
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CWriteDataSingle(ULWord deviceNumber, UByte I2CAddress, UByte I2CData)
#else
bool I2CWriteDataSingle(ULWord deviceNumber, UByte I2CAddress, UByte I2CData)
#endif
{
	ULWord I2CCommand = I2CAddress;
	bool retVal;

	if (!I2CInhibitHardwareReads(deviceNumber))
	{
		retVal = false;
		goto bail;
	}

	I2CCommand <<= 8;
	I2CCommand |= I2CData;
	
	retVal = I2CWriteData(deviceNumber, I2CCommand);

bail:
	if(!I2CEnableHardwareReads(deviceNumber))
		retVal = false;

	return retVal;
}


/*****************************************************************************************
 *	I2CInhibitHardwareReads
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CInhibitHardwareReads(ULWord deviceNumber)
#else
static bool I2CInhibitHardwareReads(ULWord deviceNumber)
#endif
{
	bool retVal = true;

	// Unspecified bad things can happen if I2C double writes are
	// interfered with.  Guard access with a mutex.
#ifdef AJALinux
	NTV2PrivateParams *pNTV2Params;

	pNTV2Params = getNTV2Params(deviceNumber);

	if(down_interruptible(&pNTV2Params->_I2CMutex))
	{
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CInhibitHardwareReads: down_interruptible() failed.\n");
		return false;	
	}
#endif

#ifdef AJAMac
	// We're assuming that this will only be called from INSIDE the IOCommandGate, so we're already protected from pre-emption
#endif


	if(!I2CWriteControl(deviceNumber, BIT(16)))		// Inhibit I2C status register reads at vertical
	{		 
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CInhibitHardwareReads: WriteControl() failed.\n");
#endif
		retVal = false;
		
		I2CWriteControl(deviceNumber, 0);			// Enable I2C status register reads at vertical
	}
	else
	{
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CInhibitHardwareReads: I2C inhibit set.\n");
#endif
	}


#ifdef AJALinux
	up(&pNTV2Params->_I2CMutex);
#endif

	return retVal;
}


/*****************************************************************************************
 *	I2CEnableHardwareReads
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CEnableHardwareReads(ULWord deviceNumber)
#else
static bool I2CEnableHardwareReads(ULWord deviceNumber)
#endif
{
	bool retVal = true;

#ifdef AJALinux
	NTV2PrivateParams *pNTV2Params;

	pNTV2Params = getNTV2Params(deviceNumber);

	if(down_interruptible(&pNTV2Params->_I2CMutex))
		return false;	
#endif

#ifdef AJAMac
	// We're assuming that this will only be called from INSIDE the IOCommandGate, so we're already protected from preemption
#endif


	if(!I2CWriteControl(deviceNumber, 0))		// Inhibit I2C status register reads at vertical
	{		 
		retVal = false;
	}

#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CEnableHardwareReads: I2C inhibit cleared.\n");
#endif

#ifdef AJALinux
	up(&pNTV2Params->_I2CMutex);
#endif

	return retVal;
}


/*****************************************************************************************
 *	I2CWriteDataDoublet
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CWriteDataDoublet(	ULWord deviceNumber,
										UByte I2CAddress1, UByte I2CData1,
										UByte I2CAddress2, UByte I2CData2)
#else
bool I2CWriteDataDoublet(	ULWord deviceNumber,
							UByte I2CAddress1, UByte I2CData1,
							UByte I2CAddress2, UByte I2CData2)
#endif
{
	bool retVal = true;
	ULWord I2CCommand;

	if (!I2CInhibitHardwareReads(deviceNumber))
	{
		retVal = false;
		goto bail;
	}

	I2CCommand = I2CAddress1;
	I2CCommand <<= 8;
	I2CCommand |= I2CData1;
	if (!I2CWriteData(deviceNumber, I2CCommand))
	{		 
		retVal = false;
		goto bail;
	}

	I2CCommand = I2CAddress2;
	I2CCommand <<= 8;
	I2CCommand |= I2CData2;
	if(!I2CWriteData(deviceNumber, I2CCommand))
	{
		retVal = false;
		goto bail;
	}

bail:
	if(!I2CEnableHardwareReads(deviceNumber))
		retVal = false;

	return retVal;
}


/*****************************************************************************************
 *	WaitForI2CReady
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::WaitForI2CReady(ULWord deviceNumber)
#else
static bool WaitForI2CReady(ULWord deviceNumber)
#endif
{
	ULWord registerValue;
	
#ifdef AJALinux
	unsigned long timeoutJiffies, deadline;	// ULWord typedef makes time_before() issue bogus warnings.
#endif

#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("WaitForI2CReady: Waiting...\n");
#endif

#ifdef AJALinux
	// Round up to granularity of HZ
	timeoutJiffies = ntv2_getRoundedUpTimeoutJiffies(10);	// Timeout after 10 msec
	deadline = jiffies + timeoutJiffies;

	// Busy wait.  If I2C busy, write should complete in about 100 usec, unless something
	// is very broken.
	do 
	{
		registerValue = ReadRegister(deviceNumber, kRegI2CWriteControl, NO_MASK, NO_SHIFT);
		if ((registerValue & BIT(31)) == 0)	// Not busy?
		{
#ifdef ENABLE_DEBUG_PRINT
			if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
				MSG("WaitForI2CReady: Got ready.\n");
#endif
			return true;
		}
	} while( time_before( jiffies, deadline));

#endif

#ifdef AJAMac
	int maxMSecs = 10;		// we'll wait a maximum of 10 msec
	do
	{
		ReadRegisterImmediate(kRegI2CWriteControl, (uint32_t *)&registerValue);
		if ((registerValue & BIT(31)) == 0)	// Not busy?
		{
//			if (maxMSecs < 10)
//				IOLog("WaitForI2CReady: waited %d msecs for I2C Ready.\n", maxMSecs);
			return true;
		}
			
		IOSleep(1);				// sleep 1 msec & try again
	} while (--maxMSecs > 0);

#endif

#ifdef ENABLE_DEBUG_PRINT
	if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
		MSG("WaitForI2CReady: Timed out.\n");
#endif

	return false;	// Timed out, + not implemented for Windows yet.
}


/*****************************************************************************************
 *	I2CWriteData
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CWriteData(ULWord deviceNumber, ULWord value)
#else
static bool I2CWriteData(ULWord deviceNumber, ULWord value)
#endif
{

		// wait for the hardware to be ready
	if (WaitForI2CReady(deviceNumber) == false)
	{
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CWriteData: First WaitForI2CReady() failed.\n");
#endif
		return false;
	}
	
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CWriteData: writing register 0x%02x = 0x%02x  (device #%d)\n", (value & 0xFF00) >> 8, (value & 0xFF), deviceNumber);
#endif

	
		// write the data to the hardware
#ifdef AJALinux
	WriteRegister(	deviceNumber,
	  				kRegI2CWriteData, 
					value,
					NO_MASK,
					NO_SHIFT);
#endif

#ifdef AJAMac
	WriteRegisterImmediate(kRegI2CWriteData, value);
#endif


		// wait for the I2C to complete writing
	if (WaitForI2CReady(deviceNumber) == false) {
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CWriteData: Second WaitForI2CReady() failed.\n");
#endif
		return false;
	}
	
#if defined(AJALinux) || defined(AJAMac)
	return true;
#else
	return false;	// Not implemented for Windows yet.
#endif
}


/*****************************************************************************************
 *	I2CWriteControl
 *****************************************************************************************/
#ifdef AJAMac
bool MacDriver::I2CWriteControl(ULWord deviceNumber, ULWord value)
#else
static bool I2CWriteControl(ULWord deviceNumber, ULWord value)
#endif
{
#ifdef ENABLE_DEBUG_PRINT
		if (MsgsEnabled(NTV2_DRIVER_I2C_DEBUG_MESSAGES))
			MSG("I2CWriteControl: writing 0x%08x to device #%d.\n", value, deviceNumber);
#endif

#ifdef AJALinux
	WriteRegister(	deviceNumber,
	  				kRegI2CWriteControl, 
					value,
					NO_MASK,
					NO_SHIFT);
	return true;
#endif

#ifdef AJAMac
	return WriteRegisterImmediate(kRegI2CWriteControl, value);
#endif


	return false;	// Not implemented for Windows yet.
}

#endif	//	defined(NTV2_BUILDING_DRIVER)
