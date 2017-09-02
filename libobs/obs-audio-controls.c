/*
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>

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

#include <math.h>

#include "util/threading.h"
#include "util/bmem.h"
#include "media-io/audio-math.h"
#include "obs.h"
#include "obs-internal.h"

#include "obs-audio-controls.h"

/* These are pointless warnings generated not by our code, but by a standard
 * library macro, INFINITY */
#ifdef _MSC_VER
#pragma warning(disable : 4056)
#pragma warning(disable : 4756)
#endif

typedef float (*obs_fader_conversion_t)(const float val);

struct fader_cb {
	obs_fader_changed_t   callback;
	void                  *param;
};

struct obs_fader {
	pthread_mutex_t        mutex;
	obs_fader_conversion_t def_to_db;
	obs_fader_conversion_t db_to_def;
	obs_source_t           *source;
	enum obs_fader_type    type;
	float                  max_db;
	float                  min_db;
	float                  cur_db;
	bool                   ignore_next_signal;

	pthread_mutex_t        callback_mutex;
	DARRAY(struct fader_cb)callbacks;
};

struct meter_cb {
	obs_volmeter_updated_t callback;
	void                   *param;
};

struct obs_volmeter {
	pthread_mutex_t        mutex;
	obs_fader_conversion_t pos_to_db;
	obs_fader_conversion_t db_to_pos;
	obs_source_t           *source;
	enum obs_fader_type    type;
	float                  cur_db;

	pthread_mutex_t        callback_mutex;
	DARRAY(struct meter_cb)callbacks;

	unsigned int           channels;
	unsigned int           update_ms;
	unsigned int           update_frames;
	unsigned int           peakhold_ms;
	unsigned int           peakhold_frames;

	unsigned int           peakhold_count;
	unsigned int           ival_frames;
	float                  ival_sum;
	float                  ival_max;

	float                  vol_peak;
	float                  vol_mag;
	float                  vol_max;
};

static float cubic_def_to_db(const float def)
{
	if (def == 1.0f)
		return 0.0f;
	else if (def <= 0.0f)
		return -INFINITY;

	return mul_to_db(def * def * def);
}

static float cubic_db_to_def(const float db)
{
	if (db == 0.0f)
		return 1.0f;
	else if (db == -INFINITY)
		return 0.0f;

	return cbrtf(db_to_mul(db));
}

static float iec_def_to_db(const float def)
{
	if (def == 1.0f)
		return 0.0f;
	else if (def <= 0.0f)
		return -INFINITY;

	float db;

	if (def >= 0.75f)
		db = (def - 1.0f) / 0.25f * 9.0f;
	else if (def >= 0.5f)
		db = (def - 0.75f) / 0.25f * 11.0f - 9.0f;
	else if (def >= 0.3f)
		db = (def - 0.5f) / 0.2f * 10.0f - 20.0f;
	else if (def >= 0.15f)
		db = (def - 0.3f) / 0.15f * 10.0f - 30.0f;
	else if (def >= 0.075f)
		db = (def - 0.15f) / 0.075f * 10.0f - 40.0f;
	else if (def >= 0.025f)
		db = (def - 0.075f) / 0.05f * 10.0f - 50.0f;
	else if (def >= 0.001f)
		db = (def - 0.025f) / 0.025f * 90.0f - 60.0f;
	else
		db = -INFINITY;

	return db;
}

static float iec_db_to_def(const float db)
{
	if (db == 0.0f)
		return 1.0f;
	else if (db == -INFINITY)
		return 0.0f;

	float def;

	if (db >= -9.0f)
		def = (db + 9.0f) / 9.0f * 0.25f + 0.75f;
	else if (db >= -20.0f)
		def = (db + 20.0f) / 11.0f * 0.25f + 0.5f;
	else if (db >= -30.0f)
		def = (db + 30.0f) / 10.0f * 0.2f + 0.3f;
	else if (db >= -40.0f)
		def = (db + 40.0f) / 10.0f * 0.15f + 0.15f;
	else if (db >= -50.0f)
		def = (db + 50.0f) / 10.0f * 0.075f + 0.075f;
	else if (db >= -60.0f)
		def = (db + 60.0f) / 10.0f * 0.05f + 0.025f;
	else if (db >= -114.0f)
		def = (db + 150.0f) / 90.0f * 0.025f;
	else
		def = 0.0f;

	return def;
}

#define LOG_OFFSET_DB  6.0f
#define LOG_RANGE_DB   96.0f
/* equals -log10f(LOG_OFFSET_DB) */
#define LOG_OFFSET_VAL -0.77815125038364363f
/* equals -log10f(-LOG_RANGE_DB + LOG_OFFSET_DB) */
#define LOG_RANGE_VAL  -2.00860017176191756f

static float log_def_to_db(const float def)
{
	if (def >= 1.0f)
		return 0.0f;
	else if (def <= 0.0f)
		return -INFINITY;

	return -(LOG_RANGE_DB + LOG_OFFSET_DB) * powf(
			(LOG_RANGE_DB + LOG_OFFSET_DB) / LOG_OFFSET_DB, -def)
			+ LOG_OFFSET_DB;
}

static float log_db_to_def(const float db)
{
	if (db >= 0.0f)
		return 1.0f;
	else if (db <= -96.0f)
		return 0.0f;

	return (-log10f(-db + LOG_OFFSET_DB) - LOG_RANGE_VAL)
			/ (LOG_OFFSET_VAL - LOG_RANGE_VAL);
}

static void signal_volume_changed(struct obs_fader *fader, const float db)
{
	pthread_mutex_lock(&fader->callback_mutex);
	for (size_t i = fader->callbacks.num; i > 0; i--) {
		struct fader_cb cb = fader->callbacks.array[i - 1];
		cb.callback(cb.param, db);
	}
	pthread_mutex_unlock(&fader->callback_mutex);
}

static void signal_levels_updated(struct obs_volmeter *volmeter,
		const float level, const float magnitude, const float peak,
		bool muted)
{
	pthread_mutex_lock(&volmeter->callback_mutex);
	for (size_t i = volmeter->callbacks.num; i > 0; i--) {
		struct meter_cb cb = volmeter->callbacks.array[i - 1];
		cb.callback(cb.param, level, magnitude, peak, muted);
	}
	pthread_mutex_unlock(&volmeter->callback_mutex);
}

static void fader_source_volume_changed(void *vptr, calldata_t *calldata)
{
	struct obs_fader *fader = (struct obs_fader *) vptr;

	pthread_mutex_lock(&fader->mutex);

	if (fader->ignore_next_signal) {
		fader->ignore_next_signal = false;
		pthread_mutex_unlock(&fader->mutex);
		return;
	}

	const float mul      = (float)calldata_float(calldata, "volume");
	const float db       = mul_to_db(mul);
	fader->cur_db        = db;

	pthread_mutex_unlock(&fader->mutex);

	signal_volume_changed(fader, db);
}

static void volmeter_source_volume_changed(void *vptr, calldata_t *calldata)
{
	struct obs_volmeter *volmeter = (struct obs_volmeter *) vptr;

	pthread_mutex_lock(&volmeter->mutex);

	float mul = (float) calldata_float(calldata, "volume");
	volmeter->cur_db = mul_to_db(mul);

	pthread_mutex_unlock(&volmeter->mutex);
}

static void fader_source_destroyed(void *vptr, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);
	struct obs_fader *fader = (struct obs_fader *) vptr;

	obs_fader_detach_source(fader);
}

static void volmeter_source_destroyed(void *vptr, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);
	struct obs_volmeter *volmeter = (struct obs_volmeter *) vptr;

	obs_volmeter_detach_source(volmeter);
}

/* TODO: Separate for individual channels */
static void volmeter_sum_and_max(float *data[MAX_AV_PLANES], size_t frames,
		float *sum, float *max)
{
	float s  = *sum;
	float m  = *max;

	for (size_t plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!data[plane])
			break;

		for (float *c = data[plane]; c < data[plane] + frames; ++c) {
			const float pow = *c * *c;
			s += pow;
			m  = (m > pow) ? m : pow;
		}
	}

	*sum = s;
	*max = m;
}

/**
 * @todo The IIR low pass filter has a different behavior depending on the
 *       update interval and sample rate, it should be replaced with something
 *       that is independent from both.
 */
static void volmeter_calc_ival_levels(obs_volmeter_t *volmeter)
{
	const unsigned int samples = volmeter->ival_frames * volmeter->channels;
	const float alpha    = 0.15f;
	const float ival_max = sqrtf(volmeter->ival_max);
	const float ival_rms = sqrtf(volmeter->ival_sum / (float)samples);

	if (ival_max > volmeter->vol_max) {
		volmeter->vol_max = ival_max;
	} else {
		volmeter->vol_max = alpha * volmeter->vol_max +
				(1.0f - alpha) * ival_max;
	}

	if (volmeter->vol_max > volmeter->vol_peak ||
			volmeter->peakhold_count > volmeter->peakhold_frames) {
		volmeter->vol_peak       = volmeter->vol_max;
		volmeter->peakhold_count = 0;
	} else {
		volmeter->peakhold_count += volmeter->ival_frames;
	}

	volmeter->vol_mag = alpha * ival_rms +
			volmeter->vol_mag * (1.0f - alpha);

	/* reset interval data */
	volmeter->ival_frames = 0;
	volmeter->ival_sum    = 0.0f;
	volmeter->ival_max    = 0.0f;
}

static bool volmeter_process_audio_data(obs_volmeter_t *volmeter,
		const struct audio_data *data)
{
	bool updated   = false;
	size_t frames  = 0;
	size_t left    = data->frames;
	float *adata[MAX_AV_PLANES];

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		adata[i] = (float*)data->data[i];

	while (left) {
		frames  = (volmeter->ival_frames + left >
				volmeter->update_frames)
			? volmeter->update_frames - volmeter->ival_frames
			: left;

		volmeter_sum_and_max(adata, frames, &volmeter->ival_sum,
				&volmeter->ival_max);

		volmeter->ival_frames += (unsigned int)frames;
		left                  -= frames;

		for (size_t i = 0; i < MAX_AV_PLANES; i++) {
			if (!adata[i])
				break;
			adata[i] += frames;
		}

		/* break if we did not reach the end of the interval */
		if (volmeter->ival_frames != volmeter->update_frames)
			break;

		volmeter_calc_ival_levels(volmeter);
		updated = true;
	}

	return updated;
}

static void volmeter_source_data_received(void *vptr, obs_source_t *source,
		const struct audio_data *data, bool muted)
{
	struct obs_volmeter *volmeter = (struct obs_volmeter *) vptr;
	bool updated = false;
	float mul, level, mag, peak;

	pthread_mutex_lock(&volmeter->mutex);

	updated = volmeter_process_audio_data(volmeter, data);

	if (updated) {
		mul   = db_to_mul(volmeter->cur_db);

		level = volmeter->db_to_pos(mul_to_db(volmeter->vol_max * mul));
		mag   = volmeter->db_to_pos(mul_to_db(volmeter->vol_mag * mul));
		peak  = volmeter->db_to_pos(
				mul_to_db(volmeter->vol_peak * mul));
	}

	pthread_mutex_unlock(&volmeter->mutex);

	if (updated)
		signal_levels_updated(volmeter, level, mag, peak, muted);

	UNUSED_PARAMETER(source);
}

static void volmeter_update_audio_settings(obs_volmeter_t *volmeter)
{
	audio_t *audio            = obs_get_audio();
	const unsigned int sr     = audio_output_get_sample_rate(audio);
	uint32_t channels         = (uint32_t)audio_output_get_channels(audio);

	pthread_mutex_lock(&volmeter->mutex);
	volmeter->channels        = channels;
	volmeter->update_frames   = volmeter->update_ms * sr / 1000;
	volmeter->peakhold_frames = volmeter->peakhold_ms * sr / 1000;
	pthread_mutex_unlock(&volmeter->mutex);
}

obs_fader_t *obs_fader_create(enum obs_fader_type type)
{
	struct obs_fader *fader = bzalloc(sizeof(struct obs_fader));
	if (!fader)
		return NULL;

	pthread_mutex_init_value(&fader->mutex);
	pthread_mutex_init_value(&fader->callback_mutex);
	if (pthread_mutex_init(&fader->mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&fader->callback_mutex, NULL) != 0)
		goto fail;

	switch(type) {
	case OBS_FADER_CUBIC:
		fader->def_to_db = cubic_def_to_db;
		fader->db_to_def = cubic_db_to_def;
		fader->max_db    = 0.0f;
		fader->min_db    = -INFINITY;
		break;
	case OBS_FADER_IEC:
		fader->def_to_db = iec_def_to_db;
		fader->db_to_def = iec_db_to_def;
		fader->max_db    = 0.0f;
		fader->min_db    = -INFINITY;
		break;
	case OBS_FADER_LOG:
		fader->def_to_db = log_def_to_db;
		fader->db_to_def = log_db_to_def;
		fader->max_db    = 0.0f;
		fader->min_db    = -96.0f;
		break;
	default:
		goto fail;
		break;
	}
	fader->type = type;

	return fader;
fail:
	obs_fader_destroy(fader);
	return NULL;
}

void obs_fader_destroy(obs_fader_t *fader)
{
	if (!fader)
		return;

	obs_fader_detach_source(fader);
	da_free(fader->callbacks);
	pthread_mutex_destroy(&fader->callback_mutex);
	pthread_mutex_destroy(&fader->mutex);

	bfree(fader);
}

bool obs_fader_set_db(obs_fader_t *fader, const float db)
{
	if (!fader)
		return false;

	pthread_mutex_lock(&fader->mutex);

	bool clamped  = false;
	fader->cur_db = db;

	if (fader->cur_db > fader->max_db) {
		fader->cur_db = fader->max_db;
		clamped       = true;
	}
	if (fader->cur_db < fader->min_db) {
		fader->cur_db = -INFINITY;
		clamped       = true;
	}

	fader->ignore_next_signal = true;
	obs_source_t *src         = fader->source;
	const float mul           = db_to_mul(fader->cur_db);

	pthread_mutex_unlock(&fader->mutex);

	if (src)
		obs_source_set_volume(src, mul);

	return !clamped;
}

float obs_fader_get_db(obs_fader_t *fader)
{
	if (!fader)
		return 0.0f;

	pthread_mutex_lock(&fader->mutex);
	const float db = fader->cur_db;
	pthread_mutex_unlock(&fader->mutex);

	return db;
}

bool obs_fader_set_deflection(obs_fader_t *fader, const float def)
{
	if (!fader)
		return false;

	return obs_fader_set_db(fader, fader->def_to_db(def));
}

float obs_fader_get_deflection(obs_fader_t *fader)
{
	if (!fader)
		return 0.0f;

	pthread_mutex_lock(&fader->mutex);
	const float def = fader->db_to_def(fader->cur_db);
	pthread_mutex_unlock(&fader->mutex);

	return def;
}

bool obs_fader_set_mul(obs_fader_t *fader, const float mul)
{
	if (!fader)
		return false;

	return obs_fader_set_db(fader, mul_to_db(mul));
}

float obs_fader_get_mul(obs_fader_t *fader)
{
	if (!fader)
		return 0.0f;

	pthread_mutex_lock(&fader->mutex);
	const float mul = db_to_mul(fader->cur_db);
	pthread_mutex_unlock(&fader->mutex);

	return mul;
}

bool obs_fader_attach_source(obs_fader_t *fader, obs_source_t *source)
{
	signal_handler_t *sh;
	float vol;

	if (!fader || !source)
		return false;

	obs_fader_detach_source(fader);

	sh = obs_source_get_signal_handler(source);
	signal_handler_connect(sh, "volume",
			fader_source_volume_changed, fader);
	signal_handler_connect(sh, "destroy",
			fader_source_destroyed, fader);
	vol = obs_source_get_volume(source);

	pthread_mutex_lock(&fader->mutex);

	fader->source = source;
	fader->cur_db = mul_to_db(vol);

	pthread_mutex_unlock(&fader->mutex);

	return true;
}

void obs_fader_detach_source(obs_fader_t *fader)
{
	signal_handler_t *sh;
	obs_source_t *source;

	if (!fader)
		return;

	pthread_mutex_lock(&fader->mutex);
	source = fader->source;
	fader->source = NULL;
	pthread_mutex_unlock(&fader->mutex);

	if (!source)
		return;

	sh = obs_source_get_signal_handler(source);
	signal_handler_disconnect(sh, "volume",
			fader_source_volume_changed, fader);
	signal_handler_disconnect(sh, "destroy",
			fader_source_destroyed, fader);

}

void obs_fader_add_callback(obs_fader_t *fader, obs_fader_changed_t callback,
		void *param)
{
	struct fader_cb cb = {callback, param};

	if (!obs_ptr_valid(fader, "obs_fader_add_callback"))
		return;

	pthread_mutex_lock(&fader->callback_mutex);
	da_push_back(fader->callbacks, &cb);
	pthread_mutex_unlock(&fader->callback_mutex);
}

void obs_fader_remove_callback(obs_fader_t *fader, obs_fader_changed_t callback,
		void *param)
{
	struct fader_cb cb = {callback, param};

	if (!obs_ptr_valid(fader, "obs_fader_remove_callback"))
		return;

	pthread_mutex_lock(&fader->callback_mutex);
	da_erase_item(fader->callbacks, &cb);
	pthread_mutex_unlock(&fader->callback_mutex);
}

obs_volmeter_t *obs_volmeter_create(enum obs_fader_type type)
{
	struct obs_volmeter *volmeter = bzalloc(sizeof(struct obs_volmeter));
	if (!volmeter)
		return NULL;

	pthread_mutex_init_value(&volmeter->mutex);
	pthread_mutex_init_value(&volmeter->callback_mutex);
	if (pthread_mutex_init(&volmeter->mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&volmeter->callback_mutex, NULL) != 0)
		goto fail;

	/* set conversion functions */
	switch(type) {
	case OBS_FADER_CUBIC:
		volmeter->pos_to_db = cubic_def_to_db;
		volmeter->db_to_pos = cubic_db_to_def;
		break;
	case OBS_FADER_IEC:
		volmeter->pos_to_db = iec_def_to_db;
		volmeter->db_to_pos = iec_db_to_def;
		break;
	case OBS_FADER_LOG:
		volmeter->pos_to_db = log_def_to_db;
		volmeter->db_to_pos = log_db_to_def;
		break;
	default:
		goto fail;
		break;
	}
	volmeter->type = type;

	obs_volmeter_set_update_interval(volmeter, 50);
	obs_volmeter_set_peak_hold(volmeter, 1500);

	return volmeter;
fail:
	obs_volmeter_destroy(volmeter);
	return NULL;
}

void obs_volmeter_destroy(obs_volmeter_t *volmeter)
{
	if (!volmeter)
		return;

	obs_volmeter_detach_source(volmeter);
	da_free(volmeter->callbacks);
	pthread_mutex_destroy(&volmeter->callback_mutex);
	pthread_mutex_destroy(&volmeter->mutex);

	bfree(volmeter);
}

bool obs_volmeter_attach_source(obs_volmeter_t *volmeter, obs_source_t *source)
{
	signal_handler_t *sh;
	float vol;

	if (!volmeter || !source)
		return false;

	obs_volmeter_detach_source(volmeter);

	sh = obs_source_get_signal_handler(source);
	signal_handler_connect(sh, "volume",
			volmeter_source_volume_changed, volmeter);
	signal_handler_connect(sh, "destroy",
			volmeter_source_destroyed, volmeter);
	obs_source_add_audio_capture_callback(source,
			volmeter_source_data_received, volmeter);
	vol = obs_source_get_volume(source);

	pthread_mutex_lock(&volmeter->mutex);

	volmeter->source = source;
	volmeter->cur_db = mul_to_db(vol);

	pthread_mutex_unlock(&volmeter->mutex);

	return true;
}

void obs_volmeter_detach_source(obs_volmeter_t *volmeter)
{
	signal_handler_t *sh;
	obs_source_t *source;

	if (!volmeter)
		return;

	pthread_mutex_lock(&volmeter->mutex);
	source = volmeter->source;
	volmeter->source = NULL;
	pthread_mutex_unlock(&volmeter->mutex);

	if (!source)
		return;

	sh = obs_source_get_signal_handler(source);
	signal_handler_disconnect(sh, "volume",
			volmeter_source_volume_changed, volmeter);
	signal_handler_disconnect(sh, "destroy",
			volmeter_source_destroyed, volmeter);
	obs_source_remove_audio_capture_callback(source,
			volmeter_source_data_received, volmeter);
}

void obs_volmeter_set_update_interval(obs_volmeter_t *volmeter,
		const unsigned int ms)
{
	if (!volmeter || !ms)
		return;

	pthread_mutex_lock(&volmeter->mutex);
	volmeter->update_ms = ms;
	pthread_mutex_unlock(&volmeter->mutex);

	volmeter_update_audio_settings(volmeter);
}

unsigned int obs_volmeter_get_update_interval(obs_volmeter_t *volmeter)
{
	if (!volmeter)
		return 0;

	pthread_mutex_lock(&volmeter->mutex);
	const unsigned int interval = volmeter->update_ms;
	pthread_mutex_unlock(&volmeter->mutex);

	return interval;
}

void obs_volmeter_set_peak_hold(obs_volmeter_t *volmeter, const unsigned int ms)
{
	if (!volmeter)
		return;

	pthread_mutex_lock(&volmeter->mutex);
	volmeter->peakhold_ms = ms;
	pthread_mutex_unlock(&volmeter->mutex);

	volmeter_update_audio_settings(volmeter);
}

unsigned int obs_volmeter_get_peak_hold(obs_volmeter_t *volmeter)
{
	if (!volmeter)
		return 0;

	pthread_mutex_lock(&volmeter->mutex);
	const unsigned int peakhold = volmeter->peakhold_ms;
	pthread_mutex_unlock(&volmeter->mutex);

	return peakhold;
}

void obs_volmeter_add_callback(obs_volmeter_t *volmeter,
		obs_volmeter_updated_t callback, void *param)
{
	struct meter_cb cb = {callback, param};

	if (!obs_ptr_valid(volmeter, "obs_volmeter_add_callback"))
		return;

	pthread_mutex_lock(&volmeter->callback_mutex);
	da_push_back(volmeter->callbacks, &cb);
	pthread_mutex_unlock(&volmeter->callback_mutex);
}

void obs_volmeter_remove_callback(obs_volmeter_t *volmeter,
		obs_volmeter_updated_t callback, void *param)
{
	struct meter_cb cb = {callback, param};

	if (!obs_ptr_valid(volmeter, "obs_volmeter_remove_callback"))
		return;

	pthread_mutex_lock(&volmeter->callback_mutex);
	da_erase_item(volmeter->callbacks, &cb);
	pthread_mutex_unlock(&volmeter->callback_mutex);
}

float obs_volmeter_get_cur_db(enum obs_fader_type type, const float def)
{
	float db;

	switch(type) {
	case OBS_FADER_CUBIC:
		db = cubic_def_to_db(def);
		break;
	case OBS_FADER_IEC:
		db = iec_def_to_db(def);
		break;
	case OBS_FADER_LOG:
		db = log_def_to_db(def);
		break;
	default:
		goto fail;
		break;
	}

	return db;
fail:
	return -INFINITY;
}
