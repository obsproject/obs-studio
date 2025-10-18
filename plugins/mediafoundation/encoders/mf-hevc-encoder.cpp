#include <obs-module.h>
#include <util/profiler.hpp>

#include "mf-common.hpp"
#include "mf-hevc-encoder.hpp"

#include <codecapi.h>
#include <mferror.h>

namespace {
eAVEncH265VProfile MapProfile(MF::HEVCProfile profile)
{
	switch (profile) {
	case MF::HEVCProfileMain:
		return eAVEncH265VProfile_Main_420_8;
	default:
		return eAVEncH265VProfile_Main_420_8;
	}
}

eAVEncCommonRateControlMode MapRateControl(MF::HEVCRateControl rc)
{
	switch (rc) {
	case MF::HEVCRateControlCBR:
		return eAVEncCommonRateControlMode_CBR;
	case MF::HEVCRateControlVBR:
		return eAVEncCommonRateControlMode_UnconstrainedVBR;
	default:
		return eAVEncCommonRateControlMode_CBR;
	}
}

UINT32 MapQpToQuality(MF::HEVCQP &qp)
{
	return 100 - (UINT32)floor(100.0 / 51.0 * qp.defaultQp + 0.5f);
}

bool ProcessNV12(std::function<void(UINT32 height, INT32 plane)> func, UINT32 height)
{
	INT32 plane = 0;

	func(height, plane++);
	func(height / 2, plane);

	return true;
}
} // namespace

MF::HEVCEncoder::HEVCEncoder(const obs_encoder_t *encoder, std::shared_ptr<EncoderDescriptor> descriptor, UINT32 width,
			     UINT32 height, UINT32 framerateNum, UINT32 framerateDen, MF::HEVCProfile profile,
			     UINT32 bitrate)
	: encoder(encoder),
	  descriptor(descriptor),
	  width(width),
	  height(height),
	  framerateNum(framerateNum),
	  framerateDen(framerateDen),
	  initialBitrate(bitrate),
	  profile(profile)
{
}

MF::HEVCEncoder::~HEVCEncoder()
{
	HRESULT hr;

	if (!descriptor->Async() || !eventGenerator || !pendingRequests)
		return;

	// Make sure all events have finished before releasing, and drain
	// all output requests until it makes an input request.
	// If you do not do this, you risk it releasing while there's still
	// encoder activity, which can cause a crash with certain interfaces.
	while (inputRequests == 0) {
		hr = ProcessOutput();
		if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr)) {
			MF::MF_LOG_COM(LOG_ERROR,
				       "HEVCEncoder::~HEVCEncoder: "
				       "ProcessOutput()",
				       hr);
			break;
		}

		if (inputRequests == 0)
			Sleep(1);
	}
}

HRESULT MF::HEVCEncoder::CreateMediaTypes(ComPtr<IMFMediaType> &i, ComPtr<IMFMediaType> &o)
{
	HRESULT hr;
	CHECK_HR_ERROR(MFCreateMediaType(&i));
	CHECK_HR_ERROR(MFCreateMediaType(&o));

	CHECK_HR_ERROR(i->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR_ERROR(i->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
	CHECK_HR_ERROR(MFSetAttributeSize(i, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(i->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(i, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));

	CHECK_HR_ERROR(o->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	CHECK_HR_ERROR(o->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC));
	CHECK_HR_ERROR(MFSetAttributeSize(o, MF_MT_FRAME_SIZE, width, height));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_FRAME_RATE, framerateNum, framerateDen));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_AVG_BITRATE, initialBitrate * 1000));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlaceMode::MFVideoInterlace_Progressive));
	CHECK_HR_ERROR(MFSetAttributeRatio(o, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_VIDEO_LEVEL, (UINT32)-1));
	CHECK_HR_ERROR(o->SetUINT32(MF_MT_MPEG2_PROFILE, MapProfile(profile)));

	return S_OK;

fail:
	return hr;
}

HRESULT MF::HEVCEncoder::DrainEvents()
{
	HRESULT hr;
	while ((hr = DrainEvent(false)) == S_OK)
		;
	if (hr == MF_E_NO_EVENTS_AVAILABLE)
		hr = S_OK;
	return hr;
}

HRESULT MF::HEVCEncoder::DrainEvent(bool block)
{
	HRESULT hr, eventStatus;
	ComPtr<IMFMediaEvent> event;
	MediaEventType type;

	hr = eventGenerator->GetEvent(block ? 0 : MF_EVENT_FLAG_NO_WAIT, &event);

	if (hr != MF_E_NO_EVENTS_AVAILABLE && FAILED(hr))
		goto fail;
	if (hr == MF_E_NO_EVENTS_AVAILABLE)
		return hr;

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

HRESULT MF::HEVCEncoder::InitializeEventGenerator()
{
	HRESULT hr;

	CHECK_HR_ERROR(transform->QueryInterface(&eventGenerator));

	return S_OK;

fail:
	return hr;
}

HRESULT MF::HEVCEncoder::InitializeExtraData()
{
	HRESULT hr;
	ComPtr<IMFMediaType> inputType;
	UINT32 headerSize;

	extraData.clear();

	CHECK_HR_ERROR(transform->GetOutputCurrentType(0, &inputType));

	CHECK_HR_ERROR(inputType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &headerSize));

	extraData.resize(headerSize);

	CHECK_HR_ERROR(inputType->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, extraData.data(), headerSize, NULL));

	return S_OK;

fail:
	return hr;
}

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
} // namespace

bool MF::HEVCEncoder::SetBitrate(UINT32 bitrate)
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

bool MF::HEVCEncoder::SetQP(MF::HEVCQP &qp)
{
	HRESULT hr;
	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING,
			       SetCodecProperty(codecApi, CODECAPI_AVEncCommonQuality, UINT32(MapQpToQuality(qp))));
		CHECK_HR_WARNING(SetCodecProperty(codecApi, CODECAPI_AVEncVideoEncodeQP, UINT64(qp.Pack(true))));
		CHECK_HR_WARNING(
			SetCodecProperty(codecApi, CODECAPI_AVEncVideoEncodeFrameTypeQP, UINT64(qp.Pack(false))));
	}

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::SetMinQP(UINT32 minQp)
{
	HRESULT hr;

	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncVideoMinQP, UINT32(minQp)));
	}

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::SetMaxQP(UINT32 maxQp)
{
	HRESULT hr;

	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncVideoMaxQP, UINT32(maxQp)));
	}

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::SetRateControl(MF::HEVCRateControl rateControl)
{
	HRESULT hr;

	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncCommonRateControlMode,
							     UINT32(MapRateControl(rateControl))));
	}

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::SetKeyframeInterval(UINT32 seconds)
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

bool MF::HEVCEncoder::SetMaxBitrate(UINT32 maxBitrate)
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

bool MF::HEVCEncoder::SetLowLatency(bool lowLatency)
{
	HRESULT hr;

	if (codecApi) {
		CHECK_HR_LEVEL(LOG_WARNING, SetCodecProperty(codecApi, CODECAPI_AVEncCommonLowLatency, lowLatency));
	}

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::SetBufferSize(UINT32 bufferSize)
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

bool MF::HEVCEncoder::SetBFrameCount(UINT32 bFrames)
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

bool MF::HEVCEncoder::Initialize(std::function<bool(void)> func)
{
	ProfileScope("HEVCEncoder::Initialize");

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
		MF::MF_LOG(LOG_ERROR, "Failed setting custom properties");
		goto fail;
	}

	MF::MF_LOG(LOG_INFO, "Activating encoder: %s", typeNames[(int)descriptor->Type()]);

	MF::MF_LOG(LOG_INFO, "  Setting output type to transform:");
	MF::LogMediaType(outputType.Get());
	CHECK_HR_ERROR(transform->SetOutputType(0, outputType.Get(), 0));

	MF::MF_LOG(LOG_INFO, "  Setting input type to transform:");
	MF::LogMediaType(inputType.Get());
	CHECK_HR_ERROR(transform->SetInputType(0, inputType.Get(), 0));

	CHECK_HR_ERROR(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL));

	CHECK_HR_ERROR(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL));

	if (descriptor->Async())
		CHECK_HR_ERROR(InitializeEventGenerator());

	CHECK_HR_ERROR(transform->GetOutputStreamInfo(0, &streamInfo));
	createOutputSample =
		!(streamInfo.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES));

	return true;

fail:
	return false;
}

bool MF::HEVCEncoder::ExtraData(UINT8 **data, UINT32 *dataLength)
{
	if (extraData.empty())
		return false;

	*data = extraData.data();
	*dataLength = (UINT32)extraData.size();

	return true;
}

HRESULT MF::HEVCEncoder::CreateEmptySample(ComPtr<IMFSample> &sample, ComPtr<IMFMediaBuffer> &buffer, DWORD length)
{
	HRESULT hr;

	CHECK_HR_ERROR(MFCreateSample(&sample));
	CHECK_HR_ERROR(MFCreateMemoryBuffer(length, &buffer));
	CHECK_HR_ERROR(sample->AddBuffer(buffer.Get()));
	return S_OK;

fail:
	return hr;
}

HRESULT MF::HEVCEncoder::EnsureCapacity(ComPtr<IMFSample> &sample, DWORD length)
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

HRESULT MF::HEVCEncoder::ProcessInput(ComPtr<IMFSample> &sample)
{
	ProfileScope("HEVCEncoder::ProcessInput(sample)");

	HRESULT hr = S_OK;
	if (descriptor->Async()) {
		if (inputRequests == 1 && inputSamples.empty()) {
			inputRequests--;
			return transform->ProcessInput(0, sample, 0);
		}

		inputSamples.push(sample);

		while (inputRequests > 0) {
			if (inputSamples.empty())
				return hr;
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

bool MF::HEVCEncoder::ProcessInput(UINT8 **data, UINT32 *linesize, UINT64 pts, MF::Status *status)
{
	ProfileScope("HEVCEncoder::ProcessInput");

	HRESULT hr;
	ComPtr<IMFSample> sample;
	ComPtr<IMFMediaBuffer> buffer;
	BYTE *bufferData;
	UINT64 sampleDur;
	UINT32 imageSize;

	CHECK_HR_ERROR(MFCalculateImageSize(MFVideoFormat_NV12, width, height, &imageSize));

	CHECK_HR_ERROR(CreateEmptySample(sample, buffer, imageSize));

	{
		ProfileScope("HEVCEncoderCopyInputSample");

		CHECK_HR_ERROR(buffer->Lock(&bufferData, NULL, NULL));

		ProcessNV12(
			[&, this](DWORD height, int plane) {
				MFCopyImage(bufferData, width, data[plane], linesize[plane], width, height);
				bufferData += width * height;
			},
			height);
	}

	CHECK_HR_ERROR(buffer->Unlock());
	CHECK_HR_ERROR(buffer->SetCurrentLength(imageSize));

	MFFrameRateToAverageTimePerFrame(framerateNum, framerateDen, &sampleDur);

	CHECK_HR_ERROR(sample->SetSampleTime(pts * sampleDur));
	CHECK_HR_ERROR(sample->SetSampleDuration(sampleDur));

	if (descriptor->Async()) {
		CHECK_HR_ERROR(DrainEvents());

		while (outputRequests > 0 && (hr = ProcessOutput()) == S_OK)
			;

		if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr)) {
			MF::MF_LOG_COM(LOG_ERROR, "ProcessOutput()", hr);
			goto fail;
		}

		while (inputRequests == 0) {
			hr = DrainEvent(false);
			if (hr == MF_E_NO_EVENTS_AVAILABLE) {
				Sleep(1);
				continue;
			}
			if (FAILED(hr)) {
				MF::MF_LOG_COM(LOG_ERROR, "DrainEvent()", hr);
				goto fail;
			}
			if (outputRequests > 0) {
				hr = ProcessOutput();
				if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT && FAILED(hr))
					goto fail;
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

HRESULT MF::HEVCEncoder::ProcessOutput()
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
	std::unique_ptr<MF::HEVCFrame> frame;
	bool keyframe = false;

	if (descriptor->Async()) {
		CHECK_HR_ERROR(DrainEvents());

		if (outputRequests == 0)
			return S_OK;

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

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
			return hr;

		if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
			CHECK_HR_ERROR(transform->GetOutputAvailableType(0, 0, &type));
			CHECK_HR_ERROR(transform->SetOutputType(0, type, 0));
			MF::MF_LOG(LOG_INFO, "Updating output type to transform");
			MF::LogMediaType(type);
			if (descriptor->Async() && outputRequests > 0) {
				outputRequests--;
				continue;
			} else {
				return MF_E_TRANSFORM_NEED_MORE_INPUT;
			}
		}

		if (hr != S_OK) {
			MF::MF_LOG_COM(LOG_ERROR, "transform->ProcessOutput()", hr);
			return hr;
		}

		break;
	}

	if (!createOutputSample)
		sample.Set(output.pSample);

	CHECK_HR_ERROR(sample->GetBufferByIndex(0, &buffer));

	keyframe = !!MFGetAttributeUINT32(sample, MFSampleExtension_CleanPoint, FALSE);

	CHECK_HR_ERROR(buffer->Lock(&bufferData, NULL, &bufferLength));

	if (keyframe && extraData.empty())
		CHECK_HR_ERROR(InitializeExtraData());

	data->reserve(bufferLength + extraData.size());

	if (keyframe)
		data->insert(data->end(), extraData.begin(), extraData.end());

	data->insert(data->end(), &bufferData[0], &bufferData[bufferLength]);
	CHECK_HR_ERROR(buffer->Unlock());

	CHECK_HR_ERROR(sample->GetSampleDuration(&sampleDur));
	CHECK_HR_ERROR(sample->GetSampleTime(&samplePts));

	sampleDts = MFGetAttributeUINT64(sample, MFSampleExtension_DecodeTimestamp, samplePts);

	frame.reset(new MF::HEVCFrame(keyframe, samplePts / sampleDur, sampleDts / sampleDur, std::move(data)));

	encodedFrames.push(std::move(frame));

	return S_OK;

fail:
	return hr;
}

bool MF::HEVCEncoder::ProcessOutput(UINT8 **data, UINT32 *dataLength, UINT64 *pts, UINT64 *dts, bool *keyframe,
				    MF::Status *status)
{
	ProfileScope("HEVCEncoder::ProcessOutput");

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

	*data = activeFrame.get()->Data();
	*dataLength = activeFrame.get()->DataLength();
	*pts = activeFrame.get()->Pts();
	*dts = activeFrame.get()->Dts();
	*keyframe = activeFrame.get()->Keyframe();
	*status = SUCCESS;

	pendingRequests--;

	return true;
}
