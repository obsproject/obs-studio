/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608_vanc.h
	@brief		Declares the AJAAncillaryData_Cea608_Vanc class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_CEA608_VANC_H
#define AJA_ANCILLARYDATA_CEA608_VANC_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"
#include "ancillarydata_cea608.h"


// CEA-608 (SMPTE 334) Ancillary Packet
const uint8_t	AJAAncillaryData_Cea608_Vanc_DID = 0x61;
const uint8_t	AJAAncillaryData_Cea608_Vanc_SID = 0x02;

const uint32_t  AJAAncillaryData_Cea608_Vanc_PayloadSize = 3;	// constant 3 bytes


/**
	@brief	This class handles VANC-based CEA-608 caption data packets (not "analog" Line 21).
**/
class AJAExport AJAAncillaryData_Cea608_Vanc : public AJAAncillaryData_Cea608
{
public:
	AJAAncillaryData_Cea608_Vanc ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData_Cea608_Vanc * pInClone);

	/**
		@brief	Constructs me from a generic AJAAncillaryData object.
		@param[in]	pInData	A valid pointer to the AJAAncillaryData object.
	**/
	AJAAncillaryData_Cea608_Vanc (const AJAAncillaryData * pInData);

	virtual									~AJAAncillaryData_Cea608_Vanc ();	///< @brief		My destructor.

	virtual void							Clear (void);						///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Cea608_Vanc &			operator = (const AJAAncillaryData_Cea608_Vanc & inRHS);

	virtual inline AJAAncillaryData_Cea608_Vanc *	Clone (void) const	{return new AJAAncillaryData_Cea608_Vanc (this);}	///< @return	A clone of myself.

	/**
		@brief		Sets my SMPTE 334 (CEA608) field/line numbers.
		@param[in]	inIsF2		Specifies the field number ('true' for F2, 'false' for F1).
		@param[in]	inLineNum	Specifies the line number (see SMPTE 334-1 for details).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLine (const bool inIsF2, const uint8_t inLineNum);

	virtual inline uint16_t					GetLineNumber (void) const	{return m_lineNum;}		///< @return	My current SMPTE 334 (CEA608) line number.
	virtual inline bool						IsField2 (void) const		{return m_isF2;}		///< @return	True if my current Field ID is Field 2;  otherwise false.

	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ParsePayloadData (void);

	/**
		@brief		Generate the payload data from my "local" ancillary data.
		@note		This method is overridden for the specific Anc data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GeneratePayloadData (void);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@param[in]	pInAncData	A valid pointer to a base AJAAncillaryData object that contains the Anc data to inspect.
		@return		AJAAncillaryDataType if I recognize this Anc data (or AJAAncillaryDataType_Unknown if unrecognized).
	**/
	static AJAAncillaryDataType				RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);


	virtual AJAStatus						GetLine (uint8_t & outFieldNum, uint8_t & outLineNum) const;	///< @deprecated	Use AJAAncillaryData_Cea608_Vanc::GetLineNumber or AJAAncillaryData_Cea608_Vanc::IsField2 instead.

protected:
	void		Init (void);	// NOT virtual - called by constructors

	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!
	bool		m_isF2;			// F2 if true;  otherwise F1
	uint8_t		m_lineNum;		// 525i: 0 = line 9 (field 1) or line 272 (field 2)
								// 625i: 0 = line 5 (field 1) or line 318 (field 2)

};	//	AJAAncillaryData_Cea608_Vanc

#endif	// AJA_ANCILLARYDATA_CEA608_VANC_H
