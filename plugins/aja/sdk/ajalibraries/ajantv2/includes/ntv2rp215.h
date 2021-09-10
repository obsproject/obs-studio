/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2rp215.h
	@brief		Declares the CNTV2RP215Decoder class. See SMPTE RP215 standard for details.
	@copyright	(C) 2006-2021 AJA Video Systems, Inc.
**/

#ifndef __NTV2_RP215_
#define __NTV2_RP215_


#include "ajatypes.h"
#ifdef MSWindows

#include "stdio.h"
#define nil NULL
#endif
#ifdef AJALinux
#define nil NULL
#endif

#include "ajatypes.h"
#include "ntv2enums.h"


#define RP215_PAYLOADSIZE 215  


class CNTV2RP215Decoder
{
public:
     CNTV2RP215Decoder(ULWord* pFrameBufferBaseAddress,NTV2VideoFormat videoFormat,NTV2FrameBufferFormat fbFormat);
    ~CNTV2RP215Decoder();

	bool Locate();
	bool Extract();
	
private:
	ULWord*					_frameBufferBasePointer;
	NTV2VideoFormat			_videoFormat;
	NTV2FrameBufferFormat	_fbFormat;
	Word					_lineNumber;
	Word					_pixelNumber;

	UByte _rp215RawBuffer[RP215_PAYLOADSIZE];

};
#if 0
bool TestQTMovieDecode(char* fileName);
#endif
#endif	// __NTV2_RP215_
