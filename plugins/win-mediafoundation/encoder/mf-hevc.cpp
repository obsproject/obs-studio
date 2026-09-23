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
#include "mf-hevc-encoder.hpp"

#include <obs-module.h>
#include <util/profiler.hpp>

#include <chrono>
#include <memory>
#include <string_view>
#include <VersionHelpers.h>

struct MFHEVC_Encoder {
	obs_encoder_t *encoder;
	std::shared_ptr<MF::EncoderDescriptor> descriptor;
	std::unique_ptr<MF::HEVCEncoder> hevcEncoder;
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
	MF::HEVCProfile profile;
	MF::HEVCRateControl rateControl;
	MF::HEVCQP qp;
	bool lowLatency;
	uint32_t bFrames;

	const char *profiler_encode = nullptr;
};

ID3D11Device *d3D11Device_HEVC = nullptr;
ID3D11DeviceContext *d3D11Ctx_HEVC = nullptr;
ID3D11Texture2D *surface_HEVC = NULL;

static const char *kLabelLowLatency = obs_module_text("MF.HEVC.LowLatency");
static const char *kLabelBFrames = obs_module_text("MF.HEVC.BFrames");
static const char *kLabelBitrate = obs_module_text("MF.HEVC.Bitrate");
static const char *kLabelCustomBufSize = obs_module_text("MF.HEVC.CustomBufsize");
static const char *kLabelBufSize = obs_module_text("MF.HEVC.BufferSize");
static const char *kLabelUseMaxBitrate = obs_module_text("MF.HEVC.CustomMaxBitrate");
static const char *kLabelMaxBitrate = obs_module_text("MF.HEVC.MaxBitrate");
static const char *kLabelKeyIntSec = obs_module_text("MF.HEVC.KeyframeIntervalSec");
static const char *kLabelRateControl = obs_module_text("MF.HEVC.RateControl");
static const char *kLabelQpI = obs_module_text("MF.HEVC.QPI");
static const char *kLabelQpP = obs_module_text("MF.HEVC.QPP");
static const char *kLabelQpB = obs_module_text("MF.HEVC.QPB");
static const char *kLabelProfile = obs_module_text("MF.HEVC.Profile");
static const char *kLabelCbr = obs_module_text("MF.HEVC.CBR");
static const char *kLabelVbr = obs_module_text("MF.HEVC.VBR");

constexpr std::string_view kMfpUseLowLatency{"mf_hevc_use_low_latency"};
constexpr std::string_view kMfpBFrames{"mf_hevc_b_frames"};
constexpr std::string_view kMfpBitrate{"mf_hevc_bitrate"};
constexpr std::string_view kMfpUseBufSize{"mf_hevc_use_buf_size"};
constexpr std::string_view kMfpBufSize{"mf_hevc_buf_size"};
constexpr std::string_view kMfpUseMaxBitrate{"mf_hevc_use_max_bitrate"};
constexpr std::string_view kMfpMaxBitrate{"mf_hevc_max_bitrate"};
constexpr std::string_view kMfpKeyInt{"mf_hevc_key_int"};
constexpr std::string_view kMfpRateControl{"mf_hevc_rate_control"};
constexpr std::string_view kMfpMinQp{"mf_hevc_min_qp"};
constexpr std::string_view kMfpMaxQp{"mf_hevc_max_qp"};
constexpr std::string_view kMfpQpI{"mf_hevc_qp_i"};
constexpr std::string_view kMfpQpP{"mf_hevc_qp_p"};
constexpr std::string_view kMfpQpB{"mf_hevc_qp_b"};
constexpr std::string_view kMfpProfile{"mf_hevc_profile"};

struct TypeData {
	std::shared_ptr<MF::EncoderDescriptor> descriptor;

	TypeData(std::shared_ptr<MF::EncoderDescriptor> descriptor_) : descriptor(descriptor_) {}
};

namespace {
const char *MFHEVC_GetName(void *type_data)
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

	MF::HEVCRateControl rateControl = (MF::HEVCRateControl)obs_data_get_int(settings, kMfpRateControl.data());

	if (rateControl == MF::HEVCRateControlCBR || rateControl == MF::HEVCRateControlVBR) {
		use_max_bitrate_modified(ppts, NULL, settings);
	}

	return true;
}

bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	MF::HEVCRateControl rateControl = (MF::HEVCRateControl)obs_data_get_int(settings, kMfpRateControl.data());

	set_visible(ppts, kMfpBitrate.data(), false);
	set_visible(ppts, kMfpUseBufSize.data(), false);
	set_visible(ppts, kMfpBufSize.data(), false);
	set_visible(ppts, kMfpUseMaxBitrate.data(), false);
	set_visible(ppts, kMfpMaxBitrate.data(), false);
	set_visible(ppts, kMfpQpI.data(), false);
	set_visible(ppts, kMfpQpP.data(), false);
	set_visible(ppts, kMfpQpB.data(), false);

	switch (rateControl) {
	case MF::HEVCRateControlCBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, kMfpBitrate.data(), true);
		set_visible(ppts, kMfpUseBufSize.data(), true);

		break;
	case MF::HEVCRateControlVBR:
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

obs_properties_t *MFHEVC_GetProperties(void *)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	obs_property_t *list = obs_properties_add_list(props, kMfpProfile.data(), kLabelProfile, OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "main", MF::HEVCProfileMain);

	obs_properties_add_int(props, kMfpKeyInt.data(), kLabelKeyIntSec, 0, 20, 1);

	list = obs_properties_add_list(props, kMfpRateControl.data(), kLabelRateControl, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, kLabelCbr, MF::HEVCRateControlCBR);
	obs_property_list_add_int(list, kLabelVbr, MF::HEVCRateControlVBR);

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

void MFHEVC_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, kMfpBitrate.data(), 6000);
	obs_data_set_default_bool(settings, kMfpUseLowLatency.data(), true);
	obs_data_set_default_int(settings, kMfpBFrames.data(), 2);
	obs_data_set_default_bool(settings, kMfpUseBufSize.data(), false);
	obs_data_set_default_int(settings, kMfpBufSize.data(), 2500);
	obs_data_set_default_bool(settings, kMfpUseMaxBitrate.data(), false);
	obs_data_set_default_int(settings, kMfpMaxBitrate.data(), 6000);
	obs_data_set_default_int(settings, kMfpKeyInt.data(), 2);
	obs_data_set_default_int(settings, kMfpRateControl.data(), MF::HEVCRateControlCBR);
	obs_data_set_default_int(settings, kMfpProfile.data(), MF::HEVCProfileMain);
	obs_data_set_default_int(settings, kMfpMinQp.data(), 1);
	obs_data_set_default_int(settings, kMfpMaxQp.data(), 51);
	obs_data_set_default_int(settings, kMfpQpI.data(), 26);
	obs_data_set_default_int(settings, kMfpQpB.data(), 26);
	obs_data_set_default_int(settings, kMfpQpP.data(), 26);
}

void UpdateParams(MFHEVC_Encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	TypeData &typeData = *static_cast<TypeData *>(obs_encoder_get_type_data(enc->encoder));

	enc->width = static_cast<uint32_t>(obs_encoder_get_width(enc->encoder));
	enc->height = static_cast<uint32_t>(obs_encoder_get_height(enc->encoder));
	enc->framerateNum = voi->fps_num;
	enc->framerateDen = voi->fps_den;

	enc->descriptor = typeData.descriptor;
	enc->profile = static_cast<MF::HEVCProfile>(obs_data_get_int(settings, kMfpProfile.data()));
	enc->rateControl = static_cast<MF::HEVCRateControl>(obs_data_get_int(settings, kMfpRateControl.data()));
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

//#undef MFTEXT
//#undef MFP

namespace {
bool ApplyCBR(MFHEVC_Encoder *enc)
{
	enc->hevcEncoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate) {
		enc->hevcEncoder->SetMaxBitrate(enc->maxBitrate);
	} else {
		enc->hevcEncoder->SetMaxBitrate(enc->bitrate);
	}

	if (enc->useBufferSize) {
		enc->hevcEncoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

bool ApplyVBR(MFHEVC_Encoder *enc)
{
	enc->hevcEncoder->SetBitrate(enc->bitrate);

	if (enc->useBufferSize) {
		enc->hevcEncoder->SetBufferSize(enc->bufferSize);
	}

	return true;
}

void *MFHEVC_Create(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFHEVC_Create");
	uint32_t minQp, maxQp;
	double bppf;
	float fps;
	auto enc = std::make_unique<MFHEVC_Encoder>();
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->hevcEncoder.reset(new MF::HEVCEncoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
						   enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->hevcEncoder->SetRateControl(enc->rateControl);
		enc.get()->hevcEncoder->SetKeyframeInterval(enc->keyint);

		enc.get()->hevcEncoder->SetLowLatency(enc->lowLatency);
		enc.get()->hevcEncoder->SetBFrameCount(enc->bFrames);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = MF::ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		MF::QpRange(MF::QCOM_H264, bppf, &minQp, &maxQp);

		enc.get()->hevcEncoder->SetMinQP(minQp);
		enc.get()->hevcEncoder->SetMaxQP(maxQp);

		switch (enc->rateControl) {
		case MF::HEVCRateControlCBR:
			return ApplyCBR(enc.get());
		case MF::HEVCRateControlVBR:
			return ApplyVBR(enc.get());
		default:
			return false;
		}
	};

	if (!enc->hevcEncoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void *MFHEVC_Create_Tex(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFHEVC_Create");

	D3D11_TEXTURE2D_DESC desc = {};
	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	MFT_REGISTER_TYPE_INFO rtInputInfo = {MFMediaType_Video, MF::GetMFVideoFormat(voi->format)};
	MFT_REGISTER_TYPE_INFO rtInfo = {MFMediaType_Video, MFVideoFormat_HEVC};
	IMFActivate **activate = NULL;
	UINT32 count = 0;
	MF::ComPtr_Dev<IMFDXGIDeviceManager> deviceManager;
	uint32_t minQp, maxQp;
	double bppf;
	float fps;

	HRESULT hr =
		MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_HARDWARE, &rtInputInfo, &rtInfo, &activate, &count);
	if (activate == NULL || hr != S_OK) {
		obs_encoder_set_last_error(
			encoder, obs_module_text("OBS does not support current color format using QCOM HEVC Encoder"));
		return NULL;
	} else {
		blog(LOG_INFO, "Format is supported");
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		if (!obs_encoder_gpu_scaling_enabled(encoder)) {
			blog(LOG_INFO, ">>> encoder CPU scaling active, fall back to old qsv encoder");
			return obs_encoder_create_rerouted(encoder, "qcom_hevc");
		}
		blog(LOG_INFO, ">>> encoder GPU scaling active");
	}

	hr = MF::CreateD3D11EncoderResources(voi, &d3D11Device_HEVC, &d3D11Ctx_HEVC, deviceManager, &surface_HEVC);

	std::unique_ptr<MFHEVC_Encoder> enc(new MFHEVC_Encoder());
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->hevcEncoder.reset(new MF::HEVCEncoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
						   enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->hevcEncoder->SetRateControl(enc->rateControl);
		enc.get()->hevcEncoder->SetKeyframeInterval(enc->keyint);

		enc.get()->hevcEncoder->SetLowLatency(enc->lowLatency);
		enc.get()->hevcEncoder->SetBFrameCount(enc->bFrames);

		fps = static_cast<float>(enc->framerateNum) / static_cast<float>(enc->framerateDen);
		bppf = MF::ComputeBppf((enc->bitrate * 1000), enc->width, enc->height, fps);
		MF::QpRange(MF::QCOM_H264, bppf, &minQp, &maxQp);

		enc.get()->hevcEncoder->SetMinQP(minQp);
		enc.get()->hevcEncoder->SetMaxQP(maxQp);

		switch (enc->rateControl) {
		case MF::HEVCRateControlCBR:
			return ApplyCBR(enc.get());
		case MF::HEVCRateControlVBR:
			return ApplyVBR(enc.get());
		default:
			return false;
		}
	};

	if (!enc->hevcEncoder->Initialize(applySettings)) {
		return nullptr;
	}

	return enc.release();
}

void MFHEVC_Destroy(void *data)
{
	MFHEVC_Encoder *enc = static_cast<MFHEVC_Encoder *>(data);
	delete enc;
}

bool MFHEVC_Encode(void *data, struct encoder_frame *frame, struct encoder_packet *packet, bool *received_packet)
{
	MFHEVC_Encoder *enc = static_cast<MFHEVC_Encoder *>(data);
	MF::Status status;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFHEVC_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	if (!enc->hevcEncoder->ProcessInput(frame->data, frame->linesize, (frame->pts / packet->timebase_num),
					    &status)) {
		return false;
	}

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->hevcEncoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
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

bool MFTHEVC_Encode_Tex(void *data, struct encoder_texture *tex, int64_t pts, uint64_t lock_key, uint64_t *next_key,
			struct encoder_packet *packet, bool *received_packet)
{
	MFHEVC_Encoder *enc = static_cast<MFHEVC_Encoder *>(data);
	MF::Status status;
	ID3D11Texture2D *pSurface = NULL;

	if (!enc->profiler_encode) {
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFHEVC_Encode(%s)", enc->descriptor->Name());
	}

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	MF::Copy_Tex(tex, lock_key, next_key, d3D11Device_HEVC, d3D11Ctx_HEVC, surface_HEVC);
	if (!enc->hevcEncoder->ProcessInput_Tex(surface_HEVC, pts / packet->timebase_num, &status)) {
		return false;
	}

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->hevcEncoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
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

bool MFHEVC_GetExtraData(void *data, uint8_t **extra_data, size_t *size)
{
	MFHEVC_Encoder *enc = static_cast<MFHEVC_Encoder *>(data);

	uint8_t *extraData;
	UINT32 extraDataLength;

	if (!enc->hevcEncoder->ExtraData(&extraData, &extraDataLength)) {
		return false;
	}

	*extra_data = extraData;
	*size = extraDataLength;

	return true;
}

bool MFHEVC_GetSEIData(void *data, uint8_t **sei_data, size_t *size)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(sei_data);
	UNUSED_PARAMETER(size);

	return false;
}

void MFHEVC_GetVideoInfo(void *, struct video_scale_info *info)
{
	info->format = VIDEO_FORMAT_NV12;
}

bool MFHEVC_Update(void *data, obs_data_t *settings)
{
	MFHEVC_Encoder *enc = static_cast<MFHEVC_Encoder *>(data);

	UpdateParams(enc, settings);

	enc->hevcEncoder->SetBitrate(enc->bitrate);
	enc->hevcEncoder->SetQP(enc->qp);

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

void RegisterMFHEVCEncoders()
{
	obs_encoder_info info = {0};
	info.type = OBS_ENCODER_VIDEO;
	info.get_name = MFHEVC_GetName;
	info.create = MFHEVC_Create_Tex;
	info.destroy = MFHEVC_Destroy;
	info.encode = MFHEVC_Encode;
	info.update = MFHEVC_Update;
	info.encode_texture2 = MFTHEVC_Encode_Tex;
	info.get_properties = MFHEVC_GetProperties;
	info.get_defaults = MFHEVC_GetDefaults;
	info.get_extra_data = MFHEVC_GetExtraData;
	info.get_sei_data = MFHEVC_GetSEIData;
	info.get_video_info = MFHEVC_GetVideoInfo;
	info.codec = "hevc";
	info.caps = OBS_ENCODER_CAP_PASS_TEXTURE;

	auto encoders = MF::EncoderDescriptor::Enumerate("hevc");
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
	info_soft.get_name = MFHEVC_GetName;
	info_soft.create = MFHEVC_Create;
	info_soft.destroy = MFHEVC_Destroy;
	info_soft.encode = MFHEVC_Encode;
	info_soft.update = MFHEVC_Update;
	info_soft.get_properties = MFHEVC_GetProperties;
	info_soft.get_defaults = MFHEVC_GetDefaults;
	info_soft.get_extra_data = MFHEVC_GetExtraData;
	info_soft.get_sei_data = MFHEVC_GetSEIData;
	info_soft.get_video_info = MFHEVC_GetVideoInfo;
	info_soft.codec = "hevc";
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

		info_soft.id = "qcom_hevc";
		info_soft.type_data = new TypeData(e);
		info_soft.free_type_data = [](void *type_data) {
			delete static_cast<TypeData *>(type_data);
		};
		obs_register_encoder(&info_soft);
	}
}
