/* SPDX-License-Identifier: MIT */
/**
	@file		audioutilities.cpp
	@copyright	(C) 2012-2021 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implementation of AJA_GenerateAudioTone function.
**/

#include "common.h"
#include "audioutilities.h"
#include <math.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

uint32_t AJA_GenerateAudioTone (uint32_t*	audioBuffer,
								uint32_t	numSamples,
								uint32_t	numChannels,
								uint32_t	numBits,
								uint32_t&	cycleSample,
								double		sampleRate,
								double*		amplitude,
								double*		frequency,
								bool		endianConvert)
{
	double cycleCurrent[AJA_MAX_AUDIO_CHANNELS];
	double cycleLength[AJA_MAX_AUDIO_CHANNELS];
	double scale = (double)((uint32_t)(1<<(numBits-1)));
	scale -= 1.0;

	uint32_t sample;
	uint32_t channel;
	for (channel = 0; channel < numChannels; channel++)
	{
		cycleLength[channel] = sampleRate / frequency[channel];
		cycleCurrent[channel] = cycleSample;
	}

	for (sample = 0; sample < numSamples; sample++)
	{
		for (channel = 0; channel < numChannels; channel++)
		{
			float nextFloat = (float)(sin(cycleCurrent[channel] / cycleLength[channel] * (M_PI * 2.0)) * amplitude[channel]);
			uint32_t value = static_cast<uint32_t>((nextFloat * scale) + 0.5);

			if (endianConvert)
			{
				value = AJA_ENDIAN_SWAP32(value);
			}
			*audioBuffer++ = value;

			cycleCurrent[channel] += 1.0;
			if (cycleCurrent[channel] > cycleLength[channel])
			{
				cycleCurrent[channel] -= cycleLength[channel];
			}
		}
		cycleSample++;
	}

	return numSamples*4*numChannels;
}
