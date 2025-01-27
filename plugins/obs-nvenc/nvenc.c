#include "nvenc-internal.h"

#include <util/darray.h>
#include <util/dstr.h>

/* ========================================================================= */

#define EXTRA_BUFFERS 5

#ifndef _WIN32
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* ------------------------------------------------------------------------- */
/* Bitstream Buffer                                                          */

static bool nv_bitstream_init(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	NV_ENC_CREATE_BITSTREAM_BUFFER buf = {NV_ENC_CREATE_BITSTREAM_BUFFER_VER};

	if (NV_FAILED(nv.nvEncCreateBitstreamBuffer(enc->session, &buf))) {
		return false;
	}

	bs->ptr = buf.bitstreamBuffer;
	return true;
}

static void nv_bitstream_free(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	if (bs->ptr) {
		nv.nvEncDestroyBitstreamBuffer(enc->session, bs->ptr);
	}
}

/* ------------------------------------------------------------------------- */
/* Implementation                                                            */

static const char *h264_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC H.264";
}

static const char *h264_nvenc_soft_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC H.264 (Fallback)";
}

#ifdef ENABLE_HEVC
static const char *hevc_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC HEVC";
}

static const char *hevc_nvenc_soft_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC HEVC (Fallback)";
}
#endif

static const char *av1_nvenc_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC AV1";
}

static const char *av1_nvenc_soft_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC AV1 (Fallback)";
}

static inline int nv_get_cap(struct nvenc_data *enc, NV_ENC_CAPS cap)
{
	if (!enc->session)
		return 0;

	NV_ENC_CAPS_PARAM param = {NV_ENC_CAPS_PARAM_VER};
	int v;

	param.capsToQuery = cap;
	nv.nvEncGetEncodeCaps(enc->session, enc->codec_guid, &param, &v);
	return v;
}

static bool nvenc_update(void *data, obs_data_t *settings)
{
	struct nvenc_data *enc = data;

	/* Only support reconfiguration of CBR bitrate */
	if (enc->can_change_bitrate) {
		enc->props.bitrate = obs_data_get_int(settings, "bitrate");
		enc->props.max_bitrate = obs_data_get_int(settings, "max_bitrate");

		bool vbr = (enc->config.rcParams.rateControlMode == NV_ENC_PARAMS_RC_VBR);
		enc->config.rcParams.averageBitRate = (uint32_t)enc->props.bitrate * 1000;
		enc->config.rcParams.maxBitRate = vbr ? (uint32_t)enc->props.max_bitrate * 1000
						      : (uint32_t)enc->props.bitrate * 1000;

		NV_ENC_RECONFIGURE_PARAMS params = {0};
		params.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		params.reInitEncodeParams = enc->params;
		params.resetEncoder = 1;
		params.forceIDR = 1;

		if (NV_FAILED(nv.nvEncReconfigureEncoder(enc->session, &params))) {
			return false;
		}
	}

	return true;
}

static bool init_session(struct nvenc_data *enc)
{
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER};
	params.apiVersion = NVENCAPI_VERSION;
#ifdef _WIN32
	if (enc->non_texture) {
		params.device = enc->cu_ctx;
		params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
	} else {
		params.device = enc->device;
		params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	}
#else
	params.device = enc->cu_ctx;
	params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
#endif

	if (NV_FAILED(nv.nvEncOpenEncodeSessionEx(&params, &enc->session))) {
		return false;
	}
	return true;
}

static void initialize_params(struct nvenc_data *enc, const GUID *nv_preset, NV_ENC_TUNING_INFO nv_tuning,
			      uint32_t width, uint32_t height, uint32_t fps_num, uint32_t fps_den)
{
	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	memset(params, 0, sizeof(*params));
	params->version = NV_ENC_INITIALIZE_PARAMS_VER;
	params->encodeGUID = enc->codec_guid;
	params->presetGUID = *nv_preset;
	params->encodeWidth = width;
	params->encodeHeight = height;
	params->darWidth = width;
	params->darHeight = height;
	params->frameRateNum = fps_num;
	params->frameRateDen = fps_den;
	params->enableEncodeAsync = 0;
	params->enablePTD = 1;
	params->encodeConfig = &enc->config;
	params->tuningInfo = nv_tuning;
#ifdef NVENC_12_1_OR_LATER
	params->splitEncodeMode = (NV_ENC_SPLIT_ENCODE_MODE)enc->props.split_encode;
#endif
}

static inline GUID get_nv_preset(const char *preset2)
{
	if (astrcmpi(preset2, "p1") == 0) {
		return NV_ENC_PRESET_P1_GUID;
	} else if (astrcmpi(preset2, "p2") == 0) {
		return NV_ENC_PRESET_P2_GUID;
	} else if (astrcmpi(preset2, "p3") == 0) {
		return NV_ENC_PRESET_P3_GUID;
	} else if (astrcmpi(preset2, "p4") == 0) {
		return NV_ENC_PRESET_P4_GUID;
	} else if (astrcmpi(preset2, "p6") == 0) {
		return NV_ENC_PRESET_P6_GUID;
	} else if (astrcmpi(preset2, "p7") == 0) {
		return NV_ENC_PRESET_P7_GUID;
	} else {
		return NV_ENC_PRESET_P5_GUID;
	}
}

static inline NV_ENC_TUNING_INFO get_nv_tuning(const char *tuning)
{
	if (astrcmpi(tuning, "ll") == 0) {
		return NV_ENC_TUNING_INFO_LOW_LATENCY;
	} else if (astrcmpi(tuning, "ull") == 0) {
		return NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
#ifdef NVENC_12_2_OR_LATER
	} else if (astrcmpi(tuning, "uhq") == 0) {
		return NV_ENC_TUNING_INFO_ULTRA_HIGH_QUALITY;
#endif
	} else {
		return NV_ENC_TUNING_INFO_HIGH_QUALITY;
	}
}

static inline NV_ENC_MULTI_PASS get_nv_multipass(const char *multipass)
{
	if (astrcmpi(multipass, "qres") == 0) {
		return NV_ENC_TWO_PASS_QUARTER_RESOLUTION;
	} else if (astrcmpi(multipass, "fullres") == 0) {
		return NV_ENC_TWO_PASS_FULL_RESOLUTION;
	} else {
		return NV_ENC_MULTI_PASS_DISABLED;
	}
}

static bool is_10_bit(const struct nvenc_data *enc)
{
	return enc->non_texture ? enc->in_format == VIDEO_FORMAT_P010
				: obs_encoder_video_tex_active(enc->encoder, VIDEO_FORMAT_P010);
}

static bool init_encoder_base(struct nvenc_data *enc, obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);

	int bitrate = (int)enc->props.bitrate;
	int max_bitrate = (int)enc->props.max_bitrate;
	int rc_lookahead = 0;

	bool cqvbr = astrcmpi(enc->props.rate_control, "CQVBR") == 0;
	bool vbr = cqvbr || astrcmpi(enc->props.rate_control, "VBR") == 0;
	bool lossless = strcmp(enc->props.rate_control, "lossless") == 0;
	bool cqp = strcmp(enc->props.rate_control, "CQP") == 0;

	NVENCSTATUS err;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	enc->cx = obs_encoder_get_width(enc->encoder);
	enc->cy = obs_encoder_get_height(enc->encoder);

	/* -------------------------- */
	/* get preset                 */

	GUID nv_preset = get_nv_preset(enc->props.preset);
	NV_ENC_TUNING_INFO nv_tuning = get_nv_tuning(enc->props.tune);
	NV_ENC_MULTI_PASS nv_multipass = get_nv_multipass(enc->props.multipass);

	if (lossless) {
		nv_tuning = NV_ENC_TUNING_INFO_LOSSLESS;
		nv_multipass = NV_ENC_MULTI_PASS_DISABLED;
		enc->props.adaptive_quantization = false;
		enc->props.cqp = 0;
		enc->props.rate_control = "Lossless";
	}

	/* -------------------------- */
	/* get preset default config  */

	NV_ENC_PRESET_CONFIG preset_config = {0};
	preset_config.version = NV_ENC_PRESET_CONFIG_VER;
	preset_config.presetCfg.version = NV_ENC_CONFIG_VER;

	err = nv.nvEncGetEncodePresetConfigEx(enc->session, enc->codec_guid, nv_preset, nv_tuning, &preset_config);
	if (nv_failed(enc->encoder, err, __FUNCTION__, "nvEncGetEncodePresetConfig")) {
		return false;
	}

	/* -------------------------- */
	/* main configuration         */

	enc->config = preset_config.presetCfg;

	int keyint = (int)enc->props.keyint_sec * voi->fps_num / voi->fps_den;
	get_user_arg_int(enc, "keyint", &keyint);

	uint32_t gop_size = keyint > 0 ? keyint : 250;

	NV_ENC_CONFIG *config = &enc->config;

	initialize_params(enc, &nv_preset, nv_tuning, voi->width, voi->height, voi->fps_num, voi->fps_den);

#ifdef NVENC_12_2_OR_LATER
	/* Force at least 4 b-frames when using the UHQ tune */
	if (nv_tuning == NV_ENC_TUNING_INFO_ULTRA_HIGH_QUALITY && enc->props.bf < 4) {
		warn("Forcing number of b-frames to 4 for UHQ tune.");
		enc->props.bf = 4;
	}
#endif

	config->gopLength = gop_size;
	config->frameIntervalP = gop_size == 1 ? 0 : (int32_t)enc->props.bf + 1;

	/* lookahead */

	const bool use_profile_lookahead = config->rcParams.enableLookahead;
	bool lookahead = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOOKAHEAD) &&
			 (enc->props.lookahead || use_profile_lookahead);

	if (lookahead) {
		rc_lookahead = use_profile_lookahead ? config->rcParams.lookaheadDepth : 8;

		/* Due to the additional calculations required to handle lookahead,
		 * get the user override here (if any). */
		get_user_arg_int(enc, "lookaheadDepth", &rc_lookahead);
	}

	int buf_count = max(4, config->frameIntervalP * 2 * 2);
	if (lookahead) {
		buf_count = max(buf_count, config->frameIntervalP + rc_lookahead + EXTRA_BUFFERS);
	}

	buf_count = min(64, buf_count);
	enc->buf_count = buf_count;

	const int output_delay = buf_count - 1;
	enc->output_delay = output_delay;

	if (lookahead) {
		const int lkd_bound = output_delay - config->frameIntervalP - 4;
		if (lkd_bound >= 0) {
			config->rcParams.enableLookahead = 1;
			config->rcParams.lookaheadDepth = min(rc_lookahead, lkd_bound);
			config->rcParams.disableIadapt = 0;
			config->rcParams.disableBadapt = 0;
		} else {
			lookahead = false;
		}
	}

	enc->config.rcParams.disableIadapt = enc->props.disable_scenecut;

	/* psycho aq */

	if (enc->props.adaptive_quantization) {
		config->rcParams.enableAQ = 1;
		config->rcParams.aqStrength = 8;
		config->rcParams.enableTemporalAQ = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ);
	}

	/* -------------------------- */
	/* rate control               */

	enc->can_change_bitrate = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE);

	config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;
	config->rcParams.averageBitRate = bitrate * 1000;
	config->rcParams.maxBitRate = vbr ? max_bitrate * 1000 : bitrate * 1000;
	config->rcParams.vbvBufferSize = bitrate * 1000;

	if (cqp || lossless) {
		int cqp_val = enc->codec == CODEC_AV1 ? (int)enc->props.cqp * 4 : (int)enc->props.cqp;

		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
		config->rcParams.constQP.qpInterP = cqp_val;
		config->rcParams.constQP.qpInterB = cqp_val;
		config->rcParams.constQP.qpIntra = cqp_val;
		enc->can_change_bitrate = false;

		bitrate = 0;
		max_bitrate = 0;

	} else if (!vbr) { /* CBR by default */
		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
	} else if (cqvbr) {
		config->rcParams.targetQuality = (uint8_t)enc->props.target_quality;
		config->rcParams.averageBitRate = 0;
		config->rcParams.vbvBufferSize = 0;
	}

	config->rcParams.multiPass = nv_multipass;
	config->rcParams.qpMapMode = NV_ENC_QP_MAP_DELTA;

	/* -------------------------- */
	/* log settings	              */

	struct dstr log = {0};

	dstr_catf(&log, "\tcodec:        %s\n", get_codec_name(enc->codec));
	dstr_catf(&log, "\trate_control: %s\n", enc->props.rate_control);

	if (bitrate && !cqvbr)
		dstr_catf(&log, "\tbitrate:      %d\n", bitrate);
	if (vbr)
		dstr_catf(&log, "\tmax_bitrate:  %d\n", max_bitrate);
	if (cqp)
		dstr_catf(&log, "\tcqp:          %ld\n", enc->props.cqp);
	if (cqvbr) {
		dstr_catf(&log, "\tcq:           %ld\n", enc->props.target_quality);
	}

	dstr_catf(&log, "\tkeyint:       %d\n", gop_size);
	dstr_catf(&log, "\tpreset:       %s\n", enc->props.preset);
	dstr_catf(&log, "\ttuning:       %s\n", enc->props.tune);
	dstr_catf(&log, "\tmultipass:    %s\n", enc->props.multipass);
	dstr_catf(&log, "\tprofile:      %s\n", enc->props.profile);
	dstr_catf(&log, "\twidth:        %d\n", enc->cx);
	dstr_catf(&log, "\theight:       %d\n", enc->cy);
	dstr_catf(&log, "\tb-frames:     %ld\n", enc->props.bf);
	dstr_catf(&log, "\tb-ref-mode:   %ld\n", enc->props.bframe_ref_mode);
	dstr_catf(&log, "\tlookahead:    %s (%d frames)\n", lookahead ? "true" : "false", rc_lookahead);
	dstr_catf(&log, "\taq:           %s\n", enc->props.adaptive_quantization ? "true" : "false");

	if (enc->props.split_encode) {
		dstr_catf(&log, "\tsplit encode: %ld\n", enc->props.split_encode);
	}
	if (enc->props.opts.count)
		dstr_catf(&log, "\tuser opts:    %s\n", enc->props.opts_str);

	info("settings:\n%s", log.array);
	dstr_free(&log);

	return true;
}

static bool init_encoder_h264(struct nvenc_data *enc, obs_data_t *settings)
{
	bool lossless = strcmp(enc->props.rate_control, "lossless") == 0;

	if (!init_encoder_base(enc, settings)) {
		return false;
	}

	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_H264 *h264_config = &config->encodeCodecConfig.h264Config;
	NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params = &h264_config->h264VUIParameters;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	if (enc->props.repeat_headers) {
		h264_config->repeatSPSPPS = 1;
		h264_config->disableSPSPPS = 0;
		h264_config->outputAUD = 1;
	}

	h264_config->idrPeriod = config->gopLength;

	h264_config->sliceMode = 3;
	h264_config->sliceModeData = 1;

	h264_config->useBFramesAsRef = (NV_ENC_BFRAME_REF_MODE)enc->props.bframe_ref_mode;

	/* Enable CBR padding */
	if (config->rcParams.rateControlMode == NV_ENC_PARAMS_RC_CBR)
		h264_config->enableFillerDataInsertion = 1;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	default:
		break;
	}

	if (lossless) {
		h264_config->qpPrimeYZeroTransformBypassFlag = 1;
	} else if (strcmp(enc->props.rate_control, "CBR") == 0) { /* CBR */
		h264_config->outputBufferingPeriodSEI = 1;
	}

	h264_config->outputPictureTimingSEI = 1;

	/* -------------------------- */
	/* profile                    */

	if (enc->in_format == VIDEO_FORMAT_I444) {
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_444_GUID;
		h264_config->chromaFormatIDC = 3;
	} else if (astrcmpi(enc->props.profile, "main") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
	} else if (astrcmpi(enc->props.profile, "baseline") == 0) {
		config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	} else if (!lossless) {
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
	}

	if (!apply_user_args(enc)) {
		obs_encoder_set_last_error(enc->encoder, obs_module_text("Opts.Invalid"));
		return false;
	}

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_encoder_hevc(struct nvenc_data *enc, obs_data_t *settings)
{
	if (!init_encoder_base(enc, settings)) {
		return false;
	}

	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_HEVC *hevc_config = &config->encodeCodecConfig.hevcConfig;
	NV_ENC_CONFIG_HEVC_VUI_PARAMETERS *vui_params = &hevc_config->hevcVUIParameters;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	if (enc->props.repeat_headers) {
		hevc_config->repeatSPSPPS = 1;
		hevc_config->disableSPSPPS = 0;
		hevc_config->outputAUD = 1;
	}

	hevc_config->idrPeriod = config->gopLength;

	hevc_config->sliceMode = 3;
	hevc_config->sliceModeData = 1;

	hevc_config->useBFramesAsRef = (NV_ENC_BFRAME_REF_MODE)enc->props.bframe_ref_mode;

	/* Enable CBR padding */
	if (config->rcParams.rateControlMode == NV_ENC_PARAMS_RC_CBR)
		hevc_config->enableFillerDataInsertion = 1;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	case VIDEO_CS_2100_PQ:
		vui_params->colourPrimaries = 9;
		vui_params->transferCharacteristics = 16;
		vui_params->colourMatrix = 9;
		vui_params->chromaSampleLocationFlag = 1;
		vui_params->chromaSampleLocationTop = 2;
		vui_params->chromaSampleLocationBot = 2;
		break;
	case VIDEO_CS_2100_HLG:
		vui_params->colourPrimaries = 9;
		vui_params->transferCharacteristics = 18;
		vui_params->colourMatrix = 9;
		vui_params->chromaSampleLocationFlag = 1;
		vui_params->chromaSampleLocationTop = 2;
		vui_params->chromaSampleLocationBot = 2;
	}

	if (astrcmpi(enc->props.rate_control, "cbr") == 0) {
		hevc_config->outputBufferingPeriodSEI = 1;
	}

	hevc_config->outputPictureTimingSEI = 1;

	/* -------------------------- */
	/* profile                    */

	bool profile_is_10bpc = false;

	if (enc->in_format == VIDEO_FORMAT_I444) {
		config->profileGUID = NV_ENC_HEVC_PROFILE_FREXT_GUID;
		hevc_config->chromaFormatIDC = 3;
	} else if (astrcmpi(enc->props.profile, "main10") == 0) {
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
		profile_is_10bpc = true;
	} else if (is_10_bit(enc)) {
		blog(LOG_WARNING, "[obs-nvenc] Forcing main10 for P010");
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN10_GUID;
		profile_is_10bpc = true;
	} else {
		config->profileGUID = NV_ENC_HEVC_PROFILE_MAIN_GUID;
	}

#ifndef NVENC_12_2_OR_LATER
	hevc_config->pixelBitDepthMinus8 = is_10_bit(enc) ? 2 : 0;
#else
	hevc_config->inputBitDepth = is_10_bit(enc) ? NV_ENC_BIT_DEPTH_10 : NV_ENC_BIT_DEPTH_8;
	hevc_config->outputBitDepth = profile_is_10bpc ? NV_ENC_BIT_DEPTH_10 : NV_ENC_BIT_DEPTH_8;
#endif

	if (!apply_user_args(enc)) {
		obs_encoder_set_last_error(enc->encoder, obs_module_text("Opts.Invalid"));
		return false;
	}

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_encoder_av1(struct nvenc_data *enc, obs_data_t *settings)
{
	if (!init_encoder_base(enc, settings)) {
		return false;
	}

	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_AV1 *av1_config = &config->encodeCodecConfig.av1Config;

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);

	av1_config->idrPeriod = config->gopLength;

	av1_config->useBFramesAsRef = (NV_ENC_BFRAME_REF_MODE)enc->props.bframe_ref_mode;

	av1_config->colorRange = (voi->range == VIDEO_RANGE_FULL);

	/* Enable CBR padding */
	if (config->rcParams.rateControlMode == NV_ENC_PARAMS_RC_CBR)
		av1_config->enableBitstreamPadding = 1;

#define PIXELCOUNT_4K (3840 * 2160)

	/* If size is 4K+, set tiles to 2 uniform columns. */
	if ((voi->width * voi->height) >= PIXELCOUNT_4K)
		av1_config->numTileColumns = 2;

	switch (voi->colorspace) {
	case VIDEO_CS_601:
		av1_config->colorPrimaries = 6;
		av1_config->transferCharacteristics = 6;
		av1_config->matrixCoefficients = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		av1_config->colorPrimaries = 1;
		av1_config->transferCharacteristics = 1;
		av1_config->matrixCoefficients = 1;
		break;
	case VIDEO_CS_SRGB:
		av1_config->colorPrimaries = 1;
		av1_config->transferCharacteristics = 13;
		av1_config->matrixCoefficients = 1;
		break;
	case VIDEO_CS_2100_PQ:
		av1_config->colorPrimaries = 9;
		av1_config->transferCharacteristics = 16;
		av1_config->matrixCoefficients = 9;
		break;
	case VIDEO_CS_2100_HLG:
		av1_config->colorPrimaries = 9;
		av1_config->transferCharacteristics = 18;
		av1_config->matrixCoefficients = 9;
	}

	/* -------------------------- */
	/* profile                    */

	config->profileGUID = NV_ENC_AV1_PROFILE_MAIN_GUID;
	av1_config->tier = NV_ENC_TIER_AV1_0;

	av1_config->level = NV_ENC_LEVEL_AV1_AUTOSELECT;
	av1_config->chromaFormatIDC = 1;
#ifndef NVENC_12_2_OR_LATER
	av1_config->pixelBitDepthMinus8 = is_10_bit(enc) ? 2 : 0;
	av1_config->inputPixelBitDepthMinus8 = av1_config->pixelBitDepthMinus8;
#else
	av1_config->inputBitDepth = is_10_bit(enc) ? NV_ENC_BIT_DEPTH_10 : NV_ENC_BIT_DEPTH_8;
	av1_config->outputBitDepth = av1_config->inputBitDepth;
#endif
	av1_config->numFwdRefs = 1;
	av1_config->numBwdRefs = 1;
	av1_config->repeatSeqHdr = 1;

	if (!apply_user_args(enc)) {
		obs_encoder_set_last_error(enc->encoder, obs_module_text("Opts.Invalid"));
		return false;
	}

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, &enc->params))) {
		return false;
	}

	return true;
}

static bool init_bitstreams(struct nvenc_data *enc)
{
	da_reserve(enc->bitstreams, enc->buf_count);
	for (uint32_t i = 0; i < enc->buf_count; i++) {
		struct nv_bitstream bitstream;
		if (!nv_bitstream_init(enc, &bitstream)) {
			return false;
		}

		da_push_back(enc->bitstreams, &bitstream);
	}

	return true;
}

static enum video_format get_preferred_format(enum video_format format)
{
	switch (format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		return VIDEO_FORMAT_P010;
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_I444:
		return VIDEO_FORMAT_I444;
	default:
		return VIDEO_FORMAT_NV12;
	}
}

static void nvenc_destroy(void *data);

static bool init_encoder(struct nvenc_data *enc, enum codec_type codec, obs_data_t *settings, obs_encoder_t *encoder)
{
	UNUSED_PARAMETER(codec);
	UNUSED_PARAMETER(encoder);

	const bool support_10bit = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_10BIT_ENCODE);
	const bool support_444 = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_YUV444_ENCODE);

	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	enum video_format pref_format = obs_encoder_get_preferred_video_format(enc->encoder);
	if (pref_format == VIDEO_FORMAT_NONE)
		pref_format = voi->format;

	enc->in_format = get_preferred_format(pref_format);

	if (enc->in_format == VIDEO_FORMAT_I444 && !support_444) {
		NV_FAIL(obs_module_text("444Unsupported"));
		return false;
	}

	if (is_10_bit(enc) && !support_10bit) {
		NV_FAIL(obs_module_text("10bitUnsupported"));
		return false;
	}

	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG:
			NV_FAIL(obs_module_text("8bitUnsupportedHdr"));
			return false;
		default:
			break;
		}
	}

	switch (enc->codec) {
	case CODEC_HEVC:
		return init_encoder_hevc(enc, settings);
	case CODEC_H264:
		return init_encoder_h264(enc, settings);
	case CODEC_AV1:
		return init_encoder_av1(enc, settings);
	}

	return false;
}

static void *nvenc_create_internal(enum codec_type codec, obs_data_t *settings, obs_encoder_t *encoder, bool texture)
{
	struct nvenc_data *enc = bzalloc(sizeof(*enc));
	enc->encoder = encoder;
	enc->codec = codec;
	enc->first_packet = true;
	enc->non_texture = !texture;

	nvenc_properties_read(&enc->props, settings);

	NV_ENCODE_API_FUNCTION_LIST init = {NV_ENCODE_API_FUNCTION_LIST_VER};

	switch (enc->codec) {
	case CODEC_H264:
		enc->codec_guid = NV_ENC_CODEC_H264_GUID;
		break;
	case CODEC_HEVC:
		enc->codec_guid = NV_ENC_CODEC_HEVC_GUID;
		break;
	case CODEC_AV1:
		enc->codec_guid = NV_ENC_CODEC_AV1_GUID;
		break;
	}

	if (!init_nvenc(encoder))
		goto fail;

#ifdef _WIN32
	if (texture ? !d3d11_init(enc, settings) : !init_cuda(encoder))
		goto fail;
#else
	if (!init_cuda(encoder))
		goto fail;
#endif

	if (NV_FAILED(nv_create_instance(&init)))
		goto fail;

	if (!cuda_ctx_init(enc, settings, texture))
		goto fail;

	if (!init_session(enc)) {
		goto fail;
	}
	if (!init_encoder(enc, codec, settings, encoder)) {
		goto fail;
	}
	if (!init_bitstreams(enc)) {
		goto fail;
	}

#ifdef _WIN32
	if (texture ? !d3d11_init_textures(enc) : !cuda_init_surfaces(enc))
		goto fail;
#else
	if (!cuda_init_surfaces(enc))
		goto fail;
#endif

	enc->codec = codec;

	return enc;

fail:
	nvenc_destroy(enc);
	return NULL;
}

static void *nvenc_create_base(enum codec_type codec, obs_data_t *settings, obs_encoder_t *encoder, bool texture)
{
	/* This encoder requires shared textures, this cannot be used on a
	 * gpu other than the one OBS is currently running on.
	 *
	 * 2024 Amendment: On Linux when using CUDA<->OpenGL interop we can
	 * in fact use shared textures even when using a different GPU, this
	 * will still copy data through the CPU, but much more efficiently than
	 * our native non-texture encoder. For now allow this via a hidden
	 * option as it may cause issues for people.
	 */
	const int gpu = (int)obs_data_get_int(settings, "device");
	const bool gpu_set = obs_data_has_user_value(settings, "device");
#ifndef _WIN32
	const bool force_tex = obs_data_get_bool(settings, "force_cuda_tex");
#else
	const bool force_tex = false;
#endif

	if (gpu_set && gpu != -1 && texture && !force_tex) {
		blog(LOG_INFO, "[obs-nvenc] different GPU selected by user, falling back "
			       "to non-texture encoder");
		goto reroute;
	}

	if (obs_encoder_scaling_enabled(encoder)) {
		if (obs_encoder_gpu_scaling_enabled(encoder)) {
			blog(LOG_INFO, "[obs-nvenc] GPU scaling enabled");
		} else if (texture) {
			blog(LOG_INFO, "[obs-nvenc] CPU scaling enabled, falling back to"
				       " non-texture encoder");
			goto reroute;
		}
	}

	if (texture && !obs_encoder_video_tex_active(encoder, VIDEO_FORMAT_NV12) &&
	    !obs_encoder_video_tex_active(encoder, VIDEO_FORMAT_P010)) {
		blog(LOG_INFO, "[obs-nvenc] nv12/p010 not active, falling back to "
			       "non-texture encoder");
		goto reroute;
	}

	struct nvenc_data *enc = nvenc_create_internal(codec, settings, encoder, texture);

	if (enc) {
		return enc;
	}

reroute:
	if (!texture) {
		blog(LOG_ERROR, "Already in non_texture encoder, can't fall back further!");
		return NULL;
	}

	switch (codec) {
	case CODEC_H264:
		return obs_encoder_create_rerouted(encoder, "obs_nvenc_h264_soft");
	case CODEC_HEVC:
		return obs_encoder_create_rerouted(encoder, "obs_nvenc_hevc_soft");
	case CODEC_AV1:
		return obs_encoder_create_rerouted(encoder, "obs_nvenc_av1_soft");
	}

	return NULL;
}

static void *h264_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_H264, settings, encoder, true);
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_HEVC, settings, encoder, true);
}
#endif

static void *av1_nvenc_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_AV1, settings, encoder, true);
}

static void *h264_nvenc_soft_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_H264, settings, encoder, false);
}

#ifdef ENABLE_HEVC
static void *hevc_nvenc_soft_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_HEVC, settings, encoder, false);
}
#endif

static void *av1_nvenc_soft_create(obs_data_t *settings, obs_encoder_t *encoder)
{
	return nvenc_create_base(CODEC_AV1, settings, encoder, false);
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize);

static void nvenc_destroy(void *data)
{
	struct nvenc_data *enc = data;

	if (enc->encode_started) {
		NV_ENC_PIC_PARAMS params = {NV_ENC_PIC_PARAMS_VER};
		params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
		nv.nvEncEncodePicture(enc->session, &params);
		get_encoded_packet(enc, true);
	}

	for (size_t i = 0; i < enc->bitstreams.num; i++) {
		nv_bitstream_free(enc, &enc->bitstreams.array[i]);
	}
	if (enc->session)
		nv.nvEncDestroyEncoder(enc->session);

#ifdef _WIN32
	d3d11_free_textures(enc);
	d3d11_free(enc);
#else
	cuda_opengl_free(enc);
#endif
	cuda_free_surfaces(enc);
	cuda_ctx_free(enc);

	bfree(enc->header);
	bfree(enc->sei);
	bfree(enc->roi_map);

	deque_free(&enc->dts_list);

	da_free(enc->surfaces);
	da_free(enc->input_textures);
	da_free(enc->bitstreams);
#ifdef _WIN32
	da_free(enc->textures);
#endif
	da_free(enc->packet_data);

	obs_free_options(enc->props.opts);
	obs_data_release(enc->props.data);

	bfree(enc);
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize)
{
	void *s = enc->session;

	da_resize(enc->packet_data, 0);

	if (!enc->buffers_queued)
		return true;
	if (!finalize && enc->buffers_queued < enc->output_delay)
		return true;

	size_t count = finalize ? enc->buffers_queued : 1;

	for (size_t i = 0; i < count; i++) {
		size_t cur_bs_idx = enc->cur_bitstream;
		struct nv_bitstream *bs = &enc->bitstreams.array[cur_bs_idx];
#ifdef _WIN32
		struct nv_texture *nvtex = enc->non_texture ? NULL : &enc->textures.array[cur_bs_idx];
		struct nv_cuda_surface *surf = enc->non_texture ? &enc->surfaces.array[cur_bs_idx] : NULL;
#else
		struct nv_cuda_surface *surf = &enc->surfaces.array[cur_bs_idx];
#endif

		/* ---------------- */

		NV_ENC_LOCK_BITSTREAM lock = {NV_ENC_LOCK_BITSTREAM_VER};
		lock.outputBitstream = bs->ptr;
		lock.doNotWait = false;

		if (NV_FAILED(nv.nvEncLockBitstream(s, &lock))) {
			return false;
		}

		if (enc->first_packet) {
			NV_ENC_SEQUENCE_PARAM_PAYLOAD payload = {0};
			uint8_t buf[256];
			uint32_t size = 0;

			payload.version = NV_ENC_SEQUENCE_PARAM_PAYLOAD_VER;
			payload.spsppsBuffer = buf;
			payload.inBufferSize = sizeof(buf);
			payload.outSPSPPSPayloadSize = &size;

			nv.nvEncGetSequenceParams(s, &payload);
			enc->header = bmemdup(buf, size);
			enc->header_size = size;
			enc->first_packet = false;
		}

		da_copy_array(enc->packet_data, lock.bitstreamBufferPtr, lock.bitstreamSizeInBytes);

		enc->packet_pts = (int64_t)lock.outputTimeStamp;
		enc->packet_keyframe = lock.pictureType == NV_ENC_PIC_TYPE_IDR;

		if (NV_FAILED(nv.nvEncUnlockBitstream(s, bs->ptr))) {
			return false;
		}

		/* ---------------- */
#ifdef _WIN32
		if (nvtex && nvtex->mapped_res) {
			NVENCSTATUS err;
			err = nv.nvEncUnmapInputResource(s, nvtex->mapped_res);
			if (nv_failed(enc->encoder, err, __FUNCTION__, "unmap")) {
				return false;
			}
			nvtex->mapped_res = NULL;
		}
#endif
		/* ---------------- */

		if (surf && surf->mapped_res) {
			NVENCSTATUS err;
			err = nv.nvEncUnmapInputResource(s, surf->mapped_res);
			if (nv_failed(enc->encoder, err, __FUNCTION__, "unmap")) {
				return false;
			}
			surf->mapped_res = NULL;
		}

		/* ---------------- */

		if (++enc->cur_bitstream == enc->buf_count)
			enc->cur_bitstream = 0;

		enc->buffers_queued--;
	}

	return true;
}

struct roi_params {
	uint32_t mb_width;
	uint32_t mb_height;
	uint32_t mb_size;
	bool av1;
	int8_t *map;
};

static void roi_cb(void *param, struct obs_encoder_roi *roi)
{
	const struct roi_params *rp = param;

	int8_t qp_val;
	/* AV1 has a larger QP range than HEVC/H.264 */
	if (rp->av1) {
		qp_val = (int8_t)(-128.0f * roi->priority);
	} else {
		qp_val = (int8_t)(-51.0f * roi->priority);
	}

	const uint32_t roi_left = roi->left / rp->mb_size;
	const uint32_t roi_top = roi->top / rp->mb_size;
	const uint32_t roi_right = (roi->right - 1) / rp->mb_size;
	const uint32_t roi_bottom = (roi->bottom - 1) / rp->mb_size;

	for (uint32_t mb_y = 0; mb_y < rp->mb_height; mb_y++) {
		if (mb_y < roi_top || mb_y > roi_bottom)
			continue;

		for (uint32_t mb_x = 0; mb_x < rp->mb_width; mb_x++) {
			if (mb_x < roi_left || mb_x > roi_right)
				continue;

			rp->map[mb_y * rp->mb_width + mb_x] = qp_val;
		}
	}
}

static void add_roi(struct nvenc_data *enc, NV_ENC_PIC_PARAMS *params)
{
	const uint32_t increment = obs_encoder_get_roi_increment(enc->encoder);

	if (enc->roi_map && enc->roi_increment == increment) {
		params->qpDeltaMap = enc->roi_map;
		params->qpDeltaMapSize = (uint32_t)enc->roi_map_size;
		return;
	}

	uint32_t mb_size = 0;
	switch (enc->codec) {
	case CODEC_H264:
		/* H.264 is always 16x16 */
		mb_size = 16;
		break;
	case CODEC_HEVC:
		/* HEVC can be 16x16, 32x32, or 64x64, but NVENC is always 32x32 */
		mb_size = 32;
		break;
	case CODEC_AV1:
		/* AV1 can be 64x64 or 128x128, but NVENC is always 64x64 */
		mb_size = 64;
		break;
	}

	const uint32_t mb_width = (enc->cx + mb_size - 1) / mb_size;
	const uint32_t mb_height = (enc->cy + mb_size - 1) / mb_size;
	const size_t map_size = mb_width * mb_height * sizeof(int8_t);

	if (map_size != enc->roi_map_size) {
		enc->roi_map = brealloc(enc->roi_map, map_size);
		enc->roi_map_size = map_size;
	}

	memset(enc->roi_map, 0, enc->roi_map_size);

	struct roi_params par = {
		.mb_width = mb_width,
		.mb_height = mb_height,
		.mb_size = mb_size,
		.av1 = enc->codec == CODEC_AV1,
		.map = enc->roi_map,
	};

	obs_encoder_enum_roi(enc->encoder, roi_cb, &par);

	enc->roi_increment = increment;
	params->qpDeltaMap = enc->roi_map;
	params->qpDeltaMapSize = (uint32_t)map_size;
}

bool nvenc_encode_base(struct nvenc_data *enc, struct nv_bitstream *bs, void *pic, int64_t pts,
		       struct encoder_packet *packet, bool *received_packet)
{
	NV_ENC_PIC_PARAMS params = {0};
	params.version = NV_ENC_PIC_PARAMS_VER;
	params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	params.inputBuffer = pic;
	params.inputTimeStamp = (uint64_t)pts;
	params.inputWidth = enc->cx;
	params.inputHeight = enc->cy;
	params.inputPitch = enc->cx;
	params.outputBitstream = bs->ptr;
	params.frameIdx = (uint32_t)pts;

	if (enc->non_texture) {
		params.bufferFmt = enc->surface_format;
	} else {
		params.bufferFmt = obs_encoder_video_tex_active(enc->encoder, VIDEO_FORMAT_P010)
					   ? NV_ENC_BUFFER_FORMAT_YUV420_10BIT
					   : NV_ENC_BUFFER_FORMAT_NV12;
	}

	/* Add ROI map if enabled */
	if (obs_encoder_has_roi(enc->encoder))
		add_roi(enc, &params);

	NVENCSTATUS err = nv.nvEncEncodePicture(enc->session, &params);
	if (err != NV_ENC_SUCCESS && err != NV_ENC_ERR_NEED_MORE_INPUT) {
		nv_failed(enc->encoder, err, __FUNCTION__, "nvEncEncodePicture");
		return false;
	}

	enc->encode_started = true;
	enc->buffers_queued++;

	if (++enc->next_bitstream == enc->buf_count) {
		enc->next_bitstream = 0;
	}

	/* ------------------------------------ */
	/* check for encoded packet and parse   */

	if (!get_encoded_packet(enc, false)) {
		return false;
	}

	/* ------------------------------------ */
	/* output encoded packet                */

	if (enc->packet_data.num) {
		int64_t dts;
		deque_pop_front(&enc->dts_list, &dts, sizeof(dts));

		/* subtract bframe delay from dts for H.264/HEVC */
		if (enc->codec != CODEC_AV1)
			dts -= enc->props.bf * packet->timebase_num;

		*received_packet = true;
		packet->data = enc->packet_data.array;
		packet->size = enc->packet_data.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->pts = enc->packet_pts;
		packet->dts = dts;
		packet->keyframe = enc->packet_keyframe;
	} else {
		*received_packet = false;
	}

	return true;
}

static void nvenc_soft_video_info(void *data, struct video_scale_info *info)
{
	struct nvenc_data *enc = data;
	info->format = enc->in_format;
}

static bool nvenc_extra_data(void *data, uint8_t **header, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->header) {
		return false;
	}

	*header = enc->header;
	*size = enc->header_size;
	return true;
}

static bool nvenc_sei_data(void *data, uint8_t **sei, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->sei) {
		return false;
	}

	*sei = enc->sei;
	*size = enc->sei_size;
	return true;
}

struct obs_encoder_info h264_nvenc_info = {
	.id = "obs_nvenc_h264_tex",
	.codec = "h264",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI,
	.get_name = h264_nvenc_get_name,
	.create = h264_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
#ifdef _WIN32
	.encode_texture2 = d3d11_encode,
#else
	.encode_texture2 = cuda_opengl_encode,
#endif
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info hevc_nvenc_info = {
	.id = "obs_nvenc_hevc_tex",
	.codec = "hevc",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI,
	.get_name = hevc_nvenc_get_name,
	.create = hevc_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
#ifdef _WIN32
	.encode_texture2 = d3d11_encode,
#else
	.encode_texture2 = cuda_opengl_encode,
#endif
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
};
#endif

struct obs_encoder_info av1_nvenc_info = {
	.id = "obs_nvenc_av1_tex",
	.codec = "av1",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI,
	.get_name = av1_nvenc_get_name,
	.create = av1_nvenc_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
#ifdef _WIN32
	.encode_texture2 = d3d11_encode,
#else
	.encode_texture2 = cuda_opengl_encode,
#endif
	.get_defaults = av1_nvenc_defaults,
	.get_properties = av1_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
};

struct obs_encoder_info h264_nvenc_soft_info = {
	.id = "obs_nvenc_h264_soft",
	.codec = "h264",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_INTERNAL,
	.get_name = h264_nvenc_soft_get_name,
	.create = h264_nvenc_soft_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode = cuda_encode,
	.get_defaults = h264_nvenc_defaults,
	.get_properties = h264_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
	.get_video_info = nvenc_soft_video_info,
};

#ifdef ENABLE_HEVC
struct obs_encoder_info hevc_nvenc_soft_info = {
	.id = "obs_nvenc_hevc_soft",
	.codec = "hevc",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_INTERNAL,
	.get_name = hevc_nvenc_soft_get_name,
	.create = hevc_nvenc_soft_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode = cuda_encode,
	.get_defaults = hevc_nvenc_defaults,
	.get_properties = hevc_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_sei_data = nvenc_sei_data,
	.get_video_info = nvenc_soft_video_info,
};
#endif

struct obs_encoder_info av1_nvenc_soft_info = {
	.id = "obs_nvenc_av1_soft",
	.codec = "av1",
	.type = OBS_ENCODER_VIDEO,
	.caps = OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI | OBS_ENCODER_CAP_INTERNAL,
	.get_name = av1_nvenc_soft_get_name,
	.create = av1_nvenc_soft_create,
	.destroy = nvenc_destroy,
	.update = nvenc_update,
	.encode = cuda_encode,
	.get_defaults = av1_nvenc_defaults,
	.get_properties = av1_nvenc_properties,
	.get_extra_data = nvenc_extra_data,
	.get_video_info = nvenc_soft_video_info,
};

void register_encoders(void)
{
	obs_register_encoder(&h264_nvenc_info);
	obs_register_encoder(&h264_nvenc_soft_info);
#ifdef ENABLE_HEVC
	obs_register_encoder(&hevc_nvenc_info);
	obs_register_encoder(&hevc_nvenc_soft_info);
#endif
	if (is_codec_supported(CODEC_AV1)) {
		obs_register_encoder(&av1_nvenc_info);
		obs_register_encoder(&av1_nvenc_soft_info);
	}
}
