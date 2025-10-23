//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/processcontextrequirements.h
// Created by  : Steinberg, 12/2019
// Description : Helper class to work with IProcessContextRequirements
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstaudioprocessor.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class ProcessContextRequirements
{
private:
	using Self = ProcessContextRequirements;

public:
	ProcessContextRequirements (uint32 inFlags = 0) : flags (inFlags) {}

	bool wantsNone () const { return flags == 0; }
	bool wantsSystemTime () const
	{
		return (flags & IProcessContextRequirements::kNeedSystemTime) != 0;
	}
	bool wantsContinousTimeSamples () const
	{
		return (flags & IProcessContextRequirements::kNeedContinousTimeSamples) != 0;
	}
	bool wantsProjectTimeMusic () const
	{
		return (flags & IProcessContextRequirements::kNeedProjectTimeMusic) != 0;
	}
	bool wantsBarPositionMusic () const
	{
		return (flags & IProcessContextRequirements::kNeedBarPositionMusic) != 0;
	}
	bool wantsCycleMusic () const
	{
		return (flags & IProcessContextRequirements::kNeedCycleMusic) != 0;
	}
	bool wantsSamplesToNextClock () const
	{
		return (flags & IProcessContextRequirements::kNeedSamplesToNextClock) != 0;
	}
	bool wantsTempo () const { return (flags & IProcessContextRequirements::kNeedTempo) != 0; }
	bool wantsTimeSignature () const
	{
		return (flags & IProcessContextRequirements::kNeedTimeSignature) != 0;
	}
	bool wantsChord () const { return (flags & IProcessContextRequirements::kNeedChord) != 0; }
	bool wantsFrameRate () const
	{
		return (flags & IProcessContextRequirements::kNeedFrameRate) != 0;
	}
	bool wantsTransportState () const
	{
		return (flags & IProcessContextRequirements::kNeedTransportState) != 0;
	}

	/** set SystemTime as requested */
	Self& needSystemTime ()
	{
		flags |= IProcessContextRequirements::kNeedSystemTime;
		return *this;
	}
	/** set ContinousTimeSamples as requested */
	Self& needContinousTimeSamples ()
	{
		flags |= IProcessContextRequirements::kNeedContinousTimeSamples;
		return *this;
	}
	/** set ProjectTimeMusic as requested */
	Self& needProjectTimeMusic ()
	{
		flags |= IProcessContextRequirements::kNeedProjectTimeMusic;
		return *this;
	}
	/** set BarPositionMusic as needed */
	Self& needBarPositionMusic ()
	{
		flags |= IProcessContextRequirements::kNeedBarPositionMusic;
		return *this;
	}
	/** set CycleMusic as needed */
	Self& needCycleMusic ()
	{
		flags |= IProcessContextRequirements::kNeedCycleMusic;
		return *this;
	}
	/** set SamplesToNextClock as needed */
	Self& needSamplesToNextClock ()
	{
		flags |= IProcessContextRequirements::kNeedSamplesToNextClock;
		return *this;
	}
	/** set Tempo as needed */
	Self& needTempo ()
	{
		flags |= IProcessContextRequirements::kNeedTempo;
		return *this;
	}
	/** set TimeSignature as needed */
	Self& needTimeSignature ()
	{
		flags |= IProcessContextRequirements::kNeedTimeSignature;
		return *this;
	}
	/** set Chord as needed */
	Self& needChord ()
	{
		flags |= IProcessContextRequirements::kNeedChord;
		return *this;
	}
	/** set FrameRate as needed */
	Self& needFrameRate ()
	{
		flags |= IProcessContextRequirements::kNeedFrameRate;
		return *this;
	}
	/** set TransportState as needed */
	Self& needTransportState ()
	{
		flags |= IProcessContextRequirements::kNeedTransportState;
		return *this;
	}

	uint32 flags {0};
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
