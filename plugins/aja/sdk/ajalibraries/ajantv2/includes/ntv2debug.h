/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2debug.cpp
	@brief		Declares the NTV2 debug output functions, including 'odprintf'.
	@note		This module should remain straight ANSI 'C' -- no C++ or STL.
	@copyright	2004-2021 AJA Video Systems, Inc. All rights reserved.
**/

#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"

#if !defined(NTV2_DEPRECATE_14_3)
AJAExport const char *	NTV2DeviceTypeString		(const NTV2DeviceType type);
#endif	//	!defined(NTV2_DEPRECATE_14_3)
AJAExport const char *	NTV2DeviceIDString			(const NTV2DeviceID id);
AJAExport const char *	NTV2DeviceString			(const NTV2DeviceID id);
AJAExport const char *	NTV2StandardString			(NTV2Standard std);
AJAExport const char *	NTV2FrameBufferFormatString	(NTV2FrameBufferFormat fmt);
AJAExport const char *	NTV2FrameGeometryString		(NTV2FrameGeometry geom);
AJAExport const char *	NTV2FrameRateString			(NTV2FrameRate rate);
AJAExport const char *	NTV2VideoFormatString		(NTV2VideoFormat fmt);
AJAExport const char *	NTV2RegisterNameString		(const ULWord inRegNum);
AJAExport const char *	NTV2InterruptEnumString		(const unsigned inInterruptEnum);
#if !defined (NTV2_DEPRECATE)
	AJAExport const char *	NTV2BoardTypeString		(NTV2BoardType type);
	AJAExport const char *	NTV2BoardIDString		(NTV2BoardID id);
#endif	//	!defined (NTV2_DEPRECATE)

// indexed by RegisterNum - 2048
extern AJAExport const char *	ntv2RegStrings_SDI_RX_Status [];

// indexed by RegisterNum
extern AJAExport const char *	ntv2RegStrings [];

#ifdef MSWindows
	AJAExport void __cdecl odprintf(const char *format, ...);
	#ifndef vcout
		#define vcout_dummy_1(x)	#x
		#define vcout_dummy_2(x)	vcout_dummy_1(x)
		#define vcout(desc) message(__FILE__ "(" vcout_dummy_2( __LINE__ )  ") : " desc)
	#endif	// vcout
#endif

#if defined(FS1) || defined(AJALinux) || defined(AJAMac)
	#ifndef odprintf
		#define odprintf printf
	#endif
#endif
