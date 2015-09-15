#pragma once

#include <obs-module.h>

#include <mfapi.h>
#include <functional>
#include <comdef.h>
#include <chrono>


#ifndef MF_LOG
#define MF_LOG(level, format, ...) \
	blog(level, "[Media Foundation encoder]: " format, ##__VA_ARGS__)
#endif
#ifndef MF_LOG_ENCODER
#define MF_LOG_ENCODER(format_name, encoder, level, format, ...) \
	blog(level, "[Media Foundation %s: '%s']: " format, \
			format_name, obs_encoder_get_name(encoder), \
			##__VA_ARGS__)
#endif

#ifndef MF_LOG_COM
#define MF_LOG_COM(level, msg, hr) MF_LOG(level, \
		msg " failed,  %S (0x%08lx)", \
		_com_error(hr).ErrorMessage(), hr)
#endif

#ifndef HRC
#define HRC(r) \
	if(FAILED(hr = (r))) { \
		MF_LOG_COM(LOG_ERROR, #r, hr); \
		goto fail; \
	}
#endif

#ifndef HR_CHECK
#define HR_CHECK(level, r) \
	if(FAILED(hr = (r))) { \
		MF_LOG_COM(level, #r, hr); \
		goto fail; \
		}
#endif

#ifndef HRL
#define HRL(r) \
	if(FAILED(hr = (r))) \
		MF_LOG_COM(LOG_WARNING, #r, hr);
#endif

namespace MF {

enum Status {
	FAILURE,
	SUCCESS,
	NOT_ACCEPTING,
	NEED_MORE_INPUT
};

bool LogMediaType(IMFMediaType *mediaType);

}
