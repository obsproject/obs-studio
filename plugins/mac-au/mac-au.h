/*****************************************************************************
Copyright (C) 2025 by pkv@obsproject.com

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
*****************************************************************************/
#pragma once
#include "mac-au-scan.h"
#include <obs-module.h>
#include <media-io/audio-resampler.h>
#include <util/platform.h>
#include <util/deque.h>
#include <util/threading.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CFString.h>
#include <CoreAudio/CoreAudio.h>

#include <atomic>
#include <mutex>
#include <string>
#include <memory>

#define MAX_PREPROC_CHANNELS 8
#define MAX_SC_CHANNELS 2
#define BUFFER_SIZE_MSEC 10
#define FRAME_SIZE 480

struct au_plugin;

struct au_data {
	obs_source_t *context;

	std::shared_ptr<au_plugin> plugin = nullptr;
	char uid[32];
	char name[64];

	uint32_t sample_rate;
	size_t frames;
	size_t channels;
	enum speaker_layout layout;
	int64_t running_sample_count = 0;
	uint64_t system_time = 0;
	uint64_t last_timestamp;
	uint64_t latency;

	struct deque info_buffer;
	struct deque input_buffers[MAX_PREPROC_CHANNELS];
	struct deque output_buffers[MAX_PREPROC_CHANNELS];
	struct deque sc_input_buffers[MAX_PREPROC_CHANNELS];

	/* PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
	float *sc_copy_buffers[MAX_PREPROC_CHANNELS];

	/* output data */
	struct obs_audio_data output_audio;
	DARRAY(float) output_data;

	/* state vars */
	std::atomic<bool> bypass;
	std::atomic<bool> sidechain_enabled;
	std::atomic<bool> has_view;
	std::atomic_flag init_in_progress = ATOMIC_FLAG_INIT;

	/* Sidechain */
	std::atomic<bool> has_sidechain;
	obs_weak_source_t *weak_sidechain = nullptr;
	std::string sidechain_name = "";
	uint64_t sidechain_check_time = 0;
	std::shared_ptr<audio_resampler> sc_resampler;
	size_t sc_channels = 0;
	uint64_t sc_last_timestamp = 0;
	uint64_t sc_latency;
	std::mutex sidechain_update_mutex;
	std::mutex sidechain_mutex;

	/* error messages */
	bool last_init_failed;
};
