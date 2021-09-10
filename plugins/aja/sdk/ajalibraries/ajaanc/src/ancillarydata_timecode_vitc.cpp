/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode_vitc.cpp
	@brief		Implements the AJAAncillaryData_Timecode_VITC class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_timecode_vitc.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Timecode_VITC::AJAAncillaryData_Timecode_VITC() : AJAAncillaryData_Timecode()
{
	Init();
}


AJAAncillaryData_Timecode_VITC::AJAAncillaryData_Timecode_VITC(const AJAAncillaryData_Timecode_VITC& clone) : AJAAncillaryData_Timecode()
{
	Init();
	*this = clone;
}


AJAAncillaryData_Timecode_VITC::AJAAncillaryData_Timecode_VITC(const AJAAncillaryData_Timecode_VITC *pClone) : AJAAncillaryData_Timecode()
{
	Init();
	if (pClone)
		*this = *pClone;
}


AJAAncillaryData_Timecode_VITC::AJAAncillaryData_Timecode_VITC(const AJAAncillaryData *pData) : AJAAncillaryData_Timecode(pData)
{
	Init();
}


void AJAAncillaryData_Timecode_VITC::Init (void)
{
	m_ancType = AJAAncillaryDataType_Timecode_VITC;
	m_coding  = AJAAncillaryDataCoding_Analog;
	m_DID	  = AJAAncillaryData_VITC_DID;
	m_SID	  = AJAAncillaryData_VITC_SID;

	m_vitcType = AJAAncillaryData_Timecode_VITC_Type_Unknown;	// note: "Unknown" defaults to "Timecode" for transmit
	SetLocationLineNumber(14);	//	Assume F1	otherwise SetLocationLineNumber(277);
}


AJAAncillaryData_Timecode_VITC & AJAAncillaryData_Timecode_VITC::operator = (const AJAAncillaryData_Timecode_VITC & inRHS)
{
	if (this != &inRHS)		// ignore self-assignment
	{
		AJAAncillaryData_Timecode::operator= (inRHS);		// copy the base class stuff
		// copy the local stuff
		m_vitcType = inRHS.m_vitcType;
	}
	return *this;
}


void AJAAncillaryData_Timecode_VITC::Clear (void)
{
	AJAAncillaryData_Timecode::Clear ();
	Init ();
}


AJAStatus AJAAncillaryData_Timecode_VITC::ParsePayloadData (void)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// reality check...
	if (GetDC() < AJAAncillaryData_VITC_PayloadSize)
	{
		Init();						// load default values
		status = AJA_STATUS_FAIL;
		m_rcvDataValid = false;
	}
	else
	{
		// we have some kind of payload data - try to parse it
		m_rcvDataValid = DecodeLine(GetPayloadData());
	}
	return status;
}


AJAStatus AJAAncillaryData_Timecode_VITC::GeneratePayloadData (void)
{
	m_DID = AJAAncillaryData_VITC_DID;
	m_SID = AJAAncillaryData_VITC_SID;

	AJAStatus status = AllocDataMemory(AJAAncillaryData_VITC_PayloadSize);
	if (AJA_FAILURE(status))
		return status;

	status = EncodeLine(&m_payload[0]);
	if (AJA_FAILURE(status))
		return status;

	// round-trip: TEST ONLY!
	//bool bResult = DecodeLine (m_pPayload);

	m_checksum = Calculate8BitChecksum ();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_VITC::SetVITCDataType (const AJAAncillaryData_Timecode_VITC_Type inType)
{
	if (   inType == AJAAncillaryData_Timecode_VITC_Type_Timecode
		|| inType == AJAAncillaryData_Timecode_VITC_Type_FilmData
		|| inType == AJAAncillaryData_Timecode_VITC_Type_ProdData)
	{
		m_vitcType = inType;
		return AJA_STATUS_SUCCESS;
	}
	return AJA_STATUS_RANGE;
}



AJAAncillaryDataType AJAAncillaryData_Timecode_VITC::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	// note: BIG ASSUMPTION! If the user deliberately captured analog line 19 on either field,
	//       we're assuming it was for the sake of getting captioning data (NTSC/525-line).
	//       The only way we could know "for sure" would be to run ParsePayloadData() on
	//       the payload data, but that's not a static method so you'd have to recast the
	//		 AJAAncillaryData object anyway!
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Analog)
		if (pInAncData->GetLocationLineNumber() == 14 || pInAncData->GetLocationLineNumber() == 277)
		return AJAAncillaryDataType_Timecode_VITC;

	return AJAAncillaryDataType_Unknown;
}



//-------------------------------------------------------------
// VITC Decode code (ported/stolen from ntv2vitc.cpp)


	// According to SMPTE-12M, in NTSC the VITC waveform should begin no EARLIER than 10.0 usecs
	// from the leading edge of sync, which translates to approx 13 pixels from the beginning of
	// active video. In PAL, the spec says the VITC waveform should start no earlier than 11.2 usecs
	// from the leading edge of sync, or 19 pixels from the beginning of active video. Just to be
	// on the safe side, we're going to start looking for the first leading edge at pixel 10, and
	// stop looking (if we haven't found it) at pixel 30.
const uint32_t VITC_DECODE_START_WINDOW	= 10;
const uint32_t VITC_DECODE_END_WINDOW   = 30;

const uint8_t VITC_Y_CLIP = 102;		//  8-bit YCbCr threshold (assumes black = 16)

	// levels
const uint8_t VITC_YUV8_LO	= 0x10;		// '0'
const uint8_t VITC_YUV8_HI	= 0xC0;		// '1'



// return the VITC bit level (true or false) at pixelNum. pLine points to the beginning
// of the line (i.e. the first byte in active video).
static bool getVITCLevel (uint32_t pixelNum, const uint8_t * pLine)
{
	uint8_t luma = pLine[pixelNum];
	return luma > VITC_Y_CLIP;
}


// Implements one bit of the SMPTE-12M CRC polynomial: x^8 + 1
// Pretty simple: if the current ms bit of the shift register is 0, shift left, adding the new bit to the ls position.
//                if the current ms bit of the shift register is 1, shift left, adding the inverse of the new bit to the ls position.
static void addToCRC (const bool inBit, uint8_t & inOutCRC)
{
	uint8_t newBit = 0;

	// a more concise way to say this is: newBit = (inOutCRC >> 7) ^ inBit;
	if (inOutCRC & 0x80)
		newBit = (inBit ? 0 : 1);
	else
		newBit = (inBit ? 1 : 0);

	inOutCRC = (inOutCRC << 1) + newBit;
}


/**
	Decodes the supplied video line and, if successful, stores the timecode data in my superclass'
	m_timeDigits member.
	Note: this routine will try to make a reasonable attempt to discover whether a VITC waveform
	is present or not and whether the VITC checksum is valid. If not, it will return 'false'
	and *pRP188 will be left untouched.
	This routine also looks for RP-201 "Film Data" and "Production Data". These are two additional
	lines of VITC waveforms, but have different CRCs. If the caller supplies pointers for pbGotFilmData
	and pbGotProdData, we will return 'true' when ANY of the possible lines is received. If the two
	pointers are NULL, we will ONLY return true for VITC timecode data.
**/
bool AJAAncillaryData_Timecode_VITC::DecodeLine (const uint8_t *pLine)
{
	bool bResult = false;

	uint8_t CRC = 0;		// accumulate the CRC here
	uint8_t tcData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };	// accumulate the data bits here

	/**
		The VITC waveform consists of 90 bits, arranged as 9 groups of 10 bits. Each 10-bit
		group consists of 2 start bits (a '1' followed by a '0') and 8 data bits. The first
		eight groups contains the 64 bits of timecode data, and the last group contains an
		8-bit CRC, generated by the SMPTE-12M polynomial: x^8 + 1.

		The timing specs call for a bit width of H/115 (NTSC) or H/116 (PAL), where "H" is the
		total length of one video line (858 pixels for NTSC, or 864 pixels for PAL). This means
		that each bit cell is approx 7.46 pixels wide (NTSC) or 7.45 pixels wide (PAL). We're
		going to use 7.5 pixels as an approximation. After all 90 bits, the cumulative error is
		3.52 pixels (NTSC) or 4.7 pixels (PAL) - or approx 1/2 bit cell. To keep this error in
		check, we're going to re-synchronize at the beginning of each group, during the 1 -> 0
		start bit transition. This means that the error will only propagate across the 10 bits
		of one group instead of the entire line.
	**/
	uint32_t currIndex = 0;		// our running pixel index
	uint32_t i;

	// First look for the beginning of the first start bit.
	bool lastPixel = getVITCLevel(VITC_DECODE_START_WINDOW, pLine);
	for (currIndex = VITC_DECODE_START_WINDOW+1; currIndex < VITC_DECODE_END_WINDOW; currIndex++)
	{
		bool thisPixel = getVITCLevel(currIndex, pLine);
		
		// look for a 0 -> 1 edge
		if (!lastPixel && thisPixel)
			break;
	}
	
	// if we didn't find a 0 -> 1 edge within the window, bail
	if (currIndex == VITC_DECODE_END_WINDOW)
		return false;
		
	// we found an edge - now make sure the next 3 pixels are '1' (this will put us in the
	// middle of the first start bit cell)
	if (   !getVITCLevel(currIndex+1, pLine)
		|| !getVITCLevel(currIndex+2, pLine)
		|| !getVITCLevel(currIndex+3, pLine) )
		return false;
	else
		currIndex += 3;
		
	// repeat for 9 groups...
	for (uint8_t group = 0; group < 9; group++)
	{
		uint8_t data = 0;
		
		// assuming our index is sitting in the middle of the leading ('1') start bit of the group,
		// look for the 1 -> 0 start bit transition. This will become our reference for the rest of
		// the group. Ideally, the transition should happen 4.5 pixels from now, but we'll look as
		// far as 8 pixels out.
		for (i = 1; i < 8; i++)
		{	
			if ( !getVITCLevel(currIndex+i, pLine) )
				break;
		}
		
		// if we couldn't find a 1->0 transition, assume we're broken and bail
		if (i == 8)
			break;
		else
			currIndex += i;		// currIndex now sits at the beginning of the 2nd ('0' start bit) of the group
		
		// all ten bits are used to calculate the CRC, so add the two start bits now
		addToCRC(true,  CRC);
		addToCRC(false, CRC);
		
		// position index in the middle of the first data bit (i.e. 1.5 bitcells from here)
		currIndex += 11;
		
		// add the 8 data bits, assuming 7.5 pixels per VITC bit cell.
		for (i = 0; i < 8; i++)
		{
			bool bit = getVITCLevel(currIndex, pLine);
			addToCRC(bit, CRC);
			currIndex += (i%2 ? 8 : 7);		// alternate between 7 and 8 pixels to maintain 7.5 average
			
				// the data bits are transmit ls-bit first, so shift in from the right
			data = (bit ? 0x80 : 0) + (data >> 1);
		}
		
		// put the collected data byte where it belongs
		if (group < 8)					// this is a data group
			tcData[group] = data;

		else							// this is the last (CRC) group: after it's finished, the CRC register should be
		{								//    one of the magic numbers or we had an error
			if (CRC == 0x00)
			{
				m_vitcType = AJAAncillaryData_Timecode_VITC_Type_Timecode;		// we've got valid "timecode" data
				bResult = true;
			}
			else if (CRC == 0xFF)
			{
				m_vitcType = AJAAncillaryData_Timecode_VITC_Type_FilmData;		// we've got a valid RP-201 "Film Data Block"
				bResult = true;
			}

			else if (CRC == 0x0F)
			{
				m_vitcType = AJAAncillaryData_Timecode_VITC_Type_ProdData;		// we've got a valid RP-201 "Production Data Block"
				bResult = true;
			}

			else
			{
				m_vitcType = AJAAncillaryData_Timecode_VITC_Type_Unknown;		// unrecognized CRC? Assume it's bad data...
				bResult = false;
			}
		}
		
		//	At this point, currIndex should now be positioned in the middle of the NEXT group's leading ('1') start bit

	}	// for (int group...
	

	// finished with all of the groups! If we think we've been successful, transfer the data bytes to the RP188_STRUCT
	if (bResult)
	{
		// the "time" digits are in the ls nibbles of each received byte
		// (note: SetTimeHexValue() performs the needed masking)
		SetTimeHexValue(kTcFrameUnits,  tcData[0]);		// frame units);
		SetTimeHexValue(kTcFrameTens,   tcData[1]);		// frame tens
		SetTimeHexValue(kTcSecondUnits, tcData[2]);		// second units
		SetTimeHexValue(kTcSecondTens,  tcData[3]);		// second tens
		SetTimeHexValue(kTcMinuteUnits, tcData[4]);		// minute units
		SetTimeHexValue(kTcMinuteTens,  tcData[5]);		// minute tens
		SetTimeHexValue(kTcHourUnits,   tcData[6]);		// hour units
		SetTimeHexValue(kTcHourTens,    tcData[7]);		// hour tens

		// the Binary Group "digits" are in the ms nibbles of each received byte
		// (note: SetBinaryGroupHexValue() performs the needed masking)
		SetBinaryGroupHexValue(kBg1, (tcData[0] >> 4));		// BG 1
		SetBinaryGroupHexValue(kBg2, (tcData[1] >> 4));		// BG 2
		SetBinaryGroupHexValue(kBg3, (tcData[2] >> 4));		// BG 3
		SetBinaryGroupHexValue(kBg4, (tcData[3] >> 4));		// BG 4
		SetBinaryGroupHexValue(kBg5, (tcData[4] >> 4));		// BG 5
		SetBinaryGroupHexValue(kBg6, (tcData[5] >> 4));		// BG 6
		SetBinaryGroupHexValue(kBg7, (tcData[6] >> 4));		// BG 7
		SetBinaryGroupHexValue(kBg8, (tcData[7] >> 4));		// BG 8
	}
	
	return bResult;
}


//-------------------------------------------------------------
// VITC Encode code (ported/stolen from ntv2vitc.cpp)

#ifdef USE_SMPTE_266M

// output one pixel in the line at the designated index. <level> is the 8-bit (Y) video level for this pixel.
static inline void DoVITCPixel (uint8_t * pOutLine, const uint32_t inPixelNum, const uint8_t inLevel)
{
	pOutLine[inPixelNum] = inLevel;
}


// Output a four-pixel transition that crosses the 50% point exactly on a pixel.
// SMPTE-266 refers to this as "Curve B"
static void DoNormalTransition (uint8_t *pLine, uint32_t& pixelIndex, bool bBit0, bool bBit1)
{
	if (!bBit0 && !bBit1)		// 0 -> 0 transition:
	{
		//	Output 4 pixels of "low" level...
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
	}
	else if (!bBit0 && bBit1)	// 0 -> 1 transition:
	{
		DoVITCPixel (pLine, pixelIndex++, 0x2A);
		DoVITCPixel (pLine, pixelIndex++, 0x68);	// 50%
		DoVITCPixel (pLine, pixelIndex++, 0xA6);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
	}
	else if (bBit0 && !bBit1)	// 1 -> 0 transition:
	{
		DoVITCPixel (pLine, pixelIndex++, 0xA6);
		DoVITCPixel (pLine, pixelIndex++, 0x68);	// 50%
		DoVITCPixel (pLine, pixelIndex++, 0x2A);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
	}
	else						// 1 -> 1 transition:
	{
		//	Output 4 pixels of "high" level...
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
	}
}


// Output a four-pixel transition that crosses the 50% point exactly between two pixels. This transition is
// used between two bits of a "pair", making each bit a "half" pixel in length (e.g. 7.5 pixels each).
static void DoHalfTransition (uint8_t *pLine, uint32_t& pixelIndex, bool bBit0, bool bBit1)
{
	if (!bBit0 && !bBit1)		// 0 -> 0 transition:
	{
		//	Output 3 pixels of "low" level...
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
	}
	else if (!bBit0 && bBit1)	// 0 -> 1 transition:
	{
		DoVITCPixel (pLine, pixelIndex++, 0x3C);
		DoVITCPixel (pLine, pixelIndex++, 0x94);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
	}
	else if (bBit0 && !bBit1)	// 1 -> 0 transition:
	{
		DoVITCPixel (pLine, pixelIndex++, 0x94);
		DoVITCPixel (pLine, pixelIndex++, 0x3C);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
	}
	else						// 1 -> 1 transition:
	{
		//	Output 3 pixels of "high" level...
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_HI);
	}
}


// Encodes 2 VITC bits (15 pixels), assuming the previous bit ended on <bPrevBit>.
static void DoVITCBitPair (uint8_t *pLine, uint32_t& pixelIndex, bool bPrevBit, bool bBit0, bool bBit1)
{
	uint8_t level;

	// 1st: do normal (SMPTE-266 calls it "Curve B") transition (50% point falls on exact pixel) from previous bit to Bit0 (pixels #0 - #3)
	DoNormalTransition(pLine, pixelIndex, bPrevBit, bBit0);

	// 2nd: do 3 pixels of "Bit0" level
	level = (bBit0 ? VITC_YUV8_HI : VITC_YUV8_LO);
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #4
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #5
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #6
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #7

	// 3rd: do "half" (SMPTE-266 calls it "Curve A") transition (50% point falls between two pixels) from Bit0 to Bit 1 (pixels #8 - #10)
	DoHalfTransition(pLine, pixelIndex, bBit0, bBit1);

	// 4th: do remaining pixels of "Bit 1" level this is the place to drop a pixel if desired)
	level = (bBit1 ? VITC_YUV8_HI : VITC_YUV8_LO);
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #11
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #12
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #13
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #14
}


// Encodes the supplied timecode data to VITC and inserts it in the designated video line.
// If this is RP-201 "Film" or "Production" data, we jigger the CRC per RP-201 specs.
AJAStatus AJAAncillaryData_Timecode_VITC::EncodeLine(uint8_t *pLine) const
{
	uint32_t i;
	uint32_t pixelIndex = 0;		// running pixel count
	uint32_t bitPair;
	bool bBit0, bBit1;
	bool bPrevBit = false;		// the last bit of the previous group
	uint8_t CRC = 0;
	
	// 1st: do 26 pixels of black before 1st start bit
	for (i = 0; i < 26; i++)
		DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);
		
	// VITC consists of 9 "groups" of 10 bits each (or 5 bit-pairs). Each group starts with a '1' bit and a '0' bit,
	// followed by 8 data bits. The first 8 groups carry the 64 bits of VITC data, and the last group carries an 8-bit
	// CRC calculated from the preceding 82 bits.
	for (uint8_t group(0);  group < kNumTimeDigits;  group++)
	{
		uint8_t tcData, bgData;
		GetTimeHexValue(group, tcData);
		GetBinaryGroupHexValue(group, bgData);
		uint8_t data = (bgData << 4) + tcData;

		// Note: we do VITC bit "pairs" because the timing works out to be 7.5 pixels per VITC bit.
		// A bit pair is exactly 15 pixels, so it's more convenient to work with
		DoVITCBitPair(pLine, pixelIndex, bPrevBit, 1, 0);			// Start bits: 1, 0
		addToCRC (1, CRC);
		addToCRC (0, CRC);				// note: start bits are included in CRC
		bPrevBit = false;

		for (bitPair = 0; bitPair < 4; bitPair++)
		{
			bBit0 = (data & 0x01) > 0;
			bBit1 = (data & 0x02) > 0;
			
			DoVITCBitPair(pLine, pixelIndex, bPrevBit, bBit0, bBit1);		// 8 Data bits per group (ls bit first)
			
			// add bits to CRC
			addToCRC (bBit0, CRC);
			addToCRC (bBit1, CRC);
			
			bPrevBit = bBit1;
			data = (data >> 2);
		}
	}
	

	// do the final "CRC" group
	DoVITCBitPair(pLine, pixelIndex, bPrevBit, 1, 0);			// Start bits: 1, 0
	
	// the CRC also includes the start bits for the CRC group, so add those now
	addToCRC (1, CRC);
	addToCRC (0, CRC);
	bPrevBit = false;

	// if we're transmitting standard VITC timecode, the CRC is sent as-is
	// if this is RP-201 "Film Data", the CRC needs to be inverted
	// if this is RP-201 "Production Data", the only the "upper" nibble of the CRC is inverted
	if (m_vitcType == AJAAncillaryData_Timecode_VITC_Type_FilmData)
		CRC = ~CRC;
	else if (m_vitcType == AJAAncillaryData_Timecode_VITC_Type_ProdData)
		CRC = CRC ^ 0x0F;

	for (i = 0; i < 4; i++)
	{
		bBit0 = (CRC & 0x80) > 0;
		bBit1 = (CRC & 0x40) > 0;
		
		DoVITCBitPair(pLine, pixelIndex, bPrevBit, bBit0, bBit1);		// 8 Data bits per group (ms bit first)
		
		bPrevBit = bBit1;
		CRC = (CRC << 2);
	}
	
	// just in case we ended on a '1' data bit, do a final transition back to black
	DoNormalTransition(pLine, pixelIndex, bPrevBit, 0);

	// fill the remainder of the line with Black (note that we're assuming 720 active pixels!)
	const uint32_t	remainingPixels	(GetDC() > pixelIndex  ?  GetDC() - pixelIndex  :  0);
	if (remainingPixels)
		for (i = 0; i < (uint32_t)remainingPixels; i++)
			DoVITCPixel (pLine, pixelIndex++, VITC_YUV8_LO);

	return AJA_STATUS_SUCCESS;
}

//------------------------------------------------------------------------------

#else	// use original NTV2 VITC algorithm

// output one pixel in the line at the designated index. <level> is a relative pixel value,
// where "0.0" represents the logic low level, and "1.0" represents the logic high level.
static void DoVITCPixel (uint8_t *pLine, uint32_t pixelNum, float level)
{
	uint8_t luma;
	if      (level <= 0.0)	luma = VITC_YUV8_LO;
	else if (level >= 1.0)	luma = VITC_YUV8_HI;
	else					luma = (uint8_t)((float)(VITC_YUV8_HI - VITC_YUV8_LO) * level) + VITC_YUV8_LO;

	pLine[pixelNum] = luma;
}


// Output a five-pixel transition that crosses the 50% point exactly on a pixel.
static void DoNormalTransition (uint8_t *pLine, uint32_t& pixelIndex, bool bBit0, bool bBit1)
{
	if (!bBit0 && !bBit1)
	{
		// 0 -> 0 transition: output 5 pixels of "low" level
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
	}
	else if (!bBit0 && bBit1)
	{
		// 0 -> 1 transition
		DoVITCPixel (pLine, pixelIndex++, (float)0.01);
		DoVITCPixel (pLine, pixelIndex++, (float)0.18);
		DoVITCPixel (pLine, pixelIndex++, (float)0.50);
		DoVITCPixel (pLine, pixelIndex++, (float)0.82);
		DoVITCPixel (pLine, pixelIndex++, (float)0.99);
	}
	else if (bBit0 && !bBit1)
	{
		// 1 -> 0 transition
		DoVITCPixel (pLine, pixelIndex++, (float)0.99);
		DoVITCPixel (pLine, pixelIndex++, (float)0.82);
		DoVITCPixel (pLine, pixelIndex++, (float)0.50);
		DoVITCPixel (pLine, pixelIndex++, (float)0.18);
		DoVITCPixel (pLine, pixelIndex++, (float)0.01);
	}
	else
	{
		// 1 -> 1 transition: output 5 pixels of "high" level
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
	}
}


// Output a four-pixel transition that crosses the 50% point exactly between two pixels. This transition is
// used between two bits of a "pair", making each bit a "half" pixel in length (e.g. 7.5 pixels each).
static void DoHalfTransition (uint8_t *pLine, uint32_t& pixelIndex, bool bBit0, bool bBit1)
{
	if (!bBit0 && !bBit1)
	{
		// 0 -> 0 transition: output 4 pixels of "low" level
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
		DoVITCPixel (pLine, pixelIndex++, (float)0.0);
	}
	else if (!bBit0 && bBit1)
	{
		// 0 -> 1 transition
		DoVITCPixel (pLine, pixelIndex++, (float)0.00);
		DoVITCPixel (pLine, pixelIndex++, (float)0.33);
		DoVITCPixel (pLine, pixelIndex++, (float)0.67);
		DoVITCPixel (pLine, pixelIndex++, (float)1.00);
	}
	else if (bBit0 && !bBit1)
	{
		// 1 -> 0 transition
		DoVITCPixel (pLine, pixelIndex++, (float)1.00);
		DoVITCPixel (pLine, pixelIndex++, (float)0.67);
		DoVITCPixel (pLine, pixelIndex++, (float)0.33);
		DoVITCPixel (pLine, pixelIndex++, (float)0.00);
	}
	else
	{
		// 1 -> 1 transition: output 4 pixels of "high" level
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
		DoVITCPixel (pLine, pixelIndex++, (float)1.0);
	}
}

// Encodes 2 VITC bits (15 pixels), assuming the previous bit ended on <bPrevBit>.
// bDropBit can be used to make a 14-pixel pair to try and keep the VITC waveform in time with the spec
static void DoVITCBitPair (uint8_t *pLine, uint32_t& pixelIndex, bool bPrevBit, bool bBit0, bool bBit1, bool bDropBit)
{
	float level;				// 0.0 -> 1.0 relative level (0.0 = logic low, 1.0 = logic high)

	// 1st: do normal transition (50% point falls on exact pixel) from previous bit to Bit0 (pixels #0 - #4)
	DoNormalTransition(pLine, pixelIndex, bPrevBit, bBit0);

	// 2nd: do 3 pixels of "Bit0" level
	level = (bBit0 ? (float)1.0 : (float)0.0);
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #5
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #6
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #7

	// 3rd: do "half" transition (50% point falls between two pixels) from Bit0 to Bit 1 (pixels #8 - #11)
	DoHalfTransition(pLine, pixelIndex, bBit0, bBit1);

	// 4th: do remaining pixels of "Bit 1" level this is the place to drop a pixel if desired)
	level = (bBit1 ? (float)1.0 : (float)0.0);
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #12
	DoVITCPixel (pLine, pixelIndex++, level);		// pixel #13
	if (!bDropBit)
		DoVITCPixel (pLine, pixelIndex++, level);	// pixel #14 (optional)
}


// Encodes the supplied timecode data to VITC and inserts it in the designated video line.
// If this is RP-201 "Film" or "Production" data, we jigger the CRC per RP-201 specs.
bool AJAAncillaryData_Timecode_VITC::EncodeLine (uint8_t *pLine)
{
	bool bResult = true;
	uint32_t i;
	uint32_t pixelIndex = 0;		// running pixel count
	uint32_t group;
	uint32_t bitPair;
	bool bBit0, bBit1;
	bool bPrevBit = false;		// the last bit of the previous group
	bool bDropBit = false;
	uint8_t CRC = 0;
	
	// 1st: do 24 pixels of black before 1st start bit
	for (i = 0; i < 24; i++)
		DoVITCPixel (pLine, pixelIndex++, 0.0);
		
	// VITC consists of 9 "groups" of 10 bits each (or 5 bit-pairs). Each group starts with a '1' bit and a '0' bit,
	// followed by 8 data bits. The first 8 groups carry the 64 bits of VITC data, and the last group carries an 8-bit
	// CRC calculated from the preceding 82 bits.
	for (group = 0; group < kNumTimeDigits; group++)
	{
		uint8_t tcData, bgData;
		GetTimeHexValue(group, tcData);
		GetBinaryGroupHexValue(group, bgData);
		uint8_t data = (bgData << 4) + tcData;

		// Note: we do VITC bit "pairs" because the timing works out to be 7.5 pixels per VITC bit.
		// A bit pair is exactly 15 pixels, so it's more convenient to work with
		DoVITCBitPair(pLine, pixelIndex, bPrevBit, 1, 0, false);			// Start bits: 1, 0
		addToCRC (1, CRC);
		addToCRC (0, CRC);				// note: start bits are included in CRC
		bPrevBit = false;

		for (bitPair = 0; bitPair < 4; bitPair++)
		{
			bBit0 = (data & 0x01) > 0;
			bBit1 = (data & 0x02) > 0;
			
			// in every other group, drop a pixel in the last pixel pair to help keep time
			bDropBit = (group % 2 && bitPair == 3);
			
			DoVITCBitPair(pLine, pixelIndex, bPrevBit, bBit0, bBit1, bDropBit);		// 8 Data bits per group (ls bit first)
			
			// add bits to CRC
			addToCRC (bBit0, CRC);
			addToCRC (bBit1, CRC);
			
			bPrevBit = bBit1;
			data = (data >> 2);
		}
	}
	

	// do the final "CRC" group
	DoVITCBitPair(pLine, pixelIndex, bPrevBit, 1, 0, false);			// Start bits: 1, 0
	
	// the CRC also includes the start bits for the CRC group, so add those now
	addToCRC (1, CRC);
	addToCRC (0, CRC);
	bPrevBit = false;

	// if we're transmitting standard VITC timecode, the CRC is sent as-is
	// if this is RP-201 "Film Data", the CRC needs to be inverted
	// if this is RP-201 "Production Data", the only the "upper" nibble of the CRC is inverted
	if (m_vitcType == AJAAncillaryData_Timecode_VITC_Type_FilmData)
		CRC = ~CRC;
	else if (m_vitcType == AJAAncillaryData_Timecode_VITC_Type_ProdData)
		CRC = CRC ^ 0x0F;

	for (i = 0; i < 4; i++)
	{
		bBit0 = (CRC & 0x80) > 0;
		bBit1 = (CRC & 0x40) > 0;
		
		DoVITCBitPair(pLine, pixelIndex, bPrevBit, bBit0, bBit1, bDropBit);		// 8 Data bits per group (ms bit first)
		
		bPrevBit = bBit1;
		CRC = (CRC << 2);
	}
	
	// just in case we ended on a '1' data bit, do a final transition back to black
	DoNormalTransition(pLine, pixelIndex, bPrevBit, 0);
	
	
	// fill the remainder of the line with Black (note that we're assuming 720 active pixels!)
	int32_t remainingPixels = m_DC - pixelIndex;
	
	if (remainingPixels > 0)
	{
		for (i = 0; i < (uint32_t)remainingPixels; i++)
			DoVITCPixel (pLine, pixelIndex++, 0.0);
	}
	
	return bResult;
}

#endif	// !USE_SMPTE_266M

string AJAAncillaryData_Timecode_VITC::VITCTypeToString (const AJAAncillaryData_Timecode_VITC_Type inType)
{
	switch (inType)
	{
		case AJAAncillaryData_Timecode_VITC_Type_Timecode:	return "timecode (CRC=0x00)";
		case AJAAncillaryData_Timecode_VITC_Type_FilmData:	return "RP-201 Film Data (CRC=0xFF)";
		case AJAAncillaryData_Timecode_VITC_Type_ProdData:	return "RP-201 Prod Data (CRC=0x0F)";
		default:											break;
	}
	return "??";
}


ostream & AJAAncillaryData_Timecode_VITC::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncillaryDataCodingToString (m_coding) << ")" << endl;
	//debugStream << "SMPTE 12M VITC Analog Data (" << ((m_coding == AJAAncillaryDataCoding_Digital) ? "Digital" : ((m_coding == AJAAncillaryDataCoding_Analog) ? "Analog" : "???????")) << ")" << endl;
	AJAAncillaryData_Timecode::Print (debugStream, bShowDetail);	// print the generic stuff
	debugStream << endl
				<< "VITC Type: " << VITCTypeToString (m_vitcType);
	return debugStream;
}
