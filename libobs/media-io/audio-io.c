/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "../util/threading.h"
#include "../util/darray.h"
#include "../util/platform.h"

#include "audio-io.h"

/* TODO: Incomplete */

struct audio_output {
	struct audio_info info;
	media_t           media;
	media_output_t    output;

	pthread_t         thread;
	pthread_mutex_t   data_mutex;
	event_t           stop_event;

	struct darray     pending_frames;

	bool              initialized;
};

/* ------------------------------------------------------------------------- */

static void *audio_thread(void *param)
{
	struct audio_output *audio = param;

	while (event_try(&audio->stop_event) == EAGAIN) {
		os_sleep_ms(5);
		/* TODO */
	}

	return NULL;
}

/* ------------------------------------------------------------------------- */

static inline bool valid_audio_params(struct audio_info *info)
{
	return info->channels > 0 && info->format && info->name &&
		info->samples_per_sec > 0 && info->speakers > 0;
}

static inline bool ao_add_to_media(audio_t audio)
{
	struct media_output_info oi;
	oi.format  = audio->info.format;
	oi.obj     = audio;
	oi.connect = NULL;

	audio->output = media_output_create(&oi);
	if (!audio->output)
		return false;

	media_add_output(audio->media, audio->output);
	return true;
}

int audio_output_open(audio_t *audio, media_t media, struct audio_info *info)
{
	struct audio_output *out;

	if (!valid_audio_params(info))
		return AUDIO_OUTPUT_INVALIDPARAM;

	out = bmalloc(sizeof(struct audio_output));
	memset(out, 0, sizeof(struct audio_output));

	memcpy(&out->info, info, sizeof(struct audio_info));
	out->media = media;

	if (pthread_mutex_init(&out->data_mutex, NULL) != 0)
		goto fail;
	if (event_init(&out->stop_event, true) != 0)
		goto fail;
	if (!ao_add_to_media(out))
		goto fail;
	if (pthread_create(&out->thread, NULL, audio_thread, out) != 0)
		goto fail;

	out->initialized = true;
	*audio = out;
	return AUDIO_OUTPUT_SUCCESS;

fail:
	audio_output_close(out);
	return AUDIO_OUTPUT_FAIL;
}

void audio_output_data(audio_t audio, struct audio_data *data)
{
	pthread_mutex_lock(&audio->data_mutex);
	/* TODO */
	pthread_mutex_unlock(&audio->data_mutex);
}

void audio_output_close(audio_t audio)
{
	void *thread_ret;

	if (!audio)
		return;

	if (audio->initialized) {
		event_signal(&audio->stop_event);
		pthread_join(audio->thread, &thread_ret);
	}

	media_remove_output(audio->media, audio->output);
	event_destroy(&audio->stop_event);
	pthread_mutex_destroy(&audio->data_mutex);
	bfree(audio);
}
