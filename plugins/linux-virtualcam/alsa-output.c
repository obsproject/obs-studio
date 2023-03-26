/*
Copyright (C) 2022 DEV47APPS, github.com/dev47apps

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define _GNU_SOURCE
#include <obs-module.h>
#include <alsa/asoundlib.h>

#include "linux-virtualcam.h"

struct alsa_output_data {
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_sframes_t period_size;
	snd_pcm_sframes_t buffer_size;
};

static bool loopback_module_loaded()
{
	return module_loaded("snd_aloop");
}

bool audio_possible()
{
	return access("/sys/module/snd_aloop", F_OK) == 0;
}

static int loopback_module_load()
{
	return run_command("pkexec modprobe snd_aloop && sleep 0.5");
}

static inline snd_pcm_format_t to_alsa_format(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_U8BIT:
		return SND_PCM_FORMAT_U8;
	case AUDIO_FORMAT_16BIT:
		return SND_PCM_FORMAT_S16;
	case AUDIO_FORMAT_32BIT:
		return SND_PCM_FORMAT_S32;
	case AUDIO_FORMAT_FLOAT:
		return SND_PCM_FORMAT_FLOAT;
	default:
		return SND_PCM_FORMAT_UNKNOWN;
	}
}

static inline enum audio_format to_obs_format(snd_pcm_format_t format)
{
	switch (format) {
	case SND_PCM_FORMAT_U8:
		return AUDIO_FORMAT_U8BIT;
	case SND_PCM_FORMAT_S16:
		return AUDIO_FORMAT_16BIT;
	case SND_PCM_FORMAT_S32:
		return AUDIO_FORMAT_32BIT;
	case SND_PCM_FORMAT_FLOAT:
		return AUDIO_FORMAT_FLOAT;
	default:
		return AUDIO_FORMAT_UNKNOWN;
	}
}

static int set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *params,
			unsigned channels, unsigned sample_rate,
			snd_pcm_format_t format, snd_pcm_access_t access,
			snd_pcm_sframes_t *period_size,
			snd_pcm_sframes_t *buffer_size)
{
	int rc, dir;
	snd_pcm_uframes_t size;

	/* choose all parameters */
	rc = snd_pcm_hw_params_any(handle, params);
	if (rc < 0) {
		blog(LOG_WARNING, "snd configuration not available");
		return rc;
	}

	/* set hardware resampling */
	int resample = 1;
	rc = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
	if (rc < 0) {
		blog(LOG_WARNING, "resample setup failed");
		return rc;
	}

	/* set the interleaved read/write format */
	rc = snd_pcm_hw_params_set_access(handle, params, access);
	if (rc < 0) {
		blog(LOG_WARNING, "access mode not available");
		return rc;
	}

	/* set the sample format */
	rc = snd_pcm_hw_params_set_format(handle, params, format);
	if (rc < 0) {
		blog(LOG_WARNING, "sample format not available");
		return rc;
	}

	/* set the channels */
	rc = snd_pcm_hw_params_set_channels(handle, params, channels);
	if (rc < 0) {
		blog(LOG_WARNING, "Channels count (%u) not available",
		     channels);
		return rc;
	}

	/* set the sampling rate */
	unsigned int rate = sample_rate;
	rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
	if (rc < 0) {
		blog(LOG_WARNING, "Rate %uHz not available", sample_rate);
		return rc;
	}

	if (sample_rate != rate) {
		blog(LOG_WARNING, "Rate doesn't match, want:%uHz, got %uHz",
		     sample_rate, rate);
		return -EINVAL;
	}

	/* set the period time in us */
	unsigned period_time = 1000 * 1000 * AUDIO_OUTPUT_FRAMES / sample_rate;
	rc = snd_pcm_hw_params_set_period_time_near(handle, params,
						    &period_time, &dir);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set period time %u", period_time);
		return rc;
	}

	rc = snd_pcm_hw_params_get_period_size(params, &size, &dir);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to get period size");
		return rc;
	}
	*period_size = size;

	blog(LOG_DEBUG, "period_size=%ld", size);

	/* ring buffer length in us */
	unsigned buffer_time = period_time * 4;
	rc = snd_pcm_hw_params_set_buffer_time_near(handle, params,
						    &buffer_time, &dir);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set buffer time");
		return rc;
	}

	rc = snd_pcm_hw_params_get_buffer_size(params, &size);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to get buffer size");
		return rc;
	}
	*buffer_size = size;

	blog(LOG_DEBUG, "buffer_size=%ld", size);

	/* write the parameters to device */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set hwparams");
		return rc;
	}

	return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams,
			snd_pcm_sframes_t period_size,
			snd_pcm_sframes_t buffer_size)
{
	int rc;

	UNUSED_PARAMETER(period_size);
	UNUSED_PARAMETER(buffer_size);

	rc = snd_pcm_sw_params_current(handle, swparams);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to determine current swparams");
		return rc;
	}

	rc = snd_pcm_sw_params_set_start_threshold(handle, swparams,
						   period_size * 2);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set start threshold");
		return rc;
	}

	rc = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set avail min");
		return rc;
	}

	rc = snd_pcm_sw_params(handle, swparams);
	if (rc < 0) {
		blog(LOG_WARNING, "Unable to set swparams");
		return rc;
	}

	return 0;
}

void *audio_start(obs_output_t *output)
{
	if (!loopback_module_loaded()) {
		/* clang-format off */

		// Should we load snd_aloop automatically?
		// Given how finicky Linux audio is, lets not.
		// Avoid messing with peoples audio.

		#if 0
		if (loopback_module_load() != 0)
			return NULL;
		#else
		blog(LOG_INFO, "ALSA Loopback module is not loaded");
		return NULL;
		#endif

		/* clang-format on */
	}

	audio_t *audio = obs_output_audio(output);
	const struct audio_output_info *aoi = audio_output_get_info(audio);
	struct audio_convert_info aci = {0};

	size_t channels = audio_output_get_channels(audio);
	uint32_t sample_rate = (int)audio_output_get_sample_rate(audio);
	snd_pcm_format_t alsa_fromat = to_alsa_format(aoi->format);

	if (alsa_fromat == SND_PCM_FORMAT_UNKNOWN) {
		alsa_fromat = to_alsa_format(AUDIO_FORMAT_16BIT);
		aci.format = AUDIO_FORMAT_16BIT;
		aci.samples_per_sec = sample_rate;
		aci.speakers = aoi->speakers;
	}

	snd_pcm_sframes_t period_size;
	snd_pcm_sframes_t buffer_size;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;

	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);

	for (int i = 0; i < 8; i++) {
		int rc;
		bool success = true;
		char device[32];

		snprintf(device, sizeof(device), "hw:Loopback,0,%d", i);
		blog(LOG_DEBUG, "alsa-output: trying device: '%s'", device);

		snd_pcm_t *handle = NULL;
		rc = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK,
				  SND_PCM_NONBLOCK);

		if (rc < 0 || !handle) {
			blog(LOG_ERROR, "snd_pcm_open failed: %s",
			     snd_strerror(rc));
			continue;
		}

		if ((rc = set_hwparams(handle, hwparams, channels, sample_rate,
				       alsa_fromat,
				       SND_PCM_ACCESS_MMAP_INTERLEAVED,
				       &period_size, &buffer_size)) < 0) {

			blog(LOG_ERROR, "alsa-output: hwparams failed: %s",
			     snd_strerror(rc));

			success = false;
		}

		else if ((rc = set_swparams(handle, swparams, period_size,
					    buffer_size)) < 0) {

			blog(LOG_ERROR, "alsa-output: swparams failed: %s",
			     snd_strerror(rc));

			success = false;
		}

		if (success) {
			struct alsa_output_data *vcam =
				(struct alsa_output_data *)bzalloc(
					sizeof(*vcam));

			vcam->handle = handle;
			vcam->hwparams = hwparams;
			vcam->swparams = swparams;
			vcam->period_size = period_size;
			vcam->buffer_size = buffer_size;

			if (aci.format != 0)
				obs_output_set_audio_conversion(output, &aci);

			snprintf(device, sizeof(device), "hw:Loopback,1,%d", i);
			blog(LOG_INFO, "ALSA output: %s", device);
			return vcam;
		}

		snd_pcm_close(handle);
	}

	blog(LOG_WARNING, "Failed to start ALSA Loopback output");
	return NULL;
}

void audio_stop(void *data)
{
	if (!data)
		return;

	struct alsa_output_data *vcam = (struct alsa_output_data *)data;
	snd_pcm_close(vcam->handle);

	bfree(data);
}

void virtual_audio(void *data, struct audio_data *frame)
{
	struct alsa_output_data *vcam = (struct alsa_output_data *)data;
	int rc = snd_pcm_mmap_writei(vcam->handle, frame->data[0],
				     frame->frames);

	if (rc == -EPIPE) { /* under-run */
		rc = snd_pcm_prepare(vcam->handle);
		UNUSED_PARAMETER(rc);
		return;
	}

	if (rc == -ESTRPIPE) {
		rc = snd_pcm_resume(vcam->handle);
		if (rc == -EAGAIN)
			return; /* wait until the suspend flag is released */

		if (rc < 0) {
			rc = snd_pcm_prepare(vcam->handle);
			UNUSED_PARAMETER(rc);
		}
	}
}
