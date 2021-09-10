/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2audiodefines.h
	@brief		Declares common audio macros and structs used in the SDK.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef NTV2_AUDIODEFINES_H
#define NTV2_AUDIODEFINES_H

#define NTV2_NUMAUDIO_CHANNELS              6
#define NTV2_AUDIOSAMPLESIZE                (sizeof (ULWord))
#define NTV2_AUDIO_WRAPADDRESS               0x000FF000
#define NTV2_AUDIO_WRAPADDRESS_MEDIUM       (0x000FF000 * 2)
#define NTV2_AUDIO_WRAPADDRESS_BIG          (0x000FF000 * 4)
#define NTV2_AUDIO_WRAPADDRESS_BIGGER       (0x000FF000 * 8)		// used with KiPro Mini 99 video buffers
#define NTV2_AUDIO_READBUFFEROFFSET          0x00100000
#define NTV2_AUDIO_READBUFFEROFFSET_MEDIUM  (0x00100000 * 2)
#define NTV2_AUDIO_READBUFFEROFFSET_BIG     (0x00100000 * 4)
#define NTV2_AUDIO_READBUFFEROFFSET_BIGGER  (0x00100000 * 8)		// used with KiPro Mini 99 video buffers

#define NTV2_AUDIO_BUFFEROFFSET_BIG			(0x00100000 * 8)

#define NTV2_NUMSAMPLES_PER_AUDIO_INTERRUPT 960
#define NTV2_TOTALSAMPLES_IN_BUFFER(numChannels) (NTV2_AUDIO_WRAPADDRESS/((numChannels)*NTV2_AUDIOSAMPLESIZE)) 
 
#define kSDIName	"SDI"
#define kAESName	"AES/EBU"
#define kADATName   "ADAT - 8 channels"
#define kAnalogName "Analog - 4 channels"
#define kNoneName   "IO - none"
#define kAllName	"8 channels"

#define kAJADeviceManufacturer	"AJA Video"	


typedef enum
{
	kNumAudioChannels2		= 2,
	kNumAudioChannels4		= 4,
	kNumAudioChannels6		= 6,
	kNumAudioChannels8		= 8,
	kNumAudioChannels16		= 16,
	kNumAudioChannelsMax	= kNumAudioChannels16	// Used in Linux and Windows too
} AudioChannelsPerFrameEnum;


typedef enum
{
	k16bitsPerSample	= 16,
	k24bitsPerSample	= 24,
	k32bitsPerSample	= 32
} AudioBitsPerSampleEnum;


typedef enum
{
	k44p1KHzSampleRate	= 44100,
	k48KHzSampleRate	= 48000,
	k96KHzSampleRate	= 96000
} AudioSampleRateEnum;


typedef enum
{
	kSourceSDI		= 0x69736469,
	kSourceAES		= 0x69616573,
	kSourceADAT		= 0x69616474,
	kSourceAnalog	= 0x69616C67,
	kSourceNone		= 0x6E6F696E,
	kSourceAll		= 0x6F757420
} AudioSourceEnum;

typedef enum { 
	kNormal = 0,
	kMuted = 1
} AudioMuteEnum;

// These are ProIO specific audio settings.  The ENUMs represent actual register 
// values.  This is probably not a great idea but for now we will leave it like this
// since it only pertains to a ProIO build.  Ideally these should just enumerations 
// and the settings layer in the muxer should translate these to what the hardware
// wants.

#endif	//	NTV2_AUDIODEFINES_H
