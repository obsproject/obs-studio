/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubtypes.h
	@brief		Declares data types and structures used in NTV2 "nub" packets.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#ifndef __NTV2NUBTYPES_H
#define __NTV2NUBTYPES_H

#include "ntv2publicinterface.h"

#define NTV2DISCOVERYPORT	7777	// the port users will be connecting to

#define NTV2NUBPORT			7474	// port we're listening on

#define INVALID_NUB_HANDLE (-1)

typedef ULWord NTV2NubProtocolVersion;

const ULWord ntv2NubProtocolVersionNone = 0;
const ULWord ntv2NubProtocolVersion1 = 1;
const ULWord ntv2NubProtocolVersion2 = 2;
const ULWord ntv2NubProtocolVersion3 = 3;	// Added get buildinfo
const ULWord maxKnownProtocolVersion = 3;

typedef enum
{
	eDiscoverQueryPkt						= 0,
	eDiscoverRespPkt						= 1,
	eNubOpenQueryPkt						= 2,
	eNubOpenRespPkt							= 3,
	eNubReadRegisterSingleQueryPkt			= 4,
	eNubReadRegisterSingleRespPkt			= 5,
	eNubWriteRegisterQueryPkt				= 6,
	eNubWriteRegisterRespPkt				= 7,
	eNubGetAutoCirculateQueryPkt			= 8,
	eNubGetAutoCirculateRespPkt				= 9,
	eNubV1ControlAutoCirculateQueryPkt		= 8,	// Dupe #, maintained for bkwd compat
	eNubV1ControlAutoCirculateRespPkt		= 9,	// Dupe #, maintained for bkwd compat
	eNubWaitForInterruptQueryPkt			= 10,
	eNubWaitForInterruptRespPkt				= 11,
	eNubDriverGetBitFileInformationQueryPkt	= 12,
	eNubDriverGetBitFileInformationRespPkt	= 13,
	eNubDownloadTestPatternQueryPkt			= 14,
	eNubDownloadTestPatternRespPkt			= 15,
	eNubReadRegisterMultiQueryPkt			= 16,
	eNubReadRegisterMultiRespPkt			= 17,
	eNubGetDriverVersionQueryPkt			= 18,
	eNubGetDriverVersionRespPkt				= 19,
	eNubV2ControlAutoCirculateQueryPkt		= 20,	// Replaces eNubV1ControlAutoCirculateQueryPkt
	eNubV2ControlAutoCirculateRespPkt		= 21,	// Replaces eNubV1ControlAutoCirculateRespPkt
	eNubDriverGetBuildInformationQueryPkt	= 22,
	eNubDriverGetBuildInformationRespPkt	= 23,
	eNubDriverDmaTransferQueryPkt			= 24,
	eNubDriverDmaTransferRespPkt			= 25,
	eNubDriverMessageQueryPkt				= 26,
	eNubDriverMessageRespPkt				= 27,
	eNumNTV2NubPktTypes
} NTV2NubPktType;


typedef struct
{
	NTV2NubProtocolVersion	protocolVersion;
	NTV2NubPktType			pktType;
	ULWord					dataLength;						// Length of payload in bytes	
	ULWord					reserved[13];					// Future use
} NTV2NubPktHeader;

#define NTV2_NUBPKT_MAX_DATASIZE	8192	// ISO C++ forbids zero-size arrays

typedef struct
{
	NTV2NubPktHeader		hdr;
	unsigned char			data[NTV2_NUBPKT_MAX_DATASIZE];	// Variable-length payload
} NTV2NubPkt;

#define NTV2_DISCOVER_BOARDINFO_DESC_STRMAX 32

typedef struct
{
    ULWord	boardMask;	// One or more of NTV2DeviceType
} NTV2DiscoverQueryPayload;


typedef struct
{
	ULWord boardNumber;		// Card number, 0 .. 3
	ULWord boardType;		// e.g. BOARDTYPE_KHD
	ULWord boardID;			// From register 50
	char   description[NTV2_DISCOVER_BOARDINFO_DESC_STRMAX];	// "IPADDR: board identifier"
} NTV2DiscoverBoardInfo;


// Enough for 4 KSDs, 4 KHDs, 4 HDNTVs and 4 XENA2s.
// Which would imply a system with 16 PCI slots.
// Four would probably be enough... 
#define NTV2_NUB_MAXBOARDS_PER_HOST 16

typedef struct
{
	ULWord					numBoards;	// Number of entries in the table below
	NTV2DiscoverBoardInfo	discoverBoardInfo[NTV2_NUB_MAXBOARDS_PER_HOST];
} NTV2DiscoverRespPayload;

typedef struct
{
	ULWord	boardNumber;	// Card number, 0 .. 3
	ULWord	boardType;		// e.g. BOARDTYPE_KHD
	LWord	handle;			// A session cookie required for reg gets/sets and close
} NTV2BoardOpenInfo;

// Single read/writes
typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord registerNumber; 
	ULWord registerValue;
	ULWord registerMask;
	ULWord registerShift;
	ULWord result;			// Actually a bool, returned from RegisterRead/RegisterWrite
} NTV2ReadWriteRegisterPayload;

// Multi reads.  TODO: Support writes.

// This following number is enough for the watcher for OEM2K.
// If it is increased, increase the protocol version number.
// Be sure the number of registers fits into the maximum packet
// size.
#define NTV2_NUB_NUM_MULTI_REGS 200	
typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord numRegs;			// In: number to read/write.  (Write not supported yet).
	ULWord result;			// Actually a bool, returned from RegisterRead
	ULWord whichRegisterFailed;	// Only if result is false.  Regs after that contain garbage. 
} NTV2ReadWriteMultiRegisterPayloadHeader;

typedef struct
{
	NTV2ReadWriteMultiRegisterPayloadHeader payloadHeader;
	NTV2ReadWriteRegisterSingle	aRegs[NTV2_NUB_NUM_MULTI_REGS];
} NTV2ReadWriteMultiRegisterPayload;

typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord result;			// Actually a bool, returned from RegisterRead/RegisterWrite
	ULWord eCommand;		// From AUTOCIRCULATE_DATA. 
	ULWord channelSpec;		
	ULWord state;
	ULWord startFrame;		// Acually LWORD
	ULWord endFrame;		// Acually LWORD
	ULWord activeFrame;     // Acually LWORD Current Frame# actually being output (or input), -1, if not active
	ULWord64                rdtscStartTime;         // Performance Counter at start
	ULWord64				audioClockStartTime;    // Register 28 with Wrap Logic
	ULWord64				rdtscCurrentTime;		// Performance Counter at time of call
	ULWord64				audioClockCurrentTime;	// Register 28 with Wrap Logic
	ULWord					framesProcessed;
	ULWord					framesDropped;
	ULWord					bufferLevel;    // how many buffers ready to record or playback

	ULWord					bWithAudio;
	ULWord					bWithRP188;
    ULWord					bFbfChange;
    ULWord					bFboChange ;
	ULWord					bWithColorCorrection;
	ULWord					bWithVidProc;          
	ULWord					bWithCustomAncData;          
} NTV2GetAutoCircPayload;

typedef struct
{
	ULWord  handle;			// A session cookie 
	ULWord result;			// Actually a bool
	ULWord eCommand;		// From AUTOCIRCULATE_DATA. 
	ULWord channelSpec;		

	ULWord lVal1;
	ULWord lVal2;
    ULWord lVal3;
    ULWord lVal4;
    ULWord lVal5;
    ULWord lVal6;

	ULWord bVal1;
	ULWord bVal2;
    ULWord bVal3;
    ULWord bVal4;
    ULWord bVal5;
    ULWord bVal6;
    ULWord bVal7;
    ULWord bVal8;

	// Can't send pointers over network so pvVal1 etc do not appear here.

} NTV2ControlAutoCircPayload; 

typedef struct
{
	LWord  handle;			// A session cookie required for reg gets/sets and close
	ULWord result;			// Actually a bool
	ULWord eInterrupt;
	ULWord timeOutMs;
} NTV2WaitForInterruptPayload;

typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	ULWord bitFileType;					// Actually enum: NTV2K2BitFileType
	BITFILE_INFO_STRUCT bitFileInfo;	// Joy: a portable proprietary type
} NTV2DriverGetBitFileInformationPayload;

typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	BUILD_INFO_STRUCT buildInfo;		// Another portable proprietary type
} NTV2DriverGetBuildInformationPayload;


typedef struct
{
	LWord  handle;						// A session cookie required for reg gets/sets and close
	ULWord result;						// Actually a bool
	ULWord channel;						// Actually an enum: NTV2Channel 
	ULWord testPatternFrameBufferFormat;// Actually an enum: NTV2FrameBufferFormat 
	ULWord signalMask;
	ULWord testPatternDMAEnable;		// Actually a bool
	ULWord testPatternNumber;
} NTV2DownloadTestPatternPayload;

#if 0
#define	NTV2NUB_DISCOVER_QUERY	"Our chief weapons are?"
#define	NTV2NUB_DISCOVER_RESP	"Fear and surprise!"

#define	NTV2NUB_OPEN_QUERY		"I didn't expect a kind of Spanish Inquisition."
#define	NTV2NUB_OPEN_RESP		"NOBODY expects the Spanish Inquisition!"

#define	NTV2NUB_READ_REG_SINGLE_QUERY	"Cardinal Fang! Fetch...THE COMFY CHAIR!"
#define	NTV2NUB_READ_REG_SINGLE_RESP	"The...Comfy Chair?"

#define	NTV2NUB_WRITE_REG_QUERY	"Biggles! Put her in the Comfy Chair!"
#define	NTV2NUB_WRITE_REG_RESP	"Is that really all it is?"

#define	NTV2NUB_GET_AUTOCIRCULATE_QUERY	"Right! How do you plead?"
#define	NTV2NUB_GET_AUTOCIRCULATE_RESP	"Innocent."

#define	NTV2NUB_CONTROL_AUTOCIRCULATE_QUERY	"Biggles! Fetch...THE SOFT CUSHIONS!"
#define	NTV2NUB_CONTROL_AUTOCIRCULATE_RESP	"Here they are, lord."

#define	NTV2NUB_WAIT_FOR_INTERRUPT_QUERY	"My old man said follow the..."
#define	NTV2NUB_WAIT_FOR_INTERRUPT_RESP		"That's enough."

#define	NTV2NUB_DRIVER_GET_BITFILE_INFO_QUERY	"Oh no - what kind of trouble?"
#define	NTV2NUB_DRIVER_GET_BITFILE_INFO_RESP	"One on't cross beams gone owt askew on treddle."

#define	NTV2NUB_DOWNLOAD_TEST_PATTERN_QUERY	"You have three last chances, the nature of which I have divulged in my previous utterance."
#define	NTV2NUB_DOWNLOAD_TEST_PATTERN_RESP	"I don't know what you're talking about."

#define	NTV2NUB_READ_REG_MULTI_QUERY	"Surprise and..."
#define	NTV2NUB_READ_REG_MULTI_RESP		"Ah!... our chief weapons are surprise... blah blah blah."

#define	NTV2NUB_GET_DRIVER_VERSION_QUERY	"Shall I...?"
#define	NTV2NUB_GET_DRIVER_VERSION_RESP		"No, just pretend for God's sake. Ha! Ha! Ha!"
#endif

#endif	//	__NTV2NUBTYPES_H
