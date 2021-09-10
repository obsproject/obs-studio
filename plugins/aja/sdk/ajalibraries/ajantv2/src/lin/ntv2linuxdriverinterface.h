/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2linuxdriverinterface.h
	@brief		Declares the CNTV2LinuxDriverInterface class.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/
#ifndef NTV2LINUXDRIVERINTERFACE_H
#define NTV2LINUXDRIVERINTERFACE_H

#include "ntv2driverinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>

#define CopyMemory(a,b,c) memcpy((a),(b),(c))

// oem dma: save locked pages in a stl::vector
#include <vector>
typedef std::vector<ULWord *> DMA_LOCKED_VEC;

/**
	@brief	Linux implementation of CNTV2DriverInterface.
**/
class CNTV2LinuxDriverInterface : public CNTV2DriverInterface
{
	public:
							CNTV2LinuxDriverInterface();
		AJA_VIRTUAL			~CNTV2LinuxDriverInterface();

		AJA_VIRTUAL bool	WriteRegister (const ULWord inRegNum, const ULWord inValue, const ULWord inMask = 0xFFFFFFFF, const ULWord inShift = 0);
		AJA_VIRTUAL bool	ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask = 0xFFFFFFFF, const ULWord inShift = 0);
#if !defined(NTV2_DEPRECATE_14_3)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0x0))
												{return pOutValue && ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift);}
#endif	//	!defined(NTV2_DEPRECATE_14_3)

		AJA_VIRTUAL bool	RestoreHardwareProcampRegisters (void);

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inTotalByteCount,
										const bool				inSynchronous = true);

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inCardOffsetBytes,
										const ULWord			inTotalByteCount,
										const ULWord			inNumSegments,
										const ULWord			inHostPitchPerSeg,
										const ULWord			inCardPitchPerSeg,
										const bool				inSynchronous = true);

	AJA_VIRTUAL bool		DmaTransfer (const NTV2DMAEngine		inDMAEngine,
										const NTV2Channel			inDMAChannel,
										const bool					inTarget,
										const ULWord				inFrameNumber,
										const ULWord				inCardOffsetBytes,
										const ULWord				inByteCount,
										const ULWord				inNumSegments,
										const ULWord				inSegmentHostPitch,
										const ULWord				inSegmentCardPitch,
										const PCHANNEL_P2P_STRUCT &	inP2PData);

	AJA_VIRTUAL bool ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS eInterruptType, PULWord & hSubcription)
											{(void)bSubscribe; (void)eInterruptType; (void)hSubcription; return true;}
	AJA_VIRTUAL bool ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType);
	AJA_VIRTUAL bool GetInterruptCount (const INTERRUPT_ENUMS eInterrupt, ULWord & outCount);
    AJA_VIRTUAL bool WaitForInterrupt (INTERRUPT_ENUMS eInterrupt, ULWord timeOutMs = 68);	// default of 68 ms timeout is enough time for 2K at 14.98 HZ

	AJA_VIRTUAL bool AutoCirculate (AUTOCIRCULATE_DATA &autoCircData);
	AJA_VIRTUAL bool NTV2Message (NTV2_HEADER * pInOutMessage);
	AJA_VIRTUAL bool ControlDriverDebugMessages(NTV2_DriverDebugMessageSet msgSet,
		  							bool enable);
	AJA_VIRTUAL bool HevcSendMessage(HevcMessageHeader* pMessage);

	AJA_VIRTUAL bool SetupBoard(void);

	// Driver allocated buffer (DMA performance enhancement, requires
	// bigphysarea patch to kernel)
	AJA_VIRTUAL bool MapDMADriverBuffer();
	AJA_VIRTUAL bool UnmapDMADriverBuffer();

	AJA_VIRTUAL bool DmaReadFrameDriverBuffer (	NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame,
												ULWord bytes,
												ULWord downSample,
												ULWord linePitch,
												ULWord poll);

	AJA_VIRTUAL bool DmaReadFrameDriverBuffer (	NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame,
												ULWord offsetSrc,
												ULWord offsetDest,
												ULWord bytes,
												ULWord downSample,
												ULWord linePitch,
												ULWord poll);

	AJA_VIRTUAL bool DmaWriteFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame,
												ULWord bytes, ULWord poll);
	AJA_VIRTUAL bool DmaWriteFrameDriverBuffer (NTV2DMAEngine DMAEngine, ULWord frameNumber, unsigned long dmaBufferFrame,
												ULWord offsetSrc, ULWord offsetDest, ULWord bytes, ULWord poll);

	// User allocated buffer methods.  Not as fast as driverbuffer methods, but no kernel patch required.
	AJA_VIRTUAL bool DmaWriteWithOffsets (NTV2DMAEngine DMAEngine, ULWord frameNumber, ULWord * pFrameBuffer,
											ULWord offsetSrc, ULWord offsetDest, ULWord bytes);
    AJA_VIRTUAL bool DmaReadWithOffsets (NTV2DMAEngine DMAEngine, ULWord frameNumber, ULWord * pFrameBuffer,
											ULWord offsetSrc, ULWord offsetDest, ULWord bytes);

public:
#if !defined(NTV2_DEPRECATE_13_0)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetRelativeVideoPlaybackDelay (ULWord frmDelay))	{(void)frmDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetRelativeVideoPlaybackDelay (ULWord* frmDelay)){(void)frmDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetAudioPlaybackPinDelay (ULWord msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetAudioPlaybackPinDelay (ULWord* msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetAudioRecordPinDelay (ULWord msDelay))		{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetAudioRecordPinDelay (ULWord* msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
#endif	//	!defined(NTV2_DEPRECATE_13_0)
#if !defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetStreamingApplication(ULWord & outAppType, int32_t & outPID))	{return CNTV2DriverInterface::GetStreamingApplication(outAppType,outPID);}	///< @deprecated	Deprecated starting in SDK 16.0.
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetStreamingApplication(ULWord * pAppType, int32_t * pPID))		{return pAppType && pPID ? CNTV2DriverInterface::GetStreamingApplication(*pAppType,*pPID) : false;}	///< @deprecated	Deprecated starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapFrameBuffers(void));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapFrameBuffers(void));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapRegisters(void));			///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapRegisters(void));		///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool GetBA0MemorySize(ULWord* memSize));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool GetBA1MemorySize(ULWord* memSize));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool GetBA2MemorySize(ULWord* memSize));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool GetBA4MemorySize(ULWord* memSize));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapXena2Flash(void));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapXena2Flash(void));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapDNXRegisters(void));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapDNXRegisters(void));///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL bool GetDMADriverBufferPhysicalAddress(ULWord* physAddr);	// Supported!
	AJA_VIRTUAL bool GetDMADriverBufferAddress(ULWord** pDMADriverBuffer);	// Supported!
	AJA_VIRTUAL bool GetDMANumDriverBuffers(ULWord* pNumDmaDriverBuffers);	// Supported!
	AJA_VIRTUAL bool SetAudioOutputMode(NTV2_GlobalAudioPlaybackMode mode);	// Supported!
	AJA_VIRTUAL bool GetAudioOutputMode(NTV2_GlobalAudioPlaybackMode* mode);// Supported!


protected:	//	PRIVATE METHODS
	AJA_VIRTUAL bool	OpenLocalPhysical (const UWord inDeviceIndex);	///< @brief	Opens the local/physical device connection.
	AJA_VIRTUAL bool	CloseLocalPhysical	(void);

protected:	//	INSTANCE DATA
 	std::string		_bitfileDirectory;
	HANDLE			_hDevice;
#if !defined(NTV2_DEPRECATE_16_0)
	ULWord *		_pDMADriverBufferAddress;
	ULWord			_BA0MemorySize;
	ULWord *		_pDNXRegisterBaseAddress;
	ULWord			_BA2MemorySize;
	ULWord			_BA4MemorySize;	         //	XENA2 only
#endif	//	!defined(NTV2_DEPRECATE_16_0)
};	//	CNTV2LinuxDriverInterface

#endif	//	NTV2LINUXDRIVERINTERFACE_H
