#include <stdlib.h>
#include <util/threading.h>
#include <util/platform.h>
#include <obs.h>

struct async_late_test {
	obs_source_t *source;
	os_event_t   *stop_signal;
	pthread_t    thread;
	bool         initialized;
};

/* middle C */
static const double rate = 261.63 / 48000.0;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define M_PI_X2 M_PI*2

static const char *ast_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Late Async Audio Source (causes audio buffering)";
}

static void ast_destroy(void *data)
{
	struct async_late_test *ctx = data;

	if (ctx->initialized) {
		os_event_signal(ctx->stop_signal);
		pthread_join(ctx->thread, NULL);
	}

	os_event_destroy(ctx->stop_signal);
	bfree(ctx);
}

static void *audio_thread(void *data)
{
	struct async_late_test *ctx = data;

	uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());

	float    *samples = bmalloc(sample_rate * sizeof(float));
	uint64_t cur_time = os_gettime_ns();
	bool     whitelist = false;
	double   cos_val = 0.0;
	uint64_t start_time = cur_time;

	struct obs_source_audio audio = {
		.speakers = SPEAKERS_MONO,
		.data = { [0] = (uint8_t*)samples },
		.samples_per_sec = sample_rate,
		.frames = sample_rate,
		.format = AUDIO_FORMAT_FLOAT
	};

	while (os_event_try(ctx->stop_signal) == EAGAIN) {
		//audio.timestamp = (cur_time - start_time) - 1000000000ULL;
		audio.timestamp = os_gettime_ns() - 1000000000ULL;

		if (whitelist) {
			for (size_t i = 0; i < sample_rate; i++) {
				cos_val += rate * M_PI_X2;
				if (cos_val > M_PI_X2)
					cos_val -= M_PI_X2;

				samples[i] = (float)(cos(cos_val) * 0.5);
			}
		}
		else {
			for (size_t i = 0; i < sample_rate; i++)
				samples[i] = 0.0f;
		}

		obs_source_output_audio(ctx->source, &audio);

		os_sleepto_ns(cur_time += 1000000000);

		whitelist = !whitelist;
	}

	bfree(samples);

	return NULL;
}

static void *ast_create(obs_data_t *settings, obs_source_t *source)
{
	struct async_late_test *ctx = bzalloc(sizeof(struct async_late_test));
	ctx->source = source;

	if (os_event_init(&ctx->stop_signal, OS_EVENT_TYPE_MANUAL) != 0) {
		ast_destroy(ctx);
		return NULL;
	}

	if (pthread_create(&ctx->thread, NULL, audio_thread, ctx) != 0) {
		ast_destroy(ctx);
		return NULL;
	}

	ctx->initialized = true;

	UNUSED_PARAMETER(settings);
	UNUSED_PARAMETER(source);
	return ctx;
}

struct obs_source_info async_late_test = {
	.id = "async_late_test",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC | OBS_SOURCE_AUDIO,
	.get_name = ast_getname,
	.create = ast_create,
	.destroy = ast_destroy,
};
