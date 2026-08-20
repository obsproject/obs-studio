/*

This is provided under a dual MIT/GPLv2 license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2026 Qualcomm Technologies, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Vignesh E, vignelum@qti.qualcomm.com
Qualcomm Technologies, Inc., Bangalore, India

MIT License

Copyright (c) 2026 Qualcomm Technologies, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE

*/

#pragma once

#include <obs-module.h>

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#undef WIN32_MEAN_AND_LEAN

#include <mfapi.h>
#include <mfidl.h>

#include <wmcodecdsp.h>

#include <vector>
#include <queue>
#include <memory>
#include <atomic>

#include <util/windows/ComPtr.hpp>

#include "mf-encoder-descriptor.hpp"
#include "mf-common.hpp"

namespace MF {
enum HEVCProfile {
	HEVCProfileMain,

};

enum HEVCRateControl {
	HEVCRateControlCBR,
	HEVCRateControlVBR,
};

struct HEVCQP {
	UINT16 defaultQp;
	UINT16 i;
	UINT16 p;
	UINT16 b;

	UINT64 Pack(bool packDefault)
	{
		int shift = packDefault ? 0 : 16;
		UINT64 packedQp;
		if (packDefault)
			packedQp = defaultQp;

		packedQp |= i << shift;
		shift += 16;
		packedQp |= p << shift;
		shift += 16;
		packedQp |= b << shift;

		return packedQp;
	}
};

struct HEVCFrame {
public:
	HEVCFrame() : keyframe(false), pts(0), dts(0), data(std::make_unique<std::vector<uint8_t>>()) {}
	HEVCFrame(bool keyframe, UINT64 pts, UINT64 dts, std::unique_ptr<std::vector<uint8_t>> data)
		: keyframe(keyframe),
		  pts(pts),
		  dts(dts),
		  data(std::move(data))
	{
	}
	bool Keyframe() { return keyframe; }
	BYTE *Data() { return data.get()->data(); }
	DWORD DataLength() { return (DWORD)data.get()->size(); }
	INT64 Pts() { return pts; }
	INT64 Dts() { return dts; }

private:
	HEVCFrame(HEVCFrame const &) = delete;
	HEVCFrame &operator=(HEVCFrame const &) = delete;

private:
	bool keyframe;
	INT64 pts;
	INT64 dts;
	std::unique_ptr<std::vector<uint8_t>> data;
};

class HEVCEncoder {
public:
	HEVCEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
		    UINT32 height, UINT32 framerateNum, UINT32 framerateDen, HEVCProfile profile, UINT32 bitrate);

	~HEVCEncoder();

	bool Initialize(std::function<bool(void)> func);
	bool ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts, Status *status);
	bool ProcessOutput(UINT8 **data, UINT32 *dataLength, UINT64 *pts, UINT64 *dts, bool *keyframe, Status *status);
	bool ExtraData(UINT8 **data, UINT32 *dataLength);

	const obs_encoder_t *ObsEncoder() { return encoder; }

public:
	bool SetBitrate(UINT32 bitrate);
	bool SetQP(HEVCQP &qp);
	bool SetMaxBitrate(UINT32 maxBitrate);
	bool SetRateControl(HEVCRateControl rateControl);
	bool SetKeyframeInterval(UINT32 seconds);
	bool SetLowLatency(bool lowLatency);
	bool SetBufferSize(UINT32 bufferSize);
	bool SetBFrameCount(UINT32 bFrames);
	bool SetMinQP(UINT32 minQp);
	bool SetMaxQP(UINT32 maxQp);

private:
	HEVCEncoder(HEVCEncoder const &) = delete;
	HEVCEncoder &operator=(HEVCEncoder const &) = delete;

private:
	HRESULT InitializeEventGenerator();
	HRESULT InitializeExtraData();
	HRESULT CreateMediaTypes(ComPtr<IMFMediaType> &inputType, ComPtr<IMFMediaType> &outputType);
	HRESULT EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length);
	HRESULT CreateEmptySample(ComPtr<IMFSample> &sample, ComPtr<IMFMediaBuffer> &buffer, DWORD length);

	HRESULT ProcessInput(ComPtr<IMFSample> &sample);
	HRESULT ProcessOutput();

	HRESULT DrainEvent(bool block);
	HRESULT DrainEvents();

private:
	const obs_encoder_t *encoder;
	std::shared_ptr<EncoderDescriptor> descriptor;
	const UINT32 width;
	const UINT32 height;
	const UINT32 framerateNum;
	const UINT32 framerateDen;
	const UINT32 initialBitrate;
	const HEVCProfile profile;

	bool createOutputSample;
	ComPtr<IMFTransform> transform;
	ComPtr<ICodecAPI> codecApi;

	std::vector<BYTE> extraData;

	// The frame returned by ProcessOutput
	// Valid until the next call to ProcessOutput
	std::unique_ptr<HEVCFrame> activeFrame;

	// Queued input samples that the encoder was not ready
	// to process
	std::queue<ComPtr<IMFSample>> inputSamples;

	// Queued output samples that have not been returned from
	// ProcessOutput yet
	std::queue<std::unique_ptr<HEVCFrame>> encodedFrames;

	ComPtr<IMFMediaEventGenerator> eventGenerator;
	std::atomic<UINT32> inputRequests{0};
	std::atomic<UINT32> outputRequests{0};
	std::atomic<UINT32> pendingRequests{0};
};
} // namespace MF
