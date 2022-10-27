/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#ifndef ASIO_DEVICE_H
#define ASIO_DEVICE_H
#pragma once
#include <obs-module.h>
#include <obs-frontend-api.h>
#include "iasiodrv.h"
#include <stdbool.h>
#include <util/platform.h>
#include <util/threading.h>
#include <util/darray.h>
#include <util/deque.h>
#include "asio-format.h"

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MAX_DEVICE_CHANNELS 32
#define MAX_CH_NAME_LENGTH 32 // set by SDK
/* We allow a maximum of 16 ASIO drivers loaded by the host. It should be plenty enough ! */
#define MAX_NUM_ASIO_DEVICES 16

#define ASIO_LOG(level, format, ...) blog(level, "[asio_device '%s']: " format, dev->device_name, ##__VA_ARGS__)
#define ASIO_LOG2(level, format, ...) blog(level, "[asio_device_list]: " format, ##__VA_ARGS__)
#define debug(format, ...) ASIO_LOG(LOG_DEBUG, format, ##__VA_ARGS__)
#define warn(format, ...) ASIO_LOG(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) ASIO_LOG(LOG_INFO, format, ##__VA_ARGS__)
#define info2(format, ...) ASIO_LOG2(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) ASIO_LOG(LOG_ERROR, format, ##__VA_ARGS__)
#define error2(format, ...) ASIO_LOG2(LOG_ERROR, format, ##__VA_ARGS__)

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
	 * 'excess_frames': circular buffer to store the frames which are passed to asio devices.
	 * Any of the 6 tracks in obs mixer can be streamed to the device. The plugin also allows to select a channel
	 * for each track. Therefore one has to specify a track index and a channel index for each output channel of
	 * the device. -1 means no track or a mute channel.
	 * obs_track[MAX_DEVICE_CHANNELS]: array which holds the track index for each device output channel.
	 * obs_track_channel[]: array which stores the channel index of an OBS audio track.
	 */
	struct asio_data *obs_output_client;
	struct deque excess_frames[MAX_DEVICE_CHANNELS];
	int obs_track[MAX_DEVICE_CHANNELS];
	int obs_track_channel[MAX_DEVICE_CHANNELS];
};

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
