/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_hdr_hlg.cpp
	@brief		Implements the AJAAncillaryData_HDR_HLG class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_hdr_hlg.h"
#include <ios>
#include <iomanip>

using namespace std;


#define AJAAncillaryData_HDR_HLG_PayloadSize 0x1D


AJAAncillaryData_HDR_HLG::AJAAncillaryData_HDR_HLG ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_HDR_HLG::AJAAncillaryData_HDR_HLG (const AJAAncillaryData_HDR_HLG & inClone)
	:	AJAAncillaryData ()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_HDR_HLG::AJAAncillaryData_HDR_HLG (const AJAAncillaryData_HDR_HLG * pInClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_HDR_HLG::AJAAncillaryData_HDR_HLG (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	Init();
}


AJAAncillaryData_HDR_HLG::~AJAAncillaryData_HDR_HLG ()
{
}


void AJAAncillaryData_HDR_HLG::Init (void)
{
	m_ancType      = AJAAncillaryDataType_HDR_HLG;
	m_coding       = AJAAncillaryDataCoding_Digital;
	m_DID          = AJAAncillaryData_HDR_HLG_DID;
	m_SID          = AJAAncillaryData_HDR_HLG_SID;
	m_location.SetDataLink(AJAAncillaryDataLink_A).SetDataChannel(AJAAncillaryDataChannel_Y).SetLineNumber(16).SetHorizontalOffset(AJAAncDataHorizOffset_AnyVanc);
	uint8_t payload[29] = {0x08,0x03,0x00,0x0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	SetPayloadData(payload, 29);
}


void AJAAncillaryData_HDR_HLG::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_HDR_HLG & AJAAncillaryData_HDR_HLG::operator = (const AJAAncillaryData_HDR_HLG & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator=(rhs);
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_HDR_HLG::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() != AJAAncillaryData_HDR_HLG_PayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_HDR_HLG::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_HDR_HLG_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_HDR_HLG_SID)
				if (pInAncData->GetDC()  == AJAAncillaryData_HDR_HLG_PayloadSize)
					return AJAAncillaryDataType_HDR_HLG;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_HDR_HLG::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl;
	return debugStream;
}
