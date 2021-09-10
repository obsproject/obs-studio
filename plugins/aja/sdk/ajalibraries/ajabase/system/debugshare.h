/* SPDX-License-Identifier: MIT */
/**
	@file		debugshare.h
	@brief		Declares the constants used for sharing debug messages. These structures are used
				to gather debug messages and share them with the applications that report and log.
	@note		This file is shared with drivers written in c.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_DEBUGSHARE_H
#define AJA_DEBUGSHARE_H

/**
 *	The list of debug message severity codes.
 *	@ingroup AJAGroupDebug
 */
///@{
typedef enum _AJADebugSeverity
{
	AJA_DebugSeverity_Emergency		= 0,	/**< System is unusable */
	AJA_DebugSeverity_Alert			= 1,	/**< Action must be taken immediately */
	AJA_DebugSeverity_Assert		= 2,	/**< Assert conditions */
	AJA_DebugSeverity_Error			= 3,	/**< Error conditions */
	AJA_DebugSeverity_Warning		= 4,	/**< Warning conditions */
	AJA_DebugSeverity_Notice		= 5,	/**< Normal but significant condition */
	AJA_DebugSeverity_Info			= 6,	/**< Informational */
	AJA_DebugSeverity_Debug			= 7,	/**< Debug-level messages */
	AJA_DebugSeverity_Size			= 8		/**< Size of severity enum, must be last */

} AJADebugSeverity;
///@}


/**
 *	The list of debug message groups.
 *	@ingroup AJAGroupDebug
 */
///@{
typedef enum _AJADebugUnit
{
    AJA_DebugUnit_Unknown					= 0,
    AJA_DebugUnit_Critical					= 1,
    AJA_DebugUnit_DriverGeneric				= 2,
    AJA_DebugUnit_ServiceGeneric		    = 3,
    AJA_DebugUnit_UserGeneric				= 4,
    AJA_DebugUnit_VideoGeneric		        = 5,
    AJA_DebugUnit_AudioGeneric		        = 6,
    AJA_DebugUnit_TimecodeGeneric		    = 7,
    AJA_DebugUnit_AncGeneric                = 8,
    AJA_DebugUnit_RoutingGeneric            = 9,
    AJA_DebugUnit_StatsGeneric				= 10,
    AJA_DebugUnit_Enumeration				= 11,
    AJA_DebugUnit_Application				= 12,
    AJA_DebugUnit_QuickTime					= 13,
    AJA_DebugUnit_ControlPanel				= 14,
    AJA_DebugUnit_Watcher				    = 15,
    AJA_DebugUnit_Plugins                   = 16,
    AJA_DebugUnit_CCLine21Decode			= 17,
    AJA_DebugUnit_CCLine21Encode			= 18,
    AJA_DebugUnit_CC608DataQueue			= 19,
    AJA_DebugUnit_CC608MsgQueue				= 20,
    AJA_DebugUnit_CC608Decode				= 21,
    AJA_DebugUnit_CC608DecodeChannel		= 22,
    AJA_DebugUnit_CC608DecodeScreen			= 23,
    AJA_DebugUnit_CC608Encode				= 24,
    AJA_DebugUnit_CC708Decode				= 25,
    AJA_DebugUnit_CC708Service				= 26,
    AJA_DebugUnit_CC708ServiceBlockQueue	= 27,
    AJA_DebugUnit_CC708Window				= 28,
    AJA_DebugUnit_CC708Encode				= 29,
    AJA_DebugUnit_CCFont					= 30,
    AJA_DebugUnit_SMPTEAnc					= 31,
    AJA_DebugUnit_AJAAncData				= 32,
    AJA_DebugUnit_AJAAncList				= 33,
    AJA_DebugUnit_BFT						= 34,
    AJA_DebugUnit_PnP						= 35,
    AJA_DebugUnit_Persistence               = 36,
    AJA_DebugUnit_Avid                      = 37,
    AJA_DebugUnit_DriverInterface           = 38,
    AJA_DebugUnit_AutoCirculate             = 39,
    AJA_DebugUnit_NMOS                      = 40,
    AJA_DebugUnit_App_DiskRead              = 41,
    AJA_DebugUnit_App_DiskWrite             = 42,
    AJA_DebugUnit_App_Decode                = 43,
    AJA_DebugUnit_App_Encode                = 44,
    AJA_DebugUnit_App_DMA                   = 45,
    AJA_DebugUnit_App_Screen                = 46,
    AJA_DebugUnit_App_User1                	= 47,
    AJA_DebugUnit_App_User2               	= 48,
    AJA_DebugUnit_Anc2110Xmit				= 49,
    AJA_DebugUnit_Anc2110Rcv				= 50,
    AJA_DebugUnit_DemoPlayout				= 51,
    AJA_DebugUnit_DemoCapture				= 52,
    AJA_DebugUnit_CSC						= 53,
    AJA_DebugUnit_LUT						= 54,
    AJA_DebugUnit_Cables					= 55,
    AJA_DebugUnit_RPCServer					= 56,
    AJA_DebugUnit_RPCClient					= 57,

    // to add a new unit:
    //
    // if there is an unused unit below:
    //
    //   * rename the next available unused and move above this comment block
    //   * make sure the enum starts with "AJA_DebugUnit_"
    //   * update the value of AJA_DebugUnit_FirstUnused with the value of the new next available unused
    //
    // else if there are not any unused units below:
    //
    //   * add the new unit above this message block with the next available index
    //   * make sure the enum starts with "AJA_DebugUnit_"
    //   * increment AJA_DebugUnit_Size by 1
    //
    // if no more unused units
    //   * set AJA_DebugUnit_FirstUnused to the same value as AJA_DebugUnit_Size
    //
    AJA_DebugUnit_FirstUnused               = 58,
    AJA_DebugUnit_Unused_58                 = AJA_DebugUnit_FirstUnused,
    AJA_DebugUnit_Unused_59                 = 59,
    AJA_DebugUnit_Unused_60                 = 60,
    AJA_DebugUnit_Unused_61                 = 61,
    AJA_DebugUnit_Unused_62                 = 62,
    AJA_DebugUnit_Unused_63                 = 63,
    AJA_DebugUnit_Unused_64                 = 64,
    AJA_DebugUnit_Unused_65                 = 65,
    AJA_DebugUnit_Unused_66                 = 66,
    AJA_DebugUnit_Unused_67                 = 67,
    AJA_DebugUnit_Unused_68                 = 68,
    AJA_DebugUnit_Unused_69                 = 69,
    AJA_DebugUnit_Unused_70                 = 70,
    AJA_DebugUnit_Unused_71                 = 71,
    AJA_DebugUnit_Unused_72                 = 72,
    AJA_DebugUnit_Unused_73                 = 73,
    AJA_DebugUnit_Unused_74                 = 74,
    AJA_DebugUnit_Unused_75                 = 75,
    AJA_DebugUnit_Unused_76                 = 76,
    AJA_DebugUnit_Unused_77                 = 77,
    AJA_DebugUnit_Unused_78                 = 78,
    AJA_DebugUnit_Unused_79                 = 79,
    AJA_DebugUnit_Unused_80                 = 80,
    AJA_DebugUnit_Unused_81                 = 81,
    AJA_DebugUnit_Unused_82                 = 82,
    AJA_DebugUnit_Unused_83                 = 83,
    AJA_DebugUnit_Unused_84                 = 84,

    AJA_DebugUnit_Size						= 85

} AJADebugUnit;
///@}


/**
	@defgroup	AJAUnitDestination	AJA_DEBUG_DESTINATION
	Bit definitions that specify the destination of a debug message.

	@ingroup AJAGroupDebug
	Use logical OR for multiple destinations.
**/
///@{
#define AJA_DEBUG_DESTINATION_NONE		0			/**< Unknown destination, used as default */
#define AJA_DEBUG_DESTINATION_DEBUG		0x00000001	/**< Send message to the debug window */
#define AJA_DEBUG_DESTINATION_CONSOLE	0x00000002	/**< Send message to the console */
#define AJA_DEBUG_DESTINATION_LOG		0x00000004	/**< Send message to a log file */
#define AJA_DEBUG_DESTINATION_DRIVER	0x00000008	/**< Send message directly to driver output (driver messages only) */
///@}

/**
	@defgroup	AJAGroupVarious	AJA_DEBUG
	Various parameters that define the characteristics of the shared debug memory space.
	SDK 16.0 introduced a new stats measurement feature that still allows logging to work in pre-16.0 clients.
	@ingroup	AJAGroupDebug
**/
///@{
#define AJA_DEBUG_MAGIC_ID              AJA_FOURCC('D','B','U','G') /**< Magic identifier of the debug system */
#define AJA_DEBUG_VERSION				110					/**< Version of the debug system */
#define AJA_DEBUG_UNIT_ARRAY_SIZE		65536				/**< Number of unit destinations */
#define AJA_DEBUG_SEVERITY_ARRAY_SIZE	64					/**< Number of severity destinations */
#define AJA_DEBUG_MAX_NUM_STATS			256					/**< Max number of individual stats measurements (multiple of 64) */
#define	AJA_DEBUG_STAT_DEQUE_SIZE		11
#define AJA_DEBUG_MESSAGE_MAX_SIZE		512					/**< Maximum size of a message */
#define AJA_DEBUG_MESSAGE_RING_SIZE		4096				/**< Size of the message ring */
#define AJA_DEBUG_FILE_NAME_MAX_SIZE	512					/**< Maximum size of a file name */
#define AJA_DEBUG_SHARE_NAME            "aja-shm-debug"     /**< Name of the shared memory for the debug messages */
#define AJA_DEBUG_TICK_RATE				1000000				/**< Resolution of debug time in ticks/second */
#define AJA_DEBUG_STATE_FILE_VERSION	510					/**< Version number of the state file format */
///@}

/**
	Structure representing the debug message.
	@ingroup	AJAGroupDebug
**/

// using a 16 byte alignment for arm64 atomic operations to work properly
#pragma pack(push, 16)

typedef struct _AJADebugMessage
{
    uint64_t	sequenceNumber;								/**< Sequence number of this message */
	int64_t		time;										/**< Time this message was generated (microseconds) */
    int64_t     wallTime;									/**< Time this message was generated as returned by time() */
	int32_t		groupIndex;									/**< Group that generated this message */
	uint32_t	destinationMask;							/**< Destination of the message */
	int32_t		severity;									/**< Severity of the message */
	int32_t		lineNumber;									/**< Source file line number that generated this message */
    uint64_t    pid;                                        /**< Process ID that generated the message */
    uint64_t    tid;                                        /**< Thread ID that generated the message */
	char		fileName[AJA_DEBUG_FILE_NAME_MAX_SIZE];		/**< Source file name that generated this message */
	char		messageText[AJA_DEBUG_MESSAGE_MAX_SIZE];	/**< Text generated for this message */
} AJADebugMessage;


/**
	64-byte structure representing an unsigned 32-bit measurement (timer, counter or data value).
	As a timer, it stores minimum, maximum and average elapsed time in microseconds.
	As a data value, it stores minimum, maximum, moving average value, and last update usec timestamp.
	As a counter, it can increment or decrement, with or without rollover/rollunder.
	@ingroup	AJAGroupDebug
**/
class AJADebugStat
{
	public:
	uint32_t	fMin;								/**< Smallest value yet seen. (Fixed at 0xFFFFFFFF and unused for counters.) */
	uint32_t	fMax;								/**< Largest value yet seen. (Fixed at zero and unused for counters.) */
	uint32_t	fCount;								/**< Update/change count */
	uint64_t	fLastTimeStamp;						/**< Timestamp (start time for timer, zero if not running;  last update time for counter or data value) */
	uint32_t	fValues[AJA_DEBUG_STAT_DEQUE_SIZE];	/**< Deque that provides an 11-sample moving average. (Unused for counters.) */

	AJADebugStat()		{Reset();}

	void Reset (void)
	{
		fMin = 0xFFFFFFFF;
		fMax = fCount = 0;
		fLastTimeStamp = 0;
		for (size_t n(0);  n < AJA_DEBUG_STAT_DEQUE_SIZE;  n++)
			fValues[n] = 0;
	}

	double Average(void) const;	/**< Returns the average of the stored values */
	uint64_t Sum (const size_t inNum = 0) const;	/**< Returns the sum of the first "inNum" stored values */
	void Start (void);	/**< Starts a timer by setting fLastTimeStamp to the high-resolution host OS system timestamp */

	/**
	 *	Stops the timer, stores the elapsed time in the deque, bumps fCount, and clears fLastTimeStamp.
	 *	Also updates fMin and/or fMax if necessary.
	 */
	void Stop (void);

	/**
	 *	Increments fCount by the given amount, and sets fLastTimeStamp.
	 *	@param[in]	inIncrement		Optionally specifies a different increment value. Defaults to 1.
	 *	@param[in]	inRollOver		Optionally controls if overflow is permitted. Defaults to true.
	 */
	void Increment (const uint32_t inIncrement = 1, const bool inRollOver = true);

	/**
	 *	Decrements fCount by the given amount, and sets fLastTimeStamp.
	 *	@param[in]	inDecrement		Optionally specifies a different increment value. Defaults to 1.
	 *	@param[in]	inRollUnder		Optionally controls if underflow is permitted. Defaults to true.
	 */
	void Decrement (const uint32_t inDecrement = 1, const bool inRollUnder = true);

	/**
	 *	Inserts the given value into the deque, bumps fCount, and sets fLastTimeStamp to the current time.
	 *	@param[in]	inValue		Specifies the value to store.
	 *	@param[in]	inStamp		Optionally specifies whether to set the timestamp or not. Defaults to true.
	 */
	void SetValue (const uint32_t inValue, const bool inStamp = true);
};


/**
	Structure representing the shared debug groups and messages.
	@ingroup	AJAGroupDebug
**/
typedef struct _AJADebugShare
{
	uint32_t			magicId;									/**< Magic cookie identifier used to id this as an AJADebugShare structure */
	uint32_t			version;									/**< Version of the debug system */
	uint64_t volatile	writeIndex;									/**< Write index for the message ring */
	int32_t volatile	clientRefCount;								/**< A count of current number of clients using structure*/

	uint32_t			messageRingCapacity;						/**< The number of messages that can be in the ring */
	uint32_t			messageTextCapacity;						/**< The maximum text capacity of message in bytes */
	uint32_t			messageFileNameCapacity;					/**< The maximum text capacity of message filename in bytes */
	uint32_t			unitArraySize;								/**< The number of unit destinations */

	uint64_t volatile	statsMessagesAccepted;						/**< The number of messages accepted into the ring since creation */
	uint64_t volatile	statsMessagesIgnored;						/**< The number of messages ignored since creation, filtered out */

	uint32_t			statCapacity;								/**< The number of stats that can be stored, or zero if no stat facility (new in SDK 16) */
	uint32_t volatile	statAllocChanges;							/**< Number of changes to statAllocMask (new in SDK 16.0) */
	uint64_t			statAllocMask[AJA_DEBUG_MAX_NUM_STATS/64];	/**< Stats allocation bitmask, 1 bit per stats measurement (new in SDK 16.0) */
	uint32_t			reserved[128 - 1 - 1 - 2*AJA_DEBUG_MAX_NUM_STATS/64];	/**< Reserved (was [128] in version 110) */
	uint32_t			unitArray[AJA_DEBUG_UNIT_ARRAY_SIZE];		/**< Array of message destinations by unit */
	AJADebugMessage		messageRing[AJA_DEBUG_MESSAGE_RING_SIZE];	/**< Message ring holding current message data */
	AJADebugStat		stats[AJA_DEBUG_MAX_NUM_STATS];				/**< Per-stat measurement data (new in v111) */
} AJADebugShare;

#pragma pack(pop)

#endif	//	AJA_DEBUGSHARE_H
