/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2discover.h
	@brief		Declares the NTV nub discovery functions.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2DISCOVER_H
#define NTV2DISCOVER_H
#if defined (AJALinux ) || defined (AJAMac)
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include "ajaexport.h"
#include "ntv2nubtypes.h"

#if defined (NTV2_NUB_CLIENT_SUPPORT)

#define DISCOVER_SUCCESS		(0)
#define DISCOVER_RECVERR		(-1)
#define DISCOVER_FULL	 		(-2)
#define DISCOVER_FOUND_NOBODY	(-3)

// Discover nubs on all network interfaces
AJAExport int	ntv2DiscoverNubs (	int						inMaxNubs,				//	Maximum number of nubs (size of sockaddr_in and boardInventory arrays)
									struct sockaddr_in		their_addr [],			//	Array of responders
									NTV2DiscoverRespPayload	boardInventory [],		//	Available boards for each responder
									int &					outFoundNubCount,		//	How many nubs were found (no more than inMaxNubs)
									int						inWaitTimeoutSecs,		//	How long to wait for discover responses In seconds
									int						sendto_count	= 5);	//	default

AJAExport int	ntv2DiscoverNubs (	const char *			hostname,
									int						inMaxNubs, 				//	Maximum number of nubs (size of sockaddr_in and boardInventory arrays)
									struct sockaddr_in		their_addr [],			//	Array of responders
									NTV2DiscoverRespPayload	boardInventory [],		//	Available boards for each responder
									int &					outFoundNubCount,		//	How many nubs were found (no more than maxNubs above)
									int						inWaitTimeoutSecs,		//	How long to wait for discover responses In seconds
									int						sendto_count	= 5,	//	How many UDP queries to send (can be lost)
									bool					appendNubs	= false);	// If true, leave existing nubs in boardInventory and add new ones
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)

#endif	//	NTV2DISCOVER_H
