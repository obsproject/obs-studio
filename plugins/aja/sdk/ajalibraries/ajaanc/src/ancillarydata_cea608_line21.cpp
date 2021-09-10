/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608_line21.cpp
	@brief		Implements the AJAAncillaryData_Cea608_line21 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_cea608_line21.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Cea608_Line21::AJAAncillaryData_Cea608_Line21() : AJAAncillaryData_Cea608()
{
	Init();
}


AJAAncillaryData_Cea608_Line21::AJAAncillaryData_Cea608_Line21(const AJAAncillaryData_Cea608_Line21& clone) : AJAAncillaryData_Cea608()
{
	Init();

	*this = clone;
}


AJAAncillaryData_Cea608_Line21::AJAAncillaryData_Cea608_Line21(const AJAAncillaryData_Cea608_Line21 *pClone) : AJAAncillaryData_Cea608()
{
	Init();

	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Cea608_Line21::AJAAncillaryData_Cea608_Line21(const AJAAncillaryData *pData) : AJAAncillaryData_Cea608(pData)
{
	Init();
}


AJAAncillaryData_Cea608_Line21::~AJAAncillaryData_Cea608_Line21()
{
}


void AJAAncillaryData_Cea608_Line21::Init (void)
{
	m_ancType = AJAAncillaryDataType_Cea608_Line21;
	m_coding  = AJAAncillaryDataCoding_Analog;
	m_DID	  = AJAAncillaryData_Cea608_Line21_DID;
	m_SID	  = AJAAncillaryData_Cea608_Line21_SID;
	m_bEncodeBufferInitialized = false;
	m_dataStartOffset = 0;
	SetLocationLineNumber(21);	//	Assume F1	otherwise SetLocationLineNumber(284);
}


AJAAncillaryData_Cea608_Line21 & AJAAncillaryData_Cea608_Line21::operator = (const AJAAncillaryData_Cea608_Line21 & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		AJAAncillaryData_Cea608::operator= (rhs);		// copy the base class stuff

		// copy the local stuff
		m_bEncodeBufferInitialized = rhs.m_bEncodeBufferInitialized;
		m_dataStartOffset = rhs.m_dataStartOffset;
	}
	return *this;
}


void AJAAncillaryData_Cea608_Line21::Clear (void)
{
	AJAAncillaryData_Cea608::Clear();
	Init();
}


AJAStatus AJAAncillaryData_Cea608_Line21::ParsePayloadData (void)
{
	if (IsEmpty())// || m_DC != AJAAncillaryData_Cea608_Line21_PayloadSize)
	{
		Init();						// load default values
		return AJA_STATUS_FAIL;
	}

	// we have some kind of payload data - try to parse it
	uint8_t char1(0), char2(0);
	bool	bGotClock	(false);
	m_rcvDataValid = false;
	AJAStatus status = DecodeLine (char1, char2, bGotClock);
	if (AJA_SUCCESS(status) && bGotClock)
	{
		m_char1 = char1;
		m_char2 = char2;
		m_rcvDataValid = true;
	}
	return status;
}


const uint32_t CC_DEFAULT_ENCODE_OFFSET = 7;	// number of pixels between start of buffer and beginning of first CC bit-cell

AJAStatus AJAAncillaryData_Cea608_Line21::GeneratePayloadData (void)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	m_DID = AJAAncillaryData_Cea608_Line21_DID;
	m_SID = AJAAncillaryData_Cea608_Line21_SID;

	// the first time through we're going to allocate the payload data and initialize the "unchanging" pixels
	// (run-in clock, etc.). After that we're going to assume: (a) the payload size never changes; and (b) the
	// unchanging bits don't change.
	if (!m_bEncodeBufferInitialized || GetDC() != AJAAncillaryData_Cea608_Line21_PayloadSize || m_dataStartOffset == 0)
		status = AllocEncodeBuffer();

	// encode the payload data
	if (AJA_SUCCESS(status))
		status = EncodeLine (m_char1, m_char2, m_dataStartOffset);

	// note: we're not going to bother with a checksum since it's tedious and the hardware ignores it anyway...
	return status;
}


AJAStatus AJAAncillaryData_Cea608_Line21::AllocEncodeBuffer (void)
{
	AJAStatus status = AllocDataMemory (AJAAncillaryData_Cea608_Line21_PayloadSize);
	if (AJA_SUCCESS(status))
	{
		// init the buffer with all of the "static" stuff (run-in clock, pre- and post- black, etc.)
		status = InitEncodeBuffer (CC_DEFAULT_ENCODE_OFFSET, m_dataStartOffset);
		if (AJA_SUCCESS(status))
			m_bEncodeBufferInitialized = true;
	}
	return status;
}


AJAAncillaryDataType AJAAncillaryData_Cea608_Line21::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	// note: BIG ASSUMPTION! If the user deliberately captured analog line 21 on either field,
	//       we're assuming it was for the sake of getting captioning data (NTSC/525-line).
	//       The only way we could know "for sure" would be to run ParsePayloadData() on
	//       the payload data, but that's not a static method so you'd have to recast the
	//		 AJAAncillaryData object anyway!
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Analog)
		if (pInAncData->GetLocationLineNumber() == 21 || pInAncData->GetLocationLineNumber() == 284)
			return AJAAncillaryDataType_Cea608_Line21;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_Cea608_Line21::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncillaryDataCodingToString (m_coding) << ")" << endl;
	return AJAAncillaryData_Cea608::Print (debugStream, bShowDetail);

}


//-------------------------------------------------------------
// Line 21 Encode code (ported/stolen from ntv2closedcaptioning.cpp)


//--------------------------------------------------------
// Encode

// This module can be used to encode two ASCII characters to a line 21 closed captioning
// waveform, or conversely decode a line 21 waveform to the two characters it contains.
// This is NOT a general module: it assumes a 720-pixel line (e.g. Standard Definition "NTSC"),
// and that the closed captioning is represented by an EIA-608 waveform.

// This implementation only handles conversion to/from 8-bit uncompressed ('2vuy') video.

// NOTE: this implementation makes a simplifying assumption that all captioning bits
// are 27 pixels wide. This is a cheat: the actual bit duration should be (H / 32), which
// for NTSC video is 858 pixels / 32 = 26.8125 pixels per bit. This difference should be
// within the tolerance of most captioning receivers, but beware - we may find that in
// practice we need more precision someday. (Note that in PAL the line width is 864 pixels,
// which is exactly 27.0 pixels/bit).

const uint8_t CC_BIT_WIDTH	 =  27;		// = round(858 / 32)
const uint8_t CC_LEVEL_LO	 =  16;		// '0' bit level
const uint8_t CC_LEVEL_HI	 = 126;		// '1' bit level
const uint8_t CC_LEVEL_MID	 =  71;		// "slice" level - mid-way between '0' and '1'
const uint8_t CC_LEVEL_TRANS =  20;


// Funny math: there are exactly 7 cycles of Clock Run-In, which is an inverted cosine wave.
// Each cycle is the same duration as one CC bit (27 pixels). HOWEVER, the CC bit edges are
// supposed to be coincident with the 50% point of the trailing edge of each clock cycle
// (think of it as 270 degrees through the cycle), while the actual clock run-in cycles are
// generated from 0 degrees to 0 degrees. So the first clock cycle begins 90 degrees into the
// first "bit", and the last clock cycle draws its last 90 degrees overlapping into the first
// zero start bit. This value is the "length" of this 90 degree offset in pixels.
const uint32_t CLOCK_RUN_IN_OFFSET = 7;		// number of pixels in 90 degrees

// the transition time between low and high ("rise-time") and high to low ("fall time") is
// three samples. Since bit cells start and end at the 50% points, some of the transition
// samples belong to the previous bit, and some belong to the next bit.
const uint32_t TRANSITION_PRE	= 1;		// one sample of the transition "belongs" to the previous bit
const uint32_t TRANSITION_POST  = 2;		// two samples of the transition "belong" to the next bit
const uint32_t TRANSITION_WIDTH = (TRANSITION_PRE + TRANSITION_POST);


// Initialize a prototype Line 21 buffer with the parts that DON'T change:
// i.e. the Clock Run-In and the Start bits.
AJAStatus AJAAncillaryData_Cea608_Line21::InitEncodeBuffer (uint32_t lineStartOffset, uint32_t & dataStartOffset)
{
	// 1 cycle of the Line 21 Clock Run-In (see note in header file about freq. approximation)
	static const uint8_t cc_clock[27] = { 16,  17,  22,  29,  38,  49,  61,  74,  86,  98, 108, 116, 122, 125,
										 125, 122, 116, 108,  98,  86,  74,  61,  49,  38,  29,  22,  17      };

	// sanity check...
	if (GetDC() < AJAAncillaryData_Cea608_Line21_PayloadSize)
		return AJA_STATUS_FAIL;

	AJAStatus status = AJA_STATUS_SUCCESS;

	uint32_t i, j;
	ByteVectorIndex	pos(0);
	const uint8_t	kSMPTE_Y_Black	(0x10);

	// fill Black until beginning of Clock Run-In
	// both the user-supplied offset to the first bit-cell, plus the "missing" quarter-cycle of the clock
	for (i = 0;  i < (lineStartOffset + CLOCK_RUN_IN_OFFSET);  i++)
		m_payload[pos++] = kSMPTE_Y_Black;	// Note: assuming SMPTE "Y" black!

	// 7 cycles of 503,496 Hz clock run-in
	for (j = 0;  j < 7;  j++)
		for (i = 0;  i < CC_BIT_WIDTH;  i++)
			m_payload[pos++] = cc_clock[i];	// clock

	// Start bit: 1 CC bits of '0' (reduced width because the last cycle of the clock run-in overlaps)
	for (i = 0; i < (CC_BIT_WIDTH - CLOCK_RUN_IN_OFFSET); i++)
		m_payload[pos++] = CC_LEVEL_LO;

	// Start bit: 1 CC bit of '0' (full width)
	for (i = 0;  i < CC_BIT_WIDTH - TRANSITION_POST;  i++)
		m_payload[pos++] = CC_LEVEL_LO;

	// encode transition between low and high
	EncodeTransition (&m_payload[pos], 0, 1);
	pos += TRANSITION_WIDTH;

	// Start bit: 1 CC bit of '1'
	for (i = 0; i < CC_BIT_WIDTH - TRANSITION_PRE; i++)
		m_payload[pos++] = CC_LEVEL_HI;

	//	Fill in black for the rest of the buffer - this will be overwritten with "real" data bits later
	while (pos < GetDC())
		m_payload[pos++] = kSMPTE_Y_Black;		// Note: assuming SMPTE "Y" black!

	// the first data bit cell starts 10 bit cells after the initial offset
	// (note: does NOT include slop for needed rise-time)
	dataStartOffset = lineStartOffset + (10 * CC_BIT_WIDTH);

	return status;
}


// encodes the supplied two bytes into the existing encode buffer and returns a pointer to same
// note: caller should NOT modify the returned buffer in any way.
AJAStatus AJAAncillaryData_Cea608_Line21::EncodeLine (uint8_t char1, uint8_t char2, uint32_t dataStartOffset)
{
	// pointer to first data bit, minus room for transition
	uint8_t *ptr = &m_payload[0] + (dataStartOffset - TRANSITION_PRE);

	// encode transition from last start bit to first bit of first character
	ptr = EncodeTransition (ptr, 1, (char1 & 0x01) );

	// encode first byte
	ptr = EncodeCharacter (ptr, char1);

	// encode transition between characters
	ptr = EncodeTransition (ptr, (char1 & 0x80), (char2 & 0x01) );

	// encode second byte
	ptr = EncodeCharacter (ptr, char2);

	// encode final transition
	ptr = EncodeTransition (ptr, (char2 & 0x80), 0);

	// return ptr to the beginning of the encode buffer
	return AJA_STATUS_SUCCESS;
}


// encodes from the end of the transition to the first bit until the start of the transition from the last bit
// returns the pointer to the next pixel following the character (i.e. the beginning of the transition to the next character)
// Note: the ms bit is supposed to be odd parity for the ls 7 bits. It is up to the caller to make this so.
uint8_t * AJAAncillaryData_Cea608_Line21::EncodeCharacter (uint8_t * ptr, uint8_t byte)
{
	uint8_t mask = 1;

	// do all 8 bits
	for (uint8_t j = 0; j < 8; j++)
	{
		// do the constant samples
		uint8_t level = ((byte & mask) == 0) ? CC_LEVEL_LO : CC_LEVEL_HI;
		
		for (uint32_t i = 0; i < (CC_BIT_WIDTH - TRANSITION_WIDTH); i++)
			*ptr++ = level;	
		
		// do the transition (except following the last bit)
		uint8_t nextMask = mask << 1;
		
		if (j < 7)
			ptr = EncodeTransition(ptr, (byte & mask), (byte & nextMask) );
			
		mask = nextMask;
	}
	return ptr;
}

// encodes from the beginning of the transition of one bit ("inStartLevel") to the next ("inEndLevel")
// returns the pointer to the next pixel following the transition
uint8_t * AJAAncillaryData_Cea608_Line21::EncodeTransition (uint8_t * ptr, const uint8_t inStartLevel, const uint8_t inEndLevel)
{
	// 4 possible transition types:
	static const uint8_t cc_trans_lo_lo[TRANSITION_WIDTH] = { CC_LEVEL_LO, CC_LEVEL_LO, CC_LEVEL_LO };	// Low -> Low
	static const uint8_t cc_trans_lo_hi[TRANSITION_WIDTH] = { CC_LEVEL_LO+CC_LEVEL_TRANS, CC_LEVEL_MID, CC_LEVEL_HI-CC_LEVEL_TRANS };	// Low -> High
	static const uint8_t cc_trans_hi_lo[TRANSITION_WIDTH] = { CC_LEVEL_HI-CC_LEVEL_TRANS, CC_LEVEL_MID, CC_LEVEL_LO+CC_LEVEL_TRANS };	// High -> Low
	static const uint8_t cc_trans_hi_hi[TRANSITION_WIDTH] = { CC_LEVEL_HI, CC_LEVEL_HI, CC_LEVEL_HI };	// High -> High

	const uint8_t *	pTrans	(NULL);

	if (inStartLevel == 0  &&  inEndLevel == 0)
		pTrans = cc_trans_lo_lo;
	else if (inStartLevel == 0  &&  inEndLevel != 0)
		pTrans = cc_trans_lo_hi;
	else if (inStartLevel != 0  &&  inEndLevel == 0)
		pTrans = cc_trans_hi_lo;
	else
		pTrans = cc_trans_hi_hi;

	for (uint32_t i = 0;  i < TRANSITION_WIDTH;  i++)
		*ptr++ = pTrans[i];

	return ptr;
}


//--------------------------------------------------------
// Decode

// Decodes the supplied line of 8-bit uncompressed ('2vuy') data and, if successful, returns the
// two 8-bit characters in *pChar1 (first character) and *pChar2 (second character).
// Note: this routine does not try to analyze the characters in any way - including trying to
// do parity checks. If desired, that is the responsibility of the caller. This routine WILL
// try to make a reasonable attempt to discover whether captioning data is present or not,
// and if it decides that the line does NOT contain encoded captioning info, will return 'false'
// and set the characters to 0.
AJAStatus AJAAncillaryData_Cea608_Line21::DecodeLine (uint8_t & outChar1, uint8_t & outChar2, bool & outGotClock) const
{
	outChar1 = outChar2 = 0xFF;
	outGotClock = false;
	if (GetDC() < AJAAncillaryData_Cea608_Line21_PayloadSize)
		return AJA_STATUS_FAIL;

	// see if the line contains a captioning clock run-in, signifying a valid captioning line
	// If successful, CheckDecodeClock will return a pointer to the middle of the first data
	// bit (i.e. the one following the last '1' start bit) and will set outGotClock to 'true'.
	// If CheckDecodeClock cannot find a valid clock run-in, it will return with outGotClock
	// set to 'false'.
	const uint8_t * pFirstDataBit = CheckDecodeClock (GetPayloadData(), outGotClock);
	if (outGotClock)
		return DecodeCharacters(pFirstDataBit, outChar1, outChar2);		// decode characters

	// note: we're NOT returning an error if no clock was found. In fact, this is a fairly
	// normal happening (a lot of programs are not captioned), so the caller should be sure
	// to check bGotClock upon return to make sure the returned characters are valid.
	return AJA_STATUS_SUCCESS;
}


const uint8_t * AJAAncillaryData_Cea608_Line21::CheckDecodeClock (const uint8_t * pInLine, bool & bGotClock)
{	
	bGotClock = false;		// assume the worst...
	if (!pInLine)
		return NULL;
	
	const uint8_t *pFirstYSample = pInLine;		// point to first pixel in line
	const uint8_t *pFirstClockEdge	= NULL;
	const uint8_t *pLastClockEdge	= NULL;
	const uint8_t *pFirstDataBit = pFirstYSample;
	
	// The rising edge of the first clock run-in cycle should happen approx 10.5 usecs from the leading
	// edge of sync, which translates to approx. 20 pixels from the left-hand edge of active video.
	// However, the tolerance on this value is +/- 500 nsec, or +/- 7 pixels. So we need to programatically
	// find the first leading clock edge and use that as our reference.
		
	// Start looking for a low->high transition starting at pixel 10, give up if we haven't found it by pixel 30.
	uint32_t pos(0);
	uint32_t startSearch = 10;
	uint32_t stopSearch  = 30;
	for (pos = startSearch;  pos < stopSearch;  pos++)
		if (pFirstYSample[pos] < CC_LEVEL_MID  &&  pFirstYSample[pos+1] >= CC_LEVEL_MID)
			break;

	// did we find it?
	if (pos < stopSearch)
	{
		pFirstClockEdge = &pFirstYSample[pos];
//		cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: found first clock edge at pixel " << i << endl;
		
		// if this is the leading edge of the first sine wave, then the crest of this wave will
		// be approx 7 pixels from here, and the trough of this wave will be approx 13 clocks after that.
		// This pattern will repeat every 27 pixels for 7 cycles.
		for (pos = 0;  pos < 7;  pos++)
		{
			uint32_t hi = (pos * CC_BIT_WIDTH) + 7;
			uint32_t lo = (pos * CC_BIT_WIDTH) + 20;
			
			if ( (pFirstClockEdge[hi] < CC_LEVEL_MID) || (pFirstClockEdge[lo] >= CC_LEVEL_MID) )
				break;
		}

		if (pos < 7)
		{
			//	Failed to find an expected clock crest or trough - abort
//			cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: failed clock run-in test at cycle " << pos << endl;
		}
		else
		{
//			cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: confirmed 7 cycles of clock run-in" << endl;

			// the 7 cycles of clock looks OK, now let's specifically find the leading edge
			// of the last clock. This will serve as our reference sample point for the data bits
			startSearch = (5 * CC_BIT_WIDTH) + 20;		// the lo point of the 6th cycle
			stopSearch  = (6 * CC_BIT_WIDTH) +  7;		// the hi point of the 7th cycle
			for (pos = startSearch;  pos < stopSearch;  pos++)
			{
				if ( (pFirstClockEdge[pos] < CC_LEVEL_MID) && (pFirstClockEdge[pos+1] >= CC_LEVEL_MID) )
					break;
			}
			
			// the mid-point of each sample bit will occur on bit-cell multiples from here
			pLastClockEdge = &pFirstClockEdge[pos+1];
//			cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: leading edge of last clock cycle at pixel " << uint64_t(pLastClockEdge - pFirstYSample) << endl;
			
			// the next three bit cells are the start bits, which should be 0, 0, 1
			if (   (pLastClockEdge[(CC_BIT_WIDTH * 1)] <  CC_LEVEL_MID)
				&& (pLastClockEdge[(CC_BIT_WIDTH * 2)] <  CC_LEVEL_MID)
				&& (pLastClockEdge[(CC_BIT_WIDTH * 3)] >= CC_LEVEL_MID) )
			{
				// so we have to assume we're good to go! Return the mid-point of the first data bit
				// past the last start bit
				pFirstDataBit = &pLastClockEdge[(CC_BIT_WIDTH * 4)];
				
//				cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: start bits correct, first data bit at pixel " << uint64_t(pFirstDataBit - pFirstYSample) << endl;
				bGotClock = true;
			}
			else
			{
//				cerr	<< "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: bad start bits: "
//						<< pLastClockEdge[(CC_BIT_WIDTH * 1)] >= CC_LEVEL_MID << ", "
//						<< pLastClockEdge[(CC_BIT_WIDTH * 2)] >= CC_LEVEL_MID << ", "
//						<< pLastClockEdge[(CC_BIT_WIDTH * 3)] >= CC_LEVEL_MID << endl;
			}
		}
	}
//	else cerr << "AJAAncillaryData_Cea608_Line21::CheckDecodeClock: couldn't find first clock edge" << endl;
	
	return pFirstDataBit;
}


// call with ptr set to mid-point of first data bit (i.e. the one following the '1' start bit)
// This method will read the two characters and return 'true' if successful.
// Note: this routine will return the parity bit of each character in the ms bit position. It
// makes no calculation or value judgment as to the correctness of the parity.
AJAStatus AJAAncillaryData_Cea608_Line21::DecodeCharacters (const uint8_t *ptr, uint8_t & outChar1, uint8_t & outChar2)
{
	// first character, ls bit first
	outChar1 = 0;
	for (uint8_t i = 0;  i < 8;  i++)
	{
		const uint8_t	bit	((ptr[i * CC_BIT_WIDTH] > CC_LEVEL_MID)  ?  1  :  0);
		outChar1 += (bit << i);
	}
	
	// advance ptr to middle of first data bit in second character
	ptr += (8 * CC_BIT_WIDTH);

	// second character, ls bit first
	outChar2 = 0;
	for (uint8_t i = 0;  i < 8;  i++)
	{
		const uint8_t	bit	((ptr[i * CC_BIT_WIDTH] > CC_LEVEL_MID)  ?  1  :  0);
		outChar2 += (bit << i);
	}

#if 0	// debug
	if (char1 == 0x80 && char2 == 0x80)
		cerr << "--- AJAAncillaryData_Cea608_Line21::DecodeCharacters: returned NULL" << endl;
	else if (((outChar1 & 0x7f) >= 0x20) && ((outChar2 & 0x7f) >= 0x20))
		cerr << "--- AJAAncillaryData_Cea608_Line21::DecodeCharacters: returned '" << (outChar1 & 0x7f) << "' '" << (outChar2 & 0x7f) << endl;
	else
		cerr << "--- AJAAncillaryData_Cea608_Line21::DecodeCharacters: returned " << xHEX0N(outChar1,2) << " " << xHEX0N(outChar2) << endl;
#endif
	return AJA_STATUS_SUCCESS;
}
