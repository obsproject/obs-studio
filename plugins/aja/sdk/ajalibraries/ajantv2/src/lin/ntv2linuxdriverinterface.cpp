/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2linuxdriverinterface.cpp
	@brief		Implementation of the CNTV2LinuxDriverInterface class.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>

#include "ntv2linuxdriverinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h" 			// For multiple bitfile support for XenaHS
#include "ntv2nubtypes.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/process.h"


using namespace std;


//	LinuxDriverInterface Logging Macros
#define	HEX2(__x__)			"0x" << hex << setw(2)  << setfill('0') << (0xFF       & uint8_t (__x__)) << dec
#define	HEX4(__x__)			"0x" << hex << setw(4)  << setfill('0') << (0xFFFF     & uint16_t(__x__)) << dec
#define	HEX8(__x__)			"0x" << hex << setw(8)  << setfill('0') << (0xFFFFFFFF & uint32_t(__x__)) << dec
#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))

#define	LDIFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LDIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LDINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LDIINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	LDIDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


CNTV2LinuxDriverInterface::CNTV2LinuxDriverInterface()
	:	_bitfileDirectory			("../xilinx")
		,_hDevice					(INVALID_HANDLE_VALUE)
#if !defined(NTV2_DEPRECATE_16_0)
		,_pDMADriverBufferAddress	(AJA_NULL)
		,_BA0MemorySize				(0)
		,_pDNXRegisterBaseAddress	(AJA_NULL)
		,_BA2MemorySize				(0)
		,_BA4MemorySize				(0)
#endif	//	!defined(NTV2_DEPRECATE_16_0)
{
}

CNTV2LinuxDriverInterface::~CNTV2LinuxDriverInterface()
{
	if (IsOpen())
		Close();
}


/////////////////////////////////////////////////////////////////////////////////////
// Board Open / Close methods
/////////////////////////////////////////////////////////////////////////////////////
bool CNTV2LinuxDriverInterface::OpenLocalPhysical (const UWord inDeviceIndex)
{
	static const string	kAJANTV2("ajantv2");
	NTV2_ASSERT(!IsRemote());
	NTV2_ASSERT(!IsOpen());

	ostringstream oss;  oss << "/dev/" << kAJANTV2 << DEC(inDeviceIndex);
	string boardStr(oss.str());
	_hDevice = HANDLE(open(boardStr.c_str(), O_RDWR));
	if (_hDevice == INVALID_HANDLE_VALUE)
		{LDIFAIL("Failed to open '" << boardStr << "'");  return false;}

	_boardNumber = inDeviceIndex;
	const NTV2DeviceIDSet	legalDeviceIDs(::NTV2GetSupportedDevices());
	if (!CNTV2DriverInterface::ReadRegister(kRegBoardID, _boardID))
	{
		LDIFAIL ("ReadRegister failed for 'kRegBoardID': ndx=" << inDeviceIndex << " hDev=" << _hDevice << " id=" << HEX8(_boardID));
		if (!CNTV2DriverInterface::ReadRegister(kRegBoardID, _boardID))
		{
			LDIFAIL ("ReadReg retry failed for 'kRegBoardID': ndx=" << inDeviceIndex << " hDev=" << _hDevice << " id=" << HEX8(_boardID));
			Close();
			return false;
		}
		LDIDBG("Retry succeeded: ndx=" << _boardNumber << " hDev=" << _hDevice << " id=" << ::NTV2DeviceIDToString(_boardID));
	}
	if (legalDeviceIDs.find(_boardID) == legalDeviceIDs.end())
	{
		LDIFAIL("Unsupported boardID=" << HEX8(_boardID) << " ndx=" << inDeviceIndex << " hDev=" << _hDevice);
		Close();
		return false;
	}
	_boardOpened = true;
	LDIINFO ("Opened '" << boardStr << "' devID=" << HEX8(_boardID) << " ndx=" << DEC(_boardNumber));
	return true;
}


bool CNTV2LinuxDriverInterface::CloseLocalPhysical (void)
{
	NTV2_ASSERT(!IsRemote());
	NTV2_ASSERT(IsOpen());
	NTV2_ASSERT(_hDevice);

#if !defined(NTV2_DEPRECATE_16_0)
	UnmapXena2Flash();
	UnmapDMADriverBuffer();
#endif	//	!defined(NTV2_DEPRECATE_16_0)

	LDIINFO ("Closed deviceID=" << HEX8(_boardID) << " ndx=" << DEC(_boardNumber) << " hDev=" << _hDevice);
	if (_hDevice != INVALID_HANDLE_VALUE)
		close(int(_hDevice));
	_hDevice = INVALID_HANDLE_VALUE;
	_boardOpened = false;
	return true;
}


///////////////////////////////////////////////////////////////////////////////////
// Read and Write Register methods
///////////////////////////////////////////////////////////////////////////////////


bool CNTV2LinuxDriverInterface::ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask,  const ULWord inShift)
{
	if (inShift >= 32)
	{
		LDIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::ReadRegister (inRegNum, outValue, inMask, inShift);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0));

	REGISTER_ACCESS ra;
	ra.RegisterNumber = inRegNum;
	ra.RegisterMask	  = inMask;
	ra.RegisterShift  = inShift;
	ra.RegisterValue  = 0xDEADBEEF;
	if (ioctl(int(_hDevice), IOCTL_NTV2_READ_REGISTER, &ra))
	{
		LDIFAIL("IOCTL_NTV2_READ_REGISTER failed");
		return false;
	}
	outValue = ra.RegisterValue;
	return true;
}


bool CNTV2LinuxDriverInterface::WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask, const ULWord inShift)
{
	if (inShift >= 32)
	{
		LDIFAIL("Shift " << DEC(inShift) << " > 31, reg=" << DEC(inRegNum) << " msk=" << xHEX0N(inMask,8));
		return false;
	}
#if defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
	if (mRecordRegWrites)
	{
		AJAAutoLock	autoLock(&mRegWritesLock);
		mRegWrites.push_back(NTV2RegInfo(inRegNum, inValue, inMask, inShift));
		if (mSkipRegWrites)
			return true;
	}
#endif	//	defined(NTV2_WRITEREG_PROFILING)	//	Register Write Profiling
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
		return CNTV2DriverInterface::WriteRegister(inRegNum, inValue, inMask, inShift);
#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	REGISTER_ACCESS ra;
	ra.RegisterNumber	= inRegNum;
	ra.RegisterValue	= inValue;
	ra.RegisterMask		= inMask;
	ra.RegisterShift	= inShift;
	if (ioctl(int(_hDevice), IOCTL_NTV2_WRITE_REGISTER, &ra))
		{LDIFAIL("IOCTL_NTV2_WRITE_REGISTER failed");  return false;}
	return true;
}

bool CNTV2LinuxDriverInterface::RestoreHardwareProcampRegisters (void)
{
	if (IsRemote())
		return false;
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	if (ioctl(int(_hDevice), IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS))
		{LDIFAIL("IOCTL_NTV2_RESTORE_HARDWARE_PROCAMP_REGISTERS failed"); return false;}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Interrupt enabling / disabling method
/////////////////////////////////////////////////////////////////////////////

// Method:	ConfigureInterrupt
// Input:	bool bEnable (turn on/off interrupt), INTERRUPT_ENUMS eInterruptType
// Output:	bool status
// Purpose:	Provides a 1 point connection to driver for interrupt calls
bool CNTV2LinuxDriverInterface::ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	NTV2_INTERRUPT_CONTROL_STRUCT intrControlStruct;
	memset(&intrControlStruct, 0, sizeof(NTV2_INTERRUPT_CONTROL_STRUCT));	// Suppress valgrind error
	intrControlStruct.eInterruptType = eInterruptType;
	intrControlStruct.enable = bEnable;
	if (ioctl(int(_hDevice), IOCTL_NTV2_INTERRUPT_CONTROL, &intrControlStruct))
	{
		LDIFAIL("IOCTL_NTV2_INTERRUPT_CONTROL failed");
		return false;
	}
	return true;
}

// Method: getInterruptCount
// Input:  INTERRUPT_ENUMS	eInterruptType.  Currently only output vertical interrupts are supported.
// Output: ULWord or equivalent(i.e. ULWord).
bool CNTV2LinuxDriverInterface::GetInterruptCount (const INTERRUPT_ENUMS eInterruptType, ULWord & outCount)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	if (     eInterruptType != eVerticalInterrupt
		  && eInterruptType != eInput1
          && eInterruptType != eInput2
          && eInterruptType != eInput3
          && eInterruptType != eInput4
          && eInterruptType != eInput5
          && eInterruptType != eInput6
          && eInterruptType != eInput7
          && eInterruptType != eInput8
          && eInterruptType != eOutput2
          && eInterruptType != eOutput3
          && eInterruptType != eOutput4
          && eInterruptType != eOutput5
          && eInterruptType != eOutput6
          && eInterruptType != eOutput7
          && eInterruptType != eOutput8
          && eInterruptType != eAuxVerticalInterrupt
		  )
	{
		LDIFAIL("Unsupported interrupt count request. Only vertical input interrupts counted.");
		return false;
	}

	NTV2_INTERRUPT_CONTROL_STRUCT intrControlStruct;
	memset(&intrControlStruct, 0, sizeof(NTV2_INTERRUPT_CONTROL_STRUCT));// Suppress valgrind error
	intrControlStruct.eInterruptType = eGetIntCount;
	intrControlStruct.interruptCount = eInterruptType;

	if (ioctl(int(_hDevice), IOCTL_NTV2_INTERRUPT_CONTROL, &intrControlStruct))
	{
		LDIFAIL("IOCTL_NTV2_INTERRUPT_CONTROL failed");
		return false;
	}

    outCount = intrControlStruct.interruptCount;
	return true;
}

// Method: WaitForInterrupt
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool CNTV2LinuxDriverInterface::WaitForInterrupt (const INTERRUPT_ENUMS	eInterrupt, const ULWord timeOutMs)
{
	if (IsRemote())
	{
		return CNTV2DriverInterface::WaitForInterrupt(eInterrupt, timeOutMs);
	}

	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );

	NTV2_WAITFOR_INTERRUPT_STRUCT waitIntrStruct;
	waitIntrStruct.eInterruptType = eInterrupt;
	waitIntrStruct.timeOutMs = timeOutMs;
	waitIntrStruct.success = 0;	// Assume failure

	if (ioctl(int(_hDevice), IOCTL_NTV2_WAITFOR_INTERRUPT, &waitIntrStruct))
	{
		LDIFAIL("IOCTL_NTV2_WAITFOR_INTERRUPT failed");
		return false;
	}
	BumpEventCount (eInterrupt);

	return waitIntrStruct.success != 0;
}

// Method: ControlDriverDebugMessages
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool CNTV2LinuxDriverInterface::ControlDriverDebugMessages (NTV2_DriverDebugMessageSet msgSet, bool enable)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	NTV2_CONTROL_DRIVER_DEBUG_MESSAGES_STRUCT cddmStruct;
	cddmStruct.msgSet = msgSet;
	cddmStruct.enable = enable;
	if (ioctl(int(_hDevice), IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES, &cddmStruct))
	{
		LDIFAIL("IOCTL_NTV2_CONTROL_DRIVER_DEBUG_MESSAGES failed");
		return false;
	}
	return cddmStruct.success != 0;
}

// Method: SetupBoard
// Output: True on successs, false on failure (ioctl failed or interrupt didn't happen)
bool CNTV2LinuxDriverInterface::SetupBoard (void)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	if (ioctl(int(_hDevice), IOCTL_NTV2_SETUP_BOARD, 0, 0))		// Suppress valgrind errors
	{
		LDIFAIL("IOCTL_NTV2_SETUP_BOARD failed");
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// OEM Mapping to Userspace Methods
//////////////////////////////////////////////////////////////////////////////

#if !defined(NTV2_DEPRECATE_16_0)
	// Method:	MapFrameBuffers
	// Input:	None
	// Output:	bool, and sets member _pBaseFrameAddress
	bool CNTV2LinuxDriverInterface::MapFrameBuffers (void)
	{
		if (!IsOpen())
			return false;
		if (!_pFrameBaseAddress)
		{
			// Get memory window size from driver
			ULWord BA1MemorySize;
			if (!GetBA1MemorySize(&BA1MemorySize))
			{
				LDIFAIL ("MapFrameBuffers failed - couldn't get BA1MemorySize");
				return false;
			}
	
			if (BA1MemorySize == 0)
			{
				LDIFAIL ("BA1MemorySize is 0 -- module loaded with MapFrameBuffers=0?");
				LDIFAIL ("PIO mode not available, only driverbuffer DMA.");
				return false;
			}
	
			// If BA1MemorySize is 0, then the module was loaded with MapFrameBuffers=0
			// and PIO mode is not available.
	
			// Map the memory.  For Xena(da) boards, the window will be the same size as the amount of
			// memory on the Xena card.  For Xena(mm) cards, it will be a window which is selected using
			// SetPCIAccessFrame().
			//
			// the offset of 0 in the call to mmap tells mmap to map BAR1 which is the framebuffers.
			_pFrameBaseAddress = reinterpret_cast<ULWord*>(mmap(AJA_NULL, BA1MemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, int(_hDevice), 0));
			if (_pFrameBaseAddress == MAP_FAILED)
			{
				_pFrameBaseAddress = AJA_NULL;
				LDIFAIL ("MapFrameBuffers failed in call to mmap()");
				return false;
			}
	
			// Set the CH1 and CH2 frame base addresses for cards that require them.
			ULWord boardIDRegister;
			ReadRegister(kRegBoardID, boardIDRegister);	//unfortunately GetBoardID is in ntv2card...ooops.
			if ( ! ::NTV2DeviceIsDirectAddressable(NTV2DeviceID(boardIDRegister)))
				_pCh1FrameBaseAddress = _pFrameBaseAddress;
		}
		return true;
	}

	// Method:	UnmapFrameBuffers
	// Input:	None
	// Output:	bool status
	bool CNTV2LinuxDriverInterface::UnmapFrameBuffers (void)
	{
		if (!_pFrameBaseAddress)
			return true;
		if (!IsOpen())
			return false;
	
		// Get memory window size from driver
		ULWord BA1MemorySize;
		if (!GetBA1MemorySize(&BA1MemorySize))
		{
			LDIFAIL ("UnmapFrameBuffers failed - couldn't get BA1MemorySize");
			return false;
		}
		if (_pFrameBaseAddress)
			munmap(_pFrameBaseAddress, BA1MemorySize);
		_pFrameBaseAddress = AJA_NULL;
		return true;
	}

	// Method:	MapRegisters
	// Input:	None
	// Output:	bool, and sets member _pBaseFrameAddress
	bool CNTV2LinuxDriverInterface::MapRegisters (void)
	{
		if (!IsOpen())
			return false;
		if (!_pRegisterBaseAddress)
		{
			// Get register window size from driver
			if (!GetBA0MemorySize(&_BA0MemorySize))
			{
				LDIFAIL ("MapRegisters failed - couldn't get BA0MemorySize");
				_pRegisterBaseAddress = AJA_NULL;
				return false;
			}
	
			if (!_BA0MemorySize)
			{
				LDIFAIL ("BA0MemorySize is 0, registers not mapped.");
				_pRegisterBaseAddress = AJA_NULL;
				return false;
			}
	
			// the offset of 0x1000 in the call to mmap tells mmap to map BAR0 which is the registers.
			// 2.4 kernel interprets offset as number of pages, so 0x1000 works. This won't work on a 2.2
			// kernel
			_pRegisterBaseAddress = (ULWord *) mmap(AJA_NULL,_BA0MemorySize,PROT_READ | PROT_WRITE,MAP_SHARED,_hDevice,0x1000);
			if (_pRegisterBaseAddress == MAP_FAILED)
			{
				_pRegisterBaseAddress = AJA_NULL;
				return false;
			}
		}
		return true;
	}

	// Method:	UnmapRegisters
	// Input:	None
	// Output:	bool status
	bool CNTV2LinuxDriverInterface::UnmapRegisters (void)
	{
		if (!IsOpen())
			return false;
		if (!_pRegisterBaseAddress)
			return true;
		if (_pRegisterBaseAddress)
			munmap(_pRegisterBaseAddress,_BA0MemorySize);
		_pRegisterBaseAddress = AJA_NULL;
		return true;
	}

	bool CNTV2LinuxDriverInterface::GetBA0MemorySize (ULWord* memSize)
	{
		return memSize ? ReadRegister (kVRegBA0MemorySize, *memSize) : false;
	}

	bool CNTV2LinuxDriverInterface::GetBA1MemorySize (ULWord* memSize)
	{
		return memSize ? ReadRegister (kVRegBA1MemorySize, *memSize) : false;
	}

	bool CNTV2LinuxDriverInterface::GetBA2MemorySize (ULWord* memSize)
	{
		return memSize ? ReadRegister (kVRegBA2MemorySize, *memSize) : false;
	}

	bool CNTV2LinuxDriverInterface::GetBA4MemorySize (ULWord* memSize)
	{
		return memSize ? ReadRegister (kVRegBA4MemorySize, *memSize) : false;
	}

	bool CNTV2LinuxDriverInterface::MapXena2Flash (void)
	{
		if (!IsOpen())
			return false;
		ULWord BA4MemorySize;
		if (!_pXena2FlashBaseAddress)
		{
			if ( !GetBA4MemorySize(&BA4MemorySize) )
			{
				LDIFAIL ("MapXena2Flash failed - couldn't get BA4MemorySize");
				_pXena2FlashBaseAddress = AJA_NULL;
				return false;
			}
			if (!BA4MemorySize)
			{
				LDIFAIL ("MapXena2Flash failed - BA4MemorySize == 0");
				_pXena2FlashBaseAddress = AJA_NULL;
				return false;
			}
			_BA4MemorySize = BA4MemorySize;
			// 0x4000 is a page offset magic token passed into the driver mmap callback that ends up mapping the right stuff
			_pXena2FlashBaseAddress = reinterpret_cast<ULWord*>(mmap(AJA_NULL, BA4MemorySize,
																	PROT_READ | PROT_WRITE, MAP_SHARED,
																	int(_hDevice), 0x4000));
			if (_pXena2FlashBaseAddress == MAP_FAILED)
			{
				_pXena2FlashBaseAddress = AJA_NULL;
				LDIFAIL ("MapXena2Flash(): mmap of BAR4 for PCI Flash failed");
				return false;
			}
		}
		return true;
	}

	bool CNTV2LinuxDriverInterface::UnmapXena2Flash (void)
	{
		if (!_pXena2FlashBaseAddress)
			return true;
		if (!IsOpen())
			return false;
		if (_pXena2FlashBaseAddress)
		{
			munmap(_pXena2FlashBaseAddress, _BA4MemorySize);
			_BA4MemorySize = 0;
		}
		_pXena2FlashBaseAddress = AJA_NULL;
		return false;
	}

	bool CNTV2LinuxDriverInterface::MapDNXRegisters (void)
	{
		ULWord BA2MemorySize;
		if (!IsOpen())
			return false;
		if (!_pDNXRegisterBaseAddress)
		{
			if (!GetBA2MemorySize(&BA2MemorySize))
			{
				LDIFAIL ("MapDNXRegisters failed - couldn't get BA2MemorySize");
				return false;
			}
			if (!BA2MemorySize)
			{
				LDIFAIL ("MapDNXRegisters failed - BA2MemorySize == 0");
				return false;
			}
			_BA2MemorySize = BA2MemorySize;
	
			// 0x8000 is a page offset magic token passed into the driver mmap callback
			// that ends up mapping the right stuff
			_pDNXRegisterBaseAddress = reinterpret_cast<ULWord*>(mmap (AJA_NULL, BA2MemorySize,
																		PROT_READ | PROT_WRITE, MAP_SHARED,
																		int(_hDevice), 0x8000));
			if (_pDNXRegisterBaseAddress == MAP_FAILED)
			{
				_pDNXRegisterBaseAddress = AJA_NULL;
				_BA2MemorySize           = 0;
				LDIFAIL ("MapDNXRegisters failed - couldn't map BAR2");
				return false;
			}
		}
		return true;
	}

	bool CNTV2LinuxDriverInterface::UnmapDNXRegisters (void)
	{
		if (!_pDNXRegisterBaseAddress)
			return true;
		if (!IsOpen())
			return false;
		if (_pDNXRegisterBaseAddress)
		{
			munmap(_pDNXRegisterBaseAddress, _BA2MemorySize);
			_BA2MemorySize = 0;
		}
		_pDNXRegisterBaseAddress = AJA_NULL;
		return false;
	}
#endif	//	!defined(NTV2_DEPRECATE_16_0)


//////////////////////////////////////////////////////////////////////////////////////
// DMA
//
// Note: Asynchronous DMA only available with driver-allocated buffers.

bool CNTV2LinuxDriverInterface::DmaTransfer	(	const NTV2DMAEngine	inDMAEngine,
												const bool			inIsRead,
												const ULWord		inFrameNumber,
												ULWord *			pFrameBuffer,
												const ULWord		inOffsetBytes,
												const ULWord		inByteCount,
												const bool			inSynchronous)
{
	if (IsRemote())
		return CNTV2DriverInterface::DmaTransfer(inDMAEngine, inIsRead, inFrameNumber, pFrameBuffer,
												inOffsetBytes, inByteCount, inSynchronous);
	if (!IsOpen())
		return false;

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine			= inDMAEngine;
	dmaControlBuf.dmaChannel		= NTV2_CHANNEL1;
    dmaControlBuf.frameNumber		= inFrameNumber;
	dmaControlBuf.frameBuffer		= pFrameBuffer;
	dmaControlBuf.frameOffsetSrc	= inIsRead ? inOffsetBytes : 0;
	dmaControlBuf.frameOffsetDest	= inIsRead ? 0 : inOffsetBytes;
    dmaControlBuf.numBytes			= inByteCount;

	// The following are used only for driver-created buffers.
	// Set them to known values.
	dmaControlBuf.downSample		= 0;	// Not applicable to this mode
	dmaControlBuf.linePitch			= 1;	// Not applicable to this mode
	ULWord numDmaDriverBuffers;
	GetDMANumDriverBuffers(&numDmaDriverBuffers);
	if ((unsigned long)pFrameBuffer >= numDmaDriverBuffers)
	{
		// Can't poll with usermode allocated buffer
		if (!inSynchronous)
			return false;
		dmaControlBuf.poll = 0;
	}
	else
		dmaControlBuf.poll = inSynchronous;		// True == app must wait for DMA completion interrupt

	int request;
	const char *errMsg(AJA_NULL);
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (inIsRead) // Reading?
	{
		if (inOffsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_READ_FRAME;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_READ;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ);
	   }
	}
	else // Writing
	{
		if (inOffsetBytes == 0) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_WRITE_FRAME;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME);
	   }
	   else // Field 1
	   {
			request = IOCTL_NTV2_DMA_WRITE;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE);
	   }
	}

	// TODO: Stick the IOCTL code inside the dmaControlBuf and collapse 4 IOCTLs into one.
	if (!ioctl(int(_hDevice), request, &dmaControlBuf))
		return true;
	LDIFAIL(errMsg);
	return false;
}

bool CNTV2LinuxDriverInterface::DmaTransfer (const NTV2DMAEngine	inDMAEngine,
											const bool				inIsRead,
											const ULWord			inFrameNumber,
											ULWord *				pFrameBuffer,
											const ULWord			inOffsetBytes,
											const ULWord			inByteCount,
											const ULWord			inNumSegments,
											const ULWord			inHostPitch,
											const ULWord			inCardPitch,
											const bool				inIsSynchronous)
{
	if (!IsOpen())
		return false;

	LDIDBG("FRM=" << inFrameNumber << " ENG=" << inDMAEngine << " NB=" << inByteCount << (inIsRead?" Rd":" Wr"));

	// NOTE: Linux driver assumes driver buffers to be used if pFrameBuffer < numDmaDriverBuffers
    NTV2_DMA_SEGMENT_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine				= inDMAEngine;
    dmaControlBuf.frameNumber			= inFrameNumber;
	dmaControlBuf.frameBuffer			= pFrameBuffer;
	dmaControlBuf.frameOffsetSrc		= inIsRead ? inOffsetBytes : 0;
	dmaControlBuf.frameOffsetDest		= inIsRead ? 0 : inOffsetBytes;
    dmaControlBuf.numBytes				= inByteCount;
    dmaControlBuf.videoNumSegments		= inNumSegments;
    dmaControlBuf.videoSegmentHostPitch	= inHostPitch;
    dmaControlBuf.videoSegmentCardPitch	= inCardPitch;
	dmaControlBuf.poll					= 0;

	ULWord numDmaDriverBuffers(0);
	GetDMANumDriverBuffers(&numDmaDriverBuffers);
	if (ULWord(ULWord64(pFrameBuffer)) >= numDmaDriverBuffers)
	{
		if (!inIsSynchronous)
			return false;	//	Async mode requires kernel-allocated buffer
	}
	else
		dmaControlBuf.poll = inIsSynchronous;	// True == app must wait for DMA completion interrupt

	int request(0);
	const char *errMsg(AJA_NULL);
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	// TODO: Stick the IOCTL code inside the dmaControlBuf and collapse 4 IOCTLs into one.
	if (inIsRead) // Reading?
	{
		if (!inOffsetBytes) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_READ_FRAME_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME_SEGMENT);
		}
		else // Field 1
		{
			request = IOCTL_NTV2_DMA_READ_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_SEGMENT);
		}
	}
	else // Writing
	{
		if (!inOffsetBytes) // Frame ( or field 0? )
		{
			request = IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME_SEGMENT);
		}
		else // Field 1
		{
			request = IOCTL_NTV2_DMA_WRITE_SEGMENT;
			errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_SEGMENT);
		}
	}

	if (ioctl(int(_hDevice), request, &dmaControlBuf))
	{
		LDIFAIL(errMsg);
		return false;
	}
	return true;
}


bool CNTV2LinuxDriverInterface::DmaTransfer (	const NTV2DMAEngine			inDMAEngine,
												const NTV2Channel			inDMAChannel,
												const bool					inIsTarget,
												const ULWord				inFrameNumber,
												const ULWord				inCardOffsetBytes,
												const ULWord				inByteCount,
												const ULWord				inNumSegments,
												const ULWord				inSegmentHostPitch,
												const ULWord				inSegmentCardPitch,
												const PCHANNEL_P2P_STRUCT &	inP2PData)
{
	if (!IsOpen())
		return false;
	if (IsRemote())
		return CNTV2DriverInterface::DmaTransfer (inDMAEngine, inDMAChannel, inIsTarget, inFrameNumber, inCardOffsetBytes, inByteCount,
													inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inP2PData);
	if (!inP2PData)
	{
		LDIFAIL( "P2PData is NULL" );
		return false;
	}

	// Information to be sent to the driver
	NTV2_DMA_P2P_CONTROL_STRUCT dmaP2PStruct;
	memset ((void*)&dmaP2PStruct, 0, sizeof(dmaP2PStruct));
	if (inIsTarget)
	{
		// reset info to be passed back to the user
		memset ((void*)inP2PData, 0, sizeof(CHANNEL_P2P_STRUCT));
		inP2PData->p2pSize = sizeof(CHANNEL_P2P_STRUCT);
	}
	else
	{
		// check for valid p2p struct
		if (inP2PData->p2pSize != sizeof(CHANNEL_P2P_STRUCT))
		{
			LDIFAIL("p2pSize=" << DEC(inP2PData->p2pSize) << " != sizeof(CHANNEL_P2P_STRUCT) " << DEC(sizeof(CHANNEL_P2P_STRUCT)));
			return false;
		}
	}

	dmaP2PStruct.bRead					= inIsTarget;
	dmaP2PStruct.dmaEngine				= inDMAEngine;
	dmaP2PStruct.dmaChannel				= inDMAChannel;
	dmaP2PStruct.ulFrameNumber			= inFrameNumber;
	dmaP2PStruct.ulFrameOffset			= inCardOffsetBytes;
	dmaP2PStruct.ulVidNumBytes			= inByteCount;
	dmaP2PStruct.ulVidNumSegments		= inNumSegments;
	dmaP2PStruct.ulVidSegmentHostPitch	= inSegmentHostPitch;
	dmaP2PStruct.ulVidSegmentCardPitch	= inSegmentCardPitch;
	dmaP2PStruct.ullVideoBusAddress		= inP2PData->videoBusAddress;
	dmaP2PStruct.ullMessageBusAddress	= inP2PData->messageBusAddress;
	dmaP2PStruct.ulVideoBusSize			= inP2PData->videoBusSize;
	dmaP2PStruct.ulMessageData			= inP2PData->messageData;
	if (ioctl(int(_hDevice), IOCTL_NTV2_DMA_P2P, &dmaP2PStruct))
	{
		LDIFAIL("IOCTL error");
		return false;
	}

	// fill in p2p data
	inP2PData->videoBusAddress		= dmaP2PStruct.ullVideoBusAddress;
	inP2PData->messageBusAddress	= dmaP2PStruct.ullMessageBusAddress;
	inP2PData->videoBusSize			= dmaP2PStruct.ulVideoBusSize;
	inP2PData->messageData			= dmaP2PStruct.ulMessageData;
	return true;
}

#define AsFrameStampStructPtr(_p_)	reinterpret_cast<FRAME_STAMP_STRUCT*>(_p_)
#define AsStatusStructPtr(_p_)		reinterpret_cast<AUTOCIRCULATE_STATUS_STRUCT*>(_p_)
#define AsTransferStatusStruct(_p_)	reinterpret_cast<PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT>(_p_)
#define AsRoutingTablePtr(_p_)		reinterpret_cast<NTV2RoutingTable*>(_p_)
#define AsPTaskStruct(_p_)			reinterpret_cast<PAUTOCIRCULATE_TASK_STRUCT>(_p_)
#define AsPTransferStruct(_p_)		reinterpret_cast<PAUTOCIRCULATE_TRANSFER_STRUCT>(_p_)


///////////////////////////////////////////////////////////////////////////
// AutoCirculate
bool CNTV2LinuxDriverInterface::AutoCirculate (AUTOCIRCULATE_DATA & autoCircData)
{
	if (IsRemote())
		return CNTV2DriverInterface::AutoCirculate(autoCircData);
	if (!IsOpen())
		return false;

	switch (autoCircData.eCommand)
	{
		case eInitAutoCirc:
		case eStartAutoCirc:
		case eStopAutoCirc:
		case eAbortAutoCirc:
		case ePauseAutoCirc:
		case eFlushAutoCirculate:
		case ePrerollAutoCirculate:
			// Pass the autoCircData structure to the driver.
			// The driver knows the implicit meanings of the
			// members of the structure based on the the
			// command contained within it.
			if(ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_CONTROL, &autoCircData))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_CONTROL failed");  return false;}
			return true;

		case eGetAutoCirc:
			// Pass the autoCircStatus structure to the driver.
			// It will read the channel spec contained within and
			// fill out the status structure accordingly.
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_STATUS, AsStatusStructPtr(autoCircData.pvVal1)))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_STATUS, failed");  return false;}
			return true;

		case eGetFrameStamp:
		{
			// Pass the frameStamp structure to the driver.
			// It will read the channel spec and frame number
			// contained within and fill out the status structure
			// accordingly.
			AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
			memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
			FRAME_STAMP_STRUCT* pFrameStamp = AsFrameStampStructPtr(autoCircData.pvVal1);
			acFrameStampCombo.acFrameStamp = *pFrameStamp;
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP, &acFrameStampCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP failed");  return false;}
			*pFrameStamp = acFrameStampCombo.acFrameStamp;
			return true;
		}

		case eGetFrameStampEx2:
		{
			// Pass the frameStamp structure to the driver.
			// It will read the channel spec and frame number
			// contained within and fill out the status structure
			// accordingly.
			AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
			memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
			FRAME_STAMP_STRUCT* pFrameStamp = AsFrameStampStructPtr(autoCircData.pvVal1);
			PAUTOCIRCULATE_TASK_STRUCT pTask = AsPTaskStruct(autoCircData.pvVal2);
			acFrameStampCombo.acFrameStamp = *pFrameStamp;
			if (pTask)
				acFrameStampCombo.acTask = *pTask;
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP, &acFrameStampCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP failed");  return false;}
			*pFrameStamp = acFrameStampCombo.acFrameStamp;
			if (pTask)
				*pTask = acFrameStampCombo.acTask;
			return true;
		}

		case eTransferAutoCirculate:
		{
			PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer = AsPTransferStruct(autoCircData.pvVal1);
			// If doing audio, insure buffer alignment is OK
			if (acTransfer->audioBufferSize)
			{
				if (acTransfer->audioBufferSize % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer size not mod 4");  return false;}
				ULWord numDmaDriverBuffers;
				GetDMANumDriverBuffers(&numDmaDriverBuffers);
				if ((unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers  &&  (unsigned long)acTransfer->audioBuffer % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer address not mod 4");  return false;}
			}

			// Can't pass multiple pointers in a single ioctl, so combine
			// them into a single structure and include channel spec too.
			AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
			memset((void*)&acXferCombo, 0, sizeof acXferCombo);
			PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = AsTransferStatusStruct(autoCircData.pvVal2);
			NTV2RoutingTable	*pXena2RoutingTable = AsRoutingTablePtr(autoCircData.pvVal3);
			acXferCombo.channelSpec = autoCircData.channelSpec;
			acXferCombo.acTransfer = *acTransfer;
			acXferCombo.acStatus = *acStatus;
			if (!pXena2RoutingTable)
				memset(&acXferCombo.acXena2RoutingTable, 0, sizeof(acXferCombo.acXena2RoutingTable));
			else
				acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;

			// Do the transfer
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_TRANSFER, &acXferCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");  return false;}
			// Copy the results back into the status buffer we were given
			*acStatus = acXferCombo.acStatus;
			return true;
		}

		case eTransferAutoCirculateEx:
		{
			PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer = AsPTransferStruct(autoCircData.pvVal1);
			// If doing audio, insure buffer alignment is OK
			if (acTransfer->audioBufferSize)
			{
				if (acTransfer->audioBufferSize % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer size not mod 4");  return false;}

				ULWord numDmaDriverBuffers;
				GetDMANumDriverBuffers(&numDmaDriverBuffers);
				if ((unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers  &&  (unsigned long)acTransfer->audioBuffer % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer address not mod 4");  return false;}
			}

			// Can't pass multiple pointers in a single ioctl, so combine
			// them into a single structure and include channel spec too.
			AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
			memset((void*)&acXferCombo, 0, sizeof acXferCombo);
			PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = AsTransferStatusStruct(autoCircData.pvVal2);
			NTV2RoutingTable	*pXena2RoutingTable = AsRoutingTablePtr(autoCircData.pvVal3);
			acXferCombo.channelSpec = autoCircData.channelSpec;
			acXferCombo.acTransfer = *acTransfer;
			acXferCombo.acStatus = *acStatus;
			if (!pXena2RoutingTable)
				memset(&acXferCombo.acXena2RoutingTable, 0, sizeof(acXferCombo.acXena2RoutingTable));
			else
				acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;

			// Do the transfer
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_TRANSFER, &acXferCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");  return false;}
			// Copy the results back into the status buffer we were given
			*acStatus = acXferCombo.acStatus;
			return true;
		}

		case eTransferAutoCirculateEx2:
		{
			PAUTOCIRCULATE_TRANSFER_STRUCT acTransfer = AsPTransferStruct(autoCircData.pvVal1);
			// If doing audio, insure buffer alignment is OK
			if (acTransfer->audioBufferSize)
			{
				if (acTransfer->audioBufferSize % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer size not mod 4");  return false;}

				ULWord numDmaDriverBuffers;
				GetDMANumDriverBuffers(&numDmaDriverBuffers);
				if ((unsigned long)acTransfer->audioBuffer >=  numDmaDriverBuffers  &&  (unsigned long)acTransfer->audioBuffer % 4)
					{LDIFAIL ("TransferAutoCirculate failed - audio buffer address not mod 4");  return false;}
			}

			// Can't pass multiple pointers in a single ioctl, so combine
			// them into a single structure and include channel spec too.
			AUTOCIRCULATE_TRANSFER_COMBO_STRUCT acXferCombo;
			memset((void*)&acXferCombo, 0, sizeof acXferCombo);
			PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT acStatus = AsTransferStatusStruct(autoCircData.pvVal2);
			NTV2RoutingTable *	pXena2RoutingTable = AsRoutingTablePtr(autoCircData.pvVal3);
			PAUTOCIRCULATE_TASK_STRUCT pTask = AsPTaskStruct(autoCircData.pvVal4);
			acXferCombo.channelSpec = autoCircData.channelSpec;
			acXferCombo.acTransfer = *acTransfer;
			acXferCombo.acStatus = *acStatus;
			if (pXena2RoutingTable)
				acXferCombo.acXena2RoutingTable = *pXena2RoutingTable;
			if (pTask)
				acXferCombo.acTask = *pTask;

			// Do the transfer
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_TRANSFER, &acXferCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_TRANSFER failed");  return false;}
			// Copy the results back into the status buffer we were given
			*acStatus = acXferCombo.acStatus;
			return true;
		}

		case eSetCaptureTask:
		{
			AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT acFrameStampCombo;
			memset(&acFrameStampCombo, 0, sizeof acFrameStampCombo);
			PAUTOCIRCULATE_TASK_STRUCT pTask = AsPTaskStruct(autoCircData.pvVal1);
			acFrameStampCombo.acFrameStamp.channelSpec = autoCircData.channelSpec;
			acFrameStampCombo.acTask = *pTask;
			if (ioctl(int(_hDevice), IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK, &acFrameStampCombo))
				{LDIFAIL("IOCTL_NTV2_AUTOCIRCULATE_CAPTURETASK failed");  return false;}
			return true;
		}

		default:
			LDIFAIL("Unsupported AC command type in AutoCirculate()");
			break;
	}	//	switch
	return false;
}


bool CNTV2LinuxDriverInterface::NTV2Message (NTV2_HEADER * pInMessage)
{
	if (!pInMessage)
		return false;	//	NULL message pointer

	if (IsRemote())
		return CNTV2DriverInterface::NTV2Message(pInMessage);	//	Implement NTV2Message on nub

	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	if (ioctl(int(_hDevice), IOCTL_AJANTV2_MESSAGE, pInMessage))
	{
		LDIFAIL("IOCTL_AJANTV2_MESSAGE failed");
		return false;
	}
	return true;
}

bool CNTV2LinuxDriverInterface::HevcSendMessage (HevcMessageHeader* pMessage)
{
	NTV2_ASSERT( (_hDevice != INVALID_HANDLE_VALUE) && (_hDevice != 0) );
	return ioctl(int(_hDevice), IOCTL_HEVC_MESSAGE, pMessage) ? false : true;
}


bool CNTV2LinuxDriverInterface::GetDMADriverBufferPhysicalAddress (ULWord* physAddr)
{
	return physAddr ? ReadRegister (kVRegDMADriverBufferPhysicalAddress, *physAddr) : false;
}

bool CNTV2LinuxDriverInterface::GetDMANumDriverBuffers (ULWord* pNumDmaDriverBuffers)
{
	return pNumDmaDriverBuffers ? ReadRegister (kVRegNumDmaDriverBuffers, *pNumDmaDriverBuffers) : false;
}

bool CNTV2LinuxDriverInterface::SetAudioOutputMode (NTV2_GlobalAudioPlaybackMode mode)
{
	return WriteRegister(kVRegGlobalAudioPlaybackMode,mode);
}

bool CNTV2LinuxDriverInterface::GetAudioOutputMode (NTV2_GlobalAudioPlaybackMode* mode)
{
	return mode ? CNTV2DriverInterface::ReadRegister(kVRegGlobalAudioPlaybackMode, *mode) : false;
}

// Method: MapDMADriverBuffer(Maps 8 Frames worth of memory from kernel space to user space.
// Input:
// Output:
bool CNTV2LinuxDriverInterface::MapDMADriverBuffer (void)
{
	if (!_pDMADriverBufferAddress)
	{
		ULWord numDmaDriverBuffers;
		if (!GetDMANumDriverBuffers(&numDmaDriverBuffers))
		{
			LDIFAIL("GetDMANumDriverBuffers() failed");
			return false;
		}

		if (!numDmaDriverBuffers)
		{
			LDIFAIL("numDmaDriverBuffers == 0");
			return false;
		}

		// the offset of 0x2000 in the call to mmap tells mmap to map the DMA Buffer into user space
		// 2.4 kernel interprets offset as number of pages, so 0x2000 works. This won't work on a 2.2
		// kernel
		_pDMADriverBufferAddress = reinterpret_cast<ULWord*>(mmap (AJA_NULL, GetFrameBufferSize() * numDmaDriverBuffers,
																	PROT_READ | PROT_WRITE,MAP_SHARED,
																	_hDevice, 0x2000));
		if (_pDMADriverBufferAddress == MAP_FAILED)
		{
			_pDMADriverBufferAddress = AJA_NULL;
			return false;
		}
	}
	return true;
}

bool CNTV2LinuxDriverInterface::GetDMADriverBufferAddress (ULWord** pDMADriverBufferAddress)
{
	if (!_pDMADriverBufferAddress)
		if (!MapDMADriverBuffer())
			return false;
	*pDMADriverBufferAddress = _pDMADriverBufferAddress;
	return true;
}

// Method: UnmapDMADriverBuffer
// Input:  NONE
// Output: NONE
bool CNTV2LinuxDriverInterface::UnmapDMADriverBuffer (void)
{
	if (_pDMADriverBufferAddress)
	{
		ULWord numDmaDriverBuffers;
		if (!GetDMANumDriverBuffers(&numDmaDriverBuffers))
		{
			LDIFAIL("GetDMANumDriverBuffers() failed");
			return false;
		}
		if (!numDmaDriverBuffers)
		{

			LDIFAIL("numDmaDriverBuffers == 0");
			return false;
		}
		munmap(_pDMADriverBufferAddress, GetFrameBufferSize() * numDmaDriverBuffers);
	}
	_pDMADriverBufferAddress = AJA_NULL;
	return true;
}

///////
// Method: DmaBufferWriteFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0 .. NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaWriteFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame, ULWord bytes, ULWord poll)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
    dmaControlBuf.frameBuffer = PULWord(dmaBufferFrame);
    dmaControlBuf.frameOffsetSrc = 0;
    dmaControlBuf.frameOffsetDest = 0;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = 0;
	dmaControlBuf.linePitch = 0;
	dmaControlBuf.poll = poll;
	if (ioctl(int(_hDevice), IOCTL_NTV2_DMA_WRITE_FRAME, &dmaControlBuf))
		{LDIFAIL("IOCTL_NTV2_DMA_WRITE_FRAME failed");  return false;}
	return true;
}

//
///////
// Method: DmaBufferWriteFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaWriteFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame, ULWord offsetSrc, ULWord offsetDest, ULWord bytes, ULWord poll)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = (PULWord)dmaBufferFrame;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = 0;
	dmaControlBuf.linePitch = 0;
	dmaControlBuf.poll = poll;
	if (ioctl(int(_hDevice), IOCTL_NTV2_DMA_WRITE_FRAME, &dmaControlBuf))
		{LDIFAIL("IOCTL_NTV2_DMA_WRITE_FRAME failed");  return false;}
	return true;
}


// Method: DmaBufferReadFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame, ULWord bytes, ULWord downSample, ULWord linePitch, ULWord poll)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = PULWord(dmaBufferFrame);
    dmaControlBuf.frameOffsetSrc = 0;
    dmaControlBuf.frameOffsetDest = 0;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = downSample;
	if (linePitch == 0) linePitch = 1;
	dmaControlBuf.linePitch = linePitch;
	dmaControlBuf.poll = poll;

	static bool bPrintedDownsampleDeprecatedMsg = false;
	if (downSample && !bPrintedDownsampleDeprecatedMsg)
		{LDIWARN("downSample is deprecated"); bPrintedDownsampleDeprecatedMsg = true;}

	if (ioctl(int(_hDevice), IOCTL_NTV2_DMA_READ_FRAME, &dmaControlBuf))
		{LDIFAIL("IOCTL_NTV2_DMA_READ_FRAME failed");  return false;}
	return true;
}

// Method: DmaBufferReadFrameDriverBuffer
// NTV2DMAEngine - DMAEngine
// ULWord frameNumber(0-NUM_FRAMEBUFFERS-1)
// ULWord dmaBufferFrame(0 .. numDmaDriverBuffers-1)
// ULWord bytes - number of bytes to dma
// ULWord poll - 0=block 1=return immediately and poll
//              via register 48
// When the board is opened the driver allocates
// a user-definable number of frames for dmaing
// This allows dma's to be done without scatter/gather
// which should help performance.
bool CNTV2LinuxDriverInterface::DmaReadFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame,
															ULWord offsetSrc, ULWord offsetDest, ULWord bytes,
															ULWord downSample, ULWord linePitch, ULWord poll)
{
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = PULWord(dmaBufferFrame);
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;
	dmaControlBuf.downSample = downSample;
	if( linePitch == 0 ) linePitch = 1;
	dmaControlBuf.linePitch = linePitch;
	dmaControlBuf.poll = poll;

	static bool bPrintedDownsampleDeprecatedMsg = false;
	if (downSample && !bPrintedDownsampleDeprecatedMsg)
		{LDIWARN("downSample is deprecated");  bPrintedDownsampleDeprecatedMsg = true;}

	if (ioctl(int(_hDevice), IOCTL_NTV2_DMA_READ_FRAME, &dmaControlBuf))
		{LDIFAIL("IOCTL_NTV2_DMA_READ_FRAME failed");  return false;}
	return true;
}

bool CNTV2LinuxDriverInterface::DmaWriteWithOffsets (NTV2DMAEngine DMAEngine, ULWord frameNumber, ULWord * pFrameBuffer,
													ULWord offsetSrc, ULWord offsetDest, ULWord bytes)
{
	// return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, (ULWord) 0, bytes, bSync);
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;
	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers
    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;

	// The following are used only for driver-created buffers.
	// Set them to known values.
	dmaControlBuf.downSample = 0;       // Not applicable to this mode
	dmaControlBuf.linePitch = 1;        // Not applicable to this mode
	dmaControlBuf.poll = 0;             // currently can't poll with a usermode allocated dma buffer

	ULWord request;
	const char *errMsg = AJA_NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (offsetSrc == 0 && offsetDest == 0) // Frame ( or field 0? )
	{
		request = IOCTL_NTV2_DMA_WRITE_FRAME;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE_FRAME);
	}
	else // Field 1 or audio
	{
		request = IOCTL_NTV2_DMA_WRITE;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_WRITE);
	}

	if (ioctl(int(_hDevice), request, &dmaControlBuf))
		{LDIFAIL(errMsg);  return false;}
	return true;
}

bool CNTV2LinuxDriverInterface::DmaReadWithOffsets (NTV2DMAEngine DMAEngine, ULWord frameNumber, ULWord * pFrameBuffer,
													ULWord offsetSrc, ULWord offsetDest, ULWord bytes)
{
	// return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, (ULWord) 0, bytes, bSync);
	if (IsRemote())
		return false;
	if (!IsOpen())
		return false;

	// NOTE: Linux driver assumes driver buffers to be used if
	// pFrameBuffer < numDmaDriverBuffers
    NTV2_DMA_CONTROL_STRUCT dmaControlBuf;
    dmaControlBuf.engine = DMAEngine;
	dmaControlBuf.dmaChannel = NTV2_CHANNEL1;
    dmaControlBuf.frameNumber = frameNumber;
	dmaControlBuf.frameBuffer = pFrameBuffer;
    dmaControlBuf.frameOffsetSrc = offsetSrc;
    dmaControlBuf.frameOffsetDest = offsetDest;
    dmaControlBuf.numBytes = bytes;

	// The following are used only for driver-created buffers.
	// Set them to known values.
	dmaControlBuf.downSample = 0;       // Not applicable to this mode
	dmaControlBuf.linePitch = 1;        // Not applicable to this mode
	dmaControlBuf.poll = 0;             // currently can't poll with a usermode allocated dma buffer
	ULWord request;
	const char *errMsg = AJA_NULL;
#define ERRMSG(s) #s " failed"

	// Usermode buffer stuff
	if (offsetSrc == 0 && offsetDest == 0) // Frame ( or field 0? )
	{
		request = IOCTL_NTV2_DMA_READ_FRAME;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_READ_FRAME);
	}
	else // Field 1 or audio
	{
		request = IOCTL_NTV2_DMA_READ;
		errMsg = ERRMSG(IOCTL_NTV2_DMA_READ);
	}

	if (ioctl(int(_hDevice), request, &dmaControlBuf))
		{LDIFAIL(errMsg);  return false;}
	return true;
#undef ERRMSG
}
