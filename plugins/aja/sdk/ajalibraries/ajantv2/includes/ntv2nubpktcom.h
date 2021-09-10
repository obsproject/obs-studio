/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubpktcom.h
	@brief		Declares functions that handle NTV2 "nub" packets.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#ifndef __NTV2NUBPKTCOM_H
#define __NTV2NUBPKTCOM_H

#include "ajaexport.h"
#include "ntv2nubtypes.h"

#ifdef MSWindows
	#include <WinSock2.h>
	typedef int socklen_t ;
#else
	#include <sys/socket.h>
#endif

extern AJAExport const char *NTV2NubQueryRespStrProtVer1[eNumNTV2NubPktTypes];
extern AJAExport const char *NTV2NubQueryRespStrProtVer2[eNumNTV2NubPktTypes];

#if !defined (AJAMac) || !defined (ntohll)
	unsigned long long ntohll(unsigned long long n);
#endif
#if !defined (AJAMac) || !defined (htonll)
	unsigned long long htonll(unsigned long long n);
#endif

const AJAExport char * nubQueryRespStr (NTV2NubProtocolVersion	protocolVersion,
										NTV2NubPktType			pktType);

AJAExport void * GetNubPktPayloadPtr (NTV2NubPkt * pPkt);

AJAExport NTV2NubPkt * BuildNubBasePacket (NTV2NubProtocolVersion	protocolVersion,
											NTV2NubPktType			pktType,
											ULWord					payloadSize,
											char **					pPayload);

AJAExport bool NBOifyNTV2NubPkt (NTV2NubPkt * pPkt);
AJAExport bool deNBOifyNTV2NubPkt (NTV2NubPkt * pPkt, ULWord size);
AJAExport bool isNTV2NubPacketType (NTV2NubPkt * pPkt, NTV2NubPktType nubPktType);
AJAExport int sendall (AJASocket s, char * buf, int * len);
AJAExport int recvtimeout_sec (AJASocket s, char * buf, int len, int timeout_seconds);
AJAExport int recvtimeout_usec (AJASocket s, char * buf, int len, int timeout_uSecs);

#define RVCFROMTIMEOUT_ERR		(-1)
#define RVCFROMTIMEOUT_TIMEDOUT	(-2)

AJAExport int recvfromtimeout (AJASocket s, char * buf, int len, int timeout,
								struct sockaddr * their_addr, socklen_t * addr_len);

// Debug functions
AJAExport void dumpDiscoveryPacket (NTV2NubPkt * pPkt, NTV2DiscoverRespPayload * boardInventory = AJA_NULL);
AJAExport void dumpBoardInventory (NTV2DiscoverRespPayload * boardInventory);

#endif	//	__NTV2NUBPKTCOM_H
