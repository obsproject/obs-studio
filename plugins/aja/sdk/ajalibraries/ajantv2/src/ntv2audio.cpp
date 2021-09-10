/* SPDX-License-Identifier: MIT */
/**
	@file	ntv2audio.cpp
	@brief	Implementations of audio-centric CNTV2Card methods.
	@copyright	(C) 2004-2021 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#ifdef AJALinux
	#include "ntv2linuxpublicinterface.h"
	#include <math.h>
#endif	//	AJALinux

#ifdef MSWindows
	#include <math.h>
	#pragma warning(disable: 4800)
#endif	//	MSWindows

using namespace std;

static const ULWord	gChannelToSDIOutControlRegNum []	= {	kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
															kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control, 0};

#if !defined (NTV2_DEPRECATE)
static const ULWord	gChannelToAudioControlRegNum []		= {	kRegAud1Control,		kRegAud2Control,		kRegAud3Control,		kRegAud4Control,
															kRegAud5Control,		kRegAud6Control,		kRegAud7Control,		kRegAud8Control,		0};
#endif	//	!defined (NTV2_DEPRECATE)

static const ULWord gAudioSystemToSrcSelectRegNum []	= {	kRegAud1SourceSelect,	kRegAud2SourceSelect,	kRegAud3SourceSelect,	kRegAud4SourceSelect,
															kRegAud5SourceSelect,	kRegAud6SourceSelect,	kRegAud7SourceSelect,	kRegAud8SourceSelect,	0};

static const ULWord	gChannelToAudioInLastAddrRegNum []	= {	kRegAud1InputLastAddr,	kRegAud2InputLastAddr,	kRegAud3InputLastAddr,	kRegAud4InputLastAddr,
															kRegAud5InputLastAddr,	kRegAud6InputLastAddr,	kRegAud7InputLastAddr,	kRegAud8InputLastAddr,	0};

static const ULWord	gChannelToAudioOutLastAddrRegNum []	= {	kRegAud1OutputLastAddr,	kRegAud2OutputLastAddr,	kRegAud3OutputLastAddr,	kRegAud4OutputLastAddr,
															kRegAud5OutputLastAddr,	kRegAud6OutputLastAddr,	kRegAud7OutputLastAddr,	kRegAud8OutputLastAddr,	0};

static const ULWord	gAudioPlayCaptureModeMasks []		= {	kRegMaskAud1PlayCapMode,	kRegMaskAud2PlayCapMode,	kRegMaskAud3PlayCapMode,	kRegMaskAud4PlayCapMode,
															kRegMaskAud5PlayCapMode,	kRegMaskAud6PlayCapMode,	kRegMaskAud7PlayCapMode,	kRegMaskAud8PlayCapMode,	0};

static const ULWord	gAudioPlayCaptureModeShifts []		= {	kRegShiftAud1PlayCapMode,	kRegShiftAud2PlayCapMode,	kRegShiftAud3PlayCapMode,	kRegShiftAud4PlayCapMode,
															kRegShiftAud5PlayCapMode,	kRegShiftAud6PlayCapMode,	kRegShiftAud7PlayCapMode,	kRegShiftAud8PlayCapMode,	0};

static const ULWord	gAudioDelayRegisterNumbers []		= {	kRegAud1Delay,	kRegAud2Delay,	kRegAud3Delay,	kRegAud4Delay,	kRegAud5Delay,	kRegAud6Delay,	kRegAud7Delay,	kRegAud8Delay,	0};

static const ULWord	gAudioSystemToAudioControlRegNum []	= {	kRegAud1Control,		kRegAud2Control,		kRegAud3Control,		kRegAud4Control,
															kRegAud5Control,		kRegAud6Control,		kRegAud7Control,		kRegAud8Control,		0};

static const ULWord gAudioRateHighMask [] = { kRegMaskAud1RateHigh, kRegMaskAud2RateHigh, kRegMaskAud3RateHigh, kRegMaskAud4RateHigh,
											  kRegMaskAud5RateHigh, kRegMaskAud6RateHigh, kRegMaskAud7RateHigh, kRegMaskAud8RateHigh };

static const ULWord gAudioRateHighShift [] = { kRegShiftAud1RateHigh, kRegShiftAud2RateHigh, kRegShiftAud3RateHigh, kRegShiftAud4RateHigh,
											   kRegShiftAud5RateHigh, kRegShiftAud6RateHigh, kRegShiftAud7RateHigh, kRegShiftAud8RateHigh };

struct PCM_CONTROL_INFO{
	ULWord pcmControlReg;
	ULWord pcmControlMask;
	ULWord pcmControlShift;
	inline PCM_CONTROL_INFO(ULWord regNum, ULWord mask, ULWord shift) : pcmControlReg(regNum), pcmControlMask(mask), pcmControlShift(shift){}
};

static const PCM_CONTROL_INFO gAudioEngineChannelPairToFieldInformation [][8]	=
{
	{
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P1_2, kRegShiftPCMControlA1P1_2),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P3_4, kRegShiftPCMControlA1P3_4),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P5_6, kRegShiftPCMControlA1P5_6),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P7_8, kRegShiftPCMControlA1P7_8),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P9_10, kRegShiftPCMControlA1P9_10),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P11_12, kRegShiftPCMControlA1P11_12),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P13_14, kRegShiftPCMControlA1P13_14),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA1P15_16, kRegShiftPCMControlA1P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P1_2, kRegShiftPCMControlA2P1_2),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P3_4, kRegShiftPCMControlA2P3_4),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P5_6, kRegShiftPCMControlA2P5_6),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P7_8, kRegShiftPCMControlA2P7_8),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P9_10, kRegShiftPCMControlA2P9_10),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P11_12, kRegShiftPCMControlA2P11_12),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P13_14, kRegShiftPCMControlA2P13_14),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA2P15_16, kRegShiftPCMControlA2P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P1_2, kRegShiftPCMControlA3P1_2),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P3_4, kRegShiftPCMControlA3P3_4),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P5_6, kRegShiftPCMControlA3P5_6),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P7_8, kRegShiftPCMControlA3P7_8),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P9_10, kRegShiftPCMControlA3P9_10),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P11_12, kRegShiftPCMControlA3P11_12),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P13_14, kRegShiftPCMControlA3P13_14),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA3P15_16, kRegShiftPCMControlA3P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P1_2, kRegShiftPCMControlA4P1_2),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P3_4, kRegShiftPCMControlA4P3_4),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P5_6, kRegShiftPCMControlA4P5_6),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P7_8, kRegShiftPCMControlA4P7_8),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P9_10, kRegShiftPCMControlA4P9_10),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P11_12, kRegShiftPCMControlA4P11_12),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P13_14, kRegShiftPCMControlA4P13_14),
		PCM_CONTROL_INFO(kRegPCMControl4321, kRegMaskPCMControlA4P15_16, kRegShiftPCMControlA4P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P1_2, kRegShiftPCMControlA5P1_2),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P3_4, kRegShiftPCMControlA5P3_4),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P5_6, kRegShiftPCMControlA5P5_6),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P7_8, kRegShiftPCMControlA5P7_8),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P9_10, kRegShiftPCMControlA5P9_10),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P11_12, kRegShiftPCMControlA5P11_12),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P13_14, kRegShiftPCMControlA5P13_14),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA5P15_16, kRegShiftPCMControlA5P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P1_2, kRegShiftPCMControlA6P1_2),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P3_4, kRegShiftPCMControlA6P3_4),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P5_6, kRegShiftPCMControlA6P5_6),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P7_8, kRegShiftPCMControlA6P7_8),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P9_10, kRegShiftPCMControlA6P9_10),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P11_12, kRegShiftPCMControlA6P11_12),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P13_14, kRegShiftPCMControlA6P13_14),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA6P15_16, kRegShiftPCMControlA6P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P1_2, kRegShiftPCMControlA7P1_2),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P3_4, kRegShiftPCMControlA7P3_4),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P5_6, kRegShiftPCMControlA7P5_6),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P7_8, kRegShiftPCMControlA7P7_8),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P9_10, kRegShiftPCMControlA7P9_10),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P11_12, kRegShiftPCMControlA7P11_12),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P13_14, kRegShiftPCMControlA7P13_14),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA7P15_16, kRegShiftPCMControlA7P15_16)
	},
	{
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P1_2, kRegShiftPCMControlA8P1_2),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P3_4, kRegShiftPCMControlA8P3_4),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P5_6, kRegShiftPCMControlA8P5_6),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P7_8, kRegShiftPCMControlA8P7_8),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P9_10, kRegShiftPCMControlA8P9_10),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P11_12, kRegShiftPCMControlA8P11_12),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P13_14, kRegShiftPCMControlA8P13_14),
		PCM_CONTROL_INFO(kRegPCMControl8765, kRegMaskPCMControlA8P15_16, kRegShiftPCMControlA8P15_16)
	}
};


bool CNTV2Card::SetNumberAudioChannels (const ULWord inNumChannels, const NTV2AudioSystem inAudioSystem)
{
	const ULWord	regAudControl	(NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? gAudioSystemToAudioControlRegNum [inAudioSystem] : 0);

	if (regAudControl == 0)
		return false;
	else if (inNumChannels == 6 || inNumChannels == 8)
	{
		// Make sure 16 channel audio is off
		WriteRegister (regAudControl, 0, kRegMaskAudio16Channel, kRegShiftAudio16Channel);

		// Now turn on 6 or 8 channel audio
		return WriteRegister (regAudControl, inNumChannels == 8, kRegMaskNumChannels, kRegShiftNumChannels);
	}
	else if (inNumChannels == 16)
	{
		// Turn 16 channel audio on, doesn't matter how 8 or 6 channel is set
		return WriteRegister (regAudControl, 1, kRegMaskAudio16Channel, kRegShiftAudio16Channel);
	}
	else
		return false;
}


bool CNTV2Card::GetNumberAudioChannels (ULWord & outNumChannels, const NTV2AudioSystem inAudioSystem)
{
	const ULWord	regAudControl	(NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? gAudioSystemToAudioControlRegNum [inAudioSystem] : 0);
	ULWord			value			(0);
	bool			status			(false);

	if (regAudControl == 0)
		return false;

	status = ReadRegister (regAudControl, value, kRegMaskAudio16Channel, kRegShiftAudio16Channel);
	if (value == 1)
		outNumChannels = 16;
	else
	{
		status = ReadRegister (regAudControl, value, kRegMaskNumChannels, kRegShiftNumChannels);
		if (value == 1)
			outNumChannels = 8;
		else
			outNumChannels = 6;
	}

	return status;
}


bool CNTV2Card::SetAudioRate (const NTV2AudioRate inRate, const NTV2AudioSystem inAudioSystem)
{
	ULWord		rateLow	(0);
	ULWord		rateHigh (0);
	bool		status;

	if ((inRate == NTV2_AUDIO_192K) && (inAudioSystem == NTV2_AUDIOSYSTEM_1))
		return false;

	if (inRate == NTV2_AUDIO_96K)
		rateLow = 1;
	else if (inRate == NTV2_AUDIO_192K)
		rateHigh = 1;

	status = WriteRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], rateLow, kRegMaskAudioRate, kRegShiftAudioRate);
	status &= WriteRegister (kRegAudioControl2, rateHigh, gAudioRateHighMask [inAudioSystem], gAudioRateHighShift [inAudioSystem]);

	return status;
}


bool CNTV2Card::GetAudioRate (NTV2AudioRate & outRate, const NTV2AudioSystem inAudioSystem)
{
	ULWord		rateLow	(0);
	ULWord		rateHigh (0);
	bool		status;

	status = ReadRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], rateLow, kRegMaskAudioRate, kRegShiftAudioRate);
	status &= ReadRegister (kRegAudioControl2, rateHigh, gAudioRateHighMask [inAudioSystem], gAudioRateHighShift [inAudioSystem]);
	if (status)
	{
		if ((rateLow == 0) && (rateHigh == 0))
			outRate = NTV2_AUDIO_48K;
		else if ((rateLow == 1) && (rateHigh == 0))
			outRate = NTV2_AUDIO_96K;
		else if ((rateLow == 0) && (rateHigh == 1))
			outRate = NTV2_AUDIO_192K;
		else
			status = false;
	}
	return status;
}


bool CNTV2Card::SetAudioBufferSize (const NTV2AudioBufferSize inValue, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem))
		return false;
	if (inValue != NTV2_AUDIO_BUFFER_SIZE_4MB && ::NTV2DeviceCanDoStackedAudio(_boardID))
		return false;	//	Stacked audio devices are fixed at 4MB
	return WriteRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], inValue, kK2RegMaskAudioBufferSize, kK2RegShiftAudioBufferSize);
}


bool CNTV2Card::GetAudioBufferSize (NTV2AudioBufferSize & outSize, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem))
		return false;
	outSize = NTV2_AUDIO_BUFFER_SIZE_4MB;	//	NTV2 has standardized on 4MB audio buffers
	if (::NTV2DeviceCanDoStackedAudio(_boardID))
		return true;	//	Done!

	ULWord	value(0);
	if (!ReadRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], value, kK2RegMaskAudioBufferSize, kK2RegShiftAudioBufferSize))
		return false;
	outSize = NTV2AudioBufferSize(value);
	return true;
}


bool CNTV2Card::SetAudioAnalogLevel (const NTV2AudioLevel inLevel, const NTV2AudioSystem inAudioSystem)
{
	return WriteRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], inLevel, kFS1RegMaskAudioLevel, kFS1RegShiftAudioLevel);
}


bool CNTV2Card::GetAudioAnalogLevel (NTV2AudioLevel & outLevel, const NTV2AudioSystem inAudioSystem)
{
	ULWord		value	(0);
	const bool	status	(ReadRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], value, kFS1RegMaskAudioLevel, kK2RegShiftAudioLevel));
	if (status)
		outLevel = static_cast <NTV2AudioLevel> (value);
	return status;
}


bool CNTV2Card::SetAudioLoopBack (const NTV2AudioLoopBack inValue, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_LOOPBACK (inValue))
		return false;
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;
	if (inValue == NTV2_AUDIO_LOOPBACK_ON)
		SetEmbeddedAudioClock (NTV2_EMBEDDED_AUDIO_CLOCK_REFERENCE, inAudioSystem);	//	Use board reference as audio clock
	return WriteRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], inValue, kRegMaskLoopBack, kRegShiftLoopBack);
}


bool CNTV2Card::GetAudioLoopBack (NTV2AudioLoopBack & outValue, const NTV2AudioSystem inAudioSystem)
{
	outValue = NTV2_AUDIO_LOOPBACK_INVALID;
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;

	ULWord		value	(0);
	const bool	status	(ReadRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], value, kRegMaskLoopBack, kRegShiftLoopBack));
	if (status)
		outValue = static_cast <NTV2AudioLoopBack> (value);
	return status;
}


bool CNTV2Card::SetEncodedAudioMode (const NTV2EncodedAudioMode inMode, const NTV2AudioSystem inAudioSystem)
{
	return WriteRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], inMode, kRegMaskEncodedAudioMode, kRegShiftEncodedAudioMode);
}


bool CNTV2Card::GetEncodedAudioMode (NTV2EncodedAudioMode & outMode, const NTV2AudioSystem inAudioSystem)
{
	ULWord		value	(0);
	const bool	status	(ReadRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], value, kRegMaskEncodedAudioMode, kRegShiftEncodedAudioMode));
	if (status)
		outMode = static_cast <NTV2EncodedAudioMode> (value);
	return status;
}


bool CNTV2Card::SetEmbeddedAudioInput (const NTV2EmbeddedAudioInput inAudioInput, const NTV2AudioSystem inAudioSystem)
{
	const ULWord	regAudSource	(gAudioSystemToSrcSelectRegNum [inAudioSystem]);
	const ULWord	numInputs		(::NTV2DeviceGetNumVideoInputs (_boardID));
	const ULWord	numHDMI			(::NTV2DeviceGetNumHDMIVideoInputs (_boardID));
	bool			status			(false);
	ULWord			value1			(0);
	ULWord			value2			(0);

	switch (inAudioInput)
	{												//	Sparse bits
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1:		value1 = 0x0;	value2 = 0x0;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2:		value1 = 0x1;	value2 = 0x0;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3:		value1 = 0x0;	value2 = 0x1;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4:		value1 = 0x1;	value2 = 0x1;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5:		value1 = 0x0;	value2 = 0x0;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6:		value1 = 0x1;	value2 = 0x0;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7:		value1 = 0x0;	value2 = 0x1;	break;
		case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8:		value1 = 0x1;	value2 = 0x1;	break;
		default:																	return false;
	}

	status = WriteRegister (regAudSource, value1, kRegMaskEmbeddedAudioInput, kRegShiftEmbeddedAudioInput);
	if (numInputs > 2 || inAudioInput > NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4 || numHDMI > 1)
		status = WriteRegister (regAudSource, value2, kRegMaskEmbeddedAudioInput2, kRegShiftEmbeddedAudioInput2);
	return status;
}


bool CNTV2Card::GetEmbeddedAudioInput (NTV2EmbeddedAudioInput & outAudioInput, const NTV2AudioSystem inAudioSystem)
{
	const ULWord	srcSelectReg	(gAudioSystemToSrcSelectRegNum [inAudioSystem]);
	const ULWord	numInputs		(::NTV2DeviceGetNumVideoInputs (_boardID));
	ULWord			value			(0);
	bool			status			(false);

	if (numInputs <= 2)
		status = ReadRegister (srcSelectReg, value, kRegMaskEmbeddedAudioInput, kRegShiftEmbeddedAudioInput);
	else
	{
		ULWord	sparse1 (0), sparse2 (0);	//	Sparse bits
		status = ReadRegister (srcSelectReg, sparse1, kRegMaskEmbeddedAudioInput, kRegShiftEmbeddedAudioInput)
				&& ReadRegister (srcSelectReg, sparse2, kRegMaskEmbeddedAudioInput2, kRegShiftEmbeddedAudioInput2);
		if (!sparse1 && !sparse2)
			value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1;
		else if (sparse1 && !sparse2)
			value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2;
		else if (!sparse1 && sparse2)
			value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3;
		else if (sparse1 && sparse2)
			value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4;

		if (inAudioSystem >= NTV2_AUDIOSYSTEM_5)
			switch (value)
			{
				case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1:		value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_5;	break;
				case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2:		value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_6;	break;
				case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_3:		value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_7;	break;
				case NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_4:		value = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_8;	break;
			}
	}
	if (status)
		outAudioInput = NTV2EmbeddedAudioInput(value);
	return status;
}


bool CNTV2Card::SetEmbeddedAudioClock (const NTV2EmbeddedAudioClock inValue, const NTV2AudioSystem inAudioSystem)
{
	return WriteRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], inValue, kRegMaskEmbeddedAudioClock, kRegShiftEmbeddedAudioClock);
}


bool CNTV2Card::GetEmbeddedAudioClock (NTV2EmbeddedAudioClock & outValue, const NTV2AudioSystem inAudioSystem)
{
	return CNTV2DriverInterface::ReadRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], outValue, kRegMaskEmbeddedAudioClock, kRegShiftEmbeddedAudioClock);
}


bool CNTV2Card::GetAudioWrapAddress (ULWord & outWrapAddress, const NTV2AudioSystem inAudioSystem)
{
	NTV2AudioBufferSize	bufferSize	(NTV2_MAX_NUM_AudioBufferSizes);
	if (!GetAudioBufferSize (bufferSize, inAudioSystem))
		return false;

	switch (bufferSize)
	{
		case NTV2_AUDIO_BUFFER_STANDARD:	outWrapAddress = NTV2_AUDIO_WRAPADDRESS;		break;
		case NTV2_AUDIO_BUFFER_BIG:			outWrapAddress = NTV2_AUDIO_WRAPADDRESS_BIG;	break;
	#if !defined (NTV2_DEPRECATE)
		case NTV2_AUDIO_BUFFER_BIGGER:		outWrapAddress = NTV2_AUDIO_WRAPADDRESS_BIGGER;	break;
		case NTV2_AUDIO_BUFFER_MEDIUM:		outWrapAddress = NTV2_AUDIO_WRAPADDRESS;		break;
	#endif	//	!defined (NTV2_DEPRECATE)
		default:							outWrapAddress = NTV2_AUDIO_WRAPADDRESS;		break;
	}
	return true;
}


bool CNTV2Card::GetAudioReadOffset (ULWord & outReadOffset, const NTV2AudioSystem inAudioSystem)
{
	NTV2AudioBufferSize	bufferSize	(NTV2_MAX_NUM_AudioBufferSizes);
	if (!GetAudioBufferSize (bufferSize, inAudioSystem))
		return false;

	switch (bufferSize)
	{
		case NTV2_AUDIO_BUFFER_STANDARD:	outReadOffset = NTV2_AUDIO_READBUFFEROFFSET;		break;
		case NTV2_AUDIO_BUFFER_BIG:			outReadOffset = NTV2_AUDIO_READBUFFEROFFSET_BIG;	break;
	#if !defined (NTV2_DEPRECATE)
		case NTV2_AUDIO_BUFFER_BIGGER:		outReadOffset = NTV2_AUDIO_READBUFFEROFFSET_BIGGER;	break;
		case NTV2_AUDIO_BUFFER_MEDIUM:		outReadOffset = NTV2_AUDIO_READBUFFEROFFSET;		break;
	#endif	//	!defined (NTV2_DEPRECATE)
		default:							outReadOffset = NTV2_AUDIO_READBUFFEROFFSET;		break;
	}
	return true;
}


bool CNTV2Card::ReadAudioLastIn (ULWord & outValue,	const NTV2AudioSystem inAudioSystem)
{
	return ReadRegister (gChannelToAudioInLastAddrRegNum[inAudioSystem], outValue);
}

bool CNTV2Card::WriteAudioLastOut (const ULWord inValue, const NTV2AudioSystem inAudioSystem)
{
	return WriteRegister (gChannelToAudioOutLastAddrRegNum[inAudioSystem], inValue);
}

bool CNTV2Card::ReadAudioLastOut (ULWord & outValue, const NTV2AudioSystem inAudioSystem)
{
	return ReadRegister (gChannelToAudioOutLastAddrRegNum[inAudioSystem], outValue);
}

#if !defined(NTV2_DEPRECATE_16_0)
	bool CNTV2Card::ReadAudioSource (ULWord & outValue, const NTV2Channel inChannel)	{return ReadRegister(gAudioSystemToSrcSelectRegNum[inChannel], outValue);}
	bool CNTV2Card::WriteAudioSource (const ULWord inValue, const NTV2Channel inChannel)	{return WriteRegister(gAudioSystemToSrcSelectRegNum[inChannel], inValue);}
#endif	//	!defined(NTV2_DEPRECATE_16_0)


bool CNTV2Card::SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2AudioSource inAudioSource, const NTV2EmbeddedAudioInput inEmbeddedSource)
{
	bool result(false);
	static const ULWord	sAudioSourceToRegValues []	= {	0x1,	//	NTV2_AUDIO_EMBEDDED
														0x0,	//	NTV2_AUDIO_AES
														0x9,	//	NTV2_AUDIO_ANALOG
														0xA,	//	NTV2_AUDIO_HDMI
														0xB};	//	NTV2_AUDIO_MIC

	if (UWord(inAudioSystem) < (::NTV2DeviceGetNumAudioSystems(_boardID) + (DeviceCanDoAudioMixer() ? 2 : 0)))
		if (NTV2_IS_VALID_AUDIO_SOURCE(inAudioSource))
			result = WriteRegister (gAudioSystemToSrcSelectRegNum [inAudioSystem],
									sAudioSourceToRegValues [inAudioSource],
									kRegMaskAudioSource, kRegShiftAudioSource);
	if (result)
        if ((inAudioSource == NTV2_AUDIO_EMBEDDED)  ||  (inAudioSource == NTV2_AUDIO_HDMI))
			if (SetEmbeddedAudioInput (inEmbeddedSource, inAudioSystem))	//	Use the specified input for grabbing embedded audio
				result = SetEmbeddedAudioClock (NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT, inAudioSystem);	//	Use video input clock (not reference)
	return result;

}	//	SetAudioSystemInputSource


bool CNTV2Card::GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource & outAudioSource, NTV2EmbeddedAudioInput & outEmbeddedSource)
{
	ULWord	regValue	(0);

	outAudioSource		= NTV2_AUDIO_SOURCE_INVALID;
	outEmbeddedSource	= NTV2_EMBEDDED_AUDIO_INPUT_INVALID;

	if (static_cast <UWord> (inAudioSystem) >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 2 : 0)))
		return false;
	if (!ReadRegister (gAudioSystemToSrcSelectRegNum [inAudioSystem], regValue, kRegMaskAudioSource, kRegShiftAudioSource))
		return false;
	switch (regValue & 0x0000000F)
	{
		case 0x1:	outAudioSource = NTV2_AUDIO_EMBEDDED;	break;
		case 0x0:	outAudioSource = NTV2_AUDIO_AES;		break;
		case 0x9:	outAudioSource = NTV2_AUDIO_ANALOG;		break;
		case 0xA:	outAudioSource = NTV2_AUDIO_HDMI;		break;
		case 0xB:	outAudioSource = NTV2_AUDIO_MIC;		break;
		default:	return false;
	}

	if (outAudioSource == NTV2_AUDIO_EMBEDDED)
		GetEmbeddedAudioInput(outEmbeddedSource, inAudioSystem);
	return true;

}


bool CNTV2Card::GetSDIOutputAudioSystem (const NTV2Channel inChannel, NTV2AudioSystem & outAudioSystem)
{
	outAudioSystem = NTV2_AUDIOSYSTEM_INVALID;
	if (ULWord (inChannel) >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
		return false;	//	illegal channel

	ULWord			b2(0),  b1(0),  b0(0);		//	The three bits that determine which audio system feeds the SDI output
	const ULWord	regNum	(gChannelToSDIOutControlRegNum [inChannel]);
	if (!ReadRegister (regNum, b2, BIT(18), 18))	//	bit 18 is MSB
		return false;
	if (!ReadRegister (regNum, b1, BIT(28), 28))
		return false;
	if (!ReadRegister (regNum, b0, BIT(30), 30))	//	bit 30 is LSB
		return false;
	outAudioSystem = static_cast <NTV2AudioSystem> (b2 * 4  +  b1 * 2  +  b0);
	return true;

}	//	GetSDIOutputAudioSystem


bool CNTV2Card::SetSDIOutputAudioSystem (const NTV2Channel inChannel, const NTV2AudioSystem inAudioSystem)
{
	if (ULWord (inChannel) >= ::NTV2DeviceGetNumVideoOutputs (_boardID))
		return false;	//	Invalid channel
	if (UWord (inAudioSystem) >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 2 : 0)))
		return false;	//	Invalid audio system

	ULWord	value	(inAudioSystem);
	ULWord	b2		(value / 4);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b2, BIT(18), 18))	//	bit 18 is MSB
		return false;

	value -= b2 * 4;
	ULWord	b1		(value / 2);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b1, BIT(28), 28))
		return false;

	value -= b1 * 2;
	ULWord	b0		(value);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b0, BIT(30), 30))	//	bit 30 is LSB
		return false;

	//NTV2AudioSystem	compareA;
	//GetSDIOutputAudioSystem (inChannel, compareA);
	//NTV2_ASSERT(compareA == inAudioSystem);
	return true;

}	//	SetSDIOutputAudioSystem


bool CNTV2Card::GetSDIOutputDS2AudioSystem (const NTV2Channel inChannel, NTV2AudioSystem & outAudioSystem)
{
	outAudioSystem = NTV2_AUDIOSYSTEM_INVALID;
	if (ULWord (inChannel) >= ::NTV2DeviceGetNumVideoOutputs (_boardID))
		return false;	//	illegal channel

	ULWord			b2(0),  b1(0),  b0(0);		//	The three bits that determine which audio system feeds the SDI output's DS2
	const ULWord	regNum	(gChannelToSDIOutControlRegNum [inChannel]);
	if (!ReadRegister (regNum, b2, BIT(19), 19))	//	bit 19 is MSB
		return false;
	if (!ReadRegister (regNum, b1, BIT(29), 29))
		return false;
	if (!ReadRegister (regNum, b0, BIT(31), 31))	//	bit 31 is LSB
		return false;
	outAudioSystem = static_cast <NTV2AudioSystem> (b2 * 4  +  b1 * 2  +  b0);
	return true;

}	//	GetSDIOutputDS2AudioSystem


bool CNTV2Card::SetSDIOutputDS2AudioSystem (const NTV2Channel inChannel, const NTV2AudioSystem inAudioSystem)
{
	if (ULWord (inChannel) >= ::NTV2DeviceGetNumVideoOutputs (_boardID))
		return false;	//	Invalid channel
	if (UWord (inAudioSystem) >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 2 : 0)))
		return false;	//	Invalid audio system

	ULWord	value	(inAudioSystem);
	ULWord	b2		(value / 4);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b2, BIT(19), 19))	//	bit 19 is MSB
		return false;

	value -= b2 * 4;
	ULWord	b1		(value / 2);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b1, BIT(29), 29))
		return false;

	value -= b1 * 2;
	ULWord	b0		(value);
	if (!WriteRegister (gChannelToSDIOutControlRegNum [inChannel], b0, BIT(31), 31))	//	bit 31 is LSB
		return false;

	//NTV2AudioSystem	compareA;
	//GetSDIOutputDS2AudioSystem (inChannel, compareA);
	//NTV2_ASSERT(compareA == inAudioSystem);
	return true;

}	//	SetSDIOutputDS2AudioSystem


static const ULWord	sAudioMixerInputSelectMasks[] = {kRegMaskAudioMixerMainInputSelect, kRegMaskAudioMixerAux1x2CHInput, kRegMaskAudioMixerAux2x2CHInput, 0};
static const ULWord	sAudioMixerInputSelectShifts[] = {kRegShiftAudioMixerMainInputSelect, kRegShiftAudioMixerAux1x2CHInput, kRegShiftAudioMixerAux2x2CHInput, 0};


bool CNTV2Card::GetAudioMixerInputAudioSystem (const NTV2AudioMixerInput inMixerInput, NTV2AudioSystem & outAudioSystem)
{
	outAudioSystem = NTV2_AUDIOSYSTEM_INVALID;
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	return CNTV2DriverInterface::ReadRegister(kRegAudioMixerInputSelects, outAudioSystem,
											sAudioMixerInputSelectMasks[inMixerInput],
											sAudioMixerInputSelectShifts[inMixerInput]);
}

bool CNTV2Card::SetAudioMixerInputAudioSystem (const NTV2AudioMixerInput inMixerInput, const NTV2AudioSystem inAudioSystem)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	if (UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID) + 1)
		return false;	//	No such audio system on this device
	return WriteRegister (kRegAudioMixerInputSelects, ULWord(inAudioSystem),
							sAudioMixerInputSelectMasks[inMixerInput],
							sAudioMixerInputSelectShifts[inMixerInput]);
}

bool CNTV2Card::GetAudioMixerInputChannelSelect (const NTV2AudioMixerInput inMixerInput, NTV2AudioChannelPair & outChannelPair)
{
	outChannelPair = NTV2_AUDIO_CHANNEL_PAIR_INVALID;
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	if (NTV2_IS_AUDIO_MIXER_INPUT_MAIN(inMixerInput))
		return CNTV2DriverInterface::ReadRegister (kRegAudioMixerChannelSelect, outChannelPair,
													kRegMaskAudioMixerChannelSelect,
													kRegShiftAudioMixerChannelSelect);
	outChannelPair = NTV2_AudioChannel1_2;	//	Aux1/Aux2 always use 1&2
	return true;
}

bool CNTV2Card::SetAudioMixerInputChannelSelect (const NTV2AudioMixerInput inMixerInput, const NTV2AudioChannelPair inChannelPair)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_AUDIO_MIXER_INPUT_MAIN(inMixerInput))
		return false;	//	Can only change Main channel selection
	if (!NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(inChannelPair))
		return false;	//	Only audio channels 1 thru 16 allowed
	return WriteRegister (kRegAudioMixerChannelSelect, ULWord(inChannelPair),
							kRegMaskAudioMixerMainInputSelect, kRegShiftAudioMixerMainInputSelect);
}

static const ULWord	sAudioMixerInputGainCh1Regs[] = {kRegAudioMixerMainGain, kRegAudioMixerAux1GainCh1, kRegAudioMixerAux2GainCh1, 0};
static const ULWord	sAudioMixerInputGainCh2Regs[] = {kRegAudioMixerMainGain, kRegAudioMixerAux1GainCh2, kRegAudioMixerAux2GainCh2, 0};

bool CNTV2Card::GetAudioMixerInputGain (const NTV2AudioMixerInput inMixerInput, const NTV2AudioMixerChannel inChannel, ULWord & outGainValue)
{
	outGainValue = 0;
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	if (!NTV2_IS_AUDIO_MIXER_CHANNELS_1_OR_2(inChannel))
		return false;	//	Bad audio channel specified -- must be Ch1 or Ch2
	return ReadRegister (inChannel == NTV2_AudioMixerChannel1
							? sAudioMixerInputGainCh1Regs[inMixerInput]
							: sAudioMixerInputGainCh2Regs[inMixerInput], outGainValue);
}

bool CNTV2Card::SetAudioMixerInputGain (const NTV2AudioMixerInput inMixerInput, const NTV2AudioMixerChannel inChannel, const ULWord inGainValue)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	if (!NTV2_IS_AUDIO_MIXER_CHANNELS_1_OR_2(inChannel))
		return false;	//	Bad audio channel specified -- must be Ch1 or Ch2
	return WriteRegister(inChannel == NTV2_AudioMixerChannel1
							? sAudioMixerInputGainCh1Regs[inMixerInput]
							: sAudioMixerInputGainCh2Regs[inMixerInput], inGainValue);
}

bool CNTV2Card::GetAudioMixerOutputGain (ULWord & outGainValue)
{
	outGainValue = 0;
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	return ReadRegister (kRegAudioMixerOutGain, outGainValue);
}

bool CNTV2Card::SetAudioMixerOutputGain (const ULWord inGainValue)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	WriteRegister(kRegAudioMixerOutLGain, inGainValue);
	return  WriteRegister(kRegAudioMixerOutRGain, inGainValue);
}

bool CNTV2Card::GetAudioMixerOutputLevels (const NTV2AudioChannelPairs & inChannelPairs,
											vector<uint32_t> & outLevels)
{
	static const ULWord	gAudMxrMainOutLvlRegs[] ={kRegAudioMixerMainOutputLevelsPair0, kRegAudioMixerMainOutputLevelsPair1,
												 kRegAudioMixerMainOutputLevelsPair2, kRegAudioMixerMainOutputLevelsPair3,
												 kRegAudioMixerMainOutputLevelsPair4, kRegAudioMixerMainOutputLevelsPair5,
												 kRegAudioMixerMainOutputLevelsPair6, kRegAudioMixerMainOutputLevelsPair7, 0};
	outLevels.clear();
	if (!DeviceCanDoAudioMixer())
		return false;

	//	If caller specified empty channelPairs set, do "all" possible pairs...
	NTV2AudioChannelPairs	chanPairs;
	if (inChannelPairs.empty())
	{
		for (NTV2AudioChannelPair chPr(NTV2_AudioChannel1_2);  NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chPr);  chPr = NTV2AudioChannelPair(chPr+1))
			chanPairs.insert(chPr);	//	Main supports Ch 1-16
	}
	else
		chanPairs = inChannelPairs;	//	Non-empty set:  do what the caller requested

	//	Build a bulk register read...
	NTV2RegisterReads	regs;
	std::set<ULWord>	regsToRead;
	for (NTV2AudioChannelPairsConstIter it(chanPairs.begin());  it != chanPairs.end();  ++it)
	{
		const NTV2AudioChannelPair	chanPair(*it);
		if (!NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chanPair))
			return false;
		uint32_t regNum(gAudMxrMainOutLvlRegs[chanPair]);
		regsToRead.insert(regNum);
	}	//	for each audio channel pair
	for (std::set<ULWord>::const_iterator it(regsToRead.begin());  it != regsToRead.end();  ++it)
		regs.push_back(NTV2RegInfo(*it));

	//	Read the level registers...
	const bool result(ReadRegisters(regs));
	if (result)
		for (NTV2RegisterReadsConstIter it(regs.begin());  it != regs.end();  ++it)
		{
			ULWord	rawLevels(it->IsValid() ? it->registerValue : 0);
			outLevels.push_back(uint32_t((rawLevels & kRegMaskAudioMixerInputLeftLevel) >> kRegShiftAudioMixerInputLeftLevel));
			outLevels.push_back(uint32_t((rawLevels & kRegMaskAudioMixerInputRightLevel) >> kRegShiftAudioMixerInputRightLevel));
		}
	else
		while (outLevels.size() < chanPairs.size() * 2)
			outLevels.push_back(0);
	return result;
}

bool CNTV2Card::GetHeadphoneOutputGain (ULWord & outGainValue)
{
	outGainValue = 0;
	if (!NTV2DeviceHasRotaryEncoder(GetDeviceID()))
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	return ReadRegister (kRegRotaryEncoder, outGainValue, kRegMaskRotaryEncoderGain, kRegShiftRotaryEncoderGain);
}

bool CNTV2Card::SetHeadphoneOutputGain (const ULWord inGainValue)
{
	if (!NTV2DeviceHasRotaryEncoder(GetDeviceID()))
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	return WriteRegister(kRegRotaryEncoder, inGainValue, kRegMaskRotaryEncoderGain, kRegShiftRotaryEncoderGain);
}

#if !defined(NTV2_DEPRECATE_15_5)
	bool CNTV2Card::SetAudioMixerAux1InputGain (const NTV2AudioMixerChannel inChannel, const ULWord inGainValue)
	{
		if (inChannel == NTV2_AudioMixerChannel1  ||  inChannel == NTV2_AudioMixerChannel2)
			return SetAudioMixerInputGain(NTV2_AudioMixerInputAux1, NTV2_AudioMixerChannel1, inGainValue)
				&& SetAudioMixerInputGain(NTV2_AudioMixerInputAux1, NTV2_AudioMixerChannel2, inGainValue);
		return false;
	}

	bool CNTV2Card::SetAudioMixerAux2InputGain(const NTV2AudioMixerChannel inChannel, const ULWord inGainValue)
	{
		if (inChannel == NTV2_AudioMixerChannel1  ||  inChannel == NTV2_AudioMixerChannel2)
			return SetAudioMixerInputGain(NTV2_AudioMixerInputAux2, NTV2_AudioMixerChannel1, inGainValue)
				&& SetAudioMixerInputGain(NTV2_AudioMixerInputAux2, NTV2_AudioMixerChannel2, inGainValue);
		return false;
	}

	ULWord CNTV2Card::GetAudioMixerMainInputChannelLevel(NTV2AudioMixerChannel inChannel)
	{
		if (!NTV2_IS_VALID_AUDIO_MIXER_CHANNEL(inChannel))
			return 0;

		const NTV2AudioChannelPair	chPair (NTV2AudioChannelPair(inChannel / 2));
		NTV2AudioChannelPairs		chPairs;
		chPairs.insert(chPair);
		vector<uint32_t>	levels;
		if (!GetAudioMixerInputLevels(NTV2_AudioMixerInputMain, chPairs, levels))
			return 0;
		return levels.at(inChannel%2);
	}

	ULWord CNTV2Card::GetAudioMixerAux1InputChannelLevel(NTV2AudioMixerChannel inChannel)
	{
		if (!NTV2_IS_VALID_AUDIO_MIXER_CHANNEL(inChannel))
			return 0;

		const NTV2AudioChannelPair	chPair (NTV2AudioChannelPair(inChannel / 2));
		NTV2AudioChannelPairs		chPairs;
		chPairs.insert(chPair);
		vector<uint32_t>	levels;
		if (!GetAudioMixerInputLevels(NTV2_AudioMixerInputAux1, chPairs, levels))
			return 0;
		return levels.at(inChannel%2);
	}

	ULWord CNTV2Card::GetAudioMixerAux2InputChannelLevel(NTV2AudioMixerChannel inChannel)
	{
		if (!NTV2_IS_VALID_AUDIO_MIXER_CHANNEL(inChannel))
			return 0;

		const NTV2AudioChannelPair	chPair (NTV2AudioChannelPair(inChannel / 2));
		NTV2AudioChannelPairs		chPairs;
		chPairs.insert(chPair);
		vector<uint32_t>	levels;
		if (!GetAudioMixerInputLevels(NTV2_AudioMixerInputAux2, chPairs, levels))
			return 0;
		return levels.at(inChannel%2);
	}

	bool CNTV2Card::GetAudioMixerMainInputEnable (bool & outEnabled)
	{
		outEnabled = false;
		NTV2AudioChannelsMuted16	mutedChannels;
		if (!GetAudioMixerInputChannelsMute (NTV2_AudioMixerInputMain, mutedChannels))
			return false;
		outEnabled = !mutedChannels.test(NTV2_AudioMixerChannel1) || !mutedChannels.test(NTV2_AudioMixerChannel2);
		return true;
	}

	bool CNTV2Card::GetAudioMixerAux1InputEnable (bool & outEnabled)
	{
		outEnabled = false;
		NTV2AudioChannelsMuted16	mutedChannels;
		if (!GetAudioMixerInputChannelsMute (NTV2_AudioMixerInputAux1, mutedChannels))
			return false;
		outEnabled = !mutedChannels.test(NTV2_AudioMixerChannel1) || !mutedChannels.test(NTV2_AudioMixerChannel2);
		return true;
	}

	bool CNTV2Card::GetAudioMixerAux2InputEnable (bool & outEnabled)
	{
		outEnabled = false;
		NTV2AudioChannelsMuted16	mutedChannels;
		if (!GetAudioMixerInputChannelsMute (NTV2_AudioMixerInputAux2, mutedChannels))
			return false;
		outEnabled = !mutedChannels.test(NTV2_AudioMixerChannel1) || !mutedChannels.test(NTV2_AudioMixerChannel2);
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_15_5)

static const ULWord	sAudioMixerInputMuteMasks[] = {kRegMaskAudioMixerMainInputEnable, kRegMaskAudioMixerAux1InputEnable, kRegMaskAudioMixerAux2InputEnable, 0};
static const ULWord	sAudioMixerInputMuteShifts[] = {kRegShiftAudioMixerMainInputEnable, kRegShiftAudioMixerAux1InputEnable, kRegShiftAudioMixerAux2InputEnable, 0};

bool CNTV2Card::GetAudioMixerOutputChannelsMute (NTV2AudioChannelsMuted16 & outMutes)
{
	outMutes.reset();
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	unsigned long	ulongvalue(0);
	if (!CNTV2DriverInterface::ReadRegister(kRegAudioMixerMutes, ulongvalue, kRegMaskAudioMixerOutputChannelsMute, kRegShiftAudioMixerOutputChannelsMute))
		return false;
	outMutes = NTV2AudioChannelsMuted16(ulongvalue);	//	Hardware uses 1=mute 0=enabled
	return true;
}

bool CNTV2Card::SetAudioMixerOutputChannelsMute (const NTV2AudioChannelsMuted16 inMutes)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	return WriteRegister(kRegAudioMixerMutes, ULWord(inMutes.to_ulong()), kRegMaskAudioMixerOutputChannelsMute, kRegShiftAudioMixerOutputChannelsMute);
}

bool CNTV2Card::GetAudioMixerInputChannelsMute (const NTV2AudioMixerInput inMixerInput, NTV2AudioChannelsMuted16 & outMutes)
{
	outMutes.reset();
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	ULWord	muteBits(0);
	if (!ReadRegister(kRegAudioMixerMutes, muteBits, sAudioMixerInputMuteMasks[inMixerInput], sAudioMixerInputMuteShifts[inMixerInput]))
		return false;
	outMutes = NTV2AudioChannelsMuted16(muteBits);
	return true;
}

bool CNTV2Card::SetAudioMixerInputChannelsMute (const NTV2AudioMixerInput inMixerInput, const NTV2AudioChannelsMuted16 inMutes)
{
	if (!DeviceCanDoAudioMixer())
		return false;	//	No Audio Mixer -- shouldn't be calling this function
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;	//	Bad Mixer Input specified
	const ULWord muteBits(ULWord(inMutes.to_ulong()));
	return WriteRegister (kRegAudioMixerMutes, muteBits, sAudioMixerInputMuteMasks[inMixerInput], sAudioMixerInputMuteShifts[inMixerInput]);
}


bool CNTV2Card::GetAudioMixerInputLevels (const NTV2AudioMixerInput inMixerInput,
											const NTV2AudioChannelPairs & inChannelPairs,
											vector<uint32_t> & outLevels)
{
	static const ULWord	gAudMxrMainInLvlRegs[] ={kRegAudioMixerMainInputLevelsPair0, kRegAudioMixerMainInputLevelsPair1,
												 kRegAudioMixerMainInputLevelsPair2, kRegAudioMixerMainInputLevelsPair3,
												 kRegAudioMixerMainInputLevelsPair4, kRegAudioMixerMainInputLevelsPair5,
												 kRegAudioMixerMainInputLevelsPair6, kRegAudioMixerMainInputLevelsPair7, 0};
	outLevels.clear();
	if (!DeviceCanDoAudioMixer())
		return false;
	if (!NTV2_IS_VALID_AUDIO_MIXER_INPUT(inMixerInput))
		return false;

	//	If caller specified empty channelPairs set, do "all" possible pairs...
	NTV2AudioChannelPairs	chanPairs;
	if (inChannelPairs.empty())
	{
		if (!NTV2_IS_AUDIO_MIXER_INPUT_MAIN(inMixerInput))
			chanPairs.insert(NTV2_AudioChannel1_2);	//	Aux1/Aux2 only support Ch1&2
		else
			for (NTV2AudioChannelPair chPr(NTV2_AudioChannel1_2);  NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chPr);  chPr = NTV2AudioChannelPair(chPr+1))
				chanPairs.insert(chPr);	//	Main supports Ch 1-16
	}
	else
		chanPairs = inChannelPairs;	//	Non-empty set:  do what the caller requested

	//	Build a bulk register read...
	NTV2RegisterReads	regs;
	std::set<ULWord>	regsToRead;
	for (NTV2AudioChannelPairsConstIter it(chanPairs.begin());  it != chanPairs.end();  ++it)
	{
		const NTV2AudioChannelPair	chanPair(*it);
		if (!NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chanPair))
			return false;
		uint32_t regNum(gAudMxrMainInLvlRegs[chanPair]);
		if (!NTV2_IS_AUDIO_MIXER_INPUT_MAIN(inMixerInput))
		{
			if (chanPair != NTV2_AudioChannel1_2)
				return false;	//	Aux1 & Aux2 can only report Chan 1&2 levels
			regNum = (inMixerInput == NTV2_AudioMixerInputAux1)
						? kRegAudioMixerAux1InputLevels
						: kRegAudioMixerAux2InputLevels;
		}
		regsToRead.insert(regNum);
	}	//	for each audio channel pair
	for (std::set<ULWord>::const_iterator it(regsToRead.begin());  it != regsToRead.end();  ++it)
		regs.push_back(NTV2RegInfo(*it));

	//	Read the level registers...
	const bool result(ReadRegisters(regs));
	if (result)
		for (NTV2RegisterReadsConstIter it(regs.begin());  it != regs.end();  ++it)
		{
			ULWord	rawLevels(it->IsValid() ? it->registerValue : 0);
			outLevels.push_back(uint32_t((rawLevels & kRegMaskAudioMixerInputLeftLevel) >> kRegShiftAudioMixerInputLeftLevel));
			outLevels.push_back(uint32_t((rawLevels & kRegMaskAudioMixerInputRightLevel) >> kRegShiftAudioMixerInputRightLevel));
		}
	else
		while (outLevels.size() < chanPairs.size() * 2)
			outLevels.push_back(0);
	return result;
}

bool CNTV2Card::GetAudioMixerLevelsSampleCount (ULWord & outSampleCount)
{
	outSampleCount = 0;
	if (!ReadRegister(kRegAudioMixerChannelSelect, outSampleCount, kRegMaskAudioMixerLevelSampleCount, kRegShiftAudioMixerLevelSampleCount))
		return false;
	outSampleCount = 1 << outSampleCount;
	return true;
}

bool CNTV2Card::SetAudioMixerLevelsSampleCount (const ULWord inSampleCount)
{
	if (!inSampleCount)
		return false;	//	Must be > 0
	if (inSampleCount > 0x00008000)
		return false;	//	Must be <= 0x8000
	ULWord result(0), sampleCount(inSampleCount);
	while (sampleCount >>= 1)
		++result;
	return WriteRegister(kRegAudioMixerChannelSelect, result, kRegMaskAudioMixerLevelSampleCount, kRegShiftAudioMixerLevelSampleCount);
}



bool CNTV2Card::SetHDMIOutAudioChannels (const NTV2HDMIAudioChannels value)
{
	return WriteRegister (kRegHDMIOutControl, static_cast <ULWord> (value), kRegMaskHDMIOutAudioCh, kRegShiftHDMIOutAudioCh);
}


bool CNTV2Card::GetHDMIOutAudioChannels (NTV2HDMIAudioChannels & outValue)
{
	return CNTV2DriverInterface::ReadRegister (kRegHDMIOutControl, outValue, kRegMaskHDMIOutAudioCh, kRegShiftHDMIOutAudioCh);
}


bool CNTV2Card::SetHDMIOutAudioSource2Channel (const NTV2AudioChannelPair inValue, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_CHANNEL_PAIR (inValue))
		return false;

	if(NTV2DeviceGetHDMIVersion(GetDeviceID()) > 3)
	{
		NTV2Audio8ChannelSelect bankSelect = (inValue >= NTV2_AudioChannel9_10) ? NTV2_AudioChannel9_16 : NTV2_AudioChannel1_8;
		ULWord channelSelect = static_cast<ULWord>(inValue) % 4;
		WriteRegister (kRegHDMIInputControl, inAudioSystem, kRegMaskHDMIOutSourceSelect, kRegShiftHDMIOutSourceSelect);
		WriteRegister (kRegHDMIOutControl, bankSelect, kRegMaskHDMIOut8ChGroupSelect, kRegShiftHDMIOut8ChGroupSelect);
		WriteRegister (kRegHDMIInputControl, channelSelect, kRegMaskHDMIOutAudio2ChannelSelect, kRegShiftHDMIOutAudio2ChannelSelect);
		return SetHDMIOutAudioChannels(NTV2_HDMIAudio2Channels);
	}
	else
	{
		const ULWord	encoding	((ULWord (inAudioSystem) << 4) | inValue);
		return WriteRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource);
	}
}


bool CNTV2Card::GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair & outValue, NTV2AudioSystem & outAudioSystem)
{
	bool	result = false;
	if(NTV2DeviceGetHDMIVersion(GetDeviceID()) > 3)
	{
		ULWord engineSelect (0), channelSelect(0), bankSelect(0);
		result = ReadRegister(kRegHDMIInputControl, engineSelect,  kRegMaskHDMIOutSourceSelect, kRegShiftHDMIOutSourceSelect);
		if (result)
		{
			outAudioSystem = NTV2AudioSystem(engineSelect);
			result = ReadRegister(kRegHDMIInputControl, channelSelect,  kRegMaskHDMIOutAudio2ChannelSelect, kRegShiftHDMIOutAudio2ChannelSelect);
			result = ReadRegister(kRegHDMIOutControl, bankSelect,  kRegMaskHDMIOut8ChGroupSelect, kRegShiftHDMIOut8ChGroupSelect);
			outValue = NTV2AudioChannelPair((bankSelect == 0 ? 0 : 4) + channelSelect);
		}
	}
	else
	{
		ULWord	encoding	(0);
		result = ReadRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource);
		if (result)
		{
			outValue = NTV2AudioChannelPair(encoding & 0x7);
			outAudioSystem = NTV2AudioSystem(encoding >> 4);
		}
	}
	return result;
}


bool CNTV2Card::SetHDMIOutAudioSource8Channel (const NTV2Audio8ChannelSelect inValue, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_CHANNEL_OCTET (inValue))
		return false;

	if(NTV2DeviceGetHDMIVersion(GetDeviceID()) > 3)
	{
		NTV2Audio8ChannelSelect channelSelect = inValue == NTV2_AudioChannel9_16 ? NTV2_AudioChannel9_16 : NTV2_AudioChannel1_8;
		WriteRegister (kRegHDMIInputControl, inAudioSystem, kRegMaskHDMIOutSourceSelect, kRegShiftHDMIOutSourceSelect);
		WriteRegister (kRegHDMIOutControl, channelSelect, kRegMaskHDMIOut8ChGroupSelect, kRegShiftHDMIOut8ChGroupSelect);
		return SetHDMIOutAudioChannels(NTV2_HDMIAudio8Channels);
	}
	else
	{
		ULWord	encoding	(0);
		ULWord	ch			(ULWord (inAudioSystem) << 2);
		if (inValue == NTV2_AudioChannel1_8)
			encoding = (ch + NTV2_AudioChannel1_4) | ((ch + NTV2_AudioChannel5_8) << 4);
		else
			encoding = (ch + NTV2_AudioChannel9_12) | ((ch + NTV2_AudioChannel13_16) << 4);

		return WriteRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource);
	}
}


bool CNTV2Card::GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect & outValue, NTV2AudioSystem & outAudioSystem)
{
	bool result = false;
	if(NTV2DeviceGetHDMIVersion(GetDeviceID()) > 3)
	{
		ULWord engineSelect (0), channelSelect(0);
		result = ReadRegister(kRegHDMIOutControl, channelSelect, kRegMaskHDMIOut8ChGroupSelect, kRegShiftHDMIOut8ChGroupSelect);
		if (result)
		{
			outValue = channelSelect == 1 ? NTV2_AudioChannel9_16 : NTV2_AudioChannel1_8;
			result = ReadRegister(kRegHDMIInputControl, engineSelect,  kRegMaskHDMIOutSourceSelect, kRegShiftHDMIOutSourceSelect);
			outAudioSystem = static_cast <NTV2AudioSystem> (engineSelect);
		}
	}
	else
	{
		ULWord	encoding	(0);
		result = ReadRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource);
		if (result)
		{
			if ((encoding & 0x3) == static_cast <ULWord> (NTV2_AudioChannel1_4))
				outValue = NTV2_AudioChannel1_8;
			else
				outValue = NTV2_AudioChannel9_16;

			outAudioSystem = static_cast <NTV2AudioSystem> ((encoding & 0xC) >> 2);
		}
	}
	return result;
}


bool CNTV2Card::SetHDMIOutAudioRate (const NTV2AudioRate inNewValue)
{
	return WriteRegister (kRegHDMIInputControl, static_cast <ULWord> (inNewValue), kRegMaskHDMIOutAudioRate, kRegShiftHDMIOutAudioRate);
}


bool CNTV2Card::GetHDMIOutAudioRate (NTV2AudioRate & outValue)
{
	return CNTV2DriverInterface::ReadRegister (kRegHDMIInputControl, outValue, kRegMaskHDMIOutAudioRate, kRegShiftHDMIOutAudioRate);
}


bool CNTV2Card::SetHDMIOutAudioFormat (const NTV2AudioFormat inNewValue)
{
	return WriteRegister (kRegHDMIInputControl, static_cast <ULWord> (inNewValue), kRegMaskHDMIOutAudioFormat, kRegShiftHDMIOutAudioFormat);
}


bool CNTV2Card::GetHDMIOutAudioFormat (NTV2AudioFormat & outValue)
{
	return CNTV2DriverInterface::ReadRegister (kRegHDMIInputControl, outValue, kRegMaskHDMIOutAudioFormat, kRegShiftHDMIOutAudioFormat);
}


bool CNTV2Card::SetAudioOutputMonitorSource (const NTV2AudioChannelPair inChannelPair, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(inChannelPair))
		return false;
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem))
		return false;
	const ULWord encoding ((ULWord(inAudioSystem) << 4) | inChannelPair);
    return WriteRegister (kRegAudioOutputSourceMap, encoding, kRegMaskMonitorSource, kRegShiftMonitorSource);
}


bool CNTV2Card::GetAudioOutputMonitorSource (NTV2AudioChannelPair & outChannelPair, NTV2AudioSystem & outAudioSystem)
{
	ULWord encoding (0);
    bool result = ReadRegister (kRegAudioOutputSourceMap, encoding, kRegMaskMonitorSource, kRegShiftMonitorSource);
	if (result)
	{
		outChannelPair = NTV2AudioChannelPair(encoding & 0xF);
		outAudioSystem = NTV2AudioSystem(encoding >> 4);
	}
	return result;
}

#if !defined(NTV2_DEPRECATE_14_3)
bool CNTV2Card::GetAudioOutputMonitorSource (NTV2AudioMonitorSelect * pOutValue, NTV2Channel * pOutChannel)
{
	NTV2AudioChannelPair	outValue	(NTV2_AudioMonitor1_2);
	NTV2AudioSystem			outChannel	(NTV2_NUM_AUDIOSYSTEMS);
	const bool				result		(GetAudioOutputMonitorSource (outValue, outChannel));
	if (result)
	{
		if (pOutValue)
			*pOutValue = NTV2AudioMonitorSelect(outValue);
		if (pOutChannel)
			*pOutChannel = NTV2Channel(outChannel);
	}
	return result;
}
#endif	//	NTV2_DEPRECATE_14_3


bool CNTV2Card::CanDoAudioWaitForVBI (void)
{
	ULWord canDo(0);
	return ReadRegister(kRegCanDoStatus, canDo, kRegMaskCanDoAudioWaitForVBI, kRegShiftCanDoAudioWaitForVBI)
			&&  (canDo ? true : false);
}

bool CNTV2Card::StartAudioOutput (const NTV2AudioSystem inAudioSystem, const bool inWaitForVBI)
{
	if (UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID))
		return false;	//	Bad AudioSystem
	const ULWord audioCtrlRegNum(gAudioSystemToAudioControlRegNum[inAudioSystem]);
	if (inWaitForVBI)
	{
		if (!CanDoAudioWaitForVBI())
			return false;	//	Caller requested wait-til-VBI, but firmware doesn't support it
		//	Set or clear the start-at-VBI bit...
		if (!WriteRegister(audioCtrlRegNum, inWaitForVBI ? 1UL : 0UL,  kRegMaskOutputStartAtVBI, kRegShiftOutputStartAtVBI))
			return false;
	}
	return WriteRegister (audioCtrlRegNum, 0, kRegMaskResetAudioOutput, kRegShiftResetAudioOutput);
}

bool CNTV2Card::StopAudioOutput (const NTV2AudioSystem inAudioSystem)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
			&&  WriteRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], 1, kRegMaskResetAudioOutput, kRegShiftResetAudioOutput);
}

bool CNTV2Card::IsAudioOutputRunning (const NTV2AudioSystem inAudioSystem, bool & outIsRunning)
{
	bool	isStopped	(true);
	bool	result	(UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
					&& CNTV2DriverInterface::ReadRegister (gAudioSystemToAudioControlRegNum[inAudioSystem],
															isStopped, kRegMaskResetAudioOutput, kRegShiftResetAudioOutput));
	if (result)
		outIsRunning = !isStopped;
	return result;
}


bool CNTV2Card::SetAudio20BitMode (const NTV2AudioSystem inAudioSystem, const bool inEnable)
{
	return ::NTV2DeviceCanDoIP(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], inEnable ? 1 : 0, kRegMask20BitMode, kRegShift20BitMode);
}


bool CNTV2Card::GetAudio20BitMode (const NTV2AudioSystem inAudioSystem, bool & outEnable)
{
	return ::NTV2DeviceCanDoIP(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& CNTV2DriverInterface::ReadRegister(gAudioSystemToAudioControlRegNum[inAudioSystem], outEnable, kRegMask20BitMode, kRegShift20BitMode);
}


bool CNTV2Card::SetAudioOutputPause (const NTV2AudioSystem inAudioSystem, const bool inEnable)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], inEnable ? 1 : 0, kRegMaskPauseAudio, kRegShiftPauseAudio);
}


bool CNTV2Card::GetAudioOutputPause (const NTV2AudioSystem inAudioSystem, bool & outEnable)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& CNTV2DriverInterface::ReadRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], outEnable, kRegMaskPauseAudio, kRegShiftPauseAudio);
}


bool CNTV2Card::StartAudioInput (const NTV2AudioSystem inAudioSystem, const bool inWaitForVBI)
{
	if (UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID))
		return false;	//	Bad AudioSystem
	const ULWord audioCtrlRegNum(gAudioSystemToAudioControlRegNum[inAudioSystem]);
	if (inWaitForVBI)
	{
		if (!CanDoAudioWaitForVBI())
			return false;	//	Caller requested wait-til-VBI, but firmware doesn't support it
		//	Set or clear the start-at-VBI bit...
		if (!WriteRegister(audioCtrlRegNum, inWaitForVBI ? 1UL : 0UL,  kRegMaskInputStartAtVBI, kRegShiftInputStartAtVBI))
			return false;
	}
	return WriteRegister (audioCtrlRegNum, 0, kRegMaskResetAudioInput, kRegShiftResetAudioInput);
}


bool CNTV2Card::StopAudioInput (const NTV2AudioSystem inAudioSystem)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioSystemToAudioControlRegNum [inAudioSystem], 1, kRegMaskResetAudioInput, kRegShiftResetAudioInput);
}


bool CNTV2Card::IsAudioInputRunning (const NTV2AudioSystem inAudioSystem, bool & outIsRunning)
{
	bool	isStopped	(true);
	bool	result	(UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
					&& CNTV2DriverInterface::ReadRegister (gAudioSystemToAudioControlRegNum[inAudioSystem],
															isStopped, kRegMaskResetAudioInput, kRegShiftResetAudioInput));
	if (result)
		outIsRunning = !isStopped;
	return result;
}


bool CNTV2Card::SetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], inEnable ? 1 : 0, kRegMaskCaptureEnable, kRegShiftCaptureEnable);
}


bool CNTV2Card::GetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& CNTV2DriverInterface::ReadRegister (gAudioSystemToAudioControlRegNum[inAudioSystem], outEnable, kRegMaskCaptureEnable, kRegShiftCaptureEnable);
}


bool CNTV2Card::SetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (kRegGlobalControl2, inEnable ? 1 : 0, gAudioPlayCaptureModeMasks[inAudioSystem], gAudioPlayCaptureModeShifts[inAudioSystem]);
}


bool CNTV2Card::GetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable)
{
	outEnable = false;
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& CNTV2DriverInterface::ReadRegister (kRegGlobalControl2, outEnable, gAudioPlayCaptureModeMasks[inAudioSystem], gAudioPlayCaptureModeShifts[inAudioSystem]);
}


bool CNTV2Card::SetAudioInputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay)
{
	return ::NTV2DeviceCanDoAudioDelay(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioDelayRegisterNumbers [inAudioSystem], inDelay, kRegMaskAudioInDelay, kRegShiftAudioInDelay);
}


bool CNTV2Card::GetAudioInputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay)
{
	return ::NTV2DeviceCanDoAudioDelay(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& ReadRegister (gAudioDelayRegisterNumbers[inAudioSystem], outDelay, kRegMaskAudioInDelay, kRegShiftAudioInDelay);
}


bool CNTV2Card::SetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay)
{
	return ::NTV2DeviceCanDoAudioDelay(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioDelayRegisterNumbers[inAudioSystem], inDelay, kRegMaskAudioOutDelay, kRegShiftAudioOutDelay);
}


bool CNTV2Card::GetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay)
{
	return ::NTV2DeviceCanDoAudioDelay(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& ReadRegister (gAudioDelayRegisterNumbers[inAudioSystem], outDelay, kRegMaskAudioOutDelay, kRegShiftAudioOutDelay);
}


bool CNTV2Card::SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const bool inNonPCM)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& WriteRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], inNonPCM ? 1 : 0, BIT(17), 17);
}


bool CNTV2Card::GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, bool & outIsNonPCM)
{
	return UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& CNTV2DriverInterface::ReadRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], outIsNonPCM, BIT(17), 17);
}


bool CNTV2Card::SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelSelect, bool inNonPCM)
{
	return ::NTV2DeviceCanDoPCMControl(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& NTV2_IS_VALID_AUDIO_CHANNEL_PAIR(inChannelSelect)
		&& WriteRegister(gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlReg,
						inNonPCM ? 1 : 0,
						gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlMask,
						gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlShift);
}


bool CNTV2Card::SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPairs & inNonPCMChannelPairs)
{
	if (!::NTV2DeviceCanDoPCMControl(_boardID) || UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID))
		return false;

	bool	result	(true);
	for (NTV2AudioChannelPair chanPair (NTV2_AudioChannel1_2);  NTV2_IS_VALID_AUDIO_CHANNEL_PAIR (chanPair);  chanPair = NTV2AudioChannelPair (chanPair + 1))
	{
		if (NTV2_IS_EXTENDED_AUDIO_CHANNEL_PAIR (chanPair))
			break;	//	Extended audio channels not yet supported

		const bool	isNonPCM	(inNonPCMChannelPairs.find (chanPair) != inNonPCMChannelPairs.end ());
		result = WriteRegister (gAudioEngineChannelPairToFieldInformation[inAudioSystem][chanPair].pcmControlReg,  isNonPCM ? 1 : 0,
								gAudioEngineChannelPairToFieldInformation[inAudioSystem][chanPair].pcmControlMask,
								gAudioEngineChannelPairToFieldInformation[inAudioSystem][chanPair].pcmControlShift);
		if (!result)
			break;
	}
	return result;
}


bool CNTV2Card::GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelSelect, bool & outIsNonPCM)
{
	return ::NTV2DeviceCanDoPCMControl(_boardID)
		&& UWord(inAudioSystem) < ::NTV2DeviceGetNumAudioSystems(_boardID)
		&& NTV2_IS_VALID_AUDIO_CHANNEL_PAIR(inChannelSelect)
		&& CNTV2DriverInterface::ReadRegister (gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlReg,
						outIsNonPCM,
						gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlMask,
						gAudioEngineChannelPairToFieldInformation[inAudioSystem][inChannelSelect].pcmControlShift);
}


bool CNTV2Card::GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outNonPCMChannelPairs)
{
	ULWord	numAudioChannels	(0);
	bool	isNonPCM			(false);

	outNonPCMChannelPairs.clear ();
	if (UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID))
		return false;	//	no such audio system on this device
	if (!GetNumberAudioChannels (numAudioChannels, inAudioSystem))
		return false;	//	fail

	const NTV2AudioChannelPair	maxPair	(NTV2AudioChannelPair(numAudioChannels/2));
	if (!GetAudioPCMControl (inAudioSystem, isNonPCM))
		return false;	//	fail

	if (isNonPCM)	//	this global mode overrides per-channel PCM control
	{
		for (UWord chPair (0);  chPair <= maxPair;  chPair++)
			outNonPCMChannelPairs.insert (NTV2AudioChannelPair (chPair));
		return true;	//	done
	}

	if (::NTV2DeviceCanDoPCMControl(_boardID))
	{
		ULWord	regVal	(0);
		if (!ReadRegister (inAudioSystem < NTV2_AUDIOSYSTEM_5 ? kRegPCMControl4321 : kRegPCMControl8765, regVal))
			return false;
		for (NTV2AudioChannelPair chanPair(NTV2_AudioChannel1_2);  NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chanPair);  chanPair = NTV2AudioChannelPair(chanPair + 1))
			if (regVal & BIT(inAudioSystem * 8 + chanPair))
				outNonPCMChannelPairs.insert (chanPair);
	}
	return true;
}


//													NTV2_AUDIOSYSTEM_1		NTV2_AUDIOSYSTEM_2		NTV2_AUDIOSYSTEM_3		NTV2_AUDIOSYSTEM_4
static const ULWord		sAudioDetectRegs []		= {	kRegAud1Detect,			kRegAud1Detect,			kRegAudDetect2,			kRegAudDetect2,
//													NTV2_AUDIOSYSTEM_5		NTV2_AUDIOSYSTEM_6		NTV2_AUDIOSYSTEM_7		NTV2_AUDIOSYSTEM_8
													kRegAudioDetect5678,	kRegAudioDetect5678,	kRegAudioDetect5678,	kRegAudioDetect5678	};

//													NTV2_AUDIOSYSTEM_1		NTV2_AUDIOSYSTEM_2		NTV2_AUDIOSYSTEM_3		NTV2_AUDIOSYSTEM_4
static const unsigned	sAudioDetectGroups []	= {	0,						1,						0,						1,
//													NTV2_AUDIOSYSTEM_5		NTV2_AUDIOSYSTEM_6		NTV2_AUDIOSYSTEM_7		NTV2_AUDIOSYSTEM_8
													0,						1,						2,						3					};

bool CNTV2Card::IsAudioChannelPairPresent (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, bool & outIsPresent)
{
	NTV2AudioChannelPairs	activeChannelPairs;
	outIsPresent = false;
	if (!GetDetectedAudioChannelPairs (inAudioSystem, activeChannelPairs))
		return false;
	if (activeChannelPairs.find (inChannelPair) != activeChannelPairs.end ())
		outIsPresent = true;
	return true;
}


bool CNTV2Card::GetDetectedAudioChannelPairs (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outDetectedChannelPairs)
{
	outDetectedChannelPairs.clear ();
	if (UWord(inAudioSystem) >= ::NTV2DeviceGetNumAudioSystems(_boardID))
		return false;

	ULWord	detectBits	(0);
	if (!ReadRegister (sAudioDetectRegs[inAudioSystem], detectBits))
		return false;

	const unsigned	bitGroup (sAudioDetectGroups[inAudioSystem]);
	for (NTV2AudioChannelPair chanPair(NTV2_AudioChannel1_2);  NTV2_IS_WITHIN_AUDIO_CHANNELS_1_TO_16(chanPair);  chanPair = NTV2AudioChannelPair(chanPair + 1))
		if (detectBits & BIT(bitGroup * 8 + chanPair))
			outDetectedChannelPairs.insert (chanPair);
	return true;
}


bool CNTV2Card::GetDetectedAESChannelPairs (NTV2AudioChannelPairs & outDetectedChannelPairs)
{
	uint32_t	valLo8(0),	valHi8(0);
	outDetectedChannelPairs.clear ();
	if (!::NTV2DeviceCanDoAESAudioIn(_boardID))
		return false;
	if (!ReadRegister(kRegInputStatus, valLo8))			//	Reg 22, bits 24..27
		return false;
	if (!ReadRegister(kRegAud1SourceSelect, valHi8))	//	Reg 25, bits 28..31
		return false;

	const uint32_t	detectBits	(((valLo8 >> 24) & 0x0000000F)  |  ((valHi8 >> 24) & 0x000000F0));
	for (NTV2AudioChannelPair chPair (NTV2_AudioChannel1_2);  chPair < NTV2_AudioChannel15_16;  chPair = NTV2AudioChannelPair(chPair+1))
		if (!(detectBits & BIT(chPair)))	//	bit set means "not connected"
			outDetectedChannelPairs.insert(chPair);
	return true;
}


bool CNTV2Card::SetSuspendHostAudio (const bool inIsSuspended)
{
	return WriteRegister (kVRegSuspendSystemAudio, ULWord(inIsSuspended));
}


bool CNTV2Card::GetSuspendHostAudio (bool & outIsSuspended)
{
	return CNTV2DriverInterface::ReadRegister (kVRegSuspendSystemAudio, outIsSuspended);
}


//	GetAESOutputSource / SetAESOutputSource:
//
//	Register 190 (kRegAudioOutputSourceMap) does the mapping by audio group (audio channel quads).
//	Each of the least-significant 4 nibbles correspond to AES output audio channel quads:
//
//		RegMask			NTV2Audio4ChannelSelect
//		==========		=======================
//		0x0000000F		NTV2_AudioChannel1_4
//		0x000000F0		NTV2_AudioChannel5_8
//		0x00000F00		NTV2_AudioChannel9_12
//		0x0000F000		NTV2_AudioChannel13_16
//
//	The value of the nibble determines the source:
//		Value	NTV2AudioSystem		NTV2Audio4ChannelSelect
//		=====	===============		=======================
//		0x0:	NTV2_AUDIOSYSTEM_1	NTV2_AudioChannel1_4
//		0x1:	NTV2_AUDIOSYSTEM_1	NTV2_AudioChannel5_8
//		0x2:	NTV2_AUDIOSYSTEM_1	NTV2_AudioChannel9_12
//		0x3:	NTV2_AUDIOSYSTEM_1	NTV2_AudioChannel13_16
//		0x4:	NTV2_AUDIOSYSTEM_2	NTV2_AudioChannel1_4
//		0x5:	NTV2_AUDIOSYSTEM_2	NTV2_AudioChannel5_8
//		0x6:	NTV2_AUDIOSYSTEM_2	NTV2_AudioChannel9_12
//		0x7:	NTV2_AUDIOSYSTEM_2	NTV2_AudioChannel13_16
//		0x8:	NTV2_AUDIOSYSTEM_3	NTV2_AudioChannel1_4
//		0x9:	NTV2_AUDIOSYSTEM_3	NTV2_AudioChannel5_8
//		0xA:	NTV2_AUDIOSYSTEM_3	NTV2_AudioChannel9_12
//		0xB:	NTV2_AUDIOSYSTEM_3	NTV2_AudioChannel13_16
//		0xC:	NTV2_AUDIOSYSTEM_4	NTV2_AudioChannel1_4
//		0xD:	NTV2_AUDIOSYSTEM_4	NTV2_AudioChannel5_8
//		0xE:	NTV2_AUDIOSYSTEM_4	NTV2_AudioChannel9_12
//		0xF:	NTV2_AUDIOSYSTEM_4	NTV2_AudioChannel13_16

static const unsigned	gAESChannelMappingShifts [4]	=	{0, 4, 8, 12};


bool CNTV2Card::GetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, NTV2AudioSystem & outSrcAudioSystem, NTV2Audio4ChannelSelect & outSrcAudioChannels)
{
	const UWord	numAESAudioOutputChannels	(::NTV2DeviceGetNumAESAudioOutputChannels(_boardID));
	const UWord	maxNumAudioChannelsForQuad	((inAESAudioChannels + 1) * 4);

	outSrcAudioSystem	= NTV2_AUDIOSYSTEM_INVALID;
	outSrcAudioChannels	= NTV2_AUDIO_CHANNEL_QUAD_INVALID;

	if (numAESAudioOutputChannels < 4)
		return false;	//	Fail, device doesn't support AES output
	if (maxNumAudioChannelsForQuad > numAESAudioOutputChannels)
		return false;	//	Fail, illegal inAESAudioChannels value

	ULWord	regValue	(0);
	if (!ReadRegister (kRegAudioOutputSourceMap, regValue))
		return false;	//	Failed in ReadRegister

	regValue = (regValue >> gAESChannelMappingShifts[inAESAudioChannels]) & 0x0000000F;
	outSrcAudioSystem = NTV2AudioSystem(regValue / 4);
	NTV2_ASSERT (NTV2_IS_VALID_AUDIO_SYSTEM (outSrcAudioSystem));

	outSrcAudioChannels = NTV2Audio4ChannelSelect(regValue % 4);
	NTV2_ASSERT (NTV2_IS_NORMAL_AUDIO_CHANNEL_QUAD (outSrcAudioChannels));
	return true;
}


bool CNTV2Card::SetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, const NTV2AudioSystem inSrcAudioSystem, const NTV2Audio4ChannelSelect inSrcAudioChannels)
{
	const ULWord	nibble	(ULWord(inSrcAudioSystem) * 4  +  ULWord(inSrcAudioChannels));
	return WriteRegister (kRegAudioOutputSourceMap,											//	reg
							nibble,															//	value
							ULWord(0xF << gAESChannelMappingShifts[inAESAudioChannels]),	//	mask
							gAESChannelMappingShifts[inAESAudioChannels]);					//	shift
}


static NTV2AudioChannelPairs BitMasksToNTV2AudioChannelPairs (const ULWord inBitMask, const ULWord inExtendedBitMask)
{
	NTV2AudioChannelPairs	result;
	if (inBitMask)
		for (NTV2AudioChannelPair channelPair (NTV2_AudioChannel1_2);  channelPair < NTV2_AudioChannel17_18;  channelPair = NTV2AudioChannelPair (channelPair + 1))
			if (inBitMask & BIT (channelPair))
				result.insert (channelPair);
	if (inExtendedBitMask)
		for (NTV2AudioChannelPair channelPair (NTV2_AudioChannel17_18);  channelPair < NTV2_MAX_NUM_AudioChannelPair;  channelPair = NTV2AudioChannelPair (channelPair + 1))
			if (inExtendedBitMask & BIT (channelPair))
				result.insert (channelPair);
	return result;
}


static inline NTV2RegisterNumber GetNonPCMDetectRegisterNumber (const NTV2Channel inSDIInputChannel, const bool inIsExtended = false)
{
	return NTV2RegisterNumber (kRegFirstNonPCMAudioDetectRegister  +  inSDIInputChannel * 2  +  (inIsExtended ? 1 : 0));
}


bool CNTV2Card::InputAudioChannelPairHasPCM (const NTV2Channel inSDIInputChannel, const NTV2AudioChannelPair inAudioChannelPair, bool & outHasPCM)
{
	if (!NTV2_IS_VALID_AUDIO_CHANNEL_PAIR (inAudioChannelPair))
		return false;
	NTV2AudioChannelPairs	withPCMs;
	if (!GetInputAudioChannelPairsWithPCM (inSDIInputChannel, withPCMs))
		return false;

	outHasPCM = withPCMs.find (inAudioChannelPair) != withPCMs.end ();	//	Test set membership
	return true;
}


bool CNTV2Card::GetInputAudioChannelPairsWithPCM (const NTV2Channel inSDIInputChannel, NTV2AudioChannelPairs & outPCMPairs)
{
	outPCMPairs.clear ();
	if (!::NTV2DeviceCanDoPCMDetection (_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL (inSDIInputChannel))
		return false;
	if (inSDIInputChannel >= ::NTV2DeviceGetNumVideoInputs (_boardID))
		return false;

	//	Read channel pair bitmask registers...
	const UWord					numChannels	(::NTV2DeviceGetMaxAudioChannels(_boardID));
	const bool					isExtended	(numChannels > 16);
	const NTV2RegisterNumber	regNum		(::GetNonPCMDetectRegisterNumber (inSDIInputChannel));
	ULWord						mask		(0);
	ULWord						extMask		(0);
	if (!ReadRegister (regNum, mask))
		return false;
	if (isExtended)
		if (!ReadRegister (regNum + 1, extMask))
			return false;

	//	Convert bitmasks to set of with-PCM pairs...
	outPCMPairs = ::BitMasksToNTV2AudioChannelPairs (~mask,  isExtended ? ~extMask : 0);
	return true;
}


bool CNTV2Card::GetInputAudioChannelPairsWithoutPCM (const NTV2Channel inSDIInputChannel, NTV2AudioChannelPairs & outNonPCMPairs)
{
	outNonPCMPairs.clear ();
	if (!::NTV2DeviceCanDoPCMDetection (_boardID))
		return false;
	if (!NTV2_IS_VALID_CHANNEL (inSDIInputChannel))
		return false;
	if (inSDIInputChannel >= ::NTV2DeviceGetNumVideoInputs (_boardID))
		return false;

	//	Read channel pair bitmask registers...
	const UWord					numChannels	(::NTV2DeviceGetMaxAudioChannels(_boardID));
	const bool					isExtended	(numChannels > 16);
	const NTV2RegisterNumber	regNum	(::GetNonPCMDetectRegisterNumber (inSDIInputChannel));
	ULWord						mask	(0);
	ULWord						extMask	(0);
	if (!ReadRegister (regNum, mask))
		return false;
	if (isExtended)
		if (!ReadRegister (regNum + 1, extMask))
			return false;

	//	Convert bitmasks to set of non-PCM pairs...
	outNonPCMPairs = ::BitMasksToNTV2AudioChannelPairs (mask, isExtended ? extMask : 0);
	return true;
}


static const ULWord	kAudCtrlRegsForSDIOutputs []	=	{	kRegAud1Control,	kRegAud1Control,	kRegAud3Control,	kRegAud3Control,
															kRegAud5Control,	kRegAud5Control,	kRegAud7Control,	kRegAud7Control	};

bool CNTV2Card::GetAudioOutputEmbedderState (const NTV2Channel inSDIOutputSpigot, bool & outIsEnabled)
{
	outIsEnabled = true;	//	presume normal
	if (!NTV2_IS_VALID_CHANNEL (inSDIOutputSpigot))
		return false;
	if (UWord(inSDIOutputSpigot) >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
		return false;

	ULWord	value	(0);
	if (!ReadRegister (kAudCtrlRegsForSDIOutputs[inSDIOutputSpigot],  value,
						(inSDIOutputSpigot & 1) ? kRegMaskEmbeddedOutputSupressCh2 : kRegMaskEmbeddedOutputSupressCh1,
						(inSDIOutputSpigot & 1) ? kRegShiftEmbeddedOutputSupressCh2 : kRegShiftEmbeddedOutputSupressCh1))
		return false;
	outIsEnabled = value ? false : true;	//	Bit sense is 1=disabled, 0=enabled/normal
	return true;
}

bool CNTV2Card::SetAudioOutputEmbedderState (const NTV2Channel inSDIOutputSpigot, const bool & inEnable)
{
	if (!NTV2_IS_VALID_CHANNEL (inSDIOutputSpigot))
		return false;
	if (UWord(inSDIOutputSpigot) >= ::NTV2DeviceGetNumVideoOutputs(_boardID))
		return false;

	return WriteRegister (kAudCtrlRegsForSDIOutputs[inSDIOutputSpigot],  inEnable ? 0 : 1,
						(inSDIOutputSpigot & 1) ? kRegMaskEmbeddedOutputSupressCh2 : kRegMaskEmbeddedOutputSupressCh1,
						(inSDIOutputSpigot & 1) ? kRegShiftEmbeddedOutputSupressCh2 : kRegShiftEmbeddedOutputSupressCh1);
}


bool CNTV2Card::GetAudioOutputEraseMode (const NTV2AudioSystem inAudioSystem, bool & outEraseModeEnabled)
{
	outEraseModeEnabled = false;
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;
	if (inAudioSystem >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;
	ULWord	regValue	(0);
	if (!ReadRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], regValue))
		return false;
	outEraseModeEnabled = (regValue & kRegMaskAudioAutoErase) ? true : false;
	return true;
}


bool CNTV2Card::SetAudioOutputEraseMode (const NTV2AudioSystem inAudioSystem, const bool & inEraseModeEnabled)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;
	if (inAudioSystem >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;
	return WriteRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], inEraseModeEnabled ? 1 : 0, kRegMaskAudioAutoErase, kRegShiftAudioAutoErase);
}

bool CNTV2Card::SetAnalogAudioTransmitEnable (const NTV2Audio4ChannelSelect inChannelQuad, const bool inXmitEnable)
{
	//	Reg 108 (kRegGlobalControl3) has two bits for controlling XLR direction:  BIT(0) for XLRs 1-4,  BIT(1) for XLRs 5-8
	if (!NTV2DeviceHasBiDirectionalAnalogAudio(_boardID))
		return false;	//	unsupported
	if (inChannelQuad > NTV2_AudioChannel5_8)
		return false;	//	NTV2_AudioChannel1_4 & NTV2_AudioChannel5_8 only
	return WriteRegister (kRegGlobalControl3,  inXmitEnable ? 0 : 1,	//	0 == xmit		1 == recv
							inChannelQuad == NTV2_AudioChannel1_4  ?  kRegMaskAnalogIOControl_14  :  kRegMaskAnalogIOControl_58,
							ULWord(inChannelQuad));
}

bool CNTV2Card::GetAnalogAudioTransmitEnable (const NTV2Audio4ChannelSelect inChannelQuad, bool & outXmitEnabled)
{
	outXmitEnabled = false;
	//	Reg 108 (kRegGlobalControl3) has two bits for controlling XLR direction:  BIT(0) for XLRs 1-4,  BIT(1) for XLRs 5-8
	if (!NTV2DeviceHasBiDirectionalAnalogAudio(_boardID))
		return false;	//	unsupported
	if (inChannelQuad > NTV2_AudioChannel5_8)
		return false;	//	NTV2_AudioChannel1_4 & NTV2_AudioChannel5_8 only
	if (!CNTV2DriverInterface::ReadRegister (kRegGlobalControl3,  outXmitEnabled,		//	false == xmit		true == recv
											inChannelQuad == NTV2_AudioChannel1_4  ?  kRegMaskAnalogIOControl_14  :  kRegMaskAnalogIOControl_58,
											ULWord(inChannelQuad)))
		return false;
	outXmitEnabled = !outXmitEnabled;	//	Flip the sense, we want xmit == true,  recv == false
	return true;
}

#if !defined(NTV2_DEPRECATE_16_1)
	bool CNTV2Card::SetAnalogAudioIOConfiguration (const NTV2AnalogAudioIO inConfig)
	{
		if (inConfig > NTV2_AnalogAudioIO_8In)
			return false;
		return SetAnalogAudioTransmitEnable (NTV2_AudioChannel1_4, inConfig == NTV2_AnalogAudioIO_8Out || inConfig == NTV2_AnalogAudioIO_4Out_4In)
			&& SetAnalogAudioTransmitEnable (NTV2_AudioChannel5_8, inConfig == NTV2_AnalogAudioIO_8Out || inConfig == NTV2_AnalogAudioIO_4In_4Out);
	}

	bool CNTV2Card::GetAnalogAudioIOConfiguration (NTV2AnalogAudioIO & outConfig)
	{
		bool xlr14Xmit(false), xlr58Xmit(false);
		if (!GetAnalogAudioTransmitEnable (NTV2_AudioChannel1_4, xlr14Xmit))
			return false;
		if (!GetAnalogAudioTransmitEnable (NTV2_AudioChannel5_8, xlr58Xmit))
			return false;
		if (xlr14Xmit && xlr58Xmit)
			outConfig = NTV2_AnalogAudioIO_8Out;
		else if (xlr14Xmit && !xlr58Xmit)
			outConfig = NTV2_AnalogAudioIO_4Out_4In;
		else if (!xlr14Xmit && xlr58Xmit)
			outConfig = NTV2_AnalogAudioIO_4In_4Out;
		else
			outConfig = NTV2_AnalogAudioIO_8In;
		return true;
	}
#endif	//	!defined(NTV2_DEPRECATE_16_1)

bool CNTV2Card::GetAudioOutputAESSyncModeBit (const NTV2AudioSystem inAudioSystem, bool & outAESSyncModeBitSet)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;
	if (inAudioSystem >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;
	ULWord	regValue(0);
	if (!ReadRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], regValue, BIT(18), 18))
		return false;
	outAESSyncModeBitSet = regValue ? true : false;
	return true;
}

bool CNTV2Card::SetAudioOutputAESSyncModeBit (const NTV2AudioSystem inAudioSystem, const bool & inAESSyncModeBitSet)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem))
		return false;
	if (inAudioSystem >= (::NTV2DeviceGetNumAudioSystems (_boardID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;
	return WriteRegister(gAudioSystemToSrcSelectRegNum[inAudioSystem], inAESSyncModeBitSet?1:0, BIT(18), 18);
}

bool CNTV2Card::GetRawAudioTimer (ULWord & outValue, const NTV2AudioSystem inAudioSystem)
{
	if (!NTV2_IS_VALID_AUDIO_SYSTEM(inAudioSystem))
		return false;
	return ReadRegister(kRegAud1Counter, outValue);
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::GetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, bool * pOutEnable)
	{
		return pOutEnable ? GetAudioPlayCaptureModeEnable (inAudioSystem, *pOutEnable) : false;
	}

	bool CNTV2Card::GetAudioInputDelay (const NTV2AudioSystem inAudioSystem, ULWord * pOutDelay)
	{
		return pOutDelay ? GetAudioInputDelay (inAudioSystem, *pOutDelay) : false;
	}

	bool CNTV2Card::GetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, ULWord * pOutDelay)
	{
		return pOutDelay ? GetAudioOutputDelay (inAudioSystem, *pOutDelay) : false;
	}

	bool CNTV2Card::SetSDIOutDS2AudioSource (const ULWord inValue, const NTV2Channel inChannel)
	{
		return SetSDIOutputDS2AudioSystem (inChannel, static_cast <NTV2AudioSystem> (inValue));
	}

	bool CNTV2Card::GetSDIOutDS2AudioSource (ULWord & outValue, const NTV2Channel inChannel)
	{
		NTV2AudioSystem	audioSystem	(NTV2_NUM_AUDIOSYSTEMS);
		const bool			result		(GetSDIOutputDS2AudioSystem (inChannel, audioSystem));
		outValue = static_cast <ULWord> (audioSystem);
		return result;
	}
	bool CNTV2Card::SetK2SDI1OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL1);}
	bool CNTV2Card::SetK2SDI2OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL2);}
	bool CNTV2Card::SetK2SDI3OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL3);}
	bool CNTV2Card::SetK2SDI4OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL4);}
	bool CNTV2Card::SetK2SDI5OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL5);}
	bool CNTV2Card::SetK2SDI6OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL6);}
	bool CNTV2Card::SetK2SDI7OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL7);}
	bool CNTV2Card::SetK2SDI8OutDS2AudioSource (ULWord value)	{return SetSDIOutDS2AudioSource (value, NTV2_CHANNEL8);}

	bool CNTV2Card::GetK2SDI1OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetK2SDI2OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetK2SDI3OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetK2SDI4OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetK2SDI5OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetK2SDI6OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetK2SDI7OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetK2SDI8OutDS2AudioSource (ULWord* value)	{return value ? GetSDIOutDS2AudioSource (*value, NTV2_CHANNEL8) : false;}

	bool CNTV2Card::SetSDIOutAudioSource (const ULWord inValue, const NTV2Channel inChannel)
	{
		return SetSDIOutputAudioSystem (inChannel, static_cast <NTV2AudioSystem> (inValue));
	}

	bool CNTV2Card::GetSDIOutAudioSource (ULWord & outValue, const NTV2Channel inChannel)
	{
		NTV2AudioSystem	audioSystem	(NTV2_NUM_AUDIOSYSTEMS);
		const bool			result		(GetSDIOutputAudioSystem (inChannel, audioSystem));
		outValue = static_cast <ULWord> (audioSystem);
		return result;
	}

	bool CNTV2Card::SetK2SDI1OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL1);}
	bool CNTV2Card::SetK2SDI2OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL2);}
	bool CNTV2Card::SetK2SDI3OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL3);}
	bool CNTV2Card::SetK2SDI4OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL4);}
	bool CNTV2Card::SetK2SDI5OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL5);}
	bool CNTV2Card::SetK2SDI6OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL6);}
	bool CNTV2Card::SetK2SDI7OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL7);}
	bool CNTV2Card::SetK2SDI8OutAudioSource (ULWord value)	{return SetSDIOutAudioSource (value, NTV2_CHANNEL8);}

	bool CNTV2Card::GetK2SDI1OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL1) : false;}
	bool CNTV2Card::GetK2SDI2OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL2) : false;}
	bool CNTV2Card::GetK2SDI3OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL3) : false;}
	bool CNTV2Card::GetK2SDI4OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL4) : false;}
	bool CNTV2Card::GetK2SDI5OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL5) : false;}
	bool CNTV2Card::GetK2SDI6OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL6) : false;}
	bool CNTV2Card::GetK2SDI7OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL7) : false;}
	bool CNTV2Card::GetK2SDI8OutAudioSource (ULWord* value)	{return value ? GetSDIOutAudioSource (*value, NTV2_CHANNEL8) : false;}

	bool CNTV2Card::SetNumberAudioChannels	(ULWord						numChannels,		NTV2Channel channel)	{return SetNumberAudioChannels (numChannels, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetNumberAudioChannels	(ULWord *					pOutNumChannels,	NTV2Channel channel)	{return pOutNumChannels	? GetNumberAudioChannels (*pOutNumChannels, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::SetAudioRate			(NTV2AudioRate				value,				NTV2Channel channel)	{return SetAudioRate (value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetAudioRate			(NTV2AudioRate *			pOutRate,			NTV2Channel channel)	{return pOutRate	? GetAudioRate (*pOutRate, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::SetEncodedAudioMode		(NTV2EncodedAudioMode		value,				NTV2Channel channel)	{return SetEncodedAudioMode (value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetEncodedAudioMode		(NTV2EncodedAudioMode *		pOutMode,			NTV2Channel channel)	{return pOutMode	? GetEncodedAudioMode (*pOutMode, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::SetEmbeddedAudioInput	(NTV2EmbeddedAudioInput		value,				NTV2Channel channel)	{return SetEmbeddedAudioInput (value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetEmbeddedAudioInput	(NTV2EmbeddedAudioInput *	pOutValue,			NTV2Channel channel)	{return pOutValue	? GetEmbeddedAudioInput (*pOutValue, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::SetAudioBufferSize		(NTV2AudioBufferSize		value,				NTV2Channel channel)	{return SetAudioBufferSize	(value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::SetAudioAnalogLevel		(NTV2AudioLevel				value,				NTV2Channel channel)	{return SetAudioAnalogLevel	(value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::SetAudioLoopBack		(NTV2AudioLoopBack			value,				NTV2Channel channel)	{return SetAudioLoopBack	(value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetAudioBufferSize		(NTV2AudioBufferSize *		pOutSize,			NTV2Channel channel)	{return pOutSize	? GetAudioBufferSize	(*pOutSize,		static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::GetAudioAnalogLevel		(NTV2AudioLevel *			pOutLevel,			NTV2Channel channel)	{return pOutLevel	? GetAudioAnalogLevel	(*pOutLevel,	static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::GetAudioLoopBack		(NTV2AudioLoopBack *		pOutValue,			NTV2Channel channel)	{return pOutValue	? GetAudioLoopBack		(*pOutValue,	static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::SetEmbeddedAudioClock	(NTV2EmbeddedAudioClock		value,				NTV2Channel channel)	{return SetEmbeddedAudioClock (value, static_cast <NTV2AudioSystem> (channel));}
	bool CNTV2Card::GetEmbeddedAudioClock	(NTV2EmbeddedAudioClock *	pOutValue,			NTV2Channel channel)	{return pOutValue	? GetEmbeddedAudioClock (*pOutValue, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::GetAudioWrapAddress		(ULWord *					pOutWrapAddress,	NTV2Channel channel)	{return pOutWrapAddress ? GetAudioWrapAddress (*pOutWrapAddress, static_cast <NTV2AudioSystem> (channel)) : false;}
	bool CNTV2Card::GetAudioReadOffset		(ULWord *					pOutReadOffset,		NTV2Channel channel)	{return pOutReadOffset ? GetAudioReadOffset (*pOutReadOffset, static_cast <NTV2AudioSystem> (channel)) : false;}


	bool CNTV2Card::SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2AudioSource inAudioSource)
	{
		static const ULWord	sAudioSourceToRegValues []	=	{0x1/*NTV2_AUDIO_EMBEDDED*/,  0x0/*NTV2_AUDIO_AES*/,  0x9/*NTV2_AUDIO_ANALOG*/,  0xA/*NTV2_AUDIO_HDMI*/};

		if (static_cast <UWord> (inAudioSystem) < ::NTV2DeviceGetNumAudioStreams (_boardID) && inAudioSource < NTV2_MAX_NUM_AudioSources)
			return WriteRegister (gAudioSystemToSrcSelectRegNum [inAudioSystem], sAudioSourceToRegValues [inAudioSource], kRegMaskAudioSource, kRegShiftAudioSource);
		else
			return false;
	}

	bool CNTV2Card::SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2InputSource inInputSource)
	{
		bool			result		(false);
		NTV2AudioSource	audioSource	(NTV2_AUDIO_EMBEDDED);

		if (NTV2_INPUT_SOURCE_IS_HDMI (inInputSource))
			audioSource = NTV2_AUDIO_HDMI;
		else if (NTV2_INPUT_SOURCE_IS_ANALOG (inInputSource))
			audioSource = NTV2_AUDIO_ANALOG;

		if (SetAudioSystemInputSource (inAudioSystem, audioSource))
		{
			if (NTV2_INPUT_SOURCE_IS_SDI (inInputSource))
			{
				//	For SDI, we go the extra mile...
				if (SetEmbeddedAudioInput (::NTV2InputSourceToEmbeddedAudioInput (inInputSource), inAudioSystem))	//	Use the specified input for grabbing embedded audio
					result = SetEmbeddedAudioClock (NTV2_EMBEDDED_AUDIO_CLOCK_VIDEO_INPUT, inAudioSystem);			//	Use video input clock (not reference)
			}
		}
		return result;

	}	//	SetAudioSystemInputSource

	bool CNTV2Card::GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource & outAudioSource)
	{
		ULWord	regValue	(0);
		outAudioSource = NTV2_MAX_NUM_AudioSources;

		if (static_cast <UWord> (inAudioSystem) >= ::NTV2DeviceGetNumAudioStreams (_boardID))
			return false;
		if (!ReadRegister (gAudioSystemToSrcSelectRegNum[inAudioSystem], regValue, kRegMaskAudioSource, kRegShiftAudioSource))
			return false;
		switch (regValue)
		{
		case 0x1:	outAudioSource = NTV2_AUDIO_EMBEDDED;	break;
		case 0x0:	outAudioSource = NTV2_AUDIO_AES;		break;
		case 0x9:	outAudioSource = NTV2_AUDIO_ANALOG;		break;
		case 0xA:	outAudioSource = NTV2_AUDIO_HDMI;		break;
		default:	return false;
		}
		return true;
	}


	bool CNTV2Card::GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource * pOutAudioSource)
	{
		return pOutAudioSource ? GetAudioSystemInputSource (inAudioSystem, *pOutAudioSource) : false;
	}

	bool CNTV2Card::ReadAudioControl (ULWord * pOutValue, const NTV2Channel inChannel)
	{
		return ReadRegister (gChannelToAudioControlRegNum[inChannel], *pOutValue);
	}

	bool CNTV2Card::WriteAudioControl (const ULWord inValue, const NTV2Channel inChannel)
	{
		return WriteRegister (gChannelToAudioControlRegNum[inChannel], inValue);
	}

	bool CNTV2Card::GetAverageAudioLevelChan1_2(ULWord *value)
	{
		return ReadRegister (kRegAverageAudioLevelChan1_2, *value, kK2RegMaskAverageAudioLevel, kK2RegShiftAverageAudioLevel);
	}
	bool CNTV2Card::GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair * pOutValue, NTV2Channel * pOutChannel)
	{
		NTV2AudioChannelPair	channelPair	(NTV2_AUDIO_CHANNEL_PAIR_INVALID);
		NTV2Channel				channel		(NTV2_MAX_NUM_CHANNELS);
		bool					result		(GetHDMIOutAudioSource2Channel (channelPair, channel));
		if (result)
		{
			if (pOutValue)
				*pOutValue = channelPair;
			if (pOutChannel)
				*pOutChannel = channel;
		}
		return result;
	}


	bool CNTV2Card::GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair & outValue, NTV2Channel & outChannel)
	{
		ULWord	encoding	(0);
		bool	result		(ReadRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource));
		if (result)
		{
			outValue = static_cast <NTV2AudioChannelPair> (encoding & 0x7);
			outChannel = static_cast <NTV2Channel> (::GetNTV2ChannelForIndex (encoding >> 4));
		}
		return result;
	}
	bool CNTV2Card::GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect * pOutValue, NTV2Channel * pOutChannel)
	{
		NTV2Audio8ChannelSelect	channelSelect	(NTV2_MAX_NUM_Audio8ChannelSelect);
		NTV2Channel				channel			(NTV2_MAX_NUM_CHANNELS);
		bool					result			(GetHDMIOutAudioSource8Channel (channelSelect, channel));
		if (result)
		{
			if (pOutValue)
				*pOutValue = channelSelect;
			if (pOutChannel)
				*pOutChannel = channel;
		}
		return result;
	}


	bool CNTV2Card::GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect & outValue, NTV2Channel & outChannel)
	{
		ULWord	encoding	(0);
		bool	result		(ReadRegister (kRegAudioOutputSourceMap, encoding, kRegMaskHDMIOutAudioSource, kRegShiftHDMIOutAudioSource));
		if (result)
		{
			if ((encoding & 0x3) == static_cast <ULWord> (NTV2_AudioChannel1_4))
				outValue = NTV2_AudioChannel1_8;
			else
				outValue = NTV2_AudioChannel9_16;

			outChannel = ::GetNTV2ChannelForIndex ((encoding & 0xC) >> 2);
		}
		return result;
	}
#endif	//	!defined (NTV2_DEPRECATE)

#ifdef MSWindows
	#pragma warning(default: 4800)
#endif
