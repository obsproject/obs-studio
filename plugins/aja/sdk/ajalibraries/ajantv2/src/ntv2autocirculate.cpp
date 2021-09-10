/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2autocirculate.cpp
	@brief		Implements the CNTV2Card AutoCirculate API functions.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2rp188.h"
#include "ntv2endian.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_timecode_atc.h"
#include "ajabase/common/timecode.h"
#include <iomanip>
#include <assert.h>
#include <algorithm>


using namespace std;

#if defined(MSWindows)
	#undef min
#endif

#if 0
	//	Debug builds can clear Anc buffers during A/C capture
	#define	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER		//	Requires non-zero kVRegZeroDeviceAncPostCapture
	#define AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER	//	Requires non-zero kVRegZeroHostAncPostCapture
#endif	//	_DEBUG

//	Logging Macros
#define ACINSTP(_p_)		" " << HEX0N(uint64_t(_p_),8)
#define ACTHIS				ACINSTP(this)

#define	ACFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	ACWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	ACNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	ACINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	ACDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_AutoCirculate,	ACTHIS << "::" << AJAFUNC << ": " << __x__)

#define	RCVFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	RCVWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	RCVNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	RCVINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	RCVDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_Anc2110Rcv,		ACTHIS << "::" << AJAFUNC << ": " << __x__)

#define	XMTFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	XMTWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	XMTNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	XMTINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)
#define	XMTDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_Anc2110Xmit,		ACTHIS << "::" << AJAFUNC << ": " << __x__)


static const char	gFBAllocLockName[]	=	"com.aja.ntv2.mutex.FBAlloc";
static AJALock		gFBAllocLock(gFBAllocLockName);	//	New in SDK 15:	Global mutex to avoid device frame buffer allocation race condition

#if !defined (NTV2_DEPRECATE_12_6)
static const NTV2Channel gCrosspointToChannel [] = {	/* NTV2CROSSPOINT_CHANNEL1	==>	*/	NTV2_CHANNEL1,
														/* NTV2CROSSPOINT_CHANNEL2	==>	*/	NTV2_CHANNEL2,
														/* NTV2CROSSPOINT_INPUT1	==>	*/	NTV2_CHANNEL1,
														/* NTV2CROSSPOINT_INPUT2	==>	*/	NTV2_CHANNEL2,
														/* NTV2CROSSPOINT_MATTE		==>	*/	NTV2_MAX_NUM_CHANNELS,
														/* NTV2CROSSPOINT_FGKEY		==>	*/	NTV2_MAX_NUM_CHANNELS,
														/* NTV2CROSSPOINT_CHANNEL3	==>	*/	NTV2_CHANNEL3,
														/* NTV2CROSSPOINT_CHANNEL4	==>	*/	NTV2_CHANNEL4,
														/* NTV2CROSSPOINT_INPUT3	==>	*/	NTV2_CHANNEL3,
														/* NTV2CROSSPOINT_INPUT4	==>	*/	NTV2_CHANNEL4,
														/* NTV2CROSSPOINT_CHANNEL5	==>	*/	NTV2_CHANNEL5,
														/* NTV2CROSSPOINT_CHANNEL6	==>	*/	NTV2_CHANNEL6,
														/* NTV2CROSSPOINT_CHANNEL7	==>	*/	NTV2_CHANNEL7,
														/* NTV2CROSSPOINT_CHANNEL8	==>	*/	NTV2_CHANNEL8,
														/* NTV2CROSSPOINT_INPUT5	==>	*/	NTV2_CHANNEL5,
														/* NTV2CROSSPOINT_INPUT6	==>	*/	NTV2_CHANNEL6,
														/* NTV2CROSSPOINT_INPUT7	==>	*/	NTV2_CHANNEL7,
														/* NTV2CROSSPOINT_INPUT8	==>	*/	NTV2_CHANNEL8,
																							NTV2_MAX_NUM_CHANNELS};
static const bool		gIsCrosspointInput [] = {		/* NTV2CROSSPOINT_CHANNEL1	==> */	false,
														/* NTV2CROSSPOINT_CHANNEL2	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT1	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT2	==>	*/	true,
														/* NTV2CROSSPOINT_MATTE		==>	*/	false,
														/* NTV2CROSSPOINT_FGKEY		==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL3	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL4	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT3	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT4	==>	*/	true,
														/* NTV2CROSSPOINT_CHANNEL5	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL6	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL7	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL8	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT5	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT6	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT7	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT8	==>	*/	true,
																							false};
#endif	//	!defined (NTV2_DEPRECATE_12_6)

#if !defined (NTV2_DEPRECATE)
bool   CNTV2Card::InitAutoCirculate(NTV2Crosspoint channelSpec,
                                    LWord startFrameNumber, 
                                    LWord endFrameNumber,
                                    bool bWithAudio,
                                    bool bWithRP188,
                                    bool bFbfChange,
                                    bool bFboChange,
									bool bWithColorCorrection ,
									bool bWithVidProc,
	                                bool bWithCustomAncData,
	                                bool bWithLTC,
									bool bWithAudio2)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );
 
	// If ending frame number has been defaulted, calculate it
	if (endFrameNumber < 0) {	
		endFrameNumber = 10;	// All boards have at least 10 frames	
	}

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eInitAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = startFrameNumber;
	autoCircData.lVal2		 = endFrameNumber;
	autoCircData.lVal3		 = bWithAudio2?1:0;
	autoCircData.lVal4		 = 1;
	autoCircData.bVal1		 = bWithAudio;
	autoCircData.bVal2		 = bWithRP188;
    autoCircData.bVal3       = bFbfChange;
    autoCircData.bVal4       = bFboChange;
    autoCircData.bVal5       = bWithColorCorrection;
    autoCircData.bVal6       = bWithVidProc;
    autoCircData.bVal7       = bWithCustomAncData;
    autoCircData.bVal8       = bWithLTC;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE_12_6)
bool   CNTV2Card::InitAutoCirculate(NTV2Crosspoint channelSpec,
                                    LWord startFrameNumber, 
                                    LWord endFrameNumber,
									LWord channelCount,
									NTV2AudioSystem audioSystem,
                                    bool bWithAudio,
                                    bool bWithRP188,
                                    bool bFbfChange,
                                    bool bFboChange,
									bool bWithColorCorrection ,
									bool bWithVidProc,
	                                bool bWithCustomAncData,
	                                bool bWithLTC)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );
 
	// If ending frame number has been defaulted, calculate it
	if (endFrameNumber < 0) {	
		endFrameNumber = 10;	// All boards have at least 10 frames	
	}

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eInitAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = startFrameNumber;
	autoCircData.lVal2		 = endFrameNumber;
	autoCircData.lVal3		 = audioSystem;
	autoCircData.lVal4		 = channelCount;
	autoCircData.bVal1		 = bWithAudio;
	autoCircData.bVal2		 = bWithRP188;
    autoCircData.bVal3       = bFbfChange;
    autoCircData.bVal4       = bFboChange;
    autoCircData.bVal5       = bWithColorCorrection;
    autoCircData.bVal6       = bWithVidProc;
    autoCircData.bVal7       = bWithCustomAncData;
    autoCircData.bVal8       = bWithLTC;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool   CNTV2Card::StartAutoCirculate (NTV2Crosspoint channelSpec, const ULWord64 inStartTime)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

 	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	::memset (&autoCircData, 0, sizeof (AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = inStartTime ? eStartAutoCircAtTime : eStartAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = static_cast <LWord> (inStartTime >> 32);
	autoCircData.lVal2		 = static_cast <LWord> (inStartTime & 0xFFFFFFFF);

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool   CNTV2Card::StopAutoCirculate(NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

  	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eStopAutoCirc;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	bool bRet =  AutoCirculate (autoCircData);

    // Wait to insure driver changes state to DISABLED
    if (bRet)
    {
		const NTV2Channel	channel	(gCrosspointToChannel [channelSpec]);
		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputFieldID (NTV2_FIELD0, channel) : WaitForOutputFieldID (NTV2_FIELD0, channel);
// 		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputVerticalInterrupt (channel) : WaitForOutputVerticalInterrupt (channel);
// 		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputVerticalInterrupt (channel) : WaitForOutputVerticalInterrupt (channel);
		AUTOCIRCULATE_STATUS_STRUCT acStatus;
		memset(&acStatus, 0, sizeof(AUTOCIRCULATE_STATUS_STRUCT));
		GetAutoCirculate(channelSpec, &acStatus );

		if ( acStatus.state != NTV2_AUTOCIRCULATE_DISABLED)
		{
			// something is wrong so abort.
			bRet = AbortAutoCirculate(channelSpec);
		}

    }

    return bRet;
}


bool   CNTV2Card::AbortAutoCirculate(NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

  	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eAbortAutoCirc;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	bool bRet =  AutoCirculate (autoCircData);

    return bRet;
}


bool   CNTV2Card::PauseAutoCirculate(NTV2Crosspoint channelSpec, bool pPlayToPause /*= true*/)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = ePauseAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.bVal1		 = pPlayToPause;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool   CNTV2Card::GetFrameStampEx2 (NTV2Crosspoint channelSpec, ULWord frameNum, 
									FRAME_STAMP_STRUCT* pFrameStamp,
									PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetFrameStampEx2;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = frameNum;
	autoCircData.pvVal1		 = (PVOID) pFrameStamp;
	autoCircData.pvVal2		 = (PVOID) pTaskStruct;

	// The following is ignored by Windows; it looks at the 
	// channel spec and frame num in the autoCircData instead.
	pFrameStamp->channelSpec = channelSpec;
	pFrameStamp->frame = frameNum;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::FlushAutoCirculate (NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eFlushAutoCirculate;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::SetActiveFrameAutoCirculate (NTV2Crosspoint channelSpec, ULWord lActiveFrame)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eSetActiveFrame;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1 = lActiveFrame;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::PrerollAutoCirculate (NTV2Crosspoint channelSpec, ULWord lPrerollFrames)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = ePrerollAutoCirculate;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1 = lPrerollFrames;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}




//  Notes:  This interface (TransferWithAutoCirculate) gives the driver control
//  over advancing the frame # to be transferred.
//  This allows the driver to assign the sequential frame # appropriately,
//  even in the face of dropped frames caused by CPU overload, etc.   
bool CNTV2Card::TransferWithAutoCirculate(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
                                          PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct)                                        
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

    // Sanity check with new parameter videoOffset
    if (pTransferStruct->videoDmaOffset)
    {
        if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
            return false;
    }
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculate;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

//
// TransferWithAutoCirculateEx
// Same as TransferWithAutoCirculate
// but adds the ability to pass routing register setups for xena2....
// actually you can send over any register setups you want. These
// will be set by the driver at the appropriate frame.
bool CNTV2Card::TransferWithAutoCirculateEx(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
                                          PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
										  NTV2RoutingTable* pXena2RoutingTable)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

    // Sanity check with new parameter videoOffset
    if (pTransferStruct->videoDmaOffset)
    {
        if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
            return false;
    }
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculateEx;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;
	autoCircData.pvVal3		 = (PVOID) pXena2RoutingTable;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


//
// TransferWithAutoCirculateEx2
// Same as TransferWithAutoCirculate
// adds flexible frame accurate task list
bool CNTV2Card::TransferWithAutoCirculateEx2(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
											 PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
											 NTV2RoutingTable* pXena2RoutingTable,
											 PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)                                        
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Sanity check with new parameter videoOffset
	if (pTransferStruct->videoDmaOffset)
	{
		if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
			return false;
	}
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculateEx2;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;
	autoCircData.pvVal3		 = (PVOID) pXena2RoutingTable;
	autoCircData.pvVal4		 = (PVOID) pTaskStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

//
// SetAutoCirculateCaptureTask
// add frame accurate task list for each captured frame
bool CNTV2Card::SetAutoCirculateCaptureTask(NTV2Crosspoint channelSpec, PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eSetCaptureTask;
	autoCircData.channelSpec = channelSpec;
	autoCircData.pvVal1		 = (PVOID) pTaskStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}
#endif	//	!defined (NTV2_DEPRECATE_12_6)

//GetFrameStamp(NTV2Crosspoint channelSpec, ULONG frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
//When a channelSpec is autocirculating, the ISR or DPC will continously fill in a
// FRAME_STAMP_STRUCT for the frame it is working on.
// The framestamp structure is intended to give enough information to determine if frames
// have been dropped either on input or output. It also allows for synchronization of 
// audio and video by stamping the audioinputaddress at the start and end of a video frame.
bool   CNTV2Card::GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
{
	// Insure that the CNTV2Card has been 'open'ed
	if (! _boardOpened )	return false;

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset((void*)&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetFrameStamp;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = LWord(frameNum);
	autoCircData.pvVal1		 = PVOID(pFrameStamp);

	// The following is ignored by Windows; it looks at the 
	// channel spec and frame num in the autoCircData instead.
	pFrameStamp->channelSpec = channelSpec;
	pFrameStamp->frame = frameNum;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

// GetAutoCirculate(NTV2Crosspoint channelSpec,AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
// Returns true if communication with the driver was successful.
// Passes back: whether associated channelSpec is currently autocirculating;
//              Frame Range (Start and End); and Current Active Frame.
//
//              Note that Current Active Frame is reliable,
//              whereas reading, for example, the Ch1OutputFrame register is not reliable,
//              because the latest-written value may or may-not have been clocked-in to the hardware.
//              Note also that this value is valid only if bIsCirculating is true.
bool   CNTV2Card::GetAutoCirculate(NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
{
	// Insure that the CNTV2Card has been 'open'ed
	if (!_boardOpened)	return false;

	// The following is ignored by Windows.
	autoCirculateStatus -> channelSpec = channelSpec;
	
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset((void*)&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.pvVal1		 = PVOID(autoCirculateStatus);

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool CNTV2Card::FindUnallocatedFrames (const UWord inFrameCount, LWord & outStartFrame, LWord & outEndFrame)
{
	AUTOCIRCULATE_STATUS			acStatus;
	typedef	std::set <uint16_t>		U16Set;
	typedef U16Set::const_iterator	U16SetConstIter;
	U16Set							allocatedFrameNumbers;
	bool							isQuadMode(false);

	//	Look for a contiguous group of available frame buffers...
	outStartFrame = outEndFrame = 0;
	if (!IsOpen())
		{ACFAIL("Not open");  return false;}
	if (!inFrameCount)
		{ACFAIL("Requested zero frames");  return false;}

	//	Inventory all allocated frames...
	for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2_MAX_NUM_CHANNELS;  chan = NTV2Channel(chan+1))
		if (AutoCirculateGetStatus(chan, acStatus)  &&  !acStatus.IsStopped())
		{
			//	This function has long been broken for UHD/4K/UHD2/8K (see issue #412)
			if (GetQuadFrameEnable(isQuadMode, chan)  &&  isQuadMode)
				{ACFAIL("UHD/4K in operation on Ch" << DEC(chan+1) << " -- caller must manage frames manually"); return false;}	//	New in SDK 16.1
			if (GetQuadQuadFrameEnable(isQuadMode, chan)  &&  isQuadMode)
				{ACFAIL("UHD2/8K in operation on Ch" << DEC(chan+1) << " -- caller must manage frames manually"); return false;}	//	New in SDK 16.1
			uint16_t	endFrameNum	(acStatus.GetEndFrame());
			for (uint16_t num(acStatus.GetStartFrame());  num <= endFrameNum;  num++)
				allocatedFrameNumbers.insert(num);
		}	//	if A/C active

	//	Find a contiguous band of unallocated frame numbers...
	const uint16_t		finalFrameNumber	(UWord(::NTV2DeviceGetNumberFrameBuffers(_boardID)) - 1);
	uint16_t			startFrameNumber	(0);
	uint16_t			endFrameNumber		(startFrameNumber + inFrameCount - 1);
	U16SetConstIter		iter				(allocatedFrameNumbers.begin());

	while (iter != allocatedFrameNumbers.end())
	{
		uint16_t	allocatedStartFrame	(*iter);
		uint16_t	allocatedEndFrame	(allocatedStartFrame);

		//	Find the end of this allocated band...
		while (++iter != allocatedFrameNumbers.end ()  &&  *iter == (allocatedEndFrame + 1))
			allocatedEndFrame = *iter;

		if (startFrameNumber < allocatedStartFrame  &&  endFrameNumber < allocatedStartFrame)
			break;	//	Found a free block!

		//	Not large enough -- move past this allocated block...
		startFrameNumber = allocatedEndFrame + 1;
		endFrameNumber = startFrameNumber + inFrameCount - 1;
	}

	if (startFrameNumber > finalFrameNumber || endFrameNumber > finalFrameNumber)
		{ACFAIL("Cannot find " << DEC(inFrameCount) << " unallocated frames");	return false;}

	outStartFrame = LWord(startFrameNumber);
	outEndFrame = LWord(endFrameNumber);
	ACDBG("Found unused " << DEC(UWord(inFrameCount)) << "-frame block (frames " << DEC(outStartFrame) << "-" << DEC(outEndFrame) << ")");
	return true;

}	//	FindUnallocatedFrames


//	Handy function to fetch the NTV2Crosspoint for a given NTV2Channel that works with both pre & post 12.3 drivers.
//	NOTE:  This relies on the channel's NTV2Mode being correct and aligned with the driver's NTV2Crosspoint!
static bool GetCurrentACChannelCrosspoint (CNTV2Card & inDevice, const NTV2Channel inChannel, NTV2Crosspoint & outCrosspoint)
{
	NTV2Mode	mode	(NTV2_MODE_DISPLAY);
	outCrosspoint = NTV2CROSSPOINT_INVALID;
	if (!inDevice.IsOpen ())
		return false;
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	if (!inDevice.GetMode (inChannel, mode))
		return false;
	outCrosspoint = (mode == NTV2_MODE_DISPLAY) ? ::NTV2ChannelToOutputCrosspoint (inChannel) : ::NTV2ChannelToInputCrosspoint (inChannel);
	return true;
}


bool CNTV2Card::AutoCirculateInitForInput (	const NTV2Channel		inChannel,
											const UWord				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UWord				inStartFrameNumber,
											const UWord				inEndFrameNumber)
{
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel
	if (!gFBAllocLock.IsValid())
		return false;	//	Mutex not ready

	AJAAutoLock	autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(static_cast <LWord>(inStartFrameNumber));
	LWord	endFrameNumber	(static_cast <LWord>(inEndFrameNumber));
	if (!endFrameNumber  &&  !startFrameNumber)
	{
		if (!inFrameCount)
			{ACFAIL("Zero frames requested");  return false;}
		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
			return false;
	}
	if (inFrameCount)
		ACWARN ("FrameCount " << DEC(inFrameCount) << " ignored -- using start/end " << DEC(inStartFrameNumber)
				<< "/" << DEC(inEndFrameNumber) << " frame numbers");
	if (endFrameNumber < startFrameNumber)	//	endFrame must be > startFrame
		{ACFAIL("EndFrame(" << DEC(endFrameNumber) << ") precedes StartFrame(" << DEC(startFrameNumber) << ")");  return false;}
	if ((endFrameNumber - startFrameNumber + 1) < 2)	//	must be at least 2 frames
		{ACFAIL("Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames"); return false;}

	//	Warn about interference from other channels...
	for (UWord chan(0);  chan < ::NTV2DeviceGetNumFrameStores(_boardID);  chan++)
	{
		ULWord		frameNum(0);
		NTV2Mode	mode(NTV2_MODE_INVALID);
		bool		isEnabled(false);
		if (inChannel != chan)
			if (IsChannelEnabled(NTV2Channel(chan), isEnabled)  &&  isEnabled)		//	FrameStore is enabled
				if (GetMode(NTV2Channel(chan), mode)  &&  mode == NTV2_MODE_INPUT)	//	Channel is capturing
					if (GetInputFrame(NTV2Channel(chan), frameNum))
						if (frameNum >= ULWord(startFrameNumber)  &&  frameNum <= ULWord(endFrameNumber))	//	Frame in range
							ACWARN("FrameStore " << DEC(chan+1) << " is writing frame " << DEC(frameNum)
									<< " -- will corrupt AutoCirculate channel " << DEC(inChannel+1) << " input frames "
									<< DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData	(eInitAutoCirc);
	autoCircData.channelSpec = ::NTV2ChannelToInputChannelSpec (inChannel);
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	autoCircData.lVal4 = inNumChannels;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_FIELDS) != 0)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_FIELDS;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL) != 0)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? true : false;
	autoCircData.bVal2 = inOptionFlags & AUTOCIRCULATE_WITH_RP188			? true : false;
	autoCircData.bVal3 = inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE		? true : false;
	autoCircData.bVal4 = inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE		? true : false;
	autoCircData.bVal5 = inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT	? true : false;
	autoCircData.bVal6 = inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC			? true : false;
	autoCircData.bVal7 = inOptionFlags & AUTOCIRCULATE_WITH_ANC				? true : false;
	autoCircData.bVal8 = inOptionFlags & AUTOCIRCULATE_WITH_LTC				? true : false;

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
		ACINFO("Channel " << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	else
		ACFAIL("Channel " << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForInput


bool CNTV2Card::AutoCirculateInitForOutput (const NTV2Channel		inChannel,
											const UWord				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UWord				inStartFrameNumber,
											const UWord				inEndFrameNumber)
{
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel
	if (!gFBAllocLock.IsValid())
		return false;	//	Mutex not ready

	AJAAutoLock	autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(static_cast <LWord>(inStartFrameNumber));
	LWord	endFrameNumber	(static_cast <LWord>(inEndFrameNumber));
	if (!endFrameNumber  &&  !startFrameNumber)
	{
		if (!inFrameCount)
			{ACFAIL("Zero frames requested");  return false;}
		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
			return false;
	}
	else if (inFrameCount)
		ACWARN ("FrameCount " << DEC(inFrameCount) << " ignored -- using start/end " << DEC(inStartFrameNumber)
				<< "/" << DEC(inEndFrameNumber) << " frame numbers");
	if (endFrameNumber < startFrameNumber)	//	endFrame must be > startFrame
		{ACFAIL("EndFrame(" << DEC(endFrameNumber) << ") precedes StartFrame(" << DEC(startFrameNumber) << ")");  return false;}
	if ((endFrameNumber - startFrameNumber + 1) < 2)	//	must be at least 2 frames
		{ACFAIL("Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames"); return false;}

	//	Warn about interference from other channels...
	for (UWord chan(0);  chan < ::NTV2DeviceGetNumFrameStores(_boardID);  chan++)
	{
		ULWord		frameNum(0);
		NTV2Mode	mode(NTV2_MODE_INVALID);
		bool		isEnabled(false);
		if (inChannel != chan)
			if (IsChannelEnabled(NTV2Channel(chan), isEnabled)  &&  isEnabled)		//	FrameStore is enabled
				if (GetMode(NTV2Channel(chan), mode)  &&  mode == NTV2_MODE_INPUT)	//	Channel is capturing
					if (GetInputFrame(NTV2Channel(chan), frameNum))
						if (frameNum >= ULWord(startFrameNumber)  &&  frameNum <= ULWord(endFrameNumber))	//	Frame in range
							ACWARN("FrameStore " << DEC(chan+1) << " is writing frame " << DEC(frameNum)
								<< " -- will corrupt AutoCirculate channel " << DEC(inChannel+1) << " output frames "
								<< DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}
	//	Warn about "with anc" and VANC mode...
	if (inOptionFlags & AUTOCIRCULATE_WITH_ANC)
	{
		NTV2VANCMode vancMode(NTV2_VANCMODE_INVALID);
		if (GetVANCMode(vancMode, inChannel)  &&  NTV2_IS_VANCMODE_ON(vancMode))
			ACWARN("FrameStore " << DEC(inChannel+1) << " has AUTOCIRCULATE_WITH_ANC set, but also has "
					<< ::NTV2VANCModeToString(vancMode) << " set -- this may cause anc insertion problems");
	}

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData	(eInitAutoCirc);
	autoCircData.channelSpec = NTV2ChannelToOutputChannelSpec (inChannel);
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	autoCircData.lVal4 = inNumChannels;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_FIELDS) != 0)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_FIELDS;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_HDMIAUX) != 0)
		autoCircData.lVal6 |= AUTOCIRCULATE_WITH_HDMIAUX;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL) != 0)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? true : false;
	autoCircData.bVal2 = ((inOptionFlags & AUTOCIRCULATE_WITH_RP188) != 0) ? true : false;
	autoCircData.bVal3 = ((inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE) != 0) ? true : false;
	autoCircData.bVal4 = ((inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE) != 0) ? true : false;
	autoCircData.bVal5 = ((inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT) != 0) ? true : false;
	autoCircData.bVal6 = ((inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC) != 0) ? true : false;
	autoCircData.bVal7 = ((inOptionFlags & AUTOCIRCULATE_WITH_ANC) != 0) ? true : false;
	autoCircData.bVal8 = ((inOptionFlags & AUTOCIRCULATE_WITH_LTC) != 0) ? true : false;
	if (::NTV2DeviceCanDo2110(_boardID))					//	If S2110 IP device...
		if (inOptionFlags & AUTOCIRCULATE_WITH_RP188)		//	and caller wants RP188
			if (!(inOptionFlags & AUTOCIRCULATE_WITH_ANC))	//	but caller failed to enable Anc playout
			{
				autoCircData.bVal7 = true;					//	Enable Anc insertion anyway
				ACDBG("Channel " << DEC(inChannel+1) << ": caller requested RP188 but not Anc -- enabled Anc inserter anyway");
			}

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
		ACINFO("Channel " << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	else
		ACFAIL("Channel " << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForOutput


bool CNTV2Card::AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime)
{
	AUTOCIRCULATE_DATA	autoCircData	(inStartTime ? eStartAutoCircAtTime : eStartAutoCirc);
	autoCircData.lVal1 = static_cast <LWord> (inStartTime >> 32);
	autoCircData.lVal2 = static_cast <LWord> (inStartTime & 0xFFFFFFFF);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	const bool result (AutoCirculate(autoCircData));
	if (result)
		ACINFO("Started channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to start channel " << DEC(inChannel+1));
	return result;
}


bool CNTV2Card::AutoCirculateStop (const NTV2Channel inChannel, const bool inAbort)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	const AUTO_CIRC_COMMAND	acCommand	(inAbort ? eAbortAutoCirc : eStopAutoCirc);
	AUTOCIRCULATE_DATA		stopInput	(acCommand,	::NTV2ChannelToInputCrosspoint (inChannel));
	AUTOCIRCULATE_DATA		stopOutput	(acCommand,	::NTV2ChannelToOutputCrosspoint (inChannel));
	NTV2Mode				mode		(NTV2_MODE_INVALID);
	AUTOCIRCULATE_STATUS	acStatus;

	//	Stop input or output A/C using the old driver call...
	const bool	stopInputFailed		(!AutoCirculate (stopInput));
	const bool	stopOutputFailed	(!AutoCirculate (stopOutput));
	if (stopInputFailed && stopOutputFailed)
	{
		ACFAIL("Failed to stop channel " << DEC(inChannel+1));
		return false;	//	Both failed
	}
	if (inAbort)
	{
		ACINFO("Aborted channel " << DEC(inChannel+1));
		return true;	//	In abort case, no more to do!
	}

	//	Wait until driver changes AC state to DISABLED...
	bool result (GetMode(inChannel, mode));
	if (NTV2_IS_INPUT_MODE(mode))
		WaitForInputFieldID(NTV2_FIELD0, inChannel);
	if (NTV2_IS_OUTPUT_MODE(mode))
		WaitForOutputFieldID(NTV2_FIELD0, inChannel);
	if (AutoCirculateGetStatus(inChannel, acStatus)  &&  acStatus.acState != NTV2_AUTOCIRCULATE_DISABLED)
	{
		ACWARN("Failed to stop channel " << DEC(inChannel+1) << " -- retrying with ABORT");
		return AutoCirculateStop(inChannel, true);	//	something's wrong -- abort (WARNING: RECURSIVE CALL!)
	}
	ACINFO("Stopped channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateStop


bool CNTV2Card::AutoCirculatePause (const NTV2Channel inChannel)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1		= false;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Paused channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to pause channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculatePause


bool CNTV2Card::AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1 = true;
	autoCircData.bVal2 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Resumed channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to resume channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateResume


bool CNTV2Card::AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eFlushAutoCirculate);
    autoCircData.bVal1 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Flushed channel " << DEC(inChannel+1) << ", " << (inClearDropCount?"cleared":"retained") << " drop count");
	else
		ACFAIL("Failed to flush channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateFlush


bool CNTV2Card::AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(ePrerollAutoCirculate);
	autoCircData.lVal1 = LWord(inPreRollFrames);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Prerolled " << DEC(inPreRollFrames) << " frame(s) on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to preroll " << DEC(inPreRollFrames) << " frame(s) on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculatePreRoll


bool CNTV2Card::AutoCirculateGetStatus (const NTV2Channel inChannel, AUTOCIRCULATE_STATUS & outStatus)
{
	outStatus.Clear ();
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, outStatus.acCrosspoint))
		return false;

	if (!NTV2_IS_VALID_NTV2CROSSPOINT (outStatus.acCrosspoint))
	{
		AUTOCIRCULATE_STATUS	notRunningStatus (::NTV2ChannelToOutputCrosspoint (inChannel));
		outStatus = notRunningStatus;
		return true;	//	AutoCirculate not running on this channel
	}

#if defined (NTV2_NUB_CLIENT_SUPPORT)
	// Auto circulate status is not implemented in the NUB yet
	if (IsRemote())
		return false;
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	const bool result(NTV2Message(reinterpret_cast<NTV2_HEADER *>(&outStatus)));
	if (result)
		;//ACDBG("GetStatus successful on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to get status on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateGetStatus


bool CNTV2Card::AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNum, FRAME_STAMP & outFrameStamp)
{
	//	Use the new driver call...
	outFrameStamp.acFrameTime = LWord64 (inChannel);
	outFrameStamp.acRequestedFrame = inFrameNum;
	return NTV2Message (reinterpret_cast <NTV2_HEADER *> (&outFrameStamp));

}	//	AutoCirculateGetFrameStamp


bool CNTV2Card::AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eSetActiveFrame);
	autoCircData.lVal1 = LWord(inNewActiveFrame);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Set active frame to " << DEC(inNewActiveFrame) << " on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to set active frame to " << DEC(inNewActiveFrame) << " on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateSetActiveFrame


bool CNTV2Card::AutoCirculateTransfer (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	if (!_boardOpened)
		return false;
	#if defined (_DEBUG)
		NTV2_ASSERT (inOutXferInfo.NTV2_IS_STRUCT_VALID ());
	#endif

	NTV2Crosspoint			crosspoint	(NTV2CROSSPOINT_INVALID);
	NTV2EveryFrameTaskMode	taskMode	(NTV2_OEM_TASKS);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, crosspoint))
		return false;
	if (!NTV2_IS_VALID_NTV2CROSSPOINT(crosspoint))
		return false;
	GetEveryFrameServices(taskMode);

	if (NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.Fill(ULWord(0xFFFFFFFF));	//	Invalidate old timecodes
	else if (NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		bool isProgressive (false);
		IsProgressiveStandard(isProgressive, inChannel);
		if (inOutXferInfo.acRP188.IsValid())
			inOutXferInfo.SetAllOutputTimeCodes(inOutXferInfo.acRP188, /*alsoSetF2*/!isProgressive);

		const NTV2_RP188 *	pArray	(reinterpret_cast <const NTV2_RP188*>(inOutXferInfo.acOutputTimeCodes.GetHostPointer()));
		if (pArray  &&  pArray[NTV2_TCINDEX_DEFAULT].IsValid())
			inOutXferInfo.SetAllOutputTimeCodes(pArray[NTV2_TCINDEX_DEFAULT], /*alsoSetF2*/!isProgressive);
	}

	bool			tmpLocalF1AncBuffer(false),  tmpLocalF2AncBuffer(false);
	NTV2_POINTER	savedAncF1,  savedAncF2;
	if (::NTV2DeviceCanDo2110(_boardID)  &&  NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		//	S2110 Playout:	So that most Retail & OEM playout apps "just work" with S2110 RTP Anc streams,
		//					our classic SDI Anc data that device firmware normally embeds into SDI output
		//					as derived from registers -- VPID & RP188 -- the SDK here automatically inserts
		//					these packets into the outgoing RTP streams, even if the client didn't provide
		//					Anc buffers in the AUTOCIRCULATE_TRANSFER object, or specify AUTOCIRCULATE_WITH_ANC.
		ULWord	F1OffsetFromBottom(0),  F2OffsetFromBottom(0);
		size_t	F1SizeInBytes(0), F2SizeInBytes(0);
		if (GetAncRegionOffsetFromBottom(F1OffsetFromBottom, NTV2_AncRgn_Field1)
			&&  GetAncRegionOffsetFromBottom(F2OffsetFromBottom, NTV2_AncRgn_Field2))
		{
			F2SizeInBytes = size_t(F2OffsetFromBottom);
			if (F2OffsetFromBottom < F1OffsetFromBottom)
				F1SizeInBytes = size_t(F1OffsetFromBottom - F2OffsetFromBottom);
			else
				F1SizeInBytes = size_t(F2OffsetFromBottom - F1OffsetFromBottom);
		}
		if ((_boardID == DEVICE_ID_IOIP_2110) || (_boardID == DEVICE_ID_IOIP_2110_RGB12))
		{	//	IoIP 2110 Playout requires room for RTP+GUMP per anc buffer, to also operate SDI5 Mon output
			ULWord	F1MonOffsetFromBottom(0),  F2MonOffsetFromBottom(0);
			const bool good (GetAncRegionOffsetFromBottom(F1MonOffsetFromBottom, NTV2_AncRgn_MonField1)
							 &&  GetAncRegionOffsetFromBottom(F2MonOffsetFromBottom, NTV2_AncRgn_MonField2));
			if (good	//	Driver expects anc regions in this order (from bottom): F2Mon, F2, F1Mon, F1
				&&	F2MonOffsetFromBottom < F2OffsetFromBottom
				&&	F2OffsetFromBottom < F1MonOffsetFromBottom
				&&	F1MonOffsetFromBottom < F1OffsetFromBottom)
			{
				F1SizeInBytes = size_t(F1OffsetFromBottom - F2OffsetFromBottom);
				F2SizeInBytes = size_t(F2OffsetFromBottom);
			}
			else
			{	//	Anc regions out of order!
				XMTWARN("IoIP 2110 playout anc rgns disordered (offsets from bottom): F2Mon=" << HEX0N(F2MonOffsetFromBottom,8)
						<< " F2=" << HEX0N(F2OffsetFromBottom,8) << " F1Mon=" << HEX0N(F1MonOffsetFromBottom,8)
						<< " F1=" << HEX0N(F1OffsetFromBottom,8));
				F1SizeInBytes = F2SizeInBytes = 0;	//	Out of order, don't do Anc
			}
			savedAncF1 = inOutXferInfo.acANCBuffer;			//	copy
			savedAncF2 = inOutXferInfo.acANCField2Buffer;	//	copy
			if (inOutXferInfo.acANCBuffer.GetByteCount() < F1SizeInBytes)
			{	//	Enlarge acANCBuffer, and copy everything from savedAncF1 into it...
				inOutXferInfo.acANCBuffer.Allocate(F1SizeInBytes);
				inOutXferInfo.acANCBuffer.Fill(uint64_t(0));
				inOutXferInfo.acANCBuffer.CopyFrom(savedAncF1, 0, 0, savedAncF1.GetByteCount());
			}
			if (inOutXferInfo.acANCField2Buffer.GetByteCount() < F2SizeInBytes)
			{	//	Enlarge acANCField2Buffer, and copy everything from savedAncF2 into it...
				inOutXferInfo.acANCField2Buffer.Allocate(F2SizeInBytes);
				inOutXferInfo.acANCField2Buffer.Fill(uint64_t(0));
				inOutXferInfo.acANCField2Buffer.CopyFrom(savedAncF2, 0, 0, savedAncF2.GetByteCount());
			}
		}	//	if IoIP 2110 playout
		else
		{	//	else KonaIP 2110 playout
			if (inOutXferInfo.acANCBuffer.IsNULL())
				tmpLocalF1AncBuffer = inOutXferInfo.acANCBuffer.Allocate(F1SizeInBytes);
			else
				savedAncF1 = inOutXferInfo.acANCBuffer;	//	copy
			if (inOutXferInfo.acANCField2Buffer.IsNULL())
				tmpLocalF2AncBuffer = inOutXferInfo.acANCField2Buffer.Allocate(F2SizeInBytes);
			else
				savedAncF2 = inOutXferInfo.acANCField2Buffer;	//	copy
		}	//	else KonaIP 2110 playout
		S2110DeviceAncToXferBuffers(inChannel, inOutXferInfo);
	}	//	if SMPTE 2110 playout
	else if (::NTV2DeviceCanDo2110(_boardID)  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
	{	//	Need local host buffers to receive 2110 Anc VPID & ATC
		if (inOutXferInfo.acANCBuffer.IsNULL())
			tmpLocalF1AncBuffer = inOutXferInfo.acANCBuffer.Allocate(2048);
		if (inOutXferInfo.acANCField2Buffer.IsNULL())
			tmpLocalF2AncBuffer = inOutXferInfo.acANCField2Buffer.Allocate(2048);
	}	//	if SMPTE 2110 capture

	/////////////////////////////////////////////////////////////////////////////
	//	Call the driver...
	inOutXferInfo.acCrosspoint = crosspoint;
	bool result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&inOutXferInfo));
	/////////////////////////////////////////////////////////////////////////////

	if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
	{
		if (::NTV2DeviceCanDo2110(_boardID))
		{	//	S2110:  decode VPID and timecode anc packets from RTP, and put into A/C Xfer and device regs
			S2110DeviceAncFromXferBuffers(inChannel, inOutXferInfo);
		}
		if (taskMode == NTV2_STANDARD_TASKS)
		{
			//	After 12.? shipped, we discovered problems with timecode capture in our classic retail stuff.
			//	The acTimeCodes[NTV2_TCINDEX_DEFAULT] was coming up empty.
			//	Rather than fix all three drivers -- the Right, but Difficult Thing To Do --
			//	we decided to do the Easy Thing, here, in user-space.

			//	First, determine the ControlPanel's current Input source (SDIIn1/HDMIIn1 or SDIIn2/HDMIIn2)...
			ULWord	inputSelect	(NTV2_Input1Select);
			ReadRegister (kVRegInputSelect, inputSelect);
			const bool	bIsInput2	(inputSelect == NTV2_Input2Select);

			//	Next, determine the ControlPanel's current TimeCode source (LTC? VITC1? VITC2)...
			RP188SourceFilterSelect TimecodeSource(kRP188SourceEmbeddedLTC);
			CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, TimecodeSource);

			//	Now convert that into an NTV2TCIndex...
			NTV2TCIndex TimecodeIndex = NTV2_TCINDEX_DEFAULT;
			switch (TimecodeSource)
			{
				default:/*kRP188SourceEmbeddedLTC:*/TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_LTC : NTV2_TCINDEX_SDI1_LTC;	break;
				case kRP188SourceEmbeddedVITC1:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2     : NTV2_TCINDEX_SDI1;		break;
				case kRP188SourceEmbeddedVITC2:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_2   : NTV2_TCINDEX_SDI1_2;	break;
				case kRP188SourceLTCPort:			TimecodeIndex = NTV2_TCINDEX_LTC1;											break;
			}

			//	Fetch the TimeCode value that's in that NTV2TCIndex slot...
			NTV2_RP188	tcValue;
			inOutXferInfo.GetInputTimeCode(tcValue, TimecodeIndex);
			if (TimecodeIndex == NTV2_TCINDEX_LTC1)
			{	//	Special case for external LTC:
				//	Our driver currently returns all-zero DBB values for external LTC.
				//	It should probably at least set DBB BIT(17) "selected RP188 received" if external LTC is present.
				//	Ticket 3367: Our QuickTime 'vdig' relies on DBB BIT(17) being set, or it assumes timecode is invalid
				if (tcValue.fLo  &&  tcValue.fHi  &&  tcValue.fLo != 0xFFFFFFFF  &&  tcValue.fHi != 0xFFFFFFFF)
					tcValue.fDBB |= 0x00020000;
			}

			//	Valid or not, stuff that TimeCode value into inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes[NTV2_TCINDEX_DEFAULT]...
			NTV2_RP188 *	pArray	(reinterpret_cast <NTV2_RP188 *> (inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.GetHostPointer()));
			if (pArray)
				pArray [NTV2_TCINDEX_DEFAULT] = tcValue;
		}	//	if retail mode
	}	//	if NTV2Message OK && capturing
	if (result  &&  NTV2_IS_OUTPUT_CROSSPOINT(crosspoint))
	{
		if (savedAncF1)
			inOutXferInfo.acANCBuffer = savedAncF1;			//	restore
		if (savedAncF2)
			inOutXferInfo.acANCField2Buffer = savedAncF2;	//	restore
	}	//	if successful playout

	if (tmpLocalF1AncBuffer)
		inOutXferInfo.acANCBuffer.Deallocate();
	if (tmpLocalF2AncBuffer)
		inOutXferInfo.acANCField2Buffer.Deallocate();

	#if defined (AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER)
		if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroDeviceAncPostCapture, doZeroing)  &&  doZeroing)
			{	//	Zero out the Anc buffer on the device...
				static NTV2_POINTER	gClearDeviceAncBuffer;
				const LWord		xferFrame	(inOutXferInfo.GetTransferFrameNumber());
				ULWord			ancOffsetF1	(0);
				ULWord			ancOffsetF2	(0);
				NTV2Framesize	fbSize		(NTV2_FRAMESIZE_INVALID);
				ReadRegister(kVRegAncField1Offset, ancOffsetF1);
				ReadRegister(kVRegAncField2Offset, ancOffsetF2);
				GetFrameBufferSize(inChannel, fbSize);
				const ULWord	fbByteCount	(::NTV2FramesizeToByteCount(fbSize));
				const ULWord	ancOffset	(ancOffsetF2 > ancOffsetF1  ?  ancOffsetF2  :  ancOffsetF1);	//	Use whichever is larger
				NTV2_ASSERT (xferFrame != -1);
				if (gClearDeviceAncBuffer.IsNULL() || (gClearDeviceAncBuffer.GetByteCount() != ancOffset))
				{
					gClearDeviceAncBuffer.Allocate(ancOffset);	//	Allocate it
					gClearDeviceAncBuffer.Fill (ULWord(0));		//	Clear it
				}
				if (xferFrame != -1  &&  fbByteCount  &&  !gClearDeviceAncBuffer.IsNULL())
					DMAWriteSegments (ULWord(xferFrame),
									reinterpret_cast<ULWord*>(gClearDeviceAncBuffer.GetHostPointer()),	//	host buffer
									fbByteCount - ancOffset,				//	device memory offset, in bytes
									gClearDeviceAncBuffer.GetByteCount(),	//	total number of bytes to xfer
									1,										//	numSegments -- one chunk of 'ancOffset'
									gClearDeviceAncBuffer.GetByteCount(),	//	segmentHostPitch
									gClearDeviceAncBuffer.GetByteCount());	//	segmentCardPitch
			}
		}
	#endif	//	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER

	#if defined (AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER)
		if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroHostAncPostCapture, doZeroing)  &&  doZeroing)
			{	//	Zero out everything past the last captured Anc byte in the client's host buffer(s)... 
				NTV2_POINTER &	clientAncBufferF1	(inOutXferInfo.acANCBuffer);
				NTV2_POINTER &	clientAncBufferF2	(inOutXferInfo.acANCField2Buffer);
				const ULWord	ancF1ByteCount		(inOutXferInfo.GetCapturedAncByteCount(false));
				const ULWord	ancF2ByteCount		(inOutXferInfo.GetCapturedAncByteCount(true));
				void *			pF1TailEnd			(clientAncBufferF1.GetHostAddress(ancF1ByteCount));
				void *			pF2TailEnd			(clientAncBufferF2.GetHostAddress(ancF2ByteCount));
				if (pF1TailEnd  &&  clientAncBufferF1.GetByteCount() > ancF1ByteCount)
					::memset (pF1TailEnd, 0, clientAncBufferF1.GetByteCount() - ancF1ByteCount);
				if (pF2TailEnd  &&  clientAncBufferF2.GetByteCount() > ancF2ByteCount)
					::memset (pF2TailEnd, 0, clientAncBufferF2.GetByteCount() - ancF2ByteCount);
			}
		}
	#endif	//	AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER

	if (result)
		ACDBG("Transfer successful for channel " << DEC(inChannel+1));
	else
		ACFAIL("Transfer failed on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateTransfer


static const AJA_FrameRate	sNTV2Rate2AJARate[] = {	AJA_FrameRate_Unknown	//	NTV2_FRAMERATE_UNKNOWN	= 0,
													,AJA_FrameRate_6000		//	NTV2_FRAMERATE_6000		= 1,
													,AJA_FrameRate_5994		//	NTV2_FRAMERATE_5994		= 2,
													,AJA_FrameRate_3000		//	NTV2_FRAMERATE_3000		= 3,
													,AJA_FrameRate_2997		//	NTV2_FRAMERATE_2997		= 4,
													,AJA_FrameRate_2500		//	NTV2_FRAMERATE_2500		= 5,
													,AJA_FrameRate_2400		//	NTV2_FRAMERATE_2400		= 6,
													,AJA_FrameRate_2398		//	NTV2_FRAMERATE_2398		= 7,
													,AJA_FrameRate_5000		//	NTV2_FRAMERATE_5000		= 8,
													,AJA_FrameRate_4800		//	NTV2_FRAMERATE_4800		= 9,
													,AJA_FrameRate_4795		//	NTV2_FRAMERATE_4795		= 10,
													,AJA_FrameRate_12000	//	NTV2_FRAMERATE_12000	= 11,
													,AJA_FrameRate_11988	//	NTV2_FRAMERATE_11988	= 12,
													,AJA_FrameRate_1500		//	NTV2_FRAMERATE_1500		= 13,
													,AJA_FrameRate_1498		//	NTV2_FRAMERATE_1498		= 14,
#if !defined(NTV2_DEPRECATE_16_0)
													,AJA_FrameRate_1900		//	NTV2_FRAMERATE_1900		= 15,	// Formerly 09 in older SDKs
													,AJA_FrameRate_1898		//	NTV2_FRAMERATE_1898		= 16, 	// Formerly 10 in older SDKs
													,AJA_FrameRate_1800		//	NTV2_FRAMERATE_1800		= 17,	// Formerly 11 in older SDKs
													,AJA_FrameRate_1798		//	NTV2_FRAMERATE_1798		= 18,	// Formerly 12 in older SDKs
#endif	//	!defined(NTV2_DEPRECATE_16_0)
													};

static const TimecodeFormat	sNTV2Rate2TCFormat[] = {kTCFormatUnknown	//	NTV2_FRAMERATE_UNKNOWN	= 0,
													,kTCFormat60fps		//	NTV2_FRAMERATE_6000		= 1,
													,kTCFormat30fps		//	NTV2_FRAMERATE_5994		= 2,
													,kTCFormat30fps		//	NTV2_FRAMERATE_3000		= 3,
													,kTCFormat30fps		//	NTV2_FRAMERATE_2997		= 4,
													,kTCFormat25fps		//	NTV2_FRAMERATE_2500		= 5,
													,kTCFormat24fps		//	NTV2_FRAMERATE_2400		= 6,
													,kTCFormat24fps		//	NTV2_FRAMERATE_2398		= 7,
													,kTCFormat50fps		//	NTV2_FRAMERATE_5000		= 8,
													,kTCFormat48fps		//	NTV2_FRAMERATE_4800		= 9,
													,kTCFormat48fps		//	NTV2_FRAMERATE_4795		= 10,
													,kTCFormat60fps		//	NTV2_FRAMERATE_12000	= 11,
													,kTCFormat60fps		//	NTV2_FRAMERATE_11988	= 12,
													,kTCFormat30fps		//	NTV2_FRAMERATE_1500		= 13,
													,kTCFormat30fps		//	NTV2_FRAMERATE_1498		= 14,
#if !defined(NTV2_DEPRECATE_16_0)
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1900		= 15,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1898		= 16,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1800		= 17,
													,kTCFormatUnknown	//	NTV2_FRAMERATE_1798		= 18,
#endif	//	!defined(NTV2_DEPRECATE_16_0)
													};

//	VPID Packet Insertion						1080	720		525		625		1080p	2K		2K1080p		2K1080i		UHD		4K		UHDHFR		4KHFR
static const uint16_t	sVPIDLineNumsF1[] = {	10,		10,		13,		9,		10,		10,		10,			10,			10,		10,		10,			10	};
static const uint16_t	sVPIDLineNumsF2[] = {	572,	0,		276,	322,	0,		0,		0,			572,		0,		0,		0,			0	};

//	SDI RX Status Registers (for setting/clearing "VPID Present" bits)
static const uint32_t	gSDIInRxStatusRegs[] = {kRegRXSDI1Status, kRegRXSDI2Status, kRegRXSDI3Status, kRegRXSDI4Status, kRegRXSDI5Status, kRegRXSDI6Status, kRegRXSDI7Status, kRegRXSDI8Status, 0};


bool CNTV2Card::S2110DeviceAncFromXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	//	IP 2110 Capture:	Extract timecode(s) and put into inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes...
	//						Extract VPID and put into SDIIn VPID regs
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Rcv));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2_POINTER &		ancF1			(inOutXferInfo.acANCBuffer);
	NTV2_POINTER &		ancF2			(inOutXferInfo.acANCField2Buffer);
	AJAAncillaryData *	pPkt			(AJA_NULL);
	uint32_t			vpidA(0), vpidB(0);
	AJAAncillaryList	pkts;

	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	if (!ancF1.IsNULL() || !ancF2.IsNULL())
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, pkts)))
			return false;	//	Packet import failed

	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(isProgressive ? 0 : smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	Look for ATC and VITC...
	for (uint32_t ndx(0);  ndx < pkts.CountAncillaryData();  ndx++)
	{
		pPkt = pkts.GetAncillaryDataAtIndex(ndx);
		if (pPkt->GetDID() == 0x41  &&  pPkt->GetSID() == 0x01)	//	VPID?
		{	//	VPID!
			if (pPkt->GetDC() != 4)
				continue;	//	Skip . . . expected DC == 4
			const uint32_t* pULWord		(reinterpret_cast<const uint32_t*>(pPkt->GetPayloadData()));
			uint32_t		vpidValue	(pULWord ? *pULWord : 0);
			if (!pPkt->GetDataLocation().IsHanc())
				continue;	//	Skip . . . expected IsHANC
			vpidValue = NTV2EndianSwap32BtoH(vpidValue);
			if (IS_LINKB_AJAAncillaryDataStream(pPkt->GetDataLocation().GetDataStream()))
				vpidB = vpidValue;
			else
				vpidA = vpidValue;
			continue;	//	Done . . . on to next packet
		}

		const AJAAncillaryDataType	ancType  (pPkt->GetAncillaryDataType());
		if (ancType != AJAAncillaryDataType_Timecode_ATC)
		{
			if (ancType == AJAAncillaryDataType_Timecode_VITC  &&  isMonitoring)
				RCVWARN("Skipped VITC packet: " << pPkt->AsString(16));
			continue;	//	Not timecode . . . skip
		}

		//	Got ATC packet!
		AJAAncillaryData_Timecode_ATC *	pATCPkt(reinterpret_cast<AJAAncillaryData_Timecode_ATC*>(pPkt));
		if (!pATCPkt)
			continue;

		AJAAncillaryData_Timecode_ATC_DBB1PayloadType	payloadType (AJAAncillaryData_Timecode_ATC_DBB1PayloadType_Unknown);
		pATCPkt->GetDBB1PayloadType(payloadType);
		NTV2TCIndex tcNdx (NTV2_TCINDEX_INVALID);
		switch(payloadType)
		{
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/true, /*inIsF2*/false);
				break;
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/false, /*inIsF2*/false);
				break;
			case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2:
				tcNdx = ::NTV2ChannelToTimecodeIndex (inChannel, /*inEmbeddedLTC*/false, /*inIsF2*/true);
				break;
			default:
				break;
		}
		if (!NTV2_IS_VALID_TIMECODE_INDEX(tcNdx))
			continue;

		NTV2_RP188	ntv2rp188;	//	<==	This is what we want to get from pATCPkt
		AJATimeCode	ajaTC;		//	We can get an AJATimeCode from it via GetTimecode
		const AJA_FrameRate	ajaRate	(sNTV2Rate2AJARate[ntv2Rate]);
		AJATimeBase			ajaTB	(ajaRate);

		bool isDF = false;
		AJAAncillaryData_Timecode_Format tcFmt = pATCPkt->GetTimecodeFormatFromTimeBase(ajaTB);
		pATCPkt->GetDropFrameFlag(isDF, tcFmt);

		pATCPkt->GetTimecode(ajaTC, ajaTB);
								//	There is an AJATimeCode function to get an NTV2_RP188:
								//	ajaTC.QueryRP188(ntv2rp188.fDBB, ntv2rp188.fLo, ntv2rp188.fHi, ajaTB, isDF);
								//	But it's not implemented!  D'OH!!
								//	Let the hacking begin...

		string	tcStr;
		ajaTC.QueryString(tcStr, ajaTB, isDF);
		CRP188 rp188(tcStr, sNTV2Rate2TCFormat[ntv2Rate]);
		rp188.SetDropFrame(isDF);
		rp188.GetRP188Reg(ntv2rp188);
		//	Finally, poke the RP188 timecode into the Input Timecodes array...
		inOutXferInfo.acTransferStatus.acFrameStamp.SetInputTimecode(tcNdx, ntv2rp188);
	}	//	for each anc packet

	if (isMonitoring)
	{
		NTV2TimeCodes	timecodes;
		inOutXferInfo.acTransferStatus.GetFrameStamp().GetInputTimeCodes(timecodes, inChannel);
		if (!timecodes.empty())	RCVDBG("Channel" << DEC(inChannel+1) << " timecodes: " << timecodes);
	}
	if (vpidA || vpidB)
	{
		if (isMonitoring)
			RCVDBG("WriteSDIInVPID chan=" << DEC(inChannel+1) << " VPIDa=" << xHEX0N(vpidA,4) << " VPIDb=" << xHEX0N(vpidB,4));
		WriteSDIInVPID(inChannel, vpidA, vpidB);
	}
	WriteRegister(gSDIInRxStatusRegs[inChannel], vpidA ? 1 : 0, BIT(20), 20);	//	Set RX VPID Valid LinkA bit if vpidA non-zero
	WriteRegister(gSDIInRxStatusRegs[inChannel], vpidB ? 1 : 0, BIT(21), 21);	//	Set RX VPID Valid LinkB bit if vpidB non-zero

	//	Normalize to SDI/GUMP...
	return AJA_SUCCESS(pkts.GetTransmitData(ancF1, ancF2, isProgressive, F2StartLine));

}	//	S2110DeviceAncFromXferBuffers


bool CNTV2Card::S2110DeviceAncFromBuffers (const NTV2Channel inChannel, NTV2_POINTER & ancF1, NTV2_POINTER & ancF2)
{
	//	IP 2110 Capture:	Extract timecode(s) and put into RP188 registers
	//						Extract VPID and put into SDIIn VPID registers
	AUTOCIRCULATE_TRANSFER	tmpXfer;	tmpXfer.acANCBuffer = ancF1;	tmpXfer.acANCField2Buffer = ancF2;
	if (!S2110DeviceAncFromXferBuffers (inChannel, tmpXfer))	//	<== This handles stuffing the VPID regs
		{RCVFAIL("S2110DeviceAncFromXferBuffers failed");  return false;}

	NTV2TimeCodes	timecodes;
	if (!tmpXfer.acTransferStatus.GetFrameStamp().GetInputTimeCodes(timecodes, inChannel))
		{RCVFAIL("GetInputTimeCodes failed");  return false;}

	for (NTV2TimeCodesConstIter iter(timecodes.begin());  iter != timecodes.end();  ++iter)
	{
		//const bool		isLTC		(NTV2_IS_ATC_LTC_TIMECODE_INDEX(iter->first));
		const NTV2_RP188	ntv2rp188	(iter->second);
		SetRP188Data (inChannel, ntv2rp188);	//	Poke the timecode into the SDIIn timecode regs
	}	//	for each good timecode associated with "inChannel"
	//	No need to log timecodes, already done in S2110DeviceAncFromXferBuffers:	//ANCDBG(timecodes);

	return true;

}	//	S2110DeviceAncFromBuffers


static inline uint32_t EndianSwap32NtoH (const uint32_t inValue)	{return NTV2EndianSwap32BtoH(inValue);}	//	Guaranteed no in-place byte-swap -- always copies


bool CNTV2Card::S2110DeviceAncToXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	//	IP 2110 Playout:	Add relevant transmit timecodes and VPID to outgoing RTP Anc
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	bool				generateRTP		(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Xmit));
	const bool			isIoIP2110		((_boardID == DEVICE_ID_IOIP_2110) || (_boardID == DEVICE_ID_IOIP_2110_RGB12));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2_POINTER &		ancF1			(inOutXferInfo.acANCBuffer);
	NTV2_POINTER &		ancF2			(inOutXferInfo.acANCField2Buffer);
	NTV2TaskMode		taskMode		(NTV2_OEM_TASKS);
	ULWord				vpidA(0), vpidB(0);
	AJAAncillaryList	packetList;
	const NTV2Channel	SDISpigotChannel(GetEveryFrameServices(taskMode) && NTV2_IS_STANDARD_TASKS(taskMode)  ?  NTV2_CHANNEL3  :  inChannel);
	ULWord				F1OffsetFromBottom(0),  F2OffsetFromBottom(0),  F1MonOffsetFromBottom(0),  F2MonOffsetFromBottom(0);
	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	IoIP 2110 Playout requires RTP+GUMP per anc buffer to operate SDI5 Mon output...
	GetAncRegionOffsetFromBottom(F1OffsetFromBottom,    NTV2_AncRgn_Field1);
	GetAncRegionOffsetFromBottom(F2OffsetFromBottom,    NTV2_AncRgn_Field2);
	GetAncRegionOffsetFromBottom(F1MonOffsetFromBottom, NTV2_AncRgn_MonField1);
	GetAncRegionOffsetFromBottom(F2MonOffsetFromBottom, NTV2_AncRgn_MonField2);
	//	Define F1 & F2 GUMP sub-buffers from ancF1 & ancF2 (only used for IoIP 2110)...
	NTV2_POINTER gumpF1(ancF1.GetHostAddress(F1OffsetFromBottom - F1MonOffsetFromBottom), // addr
						F1MonOffsetFromBottom - F2OffsetFromBottom);	// byteCount
	NTV2_POINTER gumpF2(ancF2.GetHostAddress(F2OffsetFromBottom - F2MonOffsetFromBottom), // addr
						F2MonOffsetFromBottom); // byteCount

	if (ancF1 || ancF2)
	{
		//	Import anc packet list that AutoCirculateTransfer's caller put into Xfer struct's Anc buffers (GUMP or RTP).
		//	We're going to add VPID and timecode packets to the list.
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, packetList)))
			return false;	//	Packet import failed

		if (!packetList.IsEmpty())
		{
			const bool	isF1RTP	(ancF1 ? AJARTPAncPayloadHeader::BufferStartsWithRTPHeader(ancF1) : false);
			const bool	isF2RTP	(ancF2 ? AJARTPAncPayloadHeader::BufferStartsWithRTPHeader(ancF2) : false);
			if (isIoIP2110 && isF1RTP && isF2RTP)
			{	//	Generate F1 & F2 GUMP from F1 & F2 RTP...
				packetList.GetSDITransmitData(gumpF1, gumpF2, isProgressive, F2StartLine);
			}
			else
			{
				if (ancF1)
				{
					if (isF1RTP)
					{	//	Caller F1 buffer contains RTP
						if (isIoIP2110)
						{	//	Generate GUMP from packetList...
							NTV2_POINTER	skipF2Data;
							packetList.GetSDITransmitData(gumpF1, skipF2Data, isProgressive, F2StartLine);
						}
					}
					else
					{	//	Caller F1 buffer contains GUMP
						generateRTP = true;	//	Force conversion to RTP
						if (isIoIP2110)
						{	//	Copy GUMP to where the driver expects it...
							const ULWord	gumpLength (std::min(F1MonOffsetFromBottom - F2OffsetFromBottom, gumpF1.GetByteCount()));
							gumpF1.CopyFrom(/*src=*/ancF1,  /*srcOffset=*/0,  /*dstOffset=*/0,  /*byteCount=*/gumpLength);
						}	//	if IoIP
					}	//	if F1 is GUMP
				}	//	if ancF1 non-NULL
				if (ancF2)
				{
					if (isF2RTP)
					{	//	Caller F2 buffer contains RTP
						if (isIoIP2110)
						{	//	Generate GUMP from packetList...
							NTV2_POINTER	skipF1Data;
							packetList.GetSDITransmitData(skipF1Data, gumpF2, isProgressive, F2StartLine);
						}
					}
					else
					{	//	Caller F2 buffer contains GUMP
						generateRTP = true;	//	Force conversion to RTP
						if (isIoIP2110)
						{	//	Copy GUMP to where the driver expects it...
							const ULWord	gumpLength (std::min(F2MonOffsetFromBottom, gumpF2.GetByteCount()));
							gumpF2.CopyFrom(/*src=*/ancF2,  /*srcOffset=*/0,  /*dstOffset=*/0,  /*byteCount=*/gumpLength);
						}	//	if IoIP
					}	//	if F2 is GUMP
				}	//	if ancF2 non-NULL
			}	//	else not IoIP or not F1RTP or not F2RTP
		}	//	if caller supplied any anc
	}	//	if either buffer non-empty/NULL

	if (isMonitoring)	XMTDBG("ORIG: " << packetList);	//	Original packet list from caller

	//	Callers can override our register-based VPID values...
	if (!packetList.CountAncillaryDataWithID(0x41,0x01))			//	If no VPID packets in buffer...
	{
		if (GetSDIOutVPID(vpidA, vpidB, UWord(SDISpigotChannel)))	//	...then we'll add them...
		{
			AJAAncillaryData	vpidPkt;
			vpidPkt.SetDID(0x41);
			vpidPkt.SetSID(0x01);
			vpidPkt.SetLocationVideoLink(AJAAncillaryDataLink_A);
			vpidPkt.SetLocationDataStream(AJAAncillaryDataStream_1);
			vpidPkt.SetLocationDataChannel(AJAAncillaryDataChannel_Y);
			vpidPkt.SetLocationHorizOffset(AJAAncDataHorizOffset_AnyHanc);
			if (vpidA)
			{	//	LinkA/DS1:
				vpidA = ::EndianSwap32NtoH(vpidA);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidA), 4);
				vpidPkt.SetLocationLineNumber(sVPIDLineNumsF1[standard]);
				vpidPkt.GeneratePayloadData();
				packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				}
			}
			if (vpidB)
			{	//	LinkB/DS2:
				vpidB = ::EndianSwap32NtoH(vpidB);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidB), 4);
				vpidPkt.SetLocationVideoLink(AJAAncillaryDataLink_B);
				vpidPkt.SetLocationDataStream(AJAAncillaryDataStream_2);
				vpidPkt.GeneratePayloadData();
				packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					packetList.AddAncillaryData(vpidPkt);	generateRTP = true;
				}
			}
		}	//	if user not inserting his own VPID
		else if (isMonitoring)	{XMTWARN("GetSDIOutVPID failed for SDI spigot " << ::NTV2ChannelToString(SDISpigotChannel,true));}
	}	//	if no VPID pkts in buffer
	else if (isMonitoring)	{XMTDBG(DEC(packetList.CountAncillaryDataWithID(0x41,0x01)) << " VPID packet(s) already provided, won't insert any here");}
	//	IoIP monitor GUMP VPID cannot be overridden -- SDI anc insert always inserts VPID via firmware

	//	Callers can override our register-based RP188 values...
	if (!packetList.CountAncillaryDataWithType(AJAAncillaryDataType_Timecode_ATC)		//	if no caller-specified ATC timecodes...
		&& !packetList.CountAncillaryDataWithType(AJAAncillaryDataType_Timecode_VITC))	//	...and no caller-specified VITC timecodes...
	{
		if (inOutXferInfo.acOutputTimeCodes)		//	...and if there's an output timecode array...
		{
			const AJA_FrameRate	ajaRate		(sNTV2Rate2AJARate[ntv2Rate]);
			const AJATimeBase	ajaTB		(ajaRate);
			const NTV2TCIndexes	tcIndexes	(::GetTCIndexesForSDIConnector(SDISpigotChannel));
			const size_t		maxNumTCs	(inOutXferInfo.acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
			NTV2_RP188 *		pTimecodes	(reinterpret_cast<NTV2_RP188*>(inOutXferInfo.acOutputTimeCodes.GetHostPointer()));

			//	For each timecode index for this channel...
			for (NTV2TCIndexesConstIter it(tcIndexes.begin());  it != tcIndexes.end();  ++it)
			{
				const NTV2TCIndex	tcNdx(*it);
				if (size_t(tcNdx) >= maxNumTCs)
					continue;	//	Skip -- not in the array
				if (!NTV2_IS_SDI_TIMECODE_INDEX(tcNdx))
					continue;	//	Skip -- analog or invalid

				const NTV2_RP188	regTC	(pTimecodes[tcNdx]);
				if (!regTC)
					continue;	//	Skip -- invalid timecode (all FFs)

				const bool isDF = AJATimeCode::QueryIsRP188DropFrame(regTC.fDBB, regTC.fLo, regTC.fHi);

				AJATimeCode						tc;		tc.SetRP188(regTC.fDBB, regTC.fLo, regTC.fHi, ajaTB);
				AJAAncillaryData_Timecode_ATC	atc;	atc.SetTimecode (tc, ajaTB, isDF);
				atc.SetDBB (uint8_t(regTC.fDBB & 0x000000FF), uint8_t(regTC.fDBB & 0x0000FF00 >> 8));
				if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))	//	VITC2?
				{
					atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2);
					atc.SetLocationLineNumber(sVPIDLineNumsF2[standard] - 1);	//	Line 9 in F2
				}
				else
				{	//	F1 -- only consider LTC and VITC1 ... nothing else
					if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))	//	VITC1?
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1);
					else if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx))	//	LTC?
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC);
					else
						continue;
				}
				atc.GeneratePayloadData();
				packetList.AddAncillaryData(atc);	generateRTP = true;
			}	//	for each timecode index value
		}	//	if user not inserting his own ATC/VITC
		else if (isMonitoring)	{XMTWARN("Cannot insert ATC/VITC -- Xfer struct has no acOutputTimeCodes array!");}
	}	//	if no ATC/VITC packets in buffer
	else if (isMonitoring)	{XMTDBG("ATC and/or VITC packet(s) already provided, won't insert any here");}
	//	IoIP monitor GUMP VPID cannot be overridden -- SDI anc inserter inserts RP188 via firmware

	if (generateRTP)	//	if anything added (or forced conversion from GUMP)
	{	//	Re-encode packets into the XferStruct buffers as RTP...
		//XMTDBG("CHGD: " << packetList);	//	DEBUG:  Changed packet list (to be converted to RTP)
		const bool		multiRTPPkt	= inOutXferInfo.acTransferStatus.acState == NTV2_AUTOCIRCULATE_INVALID  ?  true  :  false;
		packetList.SetAllowMultiRTPTransmit(multiRTPPkt);
		NTV2_POINTER rtpF1 (ancF1.GetHostAddress(0),  isIoIP2110  ?  F1OffsetFromBottom - F1MonOffsetFromBottom  :  ancF1.GetByteCount());
		NTV2_POINTER rtpF2 (ancF2.GetHostAddress(0),  isIoIP2110  ?  F2OffsetFromBottom - F2MonOffsetFromBottom  :  ancF2.GetByteCount());
		result = AJA_SUCCESS(packetList.GetIPTransmitData (rtpF1, rtpF2, isProgressive, F2StartLine));
		//if (isIoIP2110)	XMTDBG("F1RTP: " << rtpF1 << " F2RTP: " << rtpF2 << " Xfer: " << inOutXferInfo);
#if 0//defined(_DEBUG)
		DMAWriteAnc(31, rtpF1, rtpF2, NTV2_CHANNEL_INVALID);	//	DEBUG: DMA RTP into frame 31
		if (result)
		{
			AJAAncillaryList	compareRTP;	//	RTP into compareRTP
			NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(rtpF1, rtpF2, compareRTP)));
			//if (packetList.GetAncillaryDataWithID(0x61,0x02)->GetChecksum() != compareRTP.GetAncillaryDataWithID(0x61,0x02)->GetChecksum())
			//XMTDBG("COMPRTP608: " << packetList.GetAncillaryDataWithID(0x61,0x02)->AsString(8) << compareRTP.GetAncillaryDataWithID(0x61,0x02)->AsString(8));
			string compRTP (compareRTP.CompareWithInfo(packetList, /*ignoreLocation*/false, /*ignoreChecksum*/false));
			if (!compRTP.empty())
				XMTWARN("MISCOMPARE: " << compRTP);
		}
		if (isIoIP2110)
		{
			DMAWriteAnc(32, gumpF1, gumpF2, NTV2_CHANNEL_INVALID);	//	DEBUG: DMA GUMP into frame 32
			if (result)
			{
				AJAAncillaryList	compareGUMP;	//	GUMP into compareGUMP
				NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(gumpF1, gumpF2, compareGUMP)));
				//if (packetList.GetAncillaryDataWithID(0x61,0x02)->GetChecksum() != compareGUMP.GetAncillaryDataWithID(0x61,0x02)->GetChecksum())
				//XMTDBG("COMPGUMP608: " << packetList.GetAncillaryDataWithID(0x61,0x02)->AsString(8) << compareGUMP.GetAncillaryDataWithID(0x61,0x02)->AsString(8));
				string compGUMP (compareGUMP.CompareWithInfo(packetList, /*ignoreLocation*/false, /*ignoreChecksum*/false));
				if (!compGUMP.empty())
					XMTWARN("MISCOMPARE: " << compGUMP);
			}
		}	//	IoIP2110
#endif	//	_DEBUG
	}	//	if generateRTP
	return result;

}	//	S2110DeviceAncToXferBuffers


bool CNTV2Card::S2110DeviceAncToBuffers (const NTV2Channel inChannel, NTV2_POINTER & ancF1, NTV2_POINTER & ancF2)
{
	//	IP 2110 Playout:	Add relevant transmit timecodes and VPID to outgoing RTP Anc
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	bool				changed			(false);
	const bool			isMonitoring	(AJADebug::IsActive(AJA_DebugUnit_Anc2110Xmit));
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	const NTV2Channel	SDISpigotChannel(inChannel);	//	DMAWriteAnc usually for OEM clients -- just use inChannel
	ULWord				vpidA(0), vpidB(0);
	AJAAncillaryList	pkts;

	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (!NTV2_IS_VALID_STANDARD(standard))
		return false;	//	Bad standard
	isProgressive = NTV2_IS_PROGRESSIVE_STANDARD(standard);
	if (!ancF1.IsNULL() || !ancF2.IsNULL())
		if (AJA_FAILURE(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, pkts)))
			return false;	//	Packet import failed

	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetLastLine());	//	F2 VANC starts past last line of F1

	//	Non-autocirculate users can transmit VPID two ways:
	//	1)	Insert a VPID packet into the Anc buffer(s) themselves, or
	//	2)	Set the SDI Out VPID register before calling DMAWriteAnc
	//	Extract VPID and place into our SDI In VPID register...
	if (pkts.CountAncillaryDataWithID(0x41,0x01))			//	If no VPID packets in buffer...
	{
		if (GetSDIOutVPID(vpidA, vpidB, UWord(SDISpigotChannel)))	//	...then we'll add them...
		{
			AJAAncillaryData	vpidPkt;
			vpidPkt.SetDID(0x41);
			vpidPkt.SetSID(0x01);
			vpidPkt.SetLocationVideoLink(AJAAncillaryDataLink_A);
			vpidPkt.SetLocationDataStream(AJAAncillaryDataStream_1);
			vpidPkt.SetLocationDataChannel(AJAAncillaryDataChannel_Y);
			vpidPkt.SetLocationHorizOffset(AJAAncDataHorizOffset_AnyHanc);
			if (vpidA)
			{	//	LinkA/DS1:
				vpidA = ::EndianSwap32NtoH(vpidA);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidA), 4);
				vpidPkt.SetLocationLineNumber(sVPIDLineNumsF1[standard]);
				pkts.AddAncillaryData(vpidPkt);			changed = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					pkts.AddAncillaryData(vpidPkt);	changed = true;
				}
			}
			if (vpidB)
			{	//	LinkB/DS2:
				vpidB = ::EndianSwap32NtoH(vpidB);
				vpidPkt.SetPayloadData (reinterpret_cast<uint8_t*>(&vpidB), 4);
				vpidPkt.SetLocationVideoLink(AJAAncillaryDataLink_B);
				vpidPkt.SetLocationDataStream(AJAAncillaryDataStream_2);
				vpidPkt.GeneratePayloadData();
				pkts.AddAncillaryData(vpidPkt);	changed = true;
				if (!isProgressive)
				{	//	Ditto for Field 2...
					vpidPkt.SetLocationLineNumber(sVPIDLineNumsF2[standard]);
					pkts.AddAncillaryData(vpidPkt);	changed = true;
				}
			}
		}	//	if client didn't insert their own VPID
	}	//	if no VPID pkts in buffer
	else if (isMonitoring)	{XMTDBG(DEC(pkts.CountAncillaryDataWithID(0x41,0x01)) << " VPID packet(s) already provided, won't insert any here");}

	//	Non-autocirculate users can transmit timecode two ways:
	//	1)	Insert ATC or VITC packets into the Anc buffers themselves, or
	//	2)	Set output timecode using RP188 registers using CNTV2Card::SetRP188Data,
	//		(but this will only work on newer boards with bidirectional SDI)
	if (!pkts.CountAncillaryDataWithType(AJAAncillaryDataType_Timecode_ATC)		//	if no caller-specified ATC timecodes...
		&& !pkts.CountAncillaryDataWithType(AJAAncillaryDataType_Timecode_VITC))	//	...and no caller-specified VITC timecodes...
	{
		if (::NTV2DeviceHasBiDirectionalSDI(_boardID) && ::NTV2DeviceCanDoStackedAudio(_boardID))	//	if newer device with bidirectional SDI
		{
			const AJA_FrameRate	ajaRate		(sNTV2Rate2AJARate[ntv2Rate]);
			const AJATimeBase	ajaTB		(ajaRate);
			const NTV2TCIndexes	tcIndexes	(::GetTCIndexesForSDIConnector(SDISpigotChannel));
			NTV2_RP188			regTC;

			//	Supposedly, these newer devices support playout readback of their RP188 registers...
			GetRP188Data (inChannel, regTC);
			if (regTC)
			{
				//	For each timecode index for this channel...
				for (NTV2TCIndexesConstIter it(tcIndexes.begin());  it != tcIndexes.end();  ++it)
				{
					const NTV2TCIndex	tcNdx(*it);
					if (!NTV2_IS_SDI_TIMECODE_INDEX(tcNdx))
						continue;	//	Skip -- analog or invalid

					//	TBD:  Does the DBB indicate which timecode it's intended for?
					//	i.e. VITC? LTC? VITC2?
					//	For now, transmit all three...		TBD

					const bool isDF = AJATimeCode::QueryIsRP188DropFrame(regTC.fDBB, regTC.fLo, regTC.fHi);

					AJATimeCode						tc;		tc.SetRP188(regTC.fDBB, regTC.fLo, regTC.fHi, ajaTB);
					AJAAncillaryData_Timecode_ATC	atc;	atc.SetTimecode (tc, ajaTB, isDF);
					atc.AJAAncillaryData_Timecode_ATC::SetDBB (uint8_t(regTC.fDBB & 0x000000FF), uint8_t(regTC.fDBB & 0x0000FF00 >> 8));
					if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))	//	VITC2?
					{
						if (isProgressive)
							continue;	//	Progressive -- skip VITC2
						atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2);
						atc.SetLocationLineNumber(sVPIDLineNumsF2[standard] - 1);	//	Line 9 in F2
					}
					else
					{	//	F1 -- only consider LTC and VITC1 ... nothing else
						if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))	//	VITC1?
							atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1);
						else if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx))	//	LTC?
							atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC);
						else
							continue;
					}
					atc.GeneratePayloadData();
					pkts.AddAncillaryData(atc);				changed = true;
				}	//	for each timecode index value
			}	//	if GetRP188Data returned valid timecode
		}	//	if newer device with bidirectional spigots
	}	//	if client didn't insert their own ATC/VITC
	else if (isMonitoring)	{XMTDBG("ATC and/or VITC packet(s) already provided, won't insert any here");}

	if (changed)
	{	//	We must re-encode packets into the RTP buffers only if anything new was added...
		ancF1.Fill(ULWord(0));	ancF2.Fill(ULWord(0));	//	Clear/reset anc RTP buffers
		//XMTDBG(pkts);
		result = AJA_SUCCESS(pkts.GetIPTransmitData (ancF1, ancF2, isProgressive, F2StartLine));
#if defined(_DEBUG)
		if (result)
		{
			AJAAncillaryList	comparePkts;
			NTV2_ASSERT(AJA_SUCCESS(AJAAncillaryList::SetFromDeviceAncBuffers(ancF1, ancF2, comparePkts)));
			//XMTDBG(comparePkts);
			string compareResult (comparePkts.CompareWithInfo(pkts,false,false));
			if (!compareResult.empty())
				XMTWARN("MISCOMPARE: " << compareResult);
		}
#endif	//	_DEBUG
	}
	return result;

}	//	S2110DeviceAncToBuffers
