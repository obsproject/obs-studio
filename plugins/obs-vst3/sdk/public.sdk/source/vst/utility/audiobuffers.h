//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/audiobuffers.h
// Created by  : Steinberg, 04/2021
// Description : Audio Buffer utilities
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
#include <type_traits>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** get channel buffers from audio bus buffers 32 bit variant */
template <SymbolicSampleSizes SampleSize,
          typename std::enable_if<SampleSize == SymbolicSampleSizes::kSample32>::type* = nullptr>
inline Sample32** getChannelBuffers (AudioBusBuffers& buffer)
{
	return buffer.channelBuffers32;
}

//------------------------------------------------------------------------
/** get channel buffers from audio bus buffers 64 bit variant */
template <SymbolicSampleSizes SampleSize,
          typename std::enable_if<SampleSize == SymbolicSampleSizes::kSample64>::type* = nullptr>
inline Sample64** getChannelBuffers (AudioBusBuffers& buffer)
{
	return buffer.channelBuffers64;
}

//------------------------------------------------------------------------
} // Vst
} // Steinberg
