//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstprocesscontext.h
// Created by  : Steinberg, 10/2005
// Description : VST Processing Context Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
//------------------------------------------------------------------------
/** Frame Rate 
A frame rate describes the number of image (frame) displayed per second.
Some examples:
	- 23.976 fps     is framesPerSecond: 24 and flags: kPullDownRate
	- 24 fps         is framesPerSecond: 24 and flags: 0
	- 25 fps         is framesPerSecond: 25 and flags: 0
	- 29.97 drop fps is framesPerSecond: 30 and flags: kDropRate|kPullDownRate
	- 29.97 fps      is framesPerSecond: 30 and flags: kPullDownRate
	- 30 fps         is framesPerSecond: 30 and flags: 0
	- 30 drop fps    is framesPerSecond: 30 and flags: kDropRate
	- 50 fps         is framesPerSecond: 50 and flags: 0
	- 59.94 fps	     is framesPerSecond: 60 and flags: kPullDownRate
	- 60 fps         is framesPerSecond: 60 and flags: 0
*/
struct FrameRate
{
//------------------------------------------------------------------------
	enum FrameRateFlags
	{
		kPullDownRate = 1 << 0,
		kDropRate     = 1 << 1
	};

	uint32 framesPerSecond;		///< frame rate
	uint32 flags;				///< flags #FrameRateFlags
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Description of a chord.
A chord is described with a key note, a root note and the
\copydoc chordMask
\see ProcessContext
*/
struct Chord
{
//------------------------------------------------------------------------
	uint8 keyNote;		///< key note in chord
	uint8 rootNote;		///< lowest note in chord

	/** Bitmask of a chord. \n
	    1st bit set: minor second; 2nd bit set: major second, and so on. \n
		There is \b no bit for the keynote (root of the chord) because it is inherently always present. \n
		Examples:
		- XXXX 0000 0100 1000 (= 0x0048) -> major chord
		- XXXX 0000 0100 0100 (= 0x0044) -> minor chord
		- XXXX 0010 0100 0100 (= 0x0244) -> minor chord with minor seventh */
	int16 chordMask;

	enum Masks {
		kChordMask = 0x0FFF,	///< mask for chordMask
		kReservedMask = 0xF000	///< reserved for future use
	};
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
/** Audio processing context.
For each processing block the host provides timing information and musical parameters that can
change over time. For a host that supports jumps (like cycle) it is possible to split up a
processing block into multiple parts in order to provide a correct project time inside of every
block, but this behavior is not mandatory. Since the timing will be correct at the beginning of the
next block again, a host that is dependent on a fixed processing block size can choose to neglect
this problem.
\see IAudioProcessor, ProcessData
*/
struct ProcessContext
{
//------------------------------------------------------------------------
	/** Transport state & other flags */
	enum StatesAndFlags
	{
		kPlaying          = 1 << 1,		///< currently playing
		kCycleActive      = 1 << 2,		///< cycle is active
		kRecording        = 1 << 3,		///< currently recording

		kSystemTimeValid  = 1 << 8,		///< systemTime contains valid information
		kContTimeValid    = 1 << 17,	///< continousTimeSamples contains valid information

		kProjectTimeMusicValid = 1 << 9,///< projectTimeMusic contains valid information
		kBarPositionValid = 1 << 11,	///< barPositionMusic contains valid information
		kCycleValid       = 1 << 12,	///< cycleStartMusic and barPositionMusic contain valid information

		kTempoValid       = 1 << 10,	///< tempo contains valid information
		kTimeSigValid     = 1 << 13,	///< timeSigNumerator and timeSigDenominator contain valid information
		kChordValid       = 1 << 18,	///< chord contains valid information

		kSmpteValid       = 1 << 14,	///< smpteOffset and frameRate contain valid information
		kClockValid       = 1 << 15		///< samplesToNextClock valid
	};

	uint32 state;					///< a combination of the values from \ref StatesAndFlags

	double sampleRate;				///< current sample rate					(always valid)
	TSamples projectTimeSamples;	///< project time in samples				(always valid)

	int64 systemTime;				///< system time in nanoseconds					(optional)
	TSamples continousTimeSamples;	///< project time, without loop					(optional)

	TQuarterNotes projectTimeMusic;	///< musical position in quarter notes (1.0 equals 1 quarter note) (optional)
	TQuarterNotes barPositionMusic;	///< last bar start position, in quarter notes	(optional)
	TQuarterNotes cycleStartMusic;	///< cycle start in quarter notes				(optional)
	TQuarterNotes cycleEndMusic;	///< cycle end in quarter notes					(optional)

	double tempo;					///< tempo in BPM (Beats Per Minute)			(optional)
	int32 timeSigNumerator;			///< time signature numerator (e.g. 3 for 3/4)	(optional)
	int32 timeSigDenominator;		///< time signature denominator (e.g. 4 for 3/4) (optional)

	Chord chord;					///< musical info								(optional)

	int32 smpteOffsetSubframes;		///< SMPTE (sync) offset in subframes (1/80 of frame) (optional)
	FrameRate frameRate;			///< frame rate									(optional)

	int32 samplesToNextClock;		///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest) (optional)
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
