/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarylist.h
	@brief		Declares the AJAAncillaryList class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#ifndef AJA_ANCILLARYLIST_H
#define AJA_ANCILLARYLIST_H

#include "ancillarydata.h"
#include "ntv2formatdescriptor.h"
#include <list>
#include <map>
#include <set>


// used for "match any" searches and counts
const uint8_t AJAAncillaryDataWildcard_DID = 0xFF;
const uint8_t AJAAncillaryDataWildcard_SID = 0xFF;

/**
	@brief	Associates certain frame line numbers with specific types of "raw" or "analog" ancillary data.
			For example, you may wish to associate line 21 (in 525i) with ::AJAAncillaryDataType_Cea608_Line21.
	@note	This facility is ONLY used by AJAAncillaryList::AddReceivedAncillaryData to identify captured
			"raw" data (AJAAncillaryDataCoding_Analog).
**/
typedef std::map <uint16_t, AJAAncillaryDataType>	AJAAncillaryAnalogTypeMap;

typedef std::vector<ULWordSequence>	AJAU32Pkts;						///< @brief	Ordered sequence of U32 RTP packets (U32s in network byte order)
typedef AJAU32Pkts::const_iterator	AJAU32PktsConstIter;			///< @brief	Handy const iterator over AJAU32Pkts
typedef AJAU32Pkts::iterator		AJAU32PktsIter;					///< @brief	Handy non-const iterator over AJAU32Pkts

typedef UByteSequence				AJAAncPktCounts;				///< @brief	Ordered sequence of SMPTE Anc packet counts
typedef AJAAncPktCounts::const_iterator	AJAAncPktCountsConstIter;	///< @brief	Handy const iterator over AJAAncPktCounts
class CNTV2Card;

typedef std::set<AJAAncPktDIDSID>			AJAAncPktDIDSIDSet;				///< @brief	Set of distinct packet DID/SIDs (New in SDK 16.0)
typedef AJAAncPktDIDSIDSet::const_iterator	AJAAncPktDIDSIDSetConstIter;	///< @brief	Handy const iterator for AJAAncPktDIDSIDSet (New in SDK 16.0)
typedef AJAAncPktDIDSIDSet::iterator		AJAAncPktDIDSIDSetIter;			///< @brief	Handy non-const iterator for AJAAncPktDIDSIDSet (New in SDK 16.0)


/**
	@brief		I am an ordered collection of AJAAncillaryData instances which represent one or more SMPTE 291
				data packets that were captured from, or destined to be played into, one video field or frame.
				I can be built from the ancillary data received by the hardware during one field/frame, and/or
				built "from scratch" and used as the source of outgoing ancillary data to hardware.

				By default, packets (::AJAAncillaryData instances) remain in the order added to me.
				Use my AJAAncillaryList::SortListByDID, AJAAncillaryList::SortListBySID or AJAAncillaryList::SortListByLocation
				methods to sort my packets by DID, SDID or location.

	@warning	I am not thread-safe! When any of my non-const instance methods are called by one thread,
				do not call any of my other instance methods from any other thread.
**/
class AJAExport AJAAncillaryList
{
public:	//	CLASS METHODS

	/**
		@name	Create from Device Buffers (Capture/Ingest)
	**/
	///@{
	/**
		@brief		Returns all packets found in the VANC lines of the given NTV2 frame buffer.
		@param[in]	inFrameBuffer		Specifies the NTV2 frame buffer (or at least the portion containing the VANC lines).
		@param[in]	inFormatDesc		Describes the frame buffer (pixel format, video standard, etc.).
		@param[out]	outPackets			Receives the packets found.
		@param[in]	inFrameNum			If non-zero, specifies/sets the frame identifier for the packets.
		@return		AJA_STATUS_SUCCESS if successful.
		@bug		The ::AJAAncillaryDataLink in the ::AJAAncillaryDataLocation in each of the returned packets
					is currently ::AJAAncillaryDataLink_A, which will be incorrect if, for example, the FrameStore
					that delivered the \c inFrameBuffer was sourced from the "B" link of a Dual-Link SDI source.
	**/
	static AJAStatus						SetFromVANCData (const NTV2_POINTER & inFrameBuffer,
															const NTV2FormatDescriptor & inFormatDesc,
															AJAAncillaryList & outPackets,
															const uint32_t inFrameNum = 0);

	/**
		@brief		Returns all ancillary data packets found in the given F1 and F2 ancillary data buffers.
		@param[in]	inF1AncBuffer		Specifies the F1 ancillary data buffer.
		@param[in]	inF2AncBuffer		Specifies the F2 ancillary data buffer.
		@param[out]	outPackets			Receives the packet list.
		@param[in]	inFrameNum			If non-zero, replaces the frame identifier of new packets that have a zero frame ID.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						SetFromDeviceAncBuffers (const NTV2_POINTER & inF1AncBuffer,
																	const NTV2_POINTER & inF2AncBuffer,
																	AJAAncillaryList & outPackets,
																	const uint32_t inFrameNum = 0);

	#if !defined(NTV2_DEPRECATE_15_2)
		static inline AJAStatus	SetFromSDIAncData (const NTV2_POINTER & inF1, const NTV2_POINTER & inF2, AJAAncillaryList & outPkts)	{return SetFromDeviceAncBuffers(inF1, inF2, outPkts);}	///< @deprecated	Use SetFromDeviceAncBuffers instead.
		static inline AJAStatus	SetFromIPAncData (const NTV2_POINTER & inF1, const NTV2_POINTER & inF2, AJAAncillaryList & outPkts)		{return SetFromDeviceAncBuffers(inF1, inF2, outPkts);}	///< @deprecated	Use SetFromDeviceAncBuffers instead.
	#endif	//	!defined(NTV2_DEPRECATE_15_2)
	///@}


	/**
		@name	Global Configuration
	**/
	///@{
	/**
		@brief		Clears my global Analog Ancillary Data Type map.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						ClearAnalogAncillaryDataTypeMap (void);
	

	/**
		@brief		Copies the given map to the global Analog Ancillary Data Type map.
		@param[in]	inMap	The map to copy.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						SetAnalogAncillaryDataTypeMap (const AJAAncillaryAnalogTypeMap & inMap);
	

	/**
		@brief		Returns a copy of the global Analog Ancillary Data Type map.
		@param[out]	outMap	Receives a copy of the map.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						GetAnalogAncillaryDataTypeMap (AJAAncillaryAnalogTypeMap & outMap);


	/**
		@brief		Sets (or changes) the map entry for the designated line to the designated type.
		@param[in]	inLineNum	Specifies the frame line number to be added or changed.
		@param[in]	inType		Specifies the ancillary data type to be associated with this line.
								Use AJAAncillaryDataType_Unknown to remove any associations with the line.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static	AJAStatus						SetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum, const AJAAncillaryDataType inType);


	/**
		@brief		Answers with the ancillary data type associated with the designated line.
		@param[in]	inLineNum		Specifies the frame line number of interest.
		@return		The ancillary data type associated with the designated line, if any;
					otherwise AJAAncillaryDataType_Unknown if the line has no association.
	**/
	static AJAAncillaryDataType				GetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum);


	/**
		@brief		Sets whether or not zero-length packets are included or not.
		@param[in]	inExclude	Specify true to exclude zero-length packets.
	**/
	static void								SetIncludeZeroLengthPackets (const bool inExclude);
	static uint32_t							GetExcludedZeroLengthPacketCount (void);	///< @return	The current number of zero-length packets that have been excluded
	static void								ResetExcludedZeroLengthPacketCount (void);	///< @brief		Resets my tally of excluded zero-length packets to zero.
	static bool								IsIncludingZeroLengthPackets (void);		///< @return	True if zero-length packets are included;  otherwise false.
	///@}


public:	//	INSTANCE METHODS
	/**
		@name	Construction, Destruction, Assignment & Copying
	**/
	///@{
											AJAAncillaryList ();			///< @brief	Instantiate and initialize with a default set of values.
	inline									AJAAncillaryList (const AJAAncillaryList & inRHS)	{*this = inRHS;}	///< @brief	My copy constructor.
	virtual									~AJAAncillaryList ();			///< @brief	My destructor.

	/**
		@brief	Assignment operator -- replaces my contents with the right-hand-side value.
		@param[in]	inRHS	The value to be assigned to me.
		@return		A reference to myself.
	**/
	virtual AJAAncillaryList &				operator = (const AJAAncillaryList & inRHS);
	///@}


	/**
		@name	Fetching, Searching & Enumerating Packets
	**/
	///@{

	/**
		@brief	Answers with the number of AJAAncillaryData objects I contain (any/all types).
		@return	The number of AJAAncillaryData objects I contain.
	**/
	virtual inline uint32_t					CountAncillaryData (void) const				{return uint32_t(m_ancList.size());}

	/**
		@return	True if I'm empty;  otherwise false.
	**/
	virtual inline bool						IsEmpty (void) const						{return !CountAncillaryData();}

	/**
		@brief		Answers with the AJAAncillaryData object at the given index.
		@param[in]	inIndex		Specifies the zero-based index position.
		@return		The AJAAncillaryData object at the given index (or NULL if not found).
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataAtIndex (const uint32_t inIndex) const;

	/**
		@brief		Answers with the number of AJAAncillaryData objects having the given type.
		@param[in]	inMatchType		Specifies the AJAAncillaryDataType to match.
		@return		The number of AJAAncillaryData objects having the given type.
	**/
	virtual uint32_t						CountAncillaryDataWithType (const AJAAncillaryDataType inMatchType) const;

	/**
		@brief		Answers with the AJAAncillaryData object having the given type and index.
		@param[in]	inMatchType		Specifies the AJAAncillaryDataType to match.
		@param[in]	inIndex			Specifies the desired instance of the given type (use zero for the first one).
		@return		The AJAAncillaryData object (or NULL if not found).
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataWithType (const AJAAncillaryDataType inMatchType, const uint32_t inIndex = 0) const;

	/**
		@brief		Answers with the number of AncillaryData objects having the given DataID and SecondaryID.
		@param[in]	inDID	Specifies the DataID to match. Use AJAAncillaryDataWildcard_DID to match any/all DIDs.
		@param[in]	inSID	Specifies the secondary ID to match. Use AJAAncillaryDataWildcard_SID to match any/all SIDs.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual uint32_t						CountAncillaryDataWithID (const uint8_t inDID, const uint8_t inSID) const;

	/**
		@brief		Answers with the AJAAncillaryData object having the given DataID and SecondaryID, at the given index.
		@param[in]	inDID		DataID to match (use AJAAncillaryDataWildcard_DID to match "any" DID)
		@param[in]	inSID		Secondary ID to match (use AJAAncillaryDataWildcard_SID to match "any" SID)
		@param[in]	inIndex		Specifies which instance among those having the given DID and SID (use zero for the first one).
		@return		The AJAAncillaryData object having the given DID, SID and index.
		@note		The AJAAncillaryList owns the returned object. If the list gets Cleared or deleted, the returned pointer will become invalid.
	**/
	virtual AJAAncillaryData *				GetAncillaryDataWithID (const uint8_t inDID, const uint8_t inSID, const uint32_t inIndex = 0) const;

	virtual AJAAncPktDIDSIDSet				GetAncillaryPacketIDs (void) const;	///< @return	The set of DID/SID pairs of all of my packets. (New in SDK 16.0)
	///@}


	/**
		@name	Adding & Removing Packets
	**/
	///@{

	/**
		@brief	Removes and frees all of my AJAAncillaryData objects.
		@return	AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						Clear (void);

	/**
		@brief		Adds (appends) a copy (using AJAAncillaryData::Clone) of the given list's packet objects to me.
		@param[in]	inPackets	Specifies the AJAAncillaryList containing the packets to be copied and added to me.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddAncillaryData (const AJAAncillaryList & inPackets);

	/**
		@brief		Adds (appends) a copy (using AJAAncillaryData::Clone) of the AJAAncillaryData object to me.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to be copied and added to me.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddAncillaryData (const AJAAncillaryData * pInAncData);

	/**
		@brief		Adds a copy of the AJAAncillaryData object to me.
		@param[in]	inAncData	Specifies the AJAAncillaryData object to be copied and added to me.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual inline AJAStatus				AddAncillaryData (const AJAAncillaryData & inAncData)	{return AddAncillaryData(&inAncData);}

	/**
		@brief		Removes all copies of the AJAAncillaryData object from me.
		@note		The given AJAAncillaryData object is not freed/deleted -- it's only removed from my list.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to remove.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						RemoveAncillaryData (AJAAncillaryData * pInAncData);

	/**
		@brief		Removes all copies of the AJAAncillaryData object from me and deletes the object itself.
		@param[in]	pInAncData	Specifies the AJAAncillaryData object to remove and delete.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						DeleteAncillaryData (AJAAncillaryData * pInAncData);
	///@}


	/**
		@name	Operations
	**/
	///@{

	/**
		@brief		Sort the AncillaryDataList by DataID (DID) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListByDID (void);

	/**
		@brief		Sort the AncillaryDataList by Secondary ID (SID) value.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListBySID (void);

	/**
		@brief		Sort the AncillaryDataList by "location", i.e. where in the video (field, line num, HANC/VANC)
					the data came from or will be inserted to.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						SortListByLocation (void);

	/**
		@brief		Compares me with another list.
		@param[in]	inCompareList		Specifies the other list to be compared with me.
		@param[in]	inIgnoreLocation	If true, don't compare each packet's AJAAncillaryDataLocation info. Defaults to true.
		@param[in]	inIgnoreChecksum	If true, don't compare each packet's checksums. Defaults to true.
		@return		AJA_STATUS_SUCCESS if equal;  otherwise AJA_STATUS_FAIL.
		@note		The sort order of each list, to be considered identical, must be the same.
	**/
	virtual AJAStatus						Compare (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation = true,  const bool inIgnoreChecksum = true) const;

	/**
		@brief		Compares me with another list and returns a std::string that contains a human-readable explanation
					of the first difference found (if any).
		@param[in]	inCompareList		Specifies the other list to be compared with me.
		@param[in]	inIgnoreLocation	If true, don't compare each packet's AJAAncillaryDataLocation info. Defaults to true.
		@param[in]	inIgnoreChecksum	If true, don't compare each packet's checksums. Defaults to true.
		@return		A string that contains a human-readable explanation of the first difference found (if any);
					or an empty string if the lists are identical.
		@note		The sort order of each list, to be considered identical, must be the same.
	**/
	virtual std::string						CompareWithInfo (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation = true,  const bool inIgnoreChecksum = true) const;
	///@}


	/**
		@name	Transmit to AJA Hardware (Playout)
	**/
	///@{
	/**
		@brief		Answers with the sizes of the buffers (one for field 1, one for field 2) needed to hold the anc data inserter
					"transmit" data for all of my AJAAncillaryData objects.
		@param[in]	inIsProgressive		Specify true for insertion into Progressive (transport) frames, or false for interlaced or psf.
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where "field 2" begins;  otherwise ignored.
		@param[out]	outF1ByteCount		Receives the size (in bytes) of the buffer needed to hold the "Field 1" anc data.
		@param[out]	outF2ByteCount		Receives the size (in bytes) of the buffer needed to hold the "Field 2" anc data.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetAncillaryDataTransmitSize (const bool inIsProgressive, const uint32_t inF2StartLine,
																			uint32_t & outF1ByteCount, uint32_t & outF2ByteCount);


	/**
		@brief		Builds one or two ancillary data buffers (one for field 1, one for field 2) with the anc data inserter
					"transmit" data for all of my AJAAncillaryData objects.
		@param[in]	inIsProgressive		Specify true for insertion into Progressive (transport) frames, or false for interlaced or psf.
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where "field 2" begins;  otherwise ignored.
		@param		pOutF1AncData		Specifies the valid, non-NULL starting address of the "Field 1" ancillary data buffer.
										Note that this buffer is written for Progressive frames.
		@param[in]	inF1ByteCountMax	Specifies the capacity (in bytes) of the Field 1 buffer (may be larger than needed).
		@param		pOutF2AncData		Specifies the valid, non-NULL starting address of the "Field 2" ancillary data buffer.
										Note that this buffer is not written for Progressive frames.
		@param[in]	inF2ByteCountMax	Specifies the capacity (in bytes) of the Field 2 buffer (may be larger than needed).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetAncillaryDataTransmitData (const bool inIsProgressive, const uint32_t inF2StartLine,
																			uint8_t * pOutF1AncData, const uint32_t inF1ByteCountMax,
																			uint8_t * pOutF2AncData, const uint32_t inF2ByteCountMax);

	/**
		@brief		Encodes my AJAAncillaryData packets into the given buffers in the default \ref ancgumpformat .
					The buffer contents are replaced;  the unused remainder, if any, will be zeroed.
		@param		F1Buffer			Specifies the buffer memory into which Field 1's anc data will be written.
		@param		F2Buffer			Specifies the buffer memory into which Field 2's anc data will be written.
		@param		inIsProgressive		Specify true to designate the output ancillary data stream as progressive; 
										otherwise, specify false. Defaults to true (is progressive).
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
										Defaults to zero (progressive). For interlaced video, see NTV2SmpteLineNumber::GetLastLine .
		@note		This function has a side-effect of automatically sorting my packets by ascending location before encoding.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
															const bool inIsProgressive = true, const uint32_t inF2StartLine = 0);

	virtual inline AJAStatus				GetSDITransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
																const bool inIsProgressive = true, const uint32_t inF2StartLine = 0)
																			{return GetTransmitData(F1Buffer, F2Buffer, inIsProgressive, inF2StartLine);}	///< @deprecated	An alias for GetTransmitData

	/**
		@brief		Writes my AJAAncillaryData objects into the given tall/taller frame buffer having the given raster/format.
		@param		inFrameBuffer		Specifies the frame buffer memory on the host to modify.
		@param[in]	inFormatDesc		Describes the frame buffer's raster and pixel format.
		@note		Before writing, I automatically sort my packets by location.
		@return		AJA_STATUS_SUCCESS if successful.
		@bug		Currently ignores each packet's horizontal offset (assumes AJAAncDataHorizOffset_Anywhere).
	**/
	virtual AJAStatus						GetVANCTransmitData (NTV2_POINTER & inFrameBuffer,  const NTV2FormatDescriptor & inFormatDesc);

	/**
		@brief		Explicitly encodes my AJAAncillaryData packets into the given buffers in \ref ancrtpformat .
					The buffer contents are replaced;  the unused remainder, if any, will be zeroed.
		@param		F1Buffer			Specifies the buffer memory into which Field 1's IP/RTP data will be written.
		@param		F2Buffer			Specifies the buffer memory into which Field 2's IP/RTP data will be written.
		@param[in]	inIsProgressive		Specify true to designate the output ancillary data stream as progressive; 
										otherwise, specify false. Defaults to true (is progressive).
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
										Defaults to zero (progressive).
		@note		This function has the following side-effects:
					-	Sorts my packets by ascending location before encoding.
					-	Calls AJAAncillaryData::GenerateTransmitData on each of my packets.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetIPTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
																const bool inIsProgressive = true, const uint32_t inF2StartLine = 0);

	/**
		@brief		Answers with the number of bytes required to store IP/RTP for my AJAAncillaryData packets in \ref ancrtpformat .
		@param[out]	outF1ByteCount		Receives the requisite byte count for Field 1's IP/RTP packet data.
		@param[out]	outF2ByteCount		Receives the requisite byte count for Field 1's IP/RTP packet data.
		@param[in]	inIsProgressive		Specify true to designate the output ancillary data stream as progressive; 
										otherwise, specify false. Defaults to true (is progressive).
		@param[in]	inF2StartLine		For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
										Defaults to zero (progressive).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetIPTransmitDataLength (uint32_t & outF1ByteCount, uint32_t & outF2ByteCount,
																	const bool inIsProgressive = true, const uint32_t inF2StartLine = 0);

	/**
		@brief		Answers true if multiple RTP packets will be transmitted/encoded.
					The default behavior is to transmit/encode a single RTP packet.
		@return		True if multiple RTP packets are allowed to be encoded;  otherwise false.
	**/
	virtual inline bool						AllowMultiRTPTransmit (void) const					{return m_xmitMultiRTP;}

	/**
		@brief		Determines if multiple RTP packets will be encoded for playout (via GetIPTransmitData).
					The default behavior is to transmit/encode a single RTP packet.
		@param[in]	inAllow		Specify true to allow encoding more than one RTP packet into the destination Anc buffer.
								Specify false to transmit/encode a single RTP packet (the default).
	**/
	virtual inline void						SetAllowMultiRTPTransmit (const bool inAllow)		{m_xmitMultiRTP = inAllow;}

#if !defined(NTV2_DEPRECATE_15_5)
	/**
		@deprecated	Use SetAllowMultiRTPTransmit with the 4-parameter version of this function.
	**/
	virtual NTV2_DEPRECATED_f(AJAStatus	GetIPTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
															const bool inIsProgressive, const uint32_t inF2StartLine,
															const bool inSingleRTPPkt))
			{	const bool oldValue(AllowMultiRTPTransmit());
				SetAllowMultiRTPTransmit(!inSingleRTPPkt);
				const AJAStatus result(GetIPTransmitData(F1Buffer, F2Buffer, inIsProgressive, inF2StartLine));
				SetAllowMultiRTPTransmit(oldValue);
				return result;
			}
	/**
		@deprecated	Use SetAllowMultiRTPTransmit with the 4-parameter version of this function.
	**/
	virtual NTV2_DEPRECATED_f(AJAStatus	GetIPTransmitDataLength (uint32_t & outF1ByteCount, uint32_t & outF2ByteCount,
																const bool inIsProgressive, const uint32_t inF2StartLine,
																const bool inSingleRTPPkt))
			{	const bool oldValue(AllowMultiRTPTransmit());
				SetAllowMultiRTPTransmit(!inSingleRTPPkt);
				const AJAStatus result(GetIPTransmitDataLength(outF1ByteCount, outF2ByteCount, inIsProgressive, inF2StartLine));
				SetAllowMultiRTPTransmit(oldValue);
				return result;
			}
#endif	//	!defined(NTV2_DEPRECATE_15_5)
	///@}


	/**
		@name	Receive from AJA Hardware (Ingest)
	**/
	///@{

	/**
		@brief		Parse "raw" ancillary data bytes received from hardware (ingest) -- see \ref ancgumpformat --
					into separate AJAAncillaryData objects and appends them to me.
		@param[in]	pInReceivedData		Specifies a valid, non-NULL address of the first byte of "raw" ancillary data received by an AncExtractor widget.
		@param[in]	inByteCount			Specifies the number of bytes of data in the specified buffer to process.
		@param[in]	inFrameNum			If non-zero, replaces the frame identifier of new packets that have a zero frame ID.
		@details	For each packet parsed from the received data, AJAAncillaryDataFactory::GuessAncillaryDataType
					is called to ascertain the packet's AJAAncillaryDataType, then AJAAncillaryDataFactory::Create
					is used to instantiate the specific AJAAncillaryData subclass instance.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddReceivedAncillaryData (const uint8_t * pInReceivedData, const uint32_t inByteCount, const uint32_t inFrameNum = 0);


	/**
		@brief		Parse a "raw" RTP packet received from hardware (ingest) in network byte order into separate
					AJAAncillaryData objects and appends them to me.
		@param[in]	inReceivedData		The received packet words in network byte order.
		@details	For each packet parsed from the received data, AJAAncillaryDataFactory::GuessAncillaryDataType
					is called to ascertain the packet's AJAAncillaryDataType, then AJAAncillaryDataFactory::Create
					is used to instantiate the specific AJAAncillaryData subclass instance.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddReceivedAncillaryData (const ULWordSequence & inReceivedData);


	/**
		@brief		Adds the packet that originated in the VANC lines of an NTV2 frame buffer to my list.
		@param[in]	inPacketWords		Specifies the "raw" 16-bit User Data Words of the packet to be added. The first
										six elements must be 0x0000, 0x03ff, 0x03ff, DID, SDID, DC, data words, and CS.
										Each word will have its upper byte masked off.
		@param[in]	inLocation			Specifies where the packet was found.
		@param[in]	inFrameNum			If non-zero, specifies/sets the frame identifier for the added packets.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						AddVANCData (const UWordSequence & inPacketWords,
														const AJAAncillaryDataLocation & inLocation,
														const uint32_t inFrameNum = 0);

	/**
		@brief		Answers true if multiple RTP packets are allowed for capture/receive.
					The default behavior is to process all (multiple) received RTP packets.
		@return		True if multiple RTP packets are allowed to be decoded;  otherwise false.
	**/
	virtual inline bool						AllowMultiRTPReceive (void) const					{return m_rcvMultiRTP;}

	/**
		@brief		Determines if more than one RTP packet will be processed/decoded (via AddReceivedAncillaryData).
		@param[in]	inAllow		Specify true to allow processing/decoding multiple RTP packets from the receiving Anc buffer.
								Specify false to only process/decode the first RTP packet found in the receiving Anc buffer.
	**/
	virtual inline void						SetAllowMultiRTPReceive (const bool inAllow)		{m_rcvMultiRTP = inAllow;}

	/**
		@brief		Answers if checksum errors are to be ignored or not.
					The default behavior is to not ignore them.
		@note		This applies to capture/ingest (i.e. AddReceivedAncillaryData methods).
		@return		True if ignoring checksum errors;  otherwise false.
	**/
	virtual inline bool						IgnoreChecksumErrors (void) const	{return m_ignoreCS;}

	/**
		@brief		Determines if checksum errors encountered during capture/ingest
					(via AddReceivedAncillaryData) will be ignored or not.
		@param[in]	inIgnore	Specify true to ignore checksum errors;  otherwise use false.
	**/
	virtual inline void						SetIgnoreChecksumErrors (const bool inIgnore)		{m_ignoreCS = inIgnore;}


#if !defined(NTV2_DEPRECATE_14_2)
	/**
		@deprecated	Use the 2-parameter version of this function instead.
	**/
	virtual NTV2_DEPRECATED_f(AJAStatus	AddVANCData (const UWordSequence & inPacketWords, const uint16_t inLineNum,
													const AJAAncillaryDataVideoStream inStream = AJAAncillaryDataChannel_Y));
#endif	//	!defined(NTV2_DEPRECATE_14_2)


	/**
		@brief		Sends a "ParsePayloadData" command to all of my AJAAncillaryData objects.
		@return		AJA_STATUS_SUCCESS if all items parse successfully;  otherwise the last failure result.
	**/
	virtual AJAStatus						ParseAllAncillaryData (void);
	///@}


	/**
		@name	Printing & Debugging
	**/
	///@{

	/**
		@brief		Dumps a human-readable description of every packet in my list to the given output stream.
		@param[in]	inDetailed		If true, include some of the packet data;  otherwise omit packet data.
									Defaults to true.
		@return		The specified output stream.
	**/
	virtual std::ostream &					Print (std::ostream & inOutStream, const bool inDetailed = true) const;
	///@}


protected:
	friend class CNTV2Card;	//	CNTV2Card's member functions can call AJAAncillaryList's private & protected member functions

	typedef std::list <AJAAncillaryData *>			AJAAncillaryDataList;
	typedef AJAAncillaryDataList::const_iterator	AJAAncDataListConstIter;	///< @brief	Handy const iterator for iterating over members of an AJAAncillaryDataList.
	typedef AJAAncillaryDataList::iterator			AJAAncDataListIter;			///< @brief	Handy non-const iterator for iterating over members of an AJAAncillaryDataList.

	virtual inline AJAAncillaryDataType				GetAnalogAncillaryDataType (const AJAAncillaryData & inAncData)	{return GetAnalogAncillaryDataTypeForLine(inAncData.GetLocationLineNumber());}

	static bool								BufferHasGUMPData (const NTV2_POINTER & inBuffer);

	/**
		@brief		Appends whatever can be decoded from the given device Anc buffer to the AJAAncillaryList.
		@param[in]	inAncBuffer			Specifies the Anc buffer to be parsed.
		@param		outPacketList		The AJAAncillaryList to be appended to, for whatever packets are found in the buffer.
		@param[in]	inFrameNum			If non-zero, replaces the frame identifier of packets that have a zero frame ID.
		@note		Called by SetFromDeviceAncBuffers, once for the F1 buffer, another time for the F2 buffer.
		@return		AJA_STATUS_SUCCESS if successful, including if no Anc packets are found and added to the list.
	**/
	static AJAStatus						AddFromDeviceAncBuffer (const NTV2_POINTER & inAncBuffer,
																	AJAAncillaryList & outPacketList,
																	const uint32_t inFrameNum = 0);

	/**
		@brief		Answers with my F1 & F2 SMPTE anc packets encoded as RTP ULWordSequences.
					The returned ULWords are already network-byte-order, ready to encapsulate into an RTP packet buffer.
		@param[out]	outF1U32Pkts	Receives my F1 AJAU32Pkts, containing zero or more RTP ULWordSequences.
		@param[out]	outF2U32Pkts	Receives my F1 AJAU32Pkts, containing zero or more RTP ULWordSequences.
		@param[out]	outF1AncCounts	Receives my F1 SMPTE Anc packet counts for each of the returned F1 RTP packets (in outF1U32Pkts).
		@param[out]	outF2AncCounts	Receives my F2 SMPTE Anc packet counts for each of the returned F2 RTP packets (in outF2U32Pkts).
		@param[in]	inIsProgressive	Specify false for interlace;  true for progressive/Psf.
		@param[in]	inF2StartLine	For interlaced/psf frames, specifies the line number where Field 2 begins;  otherwise ignored.
									Defaults to zero (progressive).
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	virtual AJAStatus						GetRTPPackets (AJAU32Pkts & outF1U32Pkts,
															AJAU32Pkts & outF2U32Pkts,
															AJAAncPktCounts & outF1AncCounts,
															AJAAncPktCounts & outF2AncCounts,
															const bool inIsProgressive,
															const uint32_t inF2StartLine);
	/**
		@brief		Fills the buffer with the given RTP packets.
		@param		theBuffer		The buffer to be filled. An empty/NULL buffer is permitted, and
									will copy no data, but instead will return the byte count that
									otherwise would've been written.
		@param[out]	outBytesWritten	Receives the total bytes written into the buffer (or that would
									be written if given a non-NULL buffer).
		@param[in]	inRTPPkts		The RTP packets, a vector of zero or more RTP ULWordSequences.
		@param[in]	inAncCounts		The per-RTP-packet anc packet counts.
		@param[in]	inIsF2			Specify false for Field1 (or progressive or Psf);  true for Field2.
		@param[in]	inIsProgressive	Specify false for interlace;  true for progressive/Psf.
		@return		AJA_STATUS_SUCCESS if successful.
	**/
	static AJAStatus						WriteRTPPackets (NTV2_POINTER & theBuffer,
															uint32_t & outBytesWritten,
															const AJAU32Pkts & inRTPPkts,
															const AJAAncPktCounts & inAncCounts,
															const bool inIsF2,
															const bool inIsProgressive);

private:
	AJAAncillaryDataList	m_ancList;		///< @brief	My packet list
	bool					m_rcvMultiRTP;	///< @brief	True: Rcv 1 RTP pkt per Anc pkt;  False: Rcv 1 RTP pkt for all Anc pkts
	bool					m_xmitMultiRTP;	///< @brief	True: Xmit 1 RTP pkt per Anc pkt;  False: Xmit 1 RTP pkt for all Anc pkts
	bool					m_ignoreCS;		///< @brief	True: ignore checksum errors;  False: don't ignore CS errors

};	//	AJAAncillaryList


/**
	@brief		Writes a human-readable rendition of the given AJAAncillaryList into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inList			Specifies the AJAAncillaryList to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
inline std::ostream & operator << (std::ostream & inOutStream, const AJAAncillaryList & inList)		{return inList.Print(inOutStream);}

/**
	@brief		Writes the given AJAU32Pkts object into the given output stream in a human-readable format.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inPkts			Specifies the AJAU32Pkts object to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAU32Pkts & inPkts);

/**
	@brief		Writes the given AJAAncPktDIDSIDSet set into the given output stream in a human-readable format.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inSet			Specifies the AJAAncPktDIDSIDSet to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const AJAAncPktDIDSIDSet & inSet);	//	New in SDK 16.0

#endif	// AJA_ANCILLARYLIST_H
