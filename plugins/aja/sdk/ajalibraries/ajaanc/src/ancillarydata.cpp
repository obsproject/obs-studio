/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata.cpp
	@brief		Implementation of the AJAAncillaryData class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ntv2publicinterface.h"
#include "ancillarydata.h"
#include "ajabase/system/debug.h"	//	This makes 'ajaanc' dependent upon 'ajabase'
#if defined(AJA_LINUX)
	#include <string.h>				// For memcpy
	#include <stdlib.h>				// For realloc
#endif
#include <ios>

using namespace std;

#define	LOGMYERROR(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Error,		AJAFUNC << ":  " << __x__)
#define	LOGMYWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Warning,	AJAFUNC << ":  " << __x__)
#define	LOGMYNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Notice,		AJAFUNC << ":  " << __x__)
#define	LOGMYINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Info,		AJAFUNC << ":  " << __x__)
#define	LOGMYDEBUG(__x__)	AJA_sREPORT(AJA_DebugUnit_AJAAncData, AJA_DebugSeverity_Debug,		AJAFUNC << ":  " << __x__)

#define	RCV2110ERR(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Rcv, AJA_DebugSeverity_Error,		AJAFUNC << ":  " << __x__)
#define	RCV2110WARN(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Rcv, AJA_DebugSeverity_Warning,	AJAFUNC << ":  " << __x__)
#define	RCV2110NOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Rcv, AJA_DebugSeverity_Notice,		AJAFUNC << ":  " << __x__)
#define	RCV2110INFO(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Rcv, AJA_DebugSeverity_Info,		AJAFUNC << ":  " << __x__)
#define	RCV2110DBG(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Rcv, AJA_DebugSeverity_Debug,		AJAFUNC << ":  " << __x__)

#define	XMT2110ERR(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Xmit, AJA_DebugSeverity_Error,		AJAFUNC << ":  " << __x__)
#define	XMT2110WARN(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Xmit, AJA_DebugSeverity_Warning,	AJAFUNC << ":  " << __x__)
#define	XMT2110NOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Xmit, AJA_DebugSeverity_Notice,	AJAFUNC << ":  " << __x__)
#define	XMT2110INFO(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Xmit, AJA_DebugSeverity_Info,		AJAFUNC << ":  " << __x__)
#define	XMT2110DBG(__x__)	AJA_sREPORT(AJA_DebugUnit_Anc2110Xmit, AJA_DebugSeverity_Debug,		AJAFUNC << ":  " << __x__)

//	RCV2110DDBG & XMT2110DDBG are for EXTREMELY detailed debug logging:
#if 0	// DetailedDebugging
	#define	RCV2110DDBG(__x__)	RCV2110DBG(__x__)
	#define	XMT2110DDBG(__x__)	XMT2110DBG(__x__)
#else
	#define	RCV2110DDBG(__x__)	
	#define	XMT2110DDBG(__x__)	
#endif

#if defined(AJAHostIsBigEndian)
	//	Host is BigEndian (BE)
	#define AJA_ENDIAN_16NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		(__val__)
#else
	//	Host is LittleEndian (LE)
	#define AJA_ENDIAN_16NtoH(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		AJA_ENDIAN_SWAP64(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		AJA_ENDIAN_SWAP64(__val__)
#endif


static inline uint32_t ENDIAN_32NtoH(const uint32_t inValue)	{return AJA_ENDIAN_32NtoH(inValue);}
static inline uint32_t ENDIAN_32HtoN(const uint32_t inValue)	{return AJA_ENDIAN_32HtoN(inValue);}


const uint32_t AJAAncillaryDataWrapperSize = 7;		// 3 bytes header + DID + SID + DC + Checksum: i.e. everything EXCEPT the payload

//const uint8_t  AJAAncillaryDataAnalogDID = 0x00;		// used in header DID field when ancillary data is "analog"
//const uint8_t  AJAAncillaryDataAnalogSID = 0x00;		// used in header SID field when ancillary data is "analog"



AJAAncillaryData::AJAAncillaryData()
{
	Init();
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData & inClone)
{
	Init();
	*this = inClone;
}


AJAAncillaryData::AJAAncillaryData (const AJAAncillaryData * pClone)
{
	Init();
	if (pClone)
		*this = *pClone;
}


AJAAncillaryData::~AJAAncillaryData ()
{
	FreeDataMemory();
}


void AJAAncillaryData::Init()
{
	FreeDataMemory();	// reset all internal data to defaults

	m_DID			= 0x00;
	m_SID			= 0x00;
	m_checksum		= 0;
	m_frameID		= 0;
	m_userData		= 0;
	m_rcvDataValid	= false;
	m_coding		= AJAAncillaryDataCoding_Digital;
	m_ancType		= AJAAncillaryDataType_Unknown;
	m_bufferFmt		= AJAAncillaryBufferFormat_Unknown;
	//	Default location:
	m_location.SetDataLink(AJAAncillaryDataLink_A)					//	LinkA
				.SetDataStream(AJAAncillaryDataStream_1)			//	DS1
				.SetDataChannel(AJAAncillaryDataChannel_Y)			//	Y channel
				.SetDataSpace(AJAAncillaryDataSpace_VANC)			//	VANC
				.SetHorizontalOffset(AJAAncDataHorizOffset_AnyVanc)	//	VANC
				.SetLineNumber(AJAAncDataLineNumber_Unknown);		//	Unknown line number
}


void AJAAncillaryData::Clear (void)
{
	Init();
}


AJAAncillaryData * AJAAncillaryData::Clone (void) const
{
	return new AJAAncillaryData (this);
}


AJAStatus AJAAncillaryData::AllocDataMemory(uint32_t numBytes)
{
	AJAStatus status;
	FreeDataMemory();
	try
	{
		m_payload.reserve(numBytes);
		for (uint32_t ndx(0);  ndx < numBytes;  ndx++)
			m_payload.push_back(0);
		status = AJA_STATUS_SUCCESS;
	}
	catch(const bad_alloc &)
	{
		m_payload.clear();
		status = AJA_STATUS_MEMORY;
	}
	if (AJA_FAILURE(status))
		LOGMYERROR(::AJAStatusToString(status) << ": Unable to allocate/reserve " << DEC(numBytes) << " bytes");
	return status;
}


AJAStatus AJAAncillaryData::FreeDataMemory (void)
{
	m_payload.clear();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetDID (const uint8_t inDID)
{
	m_DID = inDID;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetSID (const uint8_t inSID)
{
	m_SID = inSID;
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJAAncillaryData::SetChecksum (const uint8_t inChecksum, const bool inValidate)
{
	m_checksum = inChecksum;
	if (inValidate)
		if (inChecksum != Calculate8BitChecksum())
			return AJA_STATUS_FAIL;
	return AJA_STATUS_SUCCESS;
}


uint16_t AJAAncillaryData::GetStreamInfo (void) const
{
	if (IS_VALID_AJAAncillaryDataStream(GetLocationDataStream()))
		return uint16_t(GetLocationDataStream());
	else if (IS_VALID_AJAAncillaryDataLink(GetLocationVideoLink()))
		return uint16_t(GetLocationVideoLink());
	return 0;
}


uint8_t AJAAncillaryData::Calculate8BitChecksum (void) const
{
	//	NOTE:	This is NOT the "real" 9-bit checksum used in SMPTE 291 Ancillary packets,
	//			but it's calculated the same way and should be the same as the LS 8-bits
	//			of the 9-bit checksum...
	uint8_t	sum	(m_DID);
	sum += m_SID;
	sum += uint8_t(m_payload.size());
	if (!m_payload.empty())
		for (ByteVector::size_type ndx(0);  ndx < m_payload.size();  ndx++)
			sum += m_payload[ndx];
NTV2_ASSERT(sum == uint8_t(Calculate9BitChecksum()));
	return sum;
}


uint16_t AJAAncillaryData::Calculate9BitChecksum (void) const
{
	//	SMPTE 291-1:2011:	6.7 Checksum Word
	uint16_t	sum	(AddEvenParity(m_DID));	//	DID
	sum += AddEvenParity(m_SID);				//	+ SDID
	sum += AddEvenParity(UByte(GetDC()));	//	+ DC
	if (!m_payload.empty())										//	+ payload:
		for (ByteVector::size_type ndx(0);  ndx < m_payload.size();  ndx++)
			sum += AddEvenParity(m_payload[ndx]);

	const bool	b8	((sum & 0x100) != 0);
	const bool	b9	(!b8);
	return (sum & 0x1FF) | (b9 ? 0x200 : 0x000);
}


//**********
// Set anc data location parameters
//
AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLocation & loc)
{
	AJAStatus	status	(SetLocationVideoLink(loc.GetDataLink()));
	if (AJA_SUCCESS(status))
		status = SetLocationDataStream(loc.GetDataStream());
	if (AJA_SUCCESS(status))
		status = SetLocationDataChannel(loc.GetDataChannel());
	if (AJA_SUCCESS(status))
		status = SetLocationHorizOffset(loc.GetHorizontalOffset());
	if (AJA_SUCCESS(status))
		status = SetLocationLineNumber(loc.GetLineNumber());
	return status;
}


#if !defined(NTV2_DEPRECATE_15_2)
	AJAStatus AJAAncillaryData::GetDataLocation (AJAAncillaryDataLink &			outLink,
												AJAAncillaryDataVideoStream &	outStream,
												AJAAncillaryDataSpace &			outAncSpace,
												uint16_t &						outLineNum)
	{
		outLink		= m_location.GetDataLink();
		outStream	= m_location.GetDataChannel();
		outAncSpace	= m_location.GetDataSpace();
		outLineNum	= m_location.GetLineNumber();
		return AJA_STATUS_SUCCESS;
	}

	AJAStatus AJAAncillaryData::SetDataLocation (const AJAAncillaryDataLink inLink, const AJAAncillaryDataChannel inChannel, const AJAAncillaryDataSpace inAncSpace, const uint16_t inLineNum, const AJAAncillaryDataStream inStream)
	{
		AJAStatus	status	(SetLocationVideoLink(inLink));
		if (AJA_SUCCESS(status))
			status = SetLocationDataStream(inStream);
		if (AJA_SUCCESS(status))
			status = SetLocationDataChannel(inChannel);
		if (AJA_SUCCESS(status))
			status = SetLocationVideoSpace(inAncSpace);
		if (AJA_SUCCESS(status))
			status = SetLocationLineNumber(inLineNum);
		return status;
	}
#endif	//	!defined(NTV2_DEPRECATE_15_2)


//-------------
//
AJAStatus AJAAncillaryData::SetLocationVideoLink (const AJAAncillaryDataLink inLinkValue)
{
	if (!IS_VALID_AJAAncillaryDataLink(inLinkValue))
		return AJA_STATUS_RANGE;

	m_location.SetDataLink(inLinkValue);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationDataStream (const AJAAncillaryDataStream inStream)
{
	if (!IS_VALID_AJAAncillaryDataStream(inStream))
		return AJA_STATUS_RANGE;

	m_location.SetDataStream(inStream);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationDataChannel (const AJAAncillaryDataChannel inChannel)
{
	if (!IS_VALID_AJAAncillaryDataChannel(inChannel))
		return AJA_STATUS_RANGE;

	m_location.SetDataChannel(inChannel);
	return AJA_STATUS_SUCCESS;
}


#if !defined(NTV2_DEPRECATE_15_2)
	AJAStatus AJAAncillaryData::SetLocationVideoSpace (const AJAAncillaryDataSpace inSpace)
	{
		if (!IS_VALID_AJAAncillaryDataSpace(inSpace))
			return AJA_STATUS_RANGE;
	
		m_location.SetDataSpace(inSpace);
		return AJA_STATUS_SUCCESS;
	}
#endif	//	!defined(NTV2_DEPRECATE_15_2)


//-------------
//
AJAStatus AJAAncillaryData::SetLocationLineNumber (const uint16_t inLineNum)
{
	//	No range checking here because we don't know how tall the frame is
	m_location.SetLineNumber(inLineNum);
	return AJA_STATUS_SUCCESS;
}


//-------------
//
AJAStatus AJAAncillaryData::SetLocationHorizOffset (const uint16_t inOffset)
{
	//	No range checking here because we don't know how wide the frame is
	m_location.SetHorizontalOffset(inOffset);
	return AJA_STATUS_SUCCESS;
}

//-------------
//
AJAStatus AJAAncillaryData::SetDataCoding (const AJAAncillaryDataCoding inCodingType)
{
	if (m_coding != AJAAncillaryDataCoding_Digital  &&  m_coding != AJAAncillaryDataCoding_Raw)
		return AJA_STATUS_RANGE;

	m_coding = inCodingType;
	return AJA_STATUS_SUCCESS;
}


//**********
// Copy payload data from external source to internal storage. Erases current payload memory
// (if any) and allocates new memory

AJAStatus AJAAncillaryData::SetPayloadData (const uint8_t * pInData, const uint32_t inNumBytes)
{
	if (pInData == NULL || inNumBytes == 0)
		return AJA_STATUS_NULL;

	//	[Re]Allocate...
	AJAStatus status (AllocDataMemory(inNumBytes));
	if (AJA_FAILURE(status))
		return status;

	//	Copy payload into memory...
	::memcpy (&m_payload[0], pInData, inNumBytes);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::SetFromSMPTE334 (const uint16_t * pInData, const uint32_t inNumWords, const AJAAncillaryDataLocation & inLocInfo)
{
	if (!pInData)
		return AJA_STATUS_NULL;
	if (inNumWords < 7)
		return AJA_STATUS_RANGE;

	const uint32_t	payloadByteCount	(uint32_t(pInData[5] & 0x00FF));
	if ((inNumWords - 7) > payloadByteCount)
		return AJA_STATUS_RANGE;

	//	[Re]Allocate...
	const AJAStatus status	(AllocDataMemory(payloadByteCount));
	if (AJA_FAILURE(status))
		return status;

	//	Copy payload into new memory...
	for (uint32_t numWord (0);  numWord < payloadByteCount;  numWord++)
		m_payload[numWord] = UByte(pInData[6+numWord] & 0x00FF);

	SetDataCoding(AJAAncillaryDataCoding_Digital);
	SetDataLocation(inLocInfo);
	SetChecksum(UByte(pInData[6+payloadByteCount] & 0x00FF));
	SetDID(UByte(pInData[3] & 0x00FF));
	SetSID(UByte(pInData[4] & 0x00FF));

	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from external source to existing internal storage. Realloc's current
// payload memory (if any) or allocates new memory

AJAStatus AJAAncillaryData::AppendPayloadData (const uint8_t * pInData, const uint32_t inNumBytes)
{
	if (pInData == NULL || inNumBytes == 0)
		return AJA_STATUS_NULL;

	try
	{
		for (uint32_t ndx(0);  ndx < inNumBytes;  ndx++)
			m_payload.push_back(pInData[ndx]);
	}
	catch(const bad_alloc &)
	{
		return AJA_STATUS_MEMORY;
	}
	
	return AJA_STATUS_SUCCESS;
}


//**********
// Append payload data from an external AJAAncillaryData object to existing internal storage.

AJAStatus AJAAncillaryData::AppendPayload (const AJAAncillaryData & inAnc)
{
	try
	{
		const uint8_t *	pInData		(inAnc.GetPayloadData());
		const uint32_t	numBytes	(uint32_t(inAnc.GetPayloadByteCount()));
		for (uint32_t ndx(0);  ndx < numBytes;  ndx++)
			m_payload.push_back(pInData[ndx]);
	}
	catch(const bad_alloc &)
	{
		return AJA_STATUS_MEMORY;
	}

	return AJA_STATUS_SUCCESS;
}


//**********
// Copy payload data from internal storage to external destination.

AJAStatus AJAAncillaryData::GetPayloadData (uint8_t * pOutData, const uint32_t inNumBytes) const
{
	if (pOutData == NULL)
		return AJA_STATUS_NULL;

	if (ByteVectorIndex(inNumBytes) > m_payload.size())
		return AJA_STATUS_RANGE;

	::memcpy (pOutData, GetPayloadData(), inNumBytes);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::GetPayloadData (UWordSequence & outUDWs, const bool inAddParity) const
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	const UWordSequence::size_type	origSize	(outUDWs.size());
	for (ByteVectorConstIter iter(m_payload.begin());  iter != m_payload.end()  &&  AJA_SUCCESS(status);  ++iter)
	{
		const uint16_t	UDW	(inAddParity ? AddEvenParity(*iter) : *iter);
		try
		{
			outUDWs.push_back(UDW);	//	Copy 8-bit data into LS 8 bits, add even parity to bit 8, and ~bit 8 to bit 9
		}
		catch(...)
		{
			status = AJA_STATUS_MEMORY;
		}
	}	//	for each packet byte
	if (AJA_FAILURE(status))
		outUDWs.resize(origSize);
	return status;
}


uint8_t AJAAncillaryData::GetPayloadByteAtIndex (const uint32_t inIndex0) const
{
	if (ByteVectorIndex(inIndex0) < m_payload.size())
		return m_payload[inIndex0];
	else
		return 0;
}


AJAStatus AJAAncillaryData::SetPayloadByteAtIndex (const uint8_t inDataByte, const uint32_t inIndex0)
{
	if (inIndex0 >= GetDC())
		return AJA_STATUS_RANGE;

	m_payload[inIndex0] = inDataByte;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData::ParsePayloadData (void)
{
	// should be overridden by derived classes to parse payload data to local data
	m_rcvDataValid = false;

	return AJA_STATUS_SUCCESS;
}


//**********
//	[Re]Initializes me from the 8-bit GUMP buffer received from extractor (ingest)
AJAStatus AJAAncillaryData::InitWithReceivedData (const uint8_t *					pInData,
													const size_t					inMaxBytes,
													const AJAAncillaryDataLocation & inLocationInfo,
													uint32_t &						outPacketByteCount)
{
	AJAStatus status = AJA_STATUS_SUCCESS;
	Clear();

	// If all is well, pInData points to the beginning of a "GUMP" packet:
	//
	//	pInData ->	0:	0xFF			// 1st byte is always FF
	//				1:  Hdr data1		// location data byte #1
	//				2:  Hdr data2		// location data byte #2
	//				3:  DID				// ancillary packet Data ID
	//				4:  SID				// ancillary packet Secondary ID (or DBN)
	//				5:  DC				// ancillary packet Data Count (size of payload: 0 - 255)
	//				6:  Payload[0]		// 1st byte of payload UDW
	//				7:  Payload[1]		// 2nd byte of payload UDW
	//             ...    ...
	//		 (5 + DC):  Payload[DC-1]	// last byte of payload UDW
	//		 (6 + DC):	CS (checksum)	// 8-bit sum of (DID + SID + DC + Payload[0] + ... + Payload[DC-1])
	//
	//		 (7 + DC):  (start of next packet, if any...) returned in packetSize.
	//
	// Note that this is the layout of the data as returned from the Anc Extractor hardware, and
	// is NOT exactly the same as SMPTE-291.
	//
	// The inMaxBytes input gives us an indication of how many "valid" bytes remain in the caller's TOTAL
	// ANC Data buffer. We use this as a sanity check to make sure we don't try to parse past the end
	// of the captured data.
	//
	// The caller provides an AJAAncillaryDataLocation struct with all of the information filled in
	// except the line number.
	//
	// When we have extracted the useful data from the packet, we return the packet size, in bytes, so the
	// caller can find the start of the next packet (if any).

	const uint32_t maxBytes(uint32_t(inMaxBytes+0));
	if (pInData == AJA_NULL)
	{
		outPacketByteCount = 0;
		LOGMYERROR("AJA_STATUS_NULL: NULL pointer");
		return AJA_STATUS_NULL;
	}

	//	The minimum size for a packet (i.e. no payload) is 7 bytes
	if (maxBytes < AJAAncillaryDataWrapperSize)
	{
		outPacketByteCount = maxBytes;
		LOGMYERROR("AJA_STATUS_RANGE: Buffer size " << maxBytes << " smaller than " << AJAAncillaryDataWrapperSize << " bytes");
		return AJA_STATUS_RANGE;
	}

	//	The first byte should be 0xFF. If it's not, then the Anc data stream may be broken...
	if (pInData[0] != 0xFF)
	{
		//	Let the caller try to resynchronize...
		outPacketByteCount = 0;
		LOGMYDEBUG("No data:  First GUMP byte is " << xHEX0N(uint16_t(pInData[0]),2) << ", expected 0xFF");
		return AJA_STATUS_SUCCESS;	//	Not necessarily an error (no data)
	}

	//	So we have at least enough bytes for a minimum packet, and the first byte is what we expect.
	//	Let's see what size this packet actually reports...
	const uint32_t	totalBytes	(pInData[5] + AJAAncillaryDataWrapperSize);

	//	If the reported packet size extends beyond the end of the buffer, we're toast...
	if (totalBytes > maxBytes)
	{
		outPacketByteCount = maxBytes;
		LOGMYERROR("AJA_STATUS_RANGE: Reported packet size " << totalBytes << " [bytes] extends past end of buffer " << inMaxBytes << " by " << (totalBytes-inMaxBytes) << " byte(s)");
		return AJA_STATUS_RANGE;
	}

	//	OK... There's enough data in the buffer to contain the packet, and everything else checks out,
	//	so continue parsing the data...
	m_DID	   = pInData[3];				// SMPTE-291 Data ID
	m_SID	   = pInData[4];				// SMPTE-291 Secondary ID (or DBN)
	m_checksum = pInData[totalBytes-1];		// Reported checksum

	//	Caller provides all of the "location" information as a default. If the packet header info is "real", overwrite it...
	m_location = inLocationInfo;

	if ((pInData[1] & 0x80) != 0)
	{
		m_coding            = ((pInData[1] & 0x40) == 0) ? AJAAncillaryDataCoding_Digital : AJAAncillaryDataCoding_Analog;	// byte 1, bit 6
		m_location.SetDataStream(AJAAncillaryDataStream_1);	//	???	GUMP doesn't tell us the data stream it came from
		m_location.SetDataChannel(((pInData[1] & 0x20) == 0) ? AJAAncillaryDataChannel_C : AJAAncillaryDataChannel_Y);		// byte 1, bit 5
		m_location.SetDataSpace(((pInData[1] & 0x10) == 0) ? AJAAncillaryDataSpace_VANC : AJAAncillaryDataSpace_HANC);		// byte 1, bit 4
		m_location.SetLineNumber(uint16_t((pInData[1] & 0x0F) << 7) + uint16_t(pInData[2] & 0x7F));							// byte 1, bits 3:0 + byte 2, bits 6:0
		SetBufferFormat(AJAAncillaryBufferFormat_SDI);
		//m_location.SetHorizontalOffset(hOffset);	//	??? GUMP doesn't report the horiz offset of where the packet started in the raster line
	}

	//	Allocate space for the payload and copy it in...
	const uint32_t	payloadSize	(pInData[5]);	//	DC:	SMPTE-291 Data Count
	if (payloadSize)
	{
		status = AllocDataMemory(payloadSize);	//	NOTE:  This also sets my "DC" value
		if (AJA_SUCCESS(status))
			for (uint32_t ndx(0);  ndx < payloadSize;  ndx++)
				m_payload[ndx] = pInData[ndx+6];
	}

	outPacketByteCount = totalBytes;
	LOGMYDEBUG("Set from GUMP buffer OK: " << AsString(32));
	return status;

}	//	InitWithReceivedData


AJAStatus AJAAncillaryData::InitWithReceivedData (const ByteVector & inData, const AJAAncillaryDataLocation & inLocationInfo)
{
	uint32_t	pktByteCount(0);
	if (inData.empty())
		return AJA_STATUS_NULL;
	return InitWithReceivedData (&inData[0], uint32_t(inData.size()), inLocationInfo, pktByteCount);
}


//**********
// This returns the number of bytes that will be returned by GenerateTransmitData(). This is usually
// called first so the caller can allocate a buffer large enough to hold the results.

AJAStatus AJAAncillaryData::GetRawPacketSize (uint32_t & outPacketSize) const
{
	outPacketSize = 0;

	if (m_coding == AJAAncillaryDataCoding_Digital)
	{
		//	Normal, "digital" ancillary data (i.e. SMPTE-291 based) will have a total size of
		//	seven bytes (3 bytes header + DID + SID + DC + Checksum) plus the payload size...
		if (GetDC() <= 255)
			outPacketSize = GetDC() + AJAAncillaryDataWrapperSize;
		else
		{
			//	More than 255 bytes of payload is illegal  --  return 255 (truncated) to prevent generating a bad GUMP buffer...
			LOGMYWARN("Illegal packet size " << DEC(GetDC()) << ", exceeds 255 -- returning truncated value (255): " << AsString(32));
			outPacketSize = 255 + AJAAncillaryDataWrapperSize;
		}
	}
	else if (m_coding == AJAAncillaryDataCoding_Raw)
	{
		//	Determine how many "packets" are needed to be generated in order to pass all of the payload data (max 255 bytes per packet)...
		if (!IsEmpty())
		{
			//	All "analog" packets will have a 255-byte payload, except for the last one...
			const uint32_t	numPackets	((GetDC() + 254) / 255);
			const uint32_t	lastPacketDC	(GetDC() % 255);

			//	Each packet has a 7-byte "wrapper" + the payload size...
			outPacketSize =  ((numPackets - 1) * (255 + AJAAncillaryDataWrapperSize))	//	All packets except the last one have a payload of 255 bytes
							+ (lastPacketDC + AJAAncillaryDataWrapperSize);				//	The last packet carries the remainder
		}
	}
	else	// huh? (coding not set)
		return AJA_STATUS_FAIL;
	
	return AJA_STATUS_SUCCESS;
}


//**********
//	Writes my payload data into the given 8-bit GUMP buffer (playback)

AJAStatus AJAAncillaryData::GenerateTransmitData (uint8_t * pData, const size_t inMaxBytes, uint32_t & outPacketSize)
{
	AJAStatus status (GeneratePayloadData());

	outPacketSize = 0;

	//	Verify that the caller has allocated enough space to hold what's going to be generated
	uint32_t myPacketSize(0), maxBytes(uint32_t(inMaxBytes+0));
	GetRawPacketSize(myPacketSize);

	if (myPacketSize == 0)
	{
		LOGMYERROR("AJA_STATUS_FAIL: nothing to do -- raw packet size is zero: " << AsString(32));
		return AJA_STATUS_FAIL;
	}
	if (myPacketSize > maxBytes)
	{
		LOGMYERROR("AJA_STATUS_FAIL: " << maxBytes << "-byte client buffer too small to hold " << myPacketSize << " byte(s): " << AsString(32));
		return AJA_STATUS_FAIL;
	}
	if (!IsDigital() && !IsRaw())	//	Coding not set: don't generate anything
	{
		LOGMYERROR("AJA_STATUS_FAIL: invalid packet coding (neither Raw nor Digital): " << AsString(32));
		return AJA_STATUS_FAIL;
	}

	if (IsDigital())
	{
		pData[0] = GetGUMPHeaderByte1();
		pData[1] = GetGUMPHeaderByte2();
		pData[2] = GetGUMPHeaderByte3();
		pData[3] = m_DID;
		pData[4] = m_SID;

		uint8_t payloadSize = uint8_t((GetDC() > 255) ? 255 : GetDC());	//	Truncate payload to max 255 bytes
		pData[5] = payloadSize;

		//	Copy payload data to raw stream...
		status = GetPayloadData(&pData[6], payloadSize);

		//	NOTE:	The hardware automatically recalculates the checksum anyway, so this byte
		//			is ignored. However, if the original source had a checksum, we'll send it back...
		pData[6+payloadSize] = Calculate8BitChecksum();

		//	Done!
		outPacketSize = myPacketSize;
	}
	else if (IsRaw())
	{
		//	"Analog" or "raw" ancillary data is special in that it may generate multiple output packets,
		//	depending on the length of the payload data.
		//	NOTE:	This code assumes that zero-length payloads have already been screened out (in GetRawPacketSize)!
		const uint32_t	numPackets	((GetDC() + 254) / 255);

		// all packets will have a 255-byte payload except the last one
		//Note: Unused --  uint32_t lastPacketDC = m_DC % 255;

		const uint8_t *	payloadPtr = GetPayloadData();
		uint32_t remainingPayloadData = GetDC();

		for (uint32_t ndx(0);  ndx < numPackets;  ndx++)
		{
			pData[0] = GetGUMPHeaderByte1();
			pData[1] = GetGUMPHeaderByte2();
			pData[2] = GetGUMPHeaderByte3();
			pData[3] = m_DID;
			pData[4] = m_SID;

			uint8_t payloadSize = uint8_t((remainingPayloadData > 255) ? 255 : remainingPayloadData);	//	Truncate payload to max 255 bytes
			pData[5] = payloadSize;		//	DC

			//	Copy payload data into GUMP buffer...
			::memcpy(&pData[6], payloadPtr, payloadSize);

			//	NOTE:	The hardware automatically recalculates the checksum anyway, so this byte
			//			is ignored. However, if the original source had a checksum we'll send it back...
			pData[6+payloadSize] = m_checksum;

			//	Advance the payloadPtr to the beginning of the next bunch of payload data...
			payloadPtr += payloadSize;

			//	Decrease the remaining data count...
			remainingPayloadData -= payloadSize;

			//	Advance the external data stream pointer to the beginning of the next packet...
			pData += (payloadSize + AJAAncillaryDataWrapperSize);
		}	//	for each raw packet

		outPacketSize = myPacketSize;	//	Done!
	}	//	else if IsRaw
	LOGMYDEBUG(outPacketSize << " byte(s) generated: " << AsString(32));
	return status;

}	//	GenerateTransmitData


uint8_t AJAAncillaryData::GetGUMPHeaderByte2 (void) const
{
	uint8_t result	(0x80);	// LE bit is always active

	if (m_coding == AJAAncillaryDataCoding_Raw)
		result |= 0x40;		// analog/raw (1) or digital (0) ancillary data

	if (m_location.IsLumaChannel())
		result |= 0x20;		// carried in Y (1) or C stream

	if (m_location.IsHanc())
		result |= 0x10;

	// ms 4 bits of line number
	result |= (m_location.GetLineNumber() >> 7) & 0x0F;	// ms 4 bits [10:7] of line number

	return result;
}


AJAStatus AJAAncillaryData::GenerateTransmitData (UWordSequence & outRawComponents)
{
	AJAStatus						status		(GeneratePayloadData());
	const UWordSequence::size_type	origSize	(outRawComponents.size());

	if (IsDigital())
	{
		try
		{
			const uint8_t	dataCount	((GetDC() > 255) ? 255 : uint8_t(GetDC()));	//	Truncate payload to max 255 bytes
			outRawComponents.push_back(0x000);											//	000
			outRawComponents.push_back(0x3FF);											//	3FF
			outRawComponents.push_back(0x3FF);											//	3FF
			outRawComponents.push_back(AddEvenParity(GetDID()));		//	DID
			outRawComponents.push_back(AddEvenParity(GetSID()));		//	SDID
			outRawComponents.push_back(AddEvenParity(dataCount));	//	DC
		}
		catch(...)
		{
			outRawComponents.resize(origSize);
			status = AJA_STATUS_MEMORY;
		}
	}

	//	Copy payload data into output vector...
	if (AJA_SUCCESS(status))
		status = GetPayloadData(outRawComponents, IsDigital() /* Add parity for Digital only */);	//	UDWs

	//	The hardware automatically recalcs the CS, but still needs to be there...
	if (AJA_SUCCESS(status) && IsDigital())
		outRawComponents.push_back(Calculate9BitChecksum());	//	CS

	if (AJA_SUCCESS(status))
		LOGMYDEBUG((origSize ? "Appended " : "Generated ")  << (outRawComponents.size() - origSize)  << " UWords from " << AsString(32) << endl << UWordSequence(outRawComponents));
	else
		LOGMYERROR("Failed: " << ::AJAStatusToString(status) << ": origSize=" << origSize << ", " << AsString(32));
	return status;
}


//	These tables implement the 16-UDWs-per-20-bytes packing cadence:
static const size_t		gIndexes[]	=	{	0,1,2,3,	3,4,5,6,	6,7,8,9,	9,10,11,12,	12,13,14,15	};
static const unsigned	gShifts[]	=	{	22,12,2,8,	24,14,4,6,	26,16,6,4,	28,18,8,2,	30,20,10,0	};
static const uint32_t	gMasks[]	=	{	0xFFC00000, 0x003FF000, 0x00000FFC, 0x00000003,
											0xFF000000, 0x00FFC000, 0x00003FF0, 0x0000000F,
											0xFC000000, 0x03FF0000, 0x0000FFC0, 0x0000003F,
											0xF0000000, 0x0FFC0000, 0x0003FF00, 0x000000FF,
											0xC0000000, 0x3FF00000, 0x000FFC00, 0x000003FF	};


AJAStatus AJAAncillaryData::GenerateTransmitData (ULWordSequence & outData)
{
	AJAStatus						status		(GeneratePayloadData());
	const ULWordSequence::size_type	origSize	(outData.size());
	uint32_t						u32			(0);	//	32-bit value

	if (!IsDigital())
		{XMT2110WARN("Analog/raw packet skipped/ignored: " << AsString(32));	return AJA_STATUS_SUCCESS;}
	if (GetDC() > 255)
		{XMT2110ERR("Data count exceeds 255: " << AsString(32));	return AJA_STATUS_RANGE;}

	//////////////////////////////////////////////////
	//	Prepare an array of 10-bit DID/SID/DC/UDWs/CS values...
		const uint16_t	did	(AddEvenParity(GetDID()));
		const uint16_t	sid	(AddEvenParity(GetSID()));
		const uint16_t	dc	(AddEvenParity(uint8_t(GetDC())));
		const uint16_t	cs	(Calculate9BitChecksum());

		UWordSequence	UDW16s;		//	10-bit even-parity words
		UDW16s.reserve(GetDC()+4);	//	Reserve DID + SID + DC + UDWs + CS
		UDW16s.push_back(did);
		UDW16s.push_back(sid);
		UDW16s.push_back(dc);

		//	Append 8-bit payload data, converting into 10-bit values with even parity added...
		status = GetPayloadData(UDW16s, true);	//	Append 10-bit even-parity UDWs
		if (AJA_FAILURE(status))
			{XMT2110ERR("GetPayloadData failed: " << AsString(32));	return status;}
		UDW16s.push_back(cs);	//	Checksum is the caboose
	//	Done -- 10-bit DID/SID/DC/UDWs/CS array is prepared
	XMT2110DBG("From " << UWordSequence(UDW16s) << " " << AsString(32));
	//////////////////////////////////////////////////

	//	Begin writing into "outData" array.
	//	My first 32-bit longword is the Anc packet header, which contains location info...
	const AJARTPAncPacketHeader	pktHdr	(GetDataLocation());
	const uint32_t				pktHdrWord	(pktHdr.GetULWord());
	outData.push_back(pktHdrWord);
//	XMT2110DDBG("outU32s[" << DEC(outData.size()-1) << "]=" << xHEX0N(ENDIAN_32NtoH(pktHdrWord),8) << " (BigEndian)");	//	Byte-Swap it to make BigEndian look right

	//	All subsequent 32-bit longwords come from the array of 10-bit values I built earlier.
	const size_t	numUDWs	(UDW16s.size());
	size_t			UDWndx	(0);
	u32 = 0;
	do
	{
		for (unsigned loopNdx(0);  loopNdx < sizeof(gIndexes) / sizeof(size_t);  loopNdx++)
		{
			const bool		is4th		((loopNdx & 3) == 3);
			const size_t	ndx			(UDWndx + gIndexes[loopNdx]);
			const bool		isPastEnd	(ndx >= numUDWs);
			const uint32_t	UDW			(isPastEnd  ?  0  :  uint32_t(UDW16s[ndx]));
			const unsigned	shift		(gShifts[loopNdx]);
			const uint32_t	mask		(gMasks[loopNdx]);
//			if (!isPastEnd)	XMT2110DDBG("u16s[" << DEC(ndx) << "]=" << xHEX0N(UDW,3));
			if (is4th)
			{
				u32 |=  (UDW >> shift) & mask;
				outData.push_back(ENDIAN_32HtoN(u32));
//				XMT2110DDBG("outU32s[" << DEC(outData.size()-1) << "]=" << xHEX0N(u32,8) << " (BigEndian)");
				u32 = 0;	//	Reset, start over
				if (isPastEnd)
					break;	//	Done, 32-bit longword aligned
				continue;
			}
			//	Continue building this 32-bit longword...
			u32 |=  (UDW << shift) & mask;
		}	//	inner loop
		UDWndx += 16;
	} while (UDWndx < numUDWs);

	/*	The Pattern:  (unrolling the above loop):														BitCnt		ShiftLeft	ShiftRight
	u32 =  (uint32_t(UDW16s.at( 0)) << 22) & 0xFFC00000;	//	0b00000|0x00|00: [ 0] all 10 bits		0+10=10		32-10=22
	u32 |= (uint32_t(UDW16s.at( 1)) << 12) & 0x003FF000;	//	0b00001|0x01|01: [ 1] all 10 bits		10+10=20	22-10=12
	u32 |= (uint32_t(UDW16s.at( 2)) <<  2) & 0x00000FFC;	//	0b00010|0x02|02: [ 2] all 10 bits		20+10=30	12-10=2
	u32 |= (uint32_t(UDW16s.at( 3)) >>  8) & 0x00000003;	//	0b00011|0x03|03: [ 3] first (MS 2 bits)	30+2=32					10-2=8
	outData.push_back(ENDIAN_32HtoN(u32));
	u32 =  (uint32_t(UDW16s.at( 3)) << 24) & 0xFF000000;	//	0b00100|0x04|04: [ 3] last (LS 8 bits)	0+8=8		32-8=24
	u32 |= (uint32_t(UDW16s.at( 4)) << 14) & 0x00FFC000;	//	0b00101|0x05|05: [ 4] all 10 bits		8+10=18		24-10=14
	u32 |= (uint32_t(UDW16s.at( 5)) <<  4) & 0x00003FF0;	//	0b00110|0x06|06: [ 5] all 10 bits		18+10=28	14-10=4
	u32 |= (uint32_t(UDW16s.at( 6)) >>  6) & 0x0000000F;	//	0b00111|0x07|07: [ 6] first (MS 4 bits)	28+4=32					10-4=6
	outData.push_back(ENDIAN_32HtoN(u32));
	u32 =  (uint32_t(UDW16s.at( 6)) << 26) & 0xFC000000;	//	0b01000|0x08|08: [ 6] last (LS 6 bits)	0+6=6		32-6=26
	u32 |= (uint32_t(UDW16s.at( 7)) << 16) & 0x03FF0000;	//	0b01001|0x09|09: [ 7] all 10 bits		6+10=16		26-10=16
	u32 |= (uint32_t(UDW16s.at( 8)) <<  6) & 0x0000FFC0;	//	0b01010|0x0A|10: [ 8] all 10 bits		16+10=26	16-10=6
	u32 |= (uint32_t(UDW16s.at( 9)) >>  4) & 0x0000003F;	//	0b01011|0x0B|11: [ 9] first (MS 6 bits)	26+6=32					10-6=4
	outData.push_back(ENDIAN_32HtoN(u32));
	u32 =  (uint32_t(UDW16s.at( 9)) << 28) & 0xF0000000;	//	0b01100|0x0C|12: [ 9] last (LS 4 bits)	0+4=4		32-4=28
	u32 |= (uint32_t(UDW16s.at(10)) << 18) & 0x0FFC0000;	//	0b01101|0x0D|13: [10] all 10 bits		4+10=14		28-10=18
	u32 |= (uint32_t(UDW16s.at(11)) <<  8) & 0x0003FF00;	//	0b01110|0x0E|14: [11] all 10 bits		14+10=24	18-10=8
	u32 |= (uint32_t(UDW16s.at(12)) >>  2) & 0x000000FF;	//	0b01111|0x0F|15: [12] first (MS 8 bits)	24+8=32					10-8=2
	outData.push_back(ENDIAN_32HtoN(u32));
	u32 =  (uint32_t(UDW16s.at(12)) << 30) & 0xC0000000;	//	0b10000|0x10|16: [12] last (LS 2 bits)	0+2=2		32-2=30
	u32 |= (uint32_t(UDW16s.at(13)) << 20) & 0x3FF00000;	//	0b10001|0x11|17: [13] all 10 bits		2+10=12		30-10=20
	u32 |= (uint32_t(UDW16s.at(14)) << 10) & 0x000FFC00;	//	0b10010|0x12|18: [14] all 10 bits		12+10=22	20-10=10
	u32 |= (uint32_t(UDW16s.at(15)) >>  0) & 0x000003FF;	//	0b10011|0x13|19: [15] all 10 bits		22+10=32				10-10=0
	outData.push_back(ENDIAN_32HtoN(u32));	*/

#if defined(_DEBUG)
{
	ostringstream	oss;
	oss << (origSize ? "Appended " : "Generated ") << (outData.size() - origSize)  << " U32s:";
	for (size_t ndx(origSize);  ndx < outData.size();  ndx++)	//	outData is in Network Byte Order
		oss << " " << HEX0N(ENDIAN_32NtoH(outData[ndx]),8);		//	so byte-swap to make it look right in the log
	oss << " BigEndian from " << AsString(32);
	XMT2110DBG(oss.str());
}
#else
	XMT2110DBG((origSize ? "Appended " : "Generated ")
					<< (outData.size() - origSize)  << " 32-bit words from " << AsString(32));
#endif
	return AJA_STATUS_SUCCESS;
}	//	GenerateTransmitData


AJAStatus AJAAncillaryData::InitWithReceivedData (const ULWordSequence & inU32s, uint16_t & inOutU32Ndx, const bool inIgnoreChecksum)
{
	const size_t	numU32s	(inU32s.size());

	Clear();	//	Reset me -- start over

	if (inOutU32Ndx >= numU32s)
		{RCV2110ERR("Index error: [" << DEC(inOutU32Ndx) << "] past end of [" << DEC(numU32s) << "] element buffer");  return AJA_STATUS_RANGE;}

	AJARTPAncPacketHeader	ancPktHeader;
	if (!ancPktHeader.ReadFromULWordVector(inU32s, inOutU32Ndx))
		{RCV2110ERR("AJARTPAncPacketHeader::ReadFromULWordVector failed at [" << DEC(inOutU32Ndx) << "]");	return AJA_STATUS_FAIL;}

	const AJAAncillaryDataLocation	dataLoc	(ancPktHeader.AsDataLocation());
	RCV2110DDBG("u32=" << xHEX0N(ENDIAN_32NtoH(inU32s.at(inOutU32Ndx)),8) << " inU32s[" << DEC(inOutU32Ndx) << " of " << DEC(numU32s) << "] AncPktHdr: " << ancPktHeader << " -- AncDataLoc: " << dataLoc);

	if (++inOutU32Ndx >= numU32s)
		{RCV2110ERR("Index error: [" << DEC(inOutU32Ndx) << "] past end of [" << DEC(numU32s) << "] element buffer");  return AJA_STATUS_RANGE;}

	//	Set location info...
	AJAStatus result;
	result = SetLocationVideoLink(dataLoc.GetDataLink());
	if (AJA_FAILURE(result))	{RCV2110ERR("SetLocationVideoLink failed, dataLoc: " << dataLoc);	return result;}

	result = SetLocationDataStream(dataLoc.GetDataStream());
	if (AJA_FAILURE(result))	{RCV2110ERR("SetLocationDataStream failed, dataLoc: " << dataLoc);	return result;}

	result = SetLocationDataChannel(dataLoc.GetDataChannel());
	if (AJA_FAILURE(result))	{RCV2110ERR("SetLocationDataChannel failed, dataLoc: " << dataLoc);	return result;}

	result = SetLocationHorizOffset(dataLoc.GetHorizontalOffset());
	if (AJA_FAILURE(result))	{RCV2110ERR("SetLocationHorizOffset failed, dataLoc: " << dataLoc);	return result;}

	result = SetLocationLineNumber(dataLoc.GetLineNumber());
	if (AJA_FAILURE(result))	{RCV2110ERR("SetLocationLineNumber failed, dataLoc: " << dataLoc);	return result;}

	//	Unpack this anc packet...
	UWordSequence	u16s;	//	10-bit even-parity words
	bool			gotChecksum	(false);
	size_t			dataCount	(0);
	uint32_t		u32			(ENDIAN_32NtoH(inU32s.at(inOutU32Ndx)));
	const size_t	startU32Ndx	(inOutU32Ndx);
	RCV2110DDBG("u32=" << xHEX0N(u32,8) << " inU32s[" << DEC(inOutU32Ndx) << " of " << DEC(numU32s) << "]");
	do
	{
		uint16_t	u16	(0);
		for (unsigned loopNdx(0);  loopNdx < 20  &&  !gotChecksum;  loopNdx++)
		{
			const bool		is4th	((loopNdx & 3) == 3);
			const bool		is1st	((loopNdx & 3) == 0);
			const unsigned	shift	(gShifts[loopNdx]);
			const uint32_t	mask	(gMasks[loopNdx]);
			if (is4th)
			{
				u16 = uint16_t((u32 & mask) << shift);

				//	Grab next u32 value...
				if (++inOutU32Ndx >= numU32s)
				{
					u16s.push_back(u16);	//	RCV2110DDBG("u16s[" << DEC(u16s.size()-1) << "]=" << xHEX0N(u16,3) << " (Past end)");
					break;	//	Past end
				}
				u32 = ENDIAN_32NtoH(inU32s.at(inOutU32Ndx));
				RCV2110DDBG("u32=" << xHEX0N(u32,8) << " inU32s[" << DEC(inOutU32Ndx) << " of " << DEC(numU32s) << "], u16=" << xHEX0N(u16,3)
							<< " from " << DEC(loopNdx) << "(" << xHEX0N(inU32s.at(inOutU32Ndx-1),8) << " & " << xHEX0N(mask,8) << ") << " << DEC(shift));
				if (shift)
					continue;	//	go around
			}
			else if (is1st)
			{
//				RCV2110DDBG("u16s[" << DEC(u16s.size()) << "]=" << xHEX0N(u16,3) << " | " << xHEX0N(uint16_t((u32 & mask) >> shift),3)
//							<< " = " << xHEX0N(u16 | uint16_t((u32 & mask) >> shift),3));
				u16 |= uint16_t((u32 & mask) >> shift);
			}
			else
			{
				u16 = uint16_t((u32 & mask) >> shift);
//				RCV2110DDBG("u16s[" << DEC(u16s.size()) << "]=" << xHEX0N(u16,3));
			}
			u16s.push_back(u16);	//	RCV2110DDBG("u16s[" << DEC(u16s.size()-1) << "]=" << xHEX0N(u16,3));
			switch(u16s.size())
			{
				case 1:		SetDID(uint8_t(u16));				break;	//	Got DID
				case 2:		SetSID(uint8_t(u16));				break;	//	Got SID
				case 3:		dataCount = size_t(u16 & 0x0FF);	break;	//	Got DC
				default:	if (u16s.size() == (dataCount + 4))
								{gotChecksum = true; RCV2110DDBG("Got checksum, DC=" << xHEX0N(dataCount,2) << " CS=" << xHEX0N(u16s.back(),3));}	//	Got CS
							break;
			}
		}	//	loop 20 times (or until gotChecksum)
		if (gotChecksum)
				break;
	} while (inOutU32Ndx < numU32s);

	if (u16s.size() < 4)
	{	ostringstream oss;
		if (u16s.size() < 1) oss << " NoDID";
		else oss << " DID=" << xHEX0N(UWord(GetDID()),2);
		if (u16s.size() < 2) oss << " NoSID";
		else oss << " SID=" << xHEX0N(UWord(GetSID()),2);
		if (u16s.size() < 3) oss << " NoDC";
		else oss << " DC=" << DEC(dataCount);
		RCV2110ERR("Incomplete/bad packet:" << oss.str() << " NoCS" << " -- only unpacked " << u16s);
		return AJA_STATUS_FAIL;
	}
	RCV2110DBG("Consumed " << DEC(inOutU32Ndx - startU32Ndx + 1) << " ULWord(s), " << (gotChecksum?"":"NoCS, ") << "DC=" << DEC(dataCount) << ", unpacked " << u16s);
	if (inOutU32Ndx < numU32s)
		inOutU32Ndx++;	//	Bump to next Anc packet, if any

	//	Did we get the whole packet?
	if (dataCount > (u16s.size()-3))		//	DC > (u16s.size minus DID, SID, DC)?
	{	//	Uh-oh:  too few u16s -- someone's pulling our leg
		RCV2110ERR("Incomplete/bad packet: " << DEC(u16s.size()) << " U16s, but missing " << DEC(dataCount - (u16s.size()-3))
					<< " byte(s), expected DC=" << DEC(dataCount) << " -- DID=" << xHEX0N(UWord(GetDID()),2) << " SID=" << xHEX0N(UWord(GetSID()),2));
		return AJA_STATUS_FAIL;
	}

	//	Copy in the Anc packet data, while stripping off parity...
	for (size_t ndx(0);  ndx < dataCount;  ndx++)
		m_payload.push_back(uint8_t(u16s.at(ndx+3)));

	result = SetChecksum(uint8_t(u16s.at(u16s.size()-1)), true /*validate*/);
	if (AJA_FAILURE(result))
	{
		if (inIgnoreChecksum)
			{RCV2110WARN("SetChecksum=" << xHEX0N(u16s.at(u16s.size()-1),3) << " failed, calculated=" << xHEX0N(Calculate9BitChecksum(),3));  result = AJA_STATUS_SUCCESS;}
		else
			{RCV2110ERR("SetChecksum=" << xHEX0N(u16s.at(u16s.size()-1),3) << " failed, calculated=" << xHEX0N(Calculate9BitChecksum(),3));  return result;}
	}
	SetBufferFormat(AJAAncillaryBufferFormat_RTP);
	RCV2110DBG(AsString(64));

	/*	The Pattern:  (unrolling the above loop):
		u32 = ENDIAN_32NtoH(inU32s.at(0));
		u16  = uint16_t((u32 & 0xFFC00000) >> 22);		//	all 10 bits				0
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x003FF000) >> 12);		//	all 10 bits				1
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x00000FFC) >>  2);		//	all 10 bits				2
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x00000003) <<  8);		//	first (MS 2 bits)		3

		u32 = ENDIAN_32NtoH(inU32s.at(1));
		u16 |= uint16_t((u32 & 0xFF000000) >> 24);		//	last (LS 8 bits)		4
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x00FFC000) >> 14);		//	all 10 bits				5
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x00003FF0) >>  4);		//	all 10 bits				6
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x0000000F) <<  6);		//	first (MS 4 bits)		7

		u32 = ENDIAN_32NtoH(inU32s.at(2));
		u16 |= uint16_t((u32 & 0xFC000000) >> 26);		//	last (LS 6 bits)		8
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x03FF0000) >> 16);		//	all 10 bits				9
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x0000FFC0) >>  6);		//	all 10 bits				10
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x0000003F) <<  4);		//	first (MS 6 bits)		11

		u32 = ENDIAN_32NtoH(inU32s.at(3));
		u16 |= uint16_t((u32 & 0xF0000000) >> 28);		//	last (LS 4 bits)		12
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x0FFC0000) >> 18);		//	all 10 bits				13
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x0003FF00) >>  8);		//	all 10 bits				14
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x000000FF) <<  2);		//	first (MS 8 bits)		15

		u32 = ENDIAN_32NtoH(inU32s.at(4));
		u16 |= uint16_t((u32 & 0xC0000000) >> 30);		//	last (LS 2 bits)		16
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x3FF00000) >> 20);		//	all 10 bits				17
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x000FFC00) >> 10);		//	all 10 bits				18
		u16s.push_back(u16);
		u16  = uint16_t((u32 & 0x000003FF) >>  0);		//	all 10 bits				19
		u16s.push_back(u16);
*/
	return result;
}	//	InitWithReceivedData (const ULWordSequence&,uint16_t&)


static const string		gEmptyString;


const string & AJAAncillaryDataLinkToString (const AJAAncillaryDataLink inValue, const bool inCompact)
{
	static const string		gAncDataLinkToStr []			= {"A", "B", "?"};
	static const string		gDAncDataLinkToStr []			= {"AJAAncillaryDataLink_A", "AJAAncillaryDataLink_B", "AJAAncillaryDataLink_Unknown"};

	return IS_VALID_AJAAncillaryDataLink(inValue) ? (inCompact ? gAncDataLinkToStr[inValue] : gDAncDataLinkToStr[inValue]) : gAncDataLinkToStr[2];
}


const string &	AJAAncillaryDataStreamToString (const AJAAncillaryDataStream inValue, const bool inCompact)
{
	static const string		gAncDataStreamToStr []			= {"DS1", "DS2", "DS3", "DS4", "?"};
	static const string		gDAncDataStreamToStr []			= {"AJAAncillaryDataStream_1", "AJAAncillaryDataStream_2",
																"AJAAncillaryDataStream_3", "AJAAncillaryDataStream_4", "AJAAncillaryDataStream_Unknown"};

	return IS_VALID_AJAAncillaryDataStream(inValue) ? (inCompact ? gAncDataStreamToStr[inValue] : gDAncDataStreamToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataChannelToString (const AJAAncillaryDataChannel inValue, const bool inCompact)
{
	static const string		gAncDataChannelToStr []		= {"C", "Y", "?"};
	static const string		gDAncDataChannelToStr []	= {"AJAAncillaryDataChannel_C", "AJAAncillaryDataChannel_Y", "AJAAncillaryDataChannel_Unknown"};

	return IS_VALID_AJAAncillaryDataChannel(inValue) ? (inCompact ? gAncDataChannelToStr[inValue] : gDAncDataChannelToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataSpaceToString (const AJAAncillaryDataSpace inValue, const bool inCompact)
{
	static const string		gAncDataSpaceToStr []			= {"VANC", "HANC", "????"};
	static const string		gDAncDataSpaceToStr []			= {"AJAAncillaryDataSpace_VANC", "AJAAncillaryDataSpace_HANC", "AJAAncillaryDataSpace_Unknown"};

	return IS_VALID_AJAAncillaryDataSpace(inValue) ? (inCompact ? gAncDataSpaceToStr[inValue] : gDAncDataSpaceToStr[inValue]) : gEmptyString;
}


string AJAAncLineNumberToString (const uint16_t inValue)
{
	ostringstream	oss;
	if (inValue == AJAAncDataLineNumber_AnyVanc)
		oss	<< "VANC";
	else if (inValue == AJAAncDataLineNumber_Anywhere)
		oss	<< "UNSP";	//	Unspecified -- anywhere
	else if (inValue == AJAAncDataLineNumber_Future)
		oss	<< "OVFL";	//	Overflow
	else if (inValue == AJAAncDataLineNumber_Unknown)
		oss	<< "UNKN";
	else
		oss << "L" << DEC(inValue);
	return oss.str();
}


string AJAAncHorizOffsetToString (const uint16_t inValue)
{
	ostringstream	oss;
	if (inValue == AJAAncDataHorizOffset_AnyHanc)
		oss	<< "HANC";
	else if (inValue == AJAAncDataHorizOffset_AnyVanc)
		oss	<< "VANC";
	else if (inValue == AJAAncDataHorizOffset_Anywhere)
		oss	<< "UNSP";
	else if (inValue == AJAAncDataHorizOffset_Future)
		oss	<< "OVFL";
	else if (inValue == AJAAncDataHorizOffset_Unknown)
		oss	<< "UNKN";
	else
		oss << "+" << DEC(inValue);
	return oss.str();
}


ostream & AJAAncillaryDataLocation::Print (ostream & oss, const bool inCompact) const
{
	oss	<< ::AJAAncillaryDataLinkToString(GetDataLink(), inCompact)
		<< "|" << ::AJAAncillaryDataStreamToString(GetDataStream(), inCompact)
		<< "|" << ::AJAAncillaryDataChannelToString(GetDataChannel(), inCompact)
		<< "|" << ::AJAAncLineNumberToString(GetLineNumber())
		<< "|" << ::AJAAncHorizOffsetToString(GetHorizontalOffset());
	return oss;
}


string AJAAncillaryDataLocationToString (const AJAAncillaryDataLocation & inValue, const bool inCompact)
{
	ostringstream	oss;
	inValue.Print(oss, inCompact);
	return oss.str();
}

ostream & operator << (ostream & inOutStream, const AJAAncillaryDataLocation & inValue)
{
	return inValue.Print(inOutStream, true);
}


const string & AJAAncillaryDataCodingToString (const AJAAncillaryDataCoding inValue, const bool inCompact)
{
	static const string		gAncDataCodingToStr []			= {"Dig", "Ana", "???"};
	static const string		gDAncDataCodingToStr []			= {"AJAAncillaryDataCoding_Digital", "AJAAncillaryDataCoding_Raw", "AJAAncillaryDataCoding_Unknown"};

	return IS_VALID_AJAAncillaryDataCoding (inValue) ? (inCompact ? gAncDataCodingToStr[inValue] : gDAncDataCodingToStr[inValue]) : gEmptyString;
}


const string &	AJAAncillaryBufferFormatToString (const AJAAncillaryBufferFormat inValue, const bool inCompact)
{
	static const string		gAncBufFmtToStr []	= {"UNK", "FBVANC", "SDI", "RTP", ""};
	static const string		gDAncBufFmtToStr []	= {"AJAAncillaryBufferFormat_Unknown", "AJAAncillaryBufferFormat_FBVANC",
													"AJAAncillaryBufferFormat_SDI", "AJAAncillaryBufferFormat_RTP", ""};

	return IS_VALID_AJAAncillaryBufferFormat (inValue) ? (inCompact ? gAncBufFmtToStr[inValue] : gDAncBufFmtToStr[inValue]) : gEmptyString;
}


const string & AJAAncillaryDataTypeToString (const AJAAncillaryDataType inValue, const bool inCompact)
{
	static const string		gAncDataTypeToStr []			= {	"Unknown", "SMPTE 2016-3 AFD", "SMPTE 12-M RP188", "SMPTE 12-M VITC",
																"SMPTE 334 CEA708", "SMPTE 334 CEA608", "CEA608 Line21", "SMPTE 352 VPID",
																"SMPTE 2051 2 Frame Marker", "524D Frame Status", "5251 Frame Status",
																"HDR SDR", "HDR10", "HDR HLG", "?"};

	static const string		gDAncDataTypeToStr []			= {	"AJAAncillaryDataType_Unknown", "AJAAncillaryDataType_Smpte2016_3", "AJAAncillaryDataType_Timecode_ATC",
																"AJAAncillaryDataType_Timecode_VITC", "AJAAncillaryDataType_Cea708", "AJAAncillaryDataType_Cea608_Vanc",
																"AJAAncillaryDataType_Cea608_Line21", "AJAAncillaryDataType_Smpte352", "AJAAncillaryDataType_Smpte2051",
																"AJAAncillaryDataType_FrameStatusInfo524D", "AJAAncillaryDataType_FrameStatusInfo5251",
																"AJAAncillaryDataType_HDR_SDR", "AJAAncillaryDataType_HDR_HDR10", "AJAAncillaryDataType_HDR_HLG", "?"};

	return inValue < AJAAncillaryDataType_Size ? (inCompact ? gAncDataTypeToStr[inValue] : gDAncDataTypeToStr[inValue]) : gEmptyString;
}


ostream & AJAAncillaryData::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	inOutStream << "Type:\t\t"	<< AJAAncillaryData::DIDSIDToString(m_DID, m_SID)			<< endl
				<< "DID:\t\t"	<< xHEX0N(uint32_t(m_DID),2)								<< endl
				<< "SID:\t\t"	<< xHEX0N(uint32_t(m_SID),2)								<< endl
				<< "DC:\t\t"	<< DEC(GetDC())												<< endl
				<< "CS:\t\t"	<< xHEX0N(uint32_t(m_checksum),2)							<< endl
				<< "Loc:\t\t"	<< m_location												<< endl
				<< "Coding:\t\t"<< ::AJAAncillaryDataCodingToString(m_coding)				<< endl
				<< "Frame:\t\t"	<< xHEX0N(GetFrameID(),8)									<< endl
				<< "Format:\t\t"<< ::AJAAncillaryBufferFormatToString(GetBufferFormat())	<< endl
				<< "Valid:\t\t"	<< (GotValidReceiveData() ? "Yes" : "No");
	if (inDumpPayload)
		{inOutStream << endl;  DumpPayload (inOutStream);}
	return inOutStream;
}


string AJAAncillaryData::AsString (const uint16_t inMaxBytes) const
{
	ostringstream	oss;
	oss	<< "[" << ::AJAAncillaryDataCodingToString(GetDataCoding())
		<< "|" << ::AJAAncillaryDataLocationToString(GetDataLocation())
		<< "|" << GetDIDSIDPair() << "|CS" << HEX0N(uint16_t(GetChecksum()),2) << "|DC=" << DEC(GetDC());
	if (m_frameID)
		oss	<< "|FRx" << HEX0N(GetFrameID(),8);
	if (IS_KNOWN_AJAAncillaryBufferFormat(m_bufferFmt))
		oss	<< "|" << ::AJAAncillaryBufferFormatToString(GetBufferFormat());
	const string	typeStr	(AJAAncillaryData::DIDSIDToString(m_DID, m_SID));
	if (!typeStr.empty())
		oss << "|" << typeStr;
	oss << "]";
	if (inMaxBytes  &&  GetDC())
	{
		uint16_t	bytesToDump = uint16_t(GetDC());
		oss << ": ";
		if (inMaxBytes < bytesToDump)
			bytesToDump = inMaxBytes;
		for (uint16_t ndx(0);  ndx < bytesToDump;  ndx++)
			oss << HEX0N(uint16_t(m_payload[ndx]),2);
		if (bytesToDump < uint16_t(GetDC()))
			oss << "...";	//	Indicate dump was truncated
	}
	return oss.str();
}


ostream & operator << (ostream & inOutStream, const AJAAncillaryDIDSIDPair & inData)
{
	inOutStream << "x" << HEX0N(uint16_t(inData.first), 2) << "x" << HEX0N(uint16_t(inData.second), 2);
	return inOutStream;
}


ostream & AJAAncillaryData::DumpPayload (ostream & inOutStream) const
{
	if (IsEmpty())
		inOutStream	<< "(NULL payload)" << endl;
	else
	{
		const int32_t	kBytesPerLine	(32);
		uint32_t		count			(GetDC());
		const uint8_t *	pData			(GetPayloadData());

		while (count > 0)
		{
			const uint32_t	numBytes	((count >= kBytesPerLine) ? kBytesPerLine : count);
			inOutStream << ((count == GetDC()) ? "Payload:  " : "          ");
			for (uint8_t num(0);  num < numBytes;  num++)
			{
				inOutStream << " " << HEX0N(uint32_t(pData[num]),2);
				if (num % 4 == 3)
					inOutStream << " ";		// an extra space every four bytes for readability
			}
			inOutStream << endl;
			pData += numBytes;
			count -= numBytes;
		}	//	loop til break
	}
	return inOutStream;
}


AJAStatus AJAAncillaryData::Compare (const AJAAncillaryData & inRHS, const bool inIgnoreLocation, const bool inIgnoreChecksum) const
{
	if (GetDID() != inRHS.GetDID())
		return AJA_STATUS_FAIL;
	if (GetSID() != inRHS.GetSID())
		return AJA_STATUS_FAIL;
	if (GetDC() != inRHS.GetDC())
		return AJA_STATUS_FAIL;

	if (!inIgnoreChecksum)
		if (GetChecksum() != inRHS.GetChecksum())
			return AJA_STATUS_FAIL;
	if (!inIgnoreLocation)
		if (!(GetDataLocation() == inRHS.GetDataLocation()))
		return AJA_STATUS_FAIL;

	if (GetDataCoding() != inRHS.GetDataCoding())
		return AJA_STATUS_FAIL;

	if (!IsEmpty())
		if (m_payload != inRHS.m_payload)
			return AJA_STATUS_FAIL;

	return AJA_STATUS_SUCCESS;
}


string AJAAncillaryData::CompareWithInfo (const AJAAncillaryData & inRHS, const bool inIgnoreLocation, const bool inIgnoreChecksum) const
{
	ostringstream	oss;
	if (GetDID() != inRHS.GetDID())
		oss << "DID mismatch: " << xHEX0N(uint16_t(GetDID()),2) << " != " << xHEX0N(uint16_t(inRHS.GetDID()),2) << endl;
	if (GetSID() != inRHS.GetSID())
		oss << "SID mismatch: " << xHEX0N(uint16_t(GetSID()),2) << " != " << xHEX0N(uint16_t(inRHS.GetSID()),2) << endl;
	if (GetDC() != inRHS.GetDC())
		oss << "DC mismatch: " << xHEX0N(GetDC(),4) << " != " << xHEX0N(inRHS.GetDC(),4) << endl;

	if (!inIgnoreChecksum)
		if (GetChecksum() != inRHS.GetChecksum())
			oss << "CS mismatch: " << xHEX0N(uint16_t(GetChecksum()),2) << " != " << xHEX0N(uint16_t(inRHS.GetChecksum()),2) << endl;
	if (!inIgnoreLocation)
		if (!(GetDataLocation() == inRHS.GetDataLocation()))
			oss << "Location mismatch: " << GetDataLocation() << " != " << inRHS.GetDataLocation() << endl;

	if (GetDataCoding() != inRHS.GetDataCoding())
		oss << "DataCoding mismatch: " << AJAAncillaryDataCodingToString(GetDataCoding()) << " != " << AJAAncillaryDataCodingToString(inRHS.GetDataCoding()) << endl;

	if (!IsEmpty())
		if (::memcmp (GetPayloadData(), inRHS.GetPayloadData(), GetPayloadByteCount()) != 0)
			{oss << "LHS: "; DumpPayload(oss);	oss	<< "RHS: "; inRHS.DumpPayload(oss);}
	return oss.str();
}

AJAAncillaryData & AJAAncillaryData::operator = (const AJAAncillaryData & inRHS)
{
	if (this != &inRHS)
	{
		m_DID = inRHS.m_DID;
		m_SID = inRHS.m_SID;
		m_checksum = inRHS.m_checksum;
		m_location = inRHS.m_location;
		m_coding = inRHS.m_coding;
		m_payload = inRHS.m_payload;
		m_rcvDataValid = inRHS.m_rcvDataValid;
		m_ancType = inRHS.m_ancType;
		m_bufferFmt = inRHS.m_bufferFmt;
		m_frameID = inRHS.m_frameID;
		m_userData = inRHS.m_userData;
	}

	return *this;
}

bool AJAAncillaryData::operator == (const AJAAncillaryData & inRHS) const
{
	return AJA_SUCCESS(Compare(inRHS, false/*ignoreLocations=false*/, false/*ignoreChecksums=false*/));
}


//	Returns the original data byte in bits 7:0, plus even parity in bit 8 and ~bit 8 in bit 9
uint16_t AJAAncillaryData::AddEvenParity (const uint8_t inDataByte)
{
	// Parity Table
	static const uint16_t gEvenParityTable[256] =
	{
		/* 0-7 */		0x200,0x101,0x102,0x203,0x104,0x205,0x206,0x107,
		/* 8-15 */		0x108,0x209,0x20A,0x10B,0x20C,0x10D,0x10E,0x20F,
		/* 16-23 */		0x110,0x211,0x212,0x113,0x214,0x115,0x116,0x217,
		/* 24-31 */		0x218,0x119,0x11A,0x21B,0x11C,0x21D,0x21E,0x11F,
		/* 32-39 */		0x120,0x221,0x222,0x123,0x224,0x125,0x126,0x227,
		/* 40-47 */		0x228,0x129,0x12A,0x22B,0x12C,0x22D,0x22E,0x12F,
		/* 48-55 */		0x230,0x131,0x132,0x233,0x134,0x235,0x236,0x137,
		/* 56-63 */		0x138,0x239,0x23A,0x13B,0x23C,0x13D,0x13E,0x23F,
		/* 64-71 */		0x140,0x241,0x242,0x143,0x244,0x145,0x146,0x247,
		/* 72-79 */		0x248,0x149,0x14A,0x24B,0x14C,0x24D,0x24E,0x14F,
		/* 80-87 */		0x250,0x151,0x152,0x253,0x154,0x255,0x256,0x157,
		/* 88-95 */		0x158,0x259,0x25A,0x15B,0x25C,0x15D,0x15E,0x25F,
		/* 96-103 */	0x260,0x161,0x162,0x263,0x164,0x265,0x266,0x167,
		/* 104-111 */	0x168,0x269,0x26A,0x16B,0x26C,0x16D,0x16E,0x26F,
		/* 112-119 */	0x170,0x271,0x272,0x173,0x274,0x175,0x176,0x277,
		/* 120-127 */	0x278,0x179,0x17A,0x27B,0x17C,0x27D,0x27E,0x17F,
		/* 128-135 */	0x180,0x281,0x282,0x183,0x284,0x185,0x186,0x287,
		/* 136-143 */	0x288,0x189,0x18A,0x28B,0x18C,0x28D,0x28E,0x18F,
		/* 144-151 */	0x290,0x191,0x192,0x293,0x194,0x295,0x296,0x197,
		/* 152-159 */	0x198,0x299,0x29A,0x19B,0x29C,0x19D,0x19E,0x29F,
		/* 160-167 */	0x2A0,0x1A1,0x1A2,0x2A3,0x1A4,0x2A5,0x2A6,0x1A7,
		/* 168-175 */	0x1A8,0x2A9,0x2AA,0x1AB,0x2AC,0x1AD,0x1AE,0x2AF,
		/* 176-183 */	0x1B0,0x2B1,0x2B2,0x1B3,0x2B4,0x1B5,0x1B6,0x2B7,
		/* 184-191 */	0x2B8,0x1B9,0x1BA,0x2BB,0x1BC,0x2BD,0x2BE,0x1BF,
		/* 192-199 */	0x2C0,0x1C1,0x1C2,0x2C3,0x1C4,0x2C5,0x2C6,0x1C7,
		/* 200-207 */	0x1C8,0x2C9,0x2CA,0x1CB,0x2CC,0x1CD,0x1CE,0x2CF,
		/* 208-215 */	0x1D0,0x2D1,0x2D2,0x1D3,0x2D4,0x1D5,0x1D6,0x2D7,
		/* 216-223 */	0x2D8,0x1D9,0x1DA,0x2DB,0x1DC,0x2DD,0x2DE,0x1DF,
		/* 224-231 */	0x1E0,0x2E1,0x2E2,0x1E3,0x2E4,0x1E5,0x1E6,0x2E7,
		/* 232-239 */	0x2E8,0x1E9,0x1EA,0x2EB,0x1EC,0x2ED,0x2EE,0x1EF,
		/* 240-247 */	0x2F0,0x1F1,0x1F2,0x2F3,0x1F4,0x2F5,0x2F6,0x1F7,
		/* 248-255 */	0x1F8,0x2F9,0x2FA,0x1FB,0x2FC,0x1FD,0x1FE,0x2FF
	};	//	gEvenParityTable
	return gEvenParityTable[inDataByte];
}


string AncChannelSearchSelectToString (const AncChannelSearchSelect inSelect, const bool inCompact)
{
	switch (inSelect)
	{
		case AncChannelSearch_Y:	return inCompact ? "Y"   : "AncChannelSearch_Y";
		case AncChannelSearch_C:	return inCompact ? "C"   : "AncChannelSearch_C";
		case AncChannelSearch_Both:	return inCompact ? "Y+C" : "AncChannelSearch_Both";
		default:					break;
	}
	return "";
}


static bool CheckAncParityAndChecksum (const AJAAncillaryData::U16Packet &	inYUV16Line,
										const uint16_t		inStartIndex,
										const uint16_t		inTotalCount,
										const uint16_t		inIncrement = 2)
{
	bool	bErr	(false);
	UWord	ndx		(0);
	if (inIncrement == 0  ||  inIncrement > 2)
		return true;	//	Increment must be 1 or 2

	//	Check parity on all words...
	for (ndx = 3;  ndx < inTotalCount - 1;  ndx++)	//	Skip ANC Data Flags and Checksum
	{
		const UWord	wordValue	(inYUV16Line[inStartIndex + ndx * inIncrement]);
		if (wordValue != AJAAncillaryData::AddEvenParity(wordValue & 0xFF))
		{
			LOGMYERROR ("Parity error at word " << ndx << ":  got " << xHEX0N(wordValue,2)
						<< ", expected " << xHEX0N(AJAAncillaryData::AddEvenParity(wordValue & 0xFF),2));
			bErr = true;
		}
	}

	//	The checksum word is different: bit 8 is part of the checksum total, and bit 9 should be ~bit 8...
	const UWord	checksum	(inYUV16Line [inStartIndex + (inTotalCount - 1) * inIncrement]);
	const bool	b8			((checksum & BIT(8)) != 0);
	const bool	b9			((checksum & BIT(9)) != 0);
	if (b8 == b9)
	{
		LOGMYERROR ("Checksum word error:  got " << xHEX0N(checksum,2) << ", expected " << xHEX0N(checksum ^ 0x200, 2));
		bErr = true;
	}

	//	Check the checksum math...
	UWord	sum	(0);
	for (ndx = 3;  ndx < inTotalCount - 1;  ndx++)
		sum += inYUV16Line [inStartIndex + ndx * inIncrement] & 0x1FF;

	if ((sum & 0x1FF) != (checksum & 0x1FF))
	{
		LOGMYERROR ("Checksum math error:  got " << xHEX0N(checksum & 0x1FF, 2) << ", expected " << xHEX0N(sum & 0x1FF, 2));
		bErr = true;
	}

	return bErr;

}	//	CheckAncParityAndChecksum


bool AJAAncillaryData::GetAncPacketsFromVANCLine (const UWordSequence &				inYUV16Line,
													const AncChannelSearchSelect	inChanSelect,
													U16Packets &					outRawPackets,
													UWordSequence &					outWordOffsets)
{
	const UWord	wordCountMax	(UWord(inYUV16Line.size ()));

	//	If we're only looking at Y samples, start the search 1 word into the line...
	const UWord	searchOffset	(inChanSelect == AncChannelSearch_Y ? 1 : 0);

	//	If we're looking in both channels (e.g. SD video), increment search words by 1, else skip at every other word...
	const UWord	searchIncr		(inChanSelect == AncChannelSearch_Both ? 1 : 2);

	outRawPackets.clear();
	outWordOffsets.clear();

	if (!IS_VALID_AncChannelSearchSelect(inChanSelect))
		{LOGMYERROR("Bad search select value " << DEC(inChanSelect)); return false;}	//	bad search select
	if (wordCountMax < 12)
		{LOGMYERROR("UWordSequence size " << DEC(wordCountMax) << " too small"); return false;}	//	too small

	for (UWord wordNum = searchOffset;  wordNum < (wordCountMax - 12);  wordNum += searchIncr)
	{
		const UWord	ancHdr0	(inYUV16Line.at (wordNum + (0 * searchIncr)));	//	0x000
		const UWord	ancHdr1	(inYUV16Line.at (wordNum + (1 * searchIncr)));	//	0x3ff
		const UWord	ancHdr2	(inYUV16Line.at (wordNum + (2 * searchIncr)));	//	0x3ff
		const UWord	ancHdr3	(inYUV16Line.at (wordNum + (3 * searchIncr)));	//	DID
		const UWord	ancHdr4	(inYUV16Line.at (wordNum + (4 * searchIncr)));	//	SDID
		const UWord	ancHdr5	(inYUV16Line.at (wordNum + (5 * searchIncr)));	//	DC
		if (ancHdr0 == 0x000  &&  ancHdr1 == 0x3ff  &&  ancHdr2 == 0x3ff)
		{
			//	Total words in ANC packet: 6 header words + data count + checksum word...
			UWord	dataCount	(ancHdr5 & 0xFF);
			UWord	totalCount	(6  +  dataCount  +  1);

			if (totalCount > wordCountMax)
			{
				totalCount = wordCountMax;
				LOGMYERROR ("packet totalCount " << totalCount << " exceeds max " << wordCountMax);
				return false;
			}

			//	Be sure we don't go past the end of the line buffer...
			if (ULWord (wordNum + totalCount) >= wordCountMax)
			{
				LOGMYDEBUG ("past end of line: " << wordNum << " + " << totalCount << " >= " << wordCountMax);
				return false;	//	Past end of line buffer
			}

			if (CheckAncParityAndChecksum (inYUV16Line, wordNum, totalCount, searchIncr))
				return false;	//	Parity/Checksum error

			U16Packet	packet;
			for (unsigned i = 0;  i < totalCount;  i++)
				packet.push_back (inYUV16Line.at (wordNum + (i * searchIncr)));
			outRawPackets.push_back (packet);
			outWordOffsets.push_back (wordNum);

			LOGMYINFO ("Found ANC packet in " << ::AncChannelSearchSelectToString(inChanSelect)
							<< ": DID=" << xHEX0N(ancHdr3,4)
							<< " SDID=" << xHEX0N(ancHdr4,4)
							<< " word=" << wordNum
							<< " DC=" << ancHdr5
							<< " pix=" << (wordNum / searchIncr));
		}	//	if ANC packet found (wildcard or matching DID/SDID)
	}	//	scan the line

	return true;

}	//	GetAncPacketsFromVANCLine


bool AJAAncillaryData::Unpack8BitYCbCrToU16sVANCLine (const void * pInYUV8Line,
														UWordSequence & outU16YUVLine,
														const uint32_t inNumPixels)
{
	const UByte *	pInYUV8Buffer	(reinterpret_cast <const UByte *> (pInYUV8Line));
	const ULWord	maxOutElements	(inNumPixels * 2);

	outU16YUVLine.clear ();
	outU16YUVLine.reserve (maxOutElements);
	while (outU16YUVLine.size() < size_t(maxOutElements))
		outU16YUVLine.push_back(0);

	if (!pInYUV8Buffer)
		{LOGMYERROR("NULL/empty YUV8 buffer");  return false;}	//	NULL pointer
	if (inNumPixels < 12)
		{LOGMYERROR("width in pixels " << DEC(inNumPixels) << " too small (< 12)"); return false;}	//	Invalid width
	if (inNumPixels % 4)	//	6)?
		{LOGMYERROR("width in pixels " << DEC(inNumPixels) << " not multiple of 4"); return false;}	//	Width not evenly divisible by 4

	//	Since Y and C may have separate/independent ANC data going on, we're going to split the task and do all
	//	the even (C) samples first, the come back and repeat it for all of the odd (Y) samples...
	for (ULWord comp = 0;  comp < 2;  comp++)
	{
		bool	bNoMoreAnc	(false);	//	Assume all ANC packets (if any) begin at the first pixel and are contiguous
										//	(i.e. no gaps between Anc packets). Once a "gap" is seen, this flag is set,
										//	and the rest of the line turns into a copy.

		ULWord	ancCount	(0);		//	Number of bytes to shift and copy (once an ANC packet is found).
										//	0 == at the start of a potential new Anc packet.

		ULWord	pixNum		(0);		//	The current pixel being inspected (note: NOT the component or sample number!)
		UWord	checksum	(0);		//	Accumulator for checksum calculations

		while (pixNum < inNumPixels)
		{
			if (bNoMoreAnc)
			{	//	NoMoreAnc -- there's a gap in the Anc data -- which is assumed to mean there are no more packets on this line.
				//	Just do a simple 8-bit -> 10-bit expansion with the remaining data on the line...
				const ULWord	index		(2 * pixNum  +  comp);
				const UWord		dataValue	(UWord(UWord(pInYUV8Buffer[index]) << 2));	//	Pad 2 LSBs with zeros
				NTV2_ASSERT (index <= ULWord(outU16YUVLine.size()));
				if (index < ULWord(outU16YUVLine.size()))
					outU16YUVLine [index] = dataValue;
				else
					outU16YUVLine.push_back (dataValue);
				pixNum++;
			}
			else
			{	//	Still processing (possible) Anc data...
				if (ancCount == 0)
				{	//	AncCount == 0 means we're at the beginning of an Anc packet -- or not...
					if ((pixNum + 7) < inNumPixels)
					{	//	An Anc packet has to be at least 7 pixels long to be real...
						if (   pInYUV8Buffer [2*(pixNum+0) + comp] == 0x00
							&& pInYUV8Buffer [2*(pixNum+1) + comp] == 0xFF
							&& pInYUV8Buffer [2*(pixNum+2) + comp] == 0xFF)
						{	//	"00-FF-FF" means a new Anc packet is being started...
							//	Stuff "000-3FF-3FF" into the output buffer...
							outU16YUVLine [2*pixNum++  + comp] = 0x000;
							outU16YUVLine [2*pixNum++  + comp] = 0x3ff;
							outU16YUVLine [2*pixNum++  + comp] = 0x3ff;

							ancCount = pInYUV8Buffer[2*(pixNum+2) + comp] + 3 + 1;	//	Number of data words + DID + SID + DC + checksum
							checksum = 0;											//	Reset checksum accumulator
						}
						else
							bNoMoreAnc = true;	//	No anc here -- assume there's no more for the rest of the line
					}
					else
						bNoMoreAnc = true;	//	Not enough room for another anc packet here -- assume no more for the rest of the line
				}	//	if ancCount == 0
				else if (ancCount == 1)
				{	//	This is the last byte of an anc packet -- the checksum. Since the original conversion to 8 bits
					//	wiped out part of the original checksum, we've been recalculating it all along until now...
					outU16YUVLine [2*pixNum + comp]  = checksum & 0x1ff;			//	LS 9 bits of checksum
					outU16YUVLine [2*pixNum + comp] |= (~checksum & 0x100) << 1;	//	bit 9 = ~bit 8;

					pixNum++;
					ancCount--;
				}	//	else if end of valid Anc packet
				else
				{	//	ancCount > 0 means an Anc packet is being processed.
					//	Copy 8-bit data into LS 8 bits, add even parity to bit 8, and ~bit 8 to bit 9...
					const UByte	ancByte	(pInYUV8Buffer [2*pixNum + comp]);
					const UWord	ancWord	(AddEvenParity (ancByte));

					outU16YUVLine [2*pixNum + comp] = ancWord;

					checksum += (ancWord & 0x1ff);	//	Add LS 9 bits to checksum

					pixNum++;
					ancCount--;
				}	//	else copying actual Anc packet
			}	//	else processing an Anc packet
		}	//	for each pixel in the line
	}	//	for each Y/C component (channel)

	return true;
}	//	Unpack8BitYCbCrToU16sVANCLine


string AJAAncillaryData::DIDSIDToString (const uint8_t inDID, const uint8_t inSID)
{
	switch (inDID)
	{
		case 0x00:	return "SMPTE-291 Control Packet";
		case 0x08:	if (inSID == 0x08) return "SMPTE-291 Control Packet";
					break;
		case 0x40:	switch (inSID)
					{
						case 0x01:	return "RP-305 SDTI Header Data";
						case 0x02:	return "RP-348 HD-SDTI Header Data";
						case 0x04:	return "SMPTE-427 Link Encryp Key Msg 1";
						case 0x05:	return "SMPTE-427 Link Encryp Key Msg 2";
						case 0x06:	return "SMPTE-427 Link Encryp MetaD";
					}
					break;
		case 0x41:	switch (inSID)
					{
						case 0x01:	return "SMPTE-352M Payload ID";
						case 0x05:	return "SMPTE-2016-3 ADF/Bar Data";
						case 0x06:	return "SMPTE-2016-4 Pan & Scan Data";
						case 0x07:	return "SMPTE-2010 ANSI/SCTE 104 Msgs";
						case 0x08:	return "SMPTE-2031 DVB/SCTE VBI Data";
					}
					break;
		case 0x43:	switch (inSID)
					{
						case 0x01:	return "BT.1685 Inter-Station Ctrl Data";
						case 0x02:	return "RDD08/OP-47 Teletext Subtitling";
						case 0x03:	return "RDD08/OP-47 VANC Multipacket";
						case 0x04:	return "ARIB TR-B29 AV Sig Error Mon MetaD";
						case 0x05:	return "RDD18 Camera Params";
					}
					break;
		case 0x44:	if (inSID == 0x04 || inSID == 0x14)	return "RP-214 KLV Encoded MetaD & Essence";
					else if (inSID == 0x44)				return "RP-223 UMID & Prog ID Label Data";
					break;
		case 0x45:	if (inSID > 0  &&  inSID < 0x0A)	return "RP-2020 Compr/Dolby Aud MetaD";
					break;
	//	case 0x46:	if (inSID == 0x01)
		case 0x50:	if (inSID == 0x01)		return "RDD08 WSS Data";
					else if (inSID == 0x51)	return "CineLink-2 Link Encryp MetaD";
					break;
		case 0x51:	if (inSID == 0x01)		return "RP-215 Film Transfer Info";
					else if (inSID == 0x02)	return "RDD-18 Cam Param MetaD Set Acq";
					break;
		case 0x5F:	if (inSID == 0xDF)		return "ARIB STD-B37 HD Captions";
					else if (inSID == 0xDE)	return "ARIB STD-B37 SD Captions";
					else if (inSID == 0xDD)	return "ARIB STD-B37 Analog Captions";
					else if (inSID == 0xDC)	return "ARIB STD-B37 Mobile Captions";
					else if ((inSID & 0xF0) == 0xD0)	return "ARIB STD-B37 ??? Captions";
					return "ARIB STD-B37 ???";
		case 0x60:	if (inSID == 0x60)		return "SMPTE-12M ATC Timecode";
					break;
		case 0x61:	if (inSID == 0x01)		return "SMPTE-334 HD CEA-708 CC";
					else if (inSID == 0x02)	return "SMPTE-334 SD CEA-608 CC";
					break;
		case 0x62:	if (inSID == 0x01)		return "RP-207 DTV Program Desc";
					else if (inSID == 0x02)	return "SMPTE-334 Data Broadcast";
					else if (inSID == 0x03)	return "RP-208 VBI Data";
					break;
		case 0x64:	if (inSID == 0x64)		return "RP-196 LTC in HANC (Obs)";
					else if (inSID == 0x7F)	return "RP-196 VITC in HANC (Obs)";
					break;
		case 0x80:	return "SMPTE-291 Ctrl Pkt 'Marked for Deletion'";
		case 0x84:	return "SMPTE-291 Ctrl Pkt 'End Marker'";
		case 0x88:	return "SMPTE-291 Ctrl Pkt 'Start Marker'";
		case 0xA0:	return "SMPTE-299M 3G HD Aud Ctrl 8";
		case 0xA1:	return "SMPTE-299M 3G HD Aud Ctrl 7";
		case 0xA2:	return "SMPTE-299M 3G HD Aud Ctrl 6";
		case 0xA3:	return "SMPTE-299M 3G HD Aud Ctrl 5";
		case 0xA4:	return "SMPTE-299M 3G HD Aud Data 8";
		case 0xA5:	return "SMPTE-299M 3G HD Aud Data 7";
		case 0xA6:	return "SMPTE-299M 3G HD Aud Data 6";
		case 0xA7:	return "SMPTE-299M 3G HD Aud Data 5";
		case 0xD1:
		case 0xD2:	return "AJA QA F1 Test Packet";
		case 0xD3:	return "AJA QA F2 Test Packet";
		case 0xE0:	return "SMPTE-299M HD Aud Ctrl 4";
		case 0xE1:	return "SMPTE-299M HD Aud Ctrl 3";
		case 0xE2:	return "SMPTE-299M HD Aud Ctrl 2";
		case 0xE3:	return "SMPTE-299M HD Aud Ctrl 1";
		case 0xE4:	return "SMPTE-299M HD Aud Data 4";
		case 0xE5:	return "SMPTE-299M HD Aud Data 3";
		case 0xE6:	return "SMPTE-299M HD Aud Data 2";
		case 0xE7:	return "SMPTE-299M HD Aud Data 1";
		case 0xEC:	return "SMPTE-272M SD Aud Ctrl 4";
		case 0xED:	return "SMPTE-272M SD Aud Ctrl 3";
		case 0xEE:	return "SMPTE-272M SD Aud Ctrl 2";
		case 0xEF:	return "SMPTE-272M SD Aud Ctrl 1";
		case 0xF0:	return "SMPTE-315 Camera Position";
		case 0xF4:	return "RP-165 Error Detect/Checkwords";
		case 0xF8:	return "SMPTE-272M SD Aud Ext Data 4";
		case 0xF9:	return "SMPTE-272M SD Aud Data 4";
		case 0xFA:	return "SMPTE-272M SD Aud Ext Data 3";
		case 0xFB:	return "SMPTE-272M SD Aud Data 3";
		case 0xFC:	return "SMPTE-272M SD Aud Ext Data 2";
		case 0xFD:	return "SMPTE-272M SD Aud Data 2";
		case 0xFE:	return "SMPTE-272M SD Aud Ext Data 1";
		case 0xFF:	return "SMPTE-272M SD Aud Data 1";
	}
	return "";
}	//	DIDSID2String


////////////////////////////////////////////////////////////////////////////////////////////////////
//	AJARTPAncPayloadHeader


bool AJARTPAncPayloadHeader::BufferStartsWithRTPHeader (const NTV2_POINTER & inBuffer)
{
	if (inBuffer.IsNULL())
		return false;
	//	Peek in buffer and see if it starts with an RTP header...
	AJARTPAncPayloadHeader	hdr;
	if (!hdr.ReadFromBuffer(inBuffer))
		return false;
	return hdr.IsValid();
}

AJARTPAncPayloadHeader::AJARTPAncPayloadHeader()
	:	mVBits			(2),		//	Playout: don't care -- hardware sets this
		mPBit			(false),	//	Playout: don't care -- hardware sets this
		mXBit			(false),	//	Playout: don't care -- hardware sets this
		mMarkerBit		(false),	//	Playout: don't care -- hardware sets this
		mCCBits			(0),		//	Playout: don't care -- hardware sets this
		mPayloadType	(0),		//	Playout: don't care -- hardware sets this
		mSequenceNumber	(0),		//	Playout: don't care -- hardware sets this
		mTimeStamp		(0),		//	Playout: don't care -- hardware sets this
		mSyncSourceID	(0),		//	Playout: client sets this
		mPayloadLength	(0),		//	Playout: WriteIPAncData sets this
		mAncCount		(0),		//	Playout: WriteIPAncData sets this
		mFieldSignal	(0)			//	Playout: WriteIPAncData sets this
{
}

bool AJARTPAncPayloadHeader::GetPacketHeaderULWordForIndex (const unsigned inIndex0, uint32_t & outULWord) const
{
	switch (inIndex0)
	{
		case 0:
		{
			uint32_t u32(uint32_t(mVBits << 30));
			u32 |= uint32_t(mPBit ? 1 : 0) << 29;
			u32 |= uint32_t(mPBit ? 1 : 0) << 29;
			u32 |= uint32_t(mXBit ? 1 : 0) << 28;
			u32 |= uint32_t(mCCBits & 0x0000000F) << 24;
			u32 |= uint32_t(IsEndOfFieldOrFrame() ? 1 : 0) << 23;
			u32 |= uint32_t(GetPayloadType() & 0x0000007F) << 16;
			u32 |= uint32_t(GetSequenceNumber() & 0x0000FFFF);
			outULWord = ENDIAN_32HtoN(u32);
			break;
		}

		case 1:
			outULWord = ENDIAN_32HtoN(GetTimeStamp());
			break;

		case 2:
			outULWord = ENDIAN_32HtoN(GetSyncSourceID());
			break;

		case 3:
		{
			const uint32_t	u32	((GetSequenceNumber() & 0xFFFF0000) | (GetPayloadLength()));
			outULWord = ENDIAN_32HtoN(u32);
			break;
		}

		case 4:
		{
			uint32_t u32(uint32_t(GetAncPacketCount() & 0x000000FF) << 24);
			u32 |= uint32_t(GetFieldSignal() & 0x00000003) << 22;
			outULWord = ENDIAN_32HtoN(u32);
			break;
		}

		default:
			outULWord = 0;
			return false;
	}
	return true;
}

bool AJARTPAncPayloadHeader::SetFromPacketHeaderULWordAtIndex(const unsigned inIndex0, const uint32_t inULWord)
{
	const uint32_t	u32	(ENDIAN_32NtoH(inULWord));
	switch (inIndex0)
	{
		case 0:		mVBits = uint8_t(u32 >> 30);
					mPBit = (u32 & 0x20000000) ? true : false;
					mXBit = (u32 & 0x10000000) ? true : false;
					mCCBits = uint8_t((u32 & 0x0F000000) >> 24);
					mMarkerBit = (u32 & 0x00800000) ? true : false;
					mPayloadType = uint8_t((u32 & 0x007F0000) >> 16);
					mSequenceNumber = (mSequenceNumber & 0xFFFF0000) | (u32 & 0x0000FFFF);	//	Replace LS 16 bits
					break;

		case 1:		mTimeStamp = u32;
					break;

		case 2:		mSyncSourceID = u32;
					break;

		case 3:		mSequenceNumber = (u32 & 0xFFFF0000) | (mSequenceNumber & 0x0000FFFF);	//	Replace MS 16 bits
					mPayloadLength = uint16_t(u32 & 0x0000FFFF);
					break;

		case 4:		mAncCount = uint8_t((u32 & 0xFF000000) >> 24);
					mFieldSignal = uint8_t((u32 & 0x00C00000) >> 22);
					break;

		default:	return false;
	}
	return true;
}

bool AJARTPAncPayloadHeader::WriteToULWordVector (ULWordSequence & outVector, const bool inReset) const
{
	if (inReset)
		outVector.clear();
	while (outVector.size() < 5)
		outVector.push_back(0);
	for (unsigned ndx(0);  ndx < 5;  ndx++)
		outVector[ndx] = GetPacketHeaderULWordForIndex(ndx);
	return true;
}

bool AJARTPAncPayloadHeader::WriteToBuffer (NTV2_POINTER & outBuffer, const ULWord inU32Offset) const
{
	const ULWord	startingByteOffset	(inU32Offset * sizeof(uint32_t));
	if ((startingByteOffset + ULWord(GetHeaderByteCount())) > outBuffer.GetByteCount())
		return false;	//	Buffer too small
	uint32_t *	pU32s	(reinterpret_cast<uint32_t*>(outBuffer.GetHostAddress(startingByteOffset)));
	for (unsigned ndx(0);  ndx < 5;  ndx++)
		pU32s[ndx] = GetPacketHeaderULWordForIndex(ndx);
	return true;
}

bool AJARTPAncPayloadHeader::ReadFromULWordVector (const ULWordSequence & inVector)
{
	//	Note: uint32_t's are assumed to be in network byte order
	if (inVector.size() < 5)
		return false;	//	Too small
	for (unsigned ndx(0);  ndx < 5;  ndx++)
		if (!SetFromPacketHeaderULWordAtIndex(ndx, inVector[ndx]))
			return false;
	return true;
}

bool AJARTPAncPayloadHeader::ReadFromBuffer(const NTV2_POINTER & inBuffer)
{
	if (inBuffer.GetByteCount() < GetHeaderByteCount())
		return false;	//	Too small
	const uint32_t *	pU32s	(reinterpret_cast <const uint32_t *> (inBuffer.GetHostPointer()));
	for (unsigned ndx(0);  ndx < 5;  ndx++)
		if (!SetFromPacketHeaderULWordAtIndex(ndx, pU32s[ndx]))
			return false;
	return true;
}

const string & AJARTPAncPayloadHeader::FieldSignalToString (const uint8_t inFBits)
{
	static const string	sStrs[]	=	{	"p/noF",	"BAD",	"i/F1",	"i/F2"	};
	return sStrs[inFBits & 0x03];
}

bool AJARTPAncPayloadHeader::operator == (const AJARTPAncPayloadHeader & inRHS) const
{
	return	mVBits			== inRHS.mVBits
		&&	mPBit			== inRHS.mPBit
		&&  mXBit			== inRHS.mXBit
		&&  mCCBits			== inRHS.mCCBits
		&&  mMarkerBit		== inRHS.mMarkerBit
		&&  mPayloadType	== inRHS.mPayloadType
		&&  mSequenceNumber	== inRHS.mSequenceNumber
		&&  mTimeStamp		== inRHS.mTimeStamp
		&&  mSyncSourceID	== inRHS.mSyncSourceID
		&&  mPayloadLength	== inRHS.mPayloadLength
		&&  mAncCount		== inRHS.mAncCount
		&&  mFieldSignal	== inRHS.mFieldSignal;
}

ostream & AJARTPAncPayloadHeader::Print (ostream & inOutStream) const
{	//	Translate back to host byte order before printing:
	const uint32_t	word0	(ENDIAN_32NtoH(GetPacketHeaderULWordForIndex(0)));
	inOutStream << xHEX0N(word0,8)
//				<< "|" << bBIN032(word0)
				<< ": V=" << DEC(uint16_t(mVBits))
				<< " P=" << mPBit << " X=" << mXBit << " CC=" << DEC(uint16_t(mCCBits))
				<< " M=" << (IsEndOfFieldOrFrame()?"EOF":"0") << " PT=" << xHEX0N(uint16_t(GetPayloadType()),2)
				<< " Seq#=" << xHEX0N(GetSequenceNumber(),8) << " TS=" << xHEX0N(GetTimeStamp(),8)
				<< " SSRC=" << xHEX0N(GetSyncSourceID(),8) << " PayLen=" << DEC(GetPayloadLength())
				<< " AncCnt=" << DEC(uint16_t(GetAncPacketCount())) << " F=" << FieldSignalToString(GetFieldSignal())
				<< (IsValid() ? "" : " (invalid)");
	return inOutStream;
}

bool AJARTPAncPayloadHeader::IsNULL(void) const
{	//	Return TRUE if all of my fields are zero...
	return !(mVBits || mPBit || mXBit || mCCBits || mMarkerBit || mPayloadType || mSequenceNumber || mTimeStamp || mSyncSourceID || mPayloadLength || mAncCount || mFieldSignal);
}

bool AJARTPAncPayloadHeader::IsValid(void) const
{	//	Simple validation -- Check that...
	return	mVBits == 0x02			//	Version == 2
		&&	!IsNULL()				//	Not NULL
		&&  IsValidFieldSignal();	//	Valid field bits
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//	AJARTPAncPacketHeader

AJARTPAncPacketHeader::AJARTPAncPacketHeader()
	:	mCBit	(false),
		mSBit	(false),
		mLineNum	(0),
		mHOffset	(0),
		mStreamNum	(0)
{
}

AJARTPAncPacketHeader::AJARTPAncPacketHeader(const AJAAncillaryDataLocation & inLoc)
	:	mCBit	(false),
		mSBit	(false),
		mLineNum	(0),
		mHOffset	(0),
		mStreamNum	(0)
{
	SetFrom(inLoc);
}

uint32_t AJARTPAncPacketHeader::GetULWord (void) const
{
	//	In network byte order:
	//
	//	 0                   1                   2                   3
	//	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
	//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	//	|C|   Line_Number       |   Horizontal_Offset   |S|  StreamNum  |
	//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	uint32_t	u32(0);	//	Build little-endian 32-bit ULWord with this
	u32 |= (uint32_t(GetLineNumber()) & 0x000007FF) << 20;	//	11-bit line number
	u32 |= (IsCBitSet() ? 0x80000000 : 0x00000000);
	u32 |= (uint32_t(GetHorizOffset()) & 0x00000FFF) << 8;	//	12-bit horiz offset
	u32 |= IsSBitSet() ? 0x00000080 : 0x00000000;			//	Data Stream flag
	if (IsSBitSet())
		u32 |= uint32_t(GetStreamNumber() & 0x7F);			//	7-bit StreamNum
	return ENDIAN_32HtoN(u32);	//	Convert to big-endian
}

bool AJARTPAncPacketHeader::SetFromULWord (const uint32_t inULWord)
{
	const uint32_t	u32	(ENDIAN_32NtoH(inULWord));
	//	 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
	//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	//	|C|   Line_Number       |   Horizontal_Offset   |S|  StreamNum  |
	//	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	if (u32 & 0x80000000)
		SetCChannel();
	else
		SetYChannel();
	SetLineNumber(uint16_t((u32 & 0x7FF00000) >> 20));
	SetHorizOffset(uint16_t((u32 & 0x000FFF00) >> 8));
	SetStreamNumber(uint8_t(u32 & 0x0000007F));
	SetDataStreamFlag(u32 & 0x00000080);
	return true;
}

bool AJARTPAncPacketHeader::WriteToULWordVector (ULWordSequence & outVector, const bool inReset) const
{
	if (inReset)
		outVector.clear();
	outVector.push_back(GetULWord());
	return true;
}

bool AJARTPAncPacketHeader::ReadFromULWordVector (const ULWordSequence & inVector, const unsigned inIndex0)
{
	if (inIndex0 >= inVector.size())
		return false;	//	Bad index -- out of range
	return SetFromULWord(inVector[inIndex0]);
}

AJAAncillaryDataLocation AJARTPAncPacketHeader::AsDataLocation(void) const
{
	AJAAncillaryDataLocation	result;
	result.SetLineNumber(GetLineNumber()).SetHorizontalOffset(GetHorizOffset())
			.SetDataChannel(IsCBitSet() ? AJAAncillaryDataChannel_C : AJAAncillaryDataChannel_Y)
			.SetDataLink(AJAAncillaryDataLink_A).SetDataStream(AJAAncillaryDataStream_1);
	if (IsSBitSet())
	{
		//	e.g. SMPTE ST 425-3 DL 3Gbps, data streams are numbered 1-4, so DS1/DS2 on linkA, DS3/DS4 on linkB
		result.SetDataStream(AJAAncillaryDataStream(GetStreamNumber()));
		if (IS_LINKB_AJAAncillaryDataStream(result.GetDataStream()))
			result.SetDataLink(AJAAncillaryDataLink_B);
	}
	return result;
}

AJARTPAncPacketHeader &	AJARTPAncPacketHeader::SetFrom(const AJAAncillaryDataLocation & inLoc)
{
	const AJAAncillaryDataLink		lnk		(inLoc.GetDataLink());
	const AJAAncillaryDataStream	ds		(inLoc.GetDataStream());
	const AJAAncillaryDataChannel	dChan	(inLoc.GetDataChannel());

	//	@bug	Dang, the sense of the C bit is opposite of our AJAAncillaryDataChannel enum!
	//			The C bit should be '1' for AJAAncillaryDataChannel_C (0).
	//			The C bit should be '0' for AJAAncillaryDataChannel_Y (1).
	//			The C bit should be '0' for SD, but we use AJAAncillaryDataChannel_Both (0) for this,
	//			which is indistinguishable from AJAAncillaryDataChannel_C (0).
	//	@todo	This needs to be addressed.
	mCBit = IS_VALID_AJAAncillaryDataChannel(dChan)  &&  (dChan != AJAAncillaryDataChannel_Y);

	//	Data Stream Flag
	//		'0':	No guidance -- don't know -- don't care
	//		'1':	StreamNum field carries info about source data stream number
	mSBit = IS_VALID_AJAAncillaryDataLink(lnk)  ||  IS_VALID_AJAAncillaryDataStream(ds);

	if (IS_VALID_AJAAncillaryDataLink(lnk))
		mStreamNum = uint8_t(lnk);
	else if (IS_VALID_AJAAncillaryDataStream(ds))
		mStreamNum = uint8_t(ds);
	else
		mStreamNum = 0;

	mLineNum = inLoc.GetLineNumber();
	mHOffset = inLoc.GetHorizontalOffset();

	return *this;
}

ostream & AJARTPAncPacketHeader::Print (ostream & inOutStream) const
{
	inOutStream << xHEX0N(GetULWord(),8) << ": C=" << (IsCBitSet() ? "1" : "0")
				<< " Line=" << DEC(GetLineNumber()) << " HOff=" << DEC(GetHorizOffset())
				<< " S=" << (IsSBitSet() ? "1" : "0") << " Strm=" << DEC(uint16_t(GetStreamNumber()));
	return inOutStream;
}
