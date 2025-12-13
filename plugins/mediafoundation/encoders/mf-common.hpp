#pragma once

#include <obs-module.h>

#include <mfapi.h>
#include <functional>
#include <comdef.h>
#include <chrono>

#ifndef CHECK_HR_ERROR
#define CHECK_HR_ERROR(r)                                 \
	if (FAILED(hr = (r))) {                \
		MF_LOG_COM(LOG_ERROR, #r, hr); \
		goto fail;                     \
	}
#endif

#ifndef CHECK_HR_LEVEL
#define CHECK_HR_LEVEL(level, r)                 \
	if (FAILED(hr = (r))) {            \
		MF_LOG_COM(level, #r, hr); \
		goto fail;                 \
	}
#endif

#ifndef CHECK_HR_WARNING
#define CHECK_HR_WARNING(r)                \
	if (FAILED(hr = (r))) \
		MF_LOG_COM(LOG_WARNING, #r, hr);
#endif

namespace MF {
enum Status { FAILURE, SUCCESS, NOT_ACCEPTING, NEED_MORE_INPUT };

bool LogMediaType(IMFMediaType *mediaType);
void MF_LOG(int level, const char *format, ...);
void MF_LOG_COM(int level, const char *msg, HRESULT hr);
} // namespace MF
