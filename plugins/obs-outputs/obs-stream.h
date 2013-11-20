#pragma once

#include "util/c99defs.h"
#include "obs.h"

struct rtmp_stream {
	obs_output_t handler;
};

EXPORT void *rtmp_stream_getname(const char *locale);
EXPORT void *rtmp_stream_create(const char *settings, obs_output_t output);
EXPORT void *rtmp_stream_destroy(void *data);
EXPORT void *rtmp_stream_update(void *data, const char *settings);
EXPORT void rtmp_stream_start(void *data);
EXPORT void rtmp_stream_stop(void *data);
