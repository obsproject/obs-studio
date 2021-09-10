/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608.cpp
	@brief		Declares the AJAAncillaryData_Cea608 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarydata_cea608.h"
#include <ios>
#include <iomanip>

using namespace std;


AJAAncillaryData_Cea608::AJAAncillaryData_Cea608() : AJAAncillaryData()
{
	Init();
}


AJAAncillaryData_Cea608::AJAAncillaryData_Cea608(const AJAAncillaryData_Cea608& clone) : AJAAncillaryData()
{
	Init();

	*this = clone;
}


AJAAncillaryData_Cea608::AJAAncillaryData_Cea608(const AJAAncillaryData_Cea608 *pClone) : AJAAncillaryData()
{
	Init();

	if (pClone != NULL_PTR)
		*this = *pClone;
}


AJAAncillaryData_Cea608::AJAAncillaryData_Cea608(const AJAAncillaryData *pData) : AJAAncillaryData(pData)
{
	Init();
}


AJAAncillaryData_Cea608::~AJAAncillaryData_Cea608()
{
}


void
AJAAncillaryData_Cea608::Init()
{
	 m_char1 = 0x80;	// CEA-608 "Null" character (with parity)
	 m_char2 = 0x80;
}


AJAAncillaryData_Cea608 & AJAAncillaryData_Cea608::operator = (const AJAAncillaryData_Cea608 & rhs)
{
	if (this != &rhs)		// ignore self-assignment
	{
		AJAAncillaryData::operator= (rhs);		// copy the base class stuff

		// copy the local stuff
		m_char1 = rhs.m_char1;
		m_char2 = rhs.m_char2;
	}
	return *this;
}


void AJAAncillaryData_Cea608::Clear (void)
{
	AJAAncillaryData::Clear();
	Init();
}


//------------------------------
// Get/Set 8-bit bytes: we assume the caller has already dealt with the parity

AJAStatus AJAAncillaryData_Cea608::SetCEA608Bytes (const uint8_t byte1, const uint8_t byte2)
{
	m_char1 = byte1;
	m_char2 = byte2;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryData_Cea608::GetCEA608Bytes (uint8_t & byte1, uint8_t & byte2, bool & bRcvdData) const
{
	byte1 = m_char1;
	byte2 = m_char2;
	bRcvdData = m_rcvDataValid;
	return AJA_STATUS_SUCCESS;
}


//------------------------------
// Get/Set 7-bit characters: automatically add/delete odd parity to bit 7

AJAStatus AJAAncillaryData_Cea608::SetCEA608Characters (uint8_t char1, uint8_t char2)
{
	m_char1 = AddOddParity (char1);
	m_char2 = AddOddParity (char2);
	return AJA_STATUS_SUCCESS;
}


uint8_t AJAAncillaryData_Cea608::AddOddParity (const uint8_t inChar)
{
	// add odd parity to bit 7
	uint8_t onesCount = 0;
	uint8_t shiftChar = inChar;

	// count the number of 1's in the ls 7 bits
	for (uint8_t i = 0; i < 7; i++)
	{
		if ((shiftChar & 0x01) != 0)
			onesCount++;

		shiftChar = shiftChar >> 1;
	}

	// set bit 7 to make the total number of 1's odd
	if ((onesCount % 2) == 1)		// we already have an odd number of 1's: set bit 7 = 0
		return (inChar & 0x7F);
	else							// we have an even number of 1's: set bit 7 = 1 to make the total odd
		return (inChar | 0x80);
}


AJAStatus AJAAncillaryData_Cea608::GetCEA608Characters (uint8_t & char1, uint8_t & char2, bool & bRcvdData) const
{
	char1 = m_char1 & 0x7F;		// strip parity bit before returning
	char2 = m_char2 & 0x7F;
	bRcvdData = m_rcvDataValid;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryData_Cea608::RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData)
{
	(void) pInAncData;
	// Since the AJAAncillaryData_Cea608 object has no "concrete" transport of its own,
	// this has to be done by its subclasses.
	return AJAAncillaryDataType_Unknown;
}

ostream & AJAAncillaryData_Cea608::Print (ostream & debugStream, const bool bShowDetail) const
{
	AJAAncillaryData::Print (debugStream, bShowDetail);
	const uint8_t	char1	(m_char1 & 0x7F);	// strip parity and see if we can print it a an ASCII character
	const uint8_t	char2	(m_char2 & 0x7F);

	debugStream << endl
				<< "Byte1=0x" << hex << setw(2) << setfill('0') << uint16_t(m_char1);
	if (char1 >= 0x20 && char1 < 0x7F)
		debugStream << " ('" << char1 << "')";

	debugStream << " Byte2=0x" << hex << setw(2) << setfill('0') << uint16_t(m_char2);
	if (char2 >= 0x20 && char2 < 0x7F)
		debugStream << " ('" << char2 << "')";
	return debugStream;
}
