/******************************************************************************
    Copyright (C) 2026 pkv <pkv@obsproject.com>

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

#pragma once

#include "../obs.h"
#include "../media-io/audio-io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct monitoring_mix_data;
struct monitoring_mix_source;

typedef void (*monitoring_mix_output_callback_t)(void *param, struct audio_data *frames);

struct monitoring_mix_data *monitoring_mix_create(uint32_t sample_rate, uint32_t channels);
void monitoring_mix_destroy(struct monitoring_mix_data *mix_data);

struct monitoring_mix_source *monitoring_mix_add_source(obs_source_t *source);

void monitoring_mix_remove_source(struct monitoring_mix_data *mix_data, struct monitoring_mix_source *mix_source);

bool monitoring_mix_output_connect(struct monitoring_mix_data *mix_data, monitoring_mix_output_callback_t callback,
				   void *param);

void monitoring_mix_output_disconnect(struct monitoring_mix_data *mix_data, monitoring_mix_output_callback_t callback,
				      void *param);

#ifdef __cplusplus
}
#endif
