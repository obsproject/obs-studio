/* SPDX-License-Identifier: MIT */
/**
	@file		debug.h
	@brief		Declares the AJADebug class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_DEBUG_H
#define AJA_DEBUG_H

#include <stdio.h>
#include <sstream>
#include "ajabase/common/public.h"
#include "ajabase/system/debugshare.h"



/** @defgroup AJAGroupMacro AJA Debug Macros 
 *	The macros are used to generate messages and assertions.
 *	@ingroup AJAGroupDebug
 *	@{
 */

/** @def AJA_ASSERT(_expression_)
 *	Assert if _expression_ is false.  
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the assert.
 *
 *  @param[in]	_expression_	Boolean expression that should be true.
 */

/** @def AJA_REPORT(_index_, _severity_, _format_)
 *	Report debug messages to active destinations.  
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *  
 *  @param[in]	_index_		Send message to this destination index.
 *  @param[in]	_severity_	Severity (::AJADebugSeverity) of the message to report.
 *  @param[in]	_format_	Format parameters passed to vsprintf. The first is the format itself.
 */

/** @def AJA_sREPORT(_index_, _severity_, _xpr_)
 *	Report a message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send message to this destination index.
 *  @param[in]	_severity_	Severity (::AJADebugSeverity) of the message to report.
 *  @param[in]	_xpr_		A std::ostream expression (e.g. <tt>"Foo" << std::hex << 3500<//tt>.
 */

/** @def AJA_sEMERGENCY(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Emergency message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Emergency message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sALERT(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Alert message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Alert message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sERROR(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Error message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Error message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sWARNING(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Warning message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Warning message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sNOTICE(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Notice message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Notice message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sINFO(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Info message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Info message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/** @def AJA_sDEBUG(_index_, _xpr_)
 *	Reports a ::AJA_DebugSeverity_Debug message to active destinations using the given std::ostream expression.
 *	@hideinitializer
 *
 *	This macro will provide the file name and line number of the reporting module.
 *
 *  @param[in]	_index_		Send ::AJA_DebugSeverity_Debug message to this destination index.
 *  @param[in]	_xpr_		A std::ostream expression.
 */

/**	@} */

#if defined(AJA_WINDOWS) 

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if (!(_expression_)) AJADebug::AssertWithMessage(__FILE__, __LINE__, #_expression_);
		#define AJA_PRINT(...) \
			AJADebug::Report(0, AJA_DebugSeverity_Debug, NULL, 0, __VA_ARGS__)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

	#define AJA_REPORT(_index_, _severity_, ...) \
		AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#elif defined(AJA_LINUX)

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if(!(_expression_)) AJADebug::AssertWithMessage(NULL, 0, #_expression_);
		#define AJA_PRINT(...) \
			AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, __VA_ARGS__)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

	#define AJA_REPORT(_index_, _severity_, ...) \
		AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#elif defined(AJA_MAC) 

	#if defined(AJA_DEBUG)
	
		#define AJA_ASSERT(_expression_) \
            if (!(_expression_)) AJADebug::AssertWithMessage(__FILE__, __LINE__, #_expression_);
		#if !defined (AJA_PRINTTYPE)
			#define AJA_PRINTTYPE		0
		#endif	//	if AJA_PRINTTYPE undefined
		#if (AJA_PRINTTYPE==0)
            #define AJA_PRINT(...) \
                AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, __VA_ARGS__)
		#elif (AJA_PRINTTYPE==1)
			#include <stdio.h>
			#define AJA_PRINT(_format_...) printf(_format_)
		#elif (AJA_PRINTTYPE==2)
			#include "/System/Library/Frameworks/FireLog.framework/Versions/A/Headers/FireLog.h"
			#define AJA_PRINT(_format_...) FireLog(_format_)
		#elif (AJA_PRINTTYPE==3)
			#include <stdio.h>
			#define AJA_PRINT(_format_...) fprintf(stderr, _format_)
		#elif (AJA_PRINTTYPE==4)
			#include <syslog.h>
			#include <stdarg.h>
			#define AJA_PRINT(_format_...) syslog(LOG_ERR, _format_)
		#endif

	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

    #define AJA_REPORT(_index_, _severity_, ...) \
        AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#else

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if(!(_expression_)) AJADebug::AssertWithMessage(NULL, 0, #_expression_);
		#define AJA_PRINT(_format_,...) \
			AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, _format_)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(_format_,...)
	#endif

	#define AJA_REPORT(_index_, _severity_, _format_) \
		AJADebug::Report(_index_, _severity_, NULL, 0, _format_);

#endif

//	Handy ostream-based macros...
#define	AJA_sASSERT(_expr_)                 do {std::ostringstream	__ss__;  __ss__ << #_expr_;  AJADebug::AssertWithMessage(__FILE__, __LINE__, __ss__.str());} while (false)
#define	AJA_sREPORT(_ndx_,_sev_,_expr_)		do {std::ostringstream	__ss__;  __ss__ << _expr_;  AJADebug::Report((_ndx_), (_sev_), __FILE__, __LINE__, __ss__.str());} while (false)
#define	AJA_sEMERGENCY(_ndx_,_expr_)		AJA_sREPORT((_ndx_), AJA_DebugSeverity_Emergency,	_expr_)
#define	AJA_sALERT(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Alert,		_expr_)
#define	AJA_sERROR(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Error,		_expr_)
#define	AJA_sWARNING(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Warning,		_expr_)
#define	AJA_sNOTICE(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Notice,		_expr_)
#define	AJA_sINFO(_ndx_,_expr_)				AJA_sREPORT((_ndx_), AJA_DebugSeverity_Info,		_expr_)
#define	AJA_sDEBUG(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Debug,		_expr_)

// forward declarations
class AJAMemory;

/** 
 *  @param[in]  inStatus   The AJAStatus value of interest.
 *	@return		A string containing the given AJAStatus value as human-readable text.
 */
AJA_EXPORT std::string AJAStatusToString (const AJAStatus inStatus);


/** 
 *	Debug class to generate debug output and assertions.
 *	@ingroup AJAGroupDebug
 */
class AJA_EXPORT AJADebug
{
public:

	AJADebug()			{}
	virtual ~AJADebug()	{}

    /**
     *	@return		The debug facility's version number.
     */
    static uint32_t Version (void);	//	New in SDK 16.0

    /**
     *	@return		The size of the debug facility's shared memory region.
     */
    static uint32_t TotalBytes (void);	//	New in SDK 16.0

	/** 
	 *	Initialize the debug system.  
	 *
	 *	Invoke before reporting debug messages.
     *
     *  @param[in]  incrementRefCount   If true will increment the ref count in the shared memory,
     *                                  NOTE that this should only be true when used in something that is
     *                                  processing the log messages, like ajalogger.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Debug system initialized
	 *				AJA_STATUS_FAIL		Initialization failed
	 */
    static AJAStatus Open (bool incrementRefCount = false);

	/** 
	 *	Release resources used by the debug system.  
	 *
	 *	Invoke before application terminates. 
	 *
     *  @param[in]  decrementRefCount   If true will decrement the ref count in the shared memory,
     *                                  NOTE that this should only be true when used in something that is
     *                                  processing the log messages, like ajalogger.
     *
	 *	@return		AJA_STATUS_SUCCESS	Debug system closed
	 *				AJA_STATUS_FAIL		Close failed
	 */
    static AJAStatus Close (bool decrementRefCount = false);

	/**
	 *	Enable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this group index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination enabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus Enable (int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Disable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this group index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus Disable (int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Enable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus SetDestination (int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Get the destination associated with a debug group.
	 *
	 *	@param[in]	inGroup						Index of the group destination to return.
	 *	@param[out]	outDestination				Receives the current destination
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus GetDestination (const int32_t inGroup, uint32_t & outDestination);

	/**
	 *	@param[in]	index					The destination index of interest.
     *	@return		True if the debug facility is open, and the destination index is valid and active;  otherwise false.
	 */
    static bool IsActive (int32_t index);

	/**
     *	@return		True if the debug facility is open;  otherwise false.
	 */
    static bool IsOpen (void);

	/** 
     *	@return		True if this class was built with AJA_DEBUG defined;  otherwise false.
	 */
    static bool IsDebugBuild (void);

	/**
	 *	Report debug message to the specified destination index.
	 *
	 *	@param[in]	index		Report the message to this destination index.
	 *	@param[in]	severity	Severity (::AJA_DEBUG_SEVERITY) of the message to report.
	 *	@param[in]	pFileName	The source filename reporting the message.
	 *	@param[in]	lineNumber	The line number in the source file reporting the message.
	 *  @param[in]	...			Format parameters to be passed to vsprintf. The first is the format itself.
	 */
	static void Report (int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, ...);

    /**
     *	Report debug message to the specified destination index.
     *
     *	@param[in]	index		Report the message to this destination index.
     *	@param[in]	severity	Severity (::AJA_DEBUG_SEVERITY) of the message to report.
     *	@param[in]	pFileName	The source filename reporting the message.
     *	@param[in]	lineNumber	The line number in the source file reporting the message.
     *  @param[in]	message		The message to report.
     */
    static void Report (int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, const std::string& message);

	/**
     *	Assert that an unexpected error has occurred.
	 *
	 *	@param[in]	pFileName		The source file name reporting the assertion.
	 *	@param[in]	lineNumber		The line number in the source file reporting the assertion.
	 *  @param[in]	pExpression		Expression that caused the assertion.
	 */
    static void AssertWithMessage (const char* pFileName, int32_t lineNumber, const std::string& pExpression);

    /**
     *	@return		The capacity of the debug logging facility's message ring.
     */
	static uint32_t MessageRingCapacity (void);	//	New in SDK 16.0

    /**
     *	Get the reference count for the number of clients accessing shared debug info
     *
     *	@param[out]	outRefCount                 Receives the current client reference count.
     *	@return		AJA_STATUS_SUCCESS			Reference count returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetClientReferenceCount (int32_t & outRefCount);

    /**
     *	Set the reference count for the number of clients accessing shared debug info
     *  NOTE: in normal circumstances this should never be used, if set to 0 or less
     *        the debug system will be closed (shared memory cleaned up)
     *
     *	@param[in]	refCount                    The client reference count to set.
     *	@return		AJA_STATUS_SUCCESS			Reference count set
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     */
    static AJAStatus SetClientReferenceCount (int32_t refCount);

	/**
	 *	Get the sequence number of the latest message
	 *
	 *	@param[out]	outSequenceNumber			Receives the current sequence number.
	 *	@return		AJA_STATUS_SUCCESS			Sequence number returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetSequenceNumber (uint64_t & outSequenceNumber);

	/**
	 *	Get the sequence number recorded in the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
     *	@param[out]	outSequenceNumber			Receives the sequence number recorded in the message.
	 *	@return		AJA_STATUS_SUCCESS			Group index returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageSequenceNumber (const uint64_t sequenceNumber, uint64_t & outSequenceNumber);

	/**
	 *	Get the group index that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outGroupIndex				Receives the group that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Group index returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageGroup (const uint64_t sequenceNumber, int32_t & outGroupIndex);

	/**
	 *	Get the destination of the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outDestination				Receives the destination of the message.
	 *	@return		AJA_STATUS_SUCCESS			Destination returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageDestination (const uint64_t sequenceNumber, uint32_t & outDestination);

	/**
	 *	Get the time the message was reported.
	 *
	 *	@param[in]	sequenceNumber			Sequence number of the message.
	 *	@param[out]	outTime					Receives the time the message was reported.
	 *	@return		AJA_STATUS_SUCCESS		message time returned
	 *				AJA_STATUS_OPEN			debug system not open
	 *				AJA_STATUS_RANGE		index out of range
	 *				AJA_STATUS_NULL			null output pointer
	 */
    static AJAStatus GetMessageTime (const uint64_t sequenceNumber, uint64_t & outTime);

    /**
     *	Get the wall clock time the message was reported.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	outTime					Receives the wall clock time the message was reported, as returned from time()
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetMessageWallClockTime (const uint64_t sequenceNumber, int64_t & outTime);


	/**
	 *	Get the source file name that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outFileName					Receives the source file name that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Message file name returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageFileName (const uint64_t sequenceNumber, std::string & outFileName);

	/**
	 *	Get the source line number that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outLineNumber				Receives the source line number that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Sequence number returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageLineNumber (const uint64_t sequenceNumber, int32_t & outLineNumber);

	/**
	 *	Get the severity of the reported message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outSeverity					Receives the severity of the reported message.
	 *	@return		AJA_STATUS_SUCCESS			Message severity returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageSeverity (const uint64_t sequenceNumber, int32_t & outSeverity);

	/**
	 *	Get the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	outMessage					Receives the message text
	 *	@return		AJA_STATUS_SUCCESS			Message text returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageText (const uint64_t sequenceNumber, std::string & outMessage);

    /**
     *	Get the Process Id that reported the message.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	outPid                  Process Id that reported the message.
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetProcessId (const uint64_t sequenceNumber, uint64_t & outPid);

    /**
     *	Get the Thread Id that reported the message.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	outTid                  Receives the thread Id that reported the message.
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetThreadId (const uint64_t sequenceNumber, uint64_t & outTid);

    /**
     *	Get the number of messages accepted into the ring since creation.
     *
     *	@param[out]	outCount                    Receives the number of messages
     *	@return		AJA_STATUS_SUCCESS			Number of messages returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetMessagesAccepted (uint64_t & outCount);

    /**
     *	Get the number of messages ignored and not put into the ring since creation.
     *
     *	@param[out]	outCount                    Receives the number of messages
     *	@return		AJA_STATUS_SUCCESS			Number of messages returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetMessagesIgnored (uint64_t & outCount);

	/**
	 *	@param[in]	severity	The Severity of interest.
	 *	@return					A human-readable string containing the name associated with the given Severity value.
	 */
	static const std::string & SeverityName (const int32_t severity);


    /**
     *	@param[in]	group	The Message Group of interest.
     *	@return				A human-readable string containing the name associated with the given Message Group.
     */
    static const std::string & GroupName (const int32_t group);

	/**
	 *	Write group state to a file.
	 *
	 *	@param[in]	pFileName					The group state file name.
	 *	@return		AJA_STATUS_SUCCESS			State saved
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus SaveState (char* pFileName);

	/**
	 *	Read group state from a file.
	 *
	 *	@param[in]	pFileName					The group state file name.
	 *	@return		AJA_STATUS_SUCCESS			State restored
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus RestoreState (char* pFileName);

    /**
     *	@return		The capacity of the debug facility's stats buffer.
     */
	static uint32_t StatsCapacity (void);	//	New in SDK 16.0

	/**
	 *	@return		True if stats are supported; otherwise false.
	 */
	static bool HasStats (void);	//	New in SDK 16.0

	/**
	 *	Registers/adds new stat, prepares for first use.
	 *
	 *	@param[in]	inKey						The timer/counter key to be registered/allocated.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatAllocate (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Unregisters/deletes the given stat.
	 *
	 *	@param[in]	inKey						The timer/counter to be unregistered/deleted.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatFree (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Answers if the given stat is allocated (in use).
	 *
	 *	@param[in]	inKey						The timer/counter of interest.
	 *	@return		True if stat is allocated;  otherwise false.
	 */
	static bool StatIsAllocated (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Resets/clears the given timer/counter stat.
	 *
	 *	@param[in]	inKey						The timer/counter of interest.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatReset (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Starts the given timer stat by recording the host OS' high-resolution clock.
	 *
	 *	@param[in]	inKey						The timer of interest.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatTimerStart (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Stops the given timer stat, storing the elapsed time since the last Start call
	 *	into the timer's deque of elapsed times.
	 *
	 *	@param[in]	inKey						The timer of interest.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatTimerStop (const uint32_t inKey);	//	New in SDK 16.0

	/**
	 *	Increments the given counter stat.
	 *
	 *	@param[in]	inKey						The counter of interest.
	 *	@param[in]	inIncrement					Optionally specifies the increment value. Defaults to 1.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatCounterIncrement (const uint32_t inKey, const uint32_t inIncrement = 1);	//	New in SDK 16.0

	/**
	 *	Records a new value for the stat.
	 *
	 *	@param[in]	inKey						The counter of interest.
	 *	@param[in]	inValue						Specifies the value.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatSetValue (const uint32_t inKey, const uint32_t inValue);	//	New in SDK 16.0

	/**
	 *	Answers with the given stat's info.
	 *
	 *	@param[in]	inKey						The timer/counter of interest.
	 *	@param[out]	outInfo						Receives the timer/counter info.
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatGetInfo (const uint32_t inKey, AJADebugStat & outInfo);	//	New in SDK 16.0

	/**
	 *	Answers with the given stat's info.
	 *
	 *	@param[out]	outKeys						Receives the list of allocated stats.
	 *	@param[out]	outSeqNum					Receives the list change sequence number (to detect if the list changed).
	 *	@return		AJA_STATUS_SUCCESS if successful.
	 */
	static AJAStatus StatGetKeys (std::vector<uint32_t> & outKeys, uint32_t & outSeqNum);	//	New in SDK 16.0

	/**
	 *	Get the current time at the debug rate.
	 *
	 *	@return		The current time in debug ticks.
	 */
	static int64_t DebugTime (void);

	//	Old APIs
	static const char* GetSeverityString (int32_t severity);
    static const char* GetGroupString (int32_t group);
	static AJAStatus GetDestination (int32_t index, uint32_t* pDestination)	{return pDestination ? GetDestination(index, *pDestination) : AJA_STATUS_NULL;}
    static AJAStatus GetClientReferenceCount (int32_t* pRefCount)			{return pRefCount ? GetClientReferenceCount(*pRefCount) : AJA_STATUS_NULL;}
    static AJAStatus GetSequenceNumber (uint64_t* pSequenceNumber)			{return pSequenceNumber ? GetSequenceNumber(*pSequenceNumber) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageSequenceNumber (uint64_t sequenceNumber, uint64_t *pSequenceNumber)	{return pSequenceNumber ? GetMessageSequenceNumber(sequenceNumber, *pSequenceNumber) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageGroup (uint64_t sequenceNumber, int32_t* pGroupIndex)				{return pGroupIndex ? GetMessageGroup(sequenceNumber, *pGroupIndex) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageDestination (uint64_t sequenceNumber, uint32_t* pDestination)		{return pDestination ? GetMessageDestination(sequenceNumber, *pDestination) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageTime (uint64_t sequenceNumber, uint64_t* pTime)						{return pTime ? GetMessageTime(sequenceNumber, *pTime) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageWallClockTime (uint64_t sequenceNumber, int64_t *pTime)				{return pTime ? GetMessageWallClockTime(sequenceNumber, *pTime) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageFileName (uint64_t sequenceNumber, const char** ppFileName);
    static AJAStatus GetMessageLineNumber (uint64_t sequenceNumber, int32_t* pLineNumber)			{return pLineNumber ? GetMessageLineNumber(sequenceNumber, *pLineNumber) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageSeverity (uint64_t sequenceNumber, int32_t* pSeverity)				{return pSeverity ? GetMessageSeverity(sequenceNumber, *pSeverity) : AJA_STATUS_NULL;}
    static AJAStatus GetMessageText (uint64_t sequenceNumber, const char** ppMessage);
    static AJAStatus GetProcessId (uint64_t sequenceNumber, uint64_t* pPid)							{return pPid ? GetProcessId(sequenceNumber, *pPid) : AJA_STATUS_NULL;}
    static AJAStatus GetThreadId (uint64_t sequenceNumber, uint64_t* pTid)							{return pTid ? GetThreadId(sequenceNumber, *pTid) : AJA_STATUS_NULL;}
    static AJAStatus GetMessagesAccepted (uint64_t* pCount)					{return pCount ? GetMessagesAccepted(*pCount) : AJA_STATUS_NULL;}
    static AJAStatus GetMessagesIgnored (uint64_t* pCount)					{return pCount ? GetMessagesIgnored(*pCount) : AJA_STATUS_NULL;}

	//	Other APIs
	static void * GetPrivateDataLoc (void);
	static size_t GetPrivateDataLen (void);

};	//	AJADebug

std::ostream & operator << (std::ostream & oss, const AJADebugStat & inStat);

#endif	//	AJA_DEBUG_H
