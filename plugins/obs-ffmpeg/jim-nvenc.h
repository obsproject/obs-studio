#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <obs-module.h>
#include "nvEncodeAPI.h"

#define do_log(level, format, ...) \
	blog(level, "[jim-nvenc] " format, ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR,   format, ##__VA_ARGS__)
#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

typedef NVENCSTATUS (NVENCAPI *NV_CREATE_INSTANCE_FUNC)(NV_ENCODE_API_FUNCTION_LIST*);

extern const char *nv_error_name(NVENCSTATUS err);
extern NV_ENCODE_API_FUNCTION_LIST nv;
extern NV_CREATE_INSTANCE_FUNC nv_create_instance;
extern bool init_nvenc(void);

static inline bool nv_failed(NVENCSTATUS err, const char *func,
		const char *call)
{
	if (err == NV_ENC_SUCCESS)
		return false;

	error("%s: %s failed: %d (%s)", func, call, (int)err,
			nv_error_name(err));
	return true;
}

#define NV_FAILED(x) nv_failed(x, __FUNCTION__, #x)
