/* SPDX-License-Identifier: MIT */
/**
	@file		timebase.cpp
	@brief		Implements the AJATimeBase class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/common.h"
#include "ajabase/common/timebase.h"
#include "ajabase/system/systemtime.h"

struct AJAFrameRateEntry
{
	AJA_FrameRate ajaFrameRate;
	int64_t       frameTimeScale;
	int64_t       frameDuration;
};
const AJAFrameRateEntry AJAFrameRateTable[] =
{
	{AJA_FrameRate_1498, 15000, 1001},
	{AJA_FrameRate_1500, 15000, 1000},
	{AJA_FrameRate_1798, 18000, 1001},
	{AJA_FrameRate_1800, 18000, 1000},
	{AJA_FrameRate_1898, 19000, 1001},
	{AJA_FrameRate_1900, 19000, 1000},
	{AJA_FrameRate_2398, 24000, 1001},
	{AJA_FrameRate_2400, 24000, 1000},
	{AJA_FrameRate_2500, 25000, 1000},
	{AJA_FrameRate_2997, 30000, 1001},
	{AJA_FrameRate_3000, 30000, 1000},
	{AJA_FrameRate_4795, 48000, 1001},
	{AJA_FrameRate_4800, 48000, 1000},
	{AJA_FrameRate_5000, 50000, 1000},
	{AJA_FrameRate_5994, 60000, 1001},
	{AJA_FrameRate_6000, 60000, 1000},
	{AJA_FrameRate_10000,100000,1000},
	{AJA_FrameRate_11988,120000,1001},
	{AJA_FrameRate_12000,120000,1000}
};
const size_t AJAFrameRateTableSize = sizeof(AJAFrameRateTable) / sizeof(AJAFrameRateEntry);

const int64_t AJATimeBaseDefaultTimeScale = 30000;
const int64_t AJATimeBaseDefaultDuration  = 1001;
const int64_t AJATimeBaseDefaultAudioRate = 48000;

AJATimeBase::AJATimeBase()
{
    SetToDefault();
}

AJATimeBase::AJATimeBase(AJA_FrameRate ajaFrameRate)
{
    SetToDefault();
    // this will set the correct timeScale and duration
    SetAJAFrameRatePrivate(ajaFrameRate);
}

AJATimeBase::AJATimeBase(int64_t frameTimeScale, int64_t frameDuration)
{
    SetToDefault();
    mFrameTimeScale = frameTimeScale;
    mFrameDuration  = frameDuration;
}

AJATimeBase::AJATimeBase(int64_t frameTimeScale, int64_t frameDuration, int64_t audioRate)
{
    SetToDefault();
	mFrameTimeScale	= frameTimeScale;
	mFrameDuration	= frameDuration;
	mAudioRate		= audioRate;
}

AJATimeBase::AJATimeBase(const AJATimeBase &other)
{
	mFrameTimeScale = other.mFrameTimeScale;
	mFrameDuration = other.mFrameDuration;
	mAudioRate = other.mAudioRate;
	mTickRate = other.mTickRate;
}

AJATimeBase::~AJATimeBase()
{
}

void AJATimeBase::SetToDefault(void)
{
    mFrameTimeScale = AJATimeBaseDefaultTimeScale;
    mFrameDuration  = AJATimeBaseDefaultDuration;
    mAudioRate      = AJATimeBaseDefaultAudioRate;
	mTickRate       = AJATime::GetSystemFrequency();
}

AJATimeBase& AJATimeBase::operator=(const AJATimeBase &t)
{
	if (this != &t)
	{
		mFrameTimeScale		= t.mFrameTimeScale;
	    mFrameDuration		= t.mFrameDuration;
	    mAudioRate			= t.mAudioRate;
	    mTickRate			= t.mTickRate;
	}

	return *this;
}

bool AJATimeBase::operator==(const AJATimeBase &b) const 
{
	bool bIsSame = true;

	if (mFrameTimeScale != b.mFrameTimeScale)
		bIsSame = false;
	if (mFrameDuration  != b.mFrameDuration)
		bIsSame = false;
	if (mAudioRate      != b.mAudioRate)
		bIsSame = false;
	if (mTickRate       != b.mTickRate)
		bIsSame = false;

	return bIsSame;
}

bool AJATimeBase::operator!=(const AJATimeBase &a) const 
{
	return !(*this == a);
}

void AJATimeBase::SetFrameRate(int64_t frameTimeScale, int64_t frameDuration)
{
	mFrameTimeScale = frameTimeScale;
	mFrameDuration = frameDuration;
}

void AJATimeBase::GetFrameRate(int64_t& frameTimeScale, int64_t& frameDuration) const
{
	frameTimeScale = mFrameTimeScale;
	frameDuration = mFrameDuration;
}

int64_t AJATimeBase::GetFrameTimeScale(void) const
{
	return mFrameTimeScale;
}

void AJATimeBase::SetFrameTimeScale(int64_t timeScale)
{
	mFrameTimeScale = timeScale;
}

int64_t AJATimeBase::GetFrameDuration(void) const
{
	return mFrameDuration;
}

void AJATimeBase::SetAudioRate(int64_t rate)
{
	mAudioRate = rate;
}

int64_t AJATimeBase::GetAudioRate() const
{
	return mAudioRate;
}

void AJATimeBase::SetAJAFrameRate(AJA_FrameRate ajaFrameRate)
{
	SetAJAFrameRatePrivate(ajaFrameRate);
}

void AJATimeBase::SetTickRate(int64_t rate)
{
	if (rate == 0)
	{
		// set to native rate
		mTickRate = AJATime::GetSystemFrequency();
	}
	else
	{
		mTickRate = rate;
	}
}

int64_t AJATimeBase::GetTickRate() const
{
	return mTickRate;
}

int64_t AJATimeBase::FramesToSamples(int64_t frames, bool round) const
{
	return Convert(frames, mFrameTimeScale, mFrameDuration, mAudioRate, 1, round, true);
}

int64_t AJATimeBase::FramesToTicks(int64_t frames, bool round) const
{
	return Convert(frames, mFrameTimeScale, mFrameDuration, mTickRate, 1, round, true);
}

double AJATimeBase::FramesToSeconds(int64_t frames) const
{
	return (double)frames * (double)mFrameDuration / (double)mFrameTimeScale;
}

double AJATimeBase::ScaleUnitsToSeconds(int64_t scaleUnits) const
{
	return (double)scaleUnits / (double)mFrameTimeScale;
}

float AJATimeBase::GetFramesPerSecond(void) const
{
	float val = 0.f;
	if (mFrameDuration)
		val = (float)mFrameTimeScale / (float)mFrameDuration;
	return val;
}

double AJATimeBase::GetFramesPerSecondDouble(void) const
{
	double val = 0.;
	if (mFrameDuration)
		val = (double)mFrameTimeScale / (double)mFrameDuration;
	return val;
}

int64_t AJATimeBase::FramesToMicroseconds(int64_t frames, bool round) const
{
	return Convert(frames, mFrameTimeScale, mFrameDuration, 1000000, 1, round, true);
}

int64_t AJATimeBase::SamplesToFrames(int64_t samples, bool round) const
{
	return Convert(samples, mAudioRate, 1, mFrameTimeScale, mFrameDuration, round, true);
}

int64_t AJATimeBase::SamplesToTicks(int64_t samples, bool round) const
{
	return Convert(samples, mAudioRate, mTickRate, round, true);
}

double AJATimeBase::SamplesToSeconds(int64_t samples)
{
	return (double)samples / (double)mAudioRate;
}

int64_t AJATimeBase::SamplesToMicroseconds(int64_t samples, bool round)
{
	return Convert(samples, mAudioRate, 1000000, round, true);
}

int64_t AJATimeBase::TicksToFrames(int64_t ticks, bool round)
{
	return Convert(ticks, mTickRate, 1, mFrameTimeScale, mFrameDuration, round, true);
}

int64_t AJATimeBase::TicksToSamples(int64_t ticks, bool round)
{
	return Convert(ticks, mTickRate, mAudioRate, round, true);
}

double AJATimeBase::TicksToSeconds(int64_t ticks)
{
	return (double)ticks / (double)mTickRate;
}

int64_t AJATimeBase::TicksToMicroseconds(int64_t ticks, bool round)
{
	return Convert(ticks, mTickRate, 1000000, round, true);
}

int64_t AJATimeBase::SecondsToFrames(double seconds, bool round)
{
	return Convert((int64_t)(seconds * 1000000000.0), 1000000000, 1, mFrameTimeScale, mFrameDuration, round, true);
}

int64_t AJATimeBase::SecondsToSamples(double seconds, bool round)
{
	return Convert((int64_t)(seconds * 1000000000.0), 1000000000, mAudioRate, round, true);
}

int64_t AJATimeBase::SecondsToTicks(double seconds, bool round)
{
	return Convert((int64_t)(seconds * 1000000000.0), 1000000000, mTickRate, round, true);
}

int64_t AJATimeBase::MicrosecondsToFrames(int64_t microseconds, bool round)
{
	return Convert(microseconds, 1000000, 1, mFrameTimeScale, mFrameDuration, round, true);
}

int64_t AJATimeBase::MicrosecondsToSamples(int64_t microseconds, bool round)
{
	return Convert(microseconds, 1000000, mAudioRate, round, true);
}

int64_t AJATimeBase::MicrosecondsToTicks(int64_t microseconds, bool round)
{
	return Convert(microseconds, 1000000, mTickRate, round, true);
}

int64_t AJATimeBase::SecondsToMicroseconds(double seconds, bool round)
{
	return Convert((int64_t)(seconds * 1000000.0), 1000000, 1000000, round, false);
}

double AJATimeBase::MicrosecondsToSeconds(int64_t microseconds)
{
	return (double)microseconds / 1000000.0;
}

bool AJATimeBase::IsCloseTo(const AJATimeBase &timeBase) const
{
	bool bIsClose = false;
	double rate1  = FramesToSeconds(1);
	double rate2  = timeBase.FramesToSeconds(1);
	double dif    = rate1 / rate2;

			// this is just a guess at what is considered close enough
			// it will differentiate between 30000/1001 and 30000/1000,
			// but 2997/100 is *not* close to 30000/1001
	if ((dif >= .9999) && (dif <= 1.0001))
		bIsClose = true;
	return bIsClose;
}

bool AJATimeBase::IsCloseTo(int64_t frameTimeScale, int64_t frameDuration) const
{
	AJATimeBase tb(frameTimeScale,frameDuration);
	return IsCloseTo(tb);
}

bool AJATimeBase::IsNonIntegralRatio (void) const
{
	return ((mFrameTimeScale % mFrameDuration) != 0) ? true : false;
}

int64_t AJATimeBase::GetSystemTicks()
{
	int64_t ticks = AJATime::GetSystemCounter();
	int64_t rate = AJATime::GetSystemFrequency();

	if (rate != mTickRate)
	{
		ticks = Convert(ticks, rate, mTickRate, false, true);
	}

	return ticks;
}

int64_t AJATimeBase::Convert(int64_t inValue, int64_t inRate, int64_t outRate, bool round, bool large)
{
	int64_t outValue = 0;

	if (round)
	{
		// convert half the out rate to the in rate and increase the in value
        int64_t roundValue = inRate / (outRate * 2);
		if (inValue > 0)
		{
			inValue += roundValue;
		}
		else
		{
			inValue -= roundValue;
		}
	}

	if (large)
	{
		// this will not overflow
		outValue = inValue / inRate * outRate;
		outValue += (inValue % inRate) * outRate / inRate;
	}
	else
	{
		// this has better performance
		outValue = inValue * outRate / inRate;
	}

	return outValue;
}

int64_t AJATimeBase::Convert(int64_t inValue, int64_t inScale, int64_t inDuration,
							 int64_t outScale, int64_t outDuration, bool round, bool large)
{
	int64_t outValue = 0;
	int64_t roundValue = 0;
	int64_t inScaleOutDuration = inScale * outDuration;
	int64_t outScaleInDuration = outScale * inDuration;

	if (round)
	{
		// convert half the out scale to the in duration and increase the in value
		roundValue = inScaleOutDuration / (outScaleInDuration * 2);
		if (inValue > 0)
		{
			inValue += roundValue;
		}
		else
		{
			inValue -= roundValue;
		}
	}

	if (large)
	{
		// this will not overflow
		outValue = inValue / inScaleOutDuration * outScaleInDuration;
		outValue += (inValue % inScaleOutDuration) * outScaleInDuration / inScaleOutDuration;
	}
	else
	{
		// this has better performance
		outValue = inValue * outScaleInDuration / inScaleOutDuration;
	}

	return outValue;
}

AJA_FrameRate AJATimeBase::GetAJAFrameRate(void) const
{
	AJA_FrameRate fr = AJA_FrameRate_Unknown;
    for (size_t i = 0; i < AJAFrameRateTableSize; i++)
    {
        if (IsCloseTo(AJAFrameRateTable[i].frameTimeScale,AJAFrameRateTable[i].frameDuration))
        {
            fr = AJAFrameRateTable[i].ajaFrameRate;
            break;
        }
	}
     
	return fr;
}

void AJATimeBase::SetAJAFrameRatePrivate(AJA_FrameRate ajaFrameRate)
{
    mFrameTimeScale = AJATimeBaseDefaultTimeScale;
    mFrameDuration  = AJATimeBaseDefaultDuration;

	for (size_t i = 0; i < AJAFrameRateTableSize; i++)
	{
		if (AJAFrameRateTable[i].ajaFrameRate == ajaFrameRate)
		{
			mFrameTimeScale = AJAFrameRateTable[i].frameTimeScale;
			mFrameDuration  = AJAFrameRateTable[i].frameDuration;
			break;
		}
	}
}
