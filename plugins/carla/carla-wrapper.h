/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <obs-module.h>

// maximum buffer used, can be smaller
#define MAX_AUDIO_BUFFER_SIZE 512

enum buffer_size_mode {
	buffer_size_direct,
	buffer_size_buffered_128,
	buffer_size_buffered_256,
	buffer_size_buffered_512,
	buffer_size_buffered_max = buffer_size_buffered_512
};

// ----------------------------------------------------------------------------
// helper methods

static inline uint32_t bufsize_mode_to_frames(enum buffer_size_mode bufsize)
{
	switch (bufsize) {
	case buffer_size_buffered_128:
		return 128;
	case buffer_size_buffered_256:
		return 256;
	default:
		return MAX_AUDIO_BUFFER_SIZE;
	}
}

// ----------------------------------------------------------------------------
// carla + obs integration methods

#ifdef __cplusplus
extern "C" {
#endif

struct carla_priv;

struct carla_priv *carla_priv_create(obs_source_t *source,
				     enum buffer_size_mode bufsize,
				     uint32_t srate);
void carla_priv_destroy(struct carla_priv *carla);

void carla_priv_activate(struct carla_priv *carla);
void carla_priv_deactivate(struct carla_priv *carla);
void carla_priv_process_audio(struct carla_priv *carla,
			      float *buffers[MAX_AV_PLANES], uint32_t frames);

void carla_priv_idle(struct carla_priv *carla);

void carla_priv_save(struct carla_priv *carla, obs_data_t *settings);
void carla_priv_load(struct carla_priv *carla, obs_data_t *settings);

void carla_priv_set_buffer_size(struct carla_priv *carla,
				enum buffer_size_mode bufsize);

void carla_priv_readd_properties(struct carla_priv *carla,
				 obs_properties_t *props, bool reset);

#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
