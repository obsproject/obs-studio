/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_hdr_hdr10.cpp
	@brief		Implements the AJAAncillaryData_HDR_HDR10 class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_hdr_hdr10.h"
#include <ios>
#include <iomanip>

using namespace std;


#define AJAAncillaryData_HDR_HDR10_PayloadSize 0x1D


AJAAncillaryData_HDR_HDR10::AJAAncillaryData_HDR_HDR10 ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_HDR_HDR10::AJAAncillaryData_HDR_HDR10 (const AJAAncillaryData_HDR_HDR10 & inClone)
	:	AJAAncillaryData ()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_HDR_HDR10::AJAAncillaryData_HDR_HDR10 (const AJAAncillaryData_HDR_HDR10 * pInClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_HDR_HDR10::AJAAncillaryData_HDR_HDR10 (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	Init();
}


AJAAncillaryData_HDR_HDR10::~AJAAncillaryData_HDR_HDR10 ()
{
}


void AJAAncillaryData_HDR_HDR10::Init (void)
{
	m_ancType      = AJAAncillaryDataType_HDR_HDR10;
	m_coding       = AJAAncillaryDataCoding_Digital;
	m_DID          = AJAAncillaryData_HDR_HDR10_DID;
	m_SID          = AJAAncillaryData_HDR_HDR10_SID;
	m_location.SetDataLink(AJAAncillaryDataLink_A).SetDataChannel(AJAAncillaryDataChannel_Y).SetHorizontalOffset(AJAAncDataHorizOffset_AnyVanc).SetLineNumber(16);
	uint8_t payload[29] = {0x08,0x02,0x00,0xC2,0x33,0xC4,0x86,0x4C,0x1D,0xB8,0x0B,0xD0,0x84,0x80,0x3E,0x13,0x3D,0x42,0x40,0xE8,0x03,0x05,0x00,0xE8,0x03,0x90,0x01,0x00,0x00};
	SetPayloadData(payload, 29);
}


void AJAAncillaryData_HDR_HDR10::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_HDR_HDR10 & AJAAncillaryData_HDR_HDR10::operator = (const AJAAncillaryData_HDR_HDR10 & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator=(rhs);
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_HDR_HDR10::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() != AJAAncillaryData_HDR_HDR10_PayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_HDR_HDR10::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_HDR_HDR10_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_HDR_HDR10_SID)
				if (pInAncData->GetDC()  == AJAAncillaryData_HDR_HDR10_PayloadSize)
					return AJAAncillaryDataType_HDR_HDR10;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_HDR_HDR10::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl;
	return debugStream;
}
