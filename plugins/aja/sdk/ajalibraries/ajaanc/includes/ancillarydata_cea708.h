/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea708.h
	@brief		Declares the AJAAncillaryData_Cea708 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_CEA708_H
#define AJA_ANCILLARYDATA_CEA708_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"


// SMPTE 334 Ancillary Packet
const uint8_t	AJAAncillaryData_CEA708_DID = 0x61;
const uint8_t	AJAAncillaryData_CEA708_SID = 0x01;


/**
	@brief	This class handles CEA-708 SMPTE 334 packets.
**/
class AJAExport AJAAncillaryData_Cea708 : public AJAAncillaryData
{
public:
	AJAAncillaryData_Cea708 ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone		The object to be cloned.
	**/
	AJAAncillaryData_Cea708 (const AJAAncillaryData_Cea708 & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea708 (const AJAAncillaryData_Cea708 * pInClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInData		A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea708 (const AJAAncillaryData * pInData);

	virtual										~AJAAncillaryData_Cea708 ();	///< @brief		My destructor.

	virtual void								Clear (void);					///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Cea708 &			operator = (const AJAAncillaryData_Cea708 & inRHS);

	virtual inline AJAAncillaryData_Cea708 *	Clone (void) const	{return new AJAAncillaryData_Cea708 (this);}	///< @return	A clone of myself.

	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus							ParsePayloadData (void);

	/**
		@brief		Generate the payload data from the "local" ancillary data.
		@note		This method is overridden for the specific Anc data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus							GeneratePayloadData (void);

	/**
		@param[in]	pInAncData	A valid pointer to a base AJAAncillaryData object that contains the Anc data to inspect.
		@return		AJAAncillaryDataType if I recognize this Anc data (or AJAAncillaryDataType_Unknown if unrecognized).
	**/
	static AJAAncillaryDataType					RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &						Print (std::ostream & inOutStream, const bool inDetailed = false) const;

protected:
	void		Init (void);	// NOT virtual - called by constructors

	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!
};	//	AJAAncillaryData_Cea708

#endif	// AJA_ANCILLARYDATA_CEA708_H
