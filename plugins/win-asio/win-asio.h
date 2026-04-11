/******************************************************************************
    Copyright (C) 2022-2025 pkv <pkv@obsproject.com>

    This file is part of win-asio.

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
#include "asio-common.h"

#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/darray.h>
#include <util/deque.h>

struct asio_device;

struct asio_data {
	/* common */
	struct asio_device *asio_device;             // asio device (source plugin: input; output plugin: output)
	int asio_client_index[MAX_NUM_ASIO_DEVICES]; // index of obs source in device client list
	const char *device_name;                     // device name
	uint8_t device_index;                        // device index in the driver list
	bool update_channels;                        // bool to track the change of driver
	enum speaker_layout speakers;                // speaker layout
	int sample_rate;                             // 44100 or 48000 Hz
	uint8_t in_channels;                         // number of device input channels
	uint8_t out_channels;                        // output :number of device output channels;
						     // source: number of obs output channels set in OBS Audio Settings
	volatile bool stopping;                      // signals the source is stopping
	bool initial_update;                         // initial update right after creation
	bool driver_loaded;                          // driver was loaded correctly
	bool is_output;                              // true if it is an output; false if it is an input capture
	/* source */
	obs_source_t *source;
	int mix_channels[MAX_AUDIO_CHANNELS]; // stores the channel re-ordering info
	volatile bool active;                 // tracks whether the device is streaming
	/* output*/
	obs_output_t *output;
	uint8_t obs_track_channels;                // number of obs output channels
	int out_mix_channels[MAX_DEVICE_CHANNELS]; // Stores which obs track and which track channel has been picked.
						   // 3 bits are reserved for the track channel (0-7) since obs
						   // supports up to 8 audio channels. 1 more bit is reserved to
						   // allow for up to 16 channels, should there be a need later to
						   // expand the channel count (presumbably for broadcast setups).
						   // Track_index is then stored as 1 << track_index + 4
						   // so: track 0 = 16, track 1 = 32, etc.
};
