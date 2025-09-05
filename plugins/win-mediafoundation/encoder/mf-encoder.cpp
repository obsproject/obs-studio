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

#include "mf-encoder.hpp"

#include <obs-module.h>
#include <util/profiler.hpp>

#include <atlbase.h>
#include <codecapi.h>
#include <mferror.h>
#include <windows.h>

#include <Codecapi.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfplay.h>
#include <mfreadwrite.h>

namespace {

HRESULT SetCodecProperty(ComPtr<ICodecAPI> &codecApi, GUID guid, bool value)
{
	VARIANT v;
	v.vt = VT_BOOL;
	v.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
	return codecApi->SetValue(&guid, &v);
}

HRESULT SetCodecProperty(ComPtr<ICodecAPI> &codecApi, GUID guid, UINT32 value)
{
	VARIANT v;
	v.vt = VT_UI4;
	v.ulVal = value;
	return codecApi->SetValue(&guid, &v);
}

HRESULT SetCodecProperty(ComPtr<ICodecAPI> &codecApi, GUID guid, UINT64 value)
{
	VARIANT v;
	v.vt = VT_UI8;
	v.ullVal = value;
	return codecApi->SetValue(&guid, &v);
}

bool ProcessNV12(std::function<void(UINT32 height, INT32 plane)> func, UINT32 height)
{
	INT32 plane = 0;
	func(height, plane++);
	func(height / 2, plane);
	return true;
}

} // namespace

// ---------------------------------------------------------------------------
// EncQP
// ---------------------------------------------------------------------------

UINT64 MF::EncQP::Pack(bool packDefault) const
{
	int shift = packDefault ? 0 : 16;
	UINT64 packedQp = 0;
	if (packDefault) {
		packedQp = defaultQp;
	}

	packedQp |= static_cast<UINT32>(i) << shift;
	shift += 16;
	packedQp |= static_cast<UINT32>(p) << shift;
	shift += 16;
	packedQp |= static_cast<UINT32>(b) << shift;
	return packedQp;
}

// ---------------------------------------------------------------------------
// EncFrame
// ---------------------------------------------------------------------------

MF::EncFrame::EncFrame() : keyframe(false), pts(0), dts(0), data(std::make_unique<std::vector<uint8_t>>()) {}

MF::EncFrame::EncFrame(bool keyframe, UINT64 pts, UINT64 dts, std::unique_ptr<std::vector<uint8_t>> data)
	: keyframe(keyframe),
	  pts(pts),
	  dts(dts),
	  data(std::move(data))
{
}

// ---------------------------------------------------------------------------
// MFBaseEncoder
// ---------------------------------------------------------------------------

MF::MFBaseEncoder::MFBaseEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor,
				 UINT32 width, UINT32 height, UINT32 framerateNum, UINT32 framerateDen, int profile,
				 UINT32 bitrate)
	: encoder(encoder),
	  descriptor(descriptor),
	  width(width),
	  height(height),
	  framerateNum(framerateNum),
	  framerateDen(framerateDen),
	  initialBitrate(bitrate),
	  profile(profile),
	  createOutputSample(false)
{
}

MF::MFBaseEncoder::~MFBaseEncoder()
{
	HRESULT hr;

	if (!descriptor->Async() || !eventGenerator || !pendingRequests) {
		return;
	}

	// Drain all output until the MFT issues another input request, to avoid
	// releasing the transform while it still has in-flight work.
	while (inputRequests == 0) {
		hr = ProcessOutput();
		if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr)) {
			MF_LOG_COM(LOG_ERROR, "MFBaseEncoder::~MFBaseEncoder: ProcessOutput()", hr);
			break;
		}

		if (inputRequests == 0) {
			Sleep(1);
		}
	}
}

HRESULT MF::MFBaseEncoder::InitializeEventGenerator()
{
	HRESULT hr;
	CHECK_HR_ERROR(transform->QueryInterface(&eventGenerator));
	return S_OK;
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::InitializeExtraData()
{
	HRESULT hr;
	ComPtr<IMFMediaType> outputType;
	UINT32 headerSize;

	extraData.clear();

	CHECK_HR_ERROR(transform->GetOutputCurrentType(0, &outputType));
	CHECK_HR_ERROR(outputType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &headerSize));
	extraData.resize(headerSize);
	CHECK_HR_ERROR(outputType->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, extraData.data(), headerSize, NULL));
	return S_OK;
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::DrainEvents()
{
	HRESULT hr;
	while ((hr = DrainEvent(false)) == S_OK) {
		;
	}

	if (hr == MF_E_NO_EVENTS_AVAILABLE) {
		hr = S_OK;
	}

	return hr;
}

HRESULT MF::MFBaseEncoder::DrainEvent(bool block)
{
	HRESULT hr, eventStatus;
	ComPtr<IMFMediaEvent> event;
	MediaEventType type;

	hr = eventGenerator->GetEvent(block ? 0 : MF_EVENT_FLAG_NO_WAIT, &event);

	if (hr != MF_E_NO_EVENTS_AVAILABLE && FAILED(hr)) {
		return hr;
	}

	if (hr == MF_E_NO_EVENTS_AVAILABLE) {
		return hr;
	}

	CHECK_HR_ERROR(event->GetType(&type));
	CHECK_HR_ERROR(event->GetStatus(&eventStatus));

	if (SUCCEEDED(eventStatus)) {
		if (type == METransformNeedInput) {
			inputRequests++;
		} else if (type == METransformHaveOutput) {
			outputRequests++;
		}
	}

	return S_OK;
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::CreateEmptySample(ComPtr<IMFSample> &sample, ComPtr<IMFMediaBuffer> &buffer, DWORD length)
{
	HRESULT hr;
	CHECK_HR_ERROR(MFCreateSample(&sample));
	CHECK_HR_ERROR(MFCreateMemoryBuffer(length, &buffer));
	CHECK_HR_ERROR(sample->AddBuffer(buffer.Get()));
	return S_OK;
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length)
{
	HRESULT hr;
	ComPtr<IMFMediaBuffer> buffer;
	DWORD currentLength;

	if (!sample) {
		CHECK_HR_ERROR(CreateEmptySample(sample, buffer, length));
	} else {
		CHECK_HR_ERROR(sample->GetBufferByIndex(0, &buffer));
	}

	CHECK_HR_ERROR(buffer->GetMaxLength(&currentLength));
	if (currentLength < length) {
		CHECK_HR_ERROR(sample->RemoveAllBuffers());
		CHECK_HR_ERROR(MFCreateMemoryBuffer(length, &buffer));
		CHECK_HR_ERROR(sample->AddBuffer(buffer));
	} else {
		buffer->SetCurrentLength(0);
	}

	return S_OK;
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::ProcessInput(ComPtr<IMFSample> &sample)
{
	HRESULT hr = S_OK;
	if (descriptor->Async()) {
		if (inputRequests == 1 && inputSamples.empty()) {
			inputRequests--;
			return transform->ProcessInput(0, sample, 0);
		}

		inputSamples.push(sample);

		while (inputRequests > 0) {
			if (inputSamples.empty()) {
				return hr;
			}
			ComPtr<IMFSample> queuedSample = inputSamples.front();
			inputSamples.pop();
			inputRequests--;
			CHECK_HR_ERROR(transform->ProcessInput(0, queuedSample, 0));
		}
	} else {
		return transform->ProcessInput(0, sample, 0);
	}
fail:
	return hr;
}

HRESULT MF::MFBaseEncoder::ProcessOutput()
{
	HRESULT hr;
	ComPtr<IMFSample> sample;
	MFT_OUTPUT_STREAM_INFO outputInfo = {0};
	DWORD outputStatus = 0;
	MFT_OUTPUT_DATA_BUFFER output = {0};
	ComPtr<IMFMediaBuffer> buffer;
	BYTE *bufferData;
	DWORD bufferLength;
	INT64 samplePts;
	INT64 sampleDts;
	INT64 sampleDur;
	auto data = std::make_unique<std::vector<BYTE>>();
	ComPtr<IMFMediaType> type;
	bool keyframe = false;

	if (descriptor->Async()) {
		CHECK_HR_ERROR(DrainEvents());
		if (outputRequests == 0) {
			return S_OK;
		}

		outputRequests--;
	}

	if (createOutputSample) {
		CHECK_HR_ERROR(transform->GetOutputStreamInfo(0, &outputInfo));
		CHECK_HR_ERROR(CreateEmptySample(sample, buffer, outputInfo.cbSize));
		output.pSample = sample;
	} else {
		output.pSample = NULL;
	}

	while (true) {
		hr = transform->ProcessOutput(0, 1, &output, &outputStatus);
		ComPtr<IMFCollection> events(output.pEvents);

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
			return hr;
		}

		if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
			CHECK_HR_ERROR(transform->GetOutputAvailableType(0, 0, &type));
			CHECK_HR_ERROR(transform->SetOutputType(0, type, 0));
			MF_LOG(LOG_INFO, "Updating output type to transform");
			LogMediaType(type);
			if (descriptor->Async() && outputRequests > 0) {
				outputRequests--;
				continue;
			} else {
				return MF_E_TRANSFORM_NEED_MORE_INPUT;
			}
		}

		if (hr != S_OK) {
			MF_LOG_COM(LOG_ERROR, "transform->ProcessOutput()", hr);
			return hr;
		}

		break;
	}

	if (!createOutputSample) {
		sample.Set(output.pSample);
	}

	CHECK_HR_ERROR(sample->GetBufferByIndex(0, &buffer));

	keyframe = !!MFGetAttributeUINT32(sample, MFSampleExtension_CleanPoint, FALSE);

	CHECK_HR_ERROR(buffer->Lock(&bufferData, NULL, &bufferLength));

	if (keyframe && extraData.empty()) {
		CHECK_HR_ERROR(InitializeExtraData());
	}

	data->reserve(bufferLength + extraData.size());

	if (keyframe) {
		data->insert(data->end(), extraData.begin(), extraData.end());
	}

	data->insert(data->end(), &bufferData[0], &bufferData[bufferLength]);
	CHECK_HR_ERROR(buffer->Unlock());

	CHECK_HR_ERROR(sample->GetSampleDuration(&sampleDur));
	CHECK_HR_ERROR(sample->GetSampleTime(&samplePts));
	sampleDts = MFGetAttributeUINT64(sample, MFSampleExtension_DecodeTimestamp, samplePts);

	encodedFrames.push(
		std::make_unique<EncFrame>(keyframe, samplePts / sampleDur, sampleDts / sampleDur, std::move(data)));
	return S_OK;
fail:
	return hr;
}

bool MF::MFBaseEncoder::Initialize(std::function<bool(void)> func)
{
	ProfileScope("MFBaseEncoder::Initialize");

	HRESULT hr;
	ComPtr<IMFMediaType> inputType, outputType;
	ComPtr<IMFAttributes> transformAttributes;
	MFT_OUTPUT_STREAM_INFO streamInfo = {0};

	CHECK_HR_ERROR(CoCreateInstance(descriptor->Guid(), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&transform)));

	CHECK_HR_ERROR(CreateMediaTypes(inputType, outputType));

	if (descriptor->Async()) {
		CHECK_HR_ERROR(transform->GetAttributes(&transformAttributes));
		CHECK_HR_ERROR(transformAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE));
	}

	CHECK_HR_ERROR(transform->QueryInterface(&codecApi));

	if (func && !func()) {
		MF_LOG(LOG_ERROR, "Failed setting custom properties");
		return false;
	}

	MF_LOG(LOG_INFO, "Activating encoder: %s", typeNames[(int)descriptor->Type()]);

	MF_LOG(LOG_INFO, "  Setting output type to transform:");
	LogMediaType(outputType.Get());
	CHECK_HR_ERROR(transform->SetOutputType(0, outputType.Get(), 0));

	MF_LOG(LOG_INFO, "  Setting input type to transform:");
	LogMediaType(inputType.Get());
	CHECK_HR_ERROR(transform->SetInputType(0, inputType.Get(), 0));

	CHECK_HR_ERROR(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL));
	CHECK_HR_ERROR(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL));

	if (descriptor->Async()) {
		CHECK_HR_ERROR(InitializeEventGenerator());
	}

	CHECK_HR_ERROR(transform->GetOutputStreamInfo(0, &streamInfo));
	createOutputSample =
		!(streamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES));

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::ExtraData(UINT8 **data, UINT32 *dataLength)
{
	if (extraData.empty()) {
		return false;
	}

	*data = extraData.data();
	*dataLength = static_cast<UINT32>(extraData.size());
	return true;
}

bool MF::MFBaseEncoder::SubmitInputSample(ComPtr<IMFSample> &sample, UINT64 pts, Status *status)
{
	HRESULT hr;
	UINT64 sampleDur;

	MFFrameRateToAverageTimePerFrame(framerateNum, framerateDen, &sampleDur);
	CHECK_HR_ERROR(sample->SetSampleTime(pts * sampleDur));
	CHECK_HR_ERROR(sample->SetSampleDuration(sampleDur));

	if (descriptor->Async()) {
		CHECK_HR_ERROR(DrainEvents());

		while (outputRequests > 0 && (hr = ProcessOutput()) == S_OK) {
			;
		}

		if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr)) {
			MF_LOG_COM(LOG_ERROR, "ProcessOutput()", hr);
			goto fail;
		}

		while (inputRequests == 0) {
			hr = DrainEvent(false);
			if (hr == MF_E_NO_EVENTS_AVAILABLE) {
				Sleep(1);
				continue;
			}
			if (FAILED(hr)) {
				MF_LOG_COM(LOG_ERROR, "DrainEvent()", hr);
				goto fail;
			}
			if (outputRequests > 0) {
				hr = ProcessOutput();
				if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr)) {
					goto fail;
				}
			}
		}
	}

	CHECK_HR_ERROR(ProcessInput(sample));

	pendingRequests++;
	*status = SUCCESS;
	return true;
fail:
	*status = FAILURE;
	return false;
}

bool MF::MFBaseEncoder::ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts, Status *status)
{
	ProfileScope("MFBaseEncoder::ProcessInput");

	HRESULT hr;
	ComPtr<IMFSample> sample;
	ComPtr<IMFMediaBuffer> buffer;
	BYTE *bufferData;
	UINT32 imageSize;

	CHECK_HR_ERROR(MFCalculateImageSize(MFVideoFormat_NV12, width, height, &imageSize));
	CHECK_HR_ERROR(CreateEmptySample(sample, buffer, imageSize));

	{
		ProfileScope("MFBaseEncoderCopyInputSample");
		CHECK_HR_ERROR(buffer->Lock(&bufferData, NULL, NULL));
		ProcessNV12(
			[&, this](DWORD h, int plane) {
				MFCopyImage(bufferData, width, data[plane], linesize[plane], width, h);
				bufferData += width * h;
			},
			height);
	}

	CHECK_HR_ERROR(buffer->Unlock());
	CHECK_HR_ERROR(buffer->SetCurrentLength(imageSize));

	return SubmitInputSample(sample, pts, status);
fail:
	*status = FAILURE;
	return false;
}

bool MF::MFBaseEncoder::ProcessInput_Tex(ID3D11Texture2D *pSurface, int64_t pts, Status *status)
{
	ProfileScope("MFBaseEncoder::ProcessInput_Tex");

	HRESULT hr;
	ComPtr<IMFSample> sample;
	ComPtr<IMFMediaBuffer> buffer;

	CHECK_HR_ERROR(MFCreateSample(&sample));
	CHECK_HR_ERROR(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), pSurface, 0, FALSE, &buffer));
	CHECK_HR_ERROR(sample->AddBuffer(buffer.Get()));

	return SubmitInputSample(sample, static_cast<UINT32>(pts), status);
fail:
	*status = FAILURE;
	return false;
}

bool MF::MFBaseEncoder::ProcessOutput(UINT8 **data, UINT32 *dataLength, UINT64 *pts, UINT64 *dts, bool *keyframe,
				      Status *status)
{
	ProfileScope("MFBaseEncoder::ProcessOutput");

	HRESULT hr;
	hr = ProcessOutput();

	if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT || encodedFrames.empty()) {
		*status = NEED_MORE_INPUT;
		return true;
	}

	if (FAILED(hr) && encodedFrames.empty()) {
		*status = FAILURE;
		return false;
	}

	activeFrame = std::move(encodedFrames.front());
	encodedFrames.pop();

	*data = activeFrame->Data();
	*dataLength = activeFrame->DataLength();
	*pts = activeFrame->Pts();
	*dts = activeFrame->Dts();
	*keyframe = activeFrame->Keyframe();
	*status = SUCCESS;

	pendingRequests--;
	return true;
}

bool MF::MFBaseEncoder::SetBitrate(UINT32 bitrate)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncCommonMeanBitRate, UINT32(bitrate * 1000)));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetQP(EncQP &qp)
{
	HRESULT hr;
	UINT32 quality = 100 - static_cast<UINT32>(floor(100.0 / 51.0 * qp.defaultQp + 0.5f));
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncCommonQuality, quality));
		CHECK_HR_WARNING(SetCodecProperty(codecApi, CODECAPI_AVEncVideoEncodeQP, UINT64(qp.Pack(true))));
		CHECK_HR_WARNING(
			SetCodecProperty(codecApi, CODECAPI_AVEncVideoEncodeFrameTypeQP, UINT64(qp.Pack(false))));
	}
	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetRateControlMode(UINT32 mode)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncCommonRateControlMode, UINT32(mode)));
	}
	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetCodecBool(GUID guid, bool value)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, guid, value));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetLowLatency(bool lowLatency)
{
	return SetCodecBool(CODECAPI_AVLowLatencyMode, lowLatency);
}

bool MF::MFBaseEncoder::SetKeyframeInterval(UINT32 seconds)
{
	HRESULT hr;
	if (codecApi) {
		float gopSize = float(framerateNum) / framerateDen * seconds;
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncMPVGOPSize, UINT32(gopSize)));
	}
	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetMaxBitrate(UINT32 maxBitrate)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncCommonMaxBitRate, UINT32(maxBitrate * 1000)));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetBufferSize(UINT32 bufferSize)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncCommonBufferSize, UINT32(bufferSize * 1000)));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetBFrameCount(UINT32 bFrames)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncMPVDefaultBPictureCount, UINT32(bFrames)));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetMinQP(UINT32 minQp)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncVideoMinQP, UINT32(minQp)));
	}

	return true;
fail:
	return false;
}

bool MF::MFBaseEncoder::SetMaxQP(UINT32 maxQp)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncVideoMaxQP, UINT32(maxQp)));
	}
	return true;
fail:
	return false;
}
