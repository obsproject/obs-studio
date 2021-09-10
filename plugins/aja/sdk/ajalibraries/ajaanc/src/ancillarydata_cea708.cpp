/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea708.cpp
	@brief		Implements the AJAAncillaryData_Cea708 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_cea708.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Cea708::AJAAncillaryData_Cea708()
	:	AJAAncillaryData()
{
	Init();
}


AJAAncillaryData_Cea708::AJAAncillaryData_Cea708(const AJAAncillaryData_Cea708& clone)
	:	AJAAncillaryData()
{
	Init();
	*this = clone;
}


AJAAncillaryData_Cea708::AJAAncillaryData_Cea708(const AJAAncillaryData_Cea708 *pClone)
	:	AJAAncillaryData()
{
	Init();
	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Cea708::AJAAncillaryData_Cea708(const AJAAncillaryData *pData)
	:	AJAAncillaryData (pData)
{
	Init();
}


AJAAncillaryData_Cea708::~AJAAncillaryData_Cea708()
{
}


void AJAAncillaryData_Cea708::Init (void)
{
	m_ancType	 = AJAAncillaryDataType_Cea708;
}


AJAAncillaryData_Cea708 & AJAAncillaryData_Cea708::operator = (const AJAAncillaryData_Cea708 & rhs)
{
	if (this != &rhs)		// ignore self-assignment
		AJAAncillaryData::operator= (rhs);		// copy the base class stuff
	return *this;
}


void AJAAncillaryData_Cea708::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


AJAStatus AJAAncillaryData_Cea708::ParsePayloadData (void)
{
	if (IsEmpty())
	{
		Init();						// load default values
		m_rcvDataValid = false;
		return AJA_STATUS_FAIL;
	}

	// we have some kind of payload data - try to parse it
	m_rcvDataValid = true;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea708::GeneratePayloadData (void)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	m_DID = AJAAncillaryData_CEA708_DID;
	m_SID = AJAAncillaryData_CEA708_SID;

//	status = AllocDataMemory(AJAAncillaryData_SMPTE2016_3_PayloadSize);

	if (AJA_SUCCESS(status))
	{
	}

	m_checksum = Calculate8BitChecksum();
	return status;
}

#define	LOGMYWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)

AJAAncillaryDataType AJAAncillaryData_Cea708::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetLocationVideoSpace() == AJAAncillaryDataSpace_VANC)		//	Must be VANC (per SMPTE 334-2)
		if (pInAncData->GetDID() == AJAAncillaryData_CEA708_DID)				//	DID == 0x61
			if (pInAncData->GetSID() == AJAAncillaryData_CEA708_SID)			//	SDID == 0x01
				if (IS_VALID_AJAAncillaryDataVideoStream(pInAncData->GetLocationDataChannel()))	//	Valid channel?
				{
					if (pInAncData->GetLocationDataChannel() == AJAAncillaryDataChannel_C)	//	Y OK, Y+C OK
						LOGMYWARN("CEA708 packet on C-channel");	//	Violates SMPTE 334-1 sec 4 -- bad praxis, but will allow
					return AJAAncillaryDataType_Cea708;
				}
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_Cea708::Print (ostream & debugStream, const bool bShowDetail) const
{
	debugStream << IDAsString() << "(" << ::AJAAncillaryDataCodingToString (m_coding) << ")" << endl;
	return AJAAncillaryData::Print (debugStream, bShowDetail);
}
