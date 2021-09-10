/* SPDX-License-Identifier: MIT */
/**
	@file		audioutilities.h
	@brief		Declaration of AJA_GenerateAudioTone function.
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_AUDIOUTILS_H
#define AJA_AUDIOUTILS_H

#include "public.h"

#define AJA_MAX_AUDIO_CHANNELS 16

uint32_t AJA_EXPORT AJA_GenerateAudioTone (
							uint32_t*	audioBuffer,
							uint32_t	numSamples,
							uint32_t	numChannels,
							uint32_t	numBits,
							uint32_t&	cycleSample,
							double		sampleRate,
							double*		amplitude,
							double*		frequency,
							bool		endianConvert);


#endif
