/* SPDX-License-Identifier: MIT */
/**
	@file		timebase.h
	@brief		Declares the AJATimeBase class.
	@copyright	(C) 2009-2021 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef TIMEBASE_H
#define TIMEBASE_H

#include "ajabase/common/public.h"
#include "ajabase/common/videotypes.h"

/**
 *	Class provides high resolution time base conversion
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJATimeBase
{
public:

	AJATimeBase();

    /**
     *	Construct the time base class.
     *
     *	@param[in]	ajaFrameRate		Frame rate define found in videotypes.h
     */
    AJATimeBase(AJA_FrameRate ajaFrameRate);

	/**
	 *	Construct the time base class.
	 *
	 *	@param[in]	frameTimeScale		Time units per second (units/second)
	 *	@param[in]	frameDuration		Time units per frame (units/frame)
	 */
	AJATimeBase(int64_t frameTimeScale, int64_t frameDuration);

	/**
	 *	Construct the time base class.
	 *
	 *	@param[in]	frameTimeScale		Time units per second (units/second)
	 *	@param[in]	frameDuration		Time units per frame (units/frame)
	 *	@param[in]	audioRate			Audio sample rate (samples/second)
	 */
	AJATimeBase(int64_t frameTimeScale, int64_t frameDuration, int64_t audioRate);

	/**
	 *  Copy constructor
	 *
	 *  @param[in]	other				AJATimeBase to copy
	 */
	AJATimeBase(const AJATimeBase &other);

	virtual ~AJATimeBase();

	/**
	 *	Set to default value.
	 */
	void SetToDefault(void);

	/**
	 *	Set the video frame rate to use in conversions.
	 *
	 *	The frame rate is specified as frameTimeScale/frameDuration = frames/second.
	 *
	 *	@param[in]	frameTimeScale	Time units per second (units/second)
	 *	@param[in]	frameDuration	Time units per frame (units/frame)
	 */
	void SetFrameRate(int64_t frameTimeScale, int64_t frameDuration);

	/**
	 *	Set the video frame rate to use in conversions.
	 *
	 *	The frame rate is specified Frame rate define found in videotypes.h
	 *
	 *	@param[in]	ajaFrameRate	AJA_FrameRate
	 */
	void SetAJAFrameRate(AJA_FrameRate ajaFrameRate);

	/**
	 *	Get the video frame rate used in conversions.
	 *
	 *	The frame rate is specified as frameTimeScale/frameDuration = frames/seconds.
	 *
	 *	@param[out]	frameTimeScale	Time units per second (units/second)
	 *	@param[out]	frameDuration	Time units per frame (units/frame)
	 */
	void GetFrameRate(int64_t& frameTimeScale, int64_t& frameDuration) const;
	void GetFrameRate(uint32_t& frameTimeScale, uint32_t& frameDuration) const;

	/**
	 *	Get the video frame time scale used in conversions.
	 *
	 *	@return		frameTimeScale	Time units per second (units/second)
	 */
	int64_t GetFrameTimeScale(void) const;
	
	/**
	 *	Set the video frame duration used in conversions.
	 *
	 *	@param[in]	timeScale	Time units per second (units/second)
	 */
	void SetFrameTimeScale(int64_t timeScale);

	/**
	 *	Get the video frame duration used in conversions.
	 *
	 *	@return		frameDuration	Time units per frame (units/frame)
	 */
	int64_t GetFrameDuration(void) const;

	/**
	 *	Set the audio sample rate to use in conversions.
	 *
	 *	@param[in]	rate	Audio rate in samples/second.
	 */
	void SetAudioRate(int64_t rate);

	/**
	 *	Get the audio sample rate used in conversions.
	 *
	 *	@return		Audio rate in samples/second.
	 */
	int64_t GetAudioRate() const;

	/**
	 *	Get the AJA_FrameRate which is close to the current value, return unknown if not close.
	 *
	 *	@return		AJA_FrameRate value, may be unknown
	 */
	AJA_FrameRate GetAJAFrameRate(void) const;

	/**
	 *	Set the system tick rate to use in conversions.
	 *
	 *	@param[in]	rate	System tick rate in ticks/second.
	 *						0 == system native rate.
	 */
	void SetTickRate(int64_t rate);

	/**
	 *	Get the system tick rate used in conversions.
	 *
	 *	@return		Tick rate in ticks/second.
	 */
	int64_t GetTickRate() const;

	/**
	 *	Convert video frames to audio samples.
	 *
	 *	@param[in]	frames	Video frame count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Audio sample count.
	 */
	int64_t FramesToSamples(int64_t frames, bool round = false) const;

	/**
	 *	Convert video frames to system ticks.
	 *
	 *	@param[in]	frames	Video frame count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				System tick count.
	 */
	int64_t FramesToTicks(int64_t frames, bool round = false) const;

	/**
	 *	Convert video frames to time in seconds.
	 *
	 *	@param[in]	frames	Video frame count to convert.
	 *	@return				Time in seconds.
	 */
	double FramesToSeconds(int64_t frames) const;

	/**
	 *	Convert time scale units to time in seconds.
	 *
	 *	@param[in]	num		Time scale units to convert.
	 *	@return				Time in seconds.
	 */
	double ScaleUnitsToSeconds(int64_t num) const;

	/**
	 *	Convert video frames to time in microseconds.
	 *
	 *	@param[in]	frames	Video frame count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Time in microseconds.
	 */
	int64_t FramesToMicroseconds(int64_t frames, bool round = false) const;

	/**
	 *	Convert audio samples to video frames.
	 *
	 *	@param[in]	samples	Audio sample count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Video frame count.
	 */
	int64_t SamplesToFrames(int64_t samples, bool round = false) const;

	/**
	 *	Convert audio samples to system ticks.
	 *
	 *	@param[in]	samples	Audio sample count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				System tick count.
	 */
	int64_t SamplesToTicks(int64_t samples, bool round = false) const;

	/**
	 *	Convert audio samples to time in seconds.
	 *
	 *	@param[in]	samples	Sample count to convert.
	 *	@return				Time in seconds.
	 */
	double SamplesToSeconds(int64_t samples);

	/**
	 *	Convert audio samples to time in microseconds.
	 *
	 *	@param[in]	samples	Sample count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Time in microseconds.
	 */
	int64_t SamplesToMicroseconds(int64_t samples, bool round = false);

	/**
	 *	Convert system ticks to video frames.
	 *
	 *	@param[in]	ticks	System tick count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Video frame count.
	 */
	int64_t TicksToFrames(int64_t ticks, bool round = false);

	/**
	 *	Convert system ticks to audio samples.
	 *
	 *	@param[in]	ticks	System tick count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Audio sample count.
	 */
	int64_t TicksToSamples(int64_t ticks, bool round = false);

	/**
	 *	Convert system ticks to time in seconds.
	 *
	 *	@param[in]	ticks	System tick count to convert.
	 *	@return				Time in seconds.
	 */
	double TicksToSeconds(int64_t ticks);

	/**
	 *	Convert system ticks to time in microseconds.
	 *
	 *	@param[in]	ticks	System tick count to convert.
	 *	@param[in]	round	Round the result.
	 *	@return				Time in microseconds.
	 */
	int64_t TicksToMicroseconds(int64_t ticks, bool round = false);

	/**
	 *	Convert time in seconds to video frames.
	 *
	 *	@param[in]	seconds	Time in seconds.
	 *	@param[in]	round	Round the result.
	 *	@return				Video frame count.
	 */
	int64_t SecondsToFrames(double seconds, bool round = false);

	/**
	 *	Convert time in seconds to audio samples.
	 *
	 *	@param[in]	seconds	Time in seconds.
	 *	@param[in]	round	Round the result.
	 *	@return				Audio sample count.
	 */
	int64_t SecondsToSamples(double seconds, bool round = false);

	/**
	 *	Convert time in seconds to system ticks.
	 *
	 *	@param[in]	seconds	Time in seconds.
	 *	@param[in]	round	Round the result.
	 *	@return				System tick count.
	 */
	int64_t SecondsToTicks(double seconds, bool round = false);

	/**
	 *	Convert microseconds to video frames.
	 *
	 *	@param[in]	microseconds	System tick count to convert.
	 *	@param[in]	round			Round the result.
	 *	@return						Video frame count.
	 */
	int64_t MicrosecondsToFrames(int64_t microseconds, bool round = false);

	/**
	 *	Convert microseconds to audio samples.
	 *
	 *	@param[in]	microseconds	System tick count to convert.
	 *	@param[in]	round			Round the result.
	 *	@return						Audio sample count.
	 */
	int64_t MicrosecondsToSamples(int64_t microseconds, bool round = false);

	/**
	 *	Convert microseconds to system ticks.
	 *
	 *	@param[in]	microseconds	System tick count to convert.
	 *	@param[in]	round			Round the result.
	 *	@return						Time in microseconds.
	 */
	int64_t MicrosecondsToTicks(int64_t microseconds, bool round = false);

	/**
	 *	Get the current value of the system tick count.
	 *
	 *	@return		System tick count.
	 */
	int64_t GetSystemTicks();

	/**
	 *	Returns if timebases are reasonably close
	 *
	 *	@param[in]	timeBase	    Time base to compare.
	 *	@return						True if the same or close (returns false if either duration is zero).
	 */
	bool IsCloseTo(const AJATimeBase &timeBase) const;

	/**
	 *	Returns if timebases are reasonably close
	 *
	 *	@param[in]	frameTimeScale	Time scale to compare.
	 *	@param[in]	frameDuration	Time duration to compare.
	 *	@return						True if the same or close (returns false if either duration is zero).
	 */
	bool IsCloseTo(int64_t frameTimeScale, int64_t frameDuration) const;

	/**
	 *	Test if the timescale is not a simple multiple of the duration
	 *
	 *	The intent is that even ratios are "non-drop", otherwise the timebase is "drop frame"
	 *	In this sense, this method can be thought of as equivalent to IsDropFrame()
	 *
	 *	@return						True if the timescale/duration ratio has a non-zero fraction, i.e. "drop frame".
	 */
	bool IsNonIntegralRatio(void) const;

	/**
	 *	Returns frames per second
	 *
	 *	@return		frames per second
	 */
	float  GetFramesPerSecond(void) const;
	double GetFramesPerSecondDouble(void) const;

	AJATimeBase&   operator=(const AJATimeBase &t); 
	bool operator==(const AJATimeBase &val) const;
	bool operator!=(const AJATimeBase &val) const;

	/**
	 *	Convert time in seconds to microseconds.
	 *
	 *	@param[in]	seconds	Time in seconds.
	 *	@param[in]	round	Round the result.
	 *	@return				Time in microseconds.
	 */
	static int64_t SecondsToMicroseconds(double seconds, bool round = false);

	/**
	 *	Convert microseconds to time in seconds.
	 *
	 *	@param[in]	microseconds	System tick count to convert.
	 *	@return						Time in seconds.
	 */
	static double MicrosecondsToSeconds(int64_t microseconds);

	static int64_t Convert(int64_t inValue, int64_t inRate, int64_t outRate, bool round, bool large);
	static int64_t Convert(int64_t inValue, int64_t inScale, int64_t inDuration,
						   int64_t outScale, int64_t outDuration, bool round, bool large);

private:
	void    SetAJAFrameRatePrivate(AJA_FrameRate ajaFrameRate);

	int64_t mFrameTimeScale;
	int64_t mFrameDuration;
	int64_t mAudioRate;
	int64_t mTickRate;
};

#endif

