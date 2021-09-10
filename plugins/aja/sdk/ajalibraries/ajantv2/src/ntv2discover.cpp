/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2discover.cpp
	@brief		Implementation for NTV nub discovery functions.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <stdlib.h>
#ifdef MSWindows
    #pragma warning(disable:4996)
	#include "winsock2.h"
	#include "Ws2tcpip.h"
	#define snprintf _snprintf
	#define vsnprintf _vsnprintf
	#define close closesocket
#else
	#include <unistd.h>
	#include <errno.h>  	 
	#include <string.h> 	 
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
	#include <sys/ioctl.h>
	#include <net/if.h>
#endif
#include "ajatypes.h"
#include "ntv2discover.h"
#include "ntv2nubpktcom.h"
#include "ntv2utf8.h"

// max number of bytes we can get at once 
#define MAXDATASIZE (sizeof(NTV2NubPktHeader) + NTV2_NUBPKT_MAX_DATASIZE)

#if defined (NTV2_NUB_CLIENT_SUPPORT)

static bool 
isDiscoverRespPacket(NTV2NubPkt *pPkt)
{
	return isNTV2NubPacketType(pPkt, eDiscoverRespPkt);
}

static NTV2NubPkt *
BuildDiscoverQueryPacket(void)
{
	NTV2NubPkt *pPkt;
	char *p;
	
	pPkt = BuildNubBasePacket(	ntv2NubProtocolVersion2,
								eDiscoverQueryPkt,
								sizeof(NTV2DiscoverQueryPayload),
								&p);
	if (pPkt == 0)
		return 0;

	NTV2DiscoverQueryPayload *pDiscoverQueryPayload = 
		(NTV2DiscoverQueryPayload *)p;

	pDiscoverQueryPayload->boardMask = htonl(ULWord(256));
	
	return pPkt;
}

static void
extractBoardInventory(NTV2NubPkt *pPkt, NTV2DiscoverRespPayload *boardInventory, const char *peername)
{
	memcpy(boardInventory, GetNubPktPayloadPtr(pPkt), sizeof(NTV2DiscoverRespPayload));

	// Convert table to host byte order
	//
	// Need numBoard member so we start with it.
	boardInventory->numBoards		= ntohl(boardInventory->numBoards);
	for (ULWord boardNum = 0; boardNum < boardInventory->numBoards; boardNum++)
	{
		NTV2DiscoverBoardInfo *dbi = &boardInventory->discoverBoardInfo[boardNum];

		dbi->boardNumber = ntohl(dbi->boardNumber);
		dbi->boardType = ntohl(dbi->boardType);
		dbi->boardID = ntohl(dbi->boardID);
		char tmp[NTV2_DISCOVER_BOARDINFO_DESC_STRMAX];
		// Prepend peername to description.
		strncpy(tmp, dbi->description, NTV2_DISCOVER_BOARDINFO_DESC_STRMAX);

		int peernameLen = snprintf(	dbi->description, 
									NTV2_DISCOVER_BOARDINFO_DESC_STRMAX, 
									"%s: ", peername);

		// Copy system name/description, breaking chars only on a UTF-8 boundary

		strncpyasutf8(	dbi->description + peernameLen, 
						tmp,
						NTV2_DISCOVER_BOARDINFO_DESC_STRMAX - peernameLen);

	}
}

// Discover nubs on all interfaces
int
ntv2DiscoverNubs(	int maxNubs, // Maximum number of nubs (size of sockaddr_in and boardInventory arrays)
					struct sockaddr_in their_addr[],			// Array of responders
					NTV2DiscoverRespPayload boardInventory[],	// Available boards for each responder
					int &nubsFound,
					int timeout,	// How long to wait for discover responses In seconds
					int sendto_count)
{
#ifdef MSWindows
	INTERFACE_INFO interfaceList[20]; //relates to ifconf below
	unsigned long nBytesReturned = 0;
#else
	char          buf[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
#endif
	int           sck;
	int           nInterfaces;
	int           i;

	nubsFound = 0;

	// Get a socket handle
	sck = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if(sck < 0)
	{
		perror("socket");
		return 1;
	}

	// Query available interfaces.
#if MSWindows
	if(WSAIoctl(sck, SIO_GET_INTERFACE_LIST, 0, 0, &interfaceList, sizeof(interfaceList), &nBytesReturned, 0, 0) < 0)
	{
		perror("WSAIoctl(SIO_GET_INTERFACE_LIST)");
		return 1;
	}
	nInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

	int retcode = DISCOVER_FOUND_NOBODY;

	for(i = 0; i < nInterfaces; i++)
	{

		// Skip interfaces with out a useful broadcast address
		sockaddr_in *pAddress = (sockaddr_in *) &(interfaceList[i].iiBroadcastAddress);
		if (strcmp("0.0.0.0", inet_ntoa(pAddress->sin_addr)))
		{
			retcode = ntv2DiscoverNubs( inet_ntoa(pAddress->sin_addr),
				maxNubs,
				their_addr,
				boardInventory,
				nubsFound,
				timeout,	
				sendto_count,
				true);

			if (retcode == DISCOVER_RECVERR || retcode == DISCOVER_FULL)
			{
				return retcode;
			}
		}
		else
		{
			// printf("%s has broadcast address of 0, skipping...\n", item->ifr_name);
		}
	}
#else
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl(SIOCGIFCONF)");
		return 1;
	}

	// Iterate through the list of interfaces.
	ifr         = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);

	int retcode = DISCOVER_FOUND_NOBODY;

	for(i = 0; i < nInterfaces; i++)
	{
		struct ifreq *item = &ifr[i];

		// Show the device name and IP address
		// printf("%s: IP %s", item->ifr_name, inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr));

		// Get the broadcast address
		if(ioctl(sck, SIOCGIFBRDADDR, item) >= 0)
		{
			// printf(", BROADCAST %s\n", inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr));
			// printf(" BCAST = %08x\n", ((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr);
			// Skip interfaces with out a useful broadcast address
			if (strcmp("0.0.0.0", inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr)))
			{
				retcode = ntv2DiscoverNubs( inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr),
					maxNubs,
					their_addr,
					boardInventory,
					nubsFound,
					timeout,	
					sendto_count,
					true);

				if (retcode == DISCOVER_RECVERR || retcode == DISCOVER_FULL)
				{
					return retcode;
				}
			}
			else
			{
				// printf("%s has broadcast address of 0, skipping...\n", item->ifr_name);
			}
		}
	}
#endif

	return retcode;
}

// Discover nubs at a particular host or particular address
int
ntv2DiscoverNubs(	const char *hostname,
					int maxNubs, // Maximum number of nubs (size of sockaddr_in and boardInventory arrays)
					struct sockaddr_in their_addr[],			// Array of responders
					NTV2DiscoverRespPayload boardInventory[],	// Available boards for each responder
					int &nubsFound,
					int timeout,	// How long to wait for discover responses In seconds
					int sendto_count,
					bool appendNubs)
{
	AJASocket sockfd;
	struct hostent *he;
	long numbytes;
	int broadcast = 1;
	int retcode = DISCOVER_FOUND_NOBODY;
	static unsigned long ignoredNTV2pkts;

	if ((he=gethostbyname(hostname)) == NULL) {  // get the host info
		// Lookup failed, probably unknown hostname
		if (!appendNubs)
		{
			nubsFound = 0;
		}
		return retcode;
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
#ifdef MSWindows
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1) 
#else
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) 
#endif
	{
		perror("setsockopt (SO_BROADCAST)");
		exit(1);
	}

	if (!appendNubs)
	{
		nubsFound = 0;
	}

	their_addr[nubsFound].sin_family = AF_INET;	 // host byte order
	their_addr[nubsFound].sin_port = htons(NTV2DISCOVERYPORT); // short, network byte order
	their_addr[nubsFound].sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr[nubsFound].sin_zero), '\0', 8);  // zero the rest of the struct

	NTV2NubPkt *pPkt = BuildDiscoverQueryPacket();

	int len =  pPkt == 0 ? 0 : sizeof(NTV2NubPktHeader) + pPkt->hdr.dataLength;

	if(NBOifyNTV2NubPkt(pPkt)) {
		// Send out a bunch of queries since it is UDP and they could be lost.
		for (int i = 0; i < sendto_count; i++)
		{
			if ((numbytes=sendto(sockfd, (char *)pPkt, len, 0,
				 (struct sockaddr *)&their_addr[nubsFound], sizeof(struct sockaddr))) == -1) {
				perror("sendto");
			}
		}
	}
	if(pPkt)
		delete  pPkt;

	char buf[sizeof(NTV2NubPkt)];
	socklen_t addr_len;
	addr_len = sizeof(struct sockaddr);

	bool done = false;
	do {

		// Zero out packet
		memset(buf, 0, sizeof(NTV2NubPkt));

		switch(numbytes=recvfromtimeout(sockfd, buf, MAXDATASIZE-1 , timeout, 
				(struct sockaddr *)&their_addr[nubsFound], &addr_len))
		{
			case RVCFROMTIMEOUT_ERR:
				perror("recvfrom");
				retcode = DISCOVER_RECVERR;
				done = true;
				break;

			case RVCFROMTIMEOUT_TIMEDOUT:
				done = true;
				break;
		}

		// Discover packet?
		if (deNBOifyNTV2NubPkt((NTV2NubPkt *)buf, (ULWord) numbytes))
		{
			if (isDiscoverRespPacket((NTV2NubPkt *)buf)) 
			{
#if 0
				printf("Got an unfiltered discover response packet from %s\n", 
					inet_ntoa(their_addr[nubsFound].sin_addr));
#endif			
				// Ignore duplicate responses. 
				bool dupe = false;
				for (int i = 0; i < nubsFound; i++)
				{
					if (their_addr[i].sin_addr.s_addr == their_addr[nubsFound].sin_addr.s_addr) 
					{
						dupe = true;
						break;
					}
				}
				if (!dupe)
				{
					extractBoardInventory((NTV2NubPkt *)buf, &boardInventory[nubsFound], inet_ntoa(their_addr[nubsFound].sin_addr));

					#if 0
					printf("Received a unique discover response packet from %s\n", inet_ntoa(their_addr[nubsFound].sin_addr));
					dumpDiscoveryPacket((NTV2NubPkt *)buf, &boardInventory[nubsFound]);
					#endif

					nubsFound++;
				}

				retcode = DISCOVER_SUCCESS;

				if (nubsFound == maxNubs)
				{
					retcode = DISCOVER_FULL;
					done = true;
				}
			}
			else
			{
				// Not a discovery response packet, count it and discard it.
				++ignoredNTV2pkts;
			}
		}
		else // Non ntv2 packet on our port.  For debugging.  TODO: Nuke this code.
		{
			printf("got packet from %s\n",inet_ntoa(their_addr[nubsFound].sin_addr));
			printf("packet is %ld bytes long\n",numbytes);
			buf[numbytes] = '\0';
			printf("Received: %s",buf);
		}
	} while(!done);

	close(sockfd);

	return retcode;
}

#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
