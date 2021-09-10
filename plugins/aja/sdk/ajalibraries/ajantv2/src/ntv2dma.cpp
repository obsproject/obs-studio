/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2dma.cpp
	@brief		Implementations of DMA-related CNTV2Card methods.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include <assert.h>
#include <map>


//#define	HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<               uint64_t(__x__)  << dec
#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	LOGGING_DMA_ANC		(AJADebug::IsActive(AJA_DebugUnit_AncGeneric))
#define	DMAANCFAIL(__x__)	AJA_sERROR  (AJA_DebugUnit_AncGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DMAANCWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_AncGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DMAANCNOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_AncGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DMAANCINFO(__x__)	AJA_sINFO   (AJA_DebugUnit_AncGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	DMAANCDBG(__x__)	AJA_sDEBUG  (AJA_DebugUnit_AncGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


using namespace std;


bool CNTV2Card::DMARead (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount, true);
}


bool CNTV2Card::DMAWrite (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast<ULWord*>(pFrameBuffer), inOffsetBytes, inByteCount, true);
}


bool CNTV2Card::DMAReadFrame (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, ULWord(0), inByteCount, true);
}

bool CNTV2Card::DMAReadFrame (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inByteCount, const NTV2Channel inChannel)
{
	NTV2Framesize hwFrameSize(NTV2_FRAMESIZE_INVALID);
	GetFrameBufferSize(inChannel, hwFrameSize);
	ULWord actualFrameSize (::NTV2FramesizeToByteCount(hwFrameSize));
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		actualFrameSize *= 4;
	if (quadQuadEnabled)
		actualFrameSize *= 4;
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, 0, pFrameBuffer, inFrameNumber * actualFrameSize, inByteCount, true);
}


bool CNTV2Card::DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), ULWord(0), inByteCount, true);
}

bool CNTV2Card::DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inByteCount, const NTV2Channel inChannel)
{
	NTV2Framesize hwFrameSize(NTV2_FRAMESIZE_INVALID);
	GetFrameBufferSize(inChannel, hwFrameSize);
	ULWord actualFrameSize (::NTV2FramesizeToByteCount(hwFrameSize));
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		actualFrameSize *= 4;
	if (quadQuadEnabled)
		actualFrameSize *= 4;
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, 0, const_cast<ULWord*>(pFrameBuffer),
						inFrameNumber * actualFrameSize, inByteCount, true);
}


bool CNTV2Card::DMAReadSegments (	const ULWord		inFrameNumber,
									ULWord *			pFrameBuffer,
									const ULWord		inOffsetBytes,
									const ULWord		inTotalByteCount,
									const ULWord		inNumSegments,
									const ULWord		inSegmentHostPitch,
									const ULWord		inSegmentCardPitch)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inTotalByteCount,
						inNumSegments, inSegmentHostPitch, inSegmentCardPitch, true);
}


bool CNTV2Card::DMAWriteSegments (	const ULWord		inFrameNumber,
									const ULWord *		pFrameBuffer,
									const ULWord		inOffsetBytes,
									const ULWord		inTotalByteCount,
									const ULWord		inNumSegments,
									const ULWord		inSegmentHostPitch,
									const ULWord		inSegmentCardPitch)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast<ULWord*>(pFrameBuffer),
						inOffsetBytes, inTotalByteCount, inNumSegments, inSegmentHostPitch, inSegmentCardPitch, true);
}


bool CNTV2Card::DmaP2PTargetFrame (NTV2Channel channel, ULWord frameNumber, ULWord frameOffset, PCHANNEL_P2P_STRUCT pP2PData)
{
	return DmaTransfer (NTV2_PIO, channel, true, frameNumber, frameOffset,  0, 0, 0, 0,  pP2PData);
}


bool CNTV2Card::DmaP2PTransferFrame (NTV2DMAEngine DMAEngine,
									ULWord frameNumber,
									ULWord frameOffset,
									ULWord transferSize,
									ULWord numSegments,
									ULWord segmentTargetPitch,
									ULWord segmentCardPitch,
									PCHANNEL_P2P_STRUCT pP2PData)
{
	return DmaTransfer (DMAEngine, NTV2_CHANNEL1, false, frameNumber, frameOffset, transferSize, numSegments,
					   segmentTargetPitch, segmentCardPitch, pP2PData);
}


bool CNTV2Card::GetAudioMemoryOffset (const ULWord inOffsetBytes,  ULWord & outAbsByteOffset,
										const NTV2AudioSystem inAudioSystem, const bool inCaptureBuffer)
{
	outAbsByteOffset = 0;
	const NTV2DeviceID	deviceID(GetDeviceID());
	if (UWord(inAudioSystem) >= (::NTV2DeviceGetNumAudioSystems(deviceID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;	//	Invalid audio system

	if (::NTV2DeviceCanDoStackedAudio(deviceID))
	{
		const ULWord	EIGHT_MEGABYTES	(0x800000);
		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize(deviceID));
		const ULWord	engineOffset	(memSize  -  EIGHT_MEGABYTES * ULWord(inAudioSystem+1));
		outAbsByteOffset = inOffsetBytes + engineOffset;
	}
	else
	{
		NTV2FrameGeometry		fg	(NTV2_FG_INVALID);
		NTV2FrameBufferFormat	fbf	(NTV2_FBF_INVALID);
		if (!GetFrameGeometry (fg, NTV2Channel(inAudioSystem)) || !GetFrameBufferFormat (NTV2Channel(inAudioSystem), fbf))
			return false;

		const ULWord	audioFrameBuffer	(::NTV2DeviceGetNumberFrameBuffers(deviceID, fg, fbf) - 1);
		outAbsByteOffset = inOffsetBytes  +  audioFrameBuffer * ::NTV2DeviceGetFrameBufferSize(deviceID, fg, fbf);
	}
	if (inCaptureBuffer)
		outAbsByteOffset += 0x400000;	//	Add 4MB offset to point to capture buffer
	return true;
}


bool CNTV2Card::DMAReadAudio (	const NTV2AudioSystem	inAudioSystem,
								ULWord *				pOutAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
	if (!pOutAudioBuffer)
		return false;	//	NULL buffer
	if (!inByteCount)
		return false;	//	Nothing to transfer

	ULWord	absoluteByteOffset	(0);
	if (!GetAudioMemoryOffset (inOffsetBytes,  absoluteByteOffset,  inAudioSystem))
		return false;

	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, 0, pOutAudioBuffer, absoluteByteOffset, inByteCount, true);
}


bool CNTV2Card::DMAWriteAudio (	const NTV2AudioSystem	inAudioSystem,
								const ULWord *			pInAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
	if (!pInAudioBuffer)
		return false;	//	NULL buffer
	if (!inByteCount)
		return false;	//	Nothing to transfer

	ULWord	absoluteByteOffset	(0);
	if (!GetAudioMemoryOffset (inOffsetBytes,  absoluteByteOffset,  inAudioSystem))
		return false;

	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, 0, const_cast <ULWord *> (pInAudioBuffer), absoluteByteOffset, inByteCount, true);
}


#if !defined(NTV2_DEPRECATE_15_2)
	bool CNTV2Card::DMAReadAnc (const ULWord		inFrameNumber,
								UByte *				pOutAncBuffer,
								const NTV2FieldID	inFieldID,
								const ULWord		inByteCount)
	{
		if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
			return false;
		if (!NTV2_IS_VALID_FIELD(inFieldID))
			return false;
	
		NTV2_POINTER	ancF1Buffer	(inFieldID ? AJA_NULL : pOutAncBuffer, inFieldID ? 0 : inByteCount);
		NTV2_POINTER	ancF2Buffer	(inFieldID ? pOutAncBuffer : AJA_NULL, inFieldID ? inByteCount : 0);
		return DMAReadAnc (inFrameNumber, ancF1Buffer, ancF2Buffer);
	}
#endif	//	!defined(NTV2_DEPRECATE_15_2)


bool CNTV2Card::DMAReadAnc (const ULWord		inFrameNumber,
							NTV2_POINTER &	 	outAncF1Buffer,
							NTV2_POINTER &	 	outAncF2Buffer,
							const NTV2Channel	inChannel)
{
	ULWord			F1Offset(0),  F2Offset(0), inByteCount(0), bytesToTransfer(0), byteOffsetToAncData(0);
	NTV2Framesize	hwFrameSize(NTV2_FRAMESIZE_INVALID);
	bool			result(true);
	if (!::NTV2DeviceCanDoCustomAnc(GetDeviceID()))
		return false;
	if (!ReadRegister (kVRegAncField1Offset, F1Offset))
		return false;
	if (!ReadRegister (kVRegAncField2Offset, F2Offset))
		return false;
	if (outAncF1Buffer.IsNULL()  &&  outAncF2Buffer.IsNULL())
		return false;
	if (!GetFrameBufferSize (inChannel, hwFrameSize))
		return false;

	ULWord frameSizeInBytes(::NTV2FramesizeToByteCount(hwFrameSize));
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameSizeInBytes *= 4;
	if (quadQuadEnabled)
		frameSizeInBytes *= 4;

	//	IMPORTANT ASSUMPTION:	F1 data is first (at lower address) in the frame buffer...!
	inByteCount      =  outAncF1Buffer.IsNULL()  ?  0  :  outAncF1Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F1Offset  ?  F1Offset  :  inByteCount;
	if (bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F1Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/true, inFrameNumber,
								reinterpret_cast <ULWord *> (outAncF1Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	inByteCount      =  outAncF2Buffer.IsNULL()  ?  0  :  outAncF2Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F2Offset  ?  F2Offset  :  inByteCount;
	if (result  &&  bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F2Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/true, inFrameNumber,
								reinterpret_cast <ULWord *> (outAncF2Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	if (result  &&  ::NTV2DeviceCanDo2110(_boardID))
		//	S2110 Capture:	So that most OEM ingest apps "just work" with S2110 RTP Anc streams, our
		//					classic SDI Anc data that device firmware normally de-embeds into registers
		//					e.g. VPID & RP188 -- the SDK here will automatically try to do the same.
		S2110DeviceAncFromBuffers (inChannel, outAncF1Buffer, outAncF2Buffer);
	return result;
}


#if !defined(NTV2_DEPRECATE_15_2)
	bool CNTV2Card::DMAWriteAnc (const ULWord		inFrameNumber,
								const UByte *		pInAncBuffer,
								const NTV2FieldID	inFieldID,
								const ULWord		inByteCount)
	{
		if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
			return false;
		if (!NTV2_IS_VALID_FIELD(inFieldID))
			return false;
	
		NTV2_POINTER	ancF1Buffer	(inFieldID ? AJA_NULL : pInAncBuffer, inFieldID ? 0 : inByteCount);
		NTV2_POINTER	ancF2Buffer	(inFieldID ? pInAncBuffer : AJA_NULL, inFieldID ? inByteCount : 0);
		return DMAWriteAnc (inFrameNumber, ancF1Buffer, ancF2Buffer);
	}
#endif	//	!defined(NTV2_DEPRECATE_15_2)


bool CNTV2Card::DMAWriteAnc (const ULWord		inFrameNumber,
							NTV2_POINTER &		inAncF1Buffer,
							NTV2_POINTER &		inAncF2Buffer,
							const NTV2Channel	inChannel)
{
	ULWord			F1Offset(0),  F2Offset(0), inByteCount(0), bytesToTransfer(0), byteOffsetToAncData(0);
	NTV2Framesize	hwFrameSize(NTV2_FRAMESIZE_INVALID);
	bool			result(true);
	if (!::NTV2DeviceCanDoCustomAnc(GetDeviceID()))
		return false;
	if (!ReadRegister (kVRegAncField1Offset, F1Offset))
		return false;
	if (!ReadRegister (kVRegAncField2Offset, F2Offset))
		return false;
	if (inAncF1Buffer.IsNULL()  &&  inAncF2Buffer.IsNULL())
		return false;
	if (!GetFrameBufferSize (inChannel, hwFrameSize))
		return false;

	ULWord frameSizeInBytes(::NTV2FramesizeToByteCount(hwFrameSize));
	bool quadEnabled(false), quadQuadEnabled(false);
	GetQuadFrameEnable(quadEnabled, inChannel);
	GetQuadQuadFrameEnable(quadQuadEnabled, inChannel);
	if (quadEnabled)
		frameSizeInBytes *= 4;
	if (quadQuadEnabled)
		frameSizeInBytes *= 4;

	//	Seamless Anc playout...
	bool	tmpLocalRP188F1AncBuffer(false), tmpLocalRP188F2AncBuffer(false);
	if (::NTV2DeviceCanDo2110(_boardID)  &&  NTV2_IS_VALID_CHANNEL(inChannel))
		//	S2110 Playout:	So that most Retail & OEM playout apps "just work" with S2110 RTP Anc streams,
		//					our classic SDI Anc data that device firmware normally embeds into SDI output
		//					as derived from registers -- e.g. VPID & RP188 -- the SDK here will automatically
		//					insert these packets into the outgoing RTP streams, even if the client didn't
		//					provide Anc buffers. (But they MUST call DMAWriteAnc for this to work!)
		{
			if (inAncF1Buffer.IsNULL())
				tmpLocalRP188F1AncBuffer = inAncF1Buffer.Allocate(2048);
			if (inAncF2Buffer.IsNULL())
				tmpLocalRP188F2AncBuffer = inAncF2Buffer.Allocate(2048);
			S2110DeviceAncToBuffers (inChannel, inAncF1Buffer, inAncF2Buffer);
		}

	//	IMPORTANT ASSUMPTION:	F1 data is first (at lower address) in the frame buffer...!
	inByteCount      =  inAncF1Buffer.IsNULL()  ?  0  :  inAncF1Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F1Offset  ?  F1Offset  :  inByteCount;
	if (bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F1Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/false, inFrameNumber,
								inAncF1Buffer, byteOffsetToAncData, bytesToTransfer, true);
	}
	inByteCount      =  inAncF2Buffer.IsNULL()  ?  0  :  inAncF2Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F2Offset  ?  F2Offset  :  inByteCount;
	if (result  &&  bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F2Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/false, inFrameNumber,
								inAncF2Buffer, byteOffsetToAncData, bytesToTransfer, true);
	}

	if (tmpLocalRP188F1AncBuffer)	inAncF1Buffer.Deallocate();
	if (tmpLocalRP188F2AncBuffer)	inAncF2Buffer.Deallocate();
	return result;
}

bool CNTV2Card::DMAWriteLUTTable (const ULWord inFrameNumber, const ULWord * pInLUTBuffer, const ULWord inLUTIndex, const ULWord inByteCount)
{
	if (!pInLUTBuffer)
		return false;	//	NULL buffer

	ULWord LUTIndexByteOffset = LUTTablePartitionSize * inLUTIndex;

    return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast <ULWord *> (pInLUTBuffer), LUTIndexByteOffset, inByteCount, true);
}


bool CNTV2Card::GetDeviceFrameInfo (const UWord inFrameNumber, const NTV2Channel inChannel, uint64_t & outAddress, uint64_t & outLength)
{
	outAddress = outLength = 0;
	static const ULWord frameSizes[] = {2, 4, 8, 16};	//	'00'=2MB    '01'=4MB    '10'=8MB    '11'=16MB
	UWord				frameSizeNdx(0);
	bool				quadEnabled(false);

	CNTV2DriverInterface::ReadRegister (kRegCh1Control, frameSizeNdx,     kK2RegMaskFrameSize,      kK2RegShiftFrameSize);
	if (::NTV2DeviceCanReportFrameSize(GetDeviceID()))
	{	//	All modern devices
		ULWord quadMultiplier(1);
		if (GetQuadFrameEnable(quadEnabled, inChannel) && quadEnabled)
			quadMultiplier = 8;	//	4!
        if (GetQuadQuadFrameEnable(quadEnabled, inChannel) && quadEnabled)
            quadMultiplier = 32;//	16!
		outLength = frameSizes[frameSizeNdx] * 1024 * 1024 * quadMultiplier;
	}
	else
	{	//	Corvid1, Corvid22, Corvid3G, IoExpress, Kona3G, Kona3GQuad, KonaLHe+, KonaLHi, TTap
		if (::NTV2DeviceSoftwareCanChangeFrameBufferSize(GetDeviceID()))
		{	//	Kona3G only at this point
			bool frameSizeSetBySW(false);
			CNTV2DriverInterface::ReadRegister (kRegCh1Control, frameSizeSetBySW, kRegMaskFrameSizeSetBySW, kRegShiftFrameSizeSetBySW);
			if (!GetQuadFrameEnable(quadEnabled, inChannel) || !quadEnabled)
				if (frameSizeSetBySW)
					outLength = frameSizes[frameSizeNdx] * 1024 * 1024;
		}
	}
	if (!outLength)
	{
		NTV2FrameBufferFormat frameBufferFormat(NTV2_FBF_10BIT_YCBCR);
		GetFrameBufferFormat(NTV2_CHANNEL1, frameBufferFormat);
		NTV2FrameGeometry frameGeometry;
		GetFrameGeometry(frameGeometry, NTV2_CHANNEL1);
		outLength = ::NTV2DeviceGetFrameBufferSize (GetDeviceID(), frameGeometry, frameBufferFormat);
	}
	outAddress = uint64_t(inFrameNumber) * outLength;
	return true;
}

bool CNTV2Card::DeviceAddressToFrameNumber (const uint64_t inAddress,  UWord & outFrameNumber,  const NTV2Channel inChannel)
{
	static const ULWord frameSizes[] = {2, 4, 8, 16};	//	'00'=2MB    '01'=4MB    '10'=8MB    '11'=16MB
	UWord				frameSizeNdx(0);
	bool				quadEnabled(false);
	uint64_t			frameBytes(0);

	outFrameNumber = 0;
	CNTV2DriverInterface::ReadRegister (kRegCh1Control, frameSizeNdx,     kK2RegMaskFrameSize,      kK2RegShiftFrameSize);
	if (::NTV2DeviceCanReportFrameSize(GetDeviceID()))
	{	//	All modern devices
		ULWord quadMultiplier(1);
		if (GetQuadFrameEnable(quadEnabled, inChannel) && quadEnabled)
			quadMultiplier = 8;	//	4!
        if (GetQuadQuadFrameEnable(quadEnabled, inChannel) && quadEnabled)
            quadMultiplier = 32;//	16!
		frameBytes = frameSizes[frameSizeNdx] * 1024 * 1024 * quadMultiplier;
	}
	else
	{	//	Corvid1, Corvid22, Corvid3G, IoExpress, Kona3G, Kona3GQuad, KonaLHe+, KonaLHi, TTap
		if (::NTV2DeviceSoftwareCanChangeFrameBufferSize(GetDeviceID()))
		{	//	Kona3G only at this point
			bool frameSizeSetBySW(false);
			CNTV2DriverInterface::ReadRegister (kRegCh1Control, frameSizeSetBySW, kRegMaskFrameSizeSetBySW, kRegShiftFrameSizeSetBySW);
			if (!GetQuadFrameEnable(quadEnabled, inChannel) || !quadEnabled)
				if (frameSizeSetBySW)
					frameBytes = frameSizes[frameSizeNdx] * 1024 * 1024;
		}
	}
	if (!frameBytes)
	{
		NTV2FrameBufferFormat frameBufferFormat(NTV2_FBF_10BIT_YCBCR);
		GetFrameBufferFormat(NTV2_CHANNEL1, frameBufferFormat);
		NTV2FrameGeometry frameGeometry;
		GetFrameGeometry(frameGeometry, NTV2_CHANNEL1);
		frameBytes = ::NTV2DeviceGetFrameBufferSize (GetDeviceID(), frameGeometry, frameBufferFormat);
	}
	outFrameNumber = UWord(inAddress / frameBytes);
	return true;
}


bool CNTV2Card::DMABufferLock (const NTV2_POINTER & inBuffer, bool inMap, bool inRDMA)
{
	if (!_boardOpened)
		return false;		//	Device not open!

	if (!inBuffer)
		return false;

    if (inRDMA)
    {
        NTV2BufferLock lockMsg (inBuffer, (DMABUFFERLOCK_LOCK | DMABUFFERLOCK_RDMA));
        return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&lockMsg));
    }

    NTV2BufferLock lockMsg (inBuffer, (DMABUFFERLOCK_LOCK | (inMap? DMABUFFERLOCK_MAP : 0)));
    return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&lockMsg));
}


bool CNTV2Card::DMABufferUnlock (const NTV2_POINTER & inBuffer)
{
	if (!_boardOpened)
		return false;		//	Device not open!

	if (!inBuffer)
		return false;

	NTV2BufferLock lockMsg (inBuffer, DMABUFFERLOCK_UNLOCK);
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&lockMsg));
}


bool CNTV2Card::DMABufferUnlockAll (void)
{
	if (!_boardOpened)
		return false;		//	Device not open!

	NTV2BufferLock unlockAllMsg (NTV2_POINTER(), DMABUFFERLOCK_UNLOCK_ALL);
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&unlockAllMsg));
}

bool CNTV2Card::DMABufferAutoLock (const bool inEnable, const bool inMap, const ULWord64 inMaxLockSize)
{
	if (!_boardOpened)
		return false;		//	Device not open!

	NTV2BufferLock autoMsg;
	if (inEnable)
	{
		autoMsg.SetMaxLockSize (inMaxLockSize);
        autoMsg.SetFlags (DMABUFFERLOCK_AUTO | DMABUFFERLOCK_MAX_SIZE | (inMap? DMABUFFERLOCK_MAP : 0));
	}
	else
	{
		autoMsg.SetMaxLockSize (0);
		autoMsg.SetFlags (DMABUFFERLOCK_MANUAL | DMABUFFERLOCK_MAX_SIZE);
	}
	return NTV2Message (reinterpret_cast<NTV2_HEADER*>(&autoMsg));
}

typedef map<ULWord,NTV2AncDataRgn>	OffsetAncRgns, SizeAncRgns;
typedef pair<ULWord,NTV2AncDataRgn>	OffsetAncRgn, SizeAncRgn;
typedef OffsetAncRgns::const_iterator	OffsetAncRgnsConstIter;
typedef OffsetAncRgns::const_reverse_iterator	OffsetAncRgnsConstRIter;
typedef map<NTV2AncDataRgn,ULWord>	AncRgnOffsets, AncRgnSizes;
typedef AncRgnOffsets::const_iterator	AncRgnOffsetsConstIter;
typedef AncRgnSizes::const_iterator		AncRgnSizesConstIter;
typedef pair<NTV2AncDataRgn,ULWord>	AncRgnOffset, AncRgnSize;


bool CNTV2Card::DMAClearAncRegion (const UWord inStartFrameNumber,  const UWord inEndFrameNumber,
									const NTV2AncillaryDataRegion inAncRegion, const NTV2Channel inChannel)
{
	if (!::NTV2DeviceCanDoCustomAnc(GetDeviceID()))
		return false;	//	no anc inserters/extractors

	ULWord offsetInBytes(0), sizeInBytes(0);
	if (!GetAncRegionOffsetAndSize(offsetInBytes, sizeInBytes, inAncRegion))
		return false;	//	no such region
	NTV2_POINTER zeroBuffer(sizeInBytes);
	if (!zeroBuffer)
		return false;
	zeroBuffer.Fill(ULWord64(0));

	for (UWord ndx(inStartFrameNumber);  ndx < inEndFrameNumber + 1;  ndx++)
		if (!DMAWriteAnc (ULWord(ndx), zeroBuffer, zeroBuffer, inChannel))
			return false;	//	DMA write failure
	return true;
}


bool CNTV2Card::GetAncRegionOffsetAndSize (ULWord & outByteOffset,  ULWord & outByteCount, const NTV2AncillaryDataRegion inAncRegion)
{
	outByteOffset = outByteCount = 0;
	if (!::NTV2DeviceCanDoCustomAnc(GetDeviceID()))
		return false;
	if (!NTV2_IS_VALID_ANC_RGN(inAncRegion))
		return false;	//	Bad param

	NTV2Framesize frameSize(NTV2_FRAMESIZE_INVALID);
	if (!GetFrameBufferSize (NTV2_CHANNEL1, frameSize))
		return false;	//	Bail

	const ULWord	frameSizeInBytes(::NTV2FramesizeToByteCount(frameSize));
	ULWord			offsetFromEnd	(0);

	//	Map all Anc memory regions...
	AncRgnOffsets	ancRgnOffsets;
	OffsetAncRgns	offsetAncRgns;
	for (NTV2AncDataRgn ancRgn(NTV2_AncRgn_Field1);  ancRgn < NTV2_MAX_NUM_AncRgns;  ancRgn = NTV2AncDataRgn(ancRgn+1))
	{
		ULWord	bytesFromBottom(0);
		if (GetAncRegionOffsetFromBottom(bytesFromBottom, ancRgn))
		{
			ancRgnOffsets.insert(AncRgnOffset(ancRgn, bytesFromBottom));
			offsetAncRgns.insert(OffsetAncRgn(bytesFromBottom, ancRgn));
		}
	}
	if (offsetAncRgns.empty())
		return false;

	AncRgnSizes	ancRgnSizes;
	for (NTV2AncDataRgn ancRgn(NTV2_AncRgn_Field1);  ancRgn < NTV2_MAX_NUM_AncRgns;  ancRgn = NTV2AncDataRgn(ancRgn+1))
	{
		AncRgnOffsetsConstIter	ancRgnOffsetIter (ancRgnOffsets.find(ancRgn));
		if (ancRgnOffsetIter != ancRgnOffsets.end())
		{
			ULWord	rgnSize(ancRgnOffsetIter->second);		//	Start with ancRgn's offset
			OffsetAncRgnsConstIter	offsetAncRgnIter (offsetAncRgns.find(ancRgnOffsetIter->second)); //	Find ancRgn's offset in offsets table
			if (offsetAncRgnIter != offsetAncRgns.end())
			{
				if (offsetAncRgnIter->second != ancRgn)
					DMAANCWARN(::NTV2AncDataRgnToStr(ancRgn) << " and " << ::NTV2AncDataRgnToStr(offsetAncRgnIter->second) << " using same offset " << xHEX0N(offsetAncRgnIter->first,8));
				else
				{
					if (offsetAncRgnIter != offsetAncRgns.begin() && --offsetAncRgnIter != offsetAncRgns.end())	//	Has a neighbor?
						rgnSize -= offsetAncRgnIter->first;			//	Yes -- subtract neighbor's offset
					ancRgnSizes.insert(AncRgnSize(ancRgn, rgnSize));
					//DMAANCDBG(::NTV2AncDataRgnToStr(ancRgn) << " offset=" << xHEX0N(ancRgnOffsets[ancRgn],8) << " size=" << xHEX0N(rgnSize,8));
				}
			}
			//else DMAANCWARN("Offset " << xHEX0N(ancRgnOffsetIter->second,8) << " missing in OffsetAncRgns table");
		}
		//else DMAANCWARN(::NTV2AncDataRgnToStr(ancRgn) << " missing in AncRgnOffsets table");
	}

	if (NTV2_IS_ALL_ANC_RGNS(inAncRegion))
	{
		//	Use the largest offset-from-end, and the sum of all sizes
		OffsetAncRgnsConstRIter rIter (offsetAncRgns.rbegin());
		if (rIter == offsetAncRgns.rend())
			return false;	//	Empty?
		offsetFromEnd = rIter->first;
		outByteOffset = frameSizeInBytes - offsetFromEnd;	//	Convert to offset from top of frame buffer
		outByteCount = offsetFromEnd;						//	The whole shebang
		return true;
	}

	AncRgnOffsetsConstIter offIter(ancRgnOffsets.find(inAncRegion));
	if (offIter == ancRgnOffsets.end())
		return false;	//	Not there

	offsetFromEnd = offIter->second;
	if (offsetFromEnd > frameSizeInBytes)
		return false;	//	Bad offset

	AncRgnSizesConstIter sizeIter(ancRgnSizes.find(inAncRegion));
	if (sizeIter == ancRgnSizes.end())
		return false;	//	Not there

	outByteOffset = frameSizeInBytes - offsetFromEnd;	//	Convert to offset from top of frame buffer
	outByteCount = sizeIter->second;
	return outByteOffset && outByteCount;
}


bool CNTV2Card::GetAncRegionOffsetFromBottom (ULWord & bytesFromBottom, const NTV2AncillaryDataRegion inAncRegion)
{
	bytesFromBottom = 0;

	if (!::NTV2DeviceCanDoCustomAnc(GetDeviceID()))
		return false;	//	No custom anc support

	//	IoIP SDIOut5 monitor anc support added in SDK/driver 15.3...
	UWord	majV(0), minV(0), pt(0), bld(0);
	GetDriverVersionComponents(majV, minV, pt, bld);
	const bool is153OrLater ((majV > 15)  ||  (majV == 15  &&  minV >= 3)  ||  (!majV && !minV && !pt && !bld));

	switch (inAncRegion)
	{
		default:					return false;	//	Bad index
		case NTV2_AncRgn_Field1:	return ReadRegister(kVRegAncField1Offset, bytesFromBottom)  &&  bytesFromBottom;// F1
		case NTV2_AncRgn_Field2:	return ReadRegister(kVRegAncField2Offset, bytesFromBottom)  &&  bytesFromBottom;// F2
		case NTV2_AncRgn_MonField1:	return is153OrLater && ReadRegister(kVRegMonAncField1Offset, bytesFromBottom) &&  bytesFromBottom;// MonF1
		case NTV2_AncRgn_MonField2:	return is153OrLater && ReadRegister(kVRegMonAncField2Offset, bytesFromBottom) &&  bytesFromBottom;// MonF2
		case NTV2_AncRgn_All:		break;			//	All Anc regions -- calculate below
	}

	//	Read all and determine the largest...
	ULWord temp(0);
	if (ReadRegister(kVRegAncField1Offset, temp)  &&  temp > bytesFromBottom)
		bytesFromBottom = temp;
	if (ReadRegister(kVRegAncField2Offset, temp)  &&  temp > bytesFromBottom)
		bytesFromBottom = temp;

	if (is153OrLater  &&  ((GetDeviceID() == DEVICE_ID_IOIP_2110) ||
						   (GetDeviceID() == DEVICE_ID_IOIP_2110_RGB12)))
	{
		if (ReadRegister(kVRegMonAncField1Offset, temp)  &&  temp > bytesFromBottom)
			bytesFromBottom = temp;
		if (ReadRegister(kVRegMonAncField2Offset, temp)  &&  temp > bytesFromBottom)
			bytesFromBottom = temp;
	}
	return bytesFromBottom > 0;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::DmaRead (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
							 const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaWrite (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
							  const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
								  const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, (ULWord) 0, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaWriteFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
									 const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), (ULWord) 0, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
									const ULWord inOffsetBytes, const ULWord inByteCount,
									const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
									const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount,
							inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
	}

	bool CNTV2Card::DmaWriteSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
									const ULWord inOffsetBytes, const ULWord inByteCount,
									const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
									const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount,
							inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
	}

	bool CNTV2Card::DmaAudioRead (const NTV2DMAEngine inDMAEngine, const NTV2AudioSystem inAudioEngine, ULWord * pOutAudioBuffer,
								 const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4
		if (!::NTV2DeviceCanDoStackedAudio (GetDeviceID ()))
			return false;

		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize (GetDeviceID ()));
		const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
		const ULWord	audioOffset		(inOffsetBytes + engineOffset);

		return DmaTransfer (inDMAEngine, true, 0, pOutAudioBuffer, audioOffset, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaAudioWrite (const NTV2DMAEngine inDMAEngine, const NTV2AudioSystem inAudioEngine, const ULWord * pInAudioBuffer,
								  const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4
		if (!::NTV2DeviceCanDoStackedAudio (GetDeviceID ()))
			return false;

		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize (GetDeviceID ()));
		const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
		const ULWord	audioOffset		(inOffsetBytes + engineOffset);

		return DmaTransfer (inDMAEngine, false, 0, const_cast <ULWord *> (pInAudioBuffer), audioOffset, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID,
								  ULWord * pFrameBuffer, ULWord bytes, bool bSync)
	{
		const ULWord	ulOffset	(fieldID == NTV2_FIELD0 ? 0 : ULWord (_ulFrameBufferSize / 2));
		return DmaTransfer (DMAEngine, true, frameNumber, (ULWord *) pFrameBuffer, ulOffset, bytes, bSync);
	}

	bool CNTV2Card::DmaWriteField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID,
								   ULWord * pFrameBuffer, ULWord bytes, bool bSync)
	{
		const ULWord	ulOffset	(fieldID == NTV2_FIELD0 ? 0 : ULWord (_ulFrameBufferSize / 2));
		return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, ulOffset, bytes, bSync);
	}
#endif	//	!defined (NTV2_DEPRECATE)
