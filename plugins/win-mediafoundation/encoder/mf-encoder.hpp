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
#include "mf-common.hpp"
#include "mf-encoder-descriptor.hpp"

#include <obs-module.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <atomic>
#include <d3d11.h>
#include <functional>
#include <memory>
#include <mfapi.h>
#include <mfidl.h>
#include <queue>
#include <vector>
#include <wmcodecdsp.h>

#include <util/windows/ComPtr.hpp>

namespace MF {

// Unified QP struct shared by all codec encoders.
struct EncQP {
	UINT16 defaultQp;
	UINT16 i;
	UINT16 p;
	UINT16 b;

	UINT64 Pack(bool packDefault) const;
};

// Unified encoded frame struct shared by all codec encoders.
struct EncFrame {
public:
	EncFrame();
	EncFrame(bool keyframe, UINT64 pts, UINT64 dts, std::unique_ptr<std::vector<uint8_t>> data);

	bool Keyframe() const { return keyframe; }
	BYTE *Data() { return data.get()->data(); }
	DWORD DataLength() { return (DWORD)data.get()->size(); }
	INT64 Pts() { return pts; }
	INT64 Dts() { return dts; }

private:
	EncFrame(EncFrame const &) = delete;
	EncFrame &operator=(EncFrame const &) = delete;

	bool keyframe;
	INT64 pts;
	INT64 dts;
	std::unique_ptr<std::vector<uint8_t>> data;
};

// Base encoder class implementing the common MFT pipeline. Codec-specific
// subclasses (H264Encoder, HEVCEncoder, AV1Encoder) override CreateMediaTypes()
// and provide their own SetRateControl() taking a codec-specific rate-control enum.
class MFBaseEncoder {
public:
	MFBaseEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
		      UINT32 height, UINT32 framerateNum, UINT32 framerateDen, int profile, UINT32 bitrate);
	virtual ~MFBaseEncoder();

	bool Initialize(std::function<bool(void)> func);
	virtual bool ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts, Status *status);
	bool ProcessInput_Tex(ID3D11Texture2D *pSurface, int64_t pts, Status *status);
	bool ProcessOutput(UINT8 **data, UINT32 *dataLength, UINT64 *pts, UINT64 *dts, bool *keyframe, Status *status);
	bool ExtraData(UINT8 **data, UINT32 *dataLength);

	const obs_encoder_t *ObsEncoder() { return encoder; }

	bool SetBitrate(UINT32 bitrate);
	bool SetQP(EncQP &qp);
	bool SetMaxBitrate(UINT32 maxBitrate);
	virtual bool SetLowLatency(bool lowLatency);
	bool SetKeyframeInterval(UINT32 seconds);
	bool SetBufferSize(UINT32 bufferSize);
	bool SetBFrameCount(UINT32 bFrames);
	bool SetMinQP(UINT32 minQp);
	bool SetMaxQP(UINT32 maxQp);

protected:
	// Codec-specific subtype, profile attribute, and input format selection.
	virtual HRESULT CreateMediaTypes(ComPtr<IMFMediaType> &inputType, ComPtr<IMFMediaType> &outputType) = 0;

	// Shared helpers for codec-specific Set* implementations in subclasses.
	bool SetRateControlMode(UINT32 mode);
	bool SetCodecBool(GUID guid, bool value);
	HRESULT CreateEmptySample(ComPtr<IMFSample> &sample, ComPtr<IMFMediaBuffer> &buffer, DWORD length);

	// Shared drain+timestamp+submit path used by ProcessInput variants.
	bool SubmitInputSample(ComPtr<IMFSample> &sample, UINT64 pts, Status *status);

	// Accessible to subclasses for use in CreateMediaTypes and ProcessInput overrides.
	const obs_encoder_t *encoder;
	std::shared_ptr<EncoderDescriptor> descriptor;
	const UINT32 width;
	const UINT32 height;
	const UINT32 framerateNum;
	const UINT32 framerateDen;
	const UINT32 initialBitrate;
	const int profile;

private:
	MFBaseEncoder(MFBaseEncoder const &) = delete;
	MFBaseEncoder &operator=(MFBaseEncoder const &) = delete;

	HRESULT InitializeEventGenerator();
	HRESULT InitializeExtraData();
	HRESULT EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length);
	HRESULT ProcessInput(ComPtr<IMFSample> &sample);
	HRESULT ProcessOutput();
	HRESULT DrainEvent(bool block);
	HRESULT DrainEvents();

	bool createOutputSample;
	ComPtr<IMFTransform> transform;
	ComPtr<ICodecAPI> codecApi;
	std::vector<BYTE> extraData;
	std::unique_ptr<EncFrame> activeFrame;
	std::queue<ComPtr<IMFSample>> inputSamples;
	std::queue<std::unique_ptr<EncFrame>> encodedFrames;
	ComPtr<IMFMediaEventGenerator> eventGenerator;
	std::atomic<UINT32> inputRequests{0};
	std::atomic<UINT32> outputRequests{0};
	std::atomic<UINT32> pendingRequests{0};
};

} // namespace MF
