/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_framestatusinfo5251.cpp
	@brief		Implements the AJAAncillaryData_FrameStatusInfo5251 class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_framestatusinfo5251.h"
#include <ios>
#include <iomanip>

using namespace std;


#define AJAAncillaryData_FrameStatusInfo5251_PayloadSize 0x08


AJAAncillaryData_FrameStatusInfo5251::AJAAncillaryData_FrameStatusInfo5251 ()
	:	AJAAncillaryData ()
{
	Init();
}


AJAAncillaryData_FrameStatusInfo5251::AJAAncillaryData_FrameStatusInfo5251 (const AJAAncillaryData_FrameStatusInfo5251 & inClone)
	:	AJAAncillaryData ()
{
	Init();
	*this = inClone;
}


AJAAncillaryData_FrameStatusInfo5251::AJAAncillaryData_FrameStatusInfo5251 (const AJAAncillaryData_FrameStatusInfo5251 * pInClone)
	:	AJAAncillaryData ()
{
	Init();
	if (pInClone)
		*this = *pInClone;
}


AJAAncillaryData_FrameStatusInfo5251::AJAAncillaryData_FrameStatusInfo5251 (const AJAAncillaryData * pInData)
	:	AJAAncillaryData (pInData)
{
	Init();
}


AJAAncillaryData_FrameStatusInfo5251::~AJAAncillaryData_FrameStatusInfo5251 ()
{
}


void AJAAncillaryData_FrameStatusInfo5251::Init (void)
{
	m_ancType      = AJAAncillaryDataType_FrameStatusInfo5251;
	m_coding       = AJAAncillaryDataCoding_Digital;
	m_DID          = AJAAncillaryData_FrameStatusInfo5251_DID;
	m_SID          = AJAAncillaryData_FrameStatusInfo5251_SID;
	m_IsRecording  = false;
	m_IsValidFrame = true;
}


void AJAAncillaryData_FrameStatusInfo5251::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAAncillaryData_FrameStatusInfo5251 & AJAAncillaryData_FrameStatusInfo5251::operator = (const AJAAncillaryData_FrameStatusInfo5251 & rhs)
{
	// Ignore self-assignment
	if (this != &rhs)
	{
		// Copy the base class members
		AJAAncillaryData::operator=(rhs);

		// Copy the local members
		m_IsRecording  = rhs.m_IsRecording;
		m_IsValidFrame = rhs.m_IsValidFrame;
	}
	return *this;
}
	

AJAStatus AJAAncillaryData_FrameStatusInfo5251::ParsePayloadData (void)
{
	// The size is specific to Canon
	if (GetDC() != AJAAncillaryData_FrameStatusInfo5251_PayloadSize)
	{
		// Load default values
		Init();
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	// This is valid for the Canon C500
	m_IsRecording  = (((m_payload[0] & 0x60) == 0x20) ? true : false);

	// This is in the Canon specification, but the Canon C500 doesn't set
	// this, thus we're placing this here just for completion in order 
	// to follow the DID:52h SDID:51h packet format.
	m_IsValidFrame = (((m_payload[0] & 0x80) == 0x00) ? true : false);

	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_FrameStatusInfo5251::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_FrameStatusInfo5251_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_FrameStatusInfo5251_SID)
				if (pInAncData->GetDC()  == AJAAncillaryData_FrameStatusInfo5251_PayloadSize)
					return AJAAncillaryDataType_FrameStatusInfo5251;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_FrameStatusInfo5251::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	debugStream << endl
				<< "Recording: " <<  (m_IsRecording  ?  "Active"  :  "Inactive");
	return debugStream;
}
