#include <obs-module.h>
#include <util/profiler.hpp>

#include <memory>
#include <chrono>

#include "mf-h264-encoder.hpp"
#include "mf-encoder-descriptor.hpp"
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
	bool advanced;
	uint32_t bitrate;
	uint32_t maxBitrate;
	bool useMaxBitrate;
	uint32_t bufferSize;
	bool useBufferSize;
	MF::H264Profile profile;
	MF::H264RateControl rateControl;
	MF::H264QP qp;
	uint32_t minQp;
	uint32_t maxQp;
	bool lowLatency;
	uint32_t bFrames;

	const char *profiler_encode = nullptr;
};

static const char *TEXT_ADVANCED = obs_module_text("MF.H264.Advanced");
static const char *TEXT_LOW_LAT = obs_module_text("MF.H264.LowLatency");
static const char *TEXT_B_FRAMES = obs_module_text("MF.H264.BFrames");
static const char *TEXT_BITRATE = obs_module_text("MF.H264.Bitrate");
static const char *TEXT_CUSTOM_BUF = obs_module_text("MF.H264.CustomBufsize");
static const char *TEXT_BUF_SIZE = obs_module_text("MF.H264.BufferSize");
static const char *TEXT_USE_MAX_BITRATE = obs_module_text("MF.H264.CustomMaxBitrate");
static const char *TEXT_MAX_BITRATE = obs_module_text("MF.H264.MaxBitrate");
static const char *TEXT_KEYINT_SEC = obs_module_text("MF.H264.KeyframeIntervalSec");
static const char *TEXT_RATE_CONTROL = obs_module_text("MF.H264.RateControl");
static const char *TEXT_MIN_QP = obs_module_text("MF.H264.MinQP");
static const char *TEXT_MAX_QP = obs_module_text("MF.H264.MaxQP");
static const char *TEXT_QPI = obs_module_text("MF.H264.QPI");
static const char *TEXT_QPP = obs_module_text("MF.H264.QPP");
static const char *TEXT_QPB = obs_module_text("MF.H264.QPB");
static const char *TEXT_PROFILE = obs_module_text("MF.H264.Profile");
static const char *TEXT_CBR = obs_module_text("MF.H264.CBR");
static const char *TEXT_VBR = obs_module_text("MF.H264.VBR");
static const char *TEXT_CQP = obs_module_text("MF.H264.CQP");

constexpr const char *MFP_USE_ADVANCED = "mf_h264_use_advanced";
constexpr const char *MFP_USE_LOWLAT = "mf_h264_use_low_latency";
constexpr const char *MFP_B_FRAMES = "mf_h264_b_frames";
constexpr const char *MFP_BITRATE = "mf_h264_bitrate";
constexpr const char *MFP_USE_BUF_SIZE = "mf_h264_use_buf_size";
constexpr const char *MFP_BUF_SIZE = "mf_h264_buf_size";
constexpr const char *MFP_USE_MAX_BITRATE = "mf_h264_use_max_bitrate";
constexpr const char *MFP_MAX_BITRATE = "mf_h264_max_bitrate";
constexpr const char *MFP_KEY_INT = "mf_h264_key_int";
constexpr const char *MFP_RATE_CONTROL = "mf_h264_rate_control";
constexpr const char *MFP_MIN_QP = "mf_h264_min_qp";
constexpr const char *MFP_MAX_QP = "mf_h264_max_qp";
constexpr const char *MFP_QP_I = "mf_h264_qp_i";
constexpr const char *MFP_QP_P = "mf_h264_qp_p";
constexpr const char *MFP_QP_B = "mf_h264_qp_b";
constexpr const char *MFP_PROFILE = "mf_h264_profile";

struct TypeData {
	std::shared_ptr<MF::EncoderDescriptor> descriptor;
	inline TypeData(std::shared_ptr<MF::EncoderDescriptor> descriptor_) : descriptor(descriptor_) {}
};

namespace {
const char *MFH264_GetName(void *type_data)
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

	MF::H264RateControl rateControl = (MF::H264RateControl)obs_data_get_int(settings, MFP_RATE_CONTROL);

	if (rateControl == MF::H264RateControlCBR || rateControl == MF::H264RateControlVBR) {
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);
		use_max_bitrate_modified(ppts, NULL, settings);
	}

	return true;
}

bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	MF::H264RateControl rateControl = (MF::H264RateControl)obs_data_get_int(settings, MFP_RATE_CONTROL);

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
	case MF::H264RateControlCBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, MFP_BITRATE, true);
		set_visible(ppts, MFP_USE_BUF_SIZE, true);
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);

		break;
	case MF::H264RateControlVBR:
		use_bufsize_modified(ppts, NULL, settings);
		use_max_bitrate_modified(ppts, NULL, settings);

		set_visible(ppts, MFP_BITRATE, true);
		set_visible(ppts, MFP_USE_BUF_SIZE, true);
		set_visible(ppts, MFP_USE_MAX_BITRATE, advanced);

		break;
	case MF::H264RateControlCQP:
		set_visible(ppts, MFP_QP_I, true);
		set_visible(ppts, MFP_QP_P, true);
		set_visible(ppts, MFP_QP_B, true);

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

	obs_property_t *list =
		obs_properties_add_list(props, MFP_PROFILE, TEXT_PROFILE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, "baseline", MF::H264ProfileBaseline);
	obs_property_list_add_int(list, "main", MF::H264ProfileMain);
	obs_property_list_add_int(list, "high", MF::H264ProfileHigh);

	obs_properties_add_int(props, MFP_KEY_INT, TEXT_KEYINT_SEC, 0, 20, 1);

	list = obs_properties_add_list(props, MFP_RATE_CONTROL, TEXT_RATE_CONTROL, OBS_COMBO_TYPE_LIST,
				       OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(list, TEXT_CBR, MF::H264RateControlCBR);
	obs_property_list_add_int(list, TEXT_VBR, MF::H264RateControlVBR);
	obs_property_list_add_int(list, TEXT_CQP, MF::H264RateControlCQP);

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

void MFH264_GetDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, MFP_BITRATE, 2500);
	obs_data_set_default_bool(settings, MFP_USE_LOWLAT, true);
	obs_data_set_default_int(settings, MFP_B_FRAMES, 2);
	obs_data_set_default_bool(settings, MFP_USE_BUF_SIZE, false);
	obs_data_set_default_int(settings, MFP_BUF_SIZE, 2500);
	obs_data_set_default_bool(settings, MFP_USE_MAX_BITRATE, false);
	obs_data_set_default_int(settings, MFP_MAX_BITRATE, 2500);
	obs_data_set_default_int(settings, MFP_KEY_INT, 2);
	obs_data_set_default_int(settings, MFP_RATE_CONTROL, MF::H264RateControlCBR);
	obs_data_set_default_int(settings, MFP_PROFILE, MF::H264ProfileMain);
	obs_data_set_default_int(settings, MFP_MIN_QP, 1);
	obs_data_set_default_int(settings, MFP_MAX_QP, 51);
	obs_data_set_default_int(settings, MFP_QP_I, 26);
	obs_data_set_default_int(settings, MFP_QP_B, 26);
	obs_data_set_default_int(settings, MFP_QP_P, 26);
	obs_data_set_default_bool(settings, MFP_USE_ADVANCED, false);
}

void UpdateParams(MFH264_Encoder *enc, obs_data_t *settings)
{
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	TypeData &typeData = *reinterpret_cast<TypeData *>(obs_encoder_get_type_data(enc->encoder));

	enc->width = (uint32_t)obs_encoder_get_width(enc->encoder);
	enc->height = (uint32_t)obs_encoder_get_height(enc->encoder);
	enc->framerateNum = voi->fps_num;
	enc->framerateDen = voi->fps_den;

	enc->descriptor = typeData.descriptor;
	enc->profile = static_cast<MF::H264Profile>(obs_data_get_int(settings, MFP_PROFILE));
	enc->rateControl = static_cast<MF::H264RateControl>(obs_data_get_int(settings, MFP_RATE_CONTROL));
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
bool ApplyCBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->useMaxBitrate)
		enc->h264Encoder->SetMaxBitrate(enc->maxBitrate);
	else
		enc->h264Encoder->SetMaxBitrate(enc->bitrate);

	if (enc->useBufferSize)
		enc->h264Encoder->SetBufferSize(enc->bufferSize);

	return true;
}

bool ApplyCVBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->advanced && enc->useMaxBitrate)
		enc->h264Encoder->SetMaxBitrate(enc->maxBitrate);
	else
		enc->h264Encoder->SetMaxBitrate(enc->bitrate);

	if (enc->useBufferSize)
		enc->h264Encoder->SetBufferSize(enc->bufferSize);

	return true;
}

bool ApplyVBR(MFH264_Encoder *enc)
{
	enc->h264Encoder->SetBitrate(enc->bitrate);

	if (enc->useBufferSize)
		enc->h264Encoder->SetBufferSize(enc->bufferSize);

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

		if (enc->advanced) {
			enc.get()->h264Encoder->SetLowLatency(enc->lowLatency);
			enc.get()->h264Encoder->SetBFrameCount(enc->bFrames);

			enc.get()->h264Encoder->SetMinQP(enc->minQp);
			enc.get()->h264Encoder->SetMaxQP(enc->maxQp);
		}

		if (enc->rateControl == MF::H264RateControlVBR && enc->advanced && enc->useMaxBitrate)
			enc->rateControl = MF::H264RateControlConstrainedVBR;

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

	if (!enc->h264Encoder->Initialize(applySettings))
		return nullptr;

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

	if (!enc->profiler_encode)
		enc->profiler_encode =
			profile_store_name(obs_get_profiler_name_store(), "MFH264_Encode(%s)", enc->descriptor->Name());

	ProfileScope(enc->profiler_encode);

	*received_packet = false;

	if (!enc->h264Encoder->ProcessInput(frame->data, frame->linesize, (frame->pts / packet->timebase_num), &status))
		return false;

	UINT8 *outputData;
	UINT32 outputDataLength;
	UINT64 outputPts;
	UINT64 outputDts;
	bool keyframe;

	if (!enc->h264Encoder->ProcessOutput(&outputData, &outputDataLength, &outputPts, &outputDts, &keyframe,
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

bool MFH264_GetExtraData(void *data, uint8_t **extra_data, size_t *size)
{
	MFH264_Encoder *enc = static_cast<MFH264_Encoder *>(data);

	uint8_t *extraData;
	UINT32 extraDataLength;

	if (!enc->h264Encoder->ExtraData(&extraData, &extraDataLength))
		return false;

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

void RegisterMFH264Encoders()
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
	info.codec = "h264";

	auto encoders = MF::EncoderDescriptor::Enumerate("h264");
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
