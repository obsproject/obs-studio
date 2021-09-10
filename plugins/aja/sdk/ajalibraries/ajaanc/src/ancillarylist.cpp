/* SPDX-License-Identifier: MIT */
/**
	@file		ancillarylist.cpp
	@brief		Implementation of the AJAAncillaryList class.
	@copyright	(C) 2010-2021 AJA Video Systems, Inc.
**/

#include "ancillarylist.h"
#include "ancillarydatafactory.h"
#include "ajabase/system/debug.h"
#include "ajantv2/includes/ntv2utils.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/lock.h"
#if defined (AJALinux)
	#include <string.h>		//	For memcpy
#endif	//	AJALinux

using namespace std;

#define	LOGMYERROR(__x__)	AJA_sERROR  (AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)
#define	LOGMYWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)
#define	LOGMYNOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)
#define	LOGMYINFO(__x__)	AJA_sINFO   (AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)
#define	LOGMYDEBUG(__x__)	AJA_sDEBUG  (AJA_DebugUnit_AJAAncList,		AJAFUNC << ": " << __x__)

#define	RCVFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Anc2110Rcv,		AJAFUNC << ": " << __x__)
#define	RCVWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Rcv,		AJAFUNC << ": " << __x__)
#define	RCVNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Rcv,		AJAFUNC << ": " << __x__)
#define	RCVINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_Anc2110Rcv,		AJAFUNC << ": " << __x__)
#define	RCVDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_Anc2110Rcv,		AJAFUNC << ": " << __x__)

#define	XMTFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Anc2110Xmit,		AJAFUNC << ": " << __x__)
#define	XMTWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Anc2110Xmit,		AJAFUNC << ": " << __x__)
#define	XMTNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Anc2110Xmit,		AJAFUNC << ": " << __x__)
#define	XMTINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_Anc2110Xmit,		AJAFUNC << ": " << __x__)
#define	XMTDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_Anc2110Xmit,		AJAFUNC << ": " << __x__)

#if defined(AJAHostIsBigEndian)
	//	Host is BigEndian (BE)
	#define AJA_ENDIAN_16NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		(__val__)
#else
	//	Host is LittleEndian (LE)
	#define AJA_ENDIAN_16NtoH(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_16HtoN(__val__)		AJA_ENDIAN_SWAP16(__val__)
	#define AJA_ENDIAN_32NtoH(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_32HtoN(__val__)		AJA_ENDIAN_SWAP32(__val__)
	#define AJA_ENDIAN_64NtoH(__val__)		AJA_ENDIAN_SWAP64(__val__)
	#define AJA_ENDIAN_64HtoN(__val__)		AJA_ENDIAN_SWAP64(__val__)
#endif

static inline uint32_t ENDIAN_32NtoH(const uint32_t inValue)	{return AJA_ENDIAN_32NtoH(inValue);}
//static inline uint32_t ENDIAN_32HtoN(const uint32_t inValue)	{return AJA_ENDIAN_32HtoN(inValue);}

static ostream & PrintULWordsBE (ostream & inOutStream, const ULWordSequence & inData, const size_t inMaxNum = 32)
{
	unsigned		numPrinted (0);
	inOutStream << DECN(inData.size(),3) << " U32s: ";
	for (ULWordSequenceConstIter iter(inData.begin());  iter != inData.end();  )
	{
		inOutStream << HEX0N(ENDIAN_32NtoH(*iter),8);
		numPrinted++;
		if (++iter != inData.end())
		{
			if (numPrinted > inMaxNum)
				{inOutStream << "...";	break;}
			inOutStream << " ";
		}
	}
	return inOutStream;
}

static string ULWordSequenceToStringBE (const ULWordSequence & inData, const size_t inMaxNum = 32)
{
	ostringstream oss;
	::PrintULWordsBE (oss, inData, inMaxNum);
	return oss.str();
}

ostream & operator << (ostream & inOutStream, const AJAU32Pkts & inPkts)
{
	unsigned	rtpPktNum(0);
	for (AJAU32PktsConstIter pktIter(inPkts.begin());  pktIter != inPkts.end();  ++pktIter)
	{	++rtpPktNum;
		inOutStream << "RTP PKT " << DEC0N(rtpPktNum,3) << ":";
		::PrintULWordsBE(inOutStream, *pktIter);
		inOutStream << endl;
	}	//	for each RTP packet
	return inOutStream;
}

ostream & operator << (ostream & inOutStream, const AJAAncPktDIDSIDSet & inSet)
{
	for (AJAAncPktDIDSIDSetConstIter it(inSet.begin());  it != inSet.end();  )
	{	inOutStream << xHEX0N(*it,4);
		if (++it != inSet.end())
			inOutStream << " ";
	}	//	for each DID/SID pair
	return inOutStream;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////


static bool		gIncludeZeroLengthPackets	(false);
static uint32_t	gExcludedZeroLengthPackets	(0);
static AJALock	gGlobalLock;

inline uint32_t AJAAncillaryList::GetExcludedZeroLengthPacketCount (void)
{	AJAAutoLock	locker(&gGlobalLock);
	return gExcludedZeroLengthPackets;
}

inline void AJAAncillaryList::ResetExcludedZeroLengthPacketCount (void)
{	AJAAutoLock	locker(&gGlobalLock);
	gExcludedZeroLengthPackets = 0;
}

inline bool AJAAncillaryList::IsIncludingZeroLengthPackets (void)
{	AJAAutoLock	locker(&gGlobalLock);
	return gIncludeZeroLengthPackets;
}

inline void AJAAncillaryList::SetIncludeZeroLengthPackets (const bool inInclude)
{	AJAAutoLock	locker(&gGlobalLock);
	gIncludeZeroLengthPackets = inInclude;
}

static inline void BumpZeroLengthPacketCount (void)
{
	AJAAtomic::Increment(&gExcludedZeroLengthPackets);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////


AJAAncillaryList::AJAAncillaryList ()
	:	m_ancList		(),
		m_rcvMultiRTP	(true),		//	By default, handle receiving multiple RTP packets
		m_xmitMultiRTP	(false),	//	By default, transmit single RTP packet
		m_ignoreCS		(false)
{
	Clear();
	SetAnalogAncillaryDataTypeForLine (20, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (21, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (22, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (283, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (284, AJAAncillaryDataType_Cea608_Line21);
	SetAnalogAncillaryDataTypeForLine (285, AJAAncillaryDataType_Cea608_Line21);
}


AJAAncillaryList::~AJAAncillaryList ()
{
	Clear();
}


AJAAncillaryList & AJAAncillaryList::operator = (const AJAAncillaryList & inRHS)
{
	if (this != &inRHS)
	{
		m_xmitMultiRTP = inRHS.m_xmitMultiRTP;
		m_rcvMultiRTP = inRHS.m_rcvMultiRTP;
		m_ignoreCS = inRHS.m_ignoreCS;
		Clear();
		for (AJAAncDataListConstIter it(inRHS.m_ancList.begin());  it != inRHS.m_ancList.end();  ++it)
			if (*it)
				AddAncillaryData((*it)->Clone());
	}
	return *this;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataAtIndex (const uint32_t inIndex) const
{
	AJAAncillaryData *	pAncData(AJA_NULL);

	if (!m_ancList.empty()  &&  inIndex < m_ancList.size())
	{
		AJAAncDataListConstIter	it	(m_ancList.begin());

		for (uint32_t i(0);  i < inIndex;  i++)	//	Dang, std::list has no random access
			++it;
		pAncData = *it;
	}

	return pAncData;
}


AJAStatus AJAAncillaryList::ParseAllAncillaryData (void)
{
	AJAStatus result = AJA_STATUS_SUCCESS;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;
		AJAStatus status = pAncData->ParsePayloadData ();

		//	Parse ALL items (even if one fails), but only return success if ALL succeed...
		if (AJA_FAILURE (status))
			result = status;
	}

	return result;
}


uint32_t AJAAncillaryList::CountAncillaryDataWithType (const AJAAncillaryDataType matchType) const
{
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *		pAncData	(*it);
		AJAAncillaryDataType	ancType		(pAncData->GetAncillaryDataType());

		if (matchType == ancType)
			count++;
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithType (const AJAAncillaryDataType matchType, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(AJA_NULL);
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *		pAncData (*it);
		AJAAncillaryDataType	ancType  (pAncData->GetAncillaryDataType());

		if (matchType == ancType)
		{
			if (index == count)
			{
				pResult = pAncData;
				break;
			}
			else
				count++;
		}
	}
	return pResult;
}


uint32_t AJAAncillaryList::CountAncillaryDataWithID (const uint8_t DID, const uint8_t SID) const
{
	uint32_t count = 0;

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData = *it;

		if (DID == AJAAncillaryDataWildcard_DID  ||  DID == pAncData->GetDID())
		{
			if (SID == AJAAncillaryDataWildcard_SID  ||  SID == pAncData->GetSID())
				count++;
		}
	}

	return count;
}


AJAAncillaryData * AJAAncillaryList::GetAncillaryDataWithID (const uint8_t inDID, const uint8_t inSID, const uint32_t index) const
{
	AJAAncillaryData *	pResult	(AJA_NULL);
	uint32_t count(0);

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		if (inDID == AJAAncillaryDataWildcard_DID  ||  inDID == (*it)->GetDID())
		{
			if (inSID == AJAAncillaryDataWildcard_SID  ||  inSID == (*it)->GetSID())
			{
				if (index == count)
				{
					pResult = *it;
					break;
				}
				count++;
			}
		}
	}
	return pResult;
}

AJAAncPktDIDSIDSet AJAAncillaryList::GetAncillaryPacketIDs (void) const
{
	AJAAncPktDIDSIDSet result;
	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		const uint16_t pktDIDSID(ToAJAAncPktDIDSID((*it)->GetDID(), (*it)->GetSID()));
		if (result.find(pktDIDSID) == result.end())
			result.insert(pktDIDSID);
	}
	return result;
}


AJAStatus AJAAncillaryList::AddAncillaryData (const AJAAncillaryList & inPackets)
{
	if (&inPackets == this)
		return AJA_STATUS_BAD_PARAM;	//	Cannot append from same list!
	for (AJAAncDataListConstIter it(inPackets.m_ancList.begin());  it != inPackets.m_ancList.end();  ++it)
	{
		const AJAAncillaryData *	pSrcPkt	(*it);
		if (!pSrcPkt)
			return AJA_STATUS_FAIL;
		AJAAncillaryData *	pNewPkt	(pSrcPkt->Clone());
		if (!pNewPkt)
			return AJA_STATUS_FAIL;
		m_ancList.push_back(pNewPkt);
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::AddAncillaryData (const AJAAncillaryData * pInAncData)
{
	if (!pInAncData)
		return AJA_STATUS_NULL;

	// Note: this makes a clone of the incoming AJAAncillaryData object and
	// saves it in the List. The caller is responsible for deleting the original,
	// and the List owns the clone.
	const bool			wasEmpty	(m_ancList.empty());
	AJAAncillaryData *	pData		(pInAncData->Clone());
	if (!pData)
		return AJA_STATUS_FAIL;

	// add it to the list
	if (pData)
		m_ancList.push_back(pData);

	LOGMYDEBUG(DEC(m_ancList.size()) << " packet(s) stored" << (wasEmpty ? " from" : " after appending") << " packet " << pData->AsString(32));
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::Clear (void)
{
	uint32_t		numDeleted	(0);
	const uint32_t	oldSize		(uint32_t(m_ancList.size()));
	for (AJAAncDataListConstIter it (m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData * pAncData(*it);
		if (pAncData)
		{
			delete pAncData;
			numDeleted++;
		}
	}

	m_ancList.clear();
	if (oldSize || numDeleted)
		LOGMYDEBUG(numDeleted << " packet(s) deleted -- list emptied");
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::RemoveAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;

	m_ancList.remove(pAncData);	//	note:	there's no feedback as to whether one or more elements existed or not
								//	note:	pAncData is NOT deleted!
	LOGMYDEBUG(DEC(m_ancList.size()) << " packet(s) remain after removing packet " << pAncData->AsString(32));
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::DeleteAncillaryData (AJAAncillaryData * pAncData)
{
	if (!pAncData)
		return AJA_STATUS_NULL;

	AJAStatus status (RemoveAncillaryData(pAncData));
	if (AJA_SUCCESS(status))
		delete pAncData;
	return status;
}


//	Sort Predicates

static bool SortByDID (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
  return lhs->GetDID() < rhs->GetDID();
}


static bool SortBySID (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
	return lhs->GetSID() < rhs->GetSID();
}

static bool SortByLocation (AJAAncillaryData * lhs, AJAAncillaryData * rhs)
{
	const AJAAncillaryDataLocation &	locLHS (lhs->GetDataLocation());
	const AJAAncillaryDataLocation &	locRHS (rhs->GetDataLocation());
	return locLHS < locRHS;
}


//	Sort Methods

AJAStatus AJAAncillaryList::SortListByDID (void)
{
	m_ancList.sort (SortByDID);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SortListBySID (void)
{
	m_ancList.sort (SortBySID);
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJAAncillaryList::SortListByLocation (void)
{
	m_ancList.sort (SortByLocation);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::Compare (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation, const bool inIgnoreChecksum) const
{
	if (inCompareList.CountAncillaryData() != CountAncillaryData())
		return AJA_STATUS_FAIL;
	for (uint32_t ndx (0);  ndx < CountAncillaryData();  ndx++)
	{
		AJAAncillaryData *	pPktA	(inCompareList.GetAncillaryDataAtIndex(ndx));
		AJAAncillaryData *	pPktB	(GetAncillaryDataAtIndex(ndx));
		if (AJA_FAILURE(pPktA->Compare(*pPktB, inIgnoreLocation, inIgnoreChecksum)))
			return AJA_STATUS_FAIL;
	}	//	for each packet
	return AJA_STATUS_SUCCESS;
}


string AJAAncillaryList::CompareWithInfo (const AJAAncillaryList & inCompareList, const bool inIgnoreLocation,  const bool inIgnoreChecksum) const
{
	ostringstream	oss;
	if (inCompareList.CountAncillaryData() != CountAncillaryData())
		{oss << "Packet count mismatch: " << DEC(CountAncillaryData()) << " vs " << DEC(inCompareList.CountAncillaryData());  return oss.str();}

	for (uint32_t ndx (0);  ndx < CountAncillaryData();  ndx++)
	{
		AJAAncillaryData *	pPktRHS	(inCompareList.GetAncillaryDataAtIndex(ndx));
		AJAAncillaryData *	pPkt	(GetAncillaryDataAtIndex(ndx));
		const string		info	(pPkt->CompareWithInfo(*pPktRHS, inIgnoreLocation, inIgnoreChecksum));
		if (!info.empty())
			{oss << "Pkt " << DEC(ndx+1) << " of " << DEC(CountAncillaryData()) << ": " << pPkt->AsString() << " != " << pPktRHS->AsString() << ": " << info;  return oss.str();}
	}	//	for each packet
	return string();
}


static bool TestForAnalogContinuation (AJAAncillaryData * pPrevData, AJAAncillaryData * pNewData)
{
	if (!pPrevData || !pNewData)
		return false;

	//	Get coding type and location for the previous ancillary packet...
	AJAAncillaryDataCoding		prevCoding	(pPrevData->GetDataCoding ());
	AJAAncillaryDataLocation	prevLoc		(pPrevData->GetDataLocation ());

	//	Get coding type and location for the new ancillary packet...
	AJAAncillaryDataCoding		newCoding	(pNewData->GetDataCoding ());
	AJAAncillaryDataLocation	newLoc		(pNewData->GetDataLocation ());

	//	Compare...
	if (   (prevCoding == AJAAncillaryDataCoding_Analog)
		&& (newCoding  == AJAAncillaryDataCoding_Analog)
		&& (prevLoc.GetLineNumber()  == newLoc.GetLineNumber())
		&& (prevLoc.GetHorizontalOffset()  == newLoc.GetHorizontalOffset())	// technically, these should ALWAYS be the same for "analog"...?
		&& (prevLoc.GetDataSpace()   == newLoc.GetDataSpace())		//	Should always be VANC
		&& (prevLoc.GetDataLink()    == newLoc.GetDataLink())		//	Link A or B -- should match
		&& (prevLoc.GetDataStream()  == newLoc.GetDataStream())		//	DS1 or DS2 -- should match
		&& (prevLoc.GetDataChannel() == newLoc.GetDataChannel()) )	//	Y or C -- should match
			return true;
	return false;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////	R E C E I V E
/////////////////////////////////////////////////////////////////

// Parse a stream of "raw" ancillary data as collected by an AJAAncExtractorWidget.
// Break the stream into separate AJAAncillaryData objects and add them to the list.
//
AJAStatus AJAAncillaryList::AddReceivedAncillaryData (const uint8_t * pRcvData,
														const uint32_t dataSize,
														const uint32_t inFrameNum)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	if (!pRcvData || !dataSize)
		return AJA_STATUS_NULL;

	//	Use this as an uninitialized template...
	AJAAncillaryData	newAncData;
	AJAAncDataLoc		defaultLoc		(AJAAncillaryDataLink_A, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, 9);
	int32_t				remainingSize	(int32_t(dataSize + 0));
	const uint8_t *		pInputData		(pRcvData);
	bool				bMoreData		(true);

	while (bMoreData)
	{
		bool bInsertNew	(false);	//	We'll set this 'true' if/when we find a new Anc packet to insert
		AJAAncillaryDataType newAncType	(AJAAncillaryDataType_Unknown);	//	We'll set this to the proper type once we know it
		uint32_t packetSize	(0);	//	This is where the AncillaryData object returns the number of bytes that were "consumed" from the input stream

		//	Reset the AncData object, then load itself from the next GUMP packet...
		newAncData.Clear();
		status = newAncData.InitWithReceivedData (pInputData, size_t(remainingSize), defaultLoc, packetSize);
		if (AJA_FAILURE(status))
		{
			//	TODO:	Someday, let's try to recover and process subsequent packets.
			break;		//	NOTE:	For now, bail on errors in the GUMP stream
		}
		else if (packetSize == 0)
			break;		//	Nothing to do

		//	Determine what type of anc data we have, and create an object of the appropriate class...
		if (newAncData.IsDigital())
		{
			//	Digital anc packets are fairly easy to categorize: you just have to look at their DID/SID.
			//	Also, they are (by definition) independent packets which become independent AJAAncillaryData objects.
			newAncType = AJAAncillaryDataFactory::GuessAncillaryDataType(&newAncData);
			bInsertNew = true;		//	Add it to the list
		}	// digital anc data
		else if (newAncData.IsAnalog())
		{	//	"Analog" packets are trickier...
			//	1)	Digitized analog lines are broken into multiple packets by the hardware,
			//		which need to be recombined into a single AJAAncillaryData object.
			//	2)	It is harder to classify the data type, since there is no single easy-to-read
			//		ID. Instead, we rely on the user to tell us what lines are expected to contain
			//		what kind of data (e.g. "expect line 21 to carry 608 captioning...").

			//	First, see if the LAST AJAAncillaryData object came from analog space and the same
			//	location as this new one. If so, assume that the new data is simply a continuation
			//	of the previous packet, and append the payload data accordingly.
			bool bAppendAnalog (false);

			if (!m_ancList.empty())
			{
				AJAAncillaryData *	pPrevData (m_ancList.back());
				if (::TestForAnalogContinuation (pPrevData, &newAncData))
				{	//	The new data is just a continuation of the previous packet:
					//	simply append the new payload data to the previous payload...
					pPrevData->AppendPayload(newAncData);
					bAppendAnalog = true;
				}
			}

			if (!bAppendAnalog)
			{	//	If this is NOT a "continuation" packet, then this is a new analog packet.
				//	See if the user has specified an expected Anc data type that matches the
				//	location of the new data...
				newAncType = GetAnalogAncillaryDataType(newAncData);
				bInsertNew = true;
			}
		}	// analog anc data
//			else 
//				Anc Coding Unknown????	(ignore it)

		if (bInsertNew)
		{
			//	Create an AJAAncillaryData object of the appropriate type, and init it with our raw data...
			AJAAncillaryData *	pData	(AJAAncillaryDataFactory::Create (newAncType, &newAncData));
			if (pData)
			{
				pData->SetBufferFormat(AJAAncillaryBufferFormat_SDI);
				if (IsIncludingZeroLengthPackets()  ||  pData->GetDC())
				{
					try	{m_ancList.push_back(pData);}	//	Append to my list
					catch(...)	{status = AJA_STATUS_FAIL;}
				}
				else ::BumpZeroLengthPacketCount();
				if (inFrameNum  &&  !pData->GetFrameID())
					pData->SetFrameID(inFrameNum);
			}
			else
				status = AJA_STATUS_FAIL;
		}

		remainingSize -= packetSize;	//	Decrease the remaining data size by the amount we just "consumed"
		pInputData += packetSize;		//	Advance the input data pointer by the same amount
		if (remainingSize <= 0)			//	All of the input data consumed?
			bMoreData = false;
	}	// while (bMoreData)

	return status;

}	//	AddReceivedAncillaryData


//	Parse a "raw" RTP packet received from hardware (ingest) in network byte order into separate
//	AJAAncillaryData objects and append them to me. 'inReceivedData' includes the RTP header.
AJAStatus AJAAncillaryList::AddReceivedAncillaryData (const ULWordSequence & inReceivedData)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	if (inReceivedData.empty())
		{LOGMYWARN("Empty RTP data vector");  return AJA_STATUS_SUCCESS;}

LOGMYDEBUG(::ULWordSequenceToStringBE(inReceivedData) << " (BigEndian)");	//	ByteSwap em to make em look right

	//	Crack open the RTP packet header...
	AJARTPAncPayloadHeader	RTPheader;
	if (!RTPheader.ReadFromULWordVector(inReceivedData))
		{LOGMYERROR("AJARTPAncPayloadHeader::ReadULWordVector failed, " << DEC(4*inReceivedData.size()) << " header bytes");  return AJA_STATUS_FAIL;}
	if (RTPheader.IsNULL())
		{LOGMYWARN("No anc packets added: NULL RTP header: " << RTPheader);  return AJA_STATUS_SUCCESS;}	//	Not an error
	if (!RTPheader.IsValid())
		{LOGMYWARN("RTP header invalid: " << RTPheader);  return AJA_STATUS_FAIL;}

	const size_t	predictedPayloadSize	(RTPheader.GetPayloadLength() / sizeof(uint32_t));	//	Payload length (excluding RTP header)
	const size_t	actualPayloadSize		(inReceivedData.size() - AJARTPAncPayloadHeader::GetHeaderWordCount());
	const uint32_t	numPackets				(RTPheader.GetAncPacketCount());
	uint32_t		pktsAdded				(0);

	//	Sanity check the RTP header against inReceivedData...
	if (actualPayloadSize < predictedPayloadSize)
		{LOGMYERROR("Expected " << DEC(predictedPayloadSize) << ", but only given " << DEC(actualPayloadSize) << " U32s: " << RTPheader);  return AJA_STATUS_BADBUFFERCOUNT;}
	if (!numPackets)
		{LOGMYWARN("No Anc packets to append: " << RTPheader);  return AJA_STATUS_SUCCESS;}
	if (!actualPayloadSize)
		{LOGMYWARN("No payload data yet non-zero packet count: " << RTPheader);  return AJA_STATUS_FAIL;}

LOGMYDEBUG(RTPheader);

	//	Parse each anc pkt in the RTP pkt...
	uint16_t	u32Ndx	(5);	//	First Anc packet starts at ULWord[5]
	unsigned	pktNum	(0);
	for (;  pktNum < numPackets  &&  AJA_SUCCESS(status);  pktNum++)
	{
		AJAAncillaryData	tempPkt;
		status = tempPkt.InitWithReceivedData(inReceivedData, u32Ndx, IgnoreChecksumErrors());
		if (AJA_FAILURE(status))
			continue;

		const AJAAncillaryDataType newAncType (AJAAncillaryDataFactory::GuessAncillaryDataType(tempPkt));
		AJAAncillaryData *	pNewPkt	(AJAAncillaryDataFactory::Create (newAncType, tempPkt));
		if (!pNewPkt)
			{status = AJA_STATUS_NULL;  continue;}

		pNewPkt->SetBufferFormat(AJAAncillaryBufferFormat_RTP);	//	Originated in RTP packet
		pNewPkt->SetFrameID(RTPheader.GetTimeStamp());			//	TimeStamp it using RTP timestamp
		if (IsIncludingZeroLengthPackets()  ||  pNewPkt->GetDC())
		{
			try	{m_ancList.push_back(pNewPkt);  pktsAdded++;}	//	Append to my list
			catch(...)	{status = AJA_STATUS_FAIL;}
		}
		else ::BumpZeroLengthPacketCount();
	}	//	for each anc packet

	if (AJA_FAILURE(status))
		LOGMYERROR(::AJAStatusToString(status) << ": Failed at pkt[" << DEC(pktNum) << "] of " << DEC(numPackets));
	if (CountAncillaryData() < numPackets)
		LOGMYWARN(DEC(pktsAdded) << " of " << DEC(numPackets) << " anc pkt(s) decoded from RTP pkt");
	else
		LOGMYINFO(DEC(numPackets) << " pkts added from RTP pkt: " << *this);
	return status;
}	//	AddReceivedAncillaryData


static AJAStatus AppendUWordPacketToGump (	UByteSequence &					outGumpPkt,
											const UWordSequence &			inPacketWords,
											const AJAAncillaryDataLocation	inLoc = AJAAncillaryDataLocation(AJAAncillaryDataLink_A,
																											AJAAncillaryDataChannel_Y,
																											AJAAncillaryDataSpace_VANC,
																											0))
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);

	if (inPacketWords.size () < 7)
		return AJA_STATUS_RANGE;
//	NOTE: Tough call. Decided not to validate the AJAAncillaryDataLocation here:
//	if (!inLoc.IsValid())
//		return AJA_STATUS_BAD_PARAM;

	//	Use this as an uninitialized template...
	UWordSequenceConstIter	iter(inPacketWords.begin());

	//	Anc packet must start with 0x000 0x3FF 0x3FF sequence...
	if (*iter != 0x0000)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;
	if (*iter != 0x03FF)
		return AJA_STATUS_FAIL;
	++iter;	//	Now pointing at DID

	outGumpPkt.reserve (outGumpPkt.size() + inPacketWords.size());	//	Expand outGumpPkt's capacity in one step
	const UByteSequence::size_type	dataByte1Ndx	(outGumpPkt.size()+1);	//	Index of byte[1] in outgoing Gump packet
	outGumpPkt.push_back(0xFF);											//	[0]	First byte always 0xFF
	outGumpPkt.push_back(0x80);											//	[1]	Location data byte 1:	"location valid" bit always set
	outGumpPkt[dataByte1Ndx] |= (inLoc.GetLineNumber() >> 7) & 0x0F;	//	[1]	Location data byte 1:	LS 4 bits == MS 4 bits of 11-bit line number
	if (inLoc.IsLumaChannel())
		outGumpPkt[dataByte1Ndx] |= 0x20;								//	[1]	Location data byte 1:	set Y/C bit for Y/luma channel
	if (inLoc.IsHanc())
		outGumpPkt[dataByte1Ndx] |= 0x10;								//	[1]	Location data byte 1:	set H/V bit for HANC
	outGumpPkt.push_back (inLoc.GetLineNumber() & 0x7F);				//	[2]	Location data byte 2:	MSB reserved; LS 7 bits == LS 7 bits of 11-bit line number

	while (iter != inPacketWords.end())
	{
		outGumpPkt.push_back(*iter & 0xFF);	//	Mask off upper byte
		++iter;
	}
	return status;
}


AJAStatus AJAAncillaryList::AddVANCData (const UWordSequence & inPacketWords, const AJAAncillaryDataLocation & inLocation, const uint32_t inFrameNum)
{
	UByteSequence	gumpPacketData;
	AJAStatus		status	(AppendUWordPacketToGump (gumpPacketData,  inPacketWords, inLocation));
	if (AJA_FAILURE(status))
		return status;

	AJAAncillaryData	pkt;
	status = pkt.InitWithReceivedData (gumpPacketData, inLocation);
	if (AJA_FAILURE(status))
		return status;
	pkt.SetBufferFormat(AJAAncillaryBufferFormat_FBVANC);

	AJAAncillaryDataType	newAncType	(AJAAncillaryDataFactory::GuessAncillaryDataType(pkt));
	AJAAncillaryData *		pData		(AJAAncillaryDataFactory::Create(newAncType, pkt));
	if (!pData)
		return AJA_STATUS_FAIL;

	if (IsIncludingZeroLengthPackets()  ||  pData->GetDC())
	{
		try	{m_ancList.push_back(pData);}	//	Append to my list
		catch(...)	{return AJA_STATUS_FAIL;}
	}
	else ::BumpZeroLengthPacketCount();

	if (inFrameNum  &&  pData->GetDID())
		pData->SetFrameID(inFrameNum);

	return AJA_STATUS_SUCCESS;

}	//	AddVANCData


#if !defined(NTV2_DEPRECATE_14_2)
	AJAStatus AJAAncillaryList::AddVANCData (const UWordSequence & inPacketWords, const uint16_t inLineNum, const AJAAncillaryDataChannel inChannel)
	{
		return AddVANCData (inPacketWords, AJAAncillaryDataLocation (AJAAncillaryDataLink_A,  inChannel, AJAAncillaryDataSpace_VANC,  inLineNum));
	}	//	AddVANCData
#endif	//	!defined(NTV2_DEPRECATE_14_2)


AJAStatus AJAAncillaryList::SetFromVANCData (const NTV2_POINTER &			inFrameBuffer,
											const NTV2FormatDescriptor &	inFormatDesc,
											AJAAncillaryList &				outPackets,
											const uint32_t					inFrameNum)
{
	outPackets.Clear();

	if (inFrameBuffer.IsNULL())
	{
		LOGMYERROR("AJA_STATUS_NULL: NULL frame buffer pointer");
		return AJA_STATUS_NULL;
	}
	if (!inFormatDesc.IsValid())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: bad NTV2FormatDescriptor");
		return AJA_STATUS_BAD_PARAM;
	}
	if (!inFormatDesc.IsVANC())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: format descriptor has no VANC lines");
		return AJA_STATUS_BAD_PARAM;
	}

	const ULWord			vancBytes	(inFormatDesc.GetTotalRasterBytes() - inFormatDesc.GetVisibleRasterBytes());
	const NTV2PixelFormat	fbf			(inFormatDesc.GetPixelFormat());
	const bool				isSD		(NTV2_IS_SD_STANDARD(inFormatDesc.GetVideoStandard()));
	if (inFrameBuffer.GetByteCount() < vancBytes)
	{
		LOGMYERROR("AJA_STATUS_FAIL: " << inFrameBuffer.GetByteCount() << "-byte frame buffer smaller than " << vancBytes << "-byte VANC region");
		return AJA_STATUS_FAIL;
	}
	if (fbf != NTV2_FBF_10BIT_YCBCR  &&  fbf != NTV2_FBF_8BIT_YCBCR)
	{
		LOGMYERROR("AJA_STATUS_UNSUPPORTED: frame buffer format " << ::NTV2FrameBufferFormatToString(fbf) << " not '2vuy' nor 'v210'");
		return AJA_STATUS_UNSUPPORTED;	//	Only 'v210' and '2vuy' currently supported
	}

	for (ULWord lineOffset (0);  lineOffset < inFormatDesc.GetFirstActiveLine();  lineOffset++)
	{
		UWordSequence	uwords;
		bool			isF2			(false);
		ULWord			smpteLineNum	(0);
		unsigned		ndx				(0);

		inFormatDesc.GetSMPTELineNumber (lineOffset, smpteLineNum, isF2);
		if (fbf == NTV2_FBF_10BIT_YCBCR)
			::UnpackLine_10BitYUVtoUWordSequence (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), lineOffset),
													inFormatDesc, uwords);
		else
			AJAAncillaryData::Unpack8BitYCbCrToU16sVANCLine (inFormatDesc.GetRowAddress(inFrameBuffer.GetHostAddress(0), lineOffset),
															uwords,  inFormatDesc.GetRasterWidth());
		if (isSD)
		{
			AJAAncillaryData::U16Packets	ycPackets;
			UWordSequence					ycHOffsets;
			AJAAncillaryDataLocation		loc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Both, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));

			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_Both, ycPackets, ycHOffsets);
			NTV2_ASSERT(ycPackets.size() == ycHOffsets.size());

			for (AJAAncillaryData::U16Packets::const_iterator it(ycPackets.begin());  it != ycPackets.end();  ++it, ndx++)
				outPackets.AddVANCData (*it, loc.SetHorizontalOffset(ycHOffsets[ndx]), inFrameNum);
		}
		else
		{
			AJAAncillaryData::U16Packets	yPackets, cPackets;
			UWordSequence					yHOffsets, cHOffsets;
			AJAAncillaryDataLocation		yLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_Y, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));
			AJAAncillaryDataLocation		cLoc	(AJAAncillaryDataLink_Unknown, AJAAncillaryDataChannel_C, AJAAncillaryDataSpace_VANC, uint16_t(smpteLineNum));
			//cerr << endl << "SetFromVANCData: +" << DEC0N(lineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, lineOffset);  cerr << ":" << endl << uwords << endl;
			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_Y, yPackets, yHOffsets);
			AJAAncillaryData::GetAncPacketsFromVANCLine (uwords, AncChannelSearch_C, cPackets, cHOffsets);
			NTV2_ASSERT(yPackets.size() == yHOffsets.size());
			NTV2_ASSERT(cPackets.size() == cHOffsets.size());

			unsigned	ndxx(0);
			for (AJAAncillaryData::U16Packets::const_iterator it(yPackets.begin());  it != yPackets.end();  ++it, ndxx++)
				outPackets.AddVANCData (*it, yLoc.SetHorizontalOffset(yHOffsets[ndxx]), inFrameNum);

			ndxx = 0;
			for (AJAAncillaryData::U16Packets::const_iterator it(cPackets.begin());  it != cPackets.end();  ++it, ndxx++)
				outPackets.AddVANCData (*it, cLoc.SetHorizontalOffset(cHOffsets[ndxx]), inFrameNum);
		}
	}	//	for each VANC line
	LOGMYDEBUG("returning " << outPackets);
	return AJA_STATUS_SUCCESS;
}

//	STATIC
AJAStatus AJAAncillaryList::AddFromDeviceAncBuffer (const NTV2_POINTER & inAncBuffer,
													AJAAncillaryList & outPackets,
													const uint32_t inFrameNum)
{
	uint32_t		RTPPacketCount	(0);	//	Number of packets encountered
	const uint32_t	origPktCount	(outPackets.CountAncillaryData());
	AJAStatus		result			(AJA_STATUS_SUCCESS);

	//	Try GUMP first...
	if (BufferHasGUMPData(inAncBuffer))
	{	//	GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP  GUMP
		result = outPackets.AddReceivedAncillaryData (inAncBuffer, inAncBuffer.GetByteCount(), inFrameNum);
		if (result == AJA_STATUS_NULL)
			result = AJA_STATUS_SUCCESS;	//	A NULL/empty buffer is not an error
	}	//	if GUMP
	else
	{
		//	RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP   RTP
		NTV2_POINTER	ancBuffer		(inAncBuffer.GetHostPointer(), inAncBuffer.GetByteCount());	//	Don't copy
		size_t			ULWordCount		(0);	//	Size of current RTP packet, including RTP header, in 32-bit words
		size_t			ULWordOffset	(0);	//	Offset to start of current RTP packet, in 32-bit words
		unsigned		retries			(0);	//	Retry count
		const unsigned	MAX_RETRIES		(4);	//	Max number of U32s past the end of an RTP packet to look for another

		while (ancBuffer  &&  retries++ < MAX_RETRIES)
		{
			if (AJARTPAncPayloadHeader::BufferStartsWithRTPHeader(ancBuffer))
			{
				ULWordSequence			U32s;
				AJARTPAncPayloadHeader	rtpHeader;

				++RTPPacketCount;	//	Increment our packet tally

				//	Read the RTP packet header to discover the RTP packet's true length...
				if (!rtpHeader.ReadFromBuffer(ancBuffer))
				{
					RCVWARN("On RTP pkt " << DEC(RTPPacketCount) << ", RTP hdr ReadFromBuffer failed at: " << ancBuffer.AsString(40));
					break;
				}

				ULWordCount = rtpHeader.GetPayloadLength() / sizeof(uint32_t)		//	payload size
								+  AJARTPAncPayloadHeader::GetHeaderWordCount();	//	plus RTP header size

				//	Read ULWordCount x U32s from ancBuffer...
				if (!ancBuffer.GetU32s (U32s,  /*U32Offset=*/0,  /*maxU32sToGet=*/ULWordCount))
				{
					RCVFAIL("On RTP pkt " << DEC(RTPPacketCount) << ", GetU32s(" << DEC(ULWordCount) << ") at: " << ancBuffer.AsString(40));
					return AJA_STATUS_BADBUFFERSIZE;	//	Ran off the end?
				}

				retries = 0;	//	Reset our retry counter when we get a good header

				//	Process the full RTP packet U32s...
				result = outPackets.AddReceivedAncillaryData(U32s);
				if (AJA_FAILURE(result))
					break;	//	Done -- failed!

				if (!outPackets.AllowMultiRTPReceive())
					break;	//	Only one RTP packet allowed -- done -- success!
			}	//	
			else
				ULWordCount = 1;	//	No RTP header found -- move ahead 1 x U32

			//	Move "ancBuffer" forward inside "inAncBuffer" to check for another RTP packet...
			ULWordOffset += ULWordCount;
			ancBuffer.Set (inAncBuffer.GetHostAddress(ULWord(ULWordOffset * sizeof(uint32_t))),	//	Increment startAddress
							inAncBuffer.GetByteCount() - ULWordOffset * sizeof(uint32_t));		//	Decrement byteCount
			RCVDBG("Moved buffer " << inAncBuffer << " forward by " << DEC(ULWordCount) << " U32s: " << ancBuffer.AsString(20));
		}	//	loop til no more RTP packets found
	}	//	else RTP

	const uint32_t	pktsAdded (outPackets.CountAncillaryData() - origPktCount);
	if (AJA_SUCCESS(result))
		LOGMYDEBUG("Success:  " << DEC(pktsAdded) << " pkts added");
	else
		LOGMYERROR(AJAStatusToString(result) << ": " << DEC(pktsAdded) << " pkts added");
	return result;

}	//	AddFromDeviceAncBuffer


//	STATIC
AJAStatus AJAAncillaryList::SetFromDeviceAncBuffers (const NTV2_POINTER & inF1AncBuffer,
													const NTV2_POINTER & inF2AncBuffer,
													AJAAncillaryList & outPackets,
													const uint32_t inFrameNum)		//	STATIC
{
	outPackets.Clear();
	const AJAStatus	resultF1 (AddFromDeviceAncBuffer(inF1AncBuffer, outPackets, inFrameNum));
	const AJAStatus	resultF2 (AddFromDeviceAncBuffer(inF2AncBuffer, outPackets, inFrameNum));
	if (AJA_FAILURE(resultF1))
		return resultF1;
	if (AJA_FAILURE(resultF2))
		return resultF2;
	return AJA_STATUS_SUCCESS;
}


bool AJAAncillaryList::BufferHasGUMPData (const NTV2_POINTER & inBuffer)
{
	if (!inBuffer)
		return false;
	const uint8_t *	pBytes (reinterpret_cast <const uint8_t*>(inBuffer.GetHostPointer()));
	if (!pBytes)
		return false;
	return *pBytes == 0xFF;
}

static const size_t		MAX_RTP_PKT_LENGTH_BYTES	(0x0000FFFF);	//	65535 max
static const size_t		MAX_RTP_PKT_LENGTH_WORDS	((MAX_RTP_PKT_LENGTH_BYTES+1) / sizeof(uint32_t) - 1);	//	16383 max
static const uint32_t	MAX_ANC_PKTS_PER_RTP_PKT	(0x000000FF);	//	255 max


AJAStatus AJAAncillaryList::GetRTPPackets (AJAU32Pkts & outF1U32Pkts,  AJAU32Pkts & outF2U32Pkts,
											AJAAncPktCounts & outF1AncCounts,  AJAAncPktCounts & outF2AncCounts,
											const bool inIsProgressive,  const uint32_t inF2StartLine)
{
	AJAStatus	result				(AJA_STATUS_SUCCESS);
	uint32_t	actF1PktCnt			(0);
	uint32_t	actF2PktCnt			(0);
	uint32_t	pktNdx				(0);
	size_t		oldPktLengthWords	(0);
	unsigned	countOverflows		(0);
	size_t		overflowWords		(0);

	//	Reserve space in AJAU32Pkts vectors...
	outF1U32Pkts.reserve(CountAncillaryData());		outF1U32Pkts.clear();
	outF2U32Pkts.reserve(CountAncillaryData());		outF2U32Pkts.clear();
	outF1AncCounts.clear();  outF2AncCounts.clear();

	ULWordSequence	F1U32s, F2U32s;
	F1U32s.reserve(MAX_RTP_PKT_LENGTH_WORDS);
	if (!inIsProgressive)
		F2U32s.reserve(MAX_RTP_PKT_LENGTH_WORDS);
	F1U32s.clear();	F2U32s.clear();

	//	Generate transmit data for each of my packets...
	for (pktNdx = 0;  pktNdx < CountAncillaryData();  pktNdx++)
	{
		AJAAncillaryData *	pAncData (GetAncillaryDataAtIndex(pktNdx));
		if (!pAncData)
			return AJA_STATUS_NULL;	//	Fail

		AJAAncillaryData &	pkt	(*pAncData);
		if (pkt.GetDataCoding() != AJAAncillaryDataCoding_Digital)
		{
			LOGMYDEBUG("Skipped Pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " -- Analog/Raw");
			continue;	//	Skip analog/raw packets
		}

		if (inIsProgressive  ||  pkt.GetLocationLineNumber() < inF2StartLine)
		{	/////////////	FIELD 1   //////////////
			if (actF1PktCnt >= MAX_ANC_PKTS_PER_RTP_PKT)
			{
				countOverflows++;
				LOGMYDEBUG("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << ", F1 RTP pkt count overflow: " << pkt.AsString(16));
				continue;
			}
			oldPktLengthWords = F1U32s.size();
			result = pkt.GenerateTransmitData(F1U32s);
			if (AJA_FAILURE(result))
				break;	//	Bail!
			if (F1U32s.size() > MAX_RTP_PKT_LENGTH_WORDS)
			{
				overflowWords += F1U32s.size() - oldPktLengthWords;
				LOGMYDEBUG("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << ": F1 RTP pkt length overflow: " << pkt.AsString(16));
				while (F1U32s.size() > oldPktLengthWords)
					F1U32s.pop_back();
				continue;
			}
			actF1PktCnt++;
			if (AllowMultiRTPTransmit())
			{	//	MULTI RTP PKTS
				outF1U32Pkts.push_back(F1U32s);	//	Append it
				outF1AncCounts.push_back(1);	//	One SMPTE Anc packet per RTP packet
				XMTDBG("F1 pkt " << DEC(actF1PktCnt) << ": " << ::ULWordSequenceToStringBE(F1U32s) << " (BigEndian)");
				F1U32s.clear();					//	Start current pkt over
			}
		}	/////////////	FIELD 1   //////////////
		else
		{	/////////////	FIELD 2   //////////////
			if (actF2PktCnt >= MAX_ANC_PKTS_PER_RTP_PKT)
			{
				countOverflows++;
				LOGMYDEBUG("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << ": F2 RTP pkt count overflow: " << pkt.AsString(16));
				continue;
			}
			oldPktLengthWords = F2U32s.size();
			result = pkt.GenerateTransmitData(F2U32s);
			if (AJA_FAILURE(result))
				break;	//	Bail!
			if (AJA_SUCCESS(result) && F2U32s.size() > MAX_RTP_PKT_LENGTH_WORDS)
			{
				overflowWords += F2U32s.size() - oldPktLengthWords;
				LOGMYDEBUG("Skipped pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << ": F2 RTP pkt length overflow: " << pkt.AsString(16));
				while (F2U32s.size() > oldPktLengthWords)
					F2U32s.pop_back();
				continue;
			}
			actF2PktCnt++;
			if (AllowMultiRTPTransmit())
			{	//	MULTI RTP PKTS
				outF2U32Pkts.push_back(F2U32s);	//	Append it
				outF2AncCounts.push_back(1);	//	One SMPTE Anc packet per RTP packet
				XMTDBG("F2 pkt " << DEC(actF2PktCnt) << ": " << ::ULWordSequenceToStringBE(F2U32s) << " (BigEndian)");
				F2U32s.clear();					//	Start current pkt over
			}
		}	/////////////	FIELD 2   //////////////
	}	//	for each SMPTE Anc packet

	if (AJA_FAILURE(result))
		LOGMYERROR(::AJAStatusToString(result) << ": Pkt " << DEC(pktNdx+1) << " of " << DEC(CountAncillaryData()) << " failed in GenerateTransmitData: " << ::AJAStatusToString(result));
	else if (!AllowMultiRTPTransmit())
	{	//	SINGLE RTP PKT
		outF1U32Pkts.push_back(F1U32s);					//	Append all F1
		outF1AncCounts.push_back(uint8_t(actF1PktCnt));	//	Total F1 SMPTE Anc packets in this one F1 RTP packet
		outF2U32Pkts.push_back(F2U32s);					//	Append all F2
		outF2AncCounts.push_back(uint8_t(actF2PktCnt));	//	Total F2 SMPTE Anc packets in this one F2 RTP packet
	}
	if (overflowWords && countOverflows)
		LOGMYWARN("Overflow: " << DEC(countOverflows) << " pkts skipped, " << DEC(overflowWords) << " U32s dropped");
	else if (overflowWords)
		LOGMYWARN("Data overflow: " << DEC(overflowWords) << " U32s dropped");
	else if (countOverflows)
		LOGMYWARN("Packet overflow: " << DEC(countOverflows) << " pkts skipped");
	XMTDBG("F1 (Content Only): " << outF1U32Pkts);
	XMTDBG("F2 (Content Only): " << outF2U32Pkts);
	return result;
}	//	GetRTPPackets



///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////	T R A N S M I T
///////////////////////////////////////////////////////////////////


AJAStatus AJAAncillaryList::GetAncillaryDataTransmitSize (const bool bProgressive, const uint32_t f2StartLine, uint32_t & ancSizeF1, uint32_t & ancSizeF2)
{
	AJAStatus	status	(AJA_STATUS_SUCCESS);
	uint32_t	f1Size	(0);
	uint32_t	f2Size	(0);

	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		AJAAncillaryData *	pAncData	(*it);
		uint32_t			packetSize	(0);
		status = pAncData->GetRawPacketSize (packetSize);
		if (status != AJA_STATUS_SUCCESS)
			break;

		if (bProgressive || pAncData->GetLocationLineNumber () < f2StartLine)
			f1Size += packetSize;
		else
			f2Size += packetSize;
	}

	ancSizeF1 = f1Size;
	ancSizeF2 = f2Size;
	return status;
}


AJAStatus AJAAncillaryList::GetAncillaryDataTransmitData (const bool bProgressive, const uint32_t f2StartLine, uint8_t * pF1AncData, const uint32_t inMaxF1Data, uint8_t * pF2AncData, const uint32_t inMaxF2Data)
{
	NTV2_POINTER	F1Buffer(pF1AncData, inMaxF1Data), F2Buffer(pF2AncData, inMaxF2Data);
	if (!F1Buffer.IsNULL())
		NTV2_ASSERT(F1Buffer.IsProvidedByClient());
	if (!F2Buffer.IsNULL())
		NTV2_ASSERT(F2Buffer.IsProvidedByClient());
	return GetTransmitData(F1Buffer, F2Buffer, bProgressive, f2StartLine);
}


AJAStatus AJAAncillaryList::GetTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
												const bool inIsProgressive, const uint32_t inF2StartLine)
{
	AJAStatus	status		(AJA_STATUS_SUCCESS);
	size_t		maxF1Data	(F1Buffer.GetByteCount());
	size_t		maxF2Data	(F2Buffer.GetByteCount());
	uint8_t *	pF1AncData	(reinterpret_cast<uint8_t*>(F1Buffer.GetHostPointer()));
	uint8_t *	pF2AncData	(reinterpret_cast<uint8_t*>(F2Buffer.GetHostPointer()));

	F1Buffer.Fill(uint64_t(0));	 F2Buffer.Fill(uint64_t(0));

	//	I need to be in ascending line order...
	SortListByLocation();

	//	Generate transmit data for each of my packets...
	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  ++it)
	{
		uint32_t			pktSize(0);
		AJAAncillaryData *	pPkt(*it);
		if (!pPkt)
			{status = AJA_STATUS_NULL;	break;}	//	Fail

		if (inIsProgressive || pPkt->GetLocationLineNumber() < inF2StartLine)
		{
			if (pF1AncData  &&  maxF1Data)
			{
				status = pPkt->GenerateTransmitData(pF1AncData, maxF1Data, pktSize);
				if (AJA_FAILURE(status))
					break;

				pF1AncData += pktSize;
				maxF1Data  -= pktSize;
			}
		}
		else if (pF2AncData  &&  maxF2Data)
		{
			status = pPkt->GenerateTransmitData(pF2AncData, maxF2Data, pktSize);
			if (AJA_FAILURE(status))
				break;

			pF2AncData += pktSize;
			maxF2Data  -= pktSize;
		}
	}	//	for each of my anc packets
	return status;
}


AJAStatus AJAAncillaryList::GetVANCTransmitData (NTV2_POINTER & inFrameBuffer,  const NTV2FormatDescriptor & inFormatDesc)
{
	if (inFrameBuffer.IsNULL())
	{
		LOGMYERROR("AJA_STATUS_NULL: null frame buffer");
		return AJA_STATUS_NULL;
	}
	if (!inFormatDesc.IsValid())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: Invalid format descriptor");
		return AJA_STATUS_BAD_PARAM;
	}
	if (!inFormatDesc.IsVANC())
	{
		LOGMYERROR("AJA_STATUS_BAD_PARAM: Not a VANC geometry");
		return AJA_STATUS_BAD_PARAM;
	}
	if (inFormatDesc.GetPixelFormat() != NTV2_FBF_10BIT_YCBCR  &&  inFormatDesc.GetPixelFormat() != NTV2_FBF_8BIT_YCBCR)
	{
		LOGMYERROR("AJA_STATUS_UNSUPPORTED: unsupported pixel format: " << inFormatDesc);
		return AJA_STATUS_UNSUPPORTED;
	}
	if (!CountAncillaryData())
	{
		LOGMYWARN("List is empty");
		return AJA_STATUS_SUCCESS;
	}

	//	I need to be in ascending line order...
	SortListByLocation();

	//	BRUTE-FORCE METHOD -- NOT VERY EFFICIENT
	const bool	isSD	(inFormatDesc.IsSDFormat());
	AJAAncillaryList	failures, successes;
	set <uint16_t>		lineOffsetsWritten;

	//	For each VANC line...
	for (UWord fbLineOffset(0);  fbLineOffset < inFormatDesc.GetFirstActiveLine();  fbLineOffset++)
	{
		ULWord smpteLine (0);	bool isF2 (false);
		inFormatDesc.GetSMPTELineNumber (fbLineOffset, smpteLine, isF2);

		//	Look for non-HANC packets destined for this line...
		for (AJAAncDataListConstIter iter(m_ancList.begin());   (iter != m_ancList.end()) && *iter;   ++iter)
		{
			bool								muxedOK	(false);
			AJAAncillaryData &					ancData (**iter);
			const AJAAncillaryDataLocation &	loc		(ancData.GetDataLocation());
			UWordSequence						u16PktComponents;
			if (ancData.GetDataCoding() != AJAAncillaryDataCoding_Digital)	//	Ignore "Raw" or "Analog" or "Unknown" packets
				continue;
			if (loc.GetDataSpace() != AJAAncillaryDataSpace_VANC)			//	Ignore "HANC" or "Unknown" packets
			{
				//LOGMYWARN("Skipping non-VANC packet " << ancData);
				continue;
			}
			if (loc.GetLineNumber() != smpteLine)							//	Wrong line number
			{
				//LOGMYWARN("Skipping packet not destined for line " << DEC(smpteLine) << " " << loc);
				continue;
			}
			if (!IS_VALID_AJAAncillaryDataChannel(loc.GetDataChannel()))	//	Bad data channel
			{
				LOGMYWARN("Skipped packet with invalid data channel " << loc);
				continue;
			}

			//	For v210 buffers, generate 10-bit packet data and put into u16 vector...
			muxedOK = AJA_SUCCESS(ancData.GenerateTransmitData(u16PktComponents));
			if (muxedOK)
			{
				if (inFormatDesc.GetPixelFormat() == NTV2_FBF_8BIT_YCBCR)	//	2vuy buffers are simple -- just copy the data
				{
					uint8_t *	pLine	(reinterpret_cast<uint8_t*>(inFormatDesc.GetWriteableRowAddress(inFrameBuffer.GetHostPointer(), fbLineOffset)));
					if (isSD)
					{	//	SD overwrites both Y & C channels in the frame buffer:
						for (unsigned ndx(0);  ndx < u16PktComponents.size();  ndx++)
							pLine[ndx] = uint8_t(u16PktComponents[ndx] & 0xFF);
					}
					else
					{	//	HD overwrites only the Y or C channel data in the frame buffer:
						unsigned	dstNdx(loc.IsLumaChannel() ? 1 : 0);
						for (unsigned srcNdx(0);  srcNdx < u16PktComponents.size();  srcNdx++)
							pLine[dstNdx + 2*srcNdx] = uint8_t(u16PktComponents[srcNdx] & 0x00FF);
					}
				}	//	if 2vuy
				else	/////////////	v210 FBF:
				{
					if (isSD)
					{	//	For SD, just pack the u16 components into the buffer...
						while (u16PktComponents.size() < 12  ||  u16PktComponents.size() % 12)	//	YUVComponentsTo10BitYUVPackedBuffer fails if packing fewer than 12 words
							u16PktComponents.push_back(0x040);	//	SMPTE black
						muxedOK = ::YUVComponentsTo10BitYUVPackedBuffer (u16PktComponents, inFrameBuffer, inFormatDesc, fbLineOffset);
					}
					else
					{	//	HD overwrites only the Y or C channel data:
						UWordSequence	YUV16Line;
						unsigned		dstNdx	(loc.IsLumaChannel() ? 1 : 0);

						//	Read original Y+C components from FB...
						muxedOK = UnpackLine_10BitYUVtoU16s (YUV16Line, inFrameBuffer, inFormatDesc, fbLineOffset);
						//cerr << "WriteVANCData|orig: +" << DEC0N(fbLineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, fbLineOffset);  cerr << ":" << endl << YUV16Line << endl;

						//	Patch the Y or C channel...
						if (muxedOK)
							for (unsigned srcNdx(0);  srcNdx < u16PktComponents.size();  srcNdx++)
								YUV16Line[dstNdx + 2*srcNdx] = u16PktComponents[srcNdx] & 0x03FF;
						//cerr << "WriteVANCData|modi: +" << DEC0N(fbLineOffset,2) << ": ";  inFormatDesc.PrintSMPTELineNumber(cerr, fbLineOffset);  cerr << ":" << endl << YUV16Line << endl;

						//	Repack the patched YUV16 line back into the FB...
						if (muxedOK)
						{
							while (YUV16Line.size() < 12  ||  YUV16Line.size() % 12)	//	YUVComponentsTo10BitYUVPackedBuffer fails if packing fewer than 12 words
								YUV16Line.push_back(0x040);	//	SMPTE black
							muxedOK = ::YUVComponentsTo10BitYUVPackedBuffer (YUV16Line, inFrameBuffer, inFormatDesc, fbLineOffset);
						}
					}	//	else HD
				}	//	else v210
			}	//	if GenerateTransmitData OK

			//	TBD:	NEED TO TAKE INTO CONSIDERATION	loc.GetHorizontalOffset()

			if (muxedOK)
			{
				lineOffsetsWritten.insert(uint16_t(fbLineOffset));	//	Remember which FB line offsets we overwrote
				successes.AddAncillaryData(ancData);
			}
			else
				failures.AddAncillaryData(ancData);
		}	//	for each packet
	}	//	for each VANC line

	//	Any analog packets?
	for (AJAAncDataListConstIter iter(m_ancList.begin());   (iter != m_ancList.end()) && *iter;   ++iter)
	{
		bool								success		(inFormatDesc.IsSDFormat());
		AJAAncillaryData &					ancData		(**iter);
		const AJAAncillaryDataLocation &	loc			(ancData.GetDataLocation());
		ULWord								lineOffset	(0);
		if (!ancData.IsRaw())
			continue;	//	Ignore "Digital" or "Unknown" packets

		if (success)
			success = inFormatDesc.GetLineOffsetFromSMPTELine (loc.GetLineNumber(), lineOffset);
		if (success  &&  lineOffsetsWritten.find(uint16_t(lineOffset)) != lineOffsetsWritten.end())
			success = false;	//	Line already written -- "analog" data overwrites entire line
		if (success)
		{
			if (inFormatDesc.GetPixelFormat() == NTV2_FBF_8BIT_YCBCR)
				::memcpy(inFormatDesc.GetWriteableRowAddress(inFrameBuffer.GetHostPointer(), lineOffset), ancData.GetPayloadData(), ancData.GetDC());	//	'2vuy' -- straight copy
			else
			{
				UWordSequence	pktComponentData;
				success = AJA_SUCCESS(ancData.GenerateTransmitData(pktComponentData));
				if (success)
					success = ::YUVComponentsTo10BitYUVPackedBuffer (pktComponentData, inFrameBuffer, inFormatDesc, UWord(lineOffset));
			}
		}

		if (success)
		{
			lineOffsetsWritten.insert(uint16_t(lineOffset));	//	Remember which FB line offsets we overwrote
			successes.AddAncillaryData(ancData);
		}
		else
			failures.AddAncillaryData(ancData);
	}	//	for each Analog packet

	if (failures.CountAncillaryData())
		LOGMYWARN("FAILURES: " << failures);
	if (successes.CountAncillaryData())
		LOGMYDEBUG("SUCCESSES: " << successes);

	//	At least one success is considered SUCCESS.
	//	Zero successes and one or more failures is considered FAIL...
	return successes.CountAncillaryData() == 0  &&  failures.CountAncillaryData() > 0  ?  AJA_STATUS_FAIL  :  AJA_STATUS_SUCCESS;
}	//	WriteVANCData


//	STATIC
AJAStatus AJAAncillaryList::WriteRTPPackets (NTV2_POINTER & theBuffer,  uint32_t & outBytesWritten,
											const AJAU32Pkts & inRTPPkts,  const AJAAncPktCounts & inAncCounts,
											const bool inIsF2,  const bool inIsProgressive)
{
	const ULWord	totPkts (ULWord(inRTPPkts.size()));
	const string	sFld	(inIsF2 ? " F2" : " F1");
	const string	sPrg	(inIsProgressive ? " Prg" : " Int");
	ULWord	u32offset(0), pktNum(1);

	outBytesWritten = 0;
	if (inRTPPkts.size() != inAncCounts.size())
		{LOGMYERROR(DEC(inRTPPkts.size()) << " RTP pkt(s) != " << DEC(inAncCounts.size()) << " anc count(s)");  return AJA_STATUS_BAD_PARAM;}

	//	For each RTP Packet...
	for (AJAU32PktsConstIter RTPPktIter(inRTPPkts.begin());  RTPPktIter != inRTPPkts.end();  pktNum++)
	{
		AJARTPAncPayloadHeader	RTPHeader;		//	The RTP packet header to be built
		ULWordSequence			RTPHeaderU32s;	//	The RTP packet header expressed as a sequence of 5 x U32s
		const ULWordSequence &	origRTPPkt(*RTPPktIter);	//	The original RTP packet data contents, a sequence of U32s
		const bool				isLastRTPPkt	(++RTPPktIter == inRTPPkts.end());	//	Last RTP packet?
		const size_t			totalRTPPktBytes(AJARTPAncPayloadHeader::GetHeaderByteCount()	//	RTP packet size,
												  +  origRTPPkt.size() * sizeof(uint32_t));		//	including header, in bytes
		ostringstream			pktNumInfo;
		pktNumInfo << " for RTP pkt " << DEC(pktNum) << " of " << DEC(totPkts);

		//	Set the RTP Packet Header's info...
		if (inIsProgressive)
			RTPHeader.SetProgressive();
		else if (inIsF2)
			RTPHeader.SetField2();
		else
			RTPHeader.SetField1();
		RTPHeader.SetEndOfFieldOrFrame(isLastRTPPkt);
		RTPHeader.SetAncPacketCount(uint8_t(inAncCounts.at(pktNum-1)));	//	Use provided Anc count
		RTPHeader.SetPayloadLength(uint16_t(origRTPPkt.size() * sizeof(uint32_t)));
		//	Playout:  Firmware looks for full RTP pkt bytecount in LS 16 bits of SequenceNumber in RTP header:
		RTPHeader.SetSequenceNumber(uint32_t(totalRTPPktBytes) & 0x0000FFFF);

		//	Convert RTP header object into 5 x U32s...
		RTPHeader.WriteToULWordVector(RTPHeaderU32s, true);
		NTV2_ASSERT(RTPHeaderU32s.size() == AJARTPAncPayloadHeader::GetHeaderWordCount());

		//	Write RTP header into theBuffer...
		if (theBuffer)
		{
			if (!theBuffer.PutU32s(RTPHeaderU32s, u32offset))
				{LOGMYERROR("RTP hdr WriteBuffer failed for buffer " << theBuffer << " at u32offset=" << DEC(u32offset)
							<< pktNumInfo.str());  return AJA_STATUS_FAIL;}
		}
		u32offset += ULWord(RTPHeaderU32s.size());	//	Move "write head" to just past end of RTP header

		//	Write RTP packet contents into theBuffer...
		if (theBuffer)
		{
			if (!theBuffer.PutU32s(origRTPPkt, u32offset))
				{LOGMYERROR("PutU32s failed writing " << DEC(origRTPPkt.size()) << " U32s in buffer " << theBuffer << " at u32offset=" << DEC(u32offset)
							<< pktNumInfo.str());  return AJA_STATUS_FAIL;}
			LOGMYDEBUG("PutU32s OK @u32offset=" << xHEX0N(u32offset,4) << ": " << RTPHeader << pktNumInfo.str());
		}

		//	Move "write head" to just past where this RTP packet's data ended...
		u32offset += ULWord(origRTPPkt.size());

		//	JeffL:  IP Anc inserters expect subsequent RTP packets to start on a 64-bit/8-byte word boundary.
		if (u32offset & 1L)	//	If it's not even...
			u32offset++;	//	...then make it even!
	}	//	for each RTP packet

	outBytesWritten = u32offset * ULWord(sizeof(uint32_t));
	if (theBuffer)
		LOGMYDEBUG(DEC(totPkts) << " RTP pkt(s), " << DEC(u32offset) << " U32s (" << DEC(outBytesWritten)
					<< " bytes) written for" << sFld << sPrg);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::GetIPTransmitData (NTV2_POINTER & F1Buffer, NTV2_POINTER & F2Buffer,
												const bool inIsProgressive, const uint32_t inF2StartLine)
{
	AJAStatus		result (AJA_STATUS_SUCCESS);
	AJAU32Pkts		F1U32Pkts, F2U32Pkts;		//	32-bit network-byte-order data
	AJAAncPktCounts	F1AncCounts, F2AncCounts;	//	Per-RTP packet anc packet counts
	uint32_t		byteCount(0);				//	Not used

	//	I need to be in ascending line order...
	F1Buffer.Fill(uint64_t(0));	 F2Buffer.Fill(uint64_t(0));
	SortListByLocation();

	//	Get F1 & F2 xmit RTP U32 words (and SMPTE anc packet counts for each RTP packet)...
	result = GetRTPPackets (F1U32Pkts, F2U32Pkts, F1AncCounts, F2AncCounts, inIsProgressive, inF2StartLine);
	if (AJA_FAILURE(result))
		return result;

	/*
	ostringstream oss;  oss << "Anc Counts: F1=";  for (size_t ndx(0);  ndx < F1AncCounts.size();  ndx++)  oss << (ndx?"|":"[") << DEC(uint16_t(F1AncCounts.at(ndx)));
	oss << "] F2=";  for (size_t ndx(0);  ndx < F2AncCounts.size();  ndx++)  oss << (ndx?"|":"[") << DEC(uint16_t(F2AncCounts.at(ndx)));  oss << "]";
	XMTDBG(oss.str());
	*/

	//	Write the F1 buffer...
	result = WriteRTPPackets (F1Buffer, byteCount, F1U32Pkts, F1AncCounts, /*isF2*/false, inIsProgressive);
	if (AJA_FAILURE(result))
		return result;

	//	Write the F2 buffer...
	if (!inIsProgressive)
		result = WriteRTPPackets (F2Buffer, byteCount, F2U32Pkts, F2AncCounts, /*isF2*/true, inIsProgressive);

	return result;

}	//	GetIPTransmitData


AJAStatus AJAAncillaryList::GetIPTransmitDataLength (uint32_t & outF1ByteCount, uint32_t & outF2ByteCount,
													const bool inIsProgressive, const uint32_t inF2StartLine)
{
	outF1ByteCount = outF2ByteCount = 0;

	//	Get F1 & F2 xmit RTP U32 words...
	AJAU32Pkts		F1U32Pkts, F2U32Pkts;	//	U32 network-byte-order data
	AJAAncPktCounts	F1AncCounts, F2AncCounts;	//	Per-RTP packet anc packet counts
	AJAStatus result (GetRTPPackets (F1U32Pkts, F2U32Pkts,  F1AncCounts, F2AncCounts,  inIsProgressive,  inF2StartLine));
	if (AJA_FAILURE(result))
		return result;

	NTV2_POINTER	nullBuffer;	//	An empty buffer tells WriteRTPPackets to just calculate byteCount...
	result = WriteRTPPackets (nullBuffer, outF1ByteCount, F1U32Pkts, F1AncCounts, /*isF2*/false, inIsProgressive);
	if (AJA_SUCCESS(result)  &&  !inIsProgressive)
		result = WriteRTPPackets (nullBuffer, outF2ByteCount, F2U32Pkts, F2AncCounts, /*isF2*/true, inIsProgressive);

	return result;
}


ostream & AJAAncillaryList::Print (ostream & inOutStream, const bool inDumpPayload) const
{
	unsigned	num	(0);
	inOutStream << DEC(CountAncillaryData()) << " pkts:" << endl;
	for (AJAAncDataListConstIter it(m_ancList.begin());  it != m_ancList.end();  )
	{
		AJAAncillaryData *	pPkt(*it);
		inOutStream << "Pkt" << DEC0N(++num,3) << ": " << pPkt->AsString(inDumpPayload ? 16 : 0);
		if (++it != m_ancList.end())
			inOutStream << endl;
	}
	return inOutStream;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////	A N A L O G   A N C   A S S O C I A T I O N S
//////////////////////////////////////////////////////////////////////////////////////////////////

static AJAAncillaryAnalogTypeMap	gAnalogTypeMap;
static AJALock						gAnalogTypeMapMutex;


AJAStatus AJAAncillaryList::ClearAnalogAncillaryDataTypeMap (void)
{
	AJAAutoLock	locker(&gAnalogTypeMapMutex);
	gAnalogTypeMap.clear();
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SetAnalogAncillaryDataTypeMap (const AJAAncillaryAnalogTypeMap & inMap)
{
	AJAAutoLock	locker(&gAnalogTypeMapMutex);
	gAnalogTypeMap = inMap;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::GetAnalogAncillaryDataTypeMap (AJAAncillaryAnalogTypeMap & outMap)
{
	AJAAutoLock	locker(&gAnalogTypeMapMutex);
	outMap = gAnalogTypeMap;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJAAncillaryList::SetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum, const AJAAncillaryDataType inAncType)
{
	AJAAutoLock	locker(&gAnalogTypeMapMutex);
	gAnalogTypeMap.erase(inLineNum);				//	In case someone has already set this line
	if (inAncType == AJAAncillaryDataType_Unknown)
		return AJA_STATUS_SUCCESS;					//	A non-entry is the same as AJAAncillaryDataType_Unknown
	else if (IS_VALID_AJAAncillaryDataType(inAncType))
			gAnalogTypeMap[inLineNum] = inAncType;
	else
		return AJA_STATUS_BAD_PARAM;
	return AJA_STATUS_SUCCESS;
}


AJAAncillaryDataType AJAAncillaryList::GetAnalogAncillaryDataTypeForLine (const uint16_t inLineNum)
{
	AJAAutoLock	locker(&gAnalogTypeMapMutex);
	AJAAncillaryDataType	ancType	(AJAAncillaryDataType_Unknown);
	if (!gAnalogTypeMap.empty ())
	{
		AJAAncillaryAnalogTypeMap::const_iterator	it	(gAnalogTypeMap.find (inLineNum));
		if (it != gAnalogTypeMap.end ())
			ancType = it->second;
	}
	return ancType;
}
