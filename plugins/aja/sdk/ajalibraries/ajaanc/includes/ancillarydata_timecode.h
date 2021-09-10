/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata_timecode.h
	@brief		Declares the AJAAncillaryData_Timecode class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_TIMECODE_H
#define AJA_ANCILLARYDATA_TIMECODE_H

#include "ajabase/common/timecode.h"
#include "ajabase/common/timebase.h"
#include "ancillarydatafactory.h"
#include "ancillarydata.h"


enum AJAAncillaryData_Timecode_Format
{
	AJAAncillaryData_Timecode_Format_Unknown,	// not set (usually defaults to 30 fps)

	AJAAncillaryData_Timecode_Format_60fps,	// 60/59.94 fps format ("NTSC")
	AJAAncillaryData_Timecode_Format_50fps,	// 50 fps format ("PAL")
	AJAAncillaryData_Timecode_Format_48fps,	// 48/47.95 fps format
	AJAAncillaryData_Timecode_Format_30fps,	// 30/29.97 fps format ("NTSC")
	AJAAncillaryData_Timecode_Format_25fps,	// 25 fps format ("PAL")
	AJAAncillaryData_Timecode_Format_24fps	// 24/23.98 fps format
};


/**
	@brief	This is the base class for the AJAAncillaryData_Timecode_ATC and AJAAncillaryData_Timecode_VITC
			classes, because they share the same "payload" data (i.e. timecode) and only differ in the transport
			(ancillary packets vs. "analog" coding).
	@note	Do not instantiate a "pure" AJAAncillaryData_Timecode object. Always use the subclasses.
**/
class AJAExport AJAAncillaryData_Timecode : public AJAAncillaryData
{
public:
											AJAAncillaryData_Timecode ();
											AJAAncillaryData_Timecode (const AJAAncillaryData_Timecode & inClone);
											AJAAncillaryData_Timecode (const AJAAncillaryData_Timecode * pClone);
											AJAAncillaryData_Timecode (const AJAAncillaryData *pData);

	virtual inline							~AJAAncillaryData_Timecode ()	{}

	virtual void							Clear (void);		///< @brief	Frees my allocated memory, if any, and resets my members to their default values.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryData_Timecode &		operator = (const AJAAncillaryData_Timecode & inRHS);

	virtual inline AJAAncillaryData_Timecode *		Clone (void) const	{return new AJAAncillaryData_Timecode (this);}	///< @return	A clone of myself.

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
	virtual AJAStatus						GeneratePayloadData (void)					{return AJA_STATUS_SUCCESS;}


	/**
		@brief		Sets my raw "time" hex values.
		@param[in]	inDigitNum	Specifies the index (0-7) of time digits in "transmission" order:
								0 = 1st (frame units) digit, 1 = 2nd digit, ... 7 = last (hour tens) digit.
		@param[in]	inHexValue	Specifies the hex value (least significant 4 bits) to be set.
		@param[in]	inMask		Optionally specifies which bits to set: (1 = set bit to new value, 0 = retain current bit value).
								Defaults to 0x0F.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetTimeHexValue (const uint8_t inDigitNum, const uint8_t inHexValue, const uint8_t inMask = 0x0f);

	/**
		@brief		Answers with my current raw "time" hex values.
		@param[in]	inDigitNum	Specifies the index (0-7) of the time digit of interest, in "transmission" order:
								0 = 1st (frame units) digit, 1 = 2nd digit, ... 7 = last (hour tens) digit.
		@param[out]	outHexValue	Receives the hex value (least significant 4 bits) to be set.
		@param[in]	inMask		Optionally specifies which bits to receive: (1 = set bit to new value, 0 = retain current bit value).
								Defaults to 0x0F.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetTimeHexValue (uint8_t inDigitNum, uint8_t & outHexValue, uint8_t inMask = 0x0f) const;

	/**
		@brief		Sets my timecode "time" using discrete BCD digits.
		@param[in]	inHourTens	Specifies the hours "tens" digit value.
		@param[in]	inHourOnes	Specifies the hours "ones" digit value.
		@param[in]	inMinTens	Specifies the minutes "tens" digit value.
		@param[in]	inMinOnes	Specifies the minutes "ones" digit value.
		@param[in]	inSecsTens	Specifies the seconds "tens" digit value.
		@param[in]	inSecsOnes	Specifies the seconds "ones" digit value.
		@param[in]	inFrameTens	Specifies the frame "tens" digit value.
		@param[in]	inFrameOnes	Specifies the frame "ones" digit value.
		@note		Each digit is masked to the number of bits allotted in the payload. Note that the digit order is reversed!
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus	SetTimeDigits (	const uint8_t inHourTens,	const uint8_t inHourOnes,
										const uint8_t inMinTens,	const uint8_t inMinOnes,
										const uint8_t inSecsTens,	const uint8_t inSecsOnes,
										const uint8_t inFrameTens,	const uint8_t inFrameOnes);

	/**
		@brief		Answers with my current timecode "time" as discrete BCD digits.
		@param[out]	outHourTens	Specifies the hours "tens" digit value.
		@param[out]	outHourOnes	Specifies the hours "ones" digit value.
		@param[out]	outMinTens	Specifies the minutes "tens" digit value.
		@param[out]	outMinOnes	Specifies the minutes "ones" digit value.
		@param[out]	outSecsTens	Specifies the seconds "tens" digit value.
		@param[out]	outSecsOnes	Specifies the seconds "ones" digit value.
		@param[out]	outFrameTens	Specifies the frame "tens" digit value.
		@param[out]	outFrameOnes	Specifies the frame "ones" digit value.
		@note		Each digit is masked to the number of bits allotted in the payload. Note that the digit order is reversed!
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus	GetTimeDigits (	uint8_t & outHourTens,	uint8_t & outHourOnes,
										uint8_t & outMinTens,	uint8_t & outMinOnes,
										uint8_t & outSecsTens,	uint8_t & outSecsOnes,
										uint8_t & outFrameTens,	uint8_t & outFrameOnes) const;

	/**
		@brief		Sets my timecode "time" with hours, minutes, seconds, frames (in decimal, not BCD digits).
 		@param[in]	inFormat	Specifies a valid AJAAncillaryData_Timecode_Format.
		@param[in]	inHours		Specifies the hours value, which must be less than 24.
		@param[in]	inMinutes	Specifies the minutes value, which must be less than 60.
		@param[in]	inSeconds	Specifies the seconds value, which must be less than 60.
		@param[in]	inFrames	Specifies the frame value, which must be less than the frame rate.
		@note		This method takes into account the "FieldID" bit for HFR timecode formats (e.g. p50, p59.94, p60, etc.).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus	SetTime (const AJAAncillaryData_Timecode_Format inFormat, const uint32_t inHours,
								const uint32_t inMinutes, const uint32_t inSeconds, const uint32_t inFrames);

	/**
		@brief		Answers with my current timecode "time" as individual hour, minute, second, and frame components (in decimal, not BCD digits).
 		@param[in]	inFormat	Specifies a valid AJAAncillaryData_Timecode_Format.
		@param[out]	outHours	Receives the hours value.
		@param[out]	outMinutes	Receives the minutes value.
		@param[out]	outSeconds	Receives the seconds value.
		@param[out]	outFrames	Receives the frame value.
		@note		This method takes into account the "FieldID" bit for HFR timecode formats (e.g. p50, p59.94, p60, etc.).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus	GetTime (const AJAAncillaryData_Timecode_Format inFormat, uint32_t & outHours,
								uint32_t & outMinutes, uint32_t & outSeconds, uint32_t & outFrames) const;

	/**
		@brief		Sets my timecode "time" from an AJATimeCode.
		@param[in]	inTimecode		Specifies the timecode.
 		@param[in]	inTimeBase		Specifies the time base (frame rate) associated with the specified timecode.
 		@param[in]	inIsDropFrame	Specify 'true' for dropframe; otherwise false.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetTimecode (const AJATimeCode & inTimecode, const AJATimeBase & inTimeBase, const bool inIsDropFrame);

	/**
		@brief		Answers with my timecode "time" as an AJATimeCode.
		@param[out]	outTimecode		Receives the timecode.
 		@param[out]	outTimeBase		Receives the time base (frame rate).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus GetTimecode (AJATimeCode & outTimecode, AJATimeBase & outTimeBase) const;


	/**
		@brief		Sets my raw "Binary Group" hex values.
		@param[in]	digitNum	the index (0 - 7) of the Binary Group value in "transmission" order, i.e. 0 = 1st (BG1) digit, 1 = 2nd digit, ..., 7 = last (BG7) digit
		@param[in]	hexValue	the hex value (ls 4 bits) to be set/get
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetBinaryGroupHexValue (uint8_t digitNum, uint8_t  hexValue, uint8_t mask = 0x0f);
	virtual AJAStatus GetBinaryGroupHexValue (uint8_t digitNum, uint8_t& hexValue, uint8_t mask = 0x0f) const;

	/**
		@brief		Sets my binary group values.
		@param[in]	bg8		binary group values (only the ls 4 bits of each value are retained) - note that the BG order is reversed!)
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetBinaryGroups (uint8_t  bg8, uint8_t  bg7, uint8_t  bg6, uint8_t  bg5, uint8_t  bg4, uint8_t  bg3, uint8_t  bg2, uint8_t  bg1);
	virtual AJAStatus GetBinaryGroups (uint8_t& bg8, uint8_t& bg7, uint8_t& bg6, uint8_t& bg5, uint8_t& bg4, uint8_t& bg3, uint8_t& bg2, uint8_t& bg1) const;


	/**
		@brief		Sets my FieldID flag.
		@param[in]	bFlag		false = field 1 (0), true = field 2 (1)
 		@param[in]	tcFmt	  	AJAAncillaryData_Timecode_Format associated with timecode (default is handled as 30-frame timecode).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetFieldIdFlag (bool  bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown);
	virtual AJAStatus GetFieldIdFlag (bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown) const;


	/**
		@brief		Sets my drop frame flag.
		@param[in]	bFlag		false = non-dropframe format, true = drop frame
 		@param[in]	tcFmt	  	AJAAncillaryData_Timecode_Format associated with timecode (default is handled as 30-frame timecode).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetDropFrameFlag (bool  bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown);
	virtual AJAStatus GetDropFrameFlag (bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown) const;


	/**
		@brief		Sets my color frame flag.
		@param[in]	bFlag		false = no relation between color frames and timecode values, true = timecode values are aligned with color frames (see SMPTE-12M).
 		@param[in]	tcFmt	  	AJAAncillaryData_Timecode_Format associated with timecode (default is handled as 30-frame timecode).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetColorFrameFlag (bool  bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown);
	virtual AJAStatus GetColorFrameFlag (bool& bFlag, AJAAncillaryData_Timecode_Format tcFmt = AJAAncillaryData_Timecode_Format_Unknown) const;


	/**
		@brief		Sets my binary group flag (3 bits).
		@param[in]	inBGFlag	Specifies the BG Flag value (only the least significant 3 bits are retained).
 		@param[in]	inFormat	Specifies the timecode format associated with timecode (default is handled as 30-frame timecode).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus SetBinaryGroupFlag (const uint8_t inBGFlag, const AJAAncillaryData_Timecode_Format inFormat = AJAAncillaryData_Timecode_Format_Unknown);

	/**
		@brief		Answers with my current binary group flag (3 bits).
		@param[out]	outBGFlag	Specifies the BG Flag value (only the least significant 3 bits are retained).
 		@param[in]	inFormat	Specifies the timecode format associated with timecode (default is handled as 30-frame timecode).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus GetBinaryGroupFlag (uint8_t & outBGFlag, const AJAAncillaryData_Timecode_Format inFormat = AJAAncillaryData_Timecode_Format_Unknown) const;

	/**
		@param[in]	pInAncData	A valid pointer to an AJAAncillaryData instance.
		@return		The AJAAncillaryDataType if I recognize this ancillary data (or unknown if unrecognized).
	**/
	static AJAAncillaryDataType			RecognizeThisAncillaryData (const AJAAncillaryData * pInAncData);

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &				Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@return		A string containing my human-readable timecode.
	**/
	virtual std::string					TimecodeString (void) const;

	/**
		@brief		Get the timecode format that matches the input timebase.
		@param[in]	inTimeBase		Specifies the time base (frame rate) associated with the specified timecode.
		@return		The AJAAncillaryData_Timecode_Format that corresponds to the input timebase, returns AJAAncillaryData_Timecode_Format_Unknown when no match found.
	**/
	static AJAAncillaryData_Timecode_Format	GetTimecodeFormatFromTimeBase (const AJATimeBase & inTimeBase);

protected:
	void Init (void);	// NOT virtual - called by constructors

	// Which timecode digits go in which elements of timeDigits[]
	// Note that this is the same order as received/transmitted, i.e. ls digits FIRST!
	enum
	{
		kTcFrameUnits	= 0,
		kTcFrameTens	= 1,
		kTcSecondUnits	= 2,
		kTcSecondTens	= 3,
		kTcMinuteUnits	= 4,
		kTcMinuteTens	= 5,
		kTcHourUnits	= 6,
		kTcHourTens		= 7,
		kNumTimeDigits	= 8
	};

	//	Which binary groups go in which elements of binaryGroup[]?
	//	Note that this is the same order as received/transmitted, i.e. ls digits FIRST!
	enum
	{
		kBg1	= 0,
		kBg2	= 1,
		kBg3	= 2,
		kBg4	= 3,
		kBg5	= 4,
		kBg6	= 5,
		kBg7	= 6,
		kBg8	= 7,
		kNumBinaryGroups = 8
	};

	// Note: if you make a change to the local member data, be sure to ALSO make the appropriate
	//		 changes in the Init() and operator= methods!

	// The timecode data is stored as "raw" hex nibbles of data, split up between "time" values and "binary group" values.
	// This means that sundry "flags" which take advantage of unused time bits (e.g. bits 2 and 3 of the hour tens digit)
	// are kept with the hex digit data, and only parsed in or out by the appropriate Set()/Get() methods.
	uint8_t		m_timeDigits[kNumTimeDigits];		// the 8 hex values of the time data, in order of received/transmitted (i.e. ls digits first)
	uint8_t		m_binaryGroup[kNumBinaryGroups];	// the 8 hex values of the "Binary Groups", in order from BG1 - BG8

};	//	AJAAncillaryData_Timecode


/**
	@brief		Writes a human-readable rendition of the given AJAAncillaryData_Timecode into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inAncData		Specifies the AJAAncillaryData_Timecode to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
inline std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryData_Timecode & inAncData)		{return inAncData.Print (inOutStream);}

#endif	// AJA_ANCILLARYDATA_TIMECODE_H
