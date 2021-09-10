/* SPDX-License-Identifier: MIT */
/**
	@file		log.cpp
	@brief		Implements the AJATimeLog class.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

//#include "ajabase/system/systemtime.h"
#include "ajabase/system/log.h"
#include "ajabase/system/systemtime.h"

bool AJALog::bInitialized = false;

#if defined(AJA_WINDOWS)
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

void __cdecl log_odprintf(const char *format, ...)
{
	char	buf[4096], *p = buf;
	va_list	args;

	va_start(args, format);
	p += _vsnprintf_s(buf, sizeof buf - 1, format, args);
	va_end(args);

	while ( p > buf  &&  isspace(p[-1]) )
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	::OutputDebugStringA(buf);
}
#endif

//---------------------------------------------------------------------------------------------------------------------
//  class AJALog
//---------------------------------------------------------------------------------------------------------------------

// singleton initialization here
AJALog::AJALog()
{
    if (bInitialized == false)
    {
        bInitialized = true;
        
        #if defined(AJA_MAC)

            #if (AJA_LOGTYPE==2)
                AJADebug::Open();

            #endif
        
        #elif defined(AJA_LINUX)
            
            // one-time initialization here as needed
        
        #elif defined(AJA_WINDOWS)
		
			#if (AJA_LOGTYPE==2)
				AJADebug::Open();
			#endif
		
            // one-time initialization here as needed
       
        #endif
    }
}

// perform singleton release here
AJALog::~AJALog()
{
}


//---------------------------------------------------------------------------------------------------------------------
// MARK: - AJARunAverage
//---------------------------------------------------------------------------------------------------------------------

void AJARunAverage::Mark(int64_t val)
{
	uint64_t index = _samplesTotal++ % _sampleSize;
	_samples[index] = val;
}

int64_t AJARunAverage::MarkAverage(int64_t val)
{
	Mark(val);
	return Average();
}

int64_t AJARunAverage::LastValue()
{
	if (_samplesTotal == 0)
		return -1;
	uint64_t lastIndex = ((_samplesTotal-1) % _sampleSize);
	return _samples[lastIndex];
}

int64_t AJARunAverage::Average()
{
	uint64_t sampleSize = _samplesTotal < _sampleSize ? _samplesTotal : _sampleSize;
	if (sampleSize == 0)
		return 0;
	
	int64_t average = 0;
	for (uint64_t i=0; i <sampleSize; i++)
		average += _samples[i];
		
	average = average / sampleSize;
	return average;
}

void AJARunAverage::Reset()
{
	_samplesTotal = 0; 
	std::fill(_samples.begin(), _samples.end(), 0);
}

void AJARunAverage::Resize(uint64_t sampleSize)
{
	_sampleSize = sampleSize;
	_samples.resize(sampleSize);
	Reset();
}



//---------------------------------------------------------------------------------------------------------------------
// MARK: - AJARunTimeAverage
// calculates a running average of time deltas
//---------------------------------------------------------------------------------------------------------------------

AJARunTimeAverage::AJARunTimeAverage(int sampleSize) : AJARunAverage(sampleSize)
{
	ResetTime();
}

void AJARunTimeAverage::Resize(uint64_t sampleSize)
{
	AJARunAverage::Resize(sampleSize);
}

void AJARunTimeAverage::Reset()
{
	AJARunAverage::Reset();
	ResetTime();
}

void AJARunTimeAverage::ResetTime()
{
	_lastTime = (int64_t)AJATime::GetSystemMicroseconds();
}

// mark current delta-time
// return delta-time
int64_t AJARunTimeAverage::MarkDeltaTime()
{
    int64_t currTime = (int64_t) AJATime::GetSystemMicroseconds();
	int64_t deltaTime = currTime - _lastTime;
	_lastTime = currTime;
	
	Mark(deltaTime);
	return deltaTime;
}

// mark current delta-time
// return running average delta-time
int64_t AJARunTimeAverage::MarkDeltaAverage()
{
    int64_t currTime = (int64_t) AJATime::GetSystemMicroseconds();
	int64_t deltaTime = currTime - _lastTime;
	_lastTime = currTime;
	
	Mark(deltaTime);
	return Average();
}


//---------------------------------------------------------------------------------------------------------------------
//  MARK: - class AJATimeLog
//---------------------------------------------------------------------------------------------------------------------

AJATimeLog::AJATimeLog()
{
    _tag = "";
    _unit = AJA_DebugUnit_Critical;
    Reset();
}

AJATimeLog::AJATimeLog(const char* tag, int unit)
{	
	_unit = unit;
	_tag = tag;
    Reset();
}

AJATimeLog::AJATimeLog(const std::string& tag, int unit)
{	
	_unit = unit;
	_tag = tag;
    Reset();
}

AJATimeLog::~AJATimeLog()
{
}

// reset time
void AJATimeLog::Reset()
{
    _time = AJATime::GetSystemMicroseconds();
}

// reset time
void AJATimeLog::PrintReset()
{
	Reset();
	PrintValue(0, "(** Reset **)");
}

// print dela time in micro seconds
void AJATimeLog::PrintDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    PrintValue(currTime-_time);
    if (bReset)
        _time = currTime;
}


// print dela time in micro seconds
int32_t AJATimeLog::GetDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
	int32_t delta = int32_t(currTime - _time);
	if (bReset)
		_time = currTime;
	return delta;
}

// print delta time in micro seconds, use additional tag
void AJATimeLog::PrintDelta(const char* addedTag, bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    PrintValue(currTime-_time, addedTag);
    if (bReset)
        _time = currTime;
}

void AJATimeLog::PrintDelta(uint64_t threashold, const char* addedTag, bool bReset)
{
	uint64_t currTime = AJATime::GetSystemMicroseconds();
	if ((currTime-_time) > threashold)
		PrintValue(currTime-_time, addedTag);
	if (bReset)
		_time = currTime;
}



void AJATimeLog::PrintValue(int64_t val)
{
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		if (_unit == AJA_DebugUnit_Critical)
			AJA_LOG("%s = %lld\n", _tag.c_str(), val);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, "%s = %lld", _tag.c_str(), val);
	#endif
}


void AJATimeLog::PrintValue(int64_t val, const char* addedTag)
{
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		if (_unit == AJA_DebugUnit_Critical)
			AJA_LOG("%s-%s = %lld\n", _tag.c_str(), addedTag, val);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, "%s-%s = %lld", _tag.c_str(), addedTag, val);
	#endif
}

void AJATimeLog::Print(const char* str)
{
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		if (_unit == AJA_DebugUnit_Critical)
			AJA_LOG("%s-%s\n", _tag.c_str(), str);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, "%s-%s", _tag.c_str(), str);
	#endif
}
