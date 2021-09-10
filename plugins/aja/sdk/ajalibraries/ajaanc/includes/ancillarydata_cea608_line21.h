/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_cea608_line21.h
	@brief		Declares the AJAAncillaryData_Cea608_line21 class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_CEA608_LINE21_H
#define AJA_ANCILLARYDATA_CEA608_LINE21_H

#include "ancillarydatafactory.h"
#include "ancillarydata.h"
#include "ancillarydata_cea608.h"


// Line 21 ("Analog") Packet IDs
const uint8_t	AJAAncillaryData_Cea608_Line21_DID = AJAAncillaryData_AnalogSID;
const uint8_t	AJAAncillaryData_Cea608_Line21_SID = AJAAncillaryData_AnalogDID;

const uint32_t  AJAAncillaryData_Cea608_Line21_PayloadSize = 720;		// note: assumes we're only using this for SD (720 pixels/line)



/**
	@brief	This class handles "analog" (Line 21) based CEA-608 caption data packets.
**/
class AJAExport AJAAncillaryData_Cea608_Line21 : public AJAAncillaryData_Cea608
{
public:
	AJAAncillaryData_Cea608_Line21 ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor.
		@param[in]	inClone	The object to be cloned.
	**/
	AJAAncillaryData_Cea608_Line21 (const AJAAncillaryData_Cea608_Line21 & inClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInClone	A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea608_Line21 (const AJAAncillaryData_Cea608_Line21 * pInClone);

	/**
		@brief	My copy constructor.
		@param[in]	pInData		A valid pointer to the object to be cloned.
	**/
	AJAAncillaryData_Cea608_Line21 (const AJAAncillaryData * pInData);

	virtual									~AJAAncillaryData_Cea608_Line21 ();	///< @brief		My destructor.

	virtual void							Clear (void);		///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Cea608_Line21 &		operator = (const AJAAncillaryData_Cea608_Line21 & inRHS);

	virtual inline AJAAncillaryData_Cea608_Line21 *	Clone (void) const	{return new AJAAncillaryData_Cea608_Line21 (this);}	///< @return	A clone of myself.

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
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@param[in]	pInAncData	A valid pointer to a base AJAAncillaryData object that contains the Anc data to inspect.
		@return		AJAAncillaryDataType if I recognize this Anc data (or AJAAncillaryDataType_Unknown if unrecognized).
	**/
	static AJAAncillaryDataType		RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);


protected:
	void				Init (void);	// NOT virtual - called by constructors
	virtual AJAStatus	AllocEncodeBuffer (void);

	// Encode methods ported/stolen from ntv2closedcaptioning.cpp

	/**
		@brief		Initializes the payload buffer with all of the "static" pixels, e.g. run-in clock, pre- and post- black, etc.
		@param[in]	inLineStartOffset	Pixel count from beginning of line (buffer) to the start of the waveform.
		@param[out]	outDataStartOffset	Receives the pixel count from beginning of line (buffer) to the first data pixel
										(required by the AJAAncillaryData_Cea608_Line21::EncodeLine function).
		@return		AJA_STATUS_SUCCESS if successful, or AJA_STATUS_FAIL if payload not allocated or wrong size.
	**/
	virtual AJAStatus	InitEncodeBuffer (const uint32_t inLineStartOffset, uint32_t & outDataStartOffset);


	/**
		@brief		Encode and insert the given 8-bit characters into the (already initialized) payload buffer.
		@param[in]	inChar1				Specifies the 8-bit "1st" character on the line.
		@param[in]	inChar2				Specifies the 8-bit "2nd" character on the line.
		@param[in]	inDataStartOffset	Specifies the pixel count from beginning of line (buffer) to the first data pixel.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus	EncodeLine (const uint8_t inChar1, const uint8_t inChar2, const uint32_t inDataStartOffset);

	/**
		@brief		Encodes a single 8-bit character from just after the transition to the first bit until just before the transition from the last bit.
		@param[in]	ptr			Pointer to the location in buffer where the encoding should start.
		@param[in]	inChar		Specifies the 8-bit character to encode.
		@return		Pointer to next pixel in the buffer following the end of the encoded byte.
	**/
	virtual uint8_t *	EncodeCharacter (uint8_t * ptr, const uint8_t inChar);


	/**
		@brief		Encodes a single bit transition from the "from" level to the "to" level.
		@param[in]	ptr				ptr to the location in buffer where the encoding should start
		@param[in]	inStartLevel	Specifies the beginning level (0=low, >0=high).
		@param[in]	inEndLevel		Specifies the ending level (0=low, >0=high).
		@return		Pointer to next pixel in the buffer following the end of the encoded transition.
	**/
	virtual uint8_t *	EncodeTransition (uint8_t * ptr, const uint8_t inStartLevel, const uint8_t inEndLevel);


	// Decode methods ported/stolen from ntv2closedcaptioning.cpp

	/**
		@brief		Decodes the payload to extract the two captioning characters.
					The caller must check \c outGotClock to determine whether a valid CEA-608 ("Line 21")
					waveform was found: lack of captioning waveform does NOT return an error!
		@param[out]	outChar1	Receives data byte 1 (set to 0xFF if no clock or data is found).
		@param[out]	outChar2	Receives data byte 2 (set to 0xFF if no clock or data is found).
		@param[out]	outGotClock	Receives 'true' if a valid CEA-608 ("Line 21") clock waveform was found.
		@return		AJA_STATUS_SUCCESS if successful, or AJA_STATUS_FAIL if payload not allocated or wrong size.
	**/
	virtual AJAStatus	DecodeLine (uint8_t & outChar1, uint8_t & outChar2, bool & outGotClock) const;


	/**
		@brief		Checks for the existence of a CEA-608 "analog" waveform and, if found, returns a pointer
					to the start of the data bits.
		@param[in]	pInLine		The start of the payload buffer.
		@param[out]	outGotClock	Receives 'true' if a valid CEA-608 ("Line 21") clock waveform was found.
		@return					Pointer to the middle of the first data bit (used by DecodeCharacters function).
	**/
	static const uint8_t *	CheckDecodeClock (const uint8_t * pInLine, bool & outGotClock);


	/**
		@brief		Decodes the two CEA-608 data characters for this line.
		@param[in]	ptr			Points to the middle of the first data bit in the waveform
								(i.e. the one following the '1' start bit).
		@param[out]	outChar1	Receives data byte 1.
		@param[out]	outChar2	Receives data byte 2.
		@return		AJA_STATUS_SUCCESS if successful.
		@note		This function returns the parity bit of each character in the MS bit position.
					It makes no calculation or value judgment as to the correctness of the parity.
	**/
	static AJAStatus	DecodeCharacters (const uint8_t * ptr, uint8_t & outChar1, uint8_t & outChar2);


protected:
	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!
	bool		m_bEncodeBufferInitialized;		///< @brief		Set 'true' after successfully allocating and initializing the m_payload buffer for encoding
	uint32_t	m_dataStartOffset;				///< @brief		Offset into the encode buffer where data starts

};	//	AJAAncillaryData_Cea608_Line21

#endif	// AJA_ANCILLARYDATA_CEA608_LINE21_H

