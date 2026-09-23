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

#include "mf-encoder-descriptor.hpp"
#include "mf-h264-encoder.hpp"

#include <obs-module.h>
#include <util/profiler.hpp>

#include <chrono>
#include <memory>
#include <string_view>

#include <VersionHelpers.h>

struct MFH264_Encoder {
	obs_encoder_t *encoder;
	std::shared_ptr<MF::EncoderDescriptor> descriptor;
	std::unique_ptr<MF::H264Encoder> h264Encoder;
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
	MF::H264Profile profile;
	MF::H264RateControl rateControl;
	MF::H264QP qp;
	bool lowLatency;
	uint32_t bFrames;

	const char *profiler_encode = nullptr;
};

ID3D11Device *d3D11Device_H264 = nullptr;
ID3D11DeviceContext *d3D11Ctx_H264 = nullptr;
ID3D11Texture2D *surface_H264 = NULL;

static const char *kLabelLowLatency = obs_module_text("MF.H264.LowLatency");
static const char *kLabelBFrames = obs_module_text("MF.H264.BFrames");
static const char *kLabelBitrate = obs_module_text("MF.H264.Bitrate");
static const char *kLabelCustomBufSize = obs_module_text("MF.H264.CustomBufsize");
static const char *kLabelBufSize = obs_module_text("MF.H264.BufferSize");
static const char *kLabelUseMaxBitrate = obs_module_text("MF.H264.CustomMaxBitrate");
static const char *kLabelMaxBitrate = obs_module_text("MF.H264.MaxBitrate");
static const char *kLabelKeyIntSec = obs_module_text("MF.H264.KeyframeIntervalSec");
static const char *kLabelRateControl = obs_module_text("MF.H264.RateControl");
static const char *kLabelQpI = obs_module_text("MF.H264.QPI");
static const char *kLabelQpP = obs_module_text("MF.H264.QPP");
static const char *kLabelQpB = obs_module_text("MF.H264.QPB");
static const char *kLabelProfile = obs_module_text("MF.H264.Profile");
static const char *kLabelCbr = obs_module_text("MF.H264.CBR");
static const char *kLabelVbr = obs_module_text("MF.H264.VBR");
static const char *kLabelCqp = obs_module_text("MF.H264.CQP");

constexpr std::string_view kMfpUseLowLatency{"mf_h264_use_low_latency"};
constexpr std::string_view kMfpBFrames{"mf_h264_b_frames"};
constexpr std::string_view kMfpBitrate{"mf_h264_bitrate"};
constexpr std::string_view kMfpUseBufSize{"mf_h264_use_buf_size"};
constexpr std::string_view kMfpBufSize{"mf_h264_buf_size"};
constexpr std::string_view kMfpUseMaxBitrate{"mf_h264_use_max_bitrate"};
constexpr std::string_view kMfpMaxBitrate{"mf_h264_max_bitrate"};
constexpr std::string_view kMfpKeyInt{"mf_h264_key_int"};
constexpr std::string_view kMfpRateControl{"mf_h264_rate_control"};
constexpr std::string_view kMfpQpI{"mf_h264_qp_i"};
constexpr std::string_view kMfpQpP{"mf_h264_qp_p"};
constexpr std::string_view kMfpQpB{"mf_h264_qp_b"};
constexpr std::string_view kMfpProfile{"mf_h264_profile"};

struct TypeData {
	std::shared_ptr<MF::EncoderDescriptor> descriptor;
	TypeData(std::shared_ptr<MF::EncoderDescriptor> descriptor_) : descriptor(descriptor_) {}
};

namespace {
const char *MFH264_GetName(void *type_data)
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

	set_visible(ppts, kMfpMaxBitrate.data(), use_max_bitrate);

	return true;
}

bool use_advanced_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	MF::H264RateControl rateControl = (MF::H264RateControl)obs_data_get_int(settings, kMfpRateControl.data());

	if (rateControl == MF::H264RateControlCBR || rateControl == MF::H264RateControlVBR) {
		use_max_bitrate_modified(ppts, NULL, settings);
	}

	return true;
}

bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	MF::H264RateControl rateControl = (MF::H264RateControl)obs_data_get_int(settings, kMfpRateControl.data());

	set_visible(ppts, kMfpBitrate.data(), false);
	set_visible(ppts, kMfpUseBufSize.data(), false);
	set_visible(ppts, kMfpBufSize.data(), false);
	set_visible(ppts, kMfpUseMaxBitrate.data(), false);
	set_visible(ppts, kMfpMaxBitrate.data(), false);
	set_visible(ppts, kMfpQpI.data(), false);
	set_visible(ppts, kMfpQpP.data(), false);
	set_visible(ppts, kMfpQpB.data(), false);

	switch (rateControl) {
	case MF::H264RateControlCBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, kMfpBitrate.data(), true);
		set_visible(ppts, kMfpUseBufSize.data(), true);

		break;
	case MF::H264RateControlVBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, kMfpBitrate.data(), true);
		set_visible(ppts, kMfpUseBufSize.data(), true);

		break;
	case MF::H264RateControlCQP:
		set_visible(ppts, kMfpQpI.data(), true);
		set_visible(ppts, kMfpQpP.data(), true);
		set_visible(ppts, kMfpQpB.data(), true);

		break;
	default:
		break;
	}

	return true;
}

obs_properties_t *MFH264_GetProperties(void *)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	obs_property_t *list = obs_properties_add_list(props, kMfpProfile.data(), kLabelProfile, OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "baseline", MF::H264ProfileBaseline);
	obs_property_list_add_int(list, "main", MF::H264ProfileMain);
	obs_property_list_add_int(list, "high", MF::H264ProfileHigh);

	obs_properties_add_int(props, kMfpKeyInt.data(), kLabelKeyIntSec, 0, 20, 1);

	list = obs_properties_add_list(props, kMfpRateControl.data(), kLabelRateControl, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, kLabelCbr, MF::H264RateControlCBR);
	obs_property_list_add_int(list, kLabelVbr, MF::H264RateControlVBR);
	obs_property_list_add_int(list, kLabelCqp, MF::H264RateControlCQP);

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

void MFH264_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, kMfpBitrate.data(), 6000);
	obs_data_set_default_bool(settings, kMfpUseLowLatency.data(), true);
	obs_data_set_default_int(settings, kMfpBFrames.data(), 2);
	obs_data_set_default_bool(settings, kMfpUseBufSize.data(), false);
	obs_data_set_default_int(settings, kMfpBufSize.data(), 2500);
	obs_data_set_default_bool(settings, kMfpUseMaxBitrate.data(), false);
	obs_data_set_default_int(settings, kMfpMaxBitrate.data(), 6000);
	obs_data_set_default_int(settings, kMfpKeyInt.data(), 2);
	obs_data_set_default_int(settings, kMfpRateControl.data(), MF::H264RateControlCBR);
	obs_data_set_default_int(settings, kMfpProfile.data(), MF::H264ProfileMain);
	obs_data_set_default_int(settings, kMfpQpI.data(), 26);
	obs_data_set_default_int(settings, kMfpQpB.data(), 26);
	obs_data_set_default_int(settings, kMfpQpP.data(), 26);
}

void UpdateParams(MFH264_Encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	TypeData &typeData = *static_cast<TypeData *>(obs_encoder_get_type_data(enc->encoder));

	enc->width = static_cast<uint32_t>(obs_encoder_get_width(enc->encoder));
	enc->height = static_cast<uint32_t>(obs_encoder_get_height(enc->encoder));
	enc->framerateNum = voi->fps_num;
	enc->framerateDen = voi->fps_den;

	enc->descriptor = typeData.descriptor;
	enc->profile = static_cast<MF::H264Profile>(obs_data_get_int(settings, kMfpProfile.data()));
	enc->rateControl = static_cast<MF::H264RateControl>(obs_data_get_int(settings, kMfpRateControl.data()));
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
bool ApplyCBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate) {
		enc->h264Encoder->SetMaxBitrate(enc->maxBitrate);
	} else {
		enc->h264Encoder->SetMaxBitrate(enc->bitrate);
	}

	if (enc->useBufferSize) {
		enc->h264Encoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

bool ApplyCVBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate) {
		enc->h264Encoder->SetMaxBitrate(enc->maxBitrate);
	} else {
		enc->h264Encoder->SetMaxBitrate(enc->bitrate);
	}

	if (enc->useBufferSize) {
		enc->h264Encoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

bool ApplyVBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->useBufferSize) {
		enc->h264Encoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

bool ApplyCQP(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetQP(enc->qp);

	return true;
}

void *MFH264_Create(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFH264_Create");

	uint32_t minQp, maxQp;
	double bppf;
	float fps;
	auto enc = std::make_unique<MFH264_Encoder>();
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->h264Encoder.reset(new MF::H264Encoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
						   enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->h264Encoder->SetRateControl(enc->rateControl);
		enc.get()->h264Encoder->SetKeyframeInterval(enc->keyint);

		enc.get()->h264Encoder->SetEntropyEncoding(MF::H264EntropyEncodingCABAC);

		enc.get()->h264Encoder->SetLowLatency(enc->lowLatency);
		enc.get()->h264Encoder->SetBFrameCount(enc->bFrames);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = MF::ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		MF::QpRange(MF::QCOM_H264, bppf, &minQp, &maxQp);

		enc.get()->h264Encoder->SetMinQP(minQp);
		enc.get()->h264Encoder->SetMaxQP(maxQp);

		if (enc->rateControl == MF::H264RateControlVBR && enc->useMaxBitrate) {
			enc->rateControl = MF::H264RateControlConstrainedVBR;
		}

		switch (enc->rateControl) {
		case MF::H264RateControlCBR:
			return ApplyCBR(enc.get());
		case MF::H264RateControlConstrainedVBR:
			return ApplyCVBR(enc.get());
		case MF::H264RateControlVBR:
			return ApplyVBR(enc.get());
		case MF::H264RateControlCQP:
			return ApplyCQP(enc.get());
		default:
			return false;
		}
	};

	if (!enc->h264Encoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void *MFH264_Create_Tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFH264_Create_Tex");

	D3D11_TEXTURE2D_DESC desc = {};
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	MFT_REGISTER_TYPE_INFO rtInputInfo = {MFMediaType_Video, MF::GetMFVideoFormat(voi->format)};
	MFT_REGISTER_TYPE_INFO rtinfo = {MFMediaType_Video, MFVideoFormat_H264};
	IMFActivate **activate = NULL;
	UINT32 count = 0;
	MF::ComPtr_Dev<IMFDXGIDeviceManager> deviceManager;
	uint32_t minQp, maxQp;
	double bppf;
	float fps;

	HRESULT hr =
		MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, &rtInputInfo, &rtinfo, &activate, &count);
	if (activate == NULL || hr != S_OK) {
		obs_encoder_set_last_error(
			encoder, obs_module_text("OBS does not support current color format using QCOM AVC Encoder"));
		return NULL;
	} else {
		blog(LOG_INFO, "Format is supported");
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		if (!obs_encoder_gpu_scaling_enabled(encoder)) {
			blog(LOG_INFO, ">>> encoder CPU scaling active, fall back to old qsv encoder");
			return obs_encoder_create_rerouted(encoder, "qcom_h264");
		}
		blog(LOG_INFO, ">>> encoder GPU scaling active");
	}

	hr = MF::CreateD3D11EncoderResources(voi, &d3D11Device_H264, &d3D11Ctx_H264, deviceManager, &surface_H264);
	std::unique_ptr<MFH264_Encoder> enc(new MFH264_Encoder());
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->h264Encoder.reset(new MF::H264Encoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
						   enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->h264Encoder->SetRateControl(enc->rateControl);
		enc.get()->h264Encoder->SetKeyframeInterval(enc->keyint);

		enc.get()->h264Encoder->SetEntropyEncoding(MF::H264EntropyEncodingCABAC);

		enc.get()->h264Encoder->SetLowLatency(enc->lowLatency);
		enc.get()->h264Encoder->SetBFrameCount(enc->bFrames);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = MF::ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		MF::QpRange(MF::QCOM_H264, bppf, &minQp, &maxQp);

		enc.get()->h264Encoder->SetMinQP(1);
		enc.get()->h264Encoder->SetMaxQP(51);

		switch (enc->rateControl) {
		case MF::H264RateControlCBR:
			return ApplyCBR(enc.get());
		case MF::H264RateControlConstrainedVBR:
			return ApplyCVBR(enc.get());
		case MF::H264RateControlVBR:
			return ApplyVBR(enc.get());
		case MF::H264RateControlCQP:
			return ApplyCQP(enc.get());
		default:
			return false;
		}
	};

	if (!enc->h264Encoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void MFH264_Destroy(void *data)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);
	delete enc;
}

bool MFH264_Encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);
	MF::Status status;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFH264_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	if (!enc->h264Encoder->ProcessInput(frame->data, frame->linesize, (frame->pts / packet->timebase_num),
					    &status)) {
		return false;
	}

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->h264Encoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
					     &status)) {
		return false;
	}

	// Needs more input, not a failure case
	if (status == MF::NEED_MORE_INPUT) {
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

bool MFTH264_Encode_Tex(void *data, struct encoder_texture *tex, int64_t pts, uint64_t lock_key, uint64_t *next_key,
			struct encoder_packet *packet, bool *received_packet)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);
	ID3D11Texture2D *pSurface = NULL;
	MF::Status status;
	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFH264_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	MF::Copy_Tex(tex, lock_key, next_key, d3D11Device_H264, d3D11Ctx_H264, surface_H264);

	if (!enc->h264Encoder->ProcessInput_Tex(surface_H264, pts / packet->timebase_num, &status)) {
		return false;
	}

	if (!enc->h264Encoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
					     &status)) {
		return false;
	}

	// Needs more input, not a failure case
	if (status == MF::NEED_MORE_INPUT) {
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

bool MFH264_GetExtraData(void *data, uint8_t **extra_data, size_t *size)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);

	uint8_t *extraData;
	UINT32 extraDataLength;

	if (!enc->h264Encoder->ExtraData(&extraData, &extraDataLength)) {
		return false;
	}
	*extra_data = extraData;
	*size = extraDataLength;

	return true;
}

bool MFH264_GetSEIData(void *data, uint8_t **sei_data, size_t *size)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(sei_data);
	UNUSED_PARAMETER(size);

	return false;
}

void MFH264_GetVideoInfo(void *, struct video_scale_info *info)
{
	info->format = VIDEO_FORMAT_NV12;
}

bool MFH264_Update(void *data, obs_data_t *settings)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);

	UpdateParams(enc, settings);

	enc->h264Encoder->SetBitrate(enc->bitrate);
	enc->h264Encoder->SetQP(enc->qp);

	return true;
}

bool CanSpawnEncoder(std::shared_ptr<MF::EncoderDescriptor> descriptor)
{
	HRESULT hr;
	ComPtr<IMFTransform> transform;

	hr = CoCreateInstance(descriptor->Guid(), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&transform));
	return hr == S_OK;
}
} // namespace

void RegisterMFH264Encoders(const char Codec[])
{
	obs_encoder_info info = {0};
	info.type = OBS_ENCODER_VIDEO;
	info.get_name = MFH264_GetName;
	info.create = MFH264_Create;
	info.destroy = MFH264_Destroy;
	info.encode = MFH264_Encode;
	info.update = MFH264_Update;
	info.get_properties = MFH264_GetProperties;
	info.get_defaults = MFH264_GetDefaults;
	info.get_extra_data = MFH264_GetExtraData;
	info.get_sei_data = MFH264_GetSEIData;
	info.get_video_info = MFH264_GetVideoInfo;
	info.codec = Codec;

	auto encoders = MF::EncoderDescriptor::Enumerate(Codec);
	for (auto e : encoders) {
		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == MF::EncoderType::H264_SOFTWARE) {
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
}

void RegisterMFH264Encoders()
{

	obs_encoder_info info = {0};
	info.type = OBS_ENCODER_VIDEO;
	info.get_name = MFH264_GetName;
	info.create = MFH264_Create_Tex;
	info.destroy = MFH264_Destroy;
	info.encode = MFH264_Encode;
	info.update = MFH264_Update;
	info.encode_texture2 = MFTH264_Encode_Tex;
	info.get_properties = MFH264_GetProperties;
	info.get_defaults = MFH264_GetDefaults;
	info.get_extra_data = MFH264_GetExtraData;
	info.get_sei_data = MFH264_GetSEIData;
	info.get_video_info = MFH264_GetVideoInfo;
	info.codec = "h264";
	info.caps = OBS_ENCODER_CAP_PASS_TEXTURE;

	auto encoders = MF::EncoderDescriptor::Enumerate("h264");
	for (auto e : encoders) {
		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == MF::EncoderType::H264_SOFTWARE) {
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
	info_soft.get_name = MFH264_GetName;
	info_soft.create = MFH264_Create;
	info_soft.destroy = MFH264_Destroy;
	info_soft.encode = MFH264_Encode;
	info_soft.update = MFH264_Update;
	info_soft.get_properties = MFH264_GetProperties;
	info_soft.get_defaults = MFH264_GetDefaults;
	info_soft.get_extra_data = MFH264_GetExtraData;
	info_soft.get_sei_data = MFH264_GetSEIData;
	info_soft.get_video_info = MFH264_GetVideoInfo;
	info_soft.codec = "h264";
	info_soft.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DEPRECATED;

	for (auto e : encoders) {

		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == MF::EncoderType::H264_SOFTWARE) {
			continue;
		}

		/* certain encoders such as quicksync will be "available" but
		 * not usable with certain processors */
		if (!CanSpawnEncoder(e)) {
			continue;
		}

		info_soft.id = "qcom_h264";
		info_soft.type_data = new TypeData(e);
		info_soft.free_type_data = [](void *type_data) {
			delete static_cast<TypeData *>(type_data);
		};
		obs_register_encoder(&info_soft);
	}
}
