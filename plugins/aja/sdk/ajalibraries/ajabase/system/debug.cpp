/* SPDX-License-Identifier: MIT */
/**
	@file		debug.cpp
	@brief		Implements the AJADebug class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/common.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/process.h"
#include "ajabase/system/system.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/thread.h"

#if defined(AJA_LINUX)
	#include <stdarg.h>
#endif
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static std::vector<std::string> sGroupLabelVector;
static const std::string sSeverityString[] = {"emergency", "alert", "assert", "error", "warning", "notice", "info", "debug"};
static AJALock sLock;
static AJADebugShare* spShare = NULL;
static bool sDebug = false;

#define addDebugGroupToLabelVector(x) sGroupLabelVector.push_back(#x)

#define	STAT_BIT_SHIFT		(1ULL<<(inKey%64))
#define	STAT_BIT_TEST		(spShare->statAllocMask[inKey/(AJA_DEBUG_MAX_NUM_STATS/64)] & STAT_BIT_SHIFT)
#define	IS_STAT_BAD			!STAT_BIT_TEST
#define	STAT_BIT_SET		spShare->statAllocMask[inKey/(AJA_DEBUG_MAX_NUM_STATS/64)] |= STAT_BIT_SHIFT
#define	STAT_BIT_CLEAR		spShare->statAllocMask[inKey/(AJA_DEBUG_MAX_NUM_STATS/64)] &= 0xFFFFFFFFFFFFFFFF - STAT_BIT_SHIFT


AJAStatus AJADebug::Open (bool incrementRefCount)
{
	if (!sLock.IsValid())
		return AJA_STATUS_INITIALIZE;

	AJAAutoLock lock(&sLock);

	// set the debug flag
	sDebug = false;
#if defined(AJA_DEBUG)
	sDebug = true;
#endif

	try
	{
		// allocate the shared data structure for messages
		if (!spShare)
		{
			// allocate the shared memory storage
			size_t size (sizeof(AJADebugShare));
			spShare = reinterpret_cast<AJADebugShare*>(AJAMemory::AllocateShared(&size, AJA_DEBUG_SHARE_NAME));
			if (spShare == NULL || spShare == reinterpret_cast<void*>(-1))
			{
				spShare = NULL;
				Close();
				return AJA_STATUS_FAIL;
			}

			if (size < (sizeof(AJADebugShare) - sizeof(AJADebugStat)*AJA_DEBUG_MAX_NUM_STATS))
			{	//	Fail anything smaller than v110's size
				Close();
				return AJA_STATUS_FAIL;
			}

			// check version
			if (spShare->version == 0)
			{	//	Initialize shared memory region...
				memset((void*)spShare, 0, sizeof(AJADebugShare));
                spShare->magicId					= AJA_DEBUG_MAGIC_ID;
                spShare->version					= AJA_DEBUG_VERSION;
                spShare->writeIndex					= 0;
                spShare->clientRefCount				= 0;
                spShare->messageRingCapacity		= AJA_DEBUG_MESSAGE_RING_SIZE;
                spShare->messageTextCapacity		= AJA_DEBUG_MESSAGE_MAX_SIZE;
                spShare->messageFileNameCapacity	= AJA_DEBUG_FILE_NAME_MAX_SIZE;
                spShare->unitArraySize				= AJA_DEBUG_UNIT_ARRAY_SIZE;
                spShare->statsMessagesAccepted		= 0;
                spShare->statsMessagesIgnored		= 0;
				spShare->statCapacity				= AJA_DEBUG_MAX_NUM_STATS;
				spShare->statAllocChanges			= 0;
				for (size_t num(0);  num < size_t(AJA_DEBUG_MAX_NUM_STATS/64);  num++)
					spShare->statAllocMask[num]		= 0;
				spShare->unitArray[AJA_DebugUnit_Critical] = AJA_DEBUG_DESTINATION_CONSOLE;
			}

			// shared data must be the correct version
			if (spShare->version != AJA_DEBUG_VERSION)
			{
				Close();
				return AJA_STATUS_FAIL;
			}

            if (incrementRefCount)
            {
                // increment reference count;
                spShare->clientRefCount++;
            }

            // Create the Unit Label Vector
            sGroupLabelVector.clear();
            addDebugGroupToLabelVector(AJA_DebugUnit_Unknown);
            addDebugGroupToLabelVector(AJA_DebugUnit_Critical);
            addDebugGroupToLabelVector(AJA_DebugUnit_DriverGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_ServiceGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_UserGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_VideoGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_AudioGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_TimecodeGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_AncGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_RoutingGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_StatsGeneric);
            addDebugGroupToLabelVector(AJA_DebugUnit_Enumeration);
            addDebugGroupToLabelVector(AJA_DebugUnit_Application);
            addDebugGroupToLabelVector(AJA_DebugUnit_QuickTime);
            addDebugGroupToLabelVector(AJA_DebugUnit_ControlPanel);
            addDebugGroupToLabelVector(AJA_DebugUnit_Watcher);
            addDebugGroupToLabelVector(AJA_DebugUnit_Plugins);
            addDebugGroupToLabelVector(AJA_DebugUnit_CCLine21Decode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CCLine21Encode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608DataQueue);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608MsgQueue);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608Decode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608DecodeChannel);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608DecodeScreen);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC608Encode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC708Decode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC708Service);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC708ServiceBlockQueue);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC708Window);
            addDebugGroupToLabelVector(AJA_DebugUnit_CC708Encode);
            addDebugGroupToLabelVector(AJA_DebugUnit_CCFont);
            addDebugGroupToLabelVector(AJA_DebugUnit_SMPTEAnc);
            addDebugGroupToLabelVector(AJA_DebugUnit_AJAAncData);
            addDebugGroupToLabelVector(AJA_DebugUnit_AJAAncList);
            addDebugGroupToLabelVector(AJA_DebugUnit_BFT);
            addDebugGroupToLabelVector(AJA_DebugUnit_PnP);
            addDebugGroupToLabelVector(AJA_DebugUnit_Persistence);
            addDebugGroupToLabelVector(AJA_DebugUnit_Avid);
            addDebugGroupToLabelVector(AJA_DebugUnit_DriverInterface);
            addDebugGroupToLabelVector(AJA_DebugUnit_AutoCirculate);
            addDebugGroupToLabelVector(AJA_DebugUnit_NMOS);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_DiskRead);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_DiskWrite);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_Decode);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_Encode);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_DMA);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_Screen);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_User1);
            addDebugGroupToLabelVector(AJA_DebugUnit_App_User2);
            addDebugGroupToLabelVector(AJA_DebugUnit_Anc2110Xmit);
            addDebugGroupToLabelVector(AJA_DebugUnit_Anc2110Rcv);
            addDebugGroupToLabelVector(AJA_DebugUnit_DemoPlayout);
            addDebugGroupToLabelVector(AJA_DebugUnit_DemoCapture);
            addDebugGroupToLabelVector(AJA_DebugUnit_CSC);
            addDebugGroupToLabelVector(AJA_DebugUnit_LUT);
            addDebugGroupToLabelVector(AJA_DebugUnit_Cables);
            addDebugGroupToLabelVector(AJA_DebugUnit_RPCServer);
            addDebugGroupToLabelVector(AJA_DebugUnit_RPCClient);

            for (int i(AJA_DebugUnit_FirstUnused);  i < AJA_DebugUnit_Size;  i++)
            {
                std::string name("AJA_DebugUnit_Unused_");
                name += aja::to_string(i);
                sGroupLabelVector.push_back(name);
            }

            assert(sGroupLabelVector.size() == AJA_DebugUnit_Size);
		}
	}
	catch(...)
	{
		Close();
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::Close (bool decrementRefCount)
{
	AJAAutoLock lock(&sLock);
	try
	{		
		if (spShare)
		{
			if (decrementRefCount)
			{
				spShare->clientRefCount--;	// decrement reference count
				if (spShare->clientRefCount <= 0)
					spShare->clientRefCount = 0;
			}

			// free the shared data structure
			if (spShare)
				AJAMemory::FreeShared(spShare);
		}
	}
	catch(...)
	{
	}
	spShare = NULL;
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::Enable (int32_t index, uint32_t destination)
{
	uint32_t currentDestination = 0;
	AJAStatus status = GetDestination(index, &currentDestination);
	if (status != AJA_STATUS_SUCCESS)
		return status;
	return SetDestination(index, currentDestination | destination);
}

	
AJAStatus AJADebug::Disable (int32_t index, uint32_t destination)
{
	uint32_t currentDestination = 0;
	AJAStatus status = GetDestination(index, &currentDestination);
	if (status != AJA_STATUS_SUCCESS)
		return status;
	return SetDestination(index, currentDestination & ~destination);
}


AJAStatus AJADebug::SetDestination (int32_t index, uint32_t destination)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;	//	Not open
	if (index < 0  ||  index >= AJA_DEBUG_UNIT_ARRAY_SIZE)
		return AJA_STATUS_RANGE;	// Bad index
	try
	{	// save the destination
		spShare->unitArray[index] = destination;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetDestination (const int32_t index, uint32_t & outDestination)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (index < 0  ||  index >= AJA_DEBUG_UNIT_ARRAY_SIZE)
		return AJA_STATUS_RANGE;
	try
	{
		outDestination = spShare->unitArray[index];
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


bool AJADebug::IsActive (int32_t index)
{
	if (!spShare)
		return false;	//	Not open
	if (index < 0  ||  index >= AJA_DEBUG_UNIT_ARRAY_SIZE)
		return false;	//	Bad index
	try
	{	// if no destination return false
		if (spShare->unitArray[index] == AJA_DEBUG_DESTINATION_NONE)
			return false;
	}
	catch(...)
	{
		return false;
	}
	return true;
}

uint32_t AJADebug::Version (void)
{
	if (!spShare)
		return 0;	//	Not open
	return spShare->version;
}

uint32_t AJADebug::TotalBytes (void)
{
	if (!spShare)
		return 0;	//	Not open
	if (HasStats())
		return uint32_t(sizeof(AJADebugShare));
	return uint32_t(sizeof(AJADebugShare))  -  AJA_DEBUG_MAX_NUM_STATS * uint32_t(sizeof(AJADebugStat)); 
}

bool AJADebug::IsOpen (void)
{
	return spShare != NULL;
}


bool AJADebug::IsDebugBuild (void)
{
    return sDebug;
}

inline int64_t debug_time (void)
{
    int64_t ticks = AJATime::GetSystemCounter();
    int64_t rate = AJATime::GetSystemFrequency();
    int64_t time = ticks / rate * AJA_DEBUG_TICK_RATE;
    time += (ticks % rate) * AJA_DEBUG_TICK_RATE / rate;
    return time;
}

inline uint64_t report_common(int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, uint64_t& writeIndex, int32_t& messageIndex)
{
	static const char * spUnknown = "unknown";
	if (!spShare)
		return false;

	// check for active client to receive messages
	if (spShare->clientRefCount <= 0)
	{
		// nobody is listening so bail quickly
		return false;
	}

	// check for valid index
	if ((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
	{
		index = AJA_DebugUnit_Unknown;
	}
	// check for destination
	if (spShare->unitArray[index] == AJA_DEBUG_DESTINATION_NONE)
	{
		AJAAtomic::Increment(&spShare->statsMessagesIgnored);
		return false;
	}

	// check for valid severity
	if ((severity < 0) || (severity >= AJA_DebugSeverity_Size))
	{
		severity = AJA_DebugSeverity_Warning;
	}

	// check for valid file name
	if (!pFileName)
		pFileName = spUnknown;

	// increment the message write index
	writeIndex = AJAAtomic::Increment(&spShare->writeIndex);

	// modulo the ring size to determine the message array index
	messageIndex = writeIndex % AJA_DEBUG_MESSAGE_RING_SIZE;

	// save the message data
	spShare->messageRing[messageIndex].groupIndex = index;
	spShare->messageRing[messageIndex].destinationMask = spShare->unitArray[index];
	spShare->messageRing[messageIndex].time = debug_time();
	spShare->messageRing[messageIndex].wallTime = (int64_t)time(NULL);
	aja::safer_strncpy(spShare->messageRing[messageIndex].fileName, pFileName, strlen(pFileName), AJA_DEBUG_FILE_NAME_MAX_SIZE);
	spShare->messageRing[messageIndex].lineNumber = lineNumber;
	spShare->messageRing[messageIndex].severity = severity;
	spShare->messageRing[messageIndex].pid = AJAProcess::GetPid();
	spShare->messageRing[messageIndex].tid = AJAThread::GetThreadId();

	return true;
}


void AJADebug::Report (int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, ...)
{
	if (!spShare)
		return;	//	Not open
	try
	{
		uint64_t writeIndex = 0;
		int32_t messageIndex = 0;
		if (report_common(index, severity, pFileName, lineNumber, writeIndex, messageIndex))
		{
			// format the message
			va_list vargs;
			va_start(vargs, lineNumber);
			const char* pFormat = va_arg(vargs, const char*);
			// check for valid message
			if (pFormat == NULL)
			{
				pFormat = (char*) "no message";
			}
			ajavsnprintf(spShare->messageRing[messageIndex].messageText,
						 AJA_DEBUG_MESSAGE_MAX_SIZE,
						 pFormat, vargs);
			va_end(vargs);

			// set last to indicate message complete
			AJAAtomic::Exchange(&spShare->messageRing[messageIndex].sequenceNumber, writeIndex);
			AJAAtomic::Increment(&spShare->statsMessagesAccepted);
		}
	}
	catch (...)
	{
	}
}


void AJADebug::Report (int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, const std::string& message)
{
	if (!spShare)
		return;	//	Not open
	try
	{
		uint64_t writeIndex = 0;
		int32_t messageIndex = 0;
		if (report_common(index, severity, pFileName, lineNumber, writeIndex, messageIndex))
		{
			// copy the message
			aja::safer_strncpy(spShare->messageRing[messageIndex].messageText, message.c_str(), message.length()+1, AJA_DEBUG_MESSAGE_MAX_SIZE);

			// set last to indicate message complete
			AJAAtomic::Exchange(&spShare->messageRing[messageIndex].sequenceNumber, writeIndex);
			AJAAtomic::Increment(&spShare->statsMessagesAccepted);
		}
	}
	catch (...)
	{
	}
}

void AJADebug::AssertWithMessage (const char* pFileName, int32_t lineNumber, const std::string& pExpression)
{
#if defined(AJA_DEBUG)
	// check for open
	if (!spShare)
		assert(false);

	try
	{
		// check for active client to receive messages
		if (spShare->clientRefCount > 0)
		{
			// check for valid file name
			if (pFileName == NULL)
			{
				pFileName = (char*) "unknown";
			}

			// increment the message write index
			uint64_t writeIndex = AJAAtomic::Increment(&spShare->writeIndex);

			// modulo the ring size to determine the message array index
			int32_t messageIndex = writeIndex % AJA_DEBUG_MESSAGE_RING_SIZE;

			// save the message data
			spShare->messageRing[messageIndex].groupIndex = AJA_DebugUnit_Critical;
			spShare->messageRing[messageIndex].destinationMask = spShare->unitArray[AJA_DebugUnit_Critical];
			spShare->messageRing[messageIndex].time = debug_time();
			spShare->messageRing[messageIndex].wallTime = (int64_t)time(NULL);
			aja::safer_strncpy(spShare->messageRing[messageIndex].fileName, pFileName, strlen(pFileName), AJA_DEBUG_FILE_NAME_MAX_SIZE);
			spShare->messageRing[messageIndex].lineNumber = lineNumber;
			spShare->messageRing[messageIndex].severity = AJA_DebugSeverity_Assert;
			spShare->messageRing[messageIndex].pid = AJAProcess::GetPid();
			spShare->messageRing[messageIndex].tid = AJAThread::GetThreadId();

			// format the message
			ajasnprintf(spShare->messageRing[messageIndex].messageText,
						AJA_DEBUG_MESSAGE_MAX_SIZE,
						"assertion failed (file %s, line %d):  %s\n",
						pFileName, lineNumber, pExpression.c_str());

			// set last to indicate message complete
			AJAAtomic::Exchange(&spShare->messageRing[messageIndex].sequenceNumber, writeIndex);
			AJAAtomic::Increment(&spShare->statsMessagesAccepted);
		}
	}
	catch (...)
	{
	}

	assert(false);
#else
	AJA_UNUSED(pFileName);
	AJA_UNUSED(lineNumber);
	AJA_UNUSED(pExpression);
#endif
}


uint32_t AJADebug::MessageRingCapacity (void)
{
	if (!spShare)
		return 0;
	return spShare->messageRingCapacity;
}


AJAStatus AJADebug::GetClientReferenceCount (int32_t & outRefCount)
{
	outRefCount = 0;
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	try
	{
		outRefCount = spShare->clientRefCount;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::SetClientReferenceCount (int32_t refCount)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	try
	{
		spShare->clientRefCount = refCount;
		if (refCount <= 0)
		{
			// this will handle shuting everything down if ref count goes to 0 or less
			AJADebug::Close();
		}
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetSequenceNumber (uint64_t & outSequenceNumber)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	try
	{
		outSequenceNumber = spShare->writeIndex;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS; 
}


AJAStatus AJADebug::GetMessageSequenceNumber (const uint64_t sequenceNumber, uint64_t & outSequenceNumber)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{        
		outSequenceNumber = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].sequenceNumber;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageGroup (const uint64_t sequenceNumber, int32_t & outGroupIndex)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outGroupIndex = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].groupIndex;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageDestination (const uint64_t sequenceNumber, uint32_t & outDestination)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outDestination = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].destinationMask;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageTime (const uint64_t sequenceNumber, uint64_t & outTime)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outTime = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].time;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::GetMessageWallClockTime (const uint64_t sequenceNumber, int64_t & outTime)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outTime = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].wallTime;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::GetMessageFileName (const uint64_t sequenceNumber, std::string & outFileName)
{
	outFileName.clear();
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outFileName = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].fileName;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::GetMessageFileName (uint64_t sequenceNumber, const char** ppFileName)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	if (ppFileName == NULL)
		return AJA_STATUS_NULL;
	try
	{
		*ppFileName = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].fileName;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageLineNumber (const uint64_t sequenceNumber, int32_t & outLineNumber)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outLineNumber = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].lineNumber;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageSeverity (const uint64_t sequenceNumber, int32_t & outSeverity)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outSeverity = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].severity;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessageText (const uint64_t sequenceNumber, std::string & outMessage)
{
	outMessage.clear();
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outMessage = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].messageText;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::GetMessageText (uint64_t sequenceNumber, const char** ppMessage)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	if (!ppMessage)
		return AJA_STATUS_NULL;
	try
	{
		*ppMessage = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].messageText;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetProcessId (const uint64_t sequenceNumber, uint64_t & outPid)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outPid = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].pid;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetThreadId (const uint64_t sequenceNumber, uint64_t & outTid)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (sequenceNumber > spShare->writeIndex)
		return AJA_STATUS_RANGE;
	try
	{
		outTid = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].tid;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessagesAccepted (uint64_t & outCount)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	try
	{
		outCount = spShare->statsMessagesAccepted;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::GetMessagesIgnored (uint64_t & outCount)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	try
	{
		outCount = spShare->statsMessagesIgnored;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}


const char* AJADebug::GetSeverityString (int32_t severity)
{
	if (severity < 0  ||  severity > 7)
		return "severity range error";
	return sSeverityString[severity].c_str();
}

const std::string & AJADebug::SeverityName (const int32_t severity)
{	static const std::string emptystr;
	if (severity < 0  ||  severity > 7)
		return emptystr;
	return sSeverityString[severity];
}


const char* AJADebug::GetGroupString (int32_t group)
{
	if (group < 0  ||  group >= int32_t(sGroupLabelVector.size()))
		return "index range error";
	if (sGroupLabelVector.at(size_t(group)).empty())
		return "no label";
	return sGroupLabelVector.at(size_t(group)).c_str();
}


const std::string & AJADebug::GroupName (const int32_t group)
{
	static const std::string sRangeErr("<bad index>");
	static const std::string sNoLabelErr("<empty>");
	if (group < 0  ||  group >= int32_t(sGroupLabelVector.size()))
		return sRangeErr;
	if (sGroupLabelVector.at(size_t(group)).empty())
		return sNoLabelErr;
	return sGroupLabelVector.at(size_t(group));
}


AJAStatus AJADebug::SaveState (char* pFileName)
{
	FILE* pFile(NULL);
	if (!spShare)
		return AJA_STATUS_INITIALIZE;

	// open a new state file
	pFile = fopen(pFileName, "w");
	if (!pFile)
		return AJA_STATUS_FAIL;

	try
	{	// write the header
		fprintf(pFile, "AJADebugVersion: %d\n", spShare->version);
		fprintf(pFile, "AJADebugStateFileVersion: %d\n", AJA_DEBUG_STATE_FILE_VERSION);
		for (int i = 0; i < AJA_DEBUG_UNIT_ARRAY_SIZE; i++)
		{
			// write groups with destinations enabled
			if (spShare->unitArray[i])
			{
                if (i < AJA_DebugUnit_Size)
                    fprintf(pFile, "GroupDestination: %6d : %08x\n", i, spShare->unitArray[i]);
                else
                    fprintf(pFile, "CustomGroupDestination: %6d : %08x\n", i, spShare->unitArray[i]);
			}
		}
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	fclose(pFile);
	return AJA_STATUS_SUCCESS;
}


AJAStatus AJADebug::RestoreState (char* pFileName)
{
	FILE* pFile(NULL);
	if (!spShare)
		return AJA_STATUS_INITIALIZE;

	// open existing file
	pFile = fopen(pFileName, "r");
	if (!pFile)
		return AJA_STATUS_FAIL;

	try
	{
		int32_t count;
		uint32_t version;
		int32_t index;
		uint32_t destination;
		int intVersion;

		// read the header
		count = fscanf(pFile, " AJADebugVersion: %d", &intVersion);
		version = intVersion;
		if((count != 1) || (version != spShare->version))
		{
            fclose(pFile);
			return AJA_STATUS_FAIL;
		}
		count = fscanf(pFile, " AJADebugStateFileVersion: %d", &intVersion);
		version = intVersion;
		if((count != 1) || (version != AJA_DEBUG_STATE_FILE_VERSION))
		{
            fclose(pFile);
			return AJA_STATUS_FAIL;
		}

		while(true)
		{
			// read groups that have destinations
			count = fscanf(pFile, " GroupDestination: %d : %x", &index, &destination);
			if(count != 2)
			{
                count = fscanf(pFile, " CustomGroupDestination: %d : %x", &index, &destination);
                if(count != 2)
                {
                    break;
                }
			}          

			// index must be in range
			if((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
			{
				continue;
			}

			// update the destination
			spShare->unitArray[index] = destination;
		}
	}
	catch(...)
	{
        fclose(pFile);
		return AJA_STATUS_FAIL;
	}

	fclose(pFile);

	return AJA_STATUS_SUCCESS;
}


int64_t AJADebug::DebugTime (void)
{
    // wrapper around the inlined local version
    return debug_time();
}


std::string AJAStatusToString (const AJAStatus inStatus)
{
	switch (inStatus)
	{
		case AJA_STATUS_SUCCESS:			return "AJA_STATUS_SUCCESS";
		case AJA_STATUS_TRUE:				return "AJA_STATUS_TRUE";
		case AJA_STATUS_UNKNOWN:			return "AJA_STATUS_UNKNOWN";
		case AJA_STATUS_FAIL:				return "AJA_STATUS_FAIL";
		case AJA_STATUS_TIMEOUT:			return "AJA_STATUS_TIMEOUT";
		case AJA_STATUS_RANGE:				return "AJA_STATUS_RANGE";
		case AJA_STATUS_INITIALIZE:			return "AJA_STATUS_INITIALIZE";
		case AJA_STATUS_NULL:				return "AJA_STATUS_NULL";
		case AJA_STATUS_OPEN:				return "AJA_STATUS_OPEN";
		case AJA_STATUS_IO:					return "AJA_STATUS_IO";
		case AJA_STATUS_DISABLED:			return "AJA_STATUS_DISABLED";
		case AJA_STATUS_BUSY:				return "AJA_STATUS_BUSY";
		case AJA_STATUS_BAD_PARAM:			return "AJA_STATUS_BAD_PARAM";
		case AJA_STATUS_FEATURE:			return "AJA_STATUS_FEATURE";
		case AJA_STATUS_UNSUPPORTED:		return "AJA_STATUS_UNSUPPORTED";
		case AJA_STATUS_READONLY:			return "AJA_STATUS_READONLY";
		case AJA_STATUS_WRITEONLY:			return "AJA_STATUS_WRITEONLY";
		case AJA_STATUS_MEMORY:				return "AJA_STATUS_MEMORY";
		case AJA_STATUS_ALIGN:				return "AJA_STATUS_ALIGN";
		case AJA_STATUS_FLUSH:				return "AJA_STATUS_FLUSH";
		case AJA_STATUS_NOINPUT:			return "AJA_STATUS_NOINPUT";
		case AJA_STATUS_SURPRISE_REMOVAL:	return "AJA_STATUS_SURPRISE_REMOVAL";
		case AJA_STATUS_NOT_FOUND:			return "AJA_STATUS_NOT_FOUND";
		case AJA_STATUS_NOBUFFER:			return "AJA_STATUS_NOBUFFER";
		case AJA_STATUS_INVALID_TIME:		return "AJA_STATUS_INVALID_TIME";
		case AJA_STATUS_NOSTREAM:			return "AJA_STATUS_NOSTREAM";
		case AJA_STATUS_TIMEEXPIRED:		return "AJA_STATUS_TIMEEXPIRED";
		case AJA_STATUS_BADBUFFERCOUNT:		return "AJA_STATUS_BADBUFFERCOUNT";
		case AJA_STATUS_BADBUFFERSIZE:		return "AJA_STATUS_BADBUFFERSIZE";
		case AJA_STATUS_STREAMCONFLICT:		return "AJA_STATUS_STREAMCONFLICT";
		case AJA_STATUS_NOTINITIALIZED:		return "AJA_STATUS_NOTINITIALIZED";
		case AJA_STATUS_STREAMRUNNING:		return "AJA_STATUS_STREAMRUNNING";
        case AJA_STATUS_REBOOT:             return "AJA_STATUS_REBOOT";
        case AJA_STATUS_POWER_CYCLE:        return "AJA_STATUS_POWER_CYCLE";
#if !defined(_DEBUG)
        default:	break;
#endif
	}
	return "<bad AJAStatus>";
}


void * AJADebug::GetPrivateDataLoc (void)
{
	if (!sLock.IsValid())
		return NULL;
	AJAAutoLock lock(&sLock);
	return spShare;
}


size_t AJADebug::GetPrivateDataLen (void)
{
	if (!sLock.IsValid())
		return 0;
	AJAAutoLock lock(&sLock);
	return spShare ? sizeof(AJADebugShare) : 0;
}


uint32_t AJADebug::StatsCapacity (void)
{
	if (!spShare)
		return 0;
	return spShare->statCapacity;
}

bool AJADebug::HasStats (void)
{
	return StatsCapacity() ? true : false;
}

AJAStatus AJADebug::StatAllocate (const uint32_t inKey)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (STAT_BIT_TEST)
			return AJA_STATUS_FAIL;
		STAT_BIT_SET;
		AJAAtomic::Increment(&spShare->statAllocChanges);
		return StatReset(inKey);
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatFree (const uint32_t inKey)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		StatReset(inKey);
		STAT_BIT_CLEAR;
		AJAAtomic::Increment(&spShare->statAllocChanges);
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

bool AJADebug::StatIsAllocated (const uint32_t inKey)
{
	if (!spShare)
		return false;
	if (inKey >= spShare->statCapacity)
		return false;
	try
	{
		return IS_STAT_BAD ? false : true;
	}
	catch(...)
	{
		return false;
	}
}

AJAStatus AJADebug::StatReset (const uint32_t inKey)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		AJADebugStat & stat(spShare->stats[inKey]);
		stat.Reset();
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatTimerStart (const uint32_t inKey)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		AJADebugStat & stat(spShare->stats[inKey]);
		stat.Start();
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatTimerStop (const uint32_t inKey)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		AJADebugStat & stat(spShare->stats[inKey]);
		stat.Stop();
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatCounterIncrement (const uint32_t inKey, const uint32_t inIncrement)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		AJADebugStat & stat(spShare->stats[inKey]);
		stat.Increment(inIncrement);
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatSetValue (const uint32_t inKey, const uint32_t inValue)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		AJADebugStat & stat(spShare->stats[inKey]);
		stat.SetValue(inValue);
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatGetInfo (const uint32_t inKey, AJADebugStat & outInfo)
{
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (inKey >= spShare->statCapacity)
		return AJA_STATUS_RANGE;
	try
	{
		if (IS_STAT_BAD)
			return AJA_STATUS_FAIL;
		outInfo = spShare->stats[inKey];
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

AJAStatus AJADebug::StatGetKeys (std::vector<uint32_t> & outKeys, uint32_t & outSeqNum)
{
	outKeys.clear();
	outSeqNum = 0;
	if (!spShare)
		return AJA_STATUS_INITIALIZE;
	if (!spShare->statCapacity)
		return AJA_STATUS_FEATURE;
	try
	{
		for (uint32_t inKey(0);  inKey < spShare->statCapacity;  inKey++)
			if (STAT_BIT_TEST)
				outKeys.push_back(inKey);
		outSeqNum = spShare->statAllocChanges;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}
	return AJA_STATUS_SUCCESS;
}

uint64_t AJADebugStat::Sum (size_t inNum) const
{
	uint64_t result(0);
	if (!inNum  ||  inNum > AJA_DEBUG_STAT_DEQUE_SIZE)
		inNum = AJA_DEBUG_STAT_DEQUE_SIZE;
	for (size_t n(0);  n < inNum;  n++)
		result += fValues[n];
	return result;
}

double AJADebugStat::Average(void) const
{
	if (!fCount)
		return 0.00;
	if (fCount < 2)
		return double(fValues[0]);
	return double(Sum(fCount)) / double(fCount);
}

void AJADebugStat::Start (void)
{
	fLastTimeStamp = AJATime::GetSystemMicroseconds();
}

void AJADebugStat::Stop (void)
{
	SetValue(uint32_t(AJATime::GetSystemMicroseconds() - fLastTimeStamp), /*zero timestamp*/false);
}

void AJADebugStat::Increment (uint32_t inIncrement, const bool inRollOver)
{
	if (inRollOver  ||  fCount != 0xFFFFFFFF)
		while (inIncrement--)
			AJAAtomic::Increment(&fCount);
	fLastTimeStamp = AJATime::GetSystemMicroseconds();
}

void AJADebugStat::Decrement (uint32_t inDecrement, const bool inRollUnder)
{
	if (inRollUnder  ||  fCount != 0xFFFFFFFF)
		while (inDecrement--)
			AJAAtomic::Decrement(&fCount);
	fLastTimeStamp = AJATime::GetSystemMicroseconds();
}

void AJADebugStat::SetValue (const uint32_t inValue, const bool inStamp)
{
	fValues[fCount % AJA_DEBUG_STAT_DEQUE_SIZE] = inValue;
	AJAAtomic::Increment(&fCount);
	if (inValue < fMin)
		fMin = inValue;
	if (inValue > fMax)
		fMax = inValue;
	fLastTimeStamp = inStamp ? AJATime::GetSystemMicroseconds() : 0;
}

std::ostream & operator << (std::ostream & oss, const AJADebugStat & inStat)
{
	oss << inStat.fMin << " (min), " << inStat.Average() << " (avg), " << inStat.fMax << " (max), " << inStat.fCount << " (cnt), " << inStat.fLastTimeStamp;
	return oss;
}
