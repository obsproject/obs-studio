/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode_vitc.h
	@brief		Declares the AJAAncillaryData_Timecode_VITC class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_TIMECODE_VITC_H
#define AJA_ANCILLARYDATA_TIMECODE_VITC_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"
#include "ancillarydata_timecode.h"


// comment this out to use older "NTV2" VITC Encode algorithm
// leave defined to use "D-VITC" parameters as defined in SMPTE-266M
#define USE_SMPTE_266M


// VITC ("Analog") Packet IDs
const uint8_t	AJAAncillaryData_VITC_DID = AJAAncillaryData_AnalogSID;
const uint8_t	AJAAncillaryData_VITC_SID = AJAAncillaryData_AnalogDID;

const uint32_t  AJAAncillaryData_VITC_PayloadSize = 720;	// note: assumes we're only using this for SD (720 pixels/line)


enum AJAAncillaryData_Timecode_VITC_Type
{
	AJAAncillaryData_Timecode_VITC_Type_Unknown		= 0,
	AJAAncillaryData_Timecode_VITC_Type_Timecode	= 1,	// CRC == 0x00
	AJAAncillaryData_Timecode_VITC_Type_FilmData	= 2,	// RP-201 Film Data (CRC == 0xFF)
	AJAAncillaryData_Timecode_VITC_Type_ProdData	= 3		// RP-201 Production Data (CRC == 0x0F)
};



/**
	@brief	This is the VITC-specific subclass of the AJAAncillaryData_Timecode class.
**/
class AJAExport AJAAncillaryData_Timecode_VITC : public AJAAncillaryData_Timecode
{
public:
											AJAAncillaryData_Timecode_VITC ();
											AJAAncillaryData_Timecode_VITC (const AJAAncillaryData_Timecode_VITC & clone);
											AJAAncillaryData_Timecode_VITC (const AJAAncillaryData_Timecode_VITC * pClone);
											AJAAncillaryData_Timecode_VITC (const AJAAncillaryData * pData);

	virtual									~AJAAncillaryData_Timecode_VITC ()		{}

	virtual void							Clear (void);		///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Timecode_VITC &		operator = (const AJAAncillaryData_Timecode_VITC & inRHS);

	virtual inline AJAAncillaryData_Timecode_VITC *	Clone (void) const	{return new AJAAncillaryData_Timecode_VITC (this);}	///< @return	A clone of myself.

	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ParsePayloadData (void);

	/**
		@brief		Generate the payload data from the "local" ancillary data.
		@note		This method is overridden for the specific Anc data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GeneratePayloadData (void);

	virtual inline AJAAncillaryData_Timecode_VITC_Type	GetVITCDataType (void) const	{return m_vitcType;}	///< @return	The "type" of received VITC data based on the parsed CRC.

	/**
		@brief		Sets my VITC data type.
		@param[in]	inType	Specifies my new VITC data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus					SetVITCDataType (const AJAAncillaryData_Timecode_VITC_Type inType);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &				Print (std::ostream & inOutStream, const bool inDetailed = false) const;


	/**
		@param[in]	pInAncData	A valid pointer to an AJAAncillaryData instance.
		@return		The AJAAncillaryDataType if I recognize this ancillary data (or unknown if unrecognized).
	**/
	static AJAAncillaryDataType			RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);

	/**
		@param[in]	inType	Specifies the VITC data type.
		@return		A string containing a human-readable representation of the given VITC data type.
	**/
	static std::string					VITCTypeToString (const AJAAncillaryData_Timecode_VITC_Type inType);


protected:
	void		Init (void);	// NOT virtual - called by constructors

	// Encode methods ported/stolen from ntv2vitc.cpp
	bool		DecodeLine (const uint8_t * pInLine);
	AJAStatus	EncodeLine (uint8_t * pOutLine) const;

#ifdef USE_SMPTE_266M
#else
	void DoVITCBitPair(uint8_t *pLine, uint32_t& pixelIndex, bool bPrevBit, bool bBit0, bool bBit1, bool bDropBit);
#endif

	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!
	AJAAncillaryData_Timecode_VITC_Type	m_vitcType;		///< @brief	The "type" of VITC received or to be transmitted

};	//	AJAAncillaryData_Timecode_VITC

#endif	// AJA_ANCILLARYDATA_TIMECODE_VITC_H
