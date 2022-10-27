/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

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
#include <util/darray.h>
#include <util/platform.h>
#include <util/threading.h>

struct asio_device;

struct asio_data {
	/* common */
	struct asio_device *asio_device;             // ASIO device (source plugin: input; output plugin: output)
	int asio_client_index[MAX_NUM_ASIO_DEVICES]; // index of OBS source in device client list
	const char *device_name;                     // device name
	int device_index;                            // device index in the driver list
	bool update_channels;                        // bool to track the change of driver
	uint8_t out_channels;                        // output:number of device output channels;
						     // source: number of OBS output channels set in OBS Audio Settings
	volatile bool stopping;                      // signals the source is stopping
	bool initial_update;                         // initial update right after creation
	bool driver_loaded;                          // driver was loaded correctly
	bool is_output;                              // true if it is an output; false if it is an input capture
	/* source */
	obs_source_t *source;
	int mix_channels[MAX_AUDIO_CHANNELS]; // stores the channel re-ordering info
	volatile bool active;                 // tracks whether the device is streaming
	/* output */
	obs_output_t *output;
	uint8_t obs_track_channels; // number of OBS output channels
	/* output_routing: bitmask storing the OBS track and the track channel per:
	 * output_routing = channel_index + 1 << (track_index + 4)
	 * 4 bits are reserved for the track channel index: 0-7 (with an extra bit in case of future expansion).
	 * track_index range is [0-6] for the 6 OBS tracks + 1 for the monitoring.
	 */
	int64_t output_routing[MAX_DEVICE_CHANNELS];
};

bool asio_output_start(void *vptr);
