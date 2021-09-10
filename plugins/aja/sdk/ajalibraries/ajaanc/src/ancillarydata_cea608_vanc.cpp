/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608_vanc.cpp
	@brief		Implements the AJAAncillaryData_Cea608_Vanc class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_cea608_vanc.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc()
	:	AJAAncillaryData_Cea608()
{
	Init();
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc& clone)
	:	AJAAncillaryData_Cea608()
{
	Init();
	*this = clone;
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc *pClone)
	:	AJAAncillaryData_Cea608()
{
	Init();
	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Cea608_Vanc::AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData *pData)
	:	AJAAncillaryData_Cea608 (pData)
{
	Init();
}


AJAAncillaryData_Cea608_Vanc::~AJAAncillaryData_Cea608_Vanc ()
{
}


void AJAAncillaryData_Cea608_Vanc::Init (void)
{
	m_ancType = AJAAncillaryDataType_Cea608_Vanc;
	m_coding  = AJAAncillaryDataCoding_Digital;
	m_DID	  = AJAAncillaryData_Cea608_Vanc_DID;
	m_SID	  = AJAAncillaryData_Cea608_Vanc_SID;

	m_isF2 = false;		// default to field 1
	m_lineNum  = 12;	// line 21 (0 = line 9 in 525i)
}


AJAAncillaryData_Cea608_Vanc & AJAAncillaryData_Cea608_Vanc::operator= (const AJAAncillaryData_Cea608_Vanc & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		AJAAncillaryData_Cea608::operator= (rhs);		// copy the base class stuff

		// copy the local stuff
		m_isF2 = rhs.m_isF2;
		m_lineNum  = rhs.m_lineNum;
	}
	return *this;
}


void AJAAncillaryData_Cea608_Vanc::Clear (void)
{
	AJAAncillaryData_Cea608::Clear();
	Init();
}


AJAStatus AJAAncillaryData_Cea608_Vanc::SetLine (const bool inIsF2, const uint8_t lineNum)
{
	m_isF2 = inIsF2;
	m_lineNum  = lineNum  & 0x1F;	// valid range is 0 (= line 9) to 31 (= line 40)
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::GetLine (uint8_t & fieldNum, uint8_t & lineNum) const
{
	fieldNum = IsField2()  ?  0x01/*NTV2_FIELD1*/  :  0x00/*NTV2_FIELD0*/;
	lineNum  = uint8_t(GetLineNumber());
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::ParsePayloadData (void)
{
	if (GetDC() < AJAAncillaryData_Cea608_Vanc_PayloadSize)
	{
		Init();						// load default values
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	//	Parse the payload data...
	m_isF2 = ((m_payload[0] >> 7) & 0x01) ? false : true;	//	Field number (flag) is bit 7 of the 1st payload word
															//	SDKs prior to 16.0 had the sense of this bit wrong.
	m_lineNum  = (m_payload[0] & 0x1F);						//	Line number is bits [4:0] of the 1st payload word
	m_char1	   = m_payload[1];		// the 1st character
	m_char2    = m_payload[2];		// the 2nd character
	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608_Vanc::GeneratePayloadData (void)
{
	m_DID = AJAAncillaryData_Cea608_Vanc_DID;
	m_SID = AJAAncillaryData_Cea608_Vanc_SID;

	AJAStatus status = AllocDataMemory (AJAAncillaryData_Cea608_Vanc_PayloadSize);
	if (AJA_SUCCESS (status))
	{
		/*
			From S334-1 and S334-2 standards documents:
				The LINE value at the start of the UDW represents the field number and line where the data are intended to be carried.
				Bit b7 of the LINE value is the field number (0 for field 2; 1 for field 1).
				Bits b6 and b5 are 0.
				Bits b4-b0 form a 5-bit unsigned integer which represents the offset (in lines) of the data insertion line,
				relative to the base line for the original image format:
					line 9 of 525-line F1
					line 272 of 525-line F2
					line 5 of 625-line F1
					line 318 of 625-line F2
			NOTE: SDKs prior to 16.0 used the opposite sense of this bit.
		*/
		m_payload[0] = uint8_t((m_isF2 ? 0x00 : 0x01) << 7) | (m_lineNum & 0x1F);	//	F2 flag goes in b7, line num in bits [4:0]
		m_payload[1] = m_char1;
		m_payload[2] = m_char2;
	}

	m_checksum = Calculate8BitChecksum();
	return status;
}

ostream & AJAAncillaryData_Cea608_Vanc::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncillaryDataCodingToString (m_coding) << ")" << endl;
	AJAAncillaryData_Cea608::Print (debugStream, bShowDetail);
	debugStream << endl
				<< "Field: " << (m_isF2 ? "F2" : "F1") << endl
				<< "Line: " << dec << uint16_t(m_lineNum);
	return debugStream;
}


AJAAncillaryDataType AJAAncillaryData_Cea608_Vanc::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_Cea608_Vanc_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_Cea608_Vanc_SID)
				if (pInAncData->GetDC() == AJAAncillaryData_Cea608_Vanc_PayloadSize)
					return AJAAncillaryDataType_Cea608_Vanc;
	return AJAAncillaryDataType_Unknown;
}
