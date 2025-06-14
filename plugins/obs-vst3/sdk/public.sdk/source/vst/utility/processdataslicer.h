//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/processdataslicer.h
// Created by  : Steinberg, 04/2021
// Description : Process the process data in slices
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
/** Process Data Slicer
 *
 *	Cuts the VST process data into slices to process
 *
 *	Example:
 *	\code{.cpp}
 *	tresult PLUGIN_API Processor::process (ProcessData& data)
 *	{
 *		ProcessDataSlicer slicer (32);
 *		slicer.process<SymbolicSampleSizes::kSample32> (data, [&] (ProcessData& data) {
 *			doSlicedProcessing (data); // data.numSamples <= 32
 *		});
 *	}
 *	\endcode
 */
class ProcessDataSlicer
{
public:
	/** Constructor
	 *
	 *	@param inSliceSice slice size in samples
	 */
	ProcessDataSlicer (int32 inSliceSize = 8) : sliceSize (inSliceSize) {}

//------------------------------------------------------------------------
	/** Process the data
	 *
	 *	@tparam SampleSize sample size 32 or 64 bit processing
	 *	@tparam DoProcessCallback the callback proc
	 *	@param data Process data
	 *	@param doProcessing process callback
	 */
	template <SymbolicSampleSizes SampleSize, typename DoProcessCallback>
	void process (ProcessData& data, DoProcessCallback doProcessing) noexcept
	{
		stopIt = false;
		auto numSamples = data.numSamples;
		auto samplesLeft = data.numSamples;
		while (samplesLeft > 0 && !stopIt)
		{
			auto currentSliceSize = samplesLeft > sliceSize ? sliceSize : samplesLeft;

			data.numSamples = currentSliceSize;
			doProcessing (data);

			advanceBuffers<SampleSize> (data.inputs, data.numInputs, currentSliceSize);
			advanceBuffers<SampleSize> (data.outputs, data.numOutputs, currentSliceSize);
			samplesLeft -= currentSliceSize;
		}
		// revert buffer pointers (otherwise some hosts may use these wrong pointers)
		advanceBuffers<SampleSize> (data.inputs, data.numInputs, -(numSamples - samplesLeft));
		advanceBuffers<SampleSize> (data.outputs, data.numOutputs, -(numSamples - samplesLeft));
		data.numSamples = numSamples;
	}

	/** Stop the slice process
	 *
	 *	If you want to break the slice processing early, you have to capture the slicer in the
	 *	DoProcessCallback and call the stop method.
	 */
	void stop () noexcept { stopIt = true; }

private:
	template <SymbolicSampleSizes SampleSize>
	void advanceBuffers (AudioBusBuffers* buffers, int32 numBuffers, int32 numSamples) const
	    noexcept
	{
		for (auto index = 0; index < numBuffers; ++index)
		{
			for (auto channelIndex = 0; channelIndex < buffers[index].numChannels; ++channelIndex)
			{
				if (SampleSize == SymbolicSampleSizes::kSample32)
					buffers[index].channelBuffers32[channelIndex] += numSamples;
				else
					buffers[index].channelBuffers64[channelIndex] += numSamples;
			}
		}
	}
	int32 sliceSize;
	bool stopIt {false};
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
