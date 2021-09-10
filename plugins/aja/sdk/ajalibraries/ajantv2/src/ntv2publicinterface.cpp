/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2publicinterface.cpp
	@brief		Implementations of methods declared in 'ntv2publicinterface.h'.
	@copyright	(C) 2016-2021 AJA Video Systems, Inc.
**/

#include "ntv2publicinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ntv2endian.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ntv2registerexpert.h"
#include <iomanip>
#include <locale>		//	For std::locale, std::numpunct, std::use_facet
#include <assert.h>
#include <string.h>		//	For memset, et al.
#include "ntv2rp188.h"
using namespace std;


ostream & operator << (ostream & inOutStream, const UWordSequence & inData)
{
	inOutStream << DEC(inData.size()) << " UWords: ";
	for (UWordSequenceConstIter iter(inData.begin());  iter != inData.end();  )
	{
		inOutStream << HEX0N(*iter,4);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const ULWordSequence & inData)
{
	inOutStream << DEC(inData.size()) << " ULWords: ";
	for (ULWordSequenceConstIter iter(inData.begin());  iter != inData.end();  )
	{
		inOutStream << HEX0N(*iter,8);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const ULWord64Sequence & inData)
{
	inOutStream << DEC(inData.size()) << " ULWord64s: ";
	for (ULWord64SequenceConstIter iter(inData.begin());  iter != inData.end();  )
	{
		inOutStream << HEX0N(*iter,16);
		if (++iter != inData.end())
			inOutStream << " ";
	}
	return inOutStream;
}


NTV2SDIInputStatus::NTV2SDIInputStatus ()
{
	Clear ();
}


void NTV2SDIInputStatus::Clear (void)
{
	mCRCTallyA			= 0;
	mCRCTallyB			= 0;
	mUnlockTally		= 0;
	mFrameRefClockCount	= 0;
	mGlobalClockCount	= 0;
	mFrameTRSError		= false;
	mLocked				= false;
	mVPIDValidA			= false;
	mVPIDValidB			= false;
}


ostream & NTV2SDIInputStatus::Print (ostream & inOutStream) const
{
	inOutStream	<< "[CRCA="			<< DEC(mCRCTallyA)
				<< " CRCB="			<< DEC(mCRCTallyB)
				<< " unlk="			<< xHEX0N(mUnlockTally,8)
				<< " frmRefClkCnt="	<< xHEX0N(mFrameRefClockCount,16)
				<< " globalClkCnt="	<< xHEX0N(mGlobalClockCount,16)
				<< " frmTRS="		<< YesNo(mFrameTRSError)
				<< " locked="		<< YesNo(mLocked)
				<< " VPIDA="		<< YesNo(mVPIDValidA)
				<< " VPIDB="		<< YesNo(mVPIDValidB)
				<< "]";
	return inOutStream;
}


void NTV2HDMIOutputStatus::Clear (void)
{
	mEnabled		= false;
	mPixel420		= false;
	mColorSpace		= NTV2_INVALID_HDMI_COLORSPACE;
	mRGBRange		= NTV2_INVALID_HDMI_RANGE;
	mProtocol		= NTV2_INVALID_HDMI_PROTOCOL;
	mVideoStandard	= NTV2_STANDARD_INVALID;
	mVideoRate		= NTV2_FRAMERATE_UNKNOWN;
	mVideoBitDepth	= NTV2_INVALID_HDMIBitDepth;
	mAudioFormat	= NTV2_AUDIO_FORMAT_INVALID;
	mAudioRate		= NTV2_AUDIO_RATE_INVALID;
	mAudioChannels	= NTV2_INVALID_HDMI_AUDIO_CHANNELS;
}

bool NTV2HDMIOutputStatus::SetFromRegValue (const ULWord inData)
{
	Clear();
	mVideoRate		= NTV2FrameRate((inData & kVRegMaskHDMOutVideoFrameRate) >> kVRegShiftHDMOutVideoFrameRate);
	if (mVideoRate == NTV2_FRAMERATE_UNKNOWN)
		return true;	//	Not enabled -- success
	mEnabled		= true;
	mPixel420		= ((inData & kVRegMaskHDMOutPixel420) >> kVRegShiftHDMOutPixel420) == 1;
	mColorSpace		= NTV2HDMIColorSpace(((inData & kVRegMaskHDMOutColorRGB) >> kVRegShiftHDMOutColorRGB) ? NTV2_HDMIColorSpaceRGB : NTV2_HDMIColorSpaceYCbCr);
	mRGBRange		= NTV2HDMIRange((inData & kVRegMaskHDMOutRangeFull) >> kVRegShiftHDMOutRangeFull);
	mProtocol		= NTV2HDMIProtocol((inData & kVRegMaskHDMOutProtocol) >> kVRegShiftHDMOutProtocol);
	mVideoStandard	= NTV2Standard((inData & kVRegMaskHDMOutVideoStandard) >> kVRegShiftHDMOutVideoStandard);
	mVideoBitDepth	= NTV2HDMIBitDepth((inData & kVRegMaskHDMOutBitDepth) >> kVRegShiftHDMOutBitDepth);
	mAudioFormat	= NTV2AudioFormat((inData & kVRegMaskHDMOutAudioFormat) >> kVRegShiftHDMOutAudioFormat);
	mAudioRate		= NTV2AudioRate((inData & kVRegMaskHDMOutAudioRate) >> kVRegShiftHDMOutAudioRate);
	mAudioChannels	= NTV2HDMIAudioChannels((inData & kVRegMaskHDMOutAudioChannels) >> kVRegShiftHDMOutAudioChannels);
	return true;
}

ostream & NTV2HDMIOutputStatus::Print (ostream & inOutStream) const
{
	inOutStream	<< "Enabled: "			<< YesNo(mEnabled);
	if (mEnabled)
		inOutStream	<< endl
					<< "Is 4:2:0: "			<< YesNo(mPixel420)									<< endl
					<< "Color Space: "		<< ::NTV2HDMIColorSpaceToString(mColorSpace,true)	<< endl;
	if (mColorSpace == NTV2_HDMIColorSpaceRGB)
		inOutStream	<< "RGB Range: "		<< ::NTV2HDMIRangeToString(mRGBRange,true)			<< endl;
	inOutStream		<< "Protocol: "			<< ::NTV2HDMIProtocolToString(mProtocol,true)		<< endl
					<< "Video Standard: "	<< ::NTV2StandardToString(mVideoStandard,true)		<< endl
					<< "Frame Rate: "		<< ::NTV2FrameRateToString(mVideoRate,true)			<< endl
					<< "Bit Depth: "		<< ::NTV2HDMIBitDepthToString(mVideoBitDepth,true)	<< endl
					<< "Audio Format: "		<< ::NTV2AudioFormatToString(mAudioFormat,true)		<< endl
					<< "Audio Rate: "		<< ::NTV2AudioRateToString(mAudioRate,true)			<< endl
					<< "Audio Channels: "	<< ::NTV2HDMIAudioChannelsToString(mAudioChannels,true);
	return inOutStream;
}


AutoCircVidProcInfo::AutoCircVidProcInfo ()
	:	mode						(AUTOCIRCVIDPROCMODE_MIX),
		foregroundVideoCrosspoint	(NTV2CROSSPOINT_CHANNEL1),
		backgroundVideoCrosspoint	(NTV2CROSSPOINT_CHANNEL1),
		foregroundKeyCrosspoint		(NTV2CROSSPOINT_CHANNEL1),
		backgroundKeyCrosspoint		(NTV2CROSSPOINT_CHANNEL1),
		transitionCoefficient		(0),
		transitionSoftness			(0)
{
}


AUTOCIRCULATE_DATA::AUTOCIRCULATE_DATA (const AUTO_CIRC_COMMAND inCommand, const NTV2Crosspoint inCrosspoint)
	:	eCommand	(inCommand),
		channelSpec	(inCrosspoint),
		lVal1		(0),
		lVal2		(0),
		lVal3		(0),
		lVal4		(0),
		lVal5		(0),
		lVal6		(0),
		bVal1		(false),
		bVal2		(false),
		bVal3		(false),
		bVal4		(false),
		bVal5		(false),
		bVal6		(false),
		bVal7		(false),
		bVal8		(false),
		pvVal1		(AJA_NULL),
		pvVal2		(AJA_NULL),
		pvVal3		(AJA_NULL),
		pvVal4		(AJA_NULL)
{
}


NTV2_HEADER::NTV2_HEADER (const ULWord inStructureType, const ULWord inStructSizeInBytes)
	:	fHeaderTag		(NTV2_HEADER_TAG),
		fType			(inStructureType),
		fHeaderVersion	(NTV2_CURRENT_HEADER_VERSION),
		fVersion		(AUTOCIRCULATE_STRUCT_VERSION),
		fSizeInBytes	(inStructSizeInBytes),
		fPointerSize	(sizeof (int *)),
		fOperation		(0),
		fResultStatus	(0)
{
}


ostream & operator << (ostream & inOutStream, const NTV2_HEADER & inObj)
{
	return inObj.Print (inOutStream);
}


ostream & NTV2_HEADER::Print (ostream & inOutStream) const
{
	inOutStream << "[";
	if (NTV2_IS_VALID_HEADER_TAG (fHeaderTag))
		inOutStream << NTV2_4CC_AS_STRING (fHeaderTag);
	else
		inOutStream << "BAD-" << HEX0N(fHeaderTag,8);
	if (NTV2_IS_VALID_STRUCT_TYPE (fType))
		inOutStream << NTV2_4CC_AS_STRING (fType);
	else
		inOutStream << "|BAD-" << HEX0N(fType,8);
	inOutStream << " v" << fHeaderVersion << " vers=" << fVersion << " sz=" << fSizeInBytes;
	return inOutStream << "]";
}


ostream & operator << (ostream & inOutStream, const NTV2_TRAILER & inObj)
{
	inOutStream << "[";
	if (NTV2_IS_VALID_TRAILER_TAG(inObj.fTrailerTag))
		inOutStream << NTV2_4CC_AS_STRING(inObj.fTrailerTag);
	else
		inOutStream << "BAD-" << HEX0N(inObj.fTrailerTag,8);
	return inOutStream << " rawVers=" << xHEX0N(inObj.fTrailerVersion,8) << " clientSDK="
				<< DEC(NTV2SDKVersionDecode_Major(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Minor(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Point(inObj.fTrailerVersion))
				<< "." << DEC(NTV2SDKVersionDecode_Build(inObj.fTrailerVersion)) << "]";
}


ostream & operator << (ostream & inOutStream, const NTV2_POINTER & inObj)
{
	return inObj.Print (inOutStream);
}


ostream & NTV2_POINTER::Print (ostream & inOutStream) const
{
	inOutStream << (IsAllocatedBySDK() ? "0X" : "0x") << HEX0N(GetRawHostPointer(),16) << "/" << DEC(GetByteCount());
	return inOutStream;
}


string NTV2_POINTER::AsString (UWord inDumpMaxBytes) const
{
	ostringstream	oss;
	oss << xHEX0N(GetRawHostPointer(),16) << ":" << DEC(GetByteCount()) << " bytes";
	if (inDumpMaxBytes  &&  GetHostPointer())
	{
		oss << ":";
		if (inDumpMaxBytes > 64)
			inDumpMaxBytes = 64;
		if (ULWord(inDumpMaxBytes) > GetByteCount())
			inDumpMaxBytes = UWord(GetByteCount());
		const UByte *	pBytes	(reinterpret_cast<const UByte *>(GetHostPointer()));
		for (UWord ndx(0);  ndx < inDumpMaxBytes;  ndx++)
			oss << HEX0N(uint16_t(pBytes[ndx]),2);
	}
	return oss.str();
}

static string print_address_offset (const size_t inRadix, const ULWord64 inOffset)
{
	const streamsize	maxAddrWidth (sizeof(ULWord64) * 2);
	ostringstream		oss;
	if (inRadix == 8)
		oss << OCT0N(inOffset,maxAddrWidth) << ": ";
	else if (inRadix == 10)
		oss << DEC0N(inOffset,maxAddrWidth) << ": ";
	else
		oss << HEX0N(inOffset,maxAddrWidth) << ": ";
	return oss.str();
}

ostream & NTV2_POINTER::Dump (	ostream &		inOStream,
								const size_t	inStartOffset,
								const size_t	inByteCount,
								const size_t	inRadix,
								const size_t	inBytesPerGroup,
								const size_t	inGroupsPerRow,
								const size_t	inAddressRadix,
								const bool		inShowAscii,
								const size_t	inAddrOffset) const
{
	if (IsNULL())
		return inOStream;
	if (inRadix != 8 && inRadix != 10 && inRadix != 16 && inRadix != 2)
		return inOStream;
	if (inAddressRadix != 0 && inAddressRadix != 8 && inAddressRadix != 10 && inAddressRadix != 16)
		return inOStream;
	if (inBytesPerGroup == 0)	//	|| inGroupsPerRow == 0)
		return inOStream;

	{
		const void *	pInStartAddress		(GetHostAddress(ULWord(inStartOffset)));
		size_t			bytesRemaining		(inByteCount ? inByteCount : GetByteCount());
		size_t			bytesInThisGroup	(0);
		size_t			groupsInThisRow		(0);
		const unsigned	maxByteWidth		(inRadix == 8 ? 4 : (inRadix == 10 ? 3 : (inRadix == 2 ? 8 : 2)));
		const UByte *	pBuffer				(reinterpret_cast <const UByte *> (pInStartAddress));
		const size_t	asciiBufferSize		(inShowAscii && inGroupsPerRow ? (inBytesPerGroup * inGroupsPerRow + 1) * sizeof (UByte) : 0);	//	Size in bytes, not chars
		UByte *			pAsciiBuffer		(asciiBufferSize ? new UByte[asciiBufferSize / sizeof(UByte)] : AJA_NULL);

		if (!pInStartAddress)
			return inOStream;

		if (pAsciiBuffer)
			::memset (pAsciiBuffer, 0, asciiBufferSize);

		if (inGroupsPerRow && inAddressRadix)
			inOStream << print_address_offset (inAddressRadix, ULWord64(pBuffer) - ULWord64(pInStartAddress) + ULWord64(inAddrOffset));
		while (bytesRemaining)
		{
			if (inRadix == 2)
				inOStream << BIN08(*pBuffer);
			else if (inRadix == 8)
				inOStream << oOCT(uint16_t(*pBuffer));
			else if (inRadix == 10)
				inOStream << DEC0N(uint16_t(*pBuffer),maxByteWidth);
			else if (inRadix == 16)
				inOStream << HEX0N(uint16_t(*pBuffer),2);

			if (pAsciiBuffer)
				pAsciiBuffer[groupsInThisRow * inBytesPerGroup + bytesInThisGroup] = isprint(*pBuffer) ? *pBuffer : '.';
			pBuffer++;
			bytesRemaining--;

			bytesInThisGroup++;
			if (bytesInThisGroup >= inBytesPerGroup)
			{
				groupsInThisRow++;
				if (inGroupsPerRow && groupsInThisRow >= inGroupsPerRow)
				{
					if (pAsciiBuffer)
					{
						inOStream << " " << pAsciiBuffer;
						::memset (pAsciiBuffer, 0, asciiBufferSize);
					}
					inOStream << endl;
					if (inAddressRadix && bytesRemaining)
						inOStream << print_address_offset (inAddressRadix, reinterpret_cast <ULWord64> (pBuffer) - reinterpret_cast <ULWord64> (pInStartAddress) + ULWord64 (inAddrOffset));
					groupsInThisRow = 0;
				}	//	if time for new row
				else
					inOStream << " ";
				bytesInThisGroup = 0;
			}	//	if time for new group
		}	//	loop til no bytes remaining

		if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup && pAsciiBuffer)
		{
			groupsInThisRow++;
			inOStream << string ((inBytesPerGroup - bytesInThisGroup) * maxByteWidth + 1, ' ');
		}

		if (groupsInThisRow)
		{
			if (groupsInThisRow < inGroupsPerRow && pAsciiBuffer)
				inOStream << string (((inGroupsPerRow - groupsInThisRow) * inBytesPerGroup * maxByteWidth + (inGroupsPerRow - groupsInThisRow)), ' ');
			if (pAsciiBuffer)
				inOStream << pAsciiBuffer;
			inOStream << endl;
		}
		else if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup)
			inOStream << endl;

		if (pAsciiBuffer)
			delete [] pAsciiBuffer;
	}	//	else radix is 16, 10, 8 or 2

	return inOStream;
}	//	Dump

string & NTV2_POINTER::Dump (	string &		inOutputString,
								const size_t	inStartOffset,
								const size_t	inByteCount,
								const size_t	inRadix,
								const size_t	inBytesPerGroup,
								const size_t	inGroupsPerRow,
								const size_t	inAddressRadix,
								const bool		inShowAscii,
								const size_t	inAddrOffset) const
{
	ostringstream oss;
	Dump (oss, inStartOffset, inByteCount, inRadix, inBytesPerGroup, inGroupsPerRow, inAddressRadix, inShowAscii, inAddrOffset);
	inOutputString = oss.str();
	return inOutputString;
}

NTV2_POINTER & NTV2_POINTER::Segment (NTV2_POINTER & outPtr, const ULWord inByteOffset, const ULWord inByteCount) const
{
	outPtr.Set(AJA_NULL, 0);	//	Make invalid
	if (inByteOffset >= GetByteCount())
		return outPtr;	//	Offset past end
	if (inByteOffset+inByteCount >= GetByteCount())
		return outPtr;	//	Segment too long
	outPtr.Set(GetHostAddress(inByteOffset), inByteCount);
	return outPtr;
}


bool NTV2_POINTER::GetU64s (ULWord64Sequence & outUint64s, const size_t inU64Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint64s.clear();
	if (IsNULL())
		return false;

	size_t				maxSize	(size_t(GetByteCount()) / sizeof(uint64_t));
	if (maxSize < inU64Offset)
		return false;	//	Past end
	maxSize -= inU64Offset;	//	Remove starting offset

	const uint64_t *	pU64	(reinterpret_cast <const uint64_t *> (GetHostAddress(ULWord(inU64Offset * sizeof(uint64_t)))));
	if (!pU64)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint64s.reserve(maxSize);
		for (size_t ndx(0);  ndx < maxSize;  ndx++)
		{
			const uint64_t	u64	(*pU64++);
			outUint64s.push_back(inByteSwap ? NTV2EndianSwap64(u64) : u64);
		}
	}
	catch (...)
	{
		outUint64s.clear();
		outUint64s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2_POINTER::GetU32s (ULWordSequence & outUint32s, const size_t inU32Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint32s.clear();
	if (IsNULL())
		return false;

	size_t	maxNumU32s	(size_t(GetByteCount()) / sizeof(uint32_t));
	if (maxNumU32s < inU32Offset)
		return false;	//	Past end
	maxNumU32s -= inU32Offset;	//	Remove starting offset

	const uint32_t * pU32 (reinterpret_cast<const uint32_t*>(GetHostAddress(ULWord(inU32Offset * sizeof(uint32_t)))));
	if (!pU32)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxNumU32s)
		maxNumU32s = inMaxSize;

	try
	{
		outUint32s.reserve(maxNumU32s);
		for (size_t ndx(0);  ndx < maxNumU32s;  ndx++)
		{
			const uint32_t	u32	(*pU32++);
			outUint32s.push_back(inByteSwap ? NTV2EndianSwap32(u32) : u32);
		}
	}
	catch (...)
	{
		outUint32s.clear();
		outUint32s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2_POINTER::GetU16s (UWordSequence & outUint16s, const size_t inU16Offset, const size_t inMaxSize, const bool inByteSwap) const
{
	outUint16s.clear();
	if (IsNULL())
		return false;

	size_t				maxSize	(size_t(GetByteCount()) / sizeof(uint16_t));
	if (maxSize < inU16Offset)
		return false;	//	Past end
	maxSize -= inU16Offset;	//	Remove starting offset

	const uint16_t *	pU16	(reinterpret_cast <const uint16_t *> (GetHostAddress(ULWord(inU16Offset * sizeof(uint16_t)))));
	if (!pU16)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint16s.reserve(maxSize);
		for (size_t ndx(0);  ndx < maxSize;  ndx++)
		{
			const uint16_t	u16	(*pU16++);
			outUint16s.push_back(inByteSwap ? NTV2EndianSwap16(u16) : u16);
		}
	}
	catch (...)
	{
		outUint16s.clear();
		outUint16s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2_POINTER::GetU8s (UByteSequence & outUint8s, const size_t inU8Offset, const size_t inMaxSize) const
{
	outUint8s.clear();
	if (IsNULL())
		return false;

	size_t	maxSize	(GetByteCount());
	if (maxSize < inU8Offset)
		return false;	//	Past end
	maxSize -= inU8Offset;	//	Remove starting offset

	const uint8_t *	pU8	(reinterpret_cast <const uint8_t *> (GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outUint8s.reserve(maxSize);
		for (size_t ndx(0);  ndx < maxSize;  ndx++)
		{
			outUint8s.push_back(*pU8++);
		}
	}
	catch (...)
	{
		outUint8s.clear();
		outUint8s.reserve(0);
		return false;
	}
	return true;
}


bool NTV2_POINTER::GetString (std::string & outString, const size_t inU8Offset, const size_t inMaxSize) const
{
	outString.clear();
	if (IsNULL())
		return false;

	size_t maxSize(GetByteCount());
	if (maxSize < inU8Offset)
		return false;		//	Past end
	maxSize -= inU8Offset;	//	Remove starting offset

	const uint8_t *	pU8	(reinterpret_cast <const uint8_t *> (GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Past end

	if (inMaxSize  &&  inMaxSize < maxSize)
		maxSize = inMaxSize;

	try
	{
		outString.reserve(maxSize);
		for (size_t ndx(0);  ndx < maxSize;  ndx++)
			outString += char(*pU8++);
	}
	catch (...)
	{
		outString.clear();
		outString.reserve(0);
		return false;
	}
	return true;
}


bool NTV2_POINTER::PutU64s (const ULWord64Sequence & inU64s, const size_t inU64Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU64s.empty())
		return true;	//	Nothing to copy

	size_t		maxU64s	(GetByteCount() / sizeof(uint64_t));
	uint64_t *	pU64	(reinterpret_cast<uint64_t*>(GetHostAddress(ULWord(inU64Offset * sizeof(uint64_t)))));
	if (!pU64)
		return false;	//	Start offset is past end
	if (maxU64s > inU64Offset)
		maxU64s -= inU64Offset;	//	Don't go past end
	if (maxU64s > inU64s.size())
		maxU64s = inU64s.size();	//	Truncate incoming vector to not go past my end
	if (inU64s.size() > maxU64s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU64s;  ndx++)
#if defined(_DEBUG)
		*pU64++ = inByteSwap ? NTV2EndianSwap64(inU64s.at(ndx)) : inU64s.at(ndx);
#else
		*pU64++ = inByteSwap ? NTV2EndianSwap64(inU64s[ndx]) : inU64s[ndx];
#endif
	return true;
}


bool NTV2_POINTER::PutU32s (const ULWordSequence & inU32s, const size_t inU32Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU32s.empty())
		return true;	//	Nothing to copy

	size_t		maxU32s	(GetByteCount() / sizeof(uint32_t));
	uint32_t *	pU32	(reinterpret_cast<uint32_t*>(GetHostAddress(ULWord(inU32Offset * sizeof(uint32_t)))));
	if (!pU32)
		return false;	//	Start offset is past end
	if (maxU32s > inU32Offset)
		maxU32s -= inU32Offset;	//	Don't go past end
	if (maxU32s > inU32s.size())
		maxU32s = inU32s.size();	//	Truncate incoming vector to not go past my end
	if (inU32s.size() > maxU32s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU32s;  ndx++)
#if defined(_DEBUG)
		*pU32++ = inByteSwap ? NTV2EndianSwap32(inU32s.at(ndx)) : inU32s.at(ndx);
#else
		*pU32++ = inByteSwap ? NTV2EndianSwap32(inU32s[ndx]) : inU32s[ndx];
#endif
	return true;
}


bool NTV2_POINTER::PutU16s (const UWordSequence & inU16s, const size_t inU16Offset, const bool inByteSwap)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU16s.empty())
		return true;	//	Nothing to copy

	size_t		maxU16s	(GetByteCount() / sizeof(uint16_t));
	uint16_t *	pU16	(reinterpret_cast<uint16_t*>(GetHostAddress(ULWord(inU16Offset * sizeof(uint16_t)))));
	if (!pU16)
		return false;	//	Start offset is past end
	if (maxU16s > inU16Offset)
		maxU16s -= inU16Offset;	//	Don't go past end
	if (maxU16s > inU16s.size())
		maxU16s = inU16s.size();	//	Truncate incoming vector to not go past my end
	if (inU16s.size() > maxU16s)
		return false;	//	Will write past end

	for (unsigned ndx(0);  ndx < maxU16s;  ndx++)
#if defined(_DEBUG)
		*pU16++ = inByteSwap ? NTV2EndianSwap16(inU16s.at(ndx)) : inU16s.at(ndx);
#else
		*pU16++ = inByteSwap ? NTV2EndianSwap16(inU16s[ndx]) : inU16s[ndx];
#endif
	return true;
}


bool NTV2_POINTER::PutU8s (const UByteSequence & inU8s, const size_t inU8Offset)
{
	if (IsNULL())
		return false;	//	No buffer or space
	if (inU8s.empty())
		return true;	//	Nothing to copy

	size_t		maxU8s	(GetByteCount());
	uint8_t *	pU8		(reinterpret_cast<uint8_t*>(GetHostAddress(ULWord(inU8Offset))));
	if (!pU8)
		return false;	//	Start offset is past end
	if (maxU8s > inU8Offset)
		maxU8s -= inU8Offset;	//	Don't go past end
	if (maxU8s > inU8s.size())
		maxU8s = inU8s.size();	//	Truncate incoming vector to not go past end
	if (inU8s.size() > maxU8s)
		return false;	//	Will write past end
#if 1
	::memcpy(pU8, &inU8s[0], maxU8s);
#else
	for (unsigned ndx(0);  ndx < maxU8s;  ndx++)
	#if defined(_DEBUG)
		*pU8++ = inU8s.at(ndx);
	#else
		*pU8++ = inU8s[ndx];
	#endif
#endif
	return true;
}


ostream & operator << (ostream & inOutStream, const NTV2_RP188 & inObj)
{
	if (inObj.IsValid ())
		return inOutStream << "{Dx" << HEX0N(inObj.fDBB,8) << "|Lx" << HEX0N(inObj.fLo,8) << "|Hx" << HEX0N(inObj.fHi,8) << "}";
	else
		return inOutStream << "{invalid}";
}


NTV2TimeCodeList & operator << (NTV2TimeCodeList & inOutList, const NTV2_RP188 & inRP188)
{
	inOutList.push_back (inRP188);
	return inOutList;
}


ostream & operator << (ostream & inOutStream, const NTV2TimeCodeList & inObj)
{
	inOutStream << inObj.size () << ":[";
	for (NTV2TimeCodeListConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << *iter;
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream << "]";
}


ostream & operator << (std::ostream & inOutStream, const NTV2TimeCodes & inObj)
{
	inOutStream << inObj.size () << ":[";
	for (NTV2TimeCodesConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (iter->first,true) << "=" << iter->second;
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream << "]";
}


ostream & operator << (std::ostream & inOutStream, const NTV2TCIndexes & inObj)
{
	for (NTV2TCIndexesConstIter iter (inObj.begin ());  iter != inObj.end ();  )
	{
		inOutStream << ::NTV2TCIndexToString (*iter);
		if (++iter != inObj.end ())
			inOutStream << ", ";
	}
	return inOutStream;
}


NTV2TCIndexes & operator += (NTV2TCIndexes & inOutSet, const NTV2TCIndexes & inSet)
{
	for (NTV2TCIndexesConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


ostream & operator << (ostream & inOutStream, const FRAME_STAMP & inObj)
{
	return inOutStream	<< inObj.acHeader
						<< " frmTime="			<< inObj.acFrameTime
						<< " reqFrm="			<< inObj.acRequestedFrame
						<< " audClkTS="			<< inObj.acAudioClockTimeStamp
						<< " audExpAdr="		<< hex << inObj.acAudioExpectedAddress << dec
						<< " audInStrtAdr="		<< hex << inObj.acAudioInStartAddress << dec
						<< " audInStopAdr="		<< hex << inObj.acAudioInStopAddress << dec
						<< " audOutStrtAdr="	<< hex << inObj.acAudioOutStartAddress << dec
						<< " audOutStopAdr="	<< hex << inObj.acAudioOutStopAddress << dec
						<< " totBytes="			<< inObj.acTotalBytesTransferred
						<< " strtSamp="			<< inObj.acStartSample
						<< " curTime="			<< inObj.acCurrentTime
						<< " curFrm="			<< inObj.acCurrentFrame
						<< " curFrmTime="		<< inObj.acCurrentFrameTime
						<< " audClkCurTime="	<< inObj.acAudioClockCurrentTime
						<< " curAudExpAdr="		<< hex << inObj.acCurrentAudioExpectedAddress << dec
						<< " curAudStrtAdr="	<< hex << inObj.acCurrentAudioStartAddress << dec
						<< " curFldCnt="		<< inObj.acCurrentFieldCount
						<< " curLnCnt="			<< inObj.acCurrentLineCount
						<< " curReps="			<< inObj.acCurrentReps
						<< " curUsrCookie="		<< hex << inObj.acCurrentUserCookie << dec
						<< " acFrame="			<< inObj.acFrame
						<< " acRP188="			<< inObj.acRP188	//	deprecated
						<< " "					<< inObj.acTrailer;
}


ostream & operator << (ostream & inOutStream, const NTV2SegmentedDMAInfo & inObj)
{
	if (inObj.acNumSegments > 1)
		inOutStream << "segs=" << inObj.acNumSegments << " numActBPR=" << inObj.acNumActiveBytesPerRow
					<< " segHostPitc=" << inObj.acSegmentHostPitch << " segDevPitc=" << inObj.acSegmentDevicePitch;
	else
		inOutStream << "n/a";
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const AUTOCIRCULATE_TRANSFER & inObj)
{
	#if defined (_DEBUG)
		NTV2_ASSERT (inObj.NTV2_IS_STRUCT_VALID ());
	#endif
	string	str	(::NTV2FrameBufferFormatToString (inObj.acFrameBufferFormat, true));
	while (str.find (' ') != string::npos)
		str.erase (str.find (' '), 1);
	inOutStream << inObj.acHeader << " vid=" << inObj.acVideoBuffer
				<< " aud=" << inObj.acAudioBuffer
				<< " ancF1=" << inObj.acANCBuffer
				<< " ancF2=" << inObj.acANCField2Buffer
				<< " outTC(" << inObj.acOutputTimeCodes << ")"
				<< " cookie=" << inObj.acInUserCookie
				<< " vidDMAoff=" << inObj.acInVideoDMAOffset
				<< " segDMA=" << inObj.acInSegmentedDMAInfo
				<< " colcor=" << inObj.acColorCorrection
				<< " fbf=" << str
				<< " fbo=" << (inObj.acFrameBufferOrientation == NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP ? "flip" : "norm")
				<< " vidProc=" << inObj.acVidProcInfo
				<< " quartsz=" << inObj.acVideoQuarterSizeExpand
				<< " p2p=" << inObj.acPeerToPeerFlags
				<< " repCnt=" << inObj.acFrameRepeatCount
				<< " desFrm=" << inObj.acDesiredFrame
				<< " rp188=" << inObj.acRP188		//	deprecated
				<< " xpt=" << inObj.acCrosspoint
				<< " status{" << inObj.acTransferStatus << "}"
				<< " " << inObj.acTrailer;
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const AUTOCIRCULATE_TRANSFER_STATUS & inObj)
{
	inOutStream	<< inObj.acHeader << " state=" << ::NTV2AutoCirculateStateToString (inObj.acState)
				<< " xferFrm=" << inObj.acTransferFrame
				<< " bufLvl=" << inObj.acBufferLevel
				<< " frms=" << inObj.acFramesProcessed
				<< " drops=" << inObj.acFramesDropped
				<< " " << inObj.acFrameStamp
				<< " audXfrSz=" << inObj.acAudioTransferSize
				<< " audStrtSamp=" << inObj.acAudioStartSample
				<< " ancF1Siz=" << inObj.acAncTransferSize
				<< " ancF2Siz=" << inObj.acAncField2TransferSize
				<< " " << inObj.acTrailer;
	return inOutStream;
}


ostream & operator << (ostream & inOutStream, const NTV2RegisterValueMap & inObj)
{
	NTV2RegValueMapConstIter	iter	(inObj.begin ());
	inOutStream << "RegValues:" << inObj.size () << "[";
	while (iter != inObj.end ())
	{
		const NTV2RegisterNumber	registerNumber	(static_cast <const NTV2RegisterNumber> (iter->first));
		const ULWord				registerValue	(iter->second);
		inOutStream	<< ::NTV2RegisterNumberToString (registerNumber) << "=0x" << hex << registerValue << dec;
		if (++iter != inObj.end ())
			inOutStream << ",";
	}
	return inOutStream << "]";
}


ostream & operator << (ostream & inOutStream, const AutoCircVidProcInfo & inObj)
{
	return inOutStream	<< "{mode=" << ::AutoCircVidProcModeToString (inObj.mode, true)
						<< ", FGvid=" << ::NTV2CrosspointToString (inObj.foregroundVideoCrosspoint)
						<< ", BGvid=" << ::NTV2CrosspointToString (inObj.backgroundVideoCrosspoint)
						<< ", FGkey=" << ::NTV2CrosspointToString (inObj.foregroundKeyCrosspoint)
						<< ", BGkey=" << ::NTV2CrosspointToString (inObj.backgroundKeyCrosspoint)
						<< ", transCoeff=" << inObj.transitionCoefficient
						<< ", transSoftn=" << inObj.transitionSoftness << "}";
}


ostream & operator << (ostream & inOutStream, const NTV2ColorCorrectionData & inObj)
{
	return inOutStream	<< "{ccMode=" << ::NTV2ColorCorrectionModeToString (inObj.ccMode)
						<< ", ccSatVal=" << inObj.ccSaturationValue
						<< ", ccTables=" << inObj.ccLookupTables << "}";
}


//	Implementation of NTV2VideoFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2VideoFormatSet & inFormats)
{
	NTV2VideoFormatSet::const_iterator	iter	(inFormats.begin ());

	inOStream	<< inFormats.size ()
				<< (inFormats.size () == 1 ? " video format:  " : " video format(s):  ");

	while (iter != inFormats.end ())
	{
		inOStream << std::string (::NTV2VideoFormatToString (*iter));
		inOStream << (++iter == inFormats.end () ? "" : ", ");
	}

	return inOStream;

}	//	operator <<


//	Implementation of NTV2FrameBufferFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2FrameBufferFormatSet & inFormats)
{
	NTV2FrameBufferFormatSetConstIter	iter	(inFormats.begin ());

	inOStream	<< inFormats.size ()
				<< (inFormats.size () == 1 ? " pixel format:  " : " pixel formats:  ");

	while (iter != inFormats.end ())
	{
		inOStream << ::NTV2FrameBufferFormatToString (*iter);
		inOStream << (++iter == inFormats.end ()  ?  ""  :  ", ");
	}

	return inOStream;

}	//	operator <<


NTV2FrameBufferFormatSet & operator += (NTV2FrameBufferFormatSet & inOutSet, const NTV2FrameBufferFormatSet inFBFs)
{
	for (NTV2FrameBufferFormatSetConstIter iter (inFBFs.begin ());  iter != inFBFs.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


//	Implementation of NTV2StandardSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2StandardSet & inStandards)
{
	NTV2StandardSetConstIter	iter	(inStandards.begin ());

	inOStream	<< inStandards.size ()
				<< (inStandards.size () == 1 ? " standard:  " : " standards:  ");

	while (iter != inStandards.end ())
	{
		inOStream << ::NTV2StandardToString(*iter);
		inOStream << (++iter == inStandards.end ()  ?  ""  :  ", ");
	}

	return inOStream;
}


NTV2StandardSet & operator += (NTV2StandardSet & inOutSet, const NTV2StandardSet inSet)
{
	for (NTV2StandardSetConstIter iter(inSet.begin ());  iter != inSet.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2GeometrySet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2GeometrySet & inGeometries)
{
	NTV2GeometrySetConstIter	iter	(inGeometries.begin ());
	inOStream	<< inGeometries.size ()
				<< (inGeometries.size () == 1 ? " geometry:  " : " geometries:  ");
	while (iter != inGeometries.end ())
	{
		inOStream << ::NTV2FrameGeometryToString(*iter);
		inOStream << (++iter == inGeometries.end ()  ?  ""  :  ", ");
	}
	return inOStream;
}


NTV2GeometrySet & operator += (NTV2GeometrySet & inOutSet, const NTV2GeometrySet inSet)
{
	for (NTV2GeometrySetConstIter iter(inSet.begin ());  iter != inSet.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


//	Implementation of NTV2FrameBufferFormatSet's ostream writer...
ostream & operator << (ostream & inOStream, const NTV2InputSourceSet & inSet)
{
	NTV2InputSourceSetConstIter	iter(inSet.begin());
	inOStream	<< inSet.size()
				<< (inSet.size() == 1 ? " input:  " : " inputs:  ");
	while (iter != inSet.end())
	{
		inOStream << ::NTV2InputSourceToString (*iter);
		inOStream << (++iter == inSet.end()  ?  ""  :  ", ");
	}
	return inOStream;
}	//	operator <<


NTV2InputSourceSet & operator += (NTV2InputSourceSet & inOutSet, const NTV2InputSourceSet & inSet)
{
	for (NTV2InputSourceSetConstIter iter (inSet.begin ());  iter != inSet.end ();  ++iter)
		inOutSet.insert (*iter);
	return inOutSet;
}


ostream & operator << (ostream & inOStream, const NTV2OutputDestinations & inSet)
{
	NTV2OutputDestinationsConstIter	iter(inSet.begin());
	inOStream	<< inSet.size()
				<< (inSet.size() == 1 ? " output:  " : " outputs:  ");
	while (iter != inSet.end())
	{
		inOStream << ::NTV2OutputDestinationToString(*iter);
		inOStream << (++iter == inSet.end()  ?  ""  :  ", ");
	}
	return inOStream;
}


NTV2OutputDestinations & operator += (NTV2OutputDestinations & inOutSet, const NTV2OutputDestinations & inSet)
{
	for (NTV2OutputDestinationsConstIter iter(inSet.begin());  iter != inSet.end();  ++iter)
		inOutSet.insert(*iter);
	return inOutSet;
}


ostream & operator << (ostream & inOutStrm, const NTV2SegmentedXferInfo & inRun)
{
	return inRun.Print(inOutStrm);
}


//	Implementation of NTV2AutoCirculateStateToString...
string NTV2AutoCirculateStateToString (const NTV2AutoCirculateState inState)
{
	static const char *	sStateStrings []	= {	"Disabled", "Initializing", "Starting", "Paused", "Stopping", "Running", "StartingAtTime", AJA_NULL};
	if (inState >= NTV2_AUTOCIRCULATE_DISABLED && inState <= NTV2_AUTOCIRCULATE_STARTING_AT_TIME)
		return string (sStateStrings [inState]);
	else
		return "<invalid>";
}



NTV2_TRAILER::NTV2_TRAILER ()
	:	fTrailerVersion		(NTV2SDKVersionEncode(AJA_NTV2_SDK_VERSION_MAJOR, AJA_NTV2_SDK_VERSION_MINOR, AJA_NTV2_SDK_VERSION_POINT, AJA_NTV2_SDK_BUILD_NUMBER)),
		fTrailerTag			(NTV2_TRAILER_TAG)
{
}


static const string sSegXferUnits[] = {"", " U8", " U16", "", " U32", "", "", "", " U64", ""};

ostream & NTV2SegmentedXferInfo::Print (ostream & inStrm, const bool inDumpSegments) const
{
	if (!isValid())
		return inStrm << "(invalid)";
	if (inDumpSegments)
	{
		//	TBD
	}
	else
	{
		inStrm	<< DEC(getSegmentCount()) << " x " << DEC(getSegmentLength())
				<< sSegXferUnits[getElementLength()] << " segs";
		if (getSourceOffset())
			inStrm	<< " srcOff=" << xHEX0N(getSourceOffset(),8);
		if (getSegmentCount() > 1)
			inStrm << " srcSpan=" << xHEX0N(getSourcePitch(),8) << (isSourceBottomUp()?" VF":"");
		if (getDestOffset())
			inStrm	<< " dstOff=" << xHEX0N(getDestOffset(),8);
		if (getSegmentCount() > 1)
			inStrm << " dstSpan=" << xHEX0N(getDestPitch(),8) << (isDestBottomUp()?" VF":"");
		inStrm << " totElm=" << DEC(getTotalElements()) << " totByt=" << xHEX0N(getTotalBytes(),8);
	}
	return inStrm;
}

string NTV2SegmentedXferInfo::getSourceCode (const bool inInclDecl) const
{
	static string var("segInfo");
	ostringstream oss;
	string units("\t// bytes");
	if (!isValid())
		return "";
	if (inInclDecl)
		oss << "NTV2SegmentedXferInfo " << var << ";" << endl;
	if (getElementLength() > 1)
	{
		units = "\t// " + sSegXferUnits[getElementLength()] + "s";
		oss << var << ".setElementLength(" << getElementLength() << ");" << endl;
	}
	oss << var << ".setSegmentCount(" << DEC(getSegmentCount()) << ");" << endl;
	oss << var << ".setSegmentLength(" << DEC(getSegmentLength()) << ");" << units << endl;
	if (getSourceOffset())
		oss << var << ".setSourceOffset(" << DEC(getSourceOffset()) << ");" << units << endl;
	oss << var << ".setSourcePitch(" << DEC(getSourcePitch()) << ");" << units << endl;
	if (isSourceBottomUp())
		oss << var << ".setSourceDirection(false);" << endl;
	if (getDestOffset())
		oss << var << ".setDestOffset(" << DEC(getDestOffset()) << ");" << units << endl;
	if (getDestPitch())
		oss << var << ".setDestPitch(" << DEC(getDestPitch()) << ");" << units << endl;
	if (isDestBottomUp())
		oss << var << ".setDestDirection(false);" << endl;
	return oss.str();
}

bool NTV2SegmentedXferInfo::containsElementAtOffset (const ULWord inElementOffset) const
{
	if (!isValid())
		return false;
	if (getSegmentCount() == 1)
	{
		if (inElementOffset >= getSourceOffset())
			if (inElementOffset < getSourceOffset()+getSegmentLength())
				return true;
		return false;
	}
	ULWord offset(getSourceOffset());
	for (ULWord seg(0);  seg < getSegmentCount();  seg++)
	{
		if (inElementOffset < offset)
			return false;	//	past element of interest already
		if (inElementOffset < offset+getSegmentLength())
			return true;	//	must be within this segment
		offset += getSourcePitch();	//	skip to next segment
	}
	return false;
}

bool NTV2SegmentedXferInfo::operator != (const NTV2SegmentedXferInfo & inRHS) const
{
	if (getElementLength() != inRHS.getElementLength())
		//	FUTURE TBD:  Need to transform RHS to match ElementLength so as to make apples-to-apples comparison
		return true;	//	For now, fail
	if (getSegmentCount() != inRHS.getSegmentCount())
		return true;
	if (getSegmentLength() != inRHS.getSegmentLength())
		return true;
	if (getSourceOffset() != inRHS.getSourceOffset())
		return true;
	if (getSourcePitch() != inRHS.getSourcePitch())
		return true;
	if (getDestOffset() != inRHS.getDestOffset())
		return true;
	if (getDestPitch() != inRHS.getDestPitch())
		return true;
	return false;
}

NTV2SegmentedXferInfo &	NTV2SegmentedXferInfo::reset (void)
{
	mFlags				= 0;
	mNumSegments		= 0;
	mElementsPerSegment	= 0;
	mInitialSrcOffset	= 0;
	mInitialDstOffset	= 0;
	mSrcElementsPerRow	= 0;
	mDstElementsPerRow	= 0;
	setElementLength(1);	//	elements == bytes
	return *this;
}

NTV2SegmentedXferInfo & NTV2SegmentedXferInfo::swapSourceAndDestination (void)
{
	std::swap(mSrcElementsPerRow, mDstElementsPerRow);
	std::swap(mInitialSrcOffset, mInitialDstOffset);
	return *this;
}


NTV2_POINTER::NTV2_POINTER (const void * pInUserPointer, const size_t inByteCount)
	:	fUserSpacePtr		(inByteCount ? NTV2_POINTER_TO_ULWORD64(pInUserPointer) : 0),
		fByteCount			(static_cast<ULWord>(pInUserPointer ? inByteCount : 0)),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
}


NTV2_POINTER::NTV2_POINTER (const size_t inByteCount)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
	if (inByteCount)
		if (Allocate(inByteCount))
			Fill(UByte(0));
}


NTV2_POINTER::NTV2_POINTER (const NTV2_POINTER & inObj)
	:	fUserSpacePtr		(0),
		fByteCount			(0),
		fFlags				(0),
	#if defined (AJAMac)
		fKernelSpacePtr		(0),
		fIOMemoryDesc		(0),
		fIOMemoryMap		(0)
	#else
		fKernelHandle		(0)
	#endif
{
	if (Allocate(inObj.GetByteCount()))
		SetFrom(inObj);
}


NTV2_POINTER & NTV2_POINTER::operator = (const NTV2_POINTER & inRHS)
{
	if (&inRHS != this)
	{
		if (inRHS.IsNULL())
			Set (AJA_NULL, 0);
		else if (GetByteCount() == inRHS.GetByteCount())
			SetFrom(inRHS);
		else if (Allocate(inRHS.GetByteCount()))
			SetFrom(inRHS);
		else
			{;}	//	Error
	}
	return *this;
}


NTV2_POINTER::~NTV2_POINTER ()
{
	Set (AJA_NULL, 0);	//	Call 'Set' to delete the array (if I allocated it)
}


bool NTV2_POINTER::ByteSwap64 (void)
{
	uint64_t *	pU64s(reinterpret_cast<uint64_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint64_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);  ndx < loopCount;  ndx++)
		pU64s[ndx] = NTV2EndianSwap64(pU64s[ndx]);
	return true;
}


bool NTV2_POINTER::ByteSwap32 (void)
{
	uint32_t *	pU32s(reinterpret_cast<uint32_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint32_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);  ndx < loopCount;  ndx++)
		pU32s[ndx] = NTV2EndianSwap32(pU32s[ndx]);
	return true;
}


bool NTV2_POINTER::ByteSwap16 (void)
{
	uint16_t *	pU16s(reinterpret_cast<uint16_t*>(GetHostPointer()));
	const size_t loopCount(GetByteCount() / sizeof(uint16_t));
	if (IsNULL())
		return false;
	for (size_t ndx(0);  ndx < loopCount;  ndx++)
		pU16s[ndx] = NTV2EndianSwap16(pU16s[ndx]);
	return true;
}


bool NTV2_POINTER::Set (const void * pInUserPointer, const size_t inByteCount)
{
	Deallocate();
	fUserSpacePtr = inByteCount ? NTV2_POINTER_TO_ULWORD64(pInUserPointer) : 0;
	fByteCount = static_cast<ULWord>(pInUserPointer ? inByteCount : 0);
	//	Return true only if both UserPointer and ByteCount are non-zero, or both are zero.
	return (pInUserPointer && inByteCount)  ||  (!pInUserPointer && !inByteCount);
}


bool NTV2_POINTER::SetAndFill (const void * pInUserPointer, const size_t inByteCount, const UByte inValue)
{
	return Set(pInUserPointer, inByteCount)  &&  Fill(inValue);
}


bool NTV2_POINTER::Allocate (const size_t inByteCount, const bool inPageAligned)
{
	if (GetByteCount()  &&  fFlags & NTV2_POINTER_ALLOCATED)	//	If already was Allocated
		if (inByteCount == GetByteCount())						//	If same byte count
		{
			::memset (GetHostPointer(), 0, GetByteCount());		//	Zero it...
			return true;										//	...and return true
		}

	bool result(Set(AJA_NULL, 0));	//	Jettison existing buffer (if any)
	if (inByteCount)
	{	//	Allocate the byte array, and call Set...
		UByte * pBuffer(AJA_NULL);
		result = false;
		if (inPageAligned)
			pBuffer = reinterpret_cast<UByte*>(AJAMemory::AllocateAligned(inByteCount, DefaultPageSize()));
		else
			try
				{pBuffer = new UByte[inByteCount];}
			catch (const std::bad_alloc &)
				{pBuffer = AJA_NULL;}
		if (pBuffer  &&  Set(pBuffer, inByteCount))
		{	//	SDK owns this memory -- set NTV2_POINTER_ALLOCATED bit -- I'm responsible for deleting
			result = true;
			fFlags |= NTV2_POINTER_ALLOCATED;
			if (inPageAligned)
				fFlags |= NTV2_POINTER_PAGE_ALIGNED;		//	Set "page aligned" flag
			::memset (GetHostPointer(), 0, inByteCount);	//	Zero it
		}
	}	//	if requested size is non-zero
	return result;
}


bool NTV2_POINTER::Deallocate (void)
{
	if (fFlags & NTV2_POINTER_ALLOCATED)
	{
		if (!IsNULL())
		{
			if (fFlags & NTV2_POINTER_PAGE_ALIGNED)
			{
				AJAMemory::FreeAligned(GetHostPointer());
				fFlags &= ~NTV2_POINTER_PAGE_ALIGNED;
			}
			else
				delete [] reinterpret_cast<UByte*>(GetHostPointer());
		}
		fUserSpacePtr = 0;
		fByteCount = 0;
		fFlags &= ~NTV2_POINTER_ALLOCATED;
	}
	return true;
}


void * NTV2_POINTER::GetHostAddress (const ULWord inByteOffset, const bool inFromEnd) const
{
	if (IsNULL())
		return AJA_NULL;
	if (inByteOffset >= GetByteCount())
		return AJA_NULL;
	UByte *	pBytes	(reinterpret_cast<UByte*>(GetHostPointer()));
	if (inFromEnd)
		pBytes += GetByteCount() - inByteOffset;
	else
		pBytes += inByteOffset;
	return pBytes;
}


bool NTV2_POINTER::SetFrom (const NTV2_POINTER & inBuffer)
{
	if (inBuffer.IsNULL())
		return false;	//	NULL or empty
	if (IsNULL())
		return false;	//	I am NULL or empty
	if (inBuffer.GetByteCount() == GetByteCount()  &&  inBuffer.GetHostPointer() == GetHostPointer())
		return true;	//	Same buffer

	size_t bytesToCopy(inBuffer.GetByteCount());
	if (bytesToCopy > GetByteCount())
		bytesToCopy = GetByteCount();
	::memcpy (GetHostPointer(), inBuffer.GetHostPointer(), bytesToCopy);
	return true;
}


bool NTV2_POINTER::CopyFrom (const void * pInSrcBuffer, const ULWord inByteCount)
{
	if (!inByteCount)
		return Set (AJA_NULL, 0);	//	Zero bytes
	if (!pInSrcBuffer)
		return false;	//	NULL src ptr
	if (!Allocate (inByteCount))
		return false;	//	Resize failed
	::memcpy (GetHostPointer(), pInSrcBuffer, inByteCount);
	return true;
}


bool NTV2_POINTER::CopyFrom (const NTV2_POINTER & inBuffer,
							const ULWord inSrcByteOffset, const ULWord inDstByteOffset, const ULWord inByteCount)
{
	if (inBuffer.IsNULL() || IsNULL())
		return false;	//	NULL or empty
	if (inSrcByteOffset + inByteCount > inBuffer.GetByteCount())
		return false;	//	Past end of src
	if (inDstByteOffset + inByteCount > GetByteCount())
		return false;	//	Past end of me

	const UByte *	pSrc	(reinterpret_cast<const UByte*>(inBuffer.GetHostPointer()));
	pSrc += inSrcByteOffset;

	UByte *			pDst	(reinterpret_cast<UByte*>(GetHostPointer()));
	pDst += inDstByteOffset;

	::memcpy (pDst, pSrc, inByteCount);
	return true;
}


bool NTV2_POINTER::CopyFrom (const NTV2_POINTER & inSrcBuffer, const NTV2SegmentedXferInfo & inXferInfo)
{
	if (!inXferInfo.isValid()  ||  inSrcBuffer.IsNULL()  ||  IsNULL())
		return false;

	//	Copy every segment...
	ULWord			srcOffset	(inXferInfo.getSourceOffset() * inXferInfo.getElementLength());
	ULWord			dstOffset	(inXferInfo.getDestOffset() * inXferInfo.getElementLength());
	const ULWord	srcPitch	(inXferInfo.getSourcePitch() * inXferInfo.getElementLength());
	const ULWord	dstPitch	(inXferInfo.getDestPitch() * inXferInfo.getElementLength());
	const ULWord	bytesPerSeg	(inXferInfo.getSegmentLength() * inXferInfo.getElementLength());
	for (ULWord segNdx(0);  segNdx < inXferInfo.getSegmentCount();  segNdx++)
	{
		const void *	pSrc (inSrcBuffer.GetHostAddress(srcOffset));
		void *			pDst (GetHostAddress(dstOffset));
		if (!pSrc)	return false;
		if (!pDst)	return false;
		if (srcOffset + bytesPerSeg > inSrcBuffer.GetByteCount())
			return false;	//	memcpy will read past end of srcBuffer
		if (dstOffset + bytesPerSeg > GetByteCount())
			return false;	//	memcpy will write past end of me
		::memcpy (pDst,  pSrc,  bytesPerSeg);
		srcOffset += srcPitch;	//	Bump src offset
		dstOffset += dstPitch;	//	Bump dst offset
	}	//	for each segment
	return true;
}


bool NTV2_POINTER::SwapWith (NTV2_POINTER & inBuffer)
{
	if (inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (IsNULL ())
		return false;	//	I am NULL or empty
	if (inBuffer.GetByteCount () != GetByteCount ())
		return false;	//	Different sizes
	if (fFlags != inBuffer.fFlags)
		return false;	//	Flags mismatch
	if (inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer

	ULWord64	tmp			= fUserSpacePtr;
	fUserSpacePtr			= inBuffer.fUserSpacePtr;
	inBuffer.fUserSpacePtr	= tmp;
	return true;
}


bool NTV2_POINTER::IsContentEqual (const NTV2_POINTER & inBuffer, const ULWord inByteOffset, const ULWord inByteCount) const
{
	if (IsNULL() || inBuffer.IsNULL())
		return false;	//	NULL or empty
	if (inBuffer.GetByteCount() != GetByteCount())
		return false;	//	Different byte counts
	if (inBuffer.GetHostPointer() == GetHostPointer())
		return true;	//	Same buffer

	ULWord	totalBytes(GetByteCount());
	if (inByteOffset >= totalBytes)
		return false;	//	Bad offset

	totalBytes -= inByteOffset;

	ULWord	byteCount(inByteCount);
	if (byteCount > totalBytes)
		byteCount = totalBytes;

	const UByte *	pByte1 (reinterpret_cast<const UByte*>(GetHostPointer()));
	const UByte *	pByte2 (reinterpret_cast<const UByte*>(inBuffer.GetHostPointer()));
	pByte1 += inByteOffset;
	pByte2 += inByteOffset;
	return ::memcmp (pByte1, pByte2, byteCount) == 0;
}


bool NTV2_POINTER::GetRingChangedByteRange (const NTV2_POINTER & inBuffer, ULWord & outByteOffsetFirst, ULWord & outByteOffsetLast) const
{
	outByteOffsetFirst = outByteOffsetLast = GetByteCount ();
	if (IsNULL () || inBuffer.IsNULL ())
		return false;	//	NULL or empty
	if (inBuffer.GetByteCount () != GetByteCount ())
		return false;	//	Different byte counts
	if (inBuffer.GetHostPointer () == GetHostPointer ())
		return true;	//	Same buffer
	if (GetByteCount() < 3)
		return false;	//	Too small

	const UByte *	pByte1 (reinterpret_cast <const UByte *> (GetHostPointer()));
	const UByte *	pByte2 (reinterpret_cast <const UByte *> (inBuffer.GetHostPointer()));

	outByteOffsetFirst = 0;
	while (outByteOffsetFirst < GetByteCount())
	{
		if (*pByte1 != *pByte2)
			break;
		pByte1++;
		pByte2++;
		outByteOffsetFirst++;
	}
	if (outByteOffsetFirst == 0)
	{
		//	Wrap case -- look for first match...
		while (outByteOffsetFirst < GetByteCount())
		{
			if (*pByte1 == *pByte2)
				break;
			pByte1++;
			pByte2++;
			outByteOffsetFirst++;
		}
		if (outByteOffsetFirst < GetByteCount())
			outByteOffsetFirst--;
	}
	if (outByteOffsetFirst == GetByteCount())
		return true;	//	Identical --- outByteOffsetFirst == outByteOffsetLast == GetByteCount()

	//	Now scan from the end...
	pByte1 = reinterpret_cast <const UByte *> (GetHostPointer());
	pByte2 = reinterpret_cast <const UByte *> (inBuffer.GetHostPointer());
	pByte1 += GetByteCount () - 1;	//	Point to last byte
	pByte2 += GetByteCount () - 1;
	while (--outByteOffsetLast)
	{
		if (*pByte1 != *pByte2)
			break;
		pByte1--;
		pByte2--;
	}
	if (outByteOffsetLast == (GetByteCount() - 1))
	{
		//	Wrap case -- look for first match...
		while (outByteOffsetLast)
		{
			if (*pByte1 == *pByte2)
				break;
			pByte1--;
			pByte2--;
			outByteOffsetLast--;
		}
		if (outByteOffsetLast < GetByteCount())
			outByteOffsetLast++;
		if (outByteOffsetLast <= outByteOffsetFirst)
			cerr << "## WARNING:  GetRingChangedByteRange:  last " << outByteOffsetLast << " <= first " << outByteOffsetFirst << " in wrap condition" << endl;
		const ULWord	tmp	(outByteOffsetLast);
		outByteOffsetLast = outByteOffsetFirst;
		outByteOffsetFirst = tmp;
		if (outByteOffsetLast >= outByteOffsetFirst)
			cerr << "## WARNING:  GetRingChangedByteRange:  last " << outByteOffsetLast << " >= first " << outByteOffsetFirst << " in wrap condition" << endl;
	}
	return true;

}	//	GetRingChangedByteRange


static size_t	gDefaultPageSize	(AJA_PAGE_SIZE);

size_t NTV2_POINTER::DefaultPageSize (void)
{
	return gDefaultPageSize;
}

bool NTV2_POINTER::SetDefaultPageSize (const size_t inNewSize)
{
	const bool result (inNewSize  &&  (!(inNewSize & (inNewSize - 1))));
	if (result)
		gDefaultPageSize = inNewSize;
	return result;
}


FRAME_STAMP::FRAME_STAMP ()
	:	acHeader						(AUTOCIRCULATE_TYPE_FRAMESTAMP, sizeof (FRAME_STAMP)),
		acFrameTime						(0),
		acRequestedFrame				(0),
		acAudioClockTimeStamp			(0),
		acAudioExpectedAddress			(0),
		acAudioInStartAddress			(0),
		acAudioInStopAddress			(0),
		acAudioOutStopAddress			(0),
		acAudioOutStartAddress			(0),
		acTotalBytesTransferred			(0),
		acStartSample					(0),
		acTimeCodes						(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acCurrentTime					(0),
		acCurrentFrame					(0),
		acCurrentFrameTime				(0),
		acAudioClockCurrentTime			(0),
		acCurrentAudioExpectedAddress	(0),
		acCurrentAudioStartAddress		(0),
		acCurrentFieldCount				(0),
		acCurrentLineCount				(0),
		acCurrentReps					(0),
		acCurrentUserCookie				(0),
		acFrame							(0),
		acRP188							()
{
	NTV2_ASSERT_STRUCT_VALID;
}


FRAME_STAMP::FRAME_STAMP (const FRAME_STAMP & inObj)
	:	acHeader						(inObj.acHeader),
		acFrameTime						(inObj.acFrameTime),
		acRequestedFrame				(inObj.acRequestedFrame),
		acAudioClockTimeStamp			(inObj.acAudioClockTimeStamp),
		acAudioExpectedAddress			(inObj.acAudioExpectedAddress),
		acAudioInStartAddress			(inObj.acAudioInStartAddress),
		acAudioInStopAddress			(inObj.acAudioInStopAddress),
		acAudioOutStopAddress			(inObj.acAudioOutStopAddress),
		acAudioOutStartAddress			(inObj.acAudioOutStartAddress),
		acTotalBytesTransferred			(inObj.acTotalBytesTransferred),
		acStartSample					(inObj.acStartSample),
		acCurrentTime					(inObj.acCurrentTime),
		acCurrentFrame					(inObj.acCurrentFrame),
		acCurrentFrameTime				(inObj.acCurrentFrameTime),
		acAudioClockCurrentTime			(inObj.acAudioClockCurrentTime),
		acCurrentAudioExpectedAddress	(inObj.acCurrentAudioExpectedAddress),
		acCurrentAudioStartAddress		(inObj.acCurrentAudioStartAddress),
		acCurrentFieldCount				(inObj.acCurrentFieldCount),
		acCurrentLineCount				(inObj.acCurrentLineCount),
		acCurrentReps					(inObj.acCurrentReps),
		acCurrentUserCookie				(inObj.acCurrentUserCookie),
		acFrame							(inObj.acFrame),
		acRP188							(inObj.acRP188),
		acTrailer						(inObj.acTrailer)
{
	acTimeCodes.SetFrom (inObj.acTimeCodes);
}


FRAME_STAMP::~FRAME_STAMP ()
{
}


bool FRAME_STAMP::GetInputTimeCodes (NTV2TimeCodeList & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord				numRP188s	(acTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	const NTV2_RP188 *	pArray		(reinterpret_cast <const NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
	outValues.clear ();
	if (!pArray)
		return false;		//	No 'acTimeCodes' array!

	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number

	for (ULWord ndx (0);  ndx < numRP188s;  ndx++)
		outValues << pArray [ndx];

	return true;
}


bool FRAME_STAMP::GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex) const
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord				numRP188s	(acTimeCodes.GetByteCount () / sizeof (NTV2_RP188));
	const NTV2_RP188 *	pArray		(reinterpret_cast <const NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
	outTimeCode.Set ();	//	invalidate
	if (!pArray)
		return false;		//	No 'acTimeCodes' array!
	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number
	if (!NTV2_IS_VALID_TIMECODE_INDEX (inTCIndex))
		return false;

	outTimeCode = pArray [inTCIndex];
	return true;
}


bool FRAME_STAMP::GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput, const bool inValidOnly) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outTimeCodes.clear();

	if (!NTV2_IS_VALID_CHANNEL(inSDIInput))
		return false;	//	Bad SDI input

	NTV2TimeCodeList	allTCs;
	if (!GetInputTimeCodes(allTCs))
		return false;	//	GetInputTimeCodes failed

	const NTV2TCIndexes	tcIndexes (GetTCIndexesForSDIInput(inSDIInput));
	for (NTV2TCIndexesConstIter iter(tcIndexes.begin());  iter != tcIndexes.end();  ++iter)
	{
		const NTV2TCIndex tcIndex(*iter);
		NTV2_ASSERT(NTV2_IS_VALID_TIMECODE_INDEX(tcIndex));
		const NTV2_RP188 tc(allTCs.at(tcIndex));
		if (!inValidOnly)
			outTimeCodes[tcIndex] = tc;
		else if (tc.IsValid())
			outTimeCodes[tcIndex] = tc;
	}
	return true;
}


bool FRAME_STAMP::GetSDIInputStatus(NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	(void)outStatus;
	(void)inSDIInputIndex0;
	return true;
}

bool FRAME_STAMP::SetInputTimecode (const NTV2TCIndex inTCNdx, const NTV2_RP188 & inTimecode)
{
	ULWord			numRP188s	(acTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray		(reinterpret_cast<NTV2_RP188*>(acTimeCodes.GetHostPointer()));
	if (!pArray  ||  !numRP188s)
		return false;		//	No 'acTimeCodes' array!

	if (numRP188s > NTV2_MAX_NUM_TIMECODE_INDEXES)
		numRP188s = NTV2_MAX_NUM_TIMECODE_INDEXES;	//	clamp to this max number
	if (ULWord(inTCNdx) >= numRP188s)
		return false;	//	Past end

	pArray[inTCNdx] = inTimecode;	//	Write the new value
	return true;	//	Success!
}


FRAME_STAMP & FRAME_STAMP::operator = (const FRAME_STAMP & inRHS)
{
	if (this != &inRHS)
	{
		acTimeCodes						= inRHS.acTimeCodes;
		acHeader						= inRHS.acHeader;
		acFrameTime						= inRHS.acFrameTime;
		acRequestedFrame				= inRHS.acRequestedFrame;
		acAudioClockTimeStamp			= inRHS.acAudioClockTimeStamp;
		acAudioExpectedAddress			= inRHS.acAudioExpectedAddress;
		acAudioInStartAddress			= inRHS.acAudioInStartAddress;
		acAudioInStopAddress			= inRHS.acAudioInStopAddress;
		acAudioOutStopAddress			= inRHS.acAudioOutStopAddress;
		acAudioOutStartAddress			= inRHS.acAudioOutStartAddress;
		acTotalBytesTransferred			= inRHS.acTotalBytesTransferred;
		acStartSample					= inRHS.acStartSample;
		acCurrentTime					= inRHS.acCurrentTime;
		acCurrentFrame					= inRHS.acCurrentFrame;
		acCurrentFrameTime				= inRHS.acCurrentFrameTime;
		acAudioClockCurrentTime			= inRHS.acAudioClockCurrentTime;
		acCurrentAudioExpectedAddress	= inRHS.acCurrentAudioExpectedAddress;
		acCurrentAudioStartAddress		= inRHS.acCurrentAudioStartAddress;
		acCurrentFieldCount				= inRHS.acCurrentFieldCount;
		acCurrentLineCount				= inRHS.acCurrentLineCount;
		acCurrentReps					= inRHS.acCurrentReps;
		acCurrentUserCookie				= inRHS.acCurrentUserCookie;
		acFrame							= inRHS.acFrame;
		acRP188							= inRHS.acRP188;
		acTrailer						= inRHS.acTrailer;
	}
	return *this;
}


bool FRAME_STAMP::SetFrom (const FRAME_STAMP_STRUCT & inOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	//acCrosspoint					= inOldStruct.channelSpec;
	acFrameTime						= inOldStruct.frameTime;
	acRequestedFrame				= inOldStruct.frame;
	acAudioClockTimeStamp			= inOldStruct.audioClockTimeStamp;
	acAudioExpectedAddress			= inOldStruct.audioExpectedAddress;
	acAudioInStartAddress			= inOldStruct.audioInStartAddress;
	acAudioInStopAddress			= inOldStruct.audioInStopAddress;
	acAudioOutStopAddress			= inOldStruct.audioOutStopAddress;
	acAudioOutStartAddress			= inOldStruct.audioOutStartAddress;
	acTotalBytesTransferred			= inOldStruct.bytesRead;
	acStartSample					= inOldStruct.startSample;
	acCurrentTime					= inOldStruct.currentTime;
	acCurrentFrame					= inOldStruct.currentFrame;
	acCurrentFrameTime				= inOldStruct.currentFrameTime;
	acAudioClockCurrentTime			= inOldStruct.audioClockCurrentTime;
	acCurrentAudioExpectedAddress	= inOldStruct.currentAudioExpectedAddress;
	acCurrentAudioStartAddress		= inOldStruct.currentAudioStartAddress;
	acCurrentFieldCount				= inOldStruct.currentFieldCount;
	acCurrentLineCount				= inOldStruct.currentLineCount;
	acCurrentReps					= inOldStruct.currentReps;
	acCurrentUserCookie				= inOldStruct.currenthUser;
	acRP188							= NTV2_RP188 (inOldStruct.currentRP188);
	if (!acTimeCodes.IsNULL() && acTimeCodes.GetByteCount () >= sizeof (NTV2_RP188))
	{
		NTV2_RP188 *	pTimecodes	(reinterpret_cast <NTV2_RP188 *> (acTimeCodes.GetHostPointer ()));
		NTV2_ASSERT (pTimecodes);
		NTV2_ASSERT (NTV2_TCINDEX_DEFAULT == 0);
		pTimecodes [NTV2_TCINDEX_DEFAULT] = acRP188;
	}
	return true;
}


bool FRAME_STAMP::CopyTo (FRAME_STAMP_STRUCT & outOldStruct) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outOldStruct.frameTime						= acFrameTime;
	outOldStruct.frame							= acRequestedFrame;
	outOldStruct.audioClockTimeStamp			= acAudioClockTimeStamp;
	outOldStruct.audioExpectedAddress			= acAudioExpectedAddress;
	outOldStruct.audioInStartAddress			= acAudioInStartAddress;
	outOldStruct.audioInStopAddress				= acAudioInStopAddress;
	outOldStruct.audioOutStopAddress			= acAudioOutStopAddress;
	outOldStruct.audioOutStartAddress			= acAudioOutStartAddress;
	outOldStruct.bytesRead						= acTotalBytesTransferred;
	outOldStruct.startSample					= acStartSample;
	outOldStruct.currentTime					= acCurrentTime;
	outOldStruct.currentFrame					= acCurrentFrame;
	outOldStruct.currentFrameTime				= acCurrentFrameTime;
	outOldStruct.audioClockCurrentTime			= acAudioClockCurrentTime;
	outOldStruct.currentAudioExpectedAddress	= acCurrentAudioExpectedAddress;
	outOldStruct.currentAudioStartAddress		= acCurrentAudioStartAddress;
	outOldStruct.currentFieldCount				= acCurrentFieldCount;
	outOldStruct.currentLineCount				= acCurrentLineCount;
	outOldStruct.currentReps					= acCurrentReps;
    outOldStruct.currenthUser					= (ULWord)acCurrentUserCookie;
	outOldStruct.currentRP188					= acRP188;
	//	Ticket 3367 -- Mark Gilbert of Gallery UK reports that after updating from AJA Retail Software 10.5 to 14.0,
	//	their QuickTime app stopped receiving timecode during capture. Turns out the QuickTime components use the new
	//	AutoCirculate APIs, but internally still use the old FRAME_STAMP_STRUCT for frame info, including timecode...
	//	...and only use the "currentRP188" field for the "retail" timecode.
	//	Sadly, this FRAME_STAMP-to-FRAME_STAMP_STRUCT function historically only set "currentRP188" from the deprecated
	//	(and completely unused) "acRP188" field, when it really should've been using the acTimeCodes[NTV2_TCINDEX_DEFAULT]
	//	value all along...
	if (!acTimeCodes.IsNULL())									//	If there's an acTimeCodes buffer...
		if (acTimeCodes.GetByteCount() >= sizeof(NTV2_RP188))	//	...and it has at least one timecode value...
		{
			const NTV2_RP188 *	pDefaultTC	(reinterpret_cast<const NTV2_RP188*>(acTimeCodes.GetHostPointer()));
			if (pDefaultTC)
				outOldStruct.currentRP188		= pDefaultTC[NTV2_TCINDEX_DEFAULT];	//	Stuff the "default" (retail) timecode into "currentRP188".
		}
	return true;
}


string FRAME_STAMP::operator [] (const unsigned inIndexNum) const
{
	ostringstream	oss;
	NTV2_RP188		rp188;
	if (GetInputTimeCode (rp188, NTV2TimecodeIndex (inIndexNum)))
	{
		if (rp188.IsValid())
		{
			CRP188	foo (rp188);
			oss << foo;
		}
		else
			oss << "---";
	}
	else if (NTV2_IS_VALID_TIMECODE_INDEX (inIndexNum))
		oss << "---";
	return oss.str();
}


NTV2SDIInStatistics::NTV2SDIInStatistics()
	:	mHeader(AUTOCIRCULATE_TYPE_SDISTATS, sizeof(NTV2SDIInStatistics)),
		mInStatistics(NTV2_MAX_NUM_CHANNELS * sizeof(NTV2SDIInputStatus))
{
	Clear();
	NTV2_ASSERT_STRUCT_VALID;
}

void NTV2SDIInStatistics::Clear(void)
{
	NTV2_ASSERT_STRUCT_VALID;
	if (mInStatistics.IsNULL())
		return;
	NTV2SDIInputStatus * pArray(reinterpret_cast <NTV2SDIInputStatus *> (mInStatistics.GetHostPointer()));
	for (int i = 0; i < NTV2_MAX_NUM_CHANNELS; i++)
		pArray[i].Clear();
}

bool NTV2SDIInStatistics::GetSDIInputStatus(NTV2SDIInputStatus & outStatus, const UWord inSDIInputIndex0)
{
	NTV2_ASSERT_STRUCT_VALID;
	const ULWord		numElements(mInStatistics.GetByteCount() / sizeof(NTV2SDIInputStatus));
	const NTV2SDIInputStatus *	pArray(reinterpret_cast <const NTV2SDIInputStatus *> (mInStatistics.GetHostPointer()));
	outStatus.Clear();
	if (!pArray)
		return false;
	if (numElements != 8)
		return false;
	if (inSDIInputIndex0 >= numElements)
		return false;
	outStatus = pArray[inSDIInputIndex0];
	return true;
}

NTV2SDIInputStatus &	NTV2SDIInStatistics::operator [] (const size_t inSDIInputIndex0)
{
	NTV2_ASSERT_STRUCT_VALID;
	static NTV2SDIInputStatus dummy;
	const ULWord	numElements(mInStatistics.GetByteCount() / sizeof(NTV2SDIInputStatus));
	NTV2SDIInputStatus * pArray(reinterpret_cast<NTV2SDIInputStatus*>(mInStatistics.GetHostPointer()));
	if (!pArray)
		return dummy;
	if (numElements != 8)
		return dummy;
	if (inSDIInputIndex0 >= numElements)
		return dummy;
	return pArray[inSDIInputIndex0];
}

std::ostream &	NTV2SDIInStatistics::Print(std::ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream << mHeader << ", " << mInStatistics << ", " << mTrailer; return inOutStream;
}


AUTOCIRCULATE_TRANSFER_STATUS::AUTOCIRCULATE_TRANSFER_STATUS ()
	:	acHeader					(AUTOCIRCULATE_TYPE_XFERSTATUS, sizeof (AUTOCIRCULATE_TRANSFER_STATUS)),
		acState						(NTV2_AUTOCIRCULATE_DISABLED),
		acTransferFrame				(0),
		acBufferLevel				(0),
		acFramesProcessed			(0),
		acFramesDropped				(0),
		acFrameStamp				(),
		acAudioTransferSize			(0),
		acAudioStartSample			(0),
		acAncTransferSize			(0),
		acAncField2TransferSize		(0)
		//acTrailer					()
{
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_STATUS::AUTOCIRCULATE_STATUS (const NTV2Crosspoint inCrosspoint)
	:	acHeader				(AUTOCIRCULATE_TYPE_STATUS, sizeof (AUTOCIRCULATE_STATUS)),
		acCrosspoint			(inCrosspoint),
		acState					(NTV2_AUTOCIRCULATE_DISABLED),
		acStartFrame			(0),
		acEndFrame				(0),
		acActiveFrame			(0),
		acRDTSCStartTime		(0),
		acAudioClockStartTime	(0),
		acRDTSCCurrentTime		(0),
		acAudioClockCurrentTime	(0),
		acFramesProcessed		(0),
		acFramesDropped			(0),
		acBufferLevel			(0),
		acOptionFlags			(0),
		acAudioSystem			(NTV2_AUDIOSYSTEM_INVALID)
{
	NTV2_ASSERT_STRUCT_VALID;
}


bool AUTOCIRCULATE_STATUS::CopyTo (AUTOCIRCULATE_STATUS_STRUCT & outOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	outOldStruct.channelSpec			= acCrosspoint;
	outOldStruct.state					= acState;
	outOldStruct.startFrame				= acStartFrame;
	outOldStruct.endFrame				= acEndFrame;
	outOldStruct.activeFrame			= acActiveFrame;
	outOldStruct.rdtscStartTime			= acRDTSCStartTime;
	outOldStruct.audioClockStartTime	= acAudioClockStartTime;
	outOldStruct.rdtscCurrentTime		= acRDTSCCurrentTime;
	outOldStruct.audioClockCurrentTime	= acAudioClockCurrentTime;
	outOldStruct.framesProcessed		= acFramesProcessed;
	outOldStruct.framesDropped			= acFramesDropped;
	outOldStruct.bufferLevel			= acBufferLevel;
	outOldStruct.bWithAudio				= NTV2_IS_VALID_AUDIO_SYSTEM (acAudioSystem);
	outOldStruct.bWithRP188				= acOptionFlags & AUTOCIRCULATE_WITH_RP188 ? 1 : 0;
	outOldStruct.bFbfChange				= acOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE ? 1 : 0;
	outOldStruct.bFboChange				= acOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE ? 1 : 0;
	outOldStruct.bWithColorCorrection	= acOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT ? 1 : 0;
	outOldStruct.bWithVidProc			= acOptionFlags & AUTOCIRCULATE_WITH_VIDPROC ? 1 : 0;
	outOldStruct.bWithCustomAncData		= acOptionFlags & AUTOCIRCULATE_WITH_ANC ? 1 : 0;
	return true;
}


bool AUTOCIRCULATE_STATUS::CopyFrom (const AUTOCIRCULATE_STATUS_STRUCT & inOldStruct)
{
	NTV2_ASSERT_STRUCT_VALID;
	acCrosspoint			= inOldStruct.channelSpec;
	acState					= inOldStruct.state;
	acStartFrame			= inOldStruct.startFrame;
	acEndFrame				= inOldStruct.endFrame;
	acActiveFrame			= inOldStruct.activeFrame;
	acRDTSCStartTime		= inOldStruct.rdtscStartTime;
	acAudioClockStartTime	= inOldStruct.audioClockStartTime;
	acRDTSCCurrentTime		= inOldStruct.rdtscCurrentTime;
	acAudioClockCurrentTime	= inOldStruct.audioClockCurrentTime;
	acFramesProcessed		= inOldStruct.framesProcessed;
	acFramesDropped			= inOldStruct.framesDropped;
	acBufferLevel			= inOldStruct.bufferLevel;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID;	//	NTV2_AUDIOSYSTEM_1;
	acOptionFlags			=	inOldStruct.bWithRP188				? AUTOCIRCULATE_WITH_RP188			: 0		|
								inOldStruct.bFbfChange				? AUTOCIRCULATE_WITH_FBFCHANGE		: 0		|
								inOldStruct.bFboChange				? AUTOCIRCULATE_WITH_FBOCHANGE		: 0		|
								inOldStruct.bWithColorCorrection	? AUTOCIRCULATE_WITH_COLORCORRECT	: 0		|
								inOldStruct.bWithVidProc			? AUTOCIRCULATE_WITH_VIDPROC		: 0		|
								inOldStruct.bWithCustomAncData		? AUTOCIRCULATE_WITH_ANC			: 0;
	return true;
}


void AUTOCIRCULATE_STATUS::Clear (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	acCrosspoint			= NTV2CROSSPOINT_INVALID;
	acState					= NTV2_AUTOCIRCULATE_DISABLED;
	acStartFrame			= 0;
	acEndFrame				= 0;
	acActiveFrame			= 0;
	acRDTSCStartTime		= 0;
	acAudioClockStartTime	= 0;
	acRDTSCCurrentTime		= 0;
	acAudioClockCurrentTime	= 0;
	acFramesProcessed		= 0;
	acFramesDropped			= 0;
	acBufferLevel			= 0;
	acOptionFlags			= 0;
	acAudioSystem			= NTV2_AUDIOSYSTEM_INVALID;
}


NTV2Channel AUTOCIRCULATE_STATUS::GetChannel (void) const
{
	return ::NTV2CrosspointToNTV2Channel(acCrosspoint);
}


struct ThousandsSeparator : std::numpunct <char>
{
	virtual inline	char			do_thousands_sep() const	{return ',';}
	virtual inline	std::string		do_grouping() const			{return "\03";}
};


template <class T> string CommaStr (const T & inNum)
{
	ostringstream	oss;
	const locale	loc	(oss.getloc(), new ThousandsSeparator);
	oss.imbue (loc);
	oss << inNum;
	return oss.str();
}	//	CommaStr


string AUTOCIRCULATE_STATUS::operator [] (const unsigned inIndexNum) const
{
	ostringstream	oss;
	if (inIndexNum == 0)
		oss << ::NTV2AutoCirculateStateToString(acState);
	else if (!IsStopped())
		switch (inIndexNum)
		{
			case 1:		oss << DEC(GetStartFrame());					break;
			case 2:		oss << DEC(GetEndFrame());						break;
			case 3:		oss << DEC(GetFrameCount());					break;
			case 4:		oss << DEC(GetActiveFrame());					break;
			case 5:		oss << xHEX0N(acRDTSCStartTime,16);				break;
			case 6:		oss << xHEX0N(acAudioClockStartTime,16);		break;
			case 7:		oss << DEC(acRDTSCCurrentTime);					break;
			case 8:		oss << DEC(acAudioClockCurrentTime);			break;
			case 9:		oss << CommaStr(GetProcessedFrameCount());		break;
			case 10:	oss << CommaStr(GetDroppedFrameCount());		break;
			case 11:	oss << DEC(GetBufferLevel());					break;
			case 12:	oss << ::NTV2AudioSystemToString(acAudioSystem, true);	break;
			case 13:	oss << (WithRP188()			? "Yes" : "No");	break;
			case 14:	oss << (WithLTC()			? "Yes" : "No");	break;
			case 15:	oss << (WithFBFChange()		? "Yes" : "No");	break;
			case 16:	oss << (WithFBOChange()		? "Yes" : "No");	break;
			case 17:	oss << (WithColorCorrect()	? "Yes" : "No");	break;
			case 18:	oss << (WithVidProc()		? "Yes" : "No");	break;
			case 19:	oss << (WithCustomAnc()		? "Yes" : "No");	break;
			case 20:	oss << (WithHDMIAuxData()	? "Yes" : "No");	break;
			case 21:	oss << (IsFieldMode()		? "Yes" : "No");	break;
			default:	break;
		}
	else if (inIndexNum < 22)
		oss << "---";
	return oss.str();
}


ostream & operator << (ostream & oss, const AUTOCIRCULATE_STATUS & inObj)
{
	if (!inObj.IsStopped())
		oss << ::NTV2ChannelToString(inObj.GetChannel(), true) << ": "
			<< (inObj.IsInput() ? "Input " : "Output")
			<< setw(12) << ::NTV2AutoCirculateStateToString(inObj.acState) << "  "
			<< setw( 5) << inObj.GetStartFrame()
			<< setw( 6) << inObj.GetEndFrame()
			<< setw( 6) << inObj.GetActiveFrame()
			<< setw( 8) << inObj.GetProcessedFrameCount()
			<< setw( 8) << inObj.GetDroppedFrameCount()
			<< setw( 7) << inObj.GetBufferLevel()
			<< setw(10) << ::NTV2AudioSystemToString(inObj.acAudioSystem, true)
			<< setw(10) << (inObj.WithRP188()			? "+RP188"		: "-RP188")
			<< setw(10) << (inObj.WithLTC()				? "+LTC"		: "-LTC")
			<< setw(10) << (inObj.WithFBFChange()		? "+FBFchg"		: "-FBFchg")
			<< setw(10) << (inObj.WithFBOChange()		? "+FBOchg"		: "-FBOchg")
			<< setw(10) << (inObj.WithColorCorrect()	? "+ColCor"		: "-ColCor")
			<< setw(10) << (inObj.WithVidProc()			? "+VidProc"	: "-VidProc")
			<< setw(10) << (inObj.WithCustomAnc()		? "+AncData"	: "-AncData")
			<< setw(10) << (inObj.IsFieldMode()			? "+FldMode"	: "-FldMode")
			<< setw(10) << (inObj.WithHDMIAuxData()		? "+HDMIAux"	: "-HDMIAux");
	return oss;
}


NTV2SegmentedDMAInfo::NTV2SegmentedDMAInfo ()
{
	Reset ();
}


NTV2SegmentedDMAInfo::NTV2SegmentedDMAInfo (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	Set (inNumSegments, inNumActiveBytesPerRow, inHostBytesPerRow, inDeviceBytesPerRow);
}


void NTV2SegmentedDMAInfo::Set (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow, const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	acNumSegments = inNumSegments;
	if (acNumSegments > 1)
	{
		acNumActiveBytesPerRow	= inNumActiveBytesPerRow;
		acSegmentHostPitch		= inHostBytesPerRow;
		acSegmentDevicePitch	= inDeviceBytesPerRow;
	}
	else
		Reset ();
}


void NTV2SegmentedDMAInfo::Reset (void)
{
	acNumSegments = acNumActiveBytesPerRow = acSegmentHostPitch = acSegmentDevicePitch = 0;
}


NTV2ColorCorrectionData::NTV2ColorCorrectionData ()
	:	ccMode				(NTV2_CCMODE_INVALID),
		ccSaturationValue	(0)
{
}


NTV2ColorCorrectionData::~NTV2ColorCorrectionData ()
{
	Clear ();
}


void NTV2ColorCorrectionData::Clear (void)
{
	ccMode = NTV2_CCMODE_INVALID;
	ccSaturationValue = 0;
	if (ccLookupTables.GetHostPointer ())
		delete [] (UByte *) ccLookupTables.GetHostPointer ();
	ccLookupTables.Set (AJA_NULL, 0);
}


bool NTV2ColorCorrectionData::Set (const NTV2ColorCorrectionMode inMode, const ULWord inSaturation, const void * pInTableData)
{
	Clear ();
	if (!NTV2_IS_VALID_COLOR_CORRECTION_MODE (inMode))
		return false;

	if (pInTableData)
	{
		if (!ccLookupTables.Set (new UByte [NTV2_COLORCORRECTOR_TABLESIZE], NTV2_COLORCORRECTOR_TABLESIZE))
			return false;
	}
	ccMode = inMode;
	ccSaturationValue = (inMode == NTV2_CCMODE_3WAY) ? inSaturation : 0;
	return true;
}


AUTOCIRCULATE_TRANSFER::AUTOCIRCULATE_TRANSFER ()
	:	acHeader					(AUTOCIRCULATE_TYPE_XFER, sizeof (AUTOCIRCULATE_TRANSFER)),
		acOutputTimeCodes			(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acTransferStatus			(),
		acInUserCookie				(0),
		acInVideoDMAOffset			(0),
		acInSegmentedDMAInfo		(),
		acColorCorrection			(),
		acFrameBufferFormat			(NTV2_FBF_10BIT_YCBCR),
		acFrameBufferOrientation	(NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN),
		acVidProcInfo				(),
		acVideoQuarterSizeExpand	(NTV2_QuarterSizeExpandOff),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(-1),
		acRP188						(),
		acCrosspoint				(NTV2CROSSPOINT_INVALID)
{
	if (acOutputTimeCodes.GetHostPointer ())
		::memset (acOutputTimeCodes.GetHostPointer (), 0xFF, acOutputTimeCodes.GetByteCount ());
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_TRANSFER::AUTOCIRCULATE_TRANSFER (ULWord * pInVideoBuffer, const ULWord inVideoByteCount, ULWord * pInAudioBuffer,
												const ULWord inAudioByteCount, ULWord * pInANCBuffer, const ULWord inANCByteCount,
												ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
	:	acHeader					(AUTOCIRCULATE_TYPE_XFER, sizeof (AUTOCIRCULATE_TRANSFER)),
		acVideoBuffer				(pInVideoBuffer, inVideoByteCount),
		acAudioBuffer				(pInAudioBuffer, inAudioByteCount),
		acANCBuffer					(pInANCBuffer, inANCByteCount),
		acANCField2Buffer			(pInANCF2Buffer, inANCF2ByteCount),
		acOutputTimeCodes			(NTV2_MAX_NUM_TIMECODE_INDEXES * sizeof (NTV2_RP188)),
		acTransferStatus			(),
		acInUserCookie				(0),
		acInVideoDMAOffset			(0),
		acInSegmentedDMAInfo		(),
		acColorCorrection			(),
		acFrameBufferFormat			(NTV2_FBF_10BIT_YCBCR),
		acFrameBufferOrientation	(NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN),
		acVidProcInfo				(),
		acVideoQuarterSizeExpand	(NTV2_QuarterSizeExpandOff),
		acPeerToPeerFlags			(0),
		acFrameRepeatCount			(1),
		acDesiredFrame				(-1),
		acRP188						(),
		acCrosspoint				(NTV2CROSSPOINT_INVALID)
{
	if (acOutputTimeCodes.GetHostPointer ())
		::memset (acOutputTimeCodes.GetHostPointer (), 0xFF, acOutputTimeCodes.GetByteCount ());
	NTV2_ASSERT_STRUCT_VALID;
}


AUTOCIRCULATE_TRANSFER::~AUTOCIRCULATE_TRANSFER ()
{
}


bool AUTOCIRCULATE_TRANSFER::SetBuffers (ULWord * pInVideoBuffer, const ULWord inVideoByteCount,
										ULWord * pInAudioBuffer, const ULWord inAudioByteCount,
										ULWord * pInANCBuffer, const ULWord inANCByteCount,
										ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	return SetVideoBuffer (pInVideoBuffer, inVideoByteCount)
			&& SetAudioBuffer (pInAudioBuffer, inAudioByteCount)
				&& SetAncBuffers (pInANCBuffer, inANCByteCount, pInANCF2Buffer, inANCF2ByteCount);
}


bool AUTOCIRCULATE_TRANSFER::SetVideoBuffer (ULWord * pInVideoBuffer, const ULWord inVideoByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acVideoBuffer.Set (pInVideoBuffer, inVideoByteCount);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetAudioBuffer (ULWord * pInAudioBuffer, const ULWord inAudioByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acAudioBuffer.Set (pInAudioBuffer, inAudioByteCount);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetAncBuffers (ULWord * pInANCBuffer, const ULWord inANCByteCount, ULWord * pInANCF2Buffer, const ULWord inANCF2ByteCount)
{
	NTV2_ASSERT_STRUCT_VALID;
	acANCBuffer.Set (pInANCBuffer, inANCByteCount);
	acANCField2Buffer.Set (pInANCF2Buffer, inANCF2ByteCount);
	return true;
}

static const NTV2_RP188		INVALID_TIMECODE_VALUE;


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCodes (const NTV2TimeCodes & inValues)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues (acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray (reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (UWord ndx (0);  ndx < UWord(maxNumValues);  ndx++)
	{
		const NTV2TCIndex		tcIndex	(static_cast<const NTV2TCIndex>(ndx));
		NTV2TimeCodesConstIter	iter	(inValues.find(tcIndex));
		pArray[ndx] = (iter != inValues.end())  ?  iter->second  :  INVALID_TIMECODE_VALUE;
	}	//	for each possible NTV2TCSource value
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetOutputTimeCode (const NTV2_RP188 & inTimeCode, const NTV2TCIndex inTCIndex)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues (acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray (reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;
	if (!NTV2_IS_VALID_TIMECODE_INDEX(inTCIndex))
		return false;

	pArray[inTCIndex] = inTimeCode;
	return true;
}

bool AUTOCIRCULATE_TRANSFER::SetAllOutputTimeCodes (const NTV2_RP188 & inTimeCode, const bool inIncludeF2)
{
	NTV2_ASSERT_STRUCT_VALID;
	ULWord			maxNumValues	(acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *	pArray			(reinterpret_cast<NTV2_RP188*>(acOutputTimeCodes.GetHostPointer()));
	if (!pArray)
		return false;
	if (maxNumValues > NTV2_MAX_NUM_TIMECODE_INDEXES)
		maxNumValues = NTV2_MAX_NUM_TIMECODE_INDEXES;

	for (ULWord tcIndex(0);  tcIndex < maxNumValues;  tcIndex++)
		if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcIndex))
			pArray[tcIndex] = inIncludeF2 ? inTimeCode : INVALID_TIMECODE_VALUE;
		else
			pArray[tcIndex] = inTimeCode;
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SetFrameBufferFormat (const NTV2FrameBufferFormat inNewFormat)
{
	NTV2_ASSERT_STRUCT_VALID;
	if (!NTV2_IS_VALID_FRAME_BUFFER_FORMAT (inNewFormat))
		return false;
	acFrameBufferFormat = inNewFormat;
	return true;
}


void AUTOCIRCULATE_TRANSFER::Clear (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffers (AJA_NULL, 0, AJA_NULL, 0, AJA_NULL, 0, AJA_NULL, 0);
}


bool AUTOCIRCULATE_TRANSFER::EnableSegmentedDMAs (const ULWord inNumSegments, const ULWord inNumActiveBytesPerRow,
													const ULWord inHostBytesPerRow, const ULWord inDeviceBytesPerRow)
{
	NTV2_ASSERT_STRUCT_VALID;
	//	Cannot allow segmented DMAs if video buffer was self-allocated by the SDK, since the video buffer size holds the segment size (in bytes)...
	if (acVideoBuffer.IsAllocatedBySDK ())
		return false;	//	Disallow
	acInSegmentedDMAInfo.Set (inNumSegments, inNumActiveBytesPerRow, inHostBytesPerRow, inDeviceBytesPerRow);
	return true;
}


bool AUTOCIRCULATE_TRANSFER::DisableSegmentedDMAs (void)
{
	NTV2_ASSERT_STRUCT_VALID;
	acInSegmentedDMAInfo.Reset ();
	return true;
}


bool AUTOCIRCULATE_TRANSFER::SegmentedDMAsEnabled (void) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acInSegmentedDMAInfo.acNumSegments > 1;
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCodes (NTV2TimeCodeList & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCodes (outValues);
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCode (NTV2_RP188 & outTimeCode, const NTV2TCIndex inTCIndex) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCode (outTimeCode, inTCIndex);
}


bool AUTOCIRCULATE_TRANSFER::GetInputTimeCodes (NTV2TimeCodes & outTimeCodes, const NTV2Channel inSDIInput, const bool inValidOnly) const
{
	NTV2_ASSERT_STRUCT_VALID;
	return acTransferStatus.acFrameStamp.GetInputTimeCodes (outTimeCodes, inSDIInput, inValidOnly);
}


NTV2DebugLogging::NTV2DebugLogging(const bool inEnable)
	:	mHeader				(NTV2_TYPE_AJADEBUGLOGGING, sizeof (NTV2DebugLogging)),
		mSharedMemory		(inEnable ? AJADebug::GetPrivateDataLoc() : AJA_NULL,  inEnable ? AJADebug::GetPrivateDataLen() : 0)
{
}


ostream & NTV2DebugLogging::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << " shMem=" << mSharedMemory << " " << mTrailer;
	return inOutStream;
}



NTV2BufferLock::NTV2BufferLock()
	:	mHeader	(NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
    SetFlags(0);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock (const NTV2_POINTER & inBuffer, const ULWord inFlags)
	:	mHeader	(NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer(inBuffer);
	SetFlags(inFlags);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock(const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags)
	:	mHeader	(NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2_POINTER(pInBuffer, inByteCount));
	SetFlags (inFlags);
	SetMaxLockSize(0);
}

NTV2BufferLock::NTV2BufferLock(const ULWord64 inMaxLockSize, const ULWord inFlags)
	:	mHeader	(NTV2_TYPE_AJABUFFERLOCK, sizeof(NTV2BufferLock))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2_POINTER());
	SetFlags (inFlags);
	SetMaxLockSize(inMaxLockSize);
}

bool NTV2BufferLock::SetBuffer (const NTV2_POINTER & inBuffer)
{	//	Just use address & length (don't deep copy)...
	NTV2_ASSERT_STRUCT_VALID;
	return mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
}

ostream & NTV2BufferLock::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << mBuffer << " flags=" << xHEX0N(mFlags,8) << " " << mTrailer;
	return inOutStream;
}


NTV2Bitstream::NTV2Bitstream()
	:	mHeader	(NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
}

NTV2Bitstream::NTV2Bitstream (const NTV2_POINTER & inBuffer, const ULWord inFlags)
	:	mHeader	(NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer(inBuffer);
	SetFlags(inFlags);
}

NTV2Bitstream::NTV2Bitstream(const ULWord * pInBuffer, const ULWord inByteCount, const ULWord inFlags)
	:	mHeader	(NTV2_TYPE_AJABITSTREAM, sizeof(NTV2Bitstream))
{
	NTV2_ASSERT_STRUCT_VALID;
	SetBuffer (NTV2_POINTER(pInBuffer, inByteCount));
	SetFlags (inFlags);
}

bool NTV2Bitstream::SetBuffer (const NTV2_POINTER & inBuffer)
{	//	Just use address & length (don't deep copy)...
	NTV2_ASSERT_STRUCT_VALID;
	return mBuffer.Set (inBuffer.GetHostPointer(), inBuffer.GetByteCount());
}

ostream & NTV2Bitstream::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << mBuffer << " flags=" << xHEX0N(mFlags,8) << " " << mTrailer;
	return inOutStream;
}


NTV2GetRegisters::NTV2GetRegisters (const NTV2RegNumSet & inRegisterNumbers)
	:	mHeader				(AUTOCIRCULATE_TYPE_GETREGS, sizeof (NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegisterNumbers.size ())),
		mOutNumRegisters	(0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegisterNumbers);
}


NTV2GetRegisters::NTV2GetRegisters (NTV2RegisterReads & inRegReads)
	:	mHeader				(AUTOCIRCULATE_TYPE_GETREGS, sizeof (NTV2GetRegisters)),
		mInNumRegisters		(ULWord (inRegReads.size ())),
		mOutNumRegisters	(0)
{
	NTV2_ASSERT_STRUCT_VALID;
	ResetUsing (inRegReads);
}


bool NTV2GetRegisters::ResetUsing (const NTV2RegisterReads & inRegReads)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord (inRegReads.size ());
	mOutNumRegisters = 0;
	const bool	result	(	mInRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutGoodRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutValues.Allocate (mInNumRegisters * sizeof (ULWord)));
	if (result)
	{
		ULWord		ndx			(0);
		ULWord *	pRegArray	(reinterpret_cast <ULWord *> (mInRegisters.GetHostPointer ()));
		assert (pRegArray);
		for (NTV2RegisterReadsConstIter iter (inRegReads.begin ());  iter != inRegReads.end ();  ++iter)
			pRegArray [ndx++] = iter->registerNumber;
		assert ((ndx * sizeof (ULWord)) == mInRegisters.GetByteCount ());
	}
	return result;
}


bool NTV2GetRegisters::ResetUsing (const NTV2RegNumSet & inRegisterNumbers)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters = ULWord (inRegisterNumbers.size ());
	mOutNumRegisters = 0;
	const bool	result	(	mInRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutGoodRegisters.Allocate (mInNumRegisters * sizeof (ULWord))
						&&	mOutValues.Allocate (mInNumRegisters * sizeof (ULWord)));
	if (result)
	{
		ULWord		ndx			(0);
		ULWord *	pRegArray	(reinterpret_cast <ULWord *> (mInRegisters.GetHostPointer ()));
		assert (pRegArray);
		for (NTV2RegNumSetConstIter iter (inRegisterNumbers.begin ());  iter != inRegisterNumbers.end ();  ++iter)
			pRegArray [ndx++] = *iter;
		assert ((ndx * sizeof (ULWord)) == mInRegisters.GetByteCount ());
	}
	return result;
}


bool NTV2GetRegisters::GetGoodRegisters (NTV2RegNumSet & outGoodRegNums) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outGoodRegNums.clear ();
	if (mOutGoodRegisters.GetHostPointer () == 0)
		return false;		//	No 'mOutGoodRegisters' array!
	if (mOutGoodRegisters.GetByteCount () == 0)
		return false;		//	No good registers!
	if (mOutNumRegisters == 0)
		return false;		//	No good registers!  (The driver sets this field.)
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	mOutNumRegisters must be less than or equal to mInNumRegisters!

	const ULWord *	pRegArray	(reinterpret_cast <const ULWord *> (mOutGoodRegisters.GetHostPointer ()));
	for (ULWord ndx (0);  ndx < mOutGoodRegisters.GetByteCount ();  ndx++)
		outGoodRegNums << pRegArray [ndx];

	return true;
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterValueMap & outValues) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outValues.clear ();
	if (mOutGoodRegisters.GetHostPointer () == 0)
		return false;		//	No 'mOutGoodRegisters' array!
	if (mOutGoodRegisters.GetByteCount () == 0)
		return false;		//	No good registers!
	if (mOutNumRegisters == 0)
		return false;		//	No good registers!  (The driver sets this field.)
	if (mOutNumRegisters > mInNumRegisters)
		return false;		//	mOutNumRegisters must be less than or equal to mInNumRegisters!
	if (mOutValues.GetHostPointer () == 0)
		return false;		//	No 'mOutValues' array!
	if (mOutValues.GetByteCount () == 0)
		return false;		//	No values!
	if (mOutGoodRegisters.GetByteCount () != mOutValues.GetByteCount ())
		return false;		//	These sizes should match

	const ULWord *	pRegArray	(reinterpret_cast <const ULWord *> (mOutGoodRegisters.GetHostPointer ()));
	const ULWord *	pValArray	(reinterpret_cast <const ULWord *> (mOutValues.GetHostPointer ()));
	for (ULWord ndx (0);  ndx < mOutNumRegisters;  ndx++)
		outValues [pRegArray [ndx]] = pValArray [ndx];

	return true;
}


bool NTV2GetRegisters::GetRegisterValues (NTV2RegisterReads & outValues) const
{
	NTV2RegisterValueMap	regValues;
	uint32_t				missingTally (0);
	if (!GetRegisterValues (regValues))
		return false;
	for (NTV2RegisterReadsIter it (outValues.begin());  it != outValues.end();  ++it)
	{
		NTV2RegValueMapConstIter	mapIter	(regValues.find (it->registerNumber));
		if (mapIter == regValues.end())
			missingTally++;	//	Missing register
		it->registerValue = mapIter->second;
	}
	return missingTally == 0;
}


ostream & NTV2GetRegisters::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << ", numRegs=" << mInNumRegisters << ", inRegs=" << mInRegisters << ", outNumGoodRegs=" << mOutNumRegisters
				<< ", outGoodRegs=" << mOutGoodRegisters << ", outValues=" << mOutValues << ", " << mTrailer;
	return inOutStream;
}


NTV2SetRegisters::NTV2SetRegisters (const NTV2RegisterWrites & inRegWrites)
	:	mHeader				(AUTOCIRCULATE_TYPE_SETREGS, sizeof (NTV2SetRegisters)),
		mInNumRegisters		(ULWord (inRegWrites.size ())),
		mOutNumFailures		(0)
{
	ResetUsing (inRegWrites);
}


bool NTV2SetRegisters::ResetUsing (const NTV2RegisterWrites & inRegWrites)
{
	NTV2_ASSERT_STRUCT_VALID;
	mInNumRegisters	= ULWord (inRegWrites.size ());
	mOutNumFailures	= 0;
	const bool	result	(mInRegInfos.Allocate (mInNumRegisters * sizeof (NTV2RegInfo)) && mOutBadRegIndexes.Allocate (mInNumRegisters * sizeof (UWord)));
	if (result)
	{
		ULWord			ndx				(0);
		NTV2RegInfo *	pRegInfoArray	(reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
		UWord *			pBadRegIndexes	(reinterpret_cast <UWord *> (mOutBadRegIndexes.GetHostPointer ()));

		for (NTV2RegisterWritesConstIter iter (inRegWrites.begin ());  iter != inRegWrites.end ();  ++iter)
		{
			if (pBadRegIndexes)
				pBadRegIndexes [ndx] = 0;
			if (pRegInfoArray)
				pRegInfoArray [ndx++] = *iter;
		}
		assert ((ndx * sizeof (NTV2RegInfo)) == mInRegInfos.GetByteCount ());
		assert ((ndx * sizeof (UWord)) == mOutBadRegIndexes.GetByteCount ());
	}
	return result;
}


bool NTV2SetRegisters::GetFailedRegisterWrites (NTV2RegisterWrites & outFailedRegWrites) const
{
	NTV2_ASSERT_STRUCT_VALID;
	outFailedRegWrites.clear ();
	return true;
}


ostream & NTV2SetRegisters::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	inOutStream	<< mHeader << ", numRegs=" << mInNumRegisters << ", inRegInfos=" << mInRegInfos << ", outNumFailures=" << mOutNumFailures
				<< ", outBadRegIndexes=" << mOutBadRegIndexes << ", " << mTrailer;
	const UWord *		pBadRegIndexes		(reinterpret_cast <UWord *> (mOutBadRegIndexes.GetHostPointer ()));
	const UWord			numBadRegIndexes	(UWord (mOutBadRegIndexes.GetByteCount () / sizeof (UWord)));
	const NTV2RegInfo *	pRegInfoArray		(reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
	const UWord			numRegInfos			(UWord (mInRegInfos.GetByteCount () / sizeof (NTV2RegInfo)));
	if (pBadRegIndexes && numBadRegIndexes && pRegInfoArray && numRegInfos)
	{
		inOutStream << endl;
		for (UWord num (0);  num < numBadRegIndexes;  num++)
		{
			const UWord	badRegIndex	(pBadRegIndexes [num]);
			if (badRegIndex < numRegInfos)
			{
				const NTV2RegInfo &	badRegInfo	(pRegInfoArray [badRegIndex]);
				inOutStream << "Bad " << num << ":  " << badRegInfo << endl;
			}
		}
	}
	return inOutStream;
}


bool NTV2RegInfo::operator < (const NTV2RegInfo & inRHS) const
{
	typedef std::pair <ULWord, ULWord>			ULWordPair;
	typedef std::pair <ULWordPair, ULWordPair>	ULWordPairs;
	const ULWordPairs	rhs	(ULWordPair (inRHS.registerNumber, inRHS.registerValue), ULWordPair (inRHS.registerMask, inRHS.registerShift));
	const ULWordPairs	mine(ULWordPair (registerNumber, registerValue), ULWordPair (registerMask, registerShift));
	return mine < rhs;
}

ostream & NTV2RegInfo::Print (ostream & oss, const bool inAsCode) const
{
	if (inAsCode)
		return PrintCode(oss);
	const string regName (::NTV2RegisterNumberToString(NTV2RegisterNumber(registerNumber)));
	oss << "[" << regName << "|" << DEC(registerNumber) << ": val=" << xHEX0N(registerValue,8);
	if (registerMask != 0xFFFFFFFF)
		oss << " msk=" << xHEX0N(registerMask,8);
	if (registerShift)
		oss << " shf=" << DEC(registerShift);
	return oss << "]";
}

ostream & NTV2RegInfo::PrintCode (ostream & oss, const int inRadix) const
{
	const string regName (::NTV2RegisterNumberToString(NTV2RegisterNumber(registerNumber)));
	const bool badName (regName.find(' ') != string::npos);
	oss << "theDevice.WriteRegister (";
	if (badName)
		oss << DEC(registerNumber);
	else
		oss << regName;
	switch (inRadix)
	{
		case 2:		oss << ", " << BIN032(registerValue);	break;
		case 8:		oss << ", " << OCT(registerValue);		break;
		case 10:	oss << ", " << DEC(registerValue);		break;
		default:	oss << ", " << xHEX0N(registerValue,8);	break;
	}
	if (registerMask != 0xFFFFFFFF)
		switch (inRadix)
		{
			case 2:		oss << ", " << BIN032(registerMask);	break;
			case 8:		oss << ", " << OCT(registerMask);		break;
			case 10:	oss << ", " << DEC(registerMask);		break;
			default:	oss << ", " << xHEX0N(registerMask,8);	break;
		}
	if (registerShift)
		oss << ", " << DEC(registerShift);
	oss << ");  // ";
	if (badName)
		oss << regName;
	else
		oss << "Reg " << DEC(registerNumber);
	//	Decode the reg value...
	string info(CNTV2RegisterExpert::GetDisplayValue(registerNumber, registerValue));
	if (!info.empty())	//	and add to end of comment
		oss << "  // " << aja::replace(info, "\n", ", ");
	return oss;
}


ostream & NTV2PrintULWordVector (const NTV2ULWordVector & inObj, ostream & inOutStream)
{
	for (NTV2ULWordVector::const_iterator it(inObj.begin());  it != inObj.end();  ++it)
		inOutStream << " " << HEX0N(*it,8);
	return inOutStream;
}


ostream & NTV2PrintChannelList (const NTV2ChannelList & inObj, const bool inCompact, ostream & inOutStream)
{
	inOutStream << (inCompact ? "Ch[" : "[");
	for (NTV2ChannelListConstIter it(inObj.begin());  it != inObj.end();  )
	{
		if (inCompact)
			inOutStream << DEC(*it+1);
		else
			inOutStream << ::NTV2ChannelToString(*it);
		if (++it != inObj.end())
			inOutStream << (inCompact ? "|" : ",");
	}
	return inOutStream << "]";
}

string NTV2ChannelListToStr (const NTV2ChannelList & inObj, const bool inCompact)
{	ostringstream oss;
	::NTV2PrintChannelList (inObj, inCompact, oss);
	return oss.str();
}

ostream & NTV2PrintChannelSet (const NTV2ChannelSet & inObj, const bool inCompact, ostream & inOutStream)
{
	inOutStream << (inCompact ? "Ch{" : "{");
	for (NTV2ChannelSetConstIter it(inObj.begin());  it != inObj.end();  )
	{
		if (inCompact)
			inOutStream << DEC(*it+1);
		else
			inOutStream << ::NTV2ChannelToString(*it);
		if (++it != inObj.end())
			inOutStream << (inCompact ? "|" : ",");
	}
	return inOutStream << "}";
}

string NTV2ChannelSetToStr (const NTV2ChannelSet & inObj, const bool inCompact)
{	ostringstream oss;
	::NTV2PrintChannelSet (inObj, inCompact, oss);
	return oss.str();
}

NTV2ChannelSet NTV2MakeChannelSet (const NTV2Channel inFirstChannel, const UWord inNumChannels)
{
	NTV2ChannelSet result;
	for (NTV2Channel ch(inFirstChannel);  ch < NTV2Channel(inFirstChannel+inNumChannels);  ch = NTV2Channel(ch+1))
		if (NTV2_IS_VALID_CHANNEL(ch))
			result.insert(ch);
	return result;
}

NTV2ChannelSet NTV2MakeChannelSet (const NTV2ChannelList inChannels)
{
	NTV2ChannelSet result;
	for (NTV2ChannelListConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		result.insert(*it);
	return result;
}

NTV2ChannelList NTV2MakeChannelList (const NTV2Channel inFirstChannel, const UWord inNumChannels)
{
	NTV2ChannelList result;
	for (NTV2Channel ch(inFirstChannel);  ch < NTV2Channel(inFirstChannel+inNumChannels);  ch = NTV2Channel(ch+1))
		if (NTV2_IS_VALID_CHANNEL(ch))
			result.push_back(ch);
	return result;
}

NTV2ChannelList NTV2MakeChannelList (const NTV2ChannelSet inChannels)
{
	NTV2ChannelList result;
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		result.push_back(*it);
	return result;
}

NTV2RegisterReadsConstIter FindFirstMatchingRegisterNumber (const uint32_t inRegNum, const NTV2RegisterReads & inRegInfos)
{
	for (NTV2RegisterReadsConstIter	iter(inRegInfos.begin());  iter != inRegInfos.end();  ++iter)	//	Ugh -- linear search
		if (iter->registerNumber == inRegNum)
			return iter;
	return inRegInfos.end();
}


ostream & operator << (std::ostream & inOutStream, const NTV2RegInfo & inObj)
{
	return inObj.Print(inOutStream);
}


ostream & operator << (ostream & inOutStream, const NTV2RegisterWrites & inObj)
{
	inOutStream << inObj.size () << " regs:" << endl;
	for (NTV2RegisterWritesConstIter iter (inObj.begin ());  iter != inObj.end ();  ++iter)
		inOutStream << *iter << endl;
	return inOutStream;
}


NTV2BankSelGetSetRegs::NTV2BankSelGetSetRegs (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inOutRegInfo, const bool inDoWrite)
	:	mHeader			(NTV2_TYPE_BANKGETSET, sizeof (NTV2BankSelGetSetRegs)),
		mIsWriting		(inDoWrite),			//	Default to reading
		mInBankInfos	(sizeof (NTV2RegInfo)),	//	Room for one bank select
		mInRegInfos		(sizeof (NTV2RegInfo))	//	Room for one register read or write
{
	NTV2RegInfo *	pRegInfo		(reinterpret_cast <NTV2RegInfo *> (mInBankInfos.GetHostPointer ()));
	if (pRegInfo)
		*pRegInfo = inBankSelect;
	pRegInfo = reinterpret_cast <NTV2RegInfo *> (mInRegInfos.GetHostPointer ());
	if (pRegInfo)
		*pRegInfo = inOutRegInfo;
	NTV2_ASSERT_STRUCT_VALID;
}


NTV2RegInfo NTV2BankSelGetSetRegs::GetRegInfo (const UWord inIndex0) const
{
	NTV2_ASSERT_STRUCT_VALID;
	NTV2RegInfo	result;
	if (!mInRegInfos.IsNULL())
	{
        const ULWord	maxNum	(mInRegInfos.GetByteCount() / ULWord(sizeof(NTV2RegInfo)));
		if (ULWord(inIndex0) < maxNum)
		{
			const NTV2RegInfo *	pRegInfo (reinterpret_cast<const NTV2RegInfo*>(mInRegInfos.GetHostPointer()));
			result = pRegInfo[inIndex0];
		}
	}
	return result;
}


ostream & NTV2BankSelGetSetRegs::Print (ostream & inOutStream) const
{
	NTV2_ASSERT_STRUCT_VALID;
	const NTV2RegInfo *	pBankRegInfo	(reinterpret_cast <const NTV2RegInfo *> (mInBankInfos.GetHostPointer ()));
	const NTV2RegInfo *	pRegInfo		(reinterpret_cast <const NTV2RegInfo *> (mInRegInfos.GetHostPointer ()));
	inOutStream << mHeader << " " << (mIsWriting ? "W" : "R") << " bankRegInfo=";
	if (mInBankInfos.IsNULL ())
		inOutStream << "-";
	else
		inOutStream << *pBankRegInfo;
	inOutStream << " regInfo=";
	if (mInRegInfos.IsNULL ())
		inOutStream << "-";
	else
		inOutStream << *pRegInfo;
	return inOutStream;
}


NTV2VirtualData::NTV2VirtualData (const ULWord inTag, const void* inVirtualData, const size_t inVirtualDataSize, const bool inDoWrite)
    :	mHeader			(NTV2_TYPE_VIRTUAL_DATA_RW, sizeof (NTV2VirtualData)),
        mTag            (inTag),                                //  setup tag
        mIsWriting		(inDoWrite),                            //	setup write/read
        mVirtualData	(inVirtualData, inVirtualDataSize)      //	setup virtual data
{
    NTV2_ASSERT_STRUCT_VALID;
}


ostream & NTV2VirtualData::Print (ostream & inOutStream) const
{
    NTV2_ASSERT_STRUCT_VALID;
    inOutStream	<< mHeader << ", mTag=" << mTag << ", mIsWriting=" << mIsWriting;
    return inOutStream;
}
