//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/processdata.cpp
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

#include "processdata.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// HostProcessData
//------------------------------------------------------------------------
HostProcessData::~HostProcessData () noexcept
{
	unprepare ();
}

//------------------------------------------------------------------------
bool HostProcessData::prepare (IComponent& component, int32 bufferSamples,
                               int32 _symbolicSampleSize)
{
	if (checkIfReallocationNeeded (component, bufferSamples, _symbolicSampleSize))
	{
		unprepare ();

		symbolicSampleSize = _symbolicSampleSize;
		channelBufferOwner = bufferSamples > 0;

		numInputs = createBuffers (component, inputs, kInput, bufferSamples);
		numOutputs = createBuffers (component, outputs, kOutput, bufferSamples);
	}
	else
	{
		// reset silence flags
		for (int32 i = 0; i < numInputs; i++)
		{
			inputs[i].silenceFlags = 0;
		}
		for (int32 i = 0; i < numOutputs; i++)
		{
			outputs[i].silenceFlags = 0;
		}
	}
	symbolicSampleSize = _symbolicSampleSize;

	return true;
}

//------------------------------------------------------------------------
void HostProcessData::unprepare ()
{
	destroyBuffers (inputs, numInputs);
	destroyBuffers (outputs, numOutputs);

	channelBufferOwner = false;
}

//------------------------------------------------------------------------
bool HostProcessData::checkIfReallocationNeeded (IComponent& component, int32 bufferSamples,
                                                 int32 _symbolicSampleSize) const
{
	if (channelBufferOwner != (bufferSamples > 0))
		return true;
	if (symbolicSampleSize != _symbolicSampleSize)
		return true;

	int32 inBusCount = component.getBusCount (kAudio, kInput);
	if (inBusCount != numInputs)
		return true;

	int32 outBusCount = component.getBusCount (kAudio, kOutput);
	if (outBusCount != numOutputs)
		return true;

	for (int32 i = 0; i < inBusCount; i++)
	{
		BusInfo busInfo = {};

		if (component.getBusInfo (kAudio, kInput, i, busInfo) == kResultTrue)
		{
			if (inputs[i].numChannels != busInfo.channelCount)
				return true;
		}
	}
	for (int32 i = 0; i < outBusCount; i++)
	{
		BusInfo busInfo = {};

		if (component.getBusInfo (kAudio, kOutput, i, busInfo) == kResultTrue)
		{
			if (outputs[i].numChannels != busInfo.channelCount)
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
int32 HostProcessData::createBuffers (IComponent& component, AudioBusBuffers*& buffers,
                                      BusDirection dir, int32 bufferSamples)
{
	int32 busCount = component.getBusCount (kAudio, dir);
	if (busCount > 0)
	{
		buffers = new AudioBusBuffers[busCount];

		for (int32 i = 0; i < busCount; i++)
		{
			BusInfo busInfo = {};

			if (component.getBusInfo (kAudio, dir, i, busInfo) == kResultTrue)
			{
				buffers[i].numChannels = busInfo.channelCount;

				// allocate for each channel
				if (busInfo.channelCount > 0)
				{
					if (symbolicSampleSize == kSample64)
						buffers[i].channelBuffers64 = new Sample64*[busInfo.channelCount];
					else
						buffers[i].channelBuffers32 = new Sample32*[busInfo.channelCount];

					for (int32 j = 0; j < busInfo.channelCount; j++)
					{
						if (symbolicSampleSize == kSample64)
						{
							if (bufferSamples > 0)
								buffers[i].channelBuffers64[j] = new Sample64[bufferSamples];
							else
								buffers[i].channelBuffers64[j] = nullptr;
						}
						else
						{
							if (bufferSamples > 0)
								buffers[i].channelBuffers32[j] = new Sample32[bufferSamples];
							else
								buffers[i].channelBuffers32[j] = nullptr;
						}
					}
				}
			}
		}
	}
	return busCount;
}

//-----------------------------------------------------------------------------
void HostProcessData::destroyBuffers (AudioBusBuffers*& buffers, int32& busCount)
{
	if (buffers)
	{
		for (int32 i = 0; i < busCount; i++)
		{
			if (channelBufferOwner)
			{
				for (int32 j = 0; j < buffers[i].numChannels; j++)
				{
					if (symbolicSampleSize == kSample64)
					{
						if (buffers[i].channelBuffers64 && buffers[i].channelBuffers64[j])
							delete[] buffers[i].channelBuffers64[j];
					}
					else
					{
						if (buffers[i].channelBuffers32 && buffers[i].channelBuffers32[j])
							delete[] buffers[i].channelBuffers32[j];
					}
				}
			}

			if (symbolicSampleSize == kSample64)
			{
				if (buffers[i].channelBuffers64)
					delete[] buffers[i].channelBuffers64;
			}
			else
			{
				if (buffers[i].channelBuffers32)
					delete[] buffers[i].channelBuffers32;
			}
		}

		delete[] buffers;
		buffers = nullptr;
	}
	busCount = 0;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
