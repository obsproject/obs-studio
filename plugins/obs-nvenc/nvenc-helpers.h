#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <obs-module.h>
#include <ffnvcodec/nvEncodeAPI.h>

#define NVCODEC_CONFIGURED_VERSION ((NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION)

#if NVENCAPI_MAJOR_VERSION > 12 || NVENCAPI_MINOR_VERSION >= 1
#define NVENC_12_1_OR_LATER
#endif

#if NVENCAPI_MAJOR_VERSION > 12 || NVENCAPI_MINOR_VERSION >= 2
#define NVENC_12_2_OR_LATER
#endif

#if NVENCAPI_MAJOR_VERSION >= 13
#define NVENC_13_0_OR_LATER
#endif

enum codec_type {
	CODEC_H264,
	CODEC_HEVC,
	CODEC_AV1,
};

static const char *get_codec_name(enum codec_type type)
{
	switch (type) {
	case CODEC_H264:
		return "H264";
	case CODEC_HEVC:
		return "HEVC";
	case CODEC_AV1:
		return "AV1";
	}

	return "Unknown";
}

struct encoder_caps {
	int bframes;
	int bref_modes;
	int engines;

	int max_width;
	int max_height;

	int temporal_filter; /* Broken prior to the 551.21 driver. */
	int lookahead_level; /* Broken prior to the 570.20 driver. */

	bool dyn_bitrate;
	bool lookahead;
	bool lossless;
	bool temporal_aq;
	bool uhq;

	/* Yeah... */
	bool ten_bit;
	bool four_four_four;
	bool four_two_two;
};

typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(NV_ENCODE_API_FUNCTION_LIST *);

extern NV_ENCODE_API_FUNCTION_LIST nv;
extern NV_CREATE_INSTANCE_FUNC nv_create_instance;

const char *nv_error_name(NVENCSTATUS err);

bool init_nvenc(obs_encoder_t *encoder);
bool nv_fail2(obs_encoder_t *encoder, void *session, const char *format, ...);
bool nv_failed2(obs_encoder_t *encoder, void *session, NVENCSTATUS err, const char *func, const char *call);

struct encoder_caps *get_encoder_caps(enum codec_type codec);
int num_encoder_devices(void);
bool is_codec_supported(enum codec_type codec);
bool has_broken_split_encoding(void);

void register_encoders(void);
void register_compat_encoders(void);

#define nv_fail(encoder, format, ...) nv_fail2(encoder, enc->session, format, ##__VA_ARGS__)

#define nv_failed(encoder, err, func, call) nv_failed2(encoder, enc->session, err, func, call)
