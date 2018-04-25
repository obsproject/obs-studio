#pragma once

#include "vaapi-common.h"
#include "vaapi-display.h"

struct vaapi_encoder;
typedef struct vaapi_encoder vaapi_encoder_t;
struct vaapi_profile_caps;
typedef struct vaapi_profile_caps vaapi_profile_caps_t;
struct encoder_frame;

enum vaapi_profile {
	VAAPI_PROFILE_NONE = 0,
	VAAPI_PROFILE_BASELINE_CONSTRAINED = 1,
	VAAPI_PROFILE_BASELINE = 2,
	VAAPI_PROFILE_MAIN = 3,
	VAAPI_PROFILE_HIGH = 4
};
typedef enum vaapi_profile vaapi_profile_t;

enum vaapi_format {
	VAAPI_FORMAT_NONE,
	VAAPI_FORMAT_YUV420,
	VAAPI_FORMAT_YUV422,
	VAAPI_FORMAT_YUV444,
};
typedef enum vaapi_format vaapi_format_t;

enum vaapi_rc {
	VAAPI_RC_NONE,
	VAAPI_RC_CBR,
	VAAPI_RC_CQP,
	VAAPI_RC_VBR,
	VAAPI_RC_VBR_CONSTRAINED
};
typedef enum vaapi_rc vaapi_rc_t;

typedef void(*vaapi_coded_block_cb)(void *opaque, coded_block_entry_t *entry);

struct vaapi_encoder_attribs
{
	vaapi_display_t *display;
	vaapi_profile_t profile;
	vaapi_format_t format;
	uint32_t width;
	uint32_t height;
	uint32_t bitrate;
	vaapi_rc_t rc_type;
	uint32_t min_qp;
	uint32_t qp;
	uint32_t max_qp_delta;

	uint32_t framerate_num;
	uint32_t framerate_den;
	uint32_t keyint;
	uint32_t refpic_cnt;
	bool use_custom_cpb;
	uint32_t cpb_size;
	uint32_t cpb_window_ms;

	uint32_t surface_cnt;
	void *coded_block_cb_opaque;
	vaapi_coded_block_cb coded_block_cb;
};
typedef struct vaapi_encoder_attribs vaapi_encoder_attribs_t;

// Lock on display
vaapi_encoder_t *vaapi_encoder_create(vaapi_encoder_attribs_t *attribs);
void vaapi_encoder_destroy(vaapi_encoder_t *encoder);
bool vaapi_encoder_encode(vaapi_encoder_t *enc,
		struct encoder_frame *frame);

// Do not lock on display
const char *vaapi_encoder_vendor(vaapi_encoder_t *enc);
bool vaapi_encoder_extra_data(vaapi_encoder_t *enc,
		uint8_t **extra_data, size_t *extra_data_size);

