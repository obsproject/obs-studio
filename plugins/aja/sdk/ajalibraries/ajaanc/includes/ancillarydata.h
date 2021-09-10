/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarydata.h
	@brief		Declares the AJAAncillaryData class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYDATA_H
#define AJA_ANCILLARYDATA_H

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ajaexport.h"
#include "ntv2publicinterface.h"
#include "ajabase/common/common.h"
#include <sstream>
#include <vector>


// Default Packet IDs used when building "analog" packets
// NOTE: there is NO guarantee that the Anc Extractor hardware will use these codes - nor does the
//       Anc Inserter hardware care. If you want to know whether a given packet is "analog" or
//		 "digital", you should look at the appropriate flag in the packet header. I just wanted to
//		 have all the locally (software) built "analog" IDs come from a single location.
const uint8_t AJAAncillaryData_AnalogDID = 0x00;
const uint8_t AJAAncillaryData_AnalogSID = 0x00;


typedef std::pair <uint8_t, uint8_t>		AJAAncillaryDIDSIDPair;	///< @brief	A DID/SID pair, typically used as an indexing key.
typedef uint16_t							AJAAncPktDIDSID;		///< @brief	Packet DID/SID pair: DID (MS 8 bits) and SID (LS 8 bits)
#define ToAJAAncPktDIDSID(_d_,_s_)			(uint16_t((_d_) << 8) | uint16_t(_s_))
#define FromAJAAncPktDIDSID(_k_,_d_,_s_)	(_d_) = uint8_t(((_k_) & 0xFF00) >> 8); (_d_) = uint8_t(_k_ & 0x00FF);

/**
	@brief	Writes a human-readable rendition of the given AJAAncillaryDIDSIDPair into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the AJAAncillaryDIDSIDPair to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryDIDSIDPair & inData);


/**
	@brief	Identifies the ancillary data types that are known to this module.
**/
enum AJAAncillaryDataType
{
	AJAAncillaryDataType_Unknown,				///< @brief	Includes data that is valid, but we don't recognize

	AJAAncillaryDataType_Smpte2016_3,			///< @brief	SMPTE 2016-3 VANC Aspect Format Description (AFD) metadata
	AJAAncillaryDataType_Timecode_ATC,			///< @brief	SMPTE 12-M Ancillary Timecode (formerly known as "RP-188")
	AJAAncillaryDataType_Timecode_VITC,			///< @brief	SMPTE 12-M Vertical Interval Timecode (aka "VITC")
	AJAAncillaryDataType_Cea708,				///< @brief	CEA708 (SMPTE 334) HD Closed Captioning
	AJAAncillaryDataType_Cea608_Vanc,			///< @brief	CEA608 SD Closed Captioning (SMPTE 334 VANC packet)
	AJAAncillaryDataType_Cea608_Line21,			///< @brief	CEA608 SD Closed Captioning ("Line 21" waveform)
	AJAAncillaryDataType_Smpte352,				///< @brief	SMPTE 352 "Payload ID"
	AJAAncillaryDataType_Smpte2051,				///< @brief	SMPTE 2051 "Two Frame Marker"
	AJAAncillaryDataType_FrameStatusInfo524D,	///< @brief	Frame Status Information, such as Active Frame flag
	AJAAncillaryDataType_FrameStatusInfo5251,	///< @brief	Frame Status Information, such as Active Frame flag
	AJAAncillaryDataType_HDR_SDR,
	AJAAncillaryDataType_HDR_HDR10,
	AJAAncillaryDataType_HDR_HLG,
	AJAAncillaryDataType_Size
};

#define	IS_VALID_AJAAncillaryDataType(_x_)		((_x_) >= AJAAncillaryDataType_Unknown  &&  (_x_) < AJAAncillaryDataType_Size)
#define	IS_KNOWN_AJAAncillaryDataType(_x_)		((_x_) > AJAAncillaryDataType_Unknown  &&  (_x_) < AJAAncillaryDataType_Size)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataType value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataType value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string & AJAAncillaryDataTypeToString (const AJAAncillaryDataType inValue, const bool inCompact = true);



/**
	@brief	Identifies which link of a video stream the ancillary data is associated with.
**/
enum AJAAncillaryDataLink
{
    AJAAncillaryDataLink_A,			///< @brief	The ancillary data is associated with Link A of the video stream.
	AJAAncillaryDataLink_B,			///< @brief	The ancillary data is associated with Link B of the video stream.
	AJAAncillaryDataLink_LeftEye	= AJAAncillaryDataLink_A,	///< @brief	The ancillary data is associated with the Left Eye stereoscopic video stream.
	AJAAncillaryDataLink_RightEye	= AJAAncillaryDataLink_B,	///< @brief	The ancillary data is associated with the Right Eye stereoscopic video stream.
	AJAAncillaryDataLink_Unknown,	///< @brief	It is not known which link of the video stream the ancillary data is associated with.
	AJAAncillaryDataLink_Size
};

#define	IS_VALID_AJAAncillaryDataLink(_x_)		((_x_) >= AJAAncillaryDataLink_A  &&  (_x_) < AJAAncillaryDataLink_Unknown)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLink value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataLink value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataLinkToString (const AJAAncillaryDataLink inValue, const bool inCompact = true);



/**
	@brief	Identifies which data stream the ancillary data is associated with.
**/
enum AJAAncillaryDataStream
{
	AJAAncillaryDataStream_1,		///< @brief	The ancillary data is associated with DS1 of the video stream (Link A).
	AJAAncillaryDataStream_2,		///< @brief	The ancillary data is associated with DS2 of the video stream (Link A).
	AJAAncillaryDataStream_3,		///< @brief	The ancillary data is associated with DS3 of the video stream (Link B).
	AJAAncillaryDataStream_4,		///< @brief	The ancillary data is associated with DS4 of the video stream (Link B).
	AJAAncillaryDataStream_Unknown,	///< @brief	It is not known which data stream the ancillary data is associated with.
	AJAAncillaryDataStream_Size
};

#define	IS_VALID_AJAAncillaryDataStream(_x_)	((_x_) >= AJAAncillaryDataStream_1  &&  (_x_) < AJAAncillaryDataStream_Unknown)
#define	IS_LINKA_AJAAncillaryDataStream(_x_)	((_x_) == AJAAncillaryDataStream_1)
#define	IS_LINKB_AJAAncillaryDataStream(_x_)	((_x_) == AJAAncillaryDataStream_2)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataStream value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataStream value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataStreamToString (const AJAAncillaryDataStream inValue, const bool inCompact = true);


/**
	@brief	Identifies which component of a video stream in which the ancillary data is placed or found.
**/
enum AJAAncillaryDataChannel
{
	AJAAncillaryDataChannel_C,			///< @brief	The ancillary data is associated with the chrominance (C) channel of the video stream.
	AJAAncillaryDataChannel_Both = AJAAncillaryDataChannel_C,	///< @brief	SD ONLY -- The ancillary data is associated with both the chroma and luma channels.
	AJAAncillaryDataChannel_Y,			///< @brief	The ancillary data is associated with the luminance (Y) channel of the video stream.
	AJAAncillaryDataChannel_Unknown,	///< @brief	It is not known which channel of the video stream the ancillary data is associated with.
	AJAAncillaryDataChannel_Size
};
typedef AJAAncillaryDataChannel	AJAAncillaryDataVideoStream;

#define	IS_VALID_AJAAncillaryDataChannel(_x_)		((_x_) >= AJAAncillaryDataChannel_C  &&  (_x_) < AJAAncillaryDataChannel_Unknown)
#define	IS_VALID_AJAAncillaryDataVideoStream(_x_)	(IS_VALID_AJAAncillaryDataChannel(_x_))
#define	AJAAncillaryDataVideoStream_C		AJAAncillaryDataChannel_C
#define	AJAAncillaryDataVideoStream_Y		AJAAncillaryDataChannel_Y
#define	AJAAncillaryDataVideoStream_Unknown	AJAAncillaryDataChannel_Unknown
#define	AJAAncillaryDataVideoStream_Size	AJAAncillaryDataChannel_Size

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataChannel value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataChannel value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataChannelToString (const AJAAncillaryDataChannel inValue, const bool inCompact = true);
AJAExport inline const std::string &	AJAAncillaryDataVideoStreamToString (const AJAAncillaryDataVideoStream inValue, const bool inCompact = true)	{return AJAAncillaryDataChannelToString(inValue,inCompact);}


/**
	@brief	Specifies which channel of a video stream in which to look for Anc data.
**/
enum AncChannelSearchSelect
{
	AncChannelSearch_Y,			///< @brief	Only look in luma samples
	AncChannelSearch_C,			///< @brief	Only look in chroma samples
	AncChannelSearch_Both,		///< @brief	Look both luma and chroma samples (SD only)
	AncChannelSearch_Invalid	///< @brief	Invalid
};

#define	IS_VALID_AncChannelSearchSelect(_x_)	((_x_) >= AncChannelSearch_Y  &&  (_x_) < AncChannelSearch_Invalid)

/**
	@return		A string containing a human-readable representation of the given AncChannelSearchSelect value (or empty if not possible).
	@param[in]	inSelect	Specifies the AncChannelSearchSelect value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise returns the longer symbolic format.
**/
AJAExport std::string AncChannelSearchSelectToString (const AncChannelSearchSelect inSelect, const bool inCompact = true);


/**
	@brief	Identified the raster section of a video stream that contains the ancillary data.
			Deprecated in favor of Horizontal Offset -- ::AJAAncDataHorizOffset_AnyVanc and
			::AJAAncDataHorizOffset_AnyHanc
**/
enum AJAAncillaryDataSpace
{
	AJAAncillaryDataSpace_VANC,		///< @brief	Ancillary data found between SAV and EAV (@see ::AJAAncDataHorizOffset_AnyVanc).
	AJAAncillaryDataSpace_HANC,		///< @brief	Ancillary data found between EAV and SAV (@see ::AJAAncDataHorizOffset_AnyHanc).
	AJAAncillaryDataSpace_Unknown,	///< @brief	It's unknown which raster section contains the ancillary data (@see ::AJAAncDataHorizOffset_Unknown).
	AJAAncillaryDataSpace_Size
};

#define	IS_VALID_AJAAncillaryDataSpace(_x_)		((_x_) >= AJAAncillaryDataSpace_VANC  &&  (_x_) < AJAAncillaryDataSpace_Unknown)
#define	IS_HANC_AJAAncillaryDataSpace(_x_)		((_x_) == AJAAncillaryDataSpace_HANC)
#define	IS_VANC_AJAAncillaryDataSpace(_x_)		((_x_) == AJAAncillaryDataSpace_VANC)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataSpace value (or empty if not possible).
	@param[in]	inValue		Specifies the AJAAncillaryDataSpace value to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataSpaceToString (const AJAAncillaryDataSpace inValue, const bool inCompact = true);


#define	AJAAncDataLineNumber_Unknown	uint16_t(0x0000)	///< @brief	Packet line number is unknown.
#define	AJAAncDataLineNumber_DontCare	uint16_t(0x07FF)	///< @brief	Packet placed/found on any legal line number (in field or frame).
#define	AJAAncDataLineNumber_Anywhere	(AJAAncDataLineNumber_DontCare)
#define	AJAAncDataLineNumber_AnyVanc	uint16_t(0x07FE)	///< @brief	Packet placed/found on any line past RP168 switch line and before SAV.
#define	AJAAncDataLineNumber_Future		uint16_t(0x07FD)	///< @brief	Line number exceeds 11 bits (future).

#define	IS_UNKNOWN_AJAAncDataLineNumber(_x_)		((_x_) == AJAAncDataLineNumber_Unknown)
#define	IS_IRRELEVANT_AJAAncDataLineNumber(_x_)		((_x_) == AJAAncDataLineNumber_DontCare)
#define	IS_GOOD_AJAAncDataLineNumber(_x_)			((_x_) > 0  &&  (_x_) < AJAAncDataLineNumber_DontCare)

AJAExport std::string AJAAncLineNumberToString (const uint16_t inValue);

//	Special horizOffset values:
#define	AJAAncDataHorizOffset_Unknown	uint16_t(0x0000)	///< @brief	Unknown.
#define	AJAAncDataHorizOffset_Anywhere	uint16_t(0x0FFF)	///< @brief	Unspecified -- Packet placed/found in any legal area of raster line.
#define	AJAAncDataHorizOffset_AnyHanc	uint16_t(0x0FFE)	///< @brief	HANC -- Packet placed/found in any legal area of raster line after EAV.
#define	AJAAncDataHorizOffset_AnyVanc	uint16_t(0x0FFD)	///< @brief	VANC -- Packet placed/found in any legal area of raster line after SAV, but before EAV.
#define	AJAAncDataHorizOffset_Future	uint16_t(0x0FFC)	///< @brief	Offset exceeds 12 bits (future).

/**
	@return		A string containing a human-readable representation of the given horizontal offset location value.
	@param[in]	inValue		Specifies the horizontal offset location value to be converted.
**/
AJAExport std::string AJAAncHorizOffsetToString (const uint16_t inValue);


/**
	@brief	Defines where the ancillary data can be found within a video stream.
**/
typedef struct AJAAncillaryDataLocation
{
	//	Instance Methods
	public:
		inline	AJAAncillaryDataLocation (	const AJAAncillaryDataLink		inLink			= AJAAncillaryDataLink_Unknown,
											const AJAAncillaryDataChannel	inChannel		= AJAAncillaryDataChannel_Unknown,
											const AJAAncillaryDataSpace		inIgnored		= AJAAncillaryDataSpace_Unknown,
											const uint16_t					inLineNum		= AJAAncDataLineNumber_Unknown,
											const uint16_t					inHorizOffset	= AJAAncDataHorizOffset_Unknown,
											const AJAAncillaryDataStream	inStream		= AJAAncillaryDataStream_1)
		{	AJA_UNUSED(inIgnored);
			SetDataLink(inLink).SetDataChannel(inChannel).SetLineNumber(inLineNum).SetHorizontalOffset(inHorizOffset).SetDataStream(inStream);
		}

		inline bool		operator == (const AJAAncillaryDataLocation & inRHS) const
		{
			//	Everything must match exactly:
			return GetDataLink() == inRHS.GetDataLink()
					&&  GetDataStream() == inRHS.GetDataStream()
						&&  GetDataChannel() == inRHS.GetDataChannel()
//	No longer necessary		&&  GetDataSpace() == inRHS.GetDataSpace()
								&&  GetLineNumber() == inRHS.GetLineNumber()
									&&  GetHorizontalOffset() == inRHS.GetHorizontalOffset();
		}

		inline bool		operator < (const AJAAncillaryDataLocation & inRHS) const
		{
			const uint64_t	lhs	(OrdinalValue());
			const uint64_t	rhs	(inRHS.OrdinalValue());
			return lhs < rhs;	//	64-bit unsigned compare:
		}

		inline bool		IsValid (void) const
		{
			return IS_VALID_AJAAncillaryDataLink(link)
					&& IS_VALID_AJAAncillaryDataStream(stream)
						&& IS_VALID_AJAAncillaryDataChannel(channel)
							&& (IS_VALID_AJAAncillaryDataSpace(GetDataSpace())
									|| (GetDataSpace() == AJAAncillaryDataSpace_Unknown));
		}

#if !defined(NTV2_DEPRECATE_15_2)
		/**
			@deprecated		Use the individual SetXXX functions (below) instead.
		**/
		inline AJAAncillaryDataLocation &	Set (	const AJAAncillaryDataLink		inLink,
													const AJAAncillaryDataChannel	inChannel,
													const AJAAncillaryDataSpace		inIgnored,
													const uint16_t					inLineNum,
													const uint16_t					inHorizOffset = AJAAncDataHorizOffset_Unknown,
													const AJAAncillaryDataStream	inStream = AJAAncillaryDataStream_1)
		{	AJA_UNUSED(inIgnored);
			link		= inLink;
			stream		= inStream;
			channel		= inChannel;
			lineNum		= inLineNum;
			horizOffset	= inHorizOffset;
			return *this;
		}
#endif	//	!defined(NTV2_DEPRECATE_15_2)

		/**
			@brief		Resets all of my location elements to an unknown, invalid state.
			@return		A reference to myself.
		**/
		inline AJAAncillaryDataLocation &	Reset (void)
		{
			link		= AJAAncillaryDataLink_Unknown;
			stream		= AJAAncillaryDataStream_Unknown;
			channel		= AJAAncillaryDataChannel_Unknown;
			lineNum		= AJAAncDataLineNumber_Unknown;
			horizOffset	= AJAAncDataHorizOffset_Unknown;
			return *this;
		}

		inline AJAAncillaryDataLink			GetDataLink (void) const				{return link;}		///< @return	My data link.
		inline bool							IsDataLinkA (void) const				{return link == AJAAncillaryDataLink_A;}
		inline bool							IsDataLinkB (void) const				{return link == AJAAncillaryDataLink_B;}

		inline AJAAncillaryDataStream		GetDataStream (void) const				{return stream;}		///< @return	My data stream.

		inline AJAAncillaryDataChannel		GetDataChannel (void) const				{return channel;}		///< @return	My data channel.
		inline bool							IsLumaChannel (void) const				{return channel == AJAAncillaryDataChannel_Y;}
		inline bool							IsChromaChannel (void) const			{return channel == AJAAncillaryDataChannel_C;}

		/**
			@return	"VANC" if my horizontal offset is "Any VANC", or "HANC" if my H offset is "Any HANC";
					otherwise "UNKNOWN".
			@note	To be truly accurate, this function would need to know the video standard/geometry
					to determine if my line number precedes SAV, and if my horizontal offset is after EAV.
		**/
		inline AJAAncillaryDataSpace		GetDataSpace (void) const
		{
			if (horizOffset == AJAAncDataHorizOffset_AnyVanc)
				return AJAAncillaryDataSpace_VANC;
			if (horizOffset == AJAAncDataHorizOffset_AnyHanc)
				return AJAAncillaryDataSpace_HANC;
			return AJAAncillaryDataSpace_Unknown;
		}
		inline bool							IsVanc (void) const						{return GetDataSpace() == AJAAncillaryDataSpace_VANC;}
		inline bool							IsHanc (void) const						{return GetDataSpace() == AJAAncillaryDataSpace_HANC;}

		inline uint16_t						GetLineNumber (void) const				{return lineNum;}		///< @return	My SMPTE line number.

		/**
			@return		The 12-bit horizontal offset of the packet.
						For HD, this is the number of luma samples (see SMPTE ST274).
						For SD, this is the number of Y/C muxed words (see SMPTE ST125).
						Can also be one of these predefined values:
						-	::AJAAncDataHorizOffset_Anywhere -- i.e., anywhere after SAV.
						-	::AJAAncDataHorizOffset_Unknown -- unspecified.
						-	::AJAAncDataHorizOffset_AnyHanc -- i.e., any legal area of the raster line after EAV.
						-	::AJAAncDataHorizOffset_AnyVanc -- i.e., any legal area of raster line after SAV, but before EAV.
		**/
		inline uint16_t						GetHorizontalOffset (void) const		{return horizOffset & 0x0FFF;}

		/**
			@brief		Writes a human-readable rendition of me into the given output stream.
			@param		ostrm		Specifies the output stream.
			@param[in]	inCompact	Specify 'true' for compact output;  otherwise 'false' for more detail.
			@return		The given output stream.
		**/
		std::ostream &						Print (std::ostream & ostrm, const bool inCompact = true) const;

		/**
			@brief	Sets my data link value to the given value (if valid).
			@param[in]	inLink		Specifies the new data link value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataLink (const AJAAncillaryDataLink inLink)						{link = inLink;	return *this;}

		/**
			@brief	Sets my data link value to the given value (if valid).
			@param[in]	inStream		Specifies the new data stream value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataStream (const AJAAncillaryDataStream inStream)				{stream = inStream;	return *this;}

		/**
			@brief	Sets my data video stream value to the given value (if valid).
			@param[in]	inChannel	Specifies the new data channel value to use. Must be valid.
			@return	A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataChannel (const AJAAncillaryDataChannel inChannel)			{channel = inChannel; return *this;}
		inline AJAAncillaryDataLocation &	SetDataVideoStream (const AJAAncillaryDataVideoStream inChannel)	{return SetDataChannel(inChannel);}

		/**
			@brief		Sets my data space value to the given value (if valid).
			@param[in]	inSpace		Specifies the new data space value to use. Must be valid.
			@return		A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetDataSpace (const AJAAncillaryDataSpace inSpace)
		{
			if (IS_VANC_AJAAncillaryDataSpace(inSpace))
				horizOffset = AJAAncDataHorizOffset_AnyVanc;
			else if (IS_HANC_AJAAncillaryDataSpace(inSpace))
				horizOffset = AJAAncDataHorizOffset_AnyHanc;
			return *this;
		}

		/**
			@brief		Sets my anc data line number value.
			@param[in]	inLineNum		Specifies the new line number value to use.
										Can also be AJAAncDataLineNumber_DontCare.
			@return		A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetLineNumber (const uint16_t inLineNum)							{lineNum = inLineNum; return *this;}


		/**
			@brief		Specifies the horizontal packet position in the raster.
			@param[in]	inHOffset	Specifies my new horizontal offset. Only the least-significant 12 bits are used.
									-	Use AJAAncillaryDataLocation::AJAAncDataHorizOffset_AnyVanc for any legal area of the raster line after SAV.
										This will reset my ::AJAAncillaryDataSpace to ::AJAAncillaryDataSpace_VANC.
									-	Use AJAAncillaryDataLocation::AJAAncDataHorizOffset_AnyHanc for any legal area of the raster line after EAV.
										This will reset my ::AJAAncillaryDataSpace to ::AJAAncillaryDataSpace_HANC.
									-	For HD, this is the number of luma samples (see SMPTE ST274).
									-	For SD, this is the number of Y/C muxed words (see SMPTE ST125).
									-	Use AJAAncDataHorizOffset_Unknown for zero -- i.e., immediately after SAV.
			@return		A non-const reference to myself.
		**/
		inline AJAAncillaryDataLocation &	SetHorizontalOffset (uint16_t inHOffset)
		{	inHOffset &= 0x0FFF;
			if (inHOffset == AJAAncDataHorizOffset_AnyVanc)
				horizOffset = inHOffset;	//	Force [any] VANC
			else if (inHOffset == AJAAncDataHorizOffset_AnyHanc)
				horizOffset = inHOffset;	//	Force [any] HANC
			else if (inHOffset == AJAAncDataHorizOffset_Anywhere)
				horizOffset = inHOffset;	//	Anywhere (unknown DataSpace)
			else
				horizOffset = inHOffset;	//	Trust the caller;  don't mess with existing DataSpace
			return *this;
		}

#if defined(_DEBUG)
	private:	//	Ordinarily, this is a private API
#endif	//	defined(_DEBUG)
		/**
			@return		A 64-bit unsigned ordinal value used for sorting/comparing.
						In highest to lowest order of magnitude:
						-	line number, ascending
						-	data space, ascending (VANC precedes HANC)
						-	horizontal offset, ascending
						-	data channel, ascending (Chroma highest, then Luma, then Unknown lowest)
						-	data stream, ascending (DS1 highest, then DS2, ... then Unknown lowest)
						-	data link, ascending (Link "A" highest, then Link "B", then Unknown lowest)
		**/
		inline uint64_t						OrdinalValue (void) const
		{	//	64-bit unsigned compare:					LLLLLLLLLLLLSSSHHHHHHHHHHHHCCCDDDDDDDKK
			const uint64_t	hOffset	(horizOffset == AJAAncDataHorizOffset_AnyVanc || horizOffset == AJAAncDataHorizOffset_Anywhere ? 0 : horizOffset);
			return ((uint64_t(lineNum) << 27)			//	LLLLLLLLLLLL
					| (uint64_t(GetDataSpace()) << 24)	//	            SSS
					| (hOffset << 12)					//	               HHHHHHHHHHHH
					| (uint64_t(channel) << 9)			//	                           CCC
					| (uint64_t(stream) << 2)			//	                              DDDDDDD
					| uint64_t(link));					//	                                     KK
		}

#if defined(NTV2_DEPRECATE_15_2)
	private:	//	First proposed to make my member data private in SDK 15.2
#endif
		AJAAncillaryDataLink		link;			///< @brief	Which data link (A or B)?
		AJAAncillaryDataStream		stream;			///< @brief	Which data stream (DS1, DS2... etc.)?
		AJAAncillaryDataChannel		channel;		///< @brief	Which channel (Y or C)?
		uint16_t					lineNum;		///< @brief	Which SMPTE line number?
		uint16_t					horizOffset;	///< @brief	12-bit horizontal offset in raster line

} AJAAncillaryDataLocation, AJAAncDataLoc, *AJAAncillaryDataLocationPtr;


/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLocation value (or empty if invalid).
	@param[in]	inValue		Specifies the AJAAncillaryDataLocation to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport std::string	AJAAncillaryDataLocationToString (const AJAAncillaryDataLocation & inValue, const bool inCompact = true);


/**
	@brief	Writes a human-readable rendition of the given AJAAncillaryDataLocation into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the AJAAncillaryDataLocation to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryDataLocation & inData);


/**
	@brief	Identifies the ancillary data coding type:  digital or non-digital (analog/raw).
**/
enum AJAAncillaryDataCoding
{
	AJAAncillaryDataCoding_Digital,		///< @brief	The ancillary data is in the form of a SMPTE-291 Ancillary Packet.
	AJAAncillaryDataCoding_Raw,			///< @brief	The ancillary data is in the form of a digitized waveform (e.g. CEA-608 captions, VITC, etc.).
	AJAAncillaryDataCoding_Analog	= AJAAncillaryDataCoding_Raw,	///< @deprecated	Use AJAAncillaryDataCoding_Raw instead.
	AJAAncillaryDataCoding_Unknown,		///< @brief	It is not known which coding type the ancillary data is using.
	AJAAncillaryDataCoding_Size
};

#define	IS_VALID_AJAAncillaryDataCoding(_x_)		((_x_) >= AJAAncillaryDataCoding_Digital  &&  (_x_) < AJAAncillaryDataCoding_Size)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryDataLocation value (or empty if invalid).
	@param[in]	inValue		Specifies the AJAAncillaryDataLocation to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryDataCodingToString (const AJAAncillaryDataCoding inValue, const bool inCompact = true);


/**
	@brief	Identifies the type of anc buffer the packet originated from:  GUMP, RTP, VANC, or unknown.
**/
enum AJAAncillaryBufferFormat
{
	AJAAncillaryBufferFormat_Unknown,		///< @brief	Unknown or "don't care".
	AJAAncillaryBufferFormat_FBVANC,		///< @brief	Frame buffer VANC line.
	AJAAncillaryBufferFormat_SDI,			///< @brief	SDI ("GUMP").
	AJAAncillaryBufferFormat_RTP,			///< @brief	RTP/IP.
	AJAAncillaryBufferFormat_Invalid,		///< @brief	Invalid.
	AJAAncillaryBufferFormat_Size = AJAAncillaryBufferFormat_Invalid
};

#define	IS_VALID_AJAAncillaryBufferFormat(_x_)		((_x_) >= AJAAncillaryBufferFormat_Unknown  &&  (_x_) < AJAAncillaryBufferFormat_Size)
#define	IS_KNOWN_AJAAncillaryBufferFormat(_x_)		((_x_) > AJAAncillaryBufferFormat_Unknown  &&  (_x_) < AJAAncillaryBufferFormat_Size)

/**
	@return		A string containing a human-readable representation of the given AJAAncillaryBufferFormat value (or empty if invalid).
	@param[in]	inValue		Specifies the AJAAncillaryBufferFormat to be converted.
	@param[in]	inCompact	If true (the default), returns the compact representation;  otherwise use the longer symbolic format.
**/
AJAExport const std::string &	AJAAncillaryBufferFormatToString (const AJAAncillaryBufferFormat inValue, const bool inCompact = true);


/**
	@brief		I am the principal class that stores a single SMPTE-291 SDI ancillary data packet OR the
				digitized contents of one "analog" raster line (e.g. line 21 captions or VITC). Since I'm
				payload-agnostic, I serve as the generic base class for more specific objects that know
				how to decode/parse specific types of ancillary data.

	@details	My AJAAncillaryData::m_payload member stores the User Data Words (UDWs) as an ordered collection of 8-bit data bytes.
				Because it's an STL vector, it knows how many UDW elements it contains. Thus, it stores the SMPTE "DC"
				(data count) value.

				I also have several member variables for metadata -- i.e., information about the packet -- including
				my data ID (DID), secondary data ID (SDID), checksum (CS), location in the video stream, etc.

				<b>Transmit -- Packet Creation</b>
				-	Use AJAAncillaryDataFactory::Create to instantiate a specific type of data packet.
				-	Or to "Manually" create a packet:
					@code{.cpp}
						const string myPacketData ("This is a test");
						AJAAncillaryData packet;  //  Defaults to AJAAncillaryDataCoding_Digital, AJAAncillaryDataLink_A, AJAAncillaryDataStream_1, AJAAncillaryDataVideoStream_Y, AJAAncillaryDataSpace_VANC
						packet.SetDID(0xAA);  packet.SetSID(0xBB);  //  Set the DID & SDID
						packet.SetLocationLineNumber(10);  //  Set the SMPTE line number you want the packet inserted at
						packet.SetPayloadData(myPacketData.c_str(), myPacketData.length());  //  Copy in the packet data
					@endcode
				-	For newer devices with Anc inserters, call AJAAncillaryData::GenerateTransmitData to fill the Anc buffer
					used in the AUTOCIRCULATE_TRANSFER::SetAncBuffers call (see \ref ancgumpformat).
				-	For multiple packets, put each AJAAncillaryData instance into an AJAAncillaryList.

				<b>Receive -- Packet Detection</b>
				-	Use AJAAncillaryDataFactory::GuessAncillaryDataType to detect the Anc packet type.

	@warning	<b>Not thread-safe!</b> When any of my non-const methods are called by one thread, do not call any of my
				methods from any other thread.
**/
class AJAExport AJAAncillaryData
{
public:
	/**
		@name	Construction, Destruction, Copying
	**/
	///@{

	AJAAncillaryData ();	///< @brief	My default constructor.

	/**
		@brief	My copy constructor (from reference).
		@param[in]	inClone	The AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData (const AJAAncillaryData & inClone);

	/**
		@brief	My copy constructor (from pointer).
		@param[in]	pInClone	A valid pointer to the AJAAncillaryData object to be cloned.
	**/
	AJAAncillaryData (const AJAAncillaryData * pInClone);

	virtual									~AJAAncillaryData ();	///< @brief		My destructor.
	virtual void							Clear (void);			///< @brief		Frees my allocated memory, if any, and resets my members to their default values.
	virtual AJAAncillaryData *				Clone (void) const;		///< @return	A clone of myself.
	///@}


	/**
		@name	Inquiry
	**/
	///@{

	virtual inline uint8_t					GetDID (void) const							{return m_DID;}							///< @return	My Data ID (DID).
	virtual inline uint8_t					GetSID (void) const							{return m_SID;}							///< @return	My secondary data ID (SID).
	virtual inline uint32_t					GetDC (void) const							{return uint32_t(m_payload.size());}	///< @return	My payload data count, in bytes.
	virtual inline AJAAncillaryDIDSIDPair	GetDIDSIDPair (void) const					{return AJAAncillaryDIDSIDPair(GetDID(),GetSID());}	///< @return	My DID & SID as an AJAAncillaryDIDSIDPair.
	virtual inline AJAAncPktDIDSID			GetDIDSID (void) const						{return ToAJAAncPktDIDSID(GetDID(),GetSID());}		///< @return	My DID & SID as an AJAAncPktDIDSID.
	virtual inline size_t					GetPayloadByteCount (void) const			{return size_t(GetDC());}				///< @return	My current payload byte count.
	virtual AJAAncillaryDataType			GetAncillaryDataType (void) const			{return m_ancType;}						///< @return	My anc data type (if known).
	virtual inline uint32_t					GetFrameID (void) const						{return m_frameID;}						///< @return	My frame identifier (if known).

	virtual inline const AJAAncillaryDataLocation &		GetDataLocation (void) const	{return m_location;}					///< @brief	My ancillary data "location" within the video stream.
	virtual inline AJAAncillaryDataCoding	GetDataCoding (void) const					{return m_coding;}						///< @return	The ancillary data coding type (e.g., digital or analog/raw waveform).
	virtual inline AJAAncillaryBufferFormat	GetBufferFormat (void) const				{return m_bufferFmt;}					///< @return	The ancillary buffer format (e.g., SDI/GUMP, RTP or Unknown).
	virtual uint8_t							GetChecksum (void) const					{return m_checksum;}					///< @return	My 8-bit checksum.

	virtual inline AJAAncillaryDataLink		GetLocationVideoLink (void) const			{return GetDataLocation().GetDataLink();}				///< @return	My current anc data video link value (A or B).
	virtual inline AJAAncillaryDataStream	GetLocationDataStream (void) const			{return GetDataLocation().GetDataStream();}				///< @return	My current anc data location's data stream (DS1,DS2...).
	virtual inline AJAAncillaryDataChannel	GetLocationDataChannel (void) const			{return GetDataLocation().GetDataChannel();}			///< @return	My current anc data location's video stream (Y or C).
	virtual inline AJAAncillaryDataSpace	GetLocationVideoSpace (void) const			{return GetDataLocation().GetDataSpace();}				///< @return	My current ancillary data space (HANC or VANC).
	virtual inline uint16_t					GetLocationLineNumber (void) const			{return GetDataLocation().GetLineNumber();}				///< @return	My current frame line number value (SMPTE line numbering).
	virtual inline uint16_t					GetLocationHorizOffset (void) const			{return GetDataLocation().GetHorizontalOffset();}		///< @return	My current horizontal offset value.
	virtual uint16_t						GetStreamInfo (void) const;																			///< @return	My 7-bit stream info (if relevant)
	virtual inline const uint64_t &			UserData (void) const						{return m_userData;}					///< @return	My user data (if any).

	/**
		@return	True if I have valid DataLink/DataStream stream information (rather than unknown).
	**/
	virtual inline bool						HasStreamInfo (void) const					{return IS_VALID_AJAAncillaryDataLink(GetLocationVideoLink()) && IS_VALID_AJAAncillaryDataStream(GetLocationDataStream());}
	virtual inline bool						IsEmpty (void) const						{return GetDC() == 0;}										///< @return	True if I have an empty payload.
	virtual inline bool						IsLumaChannel (void) const					{return GetDataLocation().IsLumaChannel ();}				///< @return	True if my location component stream is Y (luma).
	virtual inline bool						IsChromaChannel (void) const				{return GetDataLocation().IsChromaChannel ();}				///< @return	True if my location component stream is C (chroma).
	virtual inline bool						IsVanc (void) const							{return GetDataLocation().IsVanc ();}						///< @return	True if my location data space is VANC.
	virtual inline bool						IsHanc (void) const							{return GetDataLocation().IsHanc ();}						///< @return	True if my location data space is HANC.
	virtual inline bool						IsDigital (void) const						{return GetDataCoding() == AJAAncillaryDataCoding_Digital;}	///< @return	True if my coding type is digital.
	virtual inline bool						IsRaw (void) const							{return GetDataCoding() == AJAAncillaryDataCoding_Raw;}		///< @return	True if my coding type is "raw" (i.e., from an digitized waveform).

	/**
		@brief	Generates an 8-bit checksum from the DID + SID + DC + payload data.
		@note	This is NOT the same as the official SMPTE-291 checksum, which is 9 bits wide and should be calculated by the hardware embedder.
		@note	The calculated checksum is NOT stored in my m_checksum member variable. Call SetChecksum to store it.
		@return		The calculated 8-bit checksum.
	**/
	virtual uint8_t							Calculate8BitChecksum (void) const;

	/**
		@brief		Generates the official SMPTE 291 9-bit checksum from the DID + SID + DC + payload data.
		@note		This class is only applicable to 8-bit ancillary data applications.
		@return		The calculated 9-bit checksum.
	**/
	virtual uint16_t						Calculate9BitChecksum (void) const;

	/**
		@brief	Compares the received 8-bit checksum with a newly calculated 8-bit checksum. Returns 'true' if they match.
		@note	This is NOT the same as the official SMPTE-291 checksum, which is 9 bits wide and should be calculated by the hardware.
		@return	True if the calculated checksum matches received checksum; otherwise false.
	**/
	virtual bool							ChecksumOK (void) const						{return m_checksum == Calculate8BitChecksum ();}

	virtual AJAAncillaryData &				operator = (const AJAAncillaryData & inRHS);

	/**
		@param[in]	inRHS	The packet I am to be compared with.
		@return		True if I'm identical to the RHS packet.
	**/
	virtual bool							operator == (const AJAAncillaryData & inRHS) const;

	/**
		@param[in]	inRHS	The packet I am to be compared with.
		@return		True if I differ from the RHS packet.
	**/
	virtual inline bool						operator != (const AJAAncillaryData & inRHS) const		{return !(*this == inRHS);}

	/**
		@brief		Compares me with another packet.
		@param[in]	inRHS				The packet I am to be compared with.
		@param[in]	inIgnoreLocation	If true, don't compare each packet's AJAAncillaryDataLocation info. Defaults to true.
		@param[in]	inIgnoreChecksum	If true, don't compare each packet's checksums. Defaults to true.
		@return		AJA_STATUS_SUCCESS if equal;  otherwise AJA_STATUS_FAIL.
	**/
	virtual AJAStatus						Compare (const AJAAncillaryData & inRHS, const bool inIgnoreLocation = true, const bool inIgnoreChecksum = true) const;


#if !defined(NTV2_DEPRECATE_15_2)
		virtual inline AJAStatus					GetDataLocation (AJAAncillaryDataLocation & outLocInfo) const	///< @deprecated	Use the inline version instead.
																								{outLocInfo = GetDataLocation();	return AJA_STATUS_SUCCESS;}
		virtual AJAStatus							GetDataLocation (AJAAncillaryDataLink & outLink,
																	AJAAncillaryDataVideoStream & outChannel,
																	AJAAncillaryDataSpace & outAncSpace,
																	uint16_t & outLineNum);		///< @deprecated	Use the inline GetDataLocation() instead.
		virtual inline AJAAncillaryDataVideoStream	GetLocationVideoStream (void) const			{return m_location.GetDataChannel();}	///< @deprecated	Use GetLocationDataChannel instead.
		virtual inline bool							IsAnalog (void) const						{return GetDataCoding() == AJAAncillaryDataCoding_Raw;}		///< @deprecated	Use IsRaw instead.
#endif	//	!defined(NTV2_DEPRECATE_15_2)
	///@}


	/**
		@name	Modification
	**/
	///@{

	/**
		@brief		Sets my Data ID (DID).
		@param[in]	inDataID	Specifies my new Data ID (for digital ancillary data, usually the "official" SMPTE packet ID).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDID (const uint8_t inDataID);

	/**
		@brief		Sets my Secondary Data ID (SID) - (aka the Data Block Number (DBN) for "Type 1" SMPTE-291 packets).
		@param[in]	inSID		Specifies my new secondary Data ID.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetSID (const uint8_t inSID);

	/**
		@brief		Sets both my Data ID (DID) and Secondary Data ID (SID).
		@param[in]	inDIDSID	The AJAAncillaryDIDSIDPair that specifies both a DID and SID value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual inline AJAStatus				SetDIDSID (const AJAAncillaryDIDSIDPair & inDIDSID)		{SetDID(inDIDSID.first); return SetSID(inDIDSID.second);}	//	New in SDK 16.0

	/**
		@brief		Sets my 8-bit checksum. Note that it is not usually necessary to generate an 8-bit checksum, since the ANC Insertion
					hardware ignores this field and (for SMPTE-291 Anc packets) generates and inserts its own "proper" 9-bit SMPTE-291 checksum.
		@param[in]	inChecksum8		Specifies the new 8-bit checksum.
		@param[in]	inValidate		If 'true', fails the function if the given checksum doesn't match the result
									of the AJAAncillaryData::Calculate8BitChecksum function.
									If 'false', does not validate the given checksum. Defaults to 'false'.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetChecksum (const uint8_t inChecksum8, const bool inValidate = false);

	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLoc		Specifies the new AJAAncillaryDataLocation value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataLocation (const AJAAncillaryDataLocation & inLoc);

#if !defined(NTV2_DEPRECATE_15_2)
	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLink		Specifies the video link (A or B).
		@param[in]	inChannel	Specifies the video channel (Y or C or SD/both).
		@param[in]	inAncSpace	Specifies the ancillary data space (HANC or VANC).
		@param[in]	inLineNum	Specifies the frame line number (SMPTE line numbering).
		@param[in]	inStream	Optionally specifies the data stream (DS1 or DS2 for link A, or DS3 or DS4 for link B). Defaults to DS1.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataLocation (	const AJAAncillaryDataLink		inLink,
																const AJAAncillaryDataChannel	inChannel,
																const AJAAncillaryDataSpace		inAncSpace,
																const uint16_t					inLineNum,
																const AJAAncillaryDataStream	inStream  = AJAAncillaryDataStream_1);
#endif	//	!defined(NTV2_DEPRECATE_15_2)

	/**
		@brief		Sets my ancillary data "location" within the video stream.
		@param[in]	inLink	Specifies the new video link value (A or B).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationVideoLink (const AJAAncillaryDataLink inLink);

	/**
		@brief		Sets my ancillary data "location" data stream value (DS1,DS2...).
		@param[in]	inStream	Specifies my new data stream (DS1,DS2...) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationDataStream (const AJAAncillaryDataStream inStream);

	/**
		@brief		Sets my ancillary data "location" data channel value (Y or C).
		@param[in]	inChannel	Specifies my new data channel (Y or C) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationDataChannel (const AJAAncillaryDataChannel inChannel);

#if !defined(NTV2_DEPRECATE_15_2)
	virtual AJAStatus						SetLocationVideoSpace (const AJAAncillaryDataSpace inAncSpace);	///< @deprecated	Call SetLocationHorizOffset instead.
#endif	//	NTV2_DEPRECATE_15_2

	/**
		@brief	Sets my ancillary data "location" frame line number.
		@param[in]	inLineNum	Specifies the new frame line number value (SMPTE line numbering).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationLineNumber (const uint16_t inLineNum);

	/**
		@brief	Sets my ancillary data "location" horizontal offset.
		@param[in]	inOffset	Specifies the new horizontal offset value. \sa AJAAncillaryDataLocation::SetHorizontalOffset.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetLocationHorizOffset (const uint16_t inOffset);

	/**
		@brief		Sets my ancillary data coding type (e.g. digital or analog/raw waveform).
		@param[in]	inCodingType	AJAAncillaryDataCoding
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetDataCoding (const AJAAncillaryDataCoding inCodingType);

	/**
		@brief		Sets my originating frame identifier.
		@param[in]	inFrameID	Specifies my new frame identifier.
		@return		A non-constant reference to myself.
	**/
	virtual inline AJAAncillaryData &		SetFrameID (const uint32_t inFrameID)					{m_frameID = inFrameID;  return *this;}

	/**
		@brief		Sets my originating buffer format.
		@param[in]	inFmt	Specifies my new buffer format.
		@return		A non-constant reference to myself.
	**/
	virtual inline AJAAncillaryData &		SetBufferFormat (const AJAAncillaryBufferFormat inFmt)	{m_bufferFmt = inFmt;  return *this;}

	virtual inline uint64_t &				UserData (void)			{return m_userData;}	///< @return	Returns a non-constant reference to my user data. (New in SDK 16.0)

#if !defined(NTV2_DEPRECATE_14_2)
		/**
			@deprecated		Use SetLocationDataChannel function instead.
		**/
		virtual inline NTV2_DEPRECATED_f(AJAStatus	SetLocationVideoStream (const AJAAncillaryDataVideoStream inChannel))
													{return SetLocationDataChannel(inChannel);}
#endif	//	!defined(NTV2_DEPRECATE_14_2)
	///@}


	/**
		@name	Payload Data Access
	**/
	///@{

	/**
		@param[in]	inIndex0	Specifies the zero-based index value. This should be less than GetDC's result.
		@return		The payload data byte at the given zero-based index (or zero if the index value is invalid).
	**/
	virtual uint8_t							GetPayloadByteAtIndex (const uint32_t inIndex0) const;

	/**
		@return		A const pointer to my payload buffer.
	**/
	virtual inline const uint8_t *			GetPayloadData (void) const					{return m_payload.empty() ? NULL : &(m_payload[0]);}

	/**
		@brief		Copies my payload data into an external buffer.
		@param[in]	pBuffer			Specifies a valid, non-null starting address to where the payload data is to be copied.
		@param[in]	inByteCapacity	Specifies the maximum number of bytes that can be safely copied into the external buffer.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetPayloadData (uint8_t * pBuffer, const uint32_t inByteCapacity) const;

	/**
		@brief		Appends my payload data onto the given UDW vector as 10-bit User Data Words (UDWs), adding parity as needed.
		@param[out]	outUDWs			The 10-bit UDW vector to be appended to.
		@param[in]	inAddParity		If true, each UDW will have even parity added.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetPayloadData (UWordSequence & outUDWs, const bool inAddParity = true) const;
	///@}


	/**
		@name	Payload Data Modification
	**/
	///@{

	/**
		@param[in]	inDataByte	Specifies the data byte to be stored in my payload buffer.
		@param[in]	inIndex0	Specifies the zero-based index value. This should be less than GetDC's result.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetPayloadByteAtIndex (const uint8_t inDataByte, const uint32_t inIndex0);

	/**
		@brief		Copy data from external memory into my local payload memory.
		@param[in]	pInData		Specifies the address of the first byte of the external payload data to be copied (source).
		@param[in]	inByteCount	Specifies the number of bytes of payload data to be copied.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetPayloadData (const uint8_t * pInData, const uint32_t inByteCount);

	/**
		@brief		Appends data from an external buffer onto the end of my existing payload.
		@param[in]	pInBuffer		Specifies a valid, non-NULL starting address of the external buffer from which the payload data will be copied.
		@param[in]	inByteCount		Specifies the number of bytes to append.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AppendPayloadData (const uint8_t * pInBuffer, const uint32_t inByteCount);

	/**
		@brief		Appends payload data from another AJAAncillaryData object to my existing payload.
		@param[in]	inAncData	The AJAAncillaryData object whose payload data is to be appended to my own.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AppendPayload (const AJAAncillaryData & inAncData);

	/**
		@deprecated	Use AppendPayload(const AJAAncillaryData &) instead.
	**/
	virtual inline NTV2_DEPRECATED_f(AJAStatus	AppendPayload (const AJAAncillaryData * pInAncData))	{return pInAncData ? AppendPayload (*pInAncData) : AJA_STATUS_NULL;}

	/**
	 	@brief	Copies payload data from an external 16-bit source into local payload memory.
		@param[in]	pInData		A valid, non-NULL pointer to the external payload data to be copied (source).
								The upper 8 bits of each 16-bit word will be skipped and ignored.
		@param[in]	inNumWords	Specifies the number of 16-bit words of payload data to copy.
		@param[in]	inLocInfo	Specifies the anc data location information.
		@return					AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SetFromSMPTE334 (const uint16_t * pInData, const uint32_t inNumWords, const AJAAncillaryDataLocation & inLocInfo);

	/**
		@brief		Parses (interprets) the "local" ancillary data from my payload data.
		@note		This method is overridden by specific packet types (e.g. AJAAncillaryData_Cea608_Vanc).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						ParsePayloadData (void);

	/**
		@return		True if I think I have valid ancillary data;  otherwise false.
		@details	This result will only be trustworthy if I'm an AJAAncillaryData subclass (e.g. AJAAncillaryData_Cea708),
					and my ParsePayloadData method was previously called, to determine whether or not my packet
					data is legit or not. Typically, AJAAncillaryDataFactory::GuessAncillaryDataType is called
					to ascertain a packet's AJAAncillaryDataType, then AJAAncillaryDataFactory::Create is used
					to instantiate the specific AJAAncillaryData subclass instance. This is done automatically
					by AJAAncillaryList::AddReceivedAncillaryData.
	**/
	virtual inline bool						GotValidReceiveData (void) const			{return m_rcvDataValid;}

	/**
		@brief		Generates the payload data from the "local" ancillary data.
		@note		This abstract method is overridden for specific Anc data types.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual inline AJAStatus				GeneratePayloadData (void)					{return AJA_STATUS_SUCCESS;}
	///@}


	/**
		@name	Receive From AJA Hardware
	**/
	///@{

	/**
		@brief		Initializes me from "raw" ancillary data received from hardware (ingest) -- see \ref ancgumpformat.
		@param[in]	pInData				Specifies the starting address of the "raw" packet data that was received from the AJA device.
		@param[in]	inMaxBytes			Specifies the maximum number of bytes left in the source buffer.
		@param[in]	inLocationInfo		Specifies the default location info.
		@param[out]	outPacketByteCount	Receives the size (in bytes) of the parsed packet.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						InitWithReceivedData (	const uint8_t *						pInData,
																	const size_t						inMaxBytes,
																	const AJAAncillaryDataLocation &	inLocationInfo,
																	uint32_t &							outPacketByteCount);
	/**
		@brief		Initializes me from "raw" ancillary data received from hardware (ingest) -- see \ref ancgumpformat.
		@param[in]	inData				Specifies the "raw" packet data.
		@param[in]	inLocationInfo		Specifies the default location info.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						InitWithReceivedData (	const std::vector<uint8_t> &		inData,
																	const AJAAncillaryDataLocation &	inLocationInfo);

	/**
		@brief		Initializes me from the given 32-bit IP packet words received from hardware (ingest).
		@param[in]	inData				Specifies the "raw" packet data (in network byte order).
		@param		inOutStartIndex		On entry, specifies the zero-based starting index number of the first
										32-bit word associated with this Ancillary data packet.
										On exit, if successful, receives the zero-based starting index number
										of the first 32-bit word associated with the NEXT packet that may be
										in the vector.
		@param[in]	inIgnoreChecksum	If true, ignores checksum failures. Defaults to false (don't ignore).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						InitWithReceivedData (const ULWordSequence & inData, uint16_t & inOutStartIndex, const bool inIgnoreChecksum = false);
	///@}


	/**
		@name	Transmit To AJA Hardware
	**/
	///@{

	/**
		@brief		Returns the number of "raw" ancillary data bytes that will be generated by AJAAncillaryData::GenerateTransmitData
					(for playback mode).
		@param[out]	outPacketSize	Receives the size (in bytes) of the packet I will generate.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetRawPacketSize (uint32_t & outPacketSize) const;

	/**
		@brief		Generates "raw" ancillary data from my internal ancillary data (playback) -- see \ref ancgumpformat.
		@param		pBuffer				Pointer to "raw" packet data buffer to be filled.
		@param[in]	inMaxBytes			Maximum number of bytes left in the given data buffer.
		@param[out]	outPacketSize		Receives the size, in bytes, of the generated packet.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GenerateTransmitData (uint8_t * pBuffer, const size_t inMaxBytes, uint32_t & outPacketSize);

	/**
		@brief		Generates "raw" 10-bit ancillary packet component data from my internal ancillary data (playback).
		@param		outData				Specifies the vector to which data will be appended.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GenerateTransmitData (UWordSequence & outData);

	/**
		@brief		Generates the 32-bit IP packet words necessary for constructing an outgoing IP/RTP stream.
		@param		outData				Specifies the vector into which data will be appended.
										The data will be in network byte order (big-endian).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GenerateTransmitData (ULWordSequence & outData);
	///@}


	/**
		@name	Printing & Debugging
	**/
	///@{

	/**
		@brief		Streams a human-readable representation of me to the given output stream.
		@param		inOutStream		Specifies the output stream.
		@param[in]	inDetailed		Specify 'true' for a detailed representation;  otherwise use 'false' for a brief one.
		@return		The given output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = false) const;

	/**
		@brief		Dumps a human-readable representation of my payload bytes into the given output stream.
		@return		The given output stream.
	**/
	virtual std::ostream &					DumpPayload (std::ostream & inOutStream) const;

	/**
		@brief		Compares me with another packet and returns a string that describes what's different.
		@param[in]	inRHS				The packet I am to be compared with.
		@param[in]	inIgnoreLocation	If true, don't compare each packet's AJAAncillaryDataLocation info. Defaults to true.
		@param[in]	inIgnoreChecksum	If true, don't compare each packet's checksums. Defaults to true.
		@return		Empty string if equal;  otherwise a string that contains the differences.
	**/
	virtual std::string						CompareWithInfo (const AJAAncillaryData & inRHS, const bool inIgnoreLocation = true, const bool inIgnoreChecksum = true) const;

	virtual inline std::string				IDAsString (void) const	{return DIDSIDToString (GetDID(), GetSID());}	///< @return	A string representing my DID/SID.

	/**
		@return		Converts me into a compact, human-readable string.
		@param[in]	inDumpMaxBytes		Number of payload bytes to dump into the returned string. Defaults to zero (none).
	**/
	virtual std::string						AsString (const uint16_t inDumpMaxBytes = 0) const;
	

	/**
		@return	A string containing a human-readable representation of the given DID/SDID values,
				or empty for invalid or unknown values.
		@param[in]	inDID	Specifies the Data ID value.
		@param[in]	inSDID	Specifies the Secondary Data ID value.
	**/
	static std::string						DIDSIDToString (const uint8_t inDID, const uint8_t inSDID);
	///@}

	/**
		@return		The given data byte in bits 7:0, plus even parity in bit 8 and ~bit 8 in bit 9.
		@param[in]	inDataByte		The given data byte to have parity added to it.
	**/
	static uint16_t							AddEvenParity (const uint8_t inDataByte);


	typedef UWordSequence			U16Packet;	///< @brief	An ordered sequence of 10-bit packet words stored in uint16_t values.
	typedef std::vector<U16Packet>	U16Packets;	///< @brief	An ordered sequence of zero or more U16Packet values. 

	/**
		@brief		Extracts whatever VANC packets are found inside the given 16-bit YUV line buffer.
		@param[in]	inYUV16Line			Specifies the uint16_t sequence containing the 10-bit YUV VANC line data components.
										(Use ::UnpackLine_10BitYUVtoUWordSequence to convert a VANC line from an NTV2_FBF_10BIT_YCBCR
										frame buffer into this format. Use UnpackLine_8BitYUVtoU16s to convert a VANC line from an
										::NTV2_FBF_8BIT_YCBCR frame buffer into this format.)
		@param[in]	inChanSelect		Specifies the ancillary data channel to search. Use AncChannelSearch_Y for luma, AncChannelSearch_C
										for chroma, or AncChannelSearch_Both for both (SD only).
		@param[out]	outRawPackets		Receives the packet vector, which will contain one vector of uint16_t values per extracted packet.
										Each packet in the returned list will start with the 0x000/0x3FF/0x3FF/DID/SDID/DC
										sequence, followed by each 10-bit packet data word, and ending with the checksum word.
		@param[out]	outWordOffsets		Receives the horizontal word offsets into the line, one for each packet found.
										This should have the same number of elements as "outRawPackets".
										These offsets can also be used to discern which channel each packet originated in (Y or C).
		@return		True if successful;  false if failed.
		@note		This function will not finish parsing the line once a parity, checksum, or overrun error is discovered in the line.
	**/
	static bool								GetAncPacketsFromVANCLine (const UWordSequence &			inYUV16Line,
																		const AncChannelSearchSelect	inChanSelect,
																		U16Packets &					outRawPackets,
																		U16Packet &						outWordOffsets);
	/**
		@brief		Converts a single line of ::NTV2_FBF_8BIT_YCBCR data from the given source buffer into an ordered sequence of uint16_t
					values that contain the resulting 10-bit even-parity data.
		@param[in]	pInYUV8Line			A valid, non-NULL pointer to the start of the VANC line in an ::NTV2_FBF_8BIT_YCBCR video buffer.
		@param[out]	outU16YUVLine		Receives the converted 10-bit-per-component values as an ordered sequence of uint16_t values,
										which will include even parity and valid checksums.
		@param[in]	inNumPixels			Specifies the length of the line to be converted, in pixels.
		@return		True if successful;  otherwise false.
		@note		If SMPTE ancillary data is detected in the video, this routine "intelligently" stretches it by copying the 8-bits to
					the LS 8-bits of the 10-bit output, recalculating parity and checksums as needed. (This emulates what NTV2 device
					firmware does during playout of ::NTV2_FBF_8BIT_YCBCR frame buffers with ::NTV2_VANCDATA_8BITSHIFT_ENABLE.)
		@note		NTV2 firmware is expected to start the first anc packet at the first pixel position in the VANC line, and place
					subsequent packets, if any, in immediate succession, without any gaps. Therefore, a line that does not start with
					the 0x00/0xFF/0xFF packet header is assumed to not contain any packets. This saves a substantial amount of CPU time.
	**/
	static bool								Unpack8BitYCbCrToU16sVANCLine (const void * pInYUV8Line,
																			U16Packet & outU16YUVLine,
																			const uint32_t inNumPixels);

	protected:
		typedef std::vector<uint8_t>		ByteVector;
		typedef ByteVector::size_type		ByteVectorIndex;
		typedef ByteVector::const_iterator	ByteVectorConstIter;

		void								Init (void);	// NOT virtual - called by constructors

		AJAStatus							AllocDataMemory (const uint32_t inNumBytes);
		AJAStatus							FreeDataMemory (void);

		static inline uint8_t				GetGUMPHeaderByte1 (void)			{return 0xFF;}
		virtual uint8_t						GetGUMPHeaderByte2 (void) const;
		virtual inline uint8_t				GetGUMPHeaderByte3 (void) const		{return GetLocationLineNumber() & 0x7F;}	// ls 7 bits [6:0] of line num

	//	Instance Data
	protected:
		uint8_t						m_DID;			///< @brief	Official SMPTE ancillary packet ID (w/o parity)
		uint8_t						m_SID;			///< @brief	Official SMPTE secondary ID (or DBN - w/o parity)
		uint8_t						m_checksum;		///< @brief	My 8-bit checksum: DID + SID + DC + payload (w/o parity) [note: NOT the same as the 9-bit checksum in a SMPTE-291 packet!]
		AJAAncillaryDataLocation	m_location;		///< @brief	Location of the ancillary data in the video stream (Y or C, HANC or VANC, etc.)
		AJAAncillaryDataCoding		m_coding;		///< @brief	Analog or digital data
		ByteVector					m_payload;		///< @brief	My payload data (DC = size)
		bool						m_rcvDataValid;	///< @brief	This is set true (or not) by ParsePayloadData()
		AJAAncillaryDataType		m_ancType;		///< @brief	One of a known set of ancillary data types (or "Custom" if not identified)
		AJAAncillaryBufferFormat	m_bufferFmt;	///< @brief	My originating buffer format, if known
		uint32_t					m_frameID;		///< @brief	ID of my originating frame, if known
		uint64_t					m_userData;		///< @brief	User data (for client use)

};	//	AJAAncillaryData


/**
	@brief		Writes a human-readable rendition of the given AJAAncillaryData into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inAncData		Specifies the AJAAncillaryData to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
static inline std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryData & inAncData)		{return inAncData.Print(inOutStream);}


/**
	@brief		I represent the header of a SMPTE 2110 compliant RTP Anc network packet.
**/
class AJAExport AJARTPAncPayloadHeader
{
	public: // CLASS METHODS
	/**
		@name	Construction & Destruction
	**/
	///@{
		/**
			@return		True if the given buffer starts with an RTP packet header.
			@param[in]	inBuffer		Specifies the buffer to inspect.
		**/
		static bool				BufferStartsWithRTPHeader (const NTV2_POINTER & inBuffer);

		static inline size_t	GetHeaderWordCount (void)	{return 5;}											///< @return	The number of U32s in an RTP header.
		static inline size_t	GetHeaderByteCount (void)	{return GetHeaderWordCount() * sizeof(uint32_t);}	///< @return	The number of bytes in an RTP header.

		/**
			@return		A string containing a human-readable description of the given Field Bits value.
			@param[in]	inFBits		Specifies the Field Bits of interest.
		**/
		static const std::string &	FieldSignalToString (const uint8_t inFBits);
	///@}


	public: // INSTANCE METHODS
								AJARTPAncPayloadHeader ();		///< @brief	My default constructor
		virtual inline			~AJARTPAncPayloadHeader ()	{}	///< @brief	My destructor

	/**
		@name	Inquiry Methods
	**/
	///@{
		virtual bool			IsNULL (void) const;												///< @return	True if all of my fields are currently zero or false.
		virtual bool			IsValid (void) const;												///< @return	True if I'm considered "valid" in my current state.
		virtual inline bool		IsEndOfFieldOrFrame (void) const	{return mMarkerBit;}			///< @return	True if my Marker Bit value is non-zero;  otherwise false.
		virtual inline uint8_t	GetPayloadType (void) const			{return mPayloadType;}			///< @return	The Payload Type value.
		virtual inline uint32_t	GetSequenceNumber (void) const		{return mSequenceNumber;}		///< @return	The 32-bit Sequence Number value (in native host byte order).
		virtual inline uint32_t	GetTimeStamp (void) const			{return mTimeStamp;}			///< @return	The 32-bit RTP Time Stamp value (in native host byte order).
		virtual inline uint32_t	GetSyncSourceID (void) const		{return mSyncSourceID;}			///< @return	The Sync Source ID (SSID) value (in native host byte order).
		virtual inline uint16_t	GetPayloadLength (void) const		{return mPayloadLength;}		///< @return	The RTP packet payload length, in bytes (native host byte order).
		virtual inline uint8_t	GetAncPacketCount (void) const		{return mAncCount;}				///< @return	The number of SMPTE 291 Anc packets in this RTP packet payload.
		virtual inline uint8_t	GetFieldSignal (void) const			{return mFieldSignal & 3;}		///< @return	The 3-bit Field Bits value.
		virtual inline bool		IsProgressive (void) const			{return mFieldSignal == 0;}		///< @return	True if my Field Bits indicate Progressive video.
		virtual inline bool		NoFieldSpecified (void) const		{return IsProgressive();}		///< @return	True if my Field Bits indicate No Field Specified.
		virtual inline bool		IsField1 (void) const				{return mFieldSignal == 2;}		///< @return	True if my Field Bits indicate Field1.
		virtual inline bool		IsField2 (void) const				{return mFieldSignal == 3;}		///< @return	True if my Field Bits indicate Field2.
		virtual inline bool		IsValidFieldSignal (void) const		{return mFieldSignal != 1;}		///< @return	True if my Field Bits are valid. (New in SDK 16.0)
		virtual inline bool		HasPaddingBytes (void) const		{return mPBit;}					///< @return	True if my Padding Bit is set. (New in SDK 16.0)
		virtual inline bool		HasExtendedHeader (void) const		{return mXBit;}					///< @return	True if my Header Extension Bit is set. (New in SDK 16.0)

		/**
			@return		True if the RHS payload header state matches my own current state;  otherwise false.
			@param[in]	inRHS	The RHS operand ::AJARTPAncPayloadHeader to compare with me.
		**/
		virtual bool			operator == (const AJARTPAncPayloadHeader & inRHS) const;

		/**
			@return		True if the RHS payload header state doesn't match my own current state;  otherwise false.
			@param[in]	inRHS	The RHS operand ::AJARTPAncPayloadHeader to compare with me.
		**/
		virtual inline bool		operator != (const AJARTPAncPayloadHeader & inRHS) const	{return !(operator == (inRHS));}

		/**
			@brief		Writes a human-readable dump of my current state into a given output stream.
			@param		inOutStream		The output stream to write into.
			@return		A reference to the specified output stream.
		**/
		virtual std::ostream &	Print (std::ostream & inOutStream) const;
	///@}

	/**
		@name	I/O Methods
	**/
	///@{
		/**
			@brief		Writes an RTP packet header based on my current state into the given ::ULWordSequence.
						Each 32-bit word will be written in network byte order.
			@param[out]	outVector	Specifies the ::ULWordSequence to receive the RTP header words.
			@param[in]	inReset		Clears the ::ULWordSequence before appending my header, if true (the default).
									Specify false to append the 32-bit words to the ::ULWordSequence without first clearing it.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			WriteToULWordVector (ULWordSequence & outVector, const bool inReset = true) const;

		/**
			@brief		Writes an RTP packet header based on my current state into the given buffer.
						Each 32-bit word will be written in network byte order.
			@param[out]	outBuffer	Specifies the buffer to modify.
			@param[in]	inU32Offset	Specifies where to start writing in the buffer, as a count of 32-bit words.
									Defaults to zero (the start of the buffer).
			@return		True if successful;  otherwise false.
		**/
		virtual bool			WriteToBuffer (NTV2_POINTER & outBuffer, const ULWord inU32Offset = 0) const;

		/**
			@brief		Resets my current state from the RTP packet header stored in the given ::ULWordSequence.
						Each 32-bit word in the vector is expected to be in network byte order.
			@param[in]	inVector	A vector of 32-bit words. Each word must be in network byte order.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			ReadFromULWordVector (const ULWordSequence & inVector);

		/**
			@brief		Resets my current state from the RTP packet header stored in the given buffer.
						Each 32-bit word in the vector is expected to be in network byte order.
			@param[in]	inBuffer	A buffer containing a number of 32-bit words. Each word must be in network byte order.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			ReadFromBuffer (const NTV2_POINTER & inBuffer);
	///@}

	/**
		@name	Setters
	**/
	///@{
		/**
			@brief		Sets my Field Signal value to "Field 1".
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetField1 (void)									{return SetFieldSignal(2);}

		/**
			@brief		Sets my Field Signal value to "Field 2".
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetField2 (void)									{return SetFieldSignal(3);}

		/**
			@brief		Sets my Field Signal value to "Progressive".
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetProgressive (void)								{return SetFieldSignal(0);}

		/**
			@brief		Sets my Payload Type value.
			@param[in]	inPayloadType	Specifies the new Payload Type value.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetPayloadType (const uint8_t inPayloadType)		{mPayloadType = inPayloadType & 0x7F;  return *this;}

		/**
			@brief		Sets my RTP Packet Length value.
			@param[in]	inByteCount		Specifies the new RTP Payload Length value, which must be in native host byte order.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetPayloadLength (const uint16_t inByteCount)		{mPayloadLength		= inByteCount;		return *this;}

		/**
			@brief		Sets my RTP Anc Packet Count value.
			@param[in]	inPktCount	Specifies the new Anc Packet Count value.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetAncPacketCount (const uint8_t inPktCount)		{mAncCount			= inPktCount;		return *this;}

		/**
			@brief		Sets my RTP Packet Time Stamp value.
			@param[in]	inTimeStamp		Specifies my new Packet Time Stamp value, which must be in native host byte order.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetTimeStamp (const uint32_t inTimeStamp)			{mTimeStamp			= inTimeStamp;		return *this;}

		/**
			@brief		Sets my RTP Packet Sync Source ID value.
			@param[in]	inSyncSrcID		Specifies my new Sync Source ID value, which must be in native host byte order.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetSyncSourceID (const uint32_t inSyncSrcID)		{mSyncSourceID		= inSyncSrcID;		return *this;}

		/**
			@brief		Sets my RTP Packet Sequence Number value.
			@param[in]	inSeqNumber		Specifies my new Sequence Number value, which must be in native host byte order.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetSequenceNumber (const uint32_t inSeqNumber)		{mSequenceNumber	= inSeqNumber;		return *this;}

		/**
			@brief		Sets my RTP Packet CC Bits value.
			@param[in]	inCCBits		Specifies my new CC Bits value.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetCCBits (const uint8_t inCCBits)					{mCCBits			= inCCBits & 0x0F;	return *this;}

		/**
			@brief		Sets my RTP Packet End-Of-Field or End-Of-Frame (Marker Bit) value.
			@param[in]	inIsLast		Specify true to set the Marker Bit (the default);  otherwise specify false to clear it.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPayloadHeader &	SetEndOfFieldOrFrame (const bool inIsLast = true)	{mMarkerBit			= inIsLast;			return *this;}
	///@}

	#if !defined(NTV2_DEPRECATE_15_5)
		/**
			@deprecated	To get the full RTP packet length, add GetPayloadLength and GetHeaderByteCount.
		**/
		virtual inline uint16_t	GetPacketLength (void) const		{return GetPayloadLength()+uint16_t(GetHeaderByteCount());}
	#endif	//	!defined(NTV2_DEPRECATE_15_5)

	protected:
		/**
			@brief		Resets (part of) my state from a given 32-bit word in an existing RTP header.
			@param[in]	inIndex0	Specifies which 32-bit word of the RTP header as a zero-based index number (must be under 5).
			@param[in]	inULWord	Specifies the 32-bit word from the RTP header (in network-byte-order).
			@return		True if successful;  otherwise false.
		**/
		virtual bool							SetFromPacketHeaderULWordAtIndex (const unsigned inIndex0, const uint32_t inULWord);

		/**
			@brief		Sets my Field Signal value from the given 8-bit value.
			@param[in]	inFieldSignal	Specifies my new Field Signal value. Only the least significant 3 bits are used.
			@return		A non-constant reference to myself.
		**/
		virtual inline AJARTPAncPayloadHeader &	SetFieldSignal (const uint8_t inFieldSignal)	{mFieldSignal = (inFieldSignal & 0x03);  return *this;}

		/**
			@brief		Answers with the 32-bit RTP packet header value for the given position. The returned value will be in network byte order.
			@param[in]	inIndex0	Specifies which 32-bit word of the RTP packet header to calculate and return. It's a zero-based index number, and must be under 5.
			@param[out]	outULWord	Receives the requested 32-bit RTP packet header value. It will be in network byte order.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			GetPacketHeaderULWordForIndex (const unsigned inIndex0, uint32_t & outULWord) const;

		/**
			@return		The 32-bit RTP packet header value for a given position. It will be in network byte order.
			@param[in]	inIndex0	Specifies which 32-bit word of the RTP packet header to calculate and return. It's a zero-based index number, and must be under 5.
		**/
		virtual inline uint32_t	GetPacketHeaderULWordForIndex (const unsigned inIndex0) const		{uint32_t result(0); GetPacketHeaderULWordForIndex(inIndex0, result); return result;}

	private: // INSTANCE DATA
		uint8_t		mVBits;			///< @brief	Version -- currently should be 0x02
		bool		mPBit;			///< @brief	Padding -- hardware gets/sets this
		bool		mXBit;			///< @brief	Extended Header -- Hardware gets/sets this
		bool		mMarkerBit;		///< @brief	Marker Bit (last RTP pkt?) --	Playout: WriteRTPPackets sets this
		uint8_t		mCCBits;		///< @brief	CSRC Count -- Hardware gets/sets this
		uint8_t		mPayloadType;	///< @brief	Payload Type -- Hardware gets/sets this
		uint32_t	mSequenceNumber;///< @brief	Sequence Number (native host byte order) -- Hardware gets/sets this
		uint32_t	mTimeStamp;		///< @brief	Time Stamp (native host byte order) -- Hardware gets/sets this
		uint32_t	mSyncSourceID;	///< @brief	Sync Source ID (native host byte order) -- Playout: client sets this
		uint16_t	mPayloadLength;	///< @brief	RTP Payload Length, in bytes (native host byte order)
									///			Playout: WriteRTPPackets sets this
									///			Payload starts at 'C' bit of first SMPTE Anc packet data
		uint8_t		mAncCount;		///< @brief	Anc Packet Count -- Playout: WriteRTPPackets sets this
		uint8_t		mFieldSignal;	///< @brief	Field Signal -- Playout: WriteRTPPackets sets this
};	//	AJARTPAncPayloadHeader

/**
	@brief		Streams a human-readable representation of the given ::AJARTPAncPayloadHeader to the given output stream.
	@param[in]	inOutStrm		Specifies the output stream to receive the payload header's state information.
	@param[in]	inObj			Specifies the ::AJARTPAncPayloadHeader of interest.
	@return		A non-constant reference to the given output stream.
**/
static inline std::ostream & operator << (std::ostream & inOutStrm,  const AJARTPAncPayloadHeader & inObj)	{return inObj.Print(inOutStrm);}


/**
	@brief		I represent the 4-byte header of an anc packet that's inside an RTP packet.
**/
class AJAExport AJARTPAncPacketHeader
{
	public:  // INSTANCE METHODS
	/**
		@name	Construction & Destruction
	**/
	///@{
								AJARTPAncPacketHeader ();		///< @brief	My default constructor
		explicit				AJARTPAncPacketHeader (const AJAAncillaryDataLocation & inLocation);	///< @brief	Constructs me from an ::AJAAncillaryDataLocation
		virtual inline			~AJARTPAncPacketHeader ()		{}		///< @brief	My destructor
	///@}

	/**
		@name	Inquiry Methods
	**/
	///@{
		virtual uint32_t		GetULWord (void) const;										///< @return	The 4-byte header value (in network byte order) that represents my current state.
		virtual inline bool		IsCBitSet (void) const				{return mCBit;}			///< @return	True if my "C" bit ("C" channel bit) is set; otherwise false.
		virtual inline bool		IsSBitSet (void) const				{return mSBit;}			///< @return	True if my "S" bit (Data Stream valid bit) is set; otherwise false.
		virtual inline uint16_t	GetLineNumber (void) const			{return mLineNum;}		///< @return	My current line number value (in host native byte order).
		virtual inline uint16_t	GetHorizOffset (void) const			{return mHOffset;}		///< @return	My current horizontal offset value (in host native byte order).
		virtual inline uint8_t	GetStreamNumber (void) const		{return mStreamNum;}	///< @return	My current data stream number value.
		virtual AJAAncillaryDataLocation	AsDataLocation(void) const;						///< @return	An ::AJAAncillaryDataLocation that represents my current state.

		/**
			@brief		Streams a human-readable represetation of my current state to the given output stream.
			@param[in]	inOutStream		Specifies the output stream to receive my state information.
			@return		A non-constant reference to the given output stream.
		**/
		virtual std::ostream &		Print (std::ostream & inOutStream) const;
	///@}

	/**
		@name	Modifiers
	**/
	///@{
		/**
			@brief		Sets my "C" channel bit setting to 'true'.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetCChannel (void)							{mCBit = true;  return *this;}

		/**
			@brief		Sets my "C" channel bit setting to 'false'.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetYChannel (void)							{mCBit = false;  return *this;}

		/**
			@brief		Sets my line number value to least-significant 11 bits of the given value.
			@param[in]	inLineNum		Specifies my new line number value. Only the LS 11 bits are used.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetLineNumber (const uint16_t inLineNum)		{mLineNum = inLineNum & 0x7FF;  return *this;}

		/**
			@brief		Sets my horizontal offset value to least-significant 12 bits of the given value.
			@param[in]	inHOffset		Specifies my new horizontal offset value. Only the LS 12 bits are used.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetHorizOffset (const uint16_t inHOffset)	{mHOffset = inHOffset & 0x0FFF;  return *this;}

		/**
			@brief		Sets my stream number value to least-significant 7 bits of the given value.
			@param[in]	inStreamNum		Specifies my new data stream number value. Only the LS 7 bits are used.
			@note		It is recommended that SetDataStreamFlag be called with 'true' if the stream number is non-zero.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetStreamNumber (const uint8_t inStreamNum)	{mStreamNum = inStreamNum & 0x07F;  return *this;}

		/**
			@brief		Sets my data stream flag.
			@param[in]	inFlag			Specify true to signify my Data Stream Number is legitimate (and non-zero);
										otherwise 'false'.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	SetDataStreamFlag (const bool inFlag)		{mSBit = inFlag;  return *this;}

		/**
			@brief		Resets me from a given ::AJAAncillaryDataLocation.
			@param[in]	inLocation		Specifies the ::AJAAncillaryDataLocation to reset my current state from.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual AJARTPAncPacketHeader &	SetFrom (const AJAAncillaryDataLocation & inLocation);

		/**
			@brief		Assigns the given ::AJAAncillaryDataLocation to me, resetting my current state.
			@param[in]	inRHS			Specifies the ::AJAAncillaryDataLocation to reset my current state from.
			@return		A non-constant reference to myself (for daisy-chaining "Set..." calls).
		**/
		virtual inline AJARTPAncPacketHeader &	operator = (const AJAAncillaryDataLocation & inRHS)		{return SetFrom(inRHS);}
	///@}

	/**
		@name	I/O
	**/
	///@{
		/**
			@brief		Resets my current state by decoding the given 4-byte header value.
			@param[in]	inULWord	The 4-byte header value obtained from an RTP packet, in network-byte-order.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			SetFromULWord (const uint32_t inULWord);

		/**
			@brief		Resets my current state by decoding the 4-byte header value stored in the given ::ULWordSequence
						at the given zero-based index position.
			@param[in]	inVector	A ::ULWordSequence of 4-byte words, each in network-byte-order.
			@param[in]	inIndex0	Specifies the position (offset) of the header word in the ::ULWordSequence.
			@return		True if successful;  otherwise false.
		**/
		virtual bool			ReadFromULWordVector (const ULWordSequence & inVector, const unsigned inIndex0);

		/**
			@brief		Writes my 4-byte header value into the given ::ULWordSequence. The 4-byte value will be in network byte order.
			@param[out]	outVector	Specifies the ::ULWordSequence to receive my 4-byte header value.
			@param[in]	inReset		Optionally clears the ::ULWordSequence before appending, if true (the default).
									Specify false to append my header value to the ::ULWordSequence without first clearing it.
			@return		True if successful;  otherwise false.
		**/
		virtual bool	WriteToULWordVector (ULWordSequence & outVector,  const bool inReset = true) const;
	///@}

	private:  // INSTANCE DATA
		bool		mCBit;		///< @brief	My C-channel bit
		bool		mSBit;		///< @brief	My Data Stream Flag bit
		uint16_t	mLineNum;	///< @brief	My line number (in host native byte order)
		uint16_t	mHOffset;	///< @brief	My horizontal offset (in host native byte order)
		uint8_t		mStreamNum;	///< @brief	My stream number
};	//	AJARTPAncPacketHeader

/**
	@brief		Streams a human-readable representation of the given ::AJARTPAncPacketHeader to the given output stream.
	@param[in]	inOutStrm		Specifies the output stream to receive my state information.
	@param[in]	inObj			Specifies the ::AJARTPAncPacketHeader of interest.
	@return		A non-constant reference to the given output stream.
**/
static inline std::ostream & operator << (std::ostream & inOutStrm,  const AJARTPAncPacketHeader & inObj)	{return inObj.Print(inOutStrm);}

#endif	// AJA_ANCILLARYDATA_H
