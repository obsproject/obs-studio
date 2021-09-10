/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubpktcom.cpp
	@brief		Implementation of shared NTV2 "nub" packet handling functions.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(AJALinux ) || defined(AJAMac)
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#endif

#ifdef MSWindows
#include <WinSock2.h>
typedef int socklen_t;
#endif

#include "ajatypes.h"
#include "ntv2nubtypes.h"
#include "ntv2nubpktcom.h"

#ifdef AJALinux
#include <endian.h>
#endif

// Silly original protocol strings.  Replaced with smaller ones in V2.
// These MUST be in the same order as the members of NTV2NubPktType
const char * NTV2NubQueryRespStrProtVer1[eNumNTV2NubPktTypes] =
{
	"Our chief weapons are?",
	"Fear and surprise!",

	"I didn't expect a kind of Spanish Inquisition.",
	"NOBODY expects the Spanish Inquisition!",

	"Cardinal Fang! Fetch...THE COMFY CHAIR!",
	"The...Comfy Chair?",

	"Biggles! Put her in the Comfy Chair!",
	"Is that really all it is?",

	"Right! How do you plead?",
	"Innocent.",

	"My old man said follow the...",
	"That's enough.",

	"Oh no - what kind of trouble?",
	"One on't cross beams gone owt askew on treddle.",

	"You have three last chances, the nature of which I have divulged in my previous utterance.",
	"I don't know what you're talking about.",

	"Surprise and...",
	"Ah!... our chief weapons are surprise... blah blah blah.",

	"Shall I...?",
	"No, just pretend for God's sake. Ha! Ha! Ha!",

	"Biggles! Fetch...THE SOFT CUSHIONS!",
	"Here they are, lord."
};

const char *NTV2NubQueryRespStrProtVer2[eNumNTV2NubPktTypes] =
{
	// The discover strings must be the same for all protocol versions
	// so that different versions of watchers and nubs can find each other.
	"Our chief weapons are?",
	"Fear and surprise!",

	// The opens must be the same for all protocol version since
	// the open packet does the protocol version negotiation.
	"I didn't expect a kind of Spanish Inquisition.",
	"NOBODY expects the Spanish Inquisition!",

	"Read1Q",
	"Read1R",

	"Write1Q",
	"Write1R",

	"GetACQ",
	"GetACR",

	"WaitIntQ",
	"WaitIntR",

	"GetBFIQ",
	"GetBFIR",

	"DLTPQ",
	"DLTPR",

	"ReadMQ",
	"ReadMR",

	"GetDVQ",
	"GetDVR",

	"ACCtrlQ",
	"ACCtrlR"
};

static const char * NTV2NubQueryRespStrProtVer3 [eNumNTV2NubPktTypes] =
{
	// The discover strings must be the same for all protocol versions
	// so that different versions of watchers and nubs can find each other.
	"Our chief weapons are?",
	"Fear and surprise!",

	// The opens must be the same for all protocol version since
	// the open packet does the protocol version negotiation.
	"I didn't expect a kind of Spanish Inquisition.",
	"NOBODY expects the Spanish Inquisition!",

	"Read1Q",
	"Read1R",

	"Write1Q",
	"Write1R",

	"GetACQ",
	"GetACR",

	"WaitIntQ",
	"WaitIntR",

	"GetBFIQ",
	"GetBFIR",

	"DLTPQ",
	"DLTPR",

	"ReadMQ",
	"ReadMR",

	"GetDVQ",
	"GetDVR",

	"ACCtrlQ",
	"ACCtrlR",

	"GetBIQ",
	"GetBIR"
};

#if defined (AJALinux) 
	unsigned long long ntohll(unsigned long long n) 
	{
	#if __BYTE_ORDER == __BIG_ENDIAN
		return n;
	#else
		return (((unsigned long long)ntohl(n)) << 32) + ntohl(n >> 32);
	#endif
	}
#endif

#if defined (AJALinux) 
	unsigned long long htonll(unsigned long long n) 
	{
	#if __BYTE_ORDER == __BIG_ENDIAN
		return n;
	#else
		return (((unsigned long long)htonl(n)) << 32) + htonl(n >> 32);
	#endif
	}
#endif


const char * nubQueryRespStr (NTV2NubProtocolVersion	protocolVersion,  NTV2NubPktType pktType)
{
	const char *queryRespStr = AJA_NULL;
	switch (protocolVersion)
	{
		case ntv2NubProtocolVersion1:
			queryRespStr = NTV2NubQueryRespStrProtVer1[pktType];
			break;

		case ntv2NubProtocolVersion2:
			queryRespStr = NTV2NubQueryRespStrProtVer2[pktType];
			break;

		case ntv2NubProtocolVersion3:
		default:// No response table available, use newest one.
				// This can happen in the case of an open when 
				// the protcol version is being negotiated.
			queryRespStr = NTV2NubQueryRespStrProtVer3[pktType];
			break;
	}
	return queryRespStr;
}


void * GetNubPktPayloadPtr (NTV2NubPkt * pPkt)
{
	const char *queryRespStr(nubQueryRespStr(pPkt->hdr.protocolVersion, pPkt->hdr.pktType));
	char *p (reinterpret_cast<char*>(pPkt->data));
	ULWord len (ULWord(::strlen(queryRespStr)) + 1);
	p += len;
	return p;
}


// Build a nub packet and fill in all the common stuff for the given protocol and type.
NTV2NubPkt * BuildNubBasePacket (NTV2NubProtocolVersion	protocolVersion,
								NTV2NubPktType			pktType,
								ULWord					payloadSize,
								char **					pPayload)
{
	NTV2NubPktType adjustedPktType = pktType;
	const char *queryRespStr (nubQueryRespStr(protocolVersion, pktType));

	if (protocolVersion == ntv2NubProtocolVersion1)
	{
		// eNubV1ControlAutoCirculateQueryPkt is a dupe number in v1
		// eNubV1ControlAutoCirculateRespPkt is a dupe number in v1
		// Map accordingly.
		if (pktType == eNubV2ControlAutoCirculateQueryPkt)
			adjustedPktType = eNubV1ControlAutoCirculateQueryPkt;
		else if (pktType == eNubV2ControlAutoCirculateRespPkt)
			adjustedPktType = eNubV1ControlAutoCirculateRespPkt;
	}
	
	// Start with length of response string, add payload
	ULWord dataSize (ULWord(::strlen(queryRespStr)) + 1 + payloadSize);
	if (dataSize > NTV2_NUBPKT_MAX_DATASIZE)
		return AJA_NULL;

	NTV2NubPkt *pPkt(new NTV2NubPkt);
	if (!pPkt)
		return AJA_NULL;
	::memset(pPkt, 0, sizeof(NTV2NubPkt));

	pPkt->hdr.protocolVersion	= protocolVersion; 
	pPkt->hdr.pktType			= adjustedPktType;
	pPkt->hdr.dataLength		= dataSize;

	char *p (reinterpret_cast<char*>(pPkt->data));	// Work around ISO C++ forbids zero-size array
	ULWord len (ULWord(::strlen(queryRespStr)) + 1);
	::strncpy(p, queryRespStr, len); 	// Copy in query/resp string incl. terminating null
	p += len;
	*pPayload = p;
	return pPkt;
}


// Put packet header in network byte order
bool NBOifyNTV2NubPkt (NTV2NubPkt * pPkt)
{
	if (!pPkt)
		return false;
	pPkt->hdr.protocolVersion	= htonl(pPkt->hdr.protocolVersion);
	pPkt->hdr.pktType			= NTV2NubPktType(htonl(pPkt->hdr.pktType));
	pPkt->hdr.dataLength		= htonl(pPkt->hdr.dataLength);
	return true;
}


// Put packet header in host byte order
bool deNBOifyNTV2NubPkt (NTV2NubPkt * pPkt, ULWord size)
{
	if (!pPkt || size < sizeof(NTV2NubPktHeader)) 
		return false;
	pPkt->hdr.protocolVersion	= ntohl(pPkt->hdr.protocolVersion);
	pPkt->hdr.pktType			= NTV2NubPktType(ntohl(pPkt->hdr.pktType));
	pPkt->hdr.dataLength		= ntohl(pPkt->hdr.dataLength);
	return true;
}


int sendall (AJASocket s, char * buf, int * len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n = -1;

    while (total < *len)
    {
        n = int(::send(s, buf+total, size_t(bytesleft), 0));
        if (n == -1) break;
        total += n;
        bytesleft -= n;
    }
	*len = total; // return number actually sent here
    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
} 


int recvtimeout_sec (AJASocket s, char * buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(int(s+1), &fds, AJA_NULL, AJA_NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return int(::recv(s, buf, size_t(len), 0));
}


int recvtimeout_usec (AJASocket s, char * buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = 0;
    tv.tv_usec = timeout;

    // wait until timeout or data received
    n = select(int(s+1), &fds, AJA_NULL, AJA_NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return int(::recv(s, buf, size_t(len), 0));
}


int recvfromtimeout (AJASocket s, char *buf, int len, int timeout,
					struct sockaddr *their_addr, socklen_t *addr_len)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(int(s+1), &fds, AJA_NULL, AJA_NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recvfrom()
	return int(::recvfrom(s, buf, size_t(len), 0, their_addr, addr_len));
}


// Debug functions

void dumpBoardInventory (NTV2DiscoverRespPayload * boardInventory)
{
	printf("numBoards: %d\n", boardInventory->numBoards);
	// Dump devices
	for (ULWord boardNum = 0; boardNum < boardInventory->numBoards; boardNum++)
	{
		NTV2DiscoverBoardInfo *dbi = &boardInventory->discoverBoardInfo[boardNum];
		printf("Board[%d]: boardNumber = %d, boardType = %d, boardID = 0x%08X", 
				boardNum, 
				dbi->boardNumber,
				dbi->boardType,
				dbi->boardID);
		printf("\tDesc: [%s]\n", dbi->description);
	}
}


void dumpDiscoveryPacket (NTV2NubPkt * pPkt, NTV2DiscoverRespPayload * boardInventory)
{
	printf("Discovery Packet Dump\n");
	printf("Protocol Version = %d\n", pPkt->hdr.protocolVersion);
	printf("pktType = %d\n", pPkt->hdr.pktType);
	printf("dataLength = %d\n", pPkt->hdr.dataLength);
	printf("Payload:\n");

	// Print response string
	char *p (reinterpret_cast<char*>(pPkt->data));
	printf("[%s]\n", p);
	if (pPkt->hdr.pktType == eDiscoverRespPkt && boardInventory)
		dumpBoardInventory(boardInventory);
}


bool isNTV2NubPacketType (NTV2NubPkt * pPkt, NTV2NubPktType nubPktType)
{
	NTV2NubPktType fixedPktType;
	switch (pPkt->hdr.protocolVersion)
	{
		case ntv2NubProtocolVersion1:
			// eNubV1ControlAutoCirculateQueryPkt is a dupe number in v1
			// eNubV1ControlAutoCirculateRespPkt is a dupe number in v1
			// Map them to the right array index.
			if (nubPktType == eNubV2ControlAutoCirculateQueryPkt)
				fixedPktType = eNubV1ControlAutoCirculateQueryPkt;
			else if (nubPktType == eNubV2ControlAutoCirculateRespPkt)
				fixedPktType = eNubV1ControlAutoCirculateRespPkt;
			else
				fixedPktType = nubPktType;

			return fixedPktType == pPkt->hdr.pktType
					&&  !::strncmp(reinterpret_cast<const char*>(pPkt->data), NTV2NubQueryRespStrProtVer1[nubPktType], pPkt->hdr.dataLength);

		case ntv2NubProtocolVersion2:
			return pPkt->hdr.pktType == nubPktType
					&&	!::strncmp(reinterpret_cast<const char*>(pPkt->data), NTV2NubQueryRespStrProtVer2[nubPktType], pPkt->hdr.dataLength);

		case ntv2NubProtocolVersion3:
		default:	
			return pPkt->hdr.pktType == nubPktType
					&&	!::strncmp(reinterpret_cast<const char*>(pPkt->data), NTV2NubQueryRespStrProtVer3[nubPktType], pPkt->hdr.dataLength);
	}
}
