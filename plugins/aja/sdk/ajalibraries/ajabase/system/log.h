/* SPDX-License-Identifier: MIT */
/**
	@file		log.h
	@brief		Declares the AJATimeLog class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_LOG_H
#define AJA_LOG_H

#include <stdio.h>
#include "ajabase/common/public.h"
#include "debug.h"

#if defined(AJA_WINDOWS)
extern void __cdecl log_odprintf(const char *format, ...);
#elif defined (AJA_MAC)
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

// use this the select alternate platform specific loggers, 0 is used for no-log
#ifndef AJA_LOGTYPE
#define AJA_LOGTYPE    2
#endif

// define AJA_LOG here

#if defined(AJA_WINDOWS) 

	#if defined(AJA_DEBUG)

		#if (AJA_LOGTYPE==0)
			// no log
			#define AJA_LOG(...)
			#define AJA_ULOG(_unit_,...)

		#elif (AJA_LOGTYPE==1)
			#define AJA_LOG(...) log_odprintf(__VA_ARGS__)
			#define AJA_ULOG(_unit_,...)	\
				do {if (AJADebug::IsActive(_unit_)) log_odprintf(__VA_ARGS__);} while(0);

		#elif (AJA_LOGTYPE==2)
			#define AJA_LOG(...) AJA_REPORT(AJA_DebugUnit_Critical, AJA_DebugSeverity_Info, __VA_ARGS__)
			#define AJA_ULOG(_unit_,...)	\
				do {if (AJADebug::IsActive(_unit_)) AJA_REPORT(_unit_, AJA_DebugSeverity_Info, __VA_ARGS__);} while(0);

        #else
			//catch all, so builds won't break
			#define AJA_LOG(...)
			#define AJA_ULOG(_unit_,...)
		#endif

	#else
		// no log
		#define AJA_LOG(...)
		#define AJA_ULOG(_unit_,...)
	#endif

#elif defined(AJA_LINUX) || defined(AJA_MAC)

	#if defined(AJA_DEBUG)

		#if (AJA_LOGTYPE==0)
            // no log
			#define AJA_LOG(_format_...)
			#define AJA_ULOG(_unit_, _format_...)
        
        #elif (AJA_LOGTYPE==1)
            // printf
			#include <stdio.h>
			#define AJA_LOG(_format_...) printf(_format_)
			#define AJA_ULOG(_unit_, _format_...)	\
				do {if (AJADebug::IsActive(_unit_)) printf(_format_);} while(0);

        #elif (AJA_LOGTYPE==2)
            #define AJA_LOG(_format_...) AJA_REPORT(AJA_DebugUnit_Critical, AJA_DebugSeverity_Info, _format_)
			#define AJA_ULOG(_unit_, _format_...)	\
				do {if (AJADebug::IsActive(_unit_)) AJA_REPORT(_unit_, AJA_DebugSeverity_Info, _format_);} while(0);
		
		#endif
			
	#else
        // no log
        #define AJA_LOG(_format_...)
		#define AJA_ULOG(_unit_, _format_...)
	#endif

#endif

#ifndef Make4CC
#define Make4CC(my4CC)  ((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[3]), \
						((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[2]), \
						((my4CC < 0x40) ? ('0' + (char)(my4CC / 10)) : ((char*)(&my4CC))[1]), \
						((my4CC < 0x40) ? ('0' + (char)(my4CC % 10)) : ((char*)(&my4CC))[0])
#endif

/** 
 *	Supports auto initialization of logger, if needed
 *	@ingroup AJAGroupDebug
 */
class AJA_EXPORT AJALog
{
public:

    /**
	 *	Singleton initialization of logging service.
	 */
	AJALog();
    
    /**
	 *	Singleton release of logging service.
	 */
	virtual ~AJALog();
    
private:
    static bool bInitialized;
};


extern AJALog gLogInit;


/**
 *	Used to generate timer logs.
 *	@ingroup AJAGroupDebug
 */

#define TAG_SIZE    64



//---------------------------------------------------------------------------------------------------------------------
// class AJARunAverage
// calculates a running average of input values
//---------------------------------------------------------------------------------------------------------------------
class AJA_EXPORT AJARunAverage
{
protected:
	explicit AJARunAverage() {}
	uint64_t _samplesTotal;
	uint64_t _sampleSize;
	std::vector<int64_t> _samples;
	
public:
	AJARunAverage(uint64_t sampleSize)
		{ Resize(sampleSize); }
	virtual ~AJARunAverage() 
		{}
		
	virtual void Resize(uint64_t sampleSize);
	virtual void Reset();
	
	void Mark(int64_t val);
	int64_t LastValue();
	int64_t MarkAverage(int64_t val);
	int64_t Average();
	uint64_t Total()	 	{ return _samplesTotal; } 
	uint64_t SampleSize() 	{ return _sampleSize; }
};


//---------------------------------------------------------------------------------------------------------------------
// class AJARunTimeAverage
// calculates a running average of time deltas
//---------------------------------------------------------------------------------------------------------------------
class AJA_EXPORT AJARunTimeAverage : public AJARunAverage
{
protected:
	explicit AJARunTimeAverage() {}
	int64_t _lastTime;
	
public:
	AJARunTimeAverage(int sampleSize);
	virtual ~AJARunTimeAverage() 
		{}

	virtual void Resize(uint64_t sampleSize);
	virtual void Reset();
	virtual void ResetTime();

	int64_t MarkDeltaTime();
	int64_t MarkDeltaAverage();
};


//---------------------------------------------------------------------------------------------------------------------
//  class AJARunAverage
//	caculates timelogs
//---------------------------------------------------------------------------------------------------------------------
class AJA_EXPORT AJATimeLog
{
public:
	AJATimeLog();
	AJATimeLog(const char* tag, int unit=AJA_DebugUnit_Critical);
	AJATimeLog(const std::string& tag, int unit=AJA_DebugUnit_Critical);
	virtual ~AJATimeLog();
    
    /**
	 *	reset timer.
	 */
	void Reset();
	
    /**
	 *	Print does reset along with print
	 */
    void PrintReset();
	inline void PrintResetIf(bool bEnable=true)
		{ if (bEnable) PrintReset(); }
	
    /**
	 *	Print tag, appended tag, and delta-time since last reset.
	 *  @param[in]	bReset          true if time is reset after print
	 */
    void PrintDelta(bool bReset=true);
    void PrintDelta(const char* addedTag, bool bReset=true);
	void PrintDelta(uint64_t threashold, const char* addedTag, bool bReset);

    inline void PrintDelta(const std::string& addedTag, bool bReset=true)
		{ PrintDelta(addedTag.c_str(), bReset); }
	
    /**
	 *	Optional print tag, appended tag, and delta-time since last reset.
	 *  @param[in]	bEnable         true to print, false inhibits printing
	 *  @param[in]	bReset          true if time is reset after print
	 */
	inline void PrintDeltaIf(bool bEnable, bool bReset=true)
		{ if (bEnable) PrintDelta(bReset); }
    inline void PrintDeltaIf(bool bEnable, const char* addedTag, bool bReset=true)
		{ if (bEnable) PrintDelta(addedTag, bReset); }
    inline void PrintDeltaIf(bool bEnable, const std::string& addedTag, bool bReset=true)
		{ PrintDeltaIf(bEnable, addedTag.c_str(), bReset); }
		
    /**
	 *	Get delta-time since last reset.
	 *  @param[in]	bReset          true if time is reset after get
	 */
    int32_t GetDelta(bool bReset=true);
	
    /**
	 *	Optional print tag, appended tag, and delta-time since last reset.
	 *  @param[in]	val        		value to print
	 */
	void PrintValue(int64_t val);
	void PrintValue(int64_t val, const char* addedTag);
	inline void PrintValue(int64_t val, const std::string& addedTag)
		{ PrintValue(val, addedTag.c_str()); }
	
	void Print(const char* str);
	void Print(const std::string& str)
		{ Print(str.c_str()); }
		
	inline int GetUnit() 
		{ return _unit; }
	inline void SetUnit(int unit)
		{ _unit = unit; }
		
	inline std::string GetTag() 
		{return _tag;}
	inline void SetTag(const char* tag)
		{ _tag = tag; }
	
protected:
    std::string 	_tag;
    int         	_unit;
    uint64_t    	_time;
};


#endif	//	AJA_LOG_H
