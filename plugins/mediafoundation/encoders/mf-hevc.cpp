#include <obs-module.h>
#include <util/profiler.hpp>

#include <memory>
#include <chrono>

#include "mf-hevc-encoder.hpp"
#include "mf-encoder-descriptor.hpp"
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
	bool advanced;
	uint32_t bitrate;
	uint32_t maxBitrate;
	bool useMaxBitrate;
	uint32_t bufferSize;
	bool useBufferSize;
	MF::HEVCProfile profile;
	MF::HEVCRateControl rateControl;
	MF::HEVCQP qp;
	uint32_t minQp;
	uint32_t maxQp;
	bool lowLatency;
	uint32_t bFrames;

	const char *profiler_encode = nullptr;
};

static const char *TEXT_ADVANCED = obs_module_text("MF.HEVC.Advanced");
static const char *TEXT_LOW_LAT = obs_module_text("MF.HEVC.LowLatency");
static const char *TEXT_B_FRAMES = obs_module_text("MF.HEVC.BFrames");
static const char *TEXT_BITRATE = obs_module_text("MF.HEVC.Bitrate");
static const char *TEXT_CUSTOM_BUF = obs_module_text("MF.HEVC.CustomBufsize");
static const char *TEXT_BUF_SIZE = obs_module_text("MF.HEVC.BufferSize");
static const char *TEXT_USE_MAX_BITRATE = obs_module_text("MF.HEVC.CustomMaxBitrate");
static const char *TEXT_MAX_BITRATE = obs_module_text("MF.HEVC.MaxBitrate");
static const char *TEXT_KEYINT_SEC = obs_module_text("MF.HEVC.KeyframeIntervalSec");
static const char *TEXT_RATE_CONTROL = obs_module_text("MF.HEVC.RateControl");
static const char *TEXT_MIN_QP = obs_module_text("MF.HEVC.MinQP");
static const char *TEXT_MAX_QP = obs_module_text("MF.HEVC.MaxQP");
static const char *TEXT_QPI = obs_module_text("MF.HEVC.QPI");
static const char *TEXT_QPP = obs_module_text("MF.HEVC.QPP");
static const char *TEXT_QPB = obs_module_text("MF.HEVC.QPB");
static const char *TEXT_PROFILE = obs_module_text("MF.HEVC.Profile");
static const char *TEXT_CBR = obs_module_text("MF.HEVC.CBR");
static const char *TEXT_VBR = obs_module_text("MF.HEVC.VBR");

constexpr const char *MFP_USE_ADVANCED = "mf_hevc_use_advanced";
constexpr const char *MFP_USE_LOWLAT = "mf_hevc_use_low_latency";
constexpr const char *MFP_B_FRAMES = "mf_hevc_b_frames";
constexpr const char *MFP_BITRATE = "mf_hevc_bitrate";
constexpr const char *MFP_USE_BUF_SIZE = "mf_hevc_use_buf_size";
constexpr const char *MFP_BUF_SIZE = "mf_hevc_buf_size";
constexpr const char *MFP_USE_MAX_BITRATE = "mf_hevc_use_max_bitrate";
constexpr const char *MFP_MAX_BITRATE = "mf_hevc_max_bitrate";
constexpr const char *MFP_KEY_INT = "mf_hevc_key_int";
constexpr const char *MFP_RATE_CONTROL = "mf_hevc_rate_control";
constexpr const char *MFP_MIN_QP = "mf_hevc_min_qp";
constexpr const char *MFP_MAX_QP = "mf_hevc_max_qp";
constexpr const char *MFP_QP_I = "mf_hevc_qp_i";
constexpr const char *MFP_QP_P = "mf_hevc_qp_p";
constexpr const char *MFP_QP_B = "mf_hevc_qp_b";
constexpr const char *MFP_PROFILE = "mf_hevc_profile";

struct TypeData {
	std::shared_ptr<MF::EncoderDescriptor> descriptor;

	inline TypeData(std::shared_ptr<MF::EncoderDescriptor> descriptor_) : descriptor(descriptor_) {}
};

namespace {
const char *MFHEVC_GetName(void *type_data)
{
	TypeData &typeData = *reinterpret_cast<TypeData *>(type_data);
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

	bool use_bufsize = obs_data_get_bool(settings, MFP_USE_BUF_SIZE);

	set_visible(ppts, MFP_BUF_SIZE, use_bufsize);

	return true;
}

bool use_max_bitrate_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	bool advanced = obs_data_get_bool(settings, MFP_USE_ADVANCED);
	bool use_max_bitrate = obs_data_get_bool(settings, MFP_USE_MAX_BITRATE);

	set_visible(ppts, MFP_MAX_BITRATE, advanced && use_max_bitrate);

	return true;
}

bool use_advanced_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	bool advanced = obs_data_get_bool(settings, MFP_USE_ADVANCED);

	set_visible(ppts, MFP_MIN_QP, advanced);
	set_visible(ppts, MFP_MAX_QP, advanced);
	set_visible(ppts, MFP_USE_LOWLAT, advanced);
	set_visible(ppts, MFP_B_FRAMES, advanced);

	MF::HEVCRateControl rateControl = (MF::HEVCRateControl)obs_data_get_int(settings, MFP_RATE_CONTROL);

	if (rateControl == MF::HEVCRateControlCBR || rateControl == MF::HEVCRateControlVBR) {
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);
		use_max_bitrate_modified(ppts, NULL, settings);
	}

	return true;
}

bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	MF::HEVCRateControl rateControl = (MF::HEVCRateControl)obs_data_get_int(settings, MFP_RATE_CONTROL);

	bool advanced = obs_data_get_bool(settings, MFP_USE_ADVANCED);

	set_visible(ppts, MFP_BITRATE, false);
	set_visible(ppts, MFP_USE_BUF_SIZE, false);
	set_visible(ppts, MFP_BUF_SIZE, false);
	set_visible(ppts, MFP_USE_MAX_BITRATE, false);
	set_visible(ppts, MFP_MAX_BITRATE, false);
	set_visible(ppts, MFP_QP_I, false);
	set_visible(ppts, MFP_QP_P, false);
	set_visible(ppts, MFP_QP_B, false);

	switch (rateControl) {
	case MF::HEVCRateControlCBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, MFP_BITRATE, true);
		set_visible(ppts, MFP_USE_BUF_SIZE, true);
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);

		break;
	case MF::HEVCRateControlVBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, MFP_BITRATE, true);
		set_visible(ppts, MFP_USE_BUF_SIZE, true);
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);

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

	obs_property_t *list =
		obs_properties_add_list(props, MFP_PROFILE, TEXT_PROFILE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "main", MF::HEVCProfileMain);

	obs_properties_add_int(props, MFP_KEY_INT, TEXT_KEYINT_SEC, 0, 20, 1);

	list = obs_properties_add_list(props, MFP_RATE_CONTROL, TEXT_RATE_CONTROL, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, TEXT_CBR, MF::HEVCRateControlCBR);
	obs_property_list_add_int(list, TEXT_VBR, MF::HEVCRateControlVBR);

	obs_property_set_modified_callback(list, rate_control_modified);

	obs_properties_add_int(props, MFP_BITRATE, TEXT_BITRATE, 50, 10000000, 1);

	p = obs_properties_add_bool(props, MFP_USE_BUF_SIZE, TEXT_CUSTOM_BUF);
	obs_property_set_modified_callback(p, use_bufsize_modified);
	obs_properties_add_int(props, MFP_BUF_SIZE, TEXT_BUF_SIZE, 0, 10000000, 1);

	obs_properties_add_int(props, MFP_QP_I, TEXT_QPI, 0, 51, 1);
	obs_properties_add_int(props, MFP_QP_P, TEXT_QPP, 0, 51, 1);
	obs_properties_add_int(props, MFP_QP_B, TEXT_QPB, 0, 51, 1);

	p = obs_properties_add_bool(props, MFP_USE_ADVANCED, TEXT_ADVANCED);
	obs_property_set_modified_callback(p, use_advanced_modified);

	p = obs_properties_add_bool(props, MFP_USE_MAX_BITRATE, TEXT_USE_MAX_BITRATE);
	obs_property_set_modified_callback(p, use_max_bitrate_modified);
	obs_properties_add_int(props, MFP_MAX_BITRATE, TEXT_MAX_BITRATE, 50, 10000000, 1);

	obs_properties_add_bool(props, MFP_USE_LOWLAT, TEXT_LOW_LAT);
	obs_properties_add_int(props, MFP_B_FRAMES, TEXT_B_FRAMES, 0, 16, 1);
	obs_properties_add_int(props, MFP_MIN_QP, TEXT_MIN_QP, 1, 51, 1);
	obs_properties_add_int(props, MFP_MAX_QP, TEXT_MAX_QP, 1, 51, 1);
	return props;
}

void MFHEVC_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, MFP_BITRATE, 2500);
	obs_data_set_default_bool(settings, MFP_USE_LOWLAT, true);
	obs_data_set_default_int(settings, MFP_B_FRAMES, 2);
	obs_data_set_default_bool(settings, MFP_USE_BUF_SIZE, false);
	obs_data_set_default_int(settings, MFP_BUF_SIZE, 2500);
	obs_data_set_default_bool(settings, MFP_USE_MAX_BITRATE, false);
	obs_data_set_default_int(settings, MFP_MAX_BITRATE, 2500);
	obs_data_set_default_int(settings, MFP_KEY_INT, 2);
	obs_data_set_default_int(settings, MFP_RATE_CONTROL, MF::HEVCRateControlCBR);
	obs_data_set_default_int(settings, MFP_PROFILE, MF::HEVCProfileMain);
	obs_data_set_default_int(settings, MFP_MIN_QP, 1);
	obs_data_set_default_int(settings, MFP_MAX_QP, 51);
	obs_data_set_default_int(settings, MFP_QP_I, 26);
	obs_data_set_default_int(settings, MFP_QP_B, 26);
	obs_data_set_default_int(settings, MFP_QP_P, 26);
	obs_data_set_default_bool(settings, MFP_USE_ADVANCED, false);
}

void UpdateParams(MFHEVC_Encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	TypeData &typeData = *reinterpret_cast<TypeData *>(obs_encoder_get_type_data(enc->encoder));

	enc->width = (uint32_t)obs_encoder_get_width(enc->encoder);
	enc->height = (uint32_t)obs_encoder_get_height(enc->encoder);
	enc->framerateNum = voi->fps_num;
	enc->framerateDen = voi->fps_den;

	enc->descriptor = typeData.descriptor;
	enc->profile = static_cast<MF::HEVCProfile>(obs_data_get_int(settings, MFP_PROFILE));
	enc->rateControl = static_cast<MF::HEVCRateControl>(obs_data_get_int(settings, MFP_RATE_CONTROL));
	enc->keyint = static_cast<uint32_t>(obs_data_get_int(settings, MFP_KEY_INT));
	enc->bitrate = static_cast<uint32_t>(obs_data_get_int(settings, MFP_BITRATE));
	enc->useBufferSize = obs_data_get_bool(settings, MFP_USE_BUF_SIZE);
	enc->bufferSize = static_cast<uint32_t>(obs_data_get_int(settings, MFP_BUF_SIZE));
	enc->useMaxBitrate = obs_data_get_bool(settings, MFP_USE_MAX_BITRATE);
	enc->maxBitrate = static_cast<uint32_t>(obs_data_get_int(settings, MFP_MAX_BITRATE));
	enc->minQp = static_cast<uint16_t>(obs_data_get_int(settings, MFP_MIN_QP));
	enc->maxQp = static_cast<uint16_t>(obs_data_get_int(settings, MFP_MAX_QP));
	enc->qp.defaultQp = static_cast<uint16_t>(obs_data_get_int(settings, MFP_QP_I));
	enc->qp.i = static_cast<uint16_t>(obs_data_get_int(settings, MFP_QP_I));
	enc->qp.p = static_cast<uint16_t>(obs_data_get_int(settings, MFP_QP_P));
	enc->qp.b = static_cast<uint16_t>(obs_data_get_int(settings, MFP_QP_B));
	enc->lowLatency = obs_data_get_bool(settings, MFP_USE_LOWLAT);
	enc->bFrames = static_cast<uint32_t>(obs_data_get_int(settings, MFP_B_FRAMES));
	enc->advanced = obs_data_get_bool(settings, MFP_USE_ADVANCED);
}

} // namespace

//#undef MFTEXT
//#undef MFP

namespace {
bool ApplyCBR(MFHEVC_Encoder *enc)
{
	enc->hevcEncoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate)
		enc->hevcEncoder->SetMaxBitrate(enc->maxBitrate);
	else
		enc->hevcEncoder->SetMaxBitrate(enc->bitrate);

	if (enc->useBufferSize)
		enc->hevcEncoder->SetBufferSize(enc->bufferSize);

	return true;
}

bool ApplyVBR(MFHEVC_Encoder *enc)
{
	enc->hevcEncoder->SetBitrate(enc->bitrate);

	if (enc->useBufferSize)
		enc->hevcEncoder->SetBufferSize(enc->bufferSize);

	return true;
}

void *MFHEVC_Create(obs_data_t *settings, obs_encoder_t *encoder)
{
	ProfileScope("MFHEVC_Create");

	auto enc = std::make_unique<MFHEVC_Encoder>();
	enc->encoder = encoder;

	UpdateParams(enc.get(), settings);

	ProfileScope(enc->descriptor->Name());

	enc->hevcEncoder.reset(new MF::HEVCEncoder(encoder, enc->descriptor, enc->width, enc->height, enc->framerateNum,
						   enc->framerateDen, enc->profile, enc->bitrate));

	auto applySettings = [&]() {
		enc.get()->hevcEncoder->SetRateControl(enc->rateControl);
		enc.get()->hevcEncoder->SetKeyframeInterval(enc->keyint);

		if (enc->advanced) {
			enc.get()->hevcEncoder->SetLowLatency(enc->lowLatency);
			enc.get()->hevcEncoder->SetBFrameCount(enc->bFrames);

			enc.get()->hevcEncoder->SetMinQP(enc->minQp);
			enc.get()->hevcEncoder->SetMaxQP(enc->maxQp);
		}

		switch (enc->rateControl) {
		case MF::HEVCRateControlCBR:
			return ApplyCBR(enc.get());
		case MF::HEVCRateControlVBR:
			return ApplyVBR(enc.get());
		default:
			return false;
		}
	};

	if (!enc->hevcEncoder->Initialize(applySettings))
		return nullptr;

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

	if (!enc->profiler_encode)
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFHEVC_Encode(%s)", enc->descriptor->Name());

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	if (!enc->hevcEncoder->ProcessInput(frame->data, frame->linesize, (frame->pts / packet->timebase_num), &status))
		return false;

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->hevcEncoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
					     &status))
		return false;

	// Needs more input, not a failure case
	if (status == MF::NEED_MORE_INPUT)
		return true;

	packet->type = OBS_ENCODER_VIDEO;
	packet->pts = outputPts * packet->timebase_num;
	packet->dts = outputPts * packet->timebase_num;
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

	if (!enc->hevcEncoder->ExtraData(&extraData, &extraDataLength))
		return false;

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
	info.create = MFHEVC_Create;
	info.destroy = MFHEVC_Destroy;
	info.encode = MFHEVC_Encode;
	info.update = MFHEVC_Update;
	info.get_properties = MFHEVC_GetProperties;
	info.get_defaults = MFHEVC_GetDefaults;
	info.get_extra_data = MFHEVC_GetExtraData;
	info.get_sei_data = MFHEVC_GetSEIData;
	info.get_video_info = MFHEVC_GetVideoInfo;
	info.codec = "hevc";

	auto encoders = MF::EncoderDescriptor::Enumerate("hevc");
	for (auto e : encoders) {

		/* ignore the software encoder due to the fact that we already
		 * have an objectively superior software encoder available */
		if (e->Type() == MF::EncoderType::H264_SOFTWARE)
			continue;

		/* certain encoders such as quicksync will be "available" but
		 * not usable with certain processors */
		if (!CanSpawnEncoder(e))
			continue;

		info.id = e->Id();
		info.type_data = new TypeData(e);
		info.free_type_data = [](void *type_data) {
			delete reinterpret_cast<TypeData *>(type_data);
		};
		obs_register_encoder(&info);
	}
}
