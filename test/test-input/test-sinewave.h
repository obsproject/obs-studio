#pragma once

#include <util/bmem.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sinewave_data {
	bool         initialized_thread;
	pthread_t    thread;
	event_t      event;
	obs_source_t source;
};

EXPORT const char *sinewave_getname(const char *locale);

EXPORT struct sinewave_data *sinewave_create(const char *settings,
		obs_source_t source);
EXPORT void sinewave_destroy(struct sinewave_data *swd);
EXPORT uint32_t sinewave_get_output_flags(struct sinewave_data *swd);

#ifdef __cplusplus
}
#endif
