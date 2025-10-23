//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/processdata.h
// Created by  : Steinberg, 10/2005
// Description : VST Hosting Utilities
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Extension of ProcessData.

Helps setting up the buffers for the process data structure for a component.
When the prepare method is called with bufferSamples != 0 the buffer management is handled by this class.
Otherwise the buffers need to be setup explicitly.

\ingroup hostingBase
*/
class HostProcessData : public ProcessData
{
public:
//------------------------------------------------------------------------
	HostProcessData () = default;
	virtual ~HostProcessData () noexcept;

	/** Prepare buffer containers for all busses. If bufferSamples is not null buffers will be
	 * created. */
	bool prepare (IComponent& component, int32 bufferSamples, int32 _symbolicSampleSize);

	/** Remove bus buffers. */
	void unprepare ();

	/** Sets one sample buffer for all channels inside a bus. */
	bool setChannelBuffers (BusDirection dir, int32 busIndex, Sample32* sampleBuffer);
	bool setChannelBuffers64 (BusDirection dir, int32 busIndex, Sample64* sampleBuffer);

	/** Sets individual sample buffers per channel inside a bus. */
	bool setChannelBuffers (BusDirection dir, int32 busIndex, Sample32* sampleBuffers[],
	                        int32 bufferCount);
	bool setChannelBuffers64 (BusDirection dir, int32 busIndex, Sample64* sampleBuffers[],
	                          int32 bufferCount);

	/** Sets one sample buffer for a given channel inside a bus. */
	bool setChannelBuffer (BusDirection dir, int32 busIndex, int32 channelIndex,
	                       Sample32* sampleBuffer);
	bool setChannelBuffer64 (BusDirection dir, int32 busIndex, int32 channelIndex,
	                         Sample64* sampleBuffer);

	static constexpr uint64 kAllChannelsSilent =
#if SMTG_OS_MACOS
	    0xffffffffffffffffULL;
#else
	    0xffffffffffffffffUL;
#endif

//------------------------------------------------------------------------
protected:
	int32 createBuffers (IComponent& component, AudioBusBuffers*& buffers, BusDirection dir,
	                     int32 bufferSamples);
	void destroyBuffers (AudioBusBuffers*& buffers, int32& busCount);
	bool checkIfReallocationNeeded (IComponent& component, int32 bufferSamples,
	                                int32 _symbolicSampleSize) const;
	bool isValidBus (BusDirection dir, int32 busIndex) const;

	bool channelBufferOwner {false};
};

//------------------------------------------------------------------------
// inline
//------------------------------------------------------------------------
inline bool HostProcessData::isValidBus (BusDirection dir, int32 busIndex) const
{
	if (dir == kInput && (!inputs || busIndex >= numInputs))
		return false;
	if (dir == kOutput && (!outputs || busIndex >= numOutputs))
		return false;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffers (BusDirection dir, int32 busIndex,
                                                Sample32* sampleBuffer)
{
	if (channelBufferOwner || symbolicSampleSize != SymbolicSampleSizes::kSample32)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	for (int32 i = 0; i < busBuffers.numChannels; i++)
		busBuffers.channelBuffers32[i] = sampleBuffer;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffers64 (BusDirection dir, int32 busIndex,
                                                  Sample64* sampleBuffer)
{
	if (channelBufferOwner || symbolicSampleSize != SymbolicSampleSizes::kSample64)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	for (int32 i = 0; i < busBuffers.numChannels; i++)
		busBuffers.channelBuffers64[i] = sampleBuffer;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffers (BusDirection dir, int32 busIndex,
                                                Sample32* sampleBuffers[], int32 bufferCount)
{
	if (channelBufferOwner || symbolicSampleSize != SymbolicSampleSizes::kSample32)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	int32 count = bufferCount < busBuffers.numChannels ? bufferCount : busBuffers.numChannels;
	for (int32 i = 0; i < count; i++)
		busBuffers.channelBuffers32[i] = sampleBuffers ? sampleBuffers[i] : nullptr;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffers64 (BusDirection dir, int32 busIndex,
                                                  Sample64* sampleBuffers[], int32 bufferCount)
{
	if (channelBufferOwner || symbolicSampleSize != SymbolicSampleSizes::kSample64)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	int32 count = bufferCount < busBuffers.numChannels ? bufferCount : busBuffers.numChannels;
	for (int32 i = 0; i < count; i++)
		busBuffers.channelBuffers64[i] = sampleBuffers ? sampleBuffers[i] : nullptr;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffer (BusDirection dir, int32 busIndex, int32 channelIndex,
                                               Sample32* sampleBuffer)
{
	if (symbolicSampleSize != SymbolicSampleSizes::kSample32)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	if (channelIndex >= busBuffers.numChannels)
		return false;
	busBuffers.channelBuffers32[channelIndex] = sampleBuffer;
	return true;
}

//------------------------------------------------------------------------
inline bool HostProcessData::setChannelBuffer64 (BusDirection dir, int32 busIndex,
                                                 int32 channelIndex, Sample64* sampleBuffer)
{
	if (symbolicSampleSize != SymbolicSampleSizes::kSample64)
		return false;
	if (!isValidBus (dir, busIndex))
		return false;

	AudioBusBuffers& busBuffers = dir == kInput ? inputs[busIndex] : outputs[busIndex];
	if (channelIndex >= busBuffers.numChannels)
		return false;
	busBuffers.channelBuffers64[channelIndex] = sampleBuffer;
	return true;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
