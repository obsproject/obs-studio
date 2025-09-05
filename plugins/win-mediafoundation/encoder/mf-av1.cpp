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

#include "mf-av1-encoder.hpp"
#include "mf-encoder-descriptor.hpp"

#include <obs-module.h>
#include <util/profiler.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <VersionHelpers.h>

using namespace MF;

struct MFAV1_Encoder {
	obs_encoder_t *encoder;
	std::shared_ptr<EncoderDescriptor> descriptor;
	std::unique_ptr<AV1Encoder> av1Encoder;
	uint32_t width;
	uint32_t height;
	uint32_t framerateNum;
	uint32_t framerateDen;
	uint32_t keyint;
	uint32_t bitrate;
	uint32_t maxBitrate;
	bool useMaxBitrate;
	uint32_t bufferSize;
	bool useBufferSize;
	AV1Profile profile;
	AV1RateControl rateControl;
	AV1QP qp;
	bool lowLatency;
	uint32_t bFrames;

	const char *profiler_encode = nullptr;
};

ID3D11Device *d3D11Device_AV1 = NULL;
ID3D11DeviceContext *d3D11Ctx_AV1 = NULL;
ID3D11Texture2D *surface_AV1 = NULL;

static const char *kLabelLowLatency = obs_module_text("MF.AV1.LowLatency");
static const char *kLabelBFrames = obs_module_text("MF.AV1.BFrames");
static const char *kLabelBitrate = obs_module_text("MF.AV1.Bitrate");
static const char *kLabelCustomBufSize = obs_module_text("MF.AV1.CustomBufsize");
static const char *kLabelBufSize = obs_module_text("MF.AV1.BufferSize");
static const char *kLabelUseMaxBitrate = obs_module_text("MF.AV1.CustomMaxBitrate");
static const char *kLabelMaxBitrate = obs_module_text("MF.AV1.MaxBitrate");
static const char *kLabelKeyIntSec = obs_module_text("MF.AV1.KeyframeIntervalSec");
static const char *kLabelRateControl = obs_module_text("MF.AV1.RateControl");
static const char *kLabelQpI = obs_module_text("MF.AV1.QPI");
static const char *kLabelQpP = obs_module_text("MF.AV1.QPP");
static const char *kLabelQpB = obs_module_text("MF.AV1.QPB");
static const char *kLabelProfile = obs_module_text("MF.AV1.Profile");
static const char *kLabelCbr = obs_module_text("MF.AV1.CBR");
static const char *kLabelVbr = obs_module_text("MF.AV1.VBR");
static const char *kLabelCqp = obs_module_text("MF.AV1.CQP");

constexpr std::string_view kMfpUseLowLatency{"mf_av1_use_low_latency"};
constexpr std::string_view kMfpBFrames{"mf_av1_b_frames"};
constexpr std::string_view kMfpBitrate{"mf_av1_bitrate"};
constexpr std::string_view kMfpUseBufSize{"mf_av1_use_buf_size"};
constexpr std::string_view kMfpBufSize{"mf_av1_buf_size"};
constexpr std::string_view kMfpUseMaxBitrate{"mf_av1_use_max_bitrate"};
constexpr std::string_view kMfpMaxBitrate{"mf_av1_max_bitrate"};
constexpr std::string_view kMfpKeyInt{"mf_av1_key_int"};
constexpr std::string_view kMfpRateControl{"mf_av1_rate_control"};
constexpr std::string_view kMfpMinQp{"mf_av1_min_qp"};
constexpr std::string_view kMfpMaxQp{"mf_av1_max_qp"};
constexpr std::string_view kMfpQpI{"mf_av1_qp_i"};
constexpr std::string_view kMfpQpP{"mf_av1_qp_p"};
constexpr std::string_view kMfpQpB{"mf_av1_qp_b"};
constexpr std::string_view kMfpProfile{"mf_av1_profile"};

struct TypeData {
	std::shared_ptr<EncoderDescriptor> descriptor;

	TypeData(std::shared_ptr<EncoderDescriptor> descriptor_) : descriptor(descriptor_) {}
};

namespace {
const char *MFAV1_GetName(void *type_data)
{
	TypeData &typeData = *static_cast<TypeData *>(type_data);
	return obs_module_text(typeData.descriptor->Name());
}

void set_visible(obs_properties_t *ppts, const char *name, bool visible)
{
	obs_property_t *p = obs_properties_get(ppts, name);
	obs_property_set_visible(p, visible);
}

bool use_bufsize_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	bool use_bufsize = obs_data_get_bool(settings, kMfpUseBufSize.data());

	set_visible(ppts, kMfpBufSize.data(), use_bufsize);

	return true;
}

bool use_max_bitrate_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	bool use_max_bitrate = obs_data_get_bool(settings, kMfpUseMaxBitrate.data());

	return true;
}

bool use_advanced_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	AV1RateControl rateControl = (AV1RateControl)obs_data_get_int(settings, kMfpRateControl.data());

	if (rateControl == AV1RateControlCBR || rateControl == AV1RateControlVBR) {
		use_max_bitrate_modified(ppts, NULL, settings);
	}

	return true;
}

bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	AV1RateControl rateControl = (AV1RateControl)obs_data_get_int(settings, kMfpRateControl.data());

	set_visible(ppts, kMfpBitrate.data(), false);
	set_visible(ppts, kMfpUseBufSize.data(), false);
	set_visible(ppts, kMfpBufSize.data(), false);
	set_visible(ppts, kMfpUseMaxBitrate.data(), false);
	set_visible(ppts, kMfpMaxBitrate.data(), false);
	set_visible(ppts, kMfpQpI.data(), false);
	set_visible(ppts, kMfpQpP.data(), false);
	set_visible(ppts, kMfpQpB.data(), false);

	switch (rateControl) {
	case AV1RateControlCBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, kMfpBitrate.data(), true);
		set_visible(ppts, kMfpUseBufSize.data(), true);

		break;
	case AV1RateControlVBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, kMfpBitrate.data(), true);
		set_visible(ppts, kMfpUseBufSize.data(), true);

		break;
	default:
		break;
	}

	return true;
}

obs_properties_t *MFAV1_GetProperties(void *)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	obs_property_t *list = obs_properties_add_list(props, kMfpProfile.data(), kLabelProfile, OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "main", AV1ProfileMain);

	obs_properties_add_int(props, kMfpKeyInt.data(), kLabelKeyIntSec, 0, 20, 1);

	list = obs_properties_add_list(props, kMfpRateControl.data(), kLabelRateControl, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, kLabelCbr, AV1RateControlCBR);
	obs_property_list_add_int(list, kLabelVbr, AV1RateControlVBR);

	obs_property_set_modified_callback(list, rate_control_modified);

	obs_properties_add_int(props, kMfpBitrate.data(), kLabelBitrate, 50, 10000000, 1);

	p = obs_properties_add_bool(props, kMfpUseBufSize.data(), kLabelCustomBufSize);
	obs_property_set_modified_callback(p, use_bufsize_modified);
	obs_properties_add_int(props, kMfpBufSize.data(), kLabelBufSize, 0, 10000000, 1);

	obs_properties_add_int(props, kMfpQpI.data(), kLabelQpI, 0, 51, 1);
	obs_properties_add_int(props, kMfpQpP.data(), kLabelQpP, 0, 51, 1);
	obs_properties_add_int(props, kMfpQpB.data(), kLabelQpB, 0, 51, 1);

	p = obs_properties_add_bool(props, kMfpUseMaxBitrate.data(), kLabelUseMaxBitrate);
	obs_property_set_modified_callback(p, use_max_bitrate_modified);
	obs_properties_add_int(props, kMfpMaxBitrate.data(), kLabelMaxBitrate, 50, 10000000, 1);

	obs_properties_add_bool(props, kMfpUseLowLatency.data(), kLabelLowLatency);
	obs_properties_add_int(props, kMfpBFrames.data(), kLabelBFrames, 0, 16, 1);

	return props;
}

void MFAV1_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, kMfpBitrate.data(), 6000);
	obs_data_set_default_bool(settings, kMfpUseLowLatency.data(), true);
	obs_data_set_default_int(settings, kMfpBFrames.data(), 2);
	obs_data_set_default_bool(settings, kMfpUseBufSize.data(), false);
	obs_data_set_default_int(settings, kMfpBufSize.data(), 2500);
	obs_data_set_default_bool(settings, kMfpUseMaxBitrate.data(), false);
	obs_data_set_default_int(settings, kMfpMaxBitrate.data(), 6000);
	obs_data_set_default_int(settings, kMfpKeyInt.data(), 2);
	obs_data_set_default_int(settings, kMfpRateControl.data(), AV1RateControlCBR);
	obs_data_set_default_int(settings, kMfpProfile.data(), AV1ProfileMain);
	obs_data_set_default_int(settings, kMfpMinQp.data(), 1);
	obs_data_set_default_int(settings, kMfpMaxQp.data(), 51);
	obs_data_set_default_int(settings, kMfpQpI.data(), 26);
	obs_data_set_default_int(settings, kMfpQpB.data(), 26);
	obs_data_set_default_int(settings, kMfpQpP.data(), 26);
}

void UpdateParams(MFAV1_Encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	TypeData &typeData = *static_cast<TypeData *>(obs_encoder_get_type_data(enc->encoder));

	enc->width = static_cast<uint32_t>(obs_encoder_get_width(enc->encoder));
	enc->height = static_cast<uint32_t>(obs_encoder_get_height(enc->encoder));
	enc->framerateNum = voi->fps_num;
	enc->framerateDen = voi->fps_den;

	enc->descriptor = typeData.descriptor;

	enc->profile = static_cast<AV1Profile>(obs_data_get_int(settings, kMfpProfile.data()));
	enc->rateControl = static_cast<AV1RateControl>(obs_data_get_int(settings, kMfpRateControl.data()));
	enc->keyint = static_cast<uint32_t>(obs_data_get_int(settings, kMfpKeyInt.data()));
	enc->bitrate = static_cast<uint32_t>(obs_data_get_int(settings, kMfpBitrate.data()));
	enc->useBufferSize = obs_data_get_bool(settings, kMfpUseBufSize.data());
	enc->bufferSize = static_cast<uint32_t>(obs_data_get_int(settings, kMfpBufSize.data()));
	enc->useMaxBitrate = obs_data_get_bool(settings, kMfpUseMaxBitrate.data());
	enc->maxBitrate = static_cast<uint32_t>(obs_data_get_int(settings, kMfpMaxBitrate.data()));
	enc->qp.defaultQp = static_cast<uint16_t>(obs_data_get_int(settings, kMfpQpI.data()));
	enc->qp.i = static_cast<uint16_t>(obs_data_get_int(settings, kMfpQpI.data()));
	enc->qp.p = static_cast<uint16_t>(obs_data_get_int(settings, kMfpQpP.data()));
	enc->qp.b = static_cast<uint16_t>(obs_data_get_int(settings, kMfpQpB.data()));
	enc->lowLatency = obs_data_get_bool(settings, kMfpUseLowLatency.data());
	enc->bFrames = static_cast<uint32_t>(obs_data_get_int(settings, kMfpBFrames.data()));
}
} // namespace

namespace {
bool ApplyCBR(MFAV1_Encoder *enc)
{
	enc->av1Encoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate) {
		enc->av1Encoder->SetMaxBitrate(enc->maxBitrate);
	} else {
		enc->av1Encoder->SetMaxBitrate(enc->bitrate);
	}

	if (enc->useBufferSize) {
		enc->av1Encoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

bool ApplyVBR(MFAV1_Encoder *enc)
{
	enc->av1Encoder->SetBitrate(enc->bitrate);

	if (enc->useBufferSize) {
		enc->av1Encoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

void *MFAV1_Create(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFAV1_Create");

	double bppf;
	uint32_t minQp, maxQp;
	float fps;

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	MFT_REGISTER_TYPE_INFO rtinputinfo = {MFMediaType_Video, GetMFVideoFormat(voi->format)};
	MFT_REGISTER_TYPE_INFO rtinfo = {MFMediaType_Video, MFVideoFormat_AV1};
	IMFActivate **ppActivate = NULL;
	UINT32 count = 0;
	HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, &rtinputinfo, &rtinfo, &ppActivate,
			       &count);
	if (ppActivate == NULL || hr != S_OK) {
		obs_encoder_set_last_error(
			encoder, obs_module_text("OBS does not support current color format using QCOM AV1 Encoder"));
		return NULL;
	} else {
		blog(LOG_INFO, "Format is supported");
	}

	std::unique_ptr<MFAV1_Encoder> enc(new MFAV1_Encoder());
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->av1Encoder.reset(new AV1Encoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
					     enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->av1Encoder->SetRateControl(enc->rateControl);
		enc.get()->av1Encoder->SetKeyframeInterval(enc->keyint);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		QpRange(QCOM_AV1, bppf, &minQp, &maxQp);

		enc.get()->av1Encoder->SetLowLatency(enc->lowLatency);
		enc.get()->av1Encoder->SetBFrameCount(enc->bFrames);

		enc.get()->av1Encoder->SetMinQP(minQp);
		enc.get()->av1Encoder->SetMaxQP(maxQp);

		switch (enc->rateControl) {
		case AV1RateControlCBR:
			return ApplyCBR(enc.get());
		case AV1RateControlVBR:
			return ApplyVBR(enc.get());
		default:
			return false;
		}
	};

	if (!enc->av1Encoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void *MFAV1_Create_Tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFAV1_Create");

	D3D11_TEXTURE2D_DESC desc = {};

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	MFT_REGISTER_TYPE_INFO rtInputInfo = {MFMediaType_Video, GetMFVideoFormat(voi->format)};
	MFT_REGISTER_TYPE_INFO rtInfo = {MFMediaType_Video, MFVideoFormat_AV1};
	IMFActivate **activate = NULL;
	UINT32 count = 0;
	ComPtr_Dev<IMFDXGIDeviceManager> deviceManager;
	uint32_t minQp, maxQp;
	double bppf;
	float fps;

	HRESULT hr =
		MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, &rtInputInfo, &rtInfo, &activate, &count);
	if (activate == NULL || hr != S_OK) {
		obs_encoder_set_last_error(
			encoder, obs_module_text("OBS does not support current color format using QCOM AV1 Encoder"));
		return NULL;
	} else {
		blog(LOG_INFO, "Format is supported");
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		if (!obs_encoder_gpu_scaling_enabled(encoder)) {
			blog(LOG_INFO, ">>> encoder CPU scaling active, fall back to old qsv encoder");
			return obs_encoder_create_rerouted(encoder, "qcom_av1");
		}
		blog(LOG_INFO, ">>> encoder GPU scaling active");
	}

	hr = MF::CreateD3D11EncoderResources(voi, &d3D11Device_AV1, &d3D11Ctx_AV1, deviceManager, &surface_AV1);

	std::unique_ptr<MFAV1_Encoder> enc(new MFAV1_Encoder());
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->av1Encoder.reset(new AV1Encoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
					     enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->av1Encoder->SetRateControl(enc->rateControl);
		enc.get()->av1Encoder->SetKeyframeInterval(enc->keyint);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		QpRange(QCOM_AV1, bppf, &minQp, &maxQp);

		enc.get()->av1Encoder->SetLowLatency(enc->lowLatency);
		enc.get()->av1Encoder->SetBFrameCount(enc->bFrames);

		enc.get()->av1Encoder->SetMinQP(minQp);
		enc.get()->av1Encoder->SetMaxQP(maxQp);

		switch (enc->rateControl) {
		case AV1RateControlCBR:
			return ApplyCBR(enc.get());
		case AV1RateControlVBR:
			return ApplyVBR(enc.get());
		default:
			return false;
		}
	};

	if (!enc->av1Encoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void MFAV1_Destroy(void *data)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);
	delete enc;
}

bool MFAV1_Encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);
	Status status;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFAV1_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	if (!enc->av1Encoder->ProcessInput(frame->data, frame->linesize, (frame->pts / packet->timebase_num),
					   &status)) {
		return false;
	}

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->av1Encoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
					    &status)) {
		return false;
	}

	// Needs more input, not a failure case
	if (status == NEED_MORE_INPUT) {
		return true;
	}

	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = outputPts * packet->timebase_num;
	packet->dts = outputDts * packet->timebase_num;
	packet->data = outputData;
	packet->size = outputDataLength;
	packet->keyframe = keyframe;

	*received_packet = true;
	return true;
}

bool MFTAV1_Encode_Tex(void *data, struct encoder_texture *tex, int64_t pts, uint64_t lock_key, uint64_t *next_key,
		       struct encoder_packet *packet, bool *received_packet)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);
	Status status;
	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;
	ID3D11Texture2D *pSurface = NULL;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFAV1_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;
	Copy_Tex(tex, lock_key, next_key, d3D11Device_AV1, d3D11Ctx_AV1, surface_AV1);

	if (!enc->av1Encoder->ProcessInput_Tex(surface_AV1, pts / packet->timebase_num, &status)) {
		return false;
	}

	if (!enc->av1Encoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
					    &status)) {
		return false;
	}

	// Needs more input, not a failure case
	if (status == NEED_MORE_INPUT) {
		return true;
	}

	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = outputPts * packet->timebase_num;
	packet->dts = outputDts * packet->timebase_num;
	packet->data = outputData;
	packet->size = outputDataLength;
	packet->keyframe = keyframe;

	*received_packet = true;
	return true;
}

bool MFAV1_GetExtraData(void *data, uint8_t **extra_data, size_t *size)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);

	uint8_t *extraData;
	UINT32 extraDataLength;

	if (!enc->av1Encoder->ExtraData(&extraData, &extraDataLength)) {
		return false;
	}

	*extra_data = extraData;
	*size = extraDataLength;

	return true;
}

bool MFAV1_GetSEIData(void *data, uint8_t **sei_data, size_t *size)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(sei_data);
	UNUSED_PARAMETER(size);

	return false;
}

void MFAV1_GetVideoInfo(void *data, struct video_scale_info *info)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	info->format = voi->format;
}

bool MFAV1_Update(void *data, obs_data_t *settings)
{
	MFAV1_Encoder *enc = static_cast<MFAV1_Encoder *>(data);

	UpdateParams(enc, settings);

	enc->av1Encoder->SetBitrate(enc->bitrate);
	enc->av1Encoder->SetQP(enc->qp);

	return true;
}

bool CanSpawnEncoder(std::shared_ptr<EncoderDescriptor> descriptor)
{
	HRESULT hr;
	ComPtr<IMFTransform> transform;

	hr = CoCreateInstance(descriptor->Guid(), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&transform));
	return hr == S_OK;
}
} // namespace

void RegisterMFAV1Encoders()
{
	obs_encoder_info info = {0};
	info.type = OBS_ENCODER_VIDEO;
	info.get_name = MFAV1_GetName;
	info.create = MFAV1_Create_Tex;
	info.destroy = MFAV1_Destroy;
	info.encode = MFAV1_Encode;
	info.update = MFAV1_Update;
	info.encode_texture2 = MFTAV1_Encode_Tex;
	info.get_properties = MFAV1_GetProperties;
	info.get_defaults = MFAV1_GetDefaults;
	info.get_extra_data = MFAV1_GetExtraData;
	info.get_sei_data = MFAV1_GetSEIData;
	info.get_video_info = MFAV1_GetVideoInfo;
	info.codec = "av1";
	info.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE;

	auto encoders = EncoderDescriptor::Enumerate("av1");
	for (auto e : encoders) {

		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == EncoderType::H264_SOFTWARE) {
			continue;
		}

		/* certain encoders such as quicksync will be "available" but
		 * not usable with certain processors */
		if (!CanSpawnEncoder(e)) {
			continue;
		}

		info.id = e->Id();
		info.type_data = new TypeData(e);
		info.free_type_data = [](void *type_data) {
			delete static_cast<TypeData *>(type_data);
		};
		obs_register_encoder(&info);
	}

	obs_encoder_info info_soft = {0};
	info_soft.type = OBS_ENCODER_VIDEO;
	info_soft.get_name = MFAV1_GetName;
	info_soft.create = MFAV1_Create;
	info_soft.destroy = MFAV1_Destroy;
	info_soft.encode = MFAV1_Encode;
	info_soft.update = MFAV1_Update;
	info_soft.get_properties = MFAV1_GetProperties;
	info_soft.get_defaults = MFAV1_GetDefaults;
	info_soft.get_extra_data = MFAV1_GetExtraData;
	info_soft.get_sei_data = MFAV1_GetSEIData;
	info_soft.get_video_info = MFAV1_GetVideoInfo;
	info_soft.codec = "av1";
	info_soft.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DEPRECATED;

	for (auto e : encoders) {

		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == EncoderType::H264_SOFTWARE) {
			continue;
		}

		/* certain encoders such as quicksync will be "available" but
		 * not usable with certain processors */
		if (!CanSpawnEncoder(e)) {
			continue;
		}

		info_soft.id = "qcom_av1";
		info_soft.type_data = new TypeData(e);
		info_soft.free_type_data = [](void *type_data) {
			delete static_cast<TypeData *>(type_data);
		};
		obs_register_encoder(&info_soft);
	}
}
