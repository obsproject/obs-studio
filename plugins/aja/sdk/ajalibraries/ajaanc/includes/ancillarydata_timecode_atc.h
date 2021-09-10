/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode_atc.h
	@brief		Declares the AJAAncillaryData_Timecode_ATC class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_TIMECODE_ATC_H
#define AJA_ANCILLARYDATA_TIMECODE_ATC_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"
#include "ancillarydata_timecode.h"


// SMPTE 12M Ancillary Packet (formerly "RP-188")
const uint8_t	AJAAncillaryData_SMPTE12M_DID			= 0x60;
const uint8_t	AJAAncillaryData_SMPTE12M_SID			= 0x60;
const uint32_t	AJAAncillaryData_SMPTE12M_PayloadSize	= 16;	// constant 16 bytes


// see SMPTE 12M-2 Table 2
enum AJAAncillaryData_Timecode_ATC_DBB1PayloadType
{
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC				= 0x00,
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1				= 0x01,
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2				= 0x02,

	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_ReaderFilmData	= 0x06,
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_ReaderProdData	= 0x07,

	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalVideoData	= 0x7D,
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalFilmData		= 0x7E,
	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LocalProdData		= 0x7F,

	AJAAncillaryData_Timecode_ATC_DBB1PayloadType_Unknown			= 0xFF
};


/**
	@brief	I am the ATC-specific (analog) subclass of the AJAAncillaryData_Timecode class.
**/
class AJAExport AJAAncillaryData_Timecode_ATC : public AJAAncillaryData_Timecode
{
public:
	AJAAncillaryData_Timecode_ATC ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The object to be cloned.
	**/
	AJAAncillaryData_Timecode_ATC (const AJAAncillaryData_Timecode_ATC & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Timecode_ATC (const AJAAncillaryData_Timecode_ATC * pInClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInData		A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Timecode_ATC (const AJAAncillaryData * pInData);

	virtual										~AJAAncillaryData_Timecode_ATC ();	///< @brief		My destructor.

	virtual void								Clear (void);						///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Timecode_ATC &			operator = (const AJAAncillaryData_Timecode_ATC & inRHS);

	virtual inline AJAAncillaryData_Timecode_ATC *	Clone (void) const	{return new AJAAncillaryData_Timecode_ATC (this);}	///< @return	A clone of myself.


	/**
	 *	Set/Get the Distributed Binary Bits
	 *
	 *	@param[in]	dbb1		DBB bits 
	 *	@return					AJA_STATUS_SUCCESS
	 */
	virtual AJAStatus SetDBB1(uint8_t  dbb1);
	virtual AJAStatus GetDBB1(uint8_t& dbb1) const;

	virtual AJAStatus SetDBB2(uint8_t  dbb2);
	virtual AJAStatus GetDBB2(uint8_t& dbb2) const;

	virtual AJAStatus SetDBB(uint8_t  dbb1, uint8_t  dbb2);
	virtual AJAStatus GetDBB(uint8_t& dbb1, uint8_t& dbb2) const;


	/**
		@brief		Sets my payload type.
		@param[in]	inType		Specifies the payload type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				SetDBB1PayloadType (const AJAAncillaryData_Timecode_ATC_DBB1PayloadType inType);

	/**
		@brief		Answers with my current payload type.
		@param[out]	outType		Receives my payload type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				GetDBB1PayloadType (AJAAncillaryData_Timecode_ATC_DBB1PayloadType & outType) const;

	/**
		@brief		Parses out (interprets) the "local" ancillary data from my payload data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				ParsePayloadData (void);

	/**
		@brief		Generate the payload data from the "local" ancillary data.
		@note		This method is overridden for the specific Anc data type.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				GeneratePayloadData (void);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &			Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@param[in]	pInAncData	A valid pointer to a base AJAAncillaryData object that contains the Anc data to inspect.
		@return		AJAAncillaryDataType if I recognize this Anc data (or AJAAncillaryDataType_Unknown if unrecognized).
	**/
	static AJAAncillaryDataType		RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);

protected:
	void		Init (void);	// NOT virtual - called by constructors

	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!
	uint8_t		m_dbb1;			// "Distributed Binary Bits" - only transported by ATC
	uint8_t		m_dbb2;			//

};	//	AJAAncillaryData_Timecode_ATC

#endif	// AJA_ANCILLARYDATA_TIMECODE_ATC_H

