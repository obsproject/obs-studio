/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode_atc.cpp
	@brief		Implements the AJAAncillaryData_Timecode_ATC class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_timecode_atc.h"
#include "ntv2publicinterface.h"
#include <ios>
#include <iomanip>

using namespace std;



AJAAncillaryData_Timecode_ATC::AJAAncillaryData_Timecode_ATC() : AJAAncillaryData_Timecode()
{
	Init();
}


AJAAncillaryData_Timecode_ATC::AJAAncillaryData_Timecode_ATC(const AJAAncillaryData_Timecode_ATC& clone) : AJAAncillaryData_Timecode()
{
	Init();

	*this = clone;
}


AJAAncillaryData_Timecode_ATC::AJAAncillaryData_Timecode_ATC(const AJAAncillaryData_Timecode_ATC *pClone) : AJAAncillaryData_Timecode()
{
	Init();

	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Timecode_ATC::AJAAncillaryData_Timecode_ATC(const AJAAncillaryData *pData) : AJAAncillaryData_Timecode(pData)
{
	Init();
}


AJAAncillaryData_Timecode_ATC::~AJAAncillaryData_Timecode_ATC()
{
}


void AJAAncillaryData_Timecode_ATC::Init()
{
	m_ancType = AJAAncillaryDataType_Timecode_ATC;
	m_coding  = AJAAncillaryDataCoding_Digital;
	m_DID	  = AJAAncillaryData_SMPTE12M_DID;
	m_SID	  = AJAAncillaryData_SMPTE12M_SID;

	m_dbb1 = 0;
	m_dbb2 = 0;
}


AJAAncillaryData_Timecode_ATC & AJAAncillaryData_Timecode_ATC::operator = (const AJAAncillaryData_Timecode_ATC & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		AJAAncillaryData_Timecode::operator= (rhs);		// copy the base class stuff

		m_dbb1 = rhs.m_dbb1;
		m_dbb2 = rhs.m_dbb2;
	}

	return *this;
}


void AJAAncillaryData_Timecode_ATC::Clear()
{
	AJAAncillaryData_Timecode::Clear();

	Init();
}


AJAStatus AJAAncillaryData_Timecode_ATC::SetDBB1(uint8_t dbb1)
{
	m_dbb1 = dbb1;

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::GetDBB1 (uint8_t & dbb1) const
{
	dbb1 = m_dbb1;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::SetDBB2 (uint8_t dbb2)
{
	m_dbb2 = dbb2;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::GetDBB2 (uint8_t & dbb2) const
{
	dbb2 = m_dbb2;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::SetDBB (uint8_t dbb1, uint8_t dbb2)
{
	SetDBB1(dbb1);
	SetDBB2(dbb2);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::GetDBB (uint8_t & dbb1, uint8_t & dbb2) const
{
	GetDBB1(dbb1);
	GetDBB2(dbb2);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Timecode_ATC::ParsePayloadData (void)
{
	AJAStatus status = AJA_STATUS_SUCCESS;

	// reality check...
	if (GetDC() < AJAAncillaryData_SMPTE12M_PayloadSize)
	{
		Init();						// load default values
		status = AJA_STATUS_FAIL;
		m_rcvDataValid = false;
	}
	else
	{
		// we have some kind of payload data - try to parse it
		// extract the time digits from the even payload words, bits[7:4]
		// (note: SetTimeHexValue() does the needed masking)
		SetTimeHexValue(kTcFrameUnits,  (m_payload[ 0] >> 4));		// frame units);
		SetTimeHexValue(kTcFrameTens,   (m_payload[ 2] >> 4));		// frame tens
		SetTimeHexValue(kTcSecondUnits, (m_payload[ 4] >> 4));		// second units
		SetTimeHexValue(kTcSecondTens,  (m_payload[ 6] >> 4));		// second tens
		SetTimeHexValue(kTcMinuteUnits, (m_payload[ 8] >> 4));		// minute units
		SetTimeHexValue(kTcMinuteTens,  (m_payload[10] >> 4));		// minute tens
		SetTimeHexValue(kTcHourUnits,   (m_payload[12] >> 4));		// hour units
		SetTimeHexValue(kTcHourTens,    (m_payload[14] >> 4));		// hour tens

		// extract the binary group values from the odd payload words, bits[7:4]
		// (note: SetBinaryGroupHexValue() does the needed masking)
		SetBinaryGroupHexValue(kBg1, (m_payload[ 1] >> 4));		// BG 1
		SetBinaryGroupHexValue(kBg2, (m_payload[ 3] >> 4));		// BG 2
		SetBinaryGroupHexValue(kBg3, (m_payload[ 5] >> 4));		// BG 3
		SetBinaryGroupHexValue(kBg4, (m_payload[ 7] >> 4));		// BG 4
		SetBinaryGroupHexValue(kBg5, (m_payload[ 9] >> 4));		// BG 5
		SetBinaryGroupHexValue(kBg6, (m_payload[11] >> 4));		// BG 6
		SetBinaryGroupHexValue(kBg7, (m_payload[13] >> 4));		// BG 7
		SetBinaryGroupHexValue(kBg8, (m_payload[15] >> 4));		// BG 8

		// extract the distributed bits from bit[3] of all UDW
		uint8_t i;
		uint8_t dbb = 0;		// DBB1 comes from UDW1 - UDW8 bit 3, ls bit first
		for (i = 0; i < 8; i++)
		{
			dbb = dbb >> 1;
			dbb |= (m_payload[i] << 4) & 0x80;
		}
		m_dbb1 = dbb;

		dbb = 0;				// DBB2 comes from UDW9 - UDW16 bit 3, ls bit first
		for (i = 8; i < 16; i++)
		{
			dbb = dbb >> 1;
			dbb |= (m_payload[i] << 4) & 0x80;
		}
		m_dbb2 = dbb;
		m_rcvDataValid = true;
	}

	return status;
}


AJAStatus AJAAncillaryData_Timecode_ATC::GeneratePayloadData (void)
{
	SetDID(AJAAncillaryData_SMPTE12M_DID);
	SetSID(AJAAncillaryData_SMPTE12M_SID);
	SetLocationHorizOffset(AJAAncDataHorizOffset_AnyHanc);

	AJAStatus status = AllocDataMemory(AJAAncillaryData_SMPTE12M_PayloadSize);
	if (AJA_FAILURE(status))
		return status;

	// time digits in the even payload words
	m_payload[ 0] = (m_timeDigits[kTcFrameUnits]  & 0x0F) << 4;
	m_payload[ 2] = (m_timeDigits[kTcFrameTens]   & 0x0F) << 4;
	m_payload[ 4] = (m_timeDigits[kTcSecondUnits] & 0x0F) << 4;
	m_payload[ 6] = (m_timeDigits[kTcSecondTens]  & 0x0F) << 4;
	m_payload[ 8] = (m_timeDigits[kTcMinuteUnits] & 0x0F) << 4;
	m_payload[10] = (m_timeDigits[kTcMinuteTens]  & 0x0F) << 4;
	m_payload[12] = (m_timeDigits[kTcHourUnits]   & 0x0F) << 4;
	m_payload[14] = (m_timeDigits[kTcHourTens]    & 0x0F) << 4;

	// binary group data in the odd payload words
	m_payload[ 1] = (m_binaryGroup[kBg1] & 0x0F) << 4;	
	m_payload[ 3] = (m_binaryGroup[kBg2] & 0x0F) << 4;
	m_payload[ 5] = (m_binaryGroup[kBg3] & 0x0F) << 4;
	m_payload[ 7] = (m_binaryGroup[kBg4] & 0x0F) << 4;
	m_payload[ 9] = (m_binaryGroup[kBg5] & 0x0F) << 4;
	m_payload[11] = (m_binaryGroup[kBg6] & 0x0F) << 4;
	m_payload[13] = (m_binaryGroup[kBg7] & 0x0F) << 4;
	m_payload[15] = (m_binaryGroup[kBg8] & 0x0F) << 4;

	// add the distributed bits
	uint8_t i;
	uint8_t dbb = m_dbb1;		// DBB1 goes into UDW1 - 8, ls bit first
	for (i = 0; i < 8; i++)
	{
		m_payload[i] |= (dbb & 0x01) << 3;
		dbb = dbb >> 1;
	}

	dbb = m_dbb2;				// DBB2 goes into UDW9 - 16, ls bit first
	for (i = 8; i < 16; i++)
	{
		m_payload[i] |= (dbb & 0x01) << 3;
		dbb = dbb >> 1;
	}

	m_checksum = Calculate8BitChecksum();
	return AJA_STATUS_SUCCESS;
}



AJAStatus AJAAncillaryData_Timecode_ATC::SetDBB1PayloadType (const AJAAncillaryData_Timecode_ATC_DBB1PayloadType inType)
{
	if (inType != AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2)
		SetLocationLineNumber(9);	//	All but Field2 should go on line 9
	return SetDBB1(uint8_t(inType));
}


AJAStatus AJAAncillaryData_Timecode_ATC::GetDBB1PayloadType (AJAAncillaryData_Timecode_ATC_DBB1PayloadType & outType) const
{
	uint8_t dbb1;
	GetDBB1 (dbb1);
	outType = AJAAncillaryData_Timecode_ATC_DBB1PayloadType_Unknown;

	switch (dbb1)
	{
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_ReaderFilmData:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_ReaderProdData:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalVideoData:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalFilmData:
		case AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalProdData:
			outType = AJAAncillaryData_Timecode_ATC_DBB1PayloadType (dbb1);
			break;

		default:
			break;
	}
	return AJA_STATUS_SUCCESS;
}



AJAAncillaryDataType AJAAncillaryData_Timecode_ATC::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (pInAncData->GetDataCoding() == AJAAncillaryDataCoding_Digital)
		if (pInAncData->GetDID() == AJAAncillaryData_SMPTE12M_DID)
			if (pInAncData->GetSID() == AJAAncillaryData_SMPTE12M_SID)
				if (pInAncData->GetDC() == AJAAncillaryData_SMPTE12M_PayloadSize)
					return AJAAncillaryDataType_Timecode_ATC;
	return AJAAncillaryDataType_Unknown;
}


ostream & AJAAncillaryData_Timecode_ATC::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData_Timecode::Print (debugStream, bShowDetail);	// print the generic stuff
	debugStream << endl
				<< "DBB1: " << xHEX0N(uint16_t(m_dbb1),2) << endl
				<< "DBB2: " << xHEX0N(uint16_t(m_dbb2),2);
	return debugStream;
}
