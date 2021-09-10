/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2debugMacros.h
	@brief		Declares several macros useful for debugging.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef DEBUGMACROS_H
#define DEBUGMACROS_H



// Macros common to all platforms

		// Decompose a UInt32 (or Mac "OSType") into 4 ASCII characters
		// Example:  printf ("Type = %c%c%c%c\n", Make4CC(myType) );
//#define Make4CC(my4CC) ((char*)(&my4CC))[0], ((char*)(&my4CC))[1], ((char*)(&my4CC))[2], ((char*)(&my4CC))[3]

		// Advanced form: if the 'type' is a numeric value (i.e. 0 - 63), print as a decimal number.
		// Otherwise print as 4 characters.
#ifndef Make4CC
	#if TARGET_OS_WIN32 || TARGET_CPU_X86
		#define Make4CC(my4CC)  ((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[3]), \
								((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[2]), \
								((my4CC < 0x40) ? ('0' + (char)(my4CC / 10)) : ((char*)(&my4CC))[1]), \
								((my4CC < 0x40) ? ('0' + (char)(my4CC % 10)) : ((char*)(&my4CC))[0])
	#else
		#define Make4CC(my4CC)  ((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[0]), \
								((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[1]), \
								((my4CC < 0x40) ? ('0' + (char)(my4CC / 10)) : ((char*)(&my4CC))[2]), \
								((my4CC < 0x40) ? ('0' + (char)(my4CC % 10)) : ((char*)(&my4CC))[3])
	#endif
#endif


// Ping selectors
#define kPingOff				0

	// Muxer
#define kPingMuxerVBI			1
#define kPingAcThread			2
#define kPingDma				3
#define kPingMuxerAudioVox		4
#define kPingMuxerVideoVox		5

	// VideoOut
#define kPingFrameNotification  6
#define kPingCopyDrpToFrame		7
#define kPingEchoPortOut		8
#define kPingQTCallback			9

#define kPingAVSync				10

	// Transfer Codec
#define kPingXfrBandDecomp		11


	// VDig
#define kPingVDigFrameNotify	12
#define kPingVDigCompressOne	13
#define kPingVDigCompressDone   14

	// Use these for quickie tests
#define kPingTest1				15
#define kPingTest2				16
#define kPingTest3				17
#define kPingTest4				18



// Platform-specific macros

#ifdef AJAMac

	
		// For simple cases of measuring durations using the Mac "Microseconds" timer
		//
		// This assumes a pair of matching calls: "StartUSTimer(foo)" and "EndUSTimer(foo)", both within the
		// same scope (which rules out starting in one routine and ending in another - for that you have to
		// find global memory to hold the "start time"). Typical use is as follows:
		//
		//	TimerStart(foo)
		//		(code happens)
		//	TimerStop(foo)
		//
		//	and you'll get a printf that looks like:
		//		foo duration = xxx usecs
		//
		//	Note: no terminating ";"   Also, the variable passes to matching TimerStart() and TimerStop() calls must match.
		//	You may, however, use different variables in overlapping calls, such as:
		//
		//	TimerStart(foo)
		//		(code happens)
		//	TimerStart(bar)
		//		(more code happens)
		//	TimerStop(foo)
		//		(yet more...)
		//	TimerStop(bar)
		//
		//  You may use TimerLimit() instead of TimerStop() to specify a "limit" duration: the printf will only happen
		//  if the measured duration EXCEEDS the specified limit.
		
	#if (DEBUG)	
			// normal printf's
		#define TimerStart(a) UInt64 _startTime_##a, _endTime_##a;							\
								int _delta_##a;												\
								Microseconds((UnsignedWide*)&_startTime_##a);
								
		#define TimerStop(a) Microseconds((UnsignedWide*)&_endTime_##a);					\
							  _delta_##a = (int)U64Subtract(_endTime_##a, _startTime_##a);	\
							  printf (#a " duration = %d usecs\n", _delta_##a);

		#define TimerLimit(a,max) Microseconds((UnsignedWide*)&_endTime_##a);				\
							  _delta_##a = (int)U64Subtract(_endTime_##a, _startTime_##a);	\
							  if (_delta_##a >= max)										\
								printf (#a " duration = %d usecs\n", _delta_##a);

			// uses "dprintf" - which is assumed to be locally #define'd for LogProducer LogMsg
		#define LogTimerStart(a) UInt64 _startTime_##a, _endTime_##a;						\
								int _delta_##a;												\
								Microseconds((UnsignedWide*)&_startTime_##a);
								
		#define LogTimerStop(a) Microseconds((UnsignedWide*)&_endTime_##a);						\
							    _delta_##a = (int)U64Subtract(_endTime_##a, _startTime_##a);	\
							    dprintf (LOG_DEBUG, #a " duration = %d usecs\n", _delta_##a);
				
			// Mark time between successive function calls
		#define MarkTimeDelta(a)	static UInt64 _savedTime_##a = 0; UInt64 _currTime_##a; int _delta_##a;	\
									Microseconds((UnsignedWide*)&_currTime_##a);							\
									_delta_##a = (int)U64Subtract(_currTime_##a, _savedTime_##a);			\
									printf (#a " duration = %d usecs\n", _delta_##a);						\
									_savedTime_##a = _currTime_##a;

	#else
			// disable for deployment builds
		#define TimerStart(a)
		#define TimerStop(a)
		#define TimerLimit(a,b)
        
		#define LogTimerStart(a)
		#define LogTimerStop(a)
		
		#define MarkTimeDelta(a)
	#endif


		// Sleazy (but quick) ways to tell if a Mac keyboard key is pressed
	#if (DEBUG)	
		#define shiftIsDown		( (GetCurrentKeyModifiers() & shiftKey)   != 0 )
		#define capsLockIsDown	( (GetCurrentKeyModifiers() & alphaLock)  != 0 )
		#define optionIsDown	( (GetCurrentKeyModifiers() & optionKey)  != 0 )
		#define controlIsDown	( (GetCurrentKeyModifiers() & controlKey) != 0 )			
		#define commandIsDown	( (GetCurrentKeyModifiers() & cmdKey)     != 0 )			
	#else	// !DEBUG
		#define shiftIsDown		false 
		#define capsLockIsDown	false
		#define optionIsDown	false
		#define controlIsDown	false
		#define commandIsDown	false
	#endif


#else   // !AJAMac


	#define TimerStart(a)
	#define TimerStop(a)

	#define shiftIsDown		false 
	#define capsLockIsDown	false
	#define optionIsDown	false
	#define controlIsDown	false
	#define commandIsDown	false

	#if DEBUG
		#define printf debugPrintf
		void debugPrintf (char *format, ...);
	#else
		#define printf dummyPrintf
		void dummyPrintf (char *format, ...);	// see SoftwareCodec.cpp
	#endif
	
#endif


#endif  // DEBUGMACROS_H
