/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_framestatusinfo524D.cpp
	@brief		Implements the AJAAncillaryData_FrameStatusInfo524D class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_framestatusinfo524D.h"
#include <ios>
#include <iomanip>

using namespace std;

#define AJAAncillaryData_FrameStatusInfo524D_PayloadSize 0x0B


AJAAncillaryData_FrameStatusInfo524D::AJAAncillaryData_FrameStatusInfo524D()
	:	AJAAncillaryData()
{
	Init();
}


AJAAncillaryData_FrameStatusInfo524D::AJAAncillaryData_FrameStatusInfo524D (const AJAAncillaryData_FrameStatusInfo524D & inClone)
	:	AJAAncillaryData()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_FrameStatusInfo524D::AJAAncillaryData_FrameStatusInfo524D (const AJAAncillaryData_FrameStatusInfo524D * pInClone)
	:	AJAAncillaryData()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_FrameStatusInfo524D::AJAAncillaryData_FrameStatusInfo524D (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	Init();
}


AJAAncillaryData_FrameStatusInfo524D::~AJAAncillaryData_FrameStatusInfo524D ()
{
}


void AJAAncillaryData_FrameStatusInfo524D::Init (void)
{
	m_ancType     = AJAAncillaryDataType_FrameStatusInfo524D;
	m_coding      = AJAAncillaryDataCoding_Digital;
	m_DID         = AJAAncillaryData_FrameStatusInfo524D_DID;
	m_SID         = AJAAncillaryData_FrameStatusInfo524D_SID;
	m_IsRecording = false;
}


void AJAAncillaryData_FrameStatusInfo524D::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_FrameStatusInfo524D & AJAAncillaryData_FrameStatusInfo524D::operator = (const AJAAncillaryData_FrameStatusInfo524D & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator= (rhs);

		// Copy the local members
		m_IsRecording = rhs.m_IsRecording;
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_FrameStatusInfo524D::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() != AJAAncillaryData_FrameStatusInfo524D_PayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	// This is valid for the Canon C300 and C500, and bit 1 (0x02) is
	// valid for the Sony F3.
	m_IsRecording  = (((m_payload[10] & 0x03) == 0) ? false : true);
	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_FrameStatusInfo524D::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_FrameStatusInfo524D_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_FrameStatusInfo524D_SID)
				if (pInAncData->GetDC()  == AJAAncillaryData_FrameStatusInfo524D_PayloadSize)
					return AJAAncillaryDataType_FrameStatusInfo524D;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_FrameStatusInfo524D::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl
				<< "Recording: " << (m_IsRecording  ?  "Active"  :  "Inactive");
	return debugStream;
}
