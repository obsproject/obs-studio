#include "nvenc-internal.h"

#include <stdio.h>
#include <inttypes.h>

/* NVIDIA uses bitfields for a variety of options. As it is not possible to
 * use offsetof() or similar with those we resort to macros here to avoid too
 * much boilerplate. */

#define APPLY_BIT_OPT(opt_name, bits)                                                                               \
	if (strcmp(opt->name, #opt_name) == 0) {                                                                    \
		uint32_t old_val = nv_conf->opt_name;                                                               \
		nv_conf->opt_name = strtol(opt->value, NULL, 10);                                                   \
		blog(LOG_DEBUG, "[obs-nvenc] Changing parameter: \"%s\": %u -> %u (%d bit(s))", #opt_name, old_val, \
		     nv_conf->opt_name, bits);                                                                      \
		return true;                                                                                        \
	}

#define APPLY_INT_OPT(opt_name, type, format)                                                                         \
	if (strcmp(opt->name, #opt_name) == 0) {                                                                      \
		type old_val = nv_conf->opt_name;                                                                     \
		nv_conf->opt_name = (type)strtol(opt->value, NULL, 10);                                               \
		blog(LOG_DEBUG, "[obs-nvenc] Changing parameter: \"%s\": %" format " -> %" format " (%s)", #opt_name, \
		     old_val, nv_conf->opt_name, #type);                                                              \
		return true;                                                                                          \
	}

static void parse_qp_opt(const char *name, const char *val, NV_ENC_QP *qp_opt)
{
	/* QP options can be passed in either as a single value to apply to all
	 * or as three values separated by ":". */
	int32_t p, b, i;

	if (sscanf(val, "%d:%d:%d", &p, &b, &i) != 3) {
		p = b = i = atoi(val);
	}

	blog(LOG_DEBUG, "[obs-nvenc] Applying custom %s = %d / %d / %d (P / B / I)", name, p, b, i);

	/* Values should be treated as int32_t but are passed in as uint32_t
	 * for legacy reasons, see comment in nvEncodeAPI.h */
	qp_opt->qpInterP = (uint32_t)p;
	qp_opt->qpInterB = (uint32_t)b;
	qp_opt->qpIntra = (uint32_t)i;
}

#define APPLY_QP_OPT(opt_name)                                           \
	if (strcmp(opt->name, #opt_name) == 0) {                         \
		parse_qp_opt(#opt_name, opt->value, &nv_conf->opt_name); \
		return true;                                             \
	}

static bool apply_rc_opt(const struct obs_option *opt, NV_ENC_RC_PARAMS *nv_conf)
{
	APPLY_QP_OPT(constQP)
	APPLY_QP_OPT(minQP)
	APPLY_QP_OPT(maxQP)
	APPLY_QP_OPT(initialRCQP)

	APPLY_INT_OPT(averageBitRate, uint32_t, PRIu32)
	APPLY_INT_OPT(maxBitRate, uint32_t, PRIu32)
	APPLY_INT_OPT(vbvBufferSize, uint32_t, PRIu32)
	APPLY_INT_OPT(vbvInitialDelay, uint32_t, PRIu32)

	APPLY_INT_OPT(targetQuality, uint8_t, PRIu8)
	APPLY_INT_OPT(targetQualityLSB, uint8_t, PRIu8)

	APPLY_INT_OPT(cbQPIndexOffset, int8_t, PRIi8)
	APPLY_INT_OPT(crQPIndexOffset, int8_t, PRIi8)

	APPLY_BIT_OPT(enableMinQP, 1)
	APPLY_BIT_OPT(enableMaxQP, 1)
	APPLY_BIT_OPT(enableInitialRCQP, 1)
	APPLY_BIT_OPT(enableAQ, 1)
	APPLY_BIT_OPT(enableLookahead, 1)
	APPLY_BIT_OPT(disableIadapt, 1)
	APPLY_BIT_OPT(disableBadapt, 1)
	APPLY_BIT_OPT(enableTemporalAQ, 1)
	APPLY_BIT_OPT(aqStrength, 4)

#ifdef NVENC_12_2_OR_LATER
	APPLY_INT_OPT(lookaheadLevel, NV_ENC_LOOKAHEAD_LEVEL, PRIu32)
#endif

	/* Macros above will return true if succesfully evaluated.
	 * Otherwise, return false if option unknown/unsupported. */
	return false;
}

static bool apply_conf_opt(const struct obs_option *opt, NV_ENC_CONFIG *nv_conf)
{
	APPLY_INT_OPT(gopLength, uint32_t, PRIu32)
	APPLY_INT_OPT(frameIntervalP, int32_t, PRIi32)

	return false;
}

static void parse_level_opt(const char *val, uint32_t *level, bool hevc)
{
	/* Support for passing level both as raw value (e.g. "42")
	 * and human-readable format (e.g. "4.2"). */
	uint32_t int_val = 0;

	if (strstr(val, ".") != NULL) {
		uint32_t high_val, low_val;
		if (sscanf(val, "%u.%u", &high_val, &low_val) == 2) {
			int_val = high_val * 10 + low_val;
		}
	} else {
		int_val = strtol(val, NULL, 10);
	}

	if (!int_val)
		return;

	if (hevc)
		int_val *= 3;

	blog(LOG_DEBUG, "[obs-nvenc] Applying custom level = %s (%u)", val, int_val);
	*level = int_val;
}

static bool apply_h264_opt(struct obs_option *opt, NV_ENC_CONFIG_H264 *nv_conf)
{
	if (strcmp(opt->name, "level") == 0) {
		parse_level_opt(opt->value, &nv_conf->level, false);
		return true;
	}

	APPLY_INT_OPT(idrPeriod, uint32_t, PRIu32)
	APPLY_INT_OPT(useBFramesAsRef, NV_ENC_BFRAME_REF_MODE, PRIu32)

	APPLY_BIT_OPT(enableFillerDataInsertion, 1)

	return false;
}

static bool apply_hevc_opt(struct obs_option *opt, NV_ENC_CONFIG_HEVC *nv_conf)
{
	if (strcmp(opt->name, "level") == 0) {
		parse_level_opt(opt->value, &nv_conf->level, true);
		return true;
	}

	APPLY_INT_OPT(tier, uint32_t, PRIu32)
	APPLY_INT_OPT(idrPeriod, uint32_t, PRIu32)
	APPLY_INT_OPT(useBFramesAsRef, NV_ENC_BFRAME_REF_MODE, PRIu32)
#ifdef NVENC_12_2_OR_LATER
	APPLY_INT_OPT(tfLevel, NV_ENC_TEMPORAL_FILTER_LEVEL, PRIu32)
#endif

	APPLY_BIT_OPT(enableFillerDataInsertion, 1)

	return false;
}

static bool apply_av1_opt(struct obs_option *opt, NV_ENC_CONFIG_AV1 *nv_conf)
{
	APPLY_INT_OPT(level, uint32_t, PRIu32)
	APPLY_INT_OPT(tier, uint32_t, PRIu32)
	APPLY_INT_OPT(numTileColumns, uint32_t, PRIu32)
	APPLY_INT_OPT(numTileRows, uint32_t, PRIu32)
	APPLY_INT_OPT(idrPeriod, uint32_t, PRIu32)
	APPLY_INT_OPT(useBFramesAsRef, NV_ENC_BFRAME_REF_MODE, PRIu32)

	APPLY_BIT_OPT(enableBitstreamPadding, 1)

	return false;
}

static bool apply_codec_opt(enum codec_type codec, struct obs_option *opt, NV_ENC_CODEC_CONFIG *enc_config)
{
	if (codec == CODEC_H264)
		return apply_h264_opt(opt, &enc_config->h264Config);
	if (codec == CODEC_HEVC)
		return apply_hevc_opt(opt, &enc_config->hevcConfig);
	if (codec == CODEC_AV1)
		return apply_av1_opt(opt, &enc_config->av1Config);

	return false;
}

bool apply_user_args(struct nvenc_data *enc)
{
	bool success = true;

	for (size_t idx = 0; idx < enc->props.opts.count; idx++) {
		struct obs_option *opt = &enc->props.opts.options[idx];

		/* Special options handled elsewhere */
		if (strcmp(opt->name, "lookaheadDepth") == 0 || strcmp(opt->name, "keyint") == 0)
			continue;

		if (apply_rc_opt(opt, &enc->config.rcParams))
			continue;
		if (apply_conf_opt(opt, &enc->config))
			continue;
		if (apply_codec_opt(enc->codec, opt, &enc->config.encodeCodecConfig))
			continue;

		warn("Unknown custom option: \"%s\"", opt->name);
		success = false;
	}

	return success;
}

bool get_user_arg_int(struct nvenc_data *enc, const char *name, int *val)
{
	for (size_t idx = 0; idx < enc->props.opts.count; idx++) {
		struct obs_option *opt = &enc->props.opts.options[idx];
		if (strcmp(opt->name, name) != 0)
			continue;

		*val = strtol(opt->value, NULL, 10);
		return true;
	}

	return false;
}
