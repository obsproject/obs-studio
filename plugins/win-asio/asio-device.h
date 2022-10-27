/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.

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
#ifndef ASIO_DEVICE_H
#define ASIO_DEVICE_H
#pragma once
#include "asio-common.h"
#include "asio-format.h"

#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/darray.h>
#include <util/deque.h>

struct asio_data;

struct asio_device {
	/* Device name */
	char device_name[64];

	/* COM driver stuff */
	IASIO *asio;
	DWORD com_thread_id;
	CLSID clsid;

	/* channel info : number & names */
	long total_num_input_chans;
	long total_num_output_chans;
	char input_channel_names[MAX_DEVICE_CHANNELS][MAX_CH_NAME_LENGTH];
	char output_channel_names[MAX_DEVICE_CHANNELS][MAX_CH_NAME_LENGTH];

	/* Audio buffers: the meat and blood */
	ASIOBufferInfo buffer_infos[MAX_DEVICE_CHANNELS * 2];
	float *in_buffers[MAX_DEVICE_CHANNELS];
	float *out_buffers[MAX_DEVICE_CHANNELS];
	float *io_buffer_space;
	uint8_t silentBuffers8[4096];

	/* Device buffer settings (expressed usually as a number of samples  e.g. 512, 1024, etc.). This impacts the 
	 * latency of the device; at 48 kHz, there are 48000 samples per second, so a buffer size of 512 samples means
	 * a latency of 512/48000 = 0.01067 seconds or 10.67 ms.
	 */
	int current_buffer_size;
	int buffer_granularity;
	DARRAY(int) buffer_sizes; // array holding the buffer values permitted by the device
	int min_buffer_size;
	int max_buffer_size;
	int preferred_buffer_size;
	bool should_use_preferred_size;

	/* Device sample rate*/
	DARRAY(double) sample_rates;
	double current_sample_rate;

	/* Device audio format (32bit ffloat, 16bit int, 24 bit int, 32bit int */
	int current_bit_depth;
	int current_block_size_samples;
	asio_sample_format input_format[MAX_DEVICE_CHANNELS];
	asio_sample_format output_format[MAX_DEVICE_CHANNELS];
	ASIOSampleType output_type; // the format to which obs float audio data must be converted to.

	/* clocks */
	ASIOClockSource clocks[MAX_DEVICE_CHANNELS];
	int num_clock_sources;

	/* ASIO callbacks */
	ASIOCallbacks callbacks;
	volatile bool called_back;

	/* Device state */
	bool is_open;
	bool is_started;
	bool driver_failure;
	volatile bool need_to_reset;
	bool buffers_created;
	bool post_output;
	volatile bool capture_started;

	/* Misc. info */
	long input_latency;
	long output_latency;
	int xruns;
	char errorstring[256];

	/* Device slot number in the list of devices. This is used to identify the device in the list of devices
	 * created by the host. */
	int slot_number;

	/* ======= ASIO device <==> OBS communication ====== */

	/* Capture Audio (device ==> OBS).
	 * Each device will stream audio to a number of obs asio sources acting as audio clients.
	 * The clients are listed in this darray which stores the asio_data struct ptr.
	 */
	struct asio_data *obs_clients[32];
	int current_nb_clients;

	/* Output Audio (OBS ==> device).
	 * Each device can be a client to a single obs output which outputs audio to the said device.
	 * 'output_frames': circular buffer to store the frames which are passed to asio devices.
	 * Any of the 6 tracks in obs mixer can be streamed to the device. The plugin also allows to select a channel
	 * for each track. Therefore one has to specify a track index and a channel index for each output channel of
	 * the device. -1 means no track or a mute channel.
	 * obs_track[]: array which holds the track index played for each device output channel.
	 * obs_track_channel[]: array which stores the channel index of an OBS audio track played on an output channel.
	 * Ex: obs_track[ch=2] = 2 & obs_track_channel[2]=1 means we are playing on the third channel (ch +1) channel 2
	 * ( = obs_track_channel[2] + 1) from track 3 ( = obs_track[ch=2]+1)
	 */
	struct asio_data *obs_output_client;
	struct deque output_frames[MAX_DEVICE_CHANNELS];
	int obs_track[MAX_DEVICE_CHANNELS];
	int obs_track_channel[MAX_DEVICE_CHANNELS];
};

extern struct asio_device *current_asio_devices[MAX_NUM_ASIO_DEVICES];

struct asio_device *asio_device_create(const char *name, CLSID clsid, int slot_number);
void asio_device_destroy(struct asio_device *dev);
void asio_device_test(struct asio_device *dev);

void asio_device_open(struct asio_device *dev, double sample_rate, long buffer_size_samples);
void asio_device_close(struct asio_device *dev);

bool asio_device_start(struct asio_device *dev);
void asio_device_stop(struct asio_device *dev);

void asio_device_dispose_buffers(struct asio_device *dev);

void asio_device_set_output_client(struct asio_device *dev, struct asio_data *client);
struct asio_data *asio_device_get_output_client(struct asio_device *dev);
void asio_device_reload_channel_names(struct asio_device *dev);

void asio_device_reset_request(struct asio_device *dev);
long asio_device_asio_message_callback(struct asio_device *dev, long selector, long value, void *message, double *opt);
void asio_device_callback(struct asio_device *dev, long buffer_index);

double asio_device_get_sample_rate(struct asio_device *dev);
int asio_device_get_preferred_buffer_size(struct asio_device *dev);

void asio_device_show_control_panel(struct asio_device *dev);
struct asio_device *asio_device_find_by_name(const char *name);

#endif // ASIO_DEVICE_H
