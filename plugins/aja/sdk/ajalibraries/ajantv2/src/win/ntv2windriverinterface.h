/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2windriverinterface.h
	@brief		Declares the MSWindows-specific flavor of CNTV2DriverInterface.
	@copyright	(C) 2003-2021 AJA Video Systems, Inc.
**/
#ifndef NTV2WINDRIVERINTERFACE_H
#define NTV2WINDRIVERINTERFACE_H

#include "ajaexport.h"
#include "ajatypes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#include <ks.h>
#include <ksmedia.h>
#include <Setupapi.h>
#include <cfgmgr32.h>
#include "psapi.h"

#include "ntv2driverinterface.h"
#include "ntv2winpublicinterface.h"
#include "ntv2devicefeatures.h"
#include <vector>


/**
	@brief	Physical device implementations of ::CNTV2DriverInterface methods through AJA Windows driver.
**/
class AJAExport CNTV2WinDriverInterface : public CNTV2DriverInterface
{
	public:
					CNTV2WinDriverInterface();	///< @brief	My default constructor.
		virtual		~CNTV2WinDriverInterface();	///< @brief	My default destructor.

	public:
		AJA_VIRTUAL bool	WriteRegister (const ULWord inRegNum,  const ULWord inValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);	///< @brief	Physical device implementation of CNTV2DriverInterface::WriteRegister.
		AJA_VIRTUAL bool	ReadRegister (const ULWord inRegNum,  ULWord & outValue,  const ULWord inMask = 0xFFFFFFFF,  const ULWord inShift = 0);	///< @brief	Physical device implementation of CNTV2DriverInterface::ReadRegister.
#if !defined(NTV2_DEPRECATE_14_3)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadRegister (const ULWord inRegNum, ULWord * pOutValue, const ULWord inRegMask = 0xFFFFFFFF, const ULWord inRegShift = 0x0))
															{return pOutValue && ReadRegister(inRegNum, *pOutValue, inRegMask, inRegShift);}
#endif	//	NTV2_DEPRECATE_14_3
		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inByteCount,
										const bool				inSynchronous = true);

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine	inDMAEngine,
										const bool				inIsRead,
										const ULWord			inFrameNumber,
										ULWord *				pFrameBuffer,
										const ULWord			inCardOffsetBytes,
										const ULWord			inByteCount,
										const ULWord			inNumSegments,
										const ULWord			inSegmentHostPitch,
										const ULWord			inSegmentCardPitch,
										const bool				inSynchronous = true);

		AJA_VIRTUAL bool	DmaTransfer (const NTV2DMAEngine		inDMAEngine,
										const NTV2Channel			inDMAChannel,
										const bool					inIsTarget,
										const ULWord				inFrameNumber,
										const ULWord				inCardOffsetBytes,
										const ULWord				inByteCount,
										const ULWord				inNumSegments,
										const ULWord				inSegmentHostPitch,
										const ULWord				inSegmentCardPitch,
										const PCHANNEL_P2P_STRUCT &	inP2PData);

		AJA_VIRTUAL bool	ConfigureInterrupt (const bool bEnable, const INTERRUPT_ENUMS eInterruptType);
		AJA_VIRTUAL bool	ConfigureSubscription (const bool bSubscribe, const INTERRUPT_ENUMS eInterruptType, PULWord & hSubcription);
		AJA_VIRTUAL bool	GetInterruptCount (const INTERRUPT_ENUMS eInterrupt, ULWord & outCount);
		AJA_VIRTUAL bool	WaitForInterrupt (const INTERRUPT_ENUMS eInterruptType, const ULWord timeOutMs = 50);

		AJA_VIRTUAL bool	AutoCirculate (AUTOCIRCULATE_DATA &autoCircData);
		AJA_VIRTUAL bool	NTV2Message (NTV2_HEADER * pInMessage);
		AJA_VIRTUAL bool	HevcSendMessage (HevcMessageHeader* pMessage);
		AJA_VIRTUAL bool	ControlDriverDebugMessages(NTV2_DriverDebugMessageSet msgSet, bool enable)	{(void)msgSet; (void)enable; return false;}

#if !defined(NTV2_DEPRECATE_13_0)
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetRelativeVideoPlaybackDelay (ULWord frmDelay))		{(void)frmDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetRelativeVideoPlaybackDelay (ULWord* frmDelay))	{(void)frmDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetAudioPlaybackPinDelay (ULWord msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetAudioPlaybackPinDelay (ULWord* msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetAudioRecordPinDelay (ULWord msDelay))		{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetAudioRecordPinDelay (ULWord* msDelay))	{(void)msDelay; return false;}	///< @deprecated	Obsolete starting in SDK 13.0.
#endif	//	!defined(NTV2_DEPRECATE_13_0)
		AJA_VIRTUAL bool	SetAudioOutputMode (NTV2_GlobalAudioPlaybackMode mode);
		AJA_VIRTUAL bool	GetAudioOutputMode (NTV2_GlobalAudioPlaybackMode* mode);

		// Management of downloaded Xilinx bitfile
		AJA_VIRTUAL bool DriverGetBitFileInformation (BITFILE_INFO_STRUCT & outBitfileInfo,  const NTV2BitFileType inBitfileType = NTV2_VideoProcBitFile);
		AJA_VIRTUAL bool DriverSetBitFileInformation (const BITFILE_INFO_STRUCT & inBitfileInfo);

		AJA_VIRTUAL bool RestoreHardwareProcampRegisters (void);

#if !defined(NTV2_DEPRECATE_16_0)
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool SetStrictTiming(ULWord strictTiming))	{(void)strictTiming; return false;}	///< @deprecated	Deprecated starting in SDK 16.0.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetStrictTiming(ULWord* strictTiming)){(void)strictTiming; return false;}	///< @deprecated	Deprecated starting in SDK 16.0.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetStreamingApplication(ULWord & outAppType, int32_t & outPID))	{return CNTV2DriverInterface::GetStreamingApplication(outAppType,outPID);}							///< @deprecated	Deprecated starting in SDK 16.0.
		AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool GetStreamingApplication(ULWord * pAppType, int32_t * pPID))		{return pAppType && pPID ? CNTV2DriverInterface::GetStreamingApplication(*pAppType,*pPID) : false;}	///< @deprecated	Deprecated starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapFrameBuffers(void));		///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapFrameBuffers(void));	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapRegisters(void));			///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapRegisters(void));		///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapXena2Flash(void));		///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool UnmapXena2Flash(void));		///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool MapMemory(PVOID pvUserVa, ULWord ulNumBytes, bool bMap, ULWord* ulUser = NULL));	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool DmaUnlock(void));			///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool CompleteMemoryForDMA(ULWord * pFrameBuffer));	///< @deprecated	Obsolete starting in SDK 16.0.
		AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool PrepareMemoryForDMA(ULWord * pFrameBuffer, const ULWord ulNumBytes));	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

	//	PRIVATE INSTANCE METHODS
	protected:
		AJA_VIRTUAL bool	OpenLocalPhysical (const UWord inDeviceIndex);
		AJA_VIRTUAL bool	CloseLocalPhysical (void);

	//	MEMBER DATA
	protected:
		PSP_DEVICE_INTERFACE_DETAIL_DATA _pspDevIFaceDetailData;
		SP_DEVINFO_DATA _spDevInfoData;
		HDEVINFO		_hDevInfoSet;
		HANDLE			_hDevice;
		GUID			_GUID_PROPSET;
		ULWord			_previousAudioState;
		ULWord			_previousAudioSelection;
#if !defined(NTV2_DEPRECATE_16_0)
		typedef std::vector<ULWord *>	DMA_LOCKED_VEC;
		DMA_LOCKED_VEC	_vecDmaLocked;	// OEM save locked memory addresses in vector
#endif	//	!defined(NTV2_DEPRECATE_16_0)
};	//	CNTV2WinDriverInterface

#endif	//	NTV2WINDRIVERINTERFACE_H
