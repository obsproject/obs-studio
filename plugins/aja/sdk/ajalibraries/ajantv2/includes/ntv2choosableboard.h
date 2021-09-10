/* SPDX-License-Identifier: MIT */
/**
    @file		ntv2config2022.h
    @brief		Declares the NTV2ChoosableBoard struct used for "nub" discovery.
    @copyright	(C) 2005-2021 AJA Video Systems, Inc.
**/

#ifndef NTV2CHOOSABLEBOARD_H
#define NTV2CHOOSABLEBOARD_H

#define MAX_LOCAL_BOARDS	12
#define MAX_REMOTE_BOARDS	512
#define MAX_CHOOSABLE_BOARDS ( MAX_LOCAL_BOARDS + MAX_REMOTE_BOARDS)

#define NO_BOARD_CHOSEN (99999)

#define CHOOSABLE_BOARD_STRMAX	(16)
typedef struct 
{
	ULWord			boardNumber;	// Card number, 0 .. 3
	ULWord			boardType;		// e.g. BOARDTYPE_KHD
	NTV2DeviceID	boardID;
	char			description [NTV2_DISCOVER_BOARDINFO_DESC_STRMAX];	// "IPADDR: board identifier"
	char			hostname [CHOOSABLE_BOARD_STRMAX];	// 0 len == local board.
} NTV2ChoosableBoard;

#endif //NTV2CHOOSABLEBOARD_H
