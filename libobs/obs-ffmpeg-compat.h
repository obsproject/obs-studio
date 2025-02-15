#pragma once

#include <libavcodec/avcodec.h>

/* LIBAVCODEC_VERSION_CHECK checks for the right version of libav and FFmpeg
 * a is the major version
 * b and c the minor and micro versions of libav
 * d and e the minor and micro versions of FFmpeg */
#define LIBAVCODEC_VERSION_CHECK(a, b, c, d, e)                                                   \
	((LIBAVCODEC_VERSION_MICRO < 100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(a, b, c)) || \
	 (LIBAVCODEC_VERSION_MICRO >= 100 && LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(a, d, e)))

#define INPUT_BUFFER_PADDING_SIZE AV_INPUT_BUFFER_PADDING_SIZE
