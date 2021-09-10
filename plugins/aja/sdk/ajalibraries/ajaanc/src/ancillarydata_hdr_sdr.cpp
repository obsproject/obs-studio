/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_hdr_sdr.cpp
	@brief		Implements the AJAAncillaryData_HDR_SDR class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_hdr_sdr.h"
#include <ios>
#include <iomanip>

using namespace std;


#define AJAAncillaryData_HDR_SDR_PayloadSize 0x1D


AJAAncillaryData_HDR_SDR::AJAAncillaryData_HDR_SDR ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_HDR_SDR::AJAAncillaryData_HDR_SDR (const AJAAncillaryData_HDR_SDR & inClone)
	:	AJAAncillaryData ()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_HDR_SDR::AJAAncillaryData_HDR_SDR (const AJAAncillaryData_HDR_SDR * pInClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_HDR_SDR::AJAAncillaryData_HDR_SDR (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	Init();
}


AJAAncillaryData_HDR_SDR::~AJAAncillaryData_HDR_SDR ()
{
}


void AJAAncillaryData_HDR_SDR::Init (void)
{
	m_ancType      = AJAAncillaryDataType_HDR_SDR;
	m_coding       = AJAAncillaryDataCoding_Digital;
	m_DID          = AJAAncillaryData_HDR_SDR_DID;
	m_SID          = AJAAncillaryData_HDR_SDR_SID;
	m_location.SetDataLink(AJAAncillaryDataLink_A).SetDataChannel(AJAAncillaryDataChannel_Y).SetHorizontalOffset(AJAAncDataHorizOffset_AnyVanc).SetLineNumber(16);
	uint8_t payload[29] = {0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	SetPayloadData(payload, 29);
}


void AJAAncillaryData_HDR_SDR::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_HDR_SDR & AJAAncillaryData_HDR_SDR::operator = (const AJAAncillaryData_HDR_SDR & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator=(rhs);
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_HDR_SDR::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() != AJAAncillaryData_HDR_SDR_PayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_HDR_SDR::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_HDR_SDR_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_HDR_SDR_SID)
				if (pInAncData->GetDC()  == AJAAncillaryData_HDR_SDR_PayloadSize)
					return AJAAncillaryDataType_HDR_SDR;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_HDR_SDR::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl;
	return debugStream;
}
