/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608.h
	@brief		Declares the AJAAncillaryData_Cea608 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_CEA608_H
#define AJA_ANCILLARYDATA_CEA608_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"


/**
	@brief	This is the base class for handling CEA-608 caption data packets.
**/
class AJAExport AJAAncillaryData_Cea608 : public AJAAncillaryData
{
public:
	AJAAncillaryData_Cea608 ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The object to be cloned.
	**/
	AJAAncillaryData_Cea608 (const AJAAncillaryData_Cea608 & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea608 (const AJAAncillaryData_Cea608 * pInClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInData		A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea608 (const AJAAncillaryData * pInData);

	virtual										~AJAAncillaryData_Cea608 ();	///< @brief		My destructor.

	virtual void								Clear (void);					///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Cea608 &			operator = (const AJAAncillaryData_Cea608 & inRHS);


	virtual inline AJAAncillaryData_Cea608 *	Clone (void) const	{return new AJAAncillaryData_Cea608 (this);}	///< @return	A clone of myself.

	/**
		@brief		Set the CEA608 payload bytes.
		@note		This function assumes the caller has already added parity.
		@param[in]	inByte1		Specifies the first byte of the pair.
		@param[in]	inByte2		Specifies the second byte of the pair.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				SetCEA608Bytes (const uint8_t  inByte1, const uint8_t  inByte2);

	/**
		@brief		Answers with the CEA608 payload bytes.
		@param[out]	outByte1		Receives the first byte of the pair (includes parity).
		@param[out]	outByte2		Receives the second byte of the pair (includes parity).
		@param[out]	outIsValid		Receives 'true' if returned data is valid.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				GetCEA608Bytes (uint8_t & outByte1, uint8_t & outByte2, bool & outIsValid) const;


	/**
		@brief		Sets the CEA608 payload characters. Uses the least significant 7 bits of the input values
					and adds odd parity.
		@param[in]	inChar1		Specifies the first character of the pair.
		@param[in]	inChar2		Specifies the second character of the pair.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				SetCEA608Characters (const uint8_t  inChar1, const uint8_t  inChar2);

	/**
		@brief		Answers with the CEA608 payload characters.
		@param[out]	outChar1	Receives the first character of the pair (after stripping odd parity).
		@param[out]	outChar2	Receives the second character of the pair (after stripping odd parity).
		@param[out]	outIsValid	Receives 'true' if the returned characters are valid;  otherwise 'false'.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus				GetCEA608Characters (uint8_t & outChar1, uint8_t & outChar2, bool & outIsValid) const;

	/**
		@brief		Set/Clear bit 7 of a byte to make odd parity.
		@param[in]	inValue		Specifies the 7-bit input value.
		@return		The least-significant 7 bits of the input byte with bit 7 set or cleared to make odd parity.
	**/
	static uint8_t					AddOddParity (const uint8_t inValue);

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
	uint8_t		m_char1;			// the 1st character in this field
	uint8_t		m_char2;			// the 2nd character in this field

};	//	AJAAncillaryData_Cea608

#endif	// AJA_ANCILLARYDATA_CEA608_H

