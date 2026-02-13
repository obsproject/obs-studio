/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It implements an ASIO host for OBS Studio using the Steinberg ASIO SDK,
    available under a dual licence including the GNU GPLv3.
    Certain parts of the driver initialisation sequence, buffer management
    strategy and start/stop flow are inspired by the structure and behaviour
    of the JUCE ASIO host (juce_ASIO_windows.cpp), but this file is an
    independent C implementation adapted to OBS/libobs. No JUCE source code
    is used or included.

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
#include "asio-device.h"
#include "win-asio.h"
#include "asio-callbacks.h"
#include "asio-format.h"

#include <windows.h>
#include <math.h>
#include <objbase.h>
#include <guiddef.h>

#include <strsafe.h>
#include <stdlib.h>
#include <string.h>

const IID IID_IASIO = {0x261a7a60, 0xa003, 0x11d1, {0xa3, 0x90, 0x00, 0x80, 0x5f, 0x08, 0x38, 0x75}};

/* global vars for com initialization in Main thread */
bool bComInitialized = false;
os_event_t *shutting_down;
volatile bool shutting_down_atomic = false;
struct asio_device *current_asio_devices[MAX_NUM_ASIO_DEVICES] = {0};

void asio_device_destroy_all();
void OBSEvent(enum obs_frontend_event event, void *none)
{
	UNUSED_PARAMETER(none);
	if (event == OBS_FRONTEND_EVENT_EXIT || event == OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN) {
		os_atomic_set_bool(&shutting_down_atomic, true);
		asio_device_destroy_all();
	}
}

void asio_device_get_sample_format(int type, char *message)
{
	switch (type) {
	case 17:
		strcpy(message, "24 bit int");
		break;
	case 18:
		strcpy(message, "32 bit int");
		break;
	case 19:
		strcpy(message, "32 bit float");
		break;
	case 41:
		strcpy(message, "no channels available");
		break;
	default:
		snprintf(message, 64, "uncommon format number (%d)", type);
		break;
	}
}

struct asio_device *asio_device_find_by_name(const char *name)
{
	if (!name || !*name)
		return NULL;

	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; ++i) {
		struct asio_device *dev = current_asio_devices[i];
		if (dev && strcmp(dev->device_name, name) == 0)
			return dev;
	}
	return NULL;
}

void asio_device_show_control_panel(struct asio_device *dev)
{
	info("Opening ASIO control panel...");
	if (dev && dev->asio)
		ASIO_ControlPanel(dev);
}

/* ASIO driver reset must be done in the UI thread */
static void asio_device_do_reset_task(void *param)
{
	struct asio_device *dev = (struct asio_device *)param;
	info("ASIO driver reset");
	if (!dev)
		return;

	asio_device_close(dev);
	os_atomic_set_bool(&dev->need_to_reset, true);
	asio_device_open(dev, dev->current_sample_rate, dev->current_buffer_size);
	if (dev->obs_output_client) {
		os_atomic_set_bool(&dev->obs_output_client->stopping, false);
	}

	if (dev->errorstring[0] != '\0') {
		error("Failed to reopen during reset: %s", dev->errorstring);
		dev->errorstring[0] = '\0';
		return;
	}
	asio_device_reload_channel_names(dev);
	info("ASIO reset complete");
}

void asio_device_reset_request(struct asio_device *dev)
{
	info("ASIO reset requested.");
	if (!dev)
		return;

	os_atomic_set_bool(&dev->need_to_reset, true);

	const DWORD cur = GetCurrentThreadId();
	if (cur != dev->com_thread_id) {
		Sleep(500);
		obs_queue_task(OBS_TASK_UI, asio_device_do_reset_task, dev, false);
		return;
	}
	asio_device_do_reset_task((void *)dev);
}

void asio_device_reset_buffers(struct asio_device *dev)
{
	if (!dev)
		return;

	long num_input = dev->total_num_input_chans;
	long num_output = dev->total_num_output_chans;

	for (int i = 0; i < num_input; ++i) {
		dev->buffer_infos[i].isInput = ASIOTrue;
		dev->buffer_infos[i].channelNum = i;
		dev->buffer_infos[i].buffers[0] = NULL;
		dev->buffer_infos[i].buffers[1] = NULL;
	}
	for (int i = 0; i < num_output; ++i) {
		int j = (int)(i + num_input);
		dev->buffer_infos[j].isInput = ASIOFalse;
		dev->buffer_infos[j].channelNum = i;
		dev->buffer_infos[j].buffers[0] = NULL;
		dev->buffer_infos[j].buffers[1] = NULL;
	}
}

/* functions related to the 'buffer' setting in the ASIO driver, measured as a number of samples */
static bool contains_size(struct asio_device *dev, long value)
{
	for (size_t i = 0; i < dev->buffer_sizes.num; ++i)
		if (dev->buffer_sizes.array[i] == value)
			return true;

	return false;
}

void asio_device_add_buffer_sizes(struct asio_device *dev, long min_size, long max_size, long preferred_size,
				  long granularity)
{
	if (!dev)
		return;

	if (granularity >= 0) {
		long step = (granularity < 16) ? 16 : granularity;
		long size = min_size;
		if (size % step)
			size = ((size + step - 1) / step) * step;

		for (; size <= max_size && size <= 6400; size += step)
			da_push_back(dev->buffer_sizes, &size);
	} else {
		for (int p = 0; p < 18; ++p) {
			long size = 1L << p;
			if (size >= min_size && size <= max_size)
				da_push_back(dev->buffer_sizes, &size);
		}
	}

	if (preferred_size >= min_size && preferred_size <= max_size && !contains_size(dev, preferred_size)) {
		da_push_back(dev->buffer_sizes, &preferred_size);
	}
}

ASIOError asio_device_refresh_buffer_sizes(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return ASE_NotPresent;

	ASIOError err = ASIO_GetBufferSize(dev, &dev->min_buffer_size, &dev->max_buffer_size,
					   &dev->preferred_buffer_size, &dev->buffer_granularity);

	if (err == ASE_OK) {
		da_clear(dev->buffer_sizes);
		asio_device_add_buffer_sizes(dev, dev->min_buffer_size, dev->max_buffer_size,
					     dev->preferred_buffer_size, dev->buffer_granularity);
	}

	return err;
}

int asio_device_get_preferred_buffer_size(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return 0;
	int min_buffer_size = 0;
	int max_buffer_size = 0;
	int buffer_granularity = 0;
	long preferred = 0;
	if (ASIO_GetBufferSize(dev, &min_buffer_size, &max_buffer_size, &preferred, &buffer_granularity) == ASE_OK) {
		return preferred;
	}
	return 0;
}

int asio_device_read_buffer_size(struct asio_device *dev, int requested_size)
{
	if (!dev || !dev->asio)
		return requested_size;

	dev->min_buffer_size = 0;
	dev->max_buffer_size = 0;
	dev->buffer_granularity = 0;
	long new_preferred = 0;

	if (ASIO_GetBufferSize(dev, &dev->min_buffer_size, &dev->max_buffer_size, &new_preferred,
			       &dev->buffer_granularity) == ASE_OK) {
		if (dev->preferred_buffer_size != 0 && new_preferred != 0 &&
		    new_preferred != dev->preferred_buffer_size)
			dev->should_use_preferred_size = true;

		if (requested_size < dev->min_buffer_size || requested_size > dev->max_buffer_size)
			dev->should_use_preferred_size = true;

		dev->preferred_buffer_size = new_preferred;
	}

	if (dev->should_use_preferred_size) {
		info("Using preferred size for buffer..");
		ASIOError err = asio_device_refresh_buffer_sizes(dev);

		if (err == ASE_OK) {
			requested_size = dev->preferred_buffer_size;
		} else {
			requested_size = 1024;
			warn("getBufferSize1 failed, using 1024 as fallback");
		}

		dev->should_use_preferred_size = false;
	}

	return requested_size;
}

/* driver loading & removal */
static bool asio_device_remove_current_driver(struct asio_device *dev)
{
	bool released_ok = true;
	if (dev->asio) {
		__try {
			ASIO_Release(dev);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			error("Exception occurred while releasing COM object");
			released_ok = false;
		}
		dev->asio = NULL;
	}
	return released_ok;
}

static bool asio_device_try_create_driver(struct asio_device *dev, bool *crashed)
{
	bool success = false;
	__try {
		HRESULT hr =
			CoCreateInstance(&dev->clsid, NULL, CLSCTX_INPROC_SERVER, &dev->clsid, (void **)&dev->asio);
		success = SUCCEEDED(hr);
		if (!success) {
			error("CoCreateInstance failed (HRESULT 0x%lX)", (unsigned long)hr);
		}
		return success;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		error("Exception occurred during CoCreateInstance");
		*crashed = true;
	}
	return false;
}

bool asio_device_load_driver(struct asio_device *dev)
{
	if (!dev)
		return false;

	if (!asio_device_remove_current_driver(dev)) {
		strncpy(dev->errorstring, "** Driver crashed while being closed", 40);
	} else {
		info("Driver successfully removed");
	}

	bool crashed = false;
	bool ok = asio_device_try_create_driver(dev, &crashed);

	if (crashed)
		strncpy(dev->errorstring, "** Driver crashed while being opened", 40);
	else if (ok)
		info("driver com interface opened");
	else
		strncpy(dev->errorstring, "Failed to load driver", 30);

	return ok;
}

void asio_device_init_driver(struct asio_device *dev, char *driver_error)
{
	if (!dev)
		return;

	if (dev->asio == NULL) {
		if (driver_error) {
			strncpy(driver_error, "No driver", 30);
		}
		return;
	}
	HWND hwnd = GetDesktopWindow();
	bool init_ok = ASIO_Init(dev, &hwnd) == ASIOTrue;

	if (!init_ok) {
		ASIO_GetErrorMessage(dev, driver_error);
		if (!driver_error[0]) {
			strncpy(driver_error, "Driver failed to initialize", 30);
		}
	}
	/* that call seems required by sh...y drivers */
	if (driver_error && !driver_error[0]) {
		char buffer[256] = {0};
		ASIO_GetDriverName(dev, buffer);
	}
	return;
}

/* Point of entry: when a user picks an ASIO device, win-asio will create an asio_device unless the driver was already
 * created; the asio_device will then be reused. The following is done:
 * - COM initialization in UI thread if it is the first driver to be loaded in the plugin,
 * - basic initialization of the asio_device struct including the ASIO calbbacks functions,
 * - loading of the COM driver,
 * - testing of the driver (init, get channels, sample rate, buffer sizes, buffer creation, start, stop)
 */
struct asio_device *asio_device_create(const char *name, CLSID clsid, int slot_number)
{
	struct asio_device *dev = calloc(1, sizeof(struct asio_device));
	if (!dev)
		return NULL;

	dev->driver_failure = false;

	if (!bComInitialized) {
		bComInitialized = SUCCEEDED(CoInitialize(NULL));
		if (bComInitialized) {
			dev->com_thread_id = GetCurrentThreadId();
			debug("COM initialized in Main thread %lu for device %s", dev->com_thread_id, name);
		} else {
			error("Failed to initialize COM");
			free(dev);
			return NULL;
		}
	} else {
		dev->com_thread_id = GetCurrentThreadId();
		debug("COM already initialized in Main thread %lu for device %s", dev->com_thread_id, name);
	}

	strncpy(dev->device_name, name, sizeof(dev->device_name) - 1);
	if (current_asio_devices[slot_number] != NULL)
		return NULL;

	dev->clsid = clsid;
	dev->slot_number = slot_number;

	da_init(dev->buffer_sizes);
	da_init(dev->sample_rates);

	if (slot_number < 0 || slot_number >= MAX_NUM_ASIO_DEVICES) {
		blog(LOG_ERROR, "[ASIO] Invalid slot number %d in asio_device_create", slot_number);
		free(dev);
		return NULL;
	}

	dev->callbacks.bufferSwitch = callback_sets[dev->slot_number].buffer_switch;
	dev->callbacks.bufferSwitchTimeInfo = callback_sets[dev->slot_number].buffer_switch_time_info;
	dev->callbacks.asioMessage = callback_sets[dev->slot_number].asio_message;
	dev->callbacks.sampleRateDidChange = callback_sets[dev->slot_number].sample_rate_changed;

	/* we load the driver, retrieve the COM pointer and do some basics tests */
	asio_device_test(dev);
	if (dev->asio == NULL || dev->driver_failure) {
		error("Failed to load driver");
		free(dev);
		return NULL;
	} else {
		current_asio_devices[slot_number] = dev;
		for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
			deque_init(&dev->output_frames[i]);
		}
		for (int k = 0; k < MAX_DEVICE_CHANNELS; k++) {
			dev->obs_track[k] = -1;
			dev->obs_track_channel[k] = -1;
		}
		dev->current_nb_clients = 0;
		os_atomic_set_bool(&dev->capture_started, false);
	}

	return dev;
}

bool are_any_devices_still_active()
{
	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; ++i) {
		if (current_asio_devices[i] != NULL)
			return true;
	}
	return false;
}

void asio_device_destroy(struct asio_device *dev)
{
	if (!dev)
		return;

	for (int j = 0; j < MAX_DEVICE_CHANNELS; j++) {
		deque_free(&dev->output_frames[j]);
	}

	da_free(dev->buffer_sizes);
	da_free(dev->sample_rates);

	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; ++i)
		if (current_asio_devices[i] == dev)
			current_asio_devices[i] = NULL;

	asio_device_close(dev);

	dev->current_nb_clients = 0;
	memset(dev->obs_clients, 0, sizeof(dev->obs_clients));

	if (dev->obs_output_client) {
		blog(LOG_INFO,
		     "[asio_output]:\n\tDevice % s removed;\n"
		     "\tnumber of xruns: % i;\n\tincrease your buffer if you get a high count & hear cracks, pops or"
		     " else !\n-1 means your device doesn't report xruns.",
		     dev->device_name, dev->xruns);
		dev->obs_output_client = NULL;
	}
	free(dev->io_buffer_space);
	dev->io_buffer_space = NULL;

	if (!asio_device_remove_current_driver(dev))
		info("** Driver crashed while being closed");
	else
		info("Closed ASIO device");

	/* close COM if this is the last device */
	if (bComInitialized) {
		if (!are_any_devices_still_active()) {
			CoUninitialize();
			bComInitialized = false;
			debug("Last ASIO device destroyed — COM uninitialized");
		} else {
			debug("ASIO device destroyed — COM still in use by others");
		}
	}

	free(dev);
}

void asio_device_destroy_all()
{
	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; ++i) {
		if (current_asio_devices[i]) {
			asio_device_destroy(current_asio_devices[i]);
		}
	}
}

bool get_channel_name(struct asio_device *dev, char channel_names[MAX_DEVICE_CHANNELS][MAX_CH_NAME_LENGTH], int index,
		      bool is_input)
{
	if (!dev)
		return false;

	if (index < 0 || index >= MAX_DEVICE_CHANNELS)
		return false;

	ASIOChannelInfo channel_info = {.channel = index, .isInput = is_input ? ASIOTrue : ASIOFalse};

	if (ASIO_GetChannelInfo(dev, &channel_info) != ASE_OK)
		return false;

	strncpy(channel_names[index], channel_info.name, MAX_CH_NAME_LENGTH - 1);
	channel_names[index][MAX_CH_NAME_LENGTH - 1] = '\0';
	return true;
}

void clear_channel_names(char names[MAX_DEVICE_CHANNELS][MAX_CH_NAME_LENGTH])
{
	for (int i = 0; i < MAX_DEVICE_CHANNELS; ++i)
		names[i][0] = '\0';
}

double asio_device_get_sample_rate(struct asio_device *dev)
{
	double cr = 0;
	auto err = ASIO_GetSampleRate(dev, &cr);
	return cr;
}

void asio_device_set_sample_rate(struct asio_device *dev, double new_rate)
{
	if (!dev || !dev->asio)
		return;

	if (dev->current_sample_rate != new_rate) {
		info("rate change: %.0f → %.0f", dev->current_sample_rate, new_rate);

		ASIOError err = ASIO_SetSampleRate(dev, new_rate);
		if (err != ASE_OK)
			warn("setSampleRate failed with code %d", err);

		Sleep(10);

		if (err == ASE_NoClock && dev->num_clock_sources > 0) {
			info("trying to set a clock source..");
			err = ASIO_SetClockSource(dev, dev->clocks[0].index);
			if (err != ASE_OK)
				warn("setClockSource failed with code %d", err);

			Sleep(10);

			err = ASIO_SetSampleRate(dev, new_rate);
			if (err != ASE_OK)
				warn("setSampleRate (retry) failed with code %d", err);

			Sleep(10);
		}

		if (err == ASE_OK)
			dev->current_sample_rate = new_rate;
	}
}

void asio_device_update_sample_rates_list(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return;

	static const double common_rates[] = {8000,  11025, 16000,  22050,  24000,  32000,  44100,  48000,
					      88200, 96000, 176400, 192000, 352800, 384000, 705600, 768000};

	DARRAY(double) new_rates;
	da_init(new_rates);

	for (size_t i = 0; i < sizeof(common_rates) / sizeof(common_rates[0]); ++i) {
		double rate = common_rates[i];
		if (ASIO_CanSampleRate(dev, rate) == ASE_OK)
			da_push_back(new_rates, &rate);
	}

	if (new_rates.num == 0) {
		double current = asio_device_get_sample_rate(dev);
		info("No standard sample rates supported - using current rate: %.2f", current);
		if (current > 0.0)
			da_push_back(new_rates, &current);
	}

	bool changed = false;
	if (dev->sample_rates.num != new_rates.num) {
		changed = true;
	} else {
		for (size_t i = 0; i < dev->sample_rates.num; ++i) {
			if (dev->sample_rates.array[i] != new_rates.array[i]) {
				changed = true;
				break;
			}
		}
	}

	if (changed) {
		da_move(dev->sample_rates, new_rates);

		char buffer[256] = {0};
		for (size_t i = 0; i < dev->sample_rates.num; ++i) {
			char tmp[32];
			snprintf(tmp, sizeof(tmp), "%.0f%s", dev->sample_rates.array[i],
				 (i < dev->sample_rates.num - 1) ? ", " : "");
			strcat_s(buffer, sizeof(buffer), tmp);
		}
		debug("Supported sample rates: %s", buffer);
	}

	da_free(new_rates);
}

void asio_device_update_clock_sources(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return;

	memset(dev->clocks, 0, sizeof(dev->clocks));
	long num_sources = MAX_DEVICE_CHANNELS;
	ASIOError err = ASIO_GetClockSources(dev, dev->clocks, &num_sources);
	dev->num_clock_sources = (int)num_sources;

	bool is_source_set = false;

	for (int i = 0; i < dev->num_clock_sources; ++i) {
		char log_msg[128];
		snprintf(log_msg, sizeof(log_msg), "clock: %s", dev->clocks[i].name);

		if (dev->clocks[i].isCurrentSource) {
			is_source_set = true;
			strcat_s(log_msg, sizeof(log_msg), " (cur)");
		}

		info("%s", log_msg);
	}

	if (dev->num_clock_sources > 1 && !is_source_set) {
		info("setting clock source");
		err = ASIO_SetClockSource(dev, dev->clocks[0].index);
		if (err != ASE_OK)
			warn("setClockSource1 failed with code %d", err);

		Sleep(20);
	} else if (dev->num_clock_sources == 0) {
		info("no clock sources!");
	}
}

void asio_device_read_latencies(struct asio_device *dev)
{
	if (!dev)
		return;

	dev->input_latency = 0;
	dev->output_latency = 0;

	if (ASIO_GetLatencies(dev, &dev->input_latency, &dev->output_latency) != ASE_OK)
		info("getLatencies() failed");
	else
		info("Latencies (samples): in = %i, out = %i", dev->input_latency, dev->output_latency);
}

void asio_device_reload_channel_names(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return;

	long total_inputs = 0, total_outputs = 0;
	ASIOError err = ASIO_GetChannels(dev, &total_inputs, &total_outputs);
	if (err != ASE_OK)
		return;

	if (total_inputs > MAX_DEVICE_CHANNELS || total_outputs > MAX_DEVICE_CHANNELS)
		info("Only up to %d input + %d output channels are enabled. Higher channel counts are disabled.",
		     MAX_DEVICE_CHANNELS, MAX_DEVICE_CHANNELS);

	dev->total_num_input_chans = (total_inputs > MAX_DEVICE_CHANNELS) ? MAX_DEVICE_CHANNELS : total_inputs;
	dev->total_num_output_chans = (total_outputs > MAX_DEVICE_CHANNELS) ? MAX_DEVICE_CHANNELS : total_outputs;

	clear_channel_names(dev->input_channel_names);
	clear_channel_names(dev->output_channel_names);

	for (int i = 0; i < dev->total_num_input_chans; ++i) {
		if (!get_channel_name(dev, dev->input_channel_names, i, true))
			snprintf(dev->input_channel_names[i], MAX_CH_NAME_LENGTH, "Input %d", i);
	}

	for (int i = 0; i < dev->total_num_output_chans; ++i) {
		if (!get_channel_name(dev, dev->output_channel_names, i, false))
			snprintf(dev->output_channel_names[i], MAX_CH_NAME_LENGTH, "Output %d", i);
	}
}

/* we test the driver with buffers with up to 2 channels */
void asio_device_create_dummy_buffers(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return;

	const int num_dummy_inputs = dev->total_num_input_chans < 2 ? dev->total_num_input_chans : 2;
	const int num_dummy_outputs = dev->total_num_output_chans < 2 ? dev->total_num_output_chans : 2;

	for (int i = 0; i < num_dummy_inputs; ++i) {
		dev->buffer_infos[i].isInput = ASIOTrue;
		dev->buffer_infos[i].channelNum = i;
		dev->buffer_infos[i].buffers[0] = NULL;
		dev->buffer_infos[i].buffers[1] = NULL;
	}
	int output_buffer_index = num_dummy_inputs;
	for (int i = 0; i < num_dummy_outputs; ++i) {
		dev->buffer_infos[output_buffer_index + i].isInput = ASIOFalse;
		dev->buffer_infos[output_buffer_index + i].channelNum = i;
		dev->buffer_infos[output_buffer_index + i].buffers[0] = NULL;
		dev->buffer_infos[output_buffer_index + i].buffers[1] = NULL;
	}

	int num_channels = output_buffer_index + num_dummy_outputs;
	info("Creating dummy buffers: %d channels, size: %d", num_channels, dev->preferred_buffer_size);

	if (dev->preferred_buffer_size > 0) {
		ASIOError err = ASIO_CreateBuffers(dev, dev->buffer_infos, num_channels, dev->preferred_buffer_size,
						   &dev->callbacks);
		if (err != ASE_OK)
			warn("Dummy buffer creation failed with error %d", err);
		else
			dev->buffers_created = true;
	}

	long new_inputs = 0, new_outputs = 0;
	ASIO_GetChannels(dev, &new_inputs, &new_outputs);

	if (new_inputs > MAX_DEVICE_CHANNELS || new_outputs > MAX_DEVICE_CHANNELS) {
		info("Limiting to %d input + %d output channels max", MAX_DEVICE_CHANNELS, MAX_DEVICE_CHANNELS);
	}
	dev->total_num_input_chans = new_inputs > MAX_DEVICE_CHANNELS ? MAX_DEVICE_CHANNELS : new_inputs;
	dev->total_num_output_chans = new_outputs > MAX_DEVICE_CHANNELS ? MAX_DEVICE_CHANNELS : new_outputs;

	info("Detected channels after dummy buffers: %ld input, %ld output", dev->total_num_input_chans,
	     dev->total_num_output_chans);

	asio_device_update_sample_rates_list(dev);
	asio_device_reload_channel_names(dev);

	for (int i = 0; i < dev->total_num_output_chans; ++i) {
		ASIOChannelInfo chinfo = {0};
		chinfo.channel = i;
		chinfo.isInput = 0;
		ASIO_GetChannelInfo(dev, &chinfo);
		asio_format_init(&dev->output_format[i], chinfo.type);

		if (i < num_dummy_outputs) {
			asio_format_clear(&dev->output_format[i], dev->buffer_infos[output_buffer_index + i].buffers[0],
					  dev->preferred_buffer_size);
			asio_format_clear(&dev->output_format[i], dev->buffer_infos[output_buffer_index + i].buffers[1],
					  dev->preferred_buffer_size);
		}
	}
}

void asio_device_dispose_buffers(struct asio_device *dev)
{
	if (!dev)
		return;

	ASIOError err = ASE_OK;
	if (dev->asio != NULL && dev->buffers_created) {
		dev->buffers_created = false;
		err = ASIO_DisposeBuffers(dev);
	}
	if (err != ASE_OK)
		info("Device didn't dispose correctly of the buffers; error code %i", err);
}

/* Next function exists because of shi..y drivers which expect some loading sequence which should be unnecessary in
 * theory. But following JUCE & for extra safety, we do it anyway 'à la Cubase' */
void asio_device_test(struct asio_device *dev)
{
	if (!dev)
		return;

	info("opening device: %s", dev->device_name);
	os_atomic_set_bool(&dev->need_to_reset, false);

	clear_channel_names(dev->input_channel_names);
	clear_channel_names(dev->output_channel_names);

	da_clear(dev->buffer_sizes);
	da_clear(dev->sample_rates);

	dev->is_open = false;
	dev->total_num_input_chans = 0;
	dev->total_num_output_chans = 0;
	dev->xruns = 0;
	dev->errorstring[0] = '\0';

	ASIOError err = 0;

	if (asio_device_load_driver(dev)) {
		asio_device_init_driver(dev, dev->errorstring);
		if (!dev->errorstring[0]) {
			dev->total_num_input_chans = 0;
			dev->total_num_output_chans = 0;
			if (dev->asio != NULL) {
				err = ASIO_GetChannels(dev, &dev->total_num_input_chans, &dev->total_num_output_chans);
				if (err == ASE_OK) {
					info("channels in: %i, channels out: %i", dev->total_num_input_chans,
					     dev->total_num_output_chans);
					if (dev->total_num_input_chans > MAX_DEVICE_CHANNELS ||
					    dev->total_num_output_chans > MAX_DEVICE_CHANNELS)
						info("Only up to %i input + %i output channels are enabled. Higher channel counts are disabled.",
						     MAX_DEVICE_CHANNELS, MAX_DEVICE_CHANNELS);

					dev->total_num_input_chans =
						MIN(dev->total_num_input_chans, (long)MAX_DEVICE_CHANNELS);
					dev->total_num_output_chans =
						MIN(dev->total_num_output_chans, (long)MAX_DEVICE_CHANNELS);

					if (err = asio_device_refresh_buffer_sizes(dev) == ASE_OK) {
						info("buffer sizes: %ld → %ld, preferred: %ld, step: %ld",
						     dev->min_buffer_size, dev->max_buffer_size,
						     dev->preferred_buffer_size, dev->buffer_granularity);

						double current_rate = asio_device_get_sample_rate(dev);
						if (current_rate < 1.0 || current_rate > 192001.0) {
							info("setting default sample rate");
							err = ASIO_SetSampleRate(dev, 48000.0);
							info("force setting sample rate to 48 kHz");
							/* sanity check */
							current_rate = asio_device_get_sample_rate(dev);
						}
						dev->current_sample_rate = current_rate;

						dev->post_output = ASIO_OutputReady(dev) == ASE_OK;
						if (dev->post_output)
							info("outputReady true");

						asio_device_update_sample_rates_list(dev);

						/* series of steps inspired by cubase loading sequence because
						 * otherwise some devices fail to load ...*/
						asio_device_read_latencies(dev);
						asio_device_create_dummy_buffers(dev);
						asio_device_read_latencies(dev);
						err = ASIO_Start(dev);
						Sleep(80);
						ASIO_Stop(dev);
						// we dispose of buffers here and not in open function bc some drivers
						// can't set samplerate while buffers are prepared
						info("disposing of the dummy buffers");
						asio_device_dispose_buffers(dev);
					} else {
						info("Can't detect buffer sizes");
					}
				} else {
					info("Can't detect asio channels");
				}
			}
		} else {
			info("Initialization failure reported by driver:\n %s\nYour device is likely used concurrently "
			     "in another application, but ASIO usually supports a single host.\n",
			     dev->errorstring);
		}
	} else {
		info("No such device");
	}

	if (dev->errorstring[0] != '\0') {
		dev->driver_failure = true;
		if (!asio_device_remove_current_driver(dev))
			info("** Driver crashed while being closed");
	} else {
		info("device opened but not yet started");
	}

	dev->is_open = false;
	os_atomic_set_bool(&dev->need_to_reset, false);
}

void asio_device_open(struct asio_device *dev, double sample_rate, long buffer_size_samples)
{
	if (!dev) {
		blog(LOG_ERROR, "[ASIO] Invalid device handle in open()");
		return;
	}
	if (dev->is_open)
		asio_device_close(dev);

	DWORD current_thread_id = GetCurrentThreadId();
	if (current_thread_id != dev->com_thread_id) {
		error("open() called from the wrong thread! Expected COM thread ID: %lu, current: %lu\n",
		      dev->com_thread_id, current_thread_id);
		return;
	} else {
		debug("open() called from correct COM thread\n");
	}

	if (dev->asio == NULL) {
		asio_device_test(dev);
		if (dev->asio == NULL) {
			error("[Failed to load driver with error: %s", dev->errorstring);
			return;
		}
	}
	dev->is_started = false;
	dev->errorstring[0] = '\0';

	ASIOError err = ASIO_GetChannels(dev, &dev->total_num_input_chans, &dev->total_num_output_chans);
	if (dev->total_num_input_chans > MAX_DEVICE_CHANNELS || dev->total_num_output_chans > MAX_DEVICE_CHANNELS)
		info("Only up to %i input + %i output channels are enabled. Higher channel counts are disabled.",
		     MAX_DEVICE_CHANNELS, MAX_DEVICE_CHANNELS);

	dev->total_num_input_chans = MIN(dev->total_num_input_chans, (long)MAX_DEVICE_CHANNELS);
	dev->total_num_output_chans = MIN(dev->total_num_output_chans, (long)MAX_DEVICE_CHANNELS);

	/* Check if the driver supports the requested sample rate & set the rate; then set the clocks */
	double temptative_rate = sample_rate;
	dev->current_sample_rate = sample_rate;

	asio_device_update_sample_rates_list(dev);
	bool is_listed = false;
	for (size_t i = 0; i < dev->sample_rates.num; ++i) {
		if (dev->sample_rates.array[i] == sample_rate) {
			is_listed = true;
			break;
		}
	}

	if (sample_rate == 0.0 || !is_listed) {
		temptative_rate = 48000.0;
	}

	asio_device_update_clock_sources(dev);
	dev->current_sample_rate = asio_device_get_sample_rate(dev);
	asio_device_set_sample_rate(dev, temptative_rate);

	dev->current_block_size_samples = dev->current_buffer_size =
		asio_device_read_buffer_size(dev, buffer_size_samples);

	/* a sample rate change might impact the channel count, sigh... */
	err = ASIO_GetChannels(dev, &dev->total_num_input_chans, &dev->total_num_output_chans);
	assert(err == ASE_OK);

	if (dev->total_num_input_chans > MAX_DEVICE_CHANNELS || dev->total_num_output_chans > MAX_DEVICE_CHANNELS) {
		info("Only up to %d input + %d output channels are enabled. Higher channel counts are disabled.",
		     MAX_DEVICE_CHANNELS, MAX_DEVICE_CHANNELS);
	}
	dev->total_num_input_chans = MIN(dev->total_num_input_chans, (long)MAX_DEVICE_CHANNELS);
	dev->total_num_output_chans = MIN(dev->total_num_output_chans, (long)MAX_DEVICE_CHANNELS);

	if (ASIO_Future(dev, kAsioCanReportOverload, NULL) != ASE_OK)
		dev->xruns = -1;

	if (os_atomic_load_bool(&dev->need_to_reset)) {
		info("Resetting");

		if (!asio_device_remove_current_driver(dev))
			error("** Driver crashed while being closed");

		asio_device_load_driver(dev);

		char init_error[256] = {0};
		asio_device_init_driver(dev, init_error);

		if (init_error[0] != '\0') {
			error("ASIOInit: %s", init_error);
		} else {
			double rate = asio_device_get_sample_rate(dev);
			asio_device_set_sample_rate(dev, rate);
		}
		os_atomic_set_bool(&dev->need_to_reset, false);
	}

	/* buffers creation routine; if this fails, try a second time with preferredBufferSize*/
	long total_buffers = dev->total_num_input_chans + dev->total_num_output_chans;
	asio_device_reset_buffers(dev);

	info("creating buffers: %i in-out channels, size: %i samples", total_buffers, dev->current_block_size_samples);
	err = ASIO_CreateBuffers(dev, dev->buffer_infos, (long)total_buffers, dev->current_block_size_samples,
				 &dev->callbacks);

	if (err != ASE_OK) {
		dev->current_block_size_samples = dev->preferred_buffer_size;
		info("createBuffers failed, trying preferred size: %i", dev->current_block_size_samples);
		err = ASIO_DisposeBuffers(dev);
		err = ASIO_CreateBuffers(dev, dev->buffer_infos, (long)total_buffers, dev->current_block_size_samples,
					 &dev->callbacks);
		if (err != ASE_OK)
			info("createBuffers failed again, when trying preferred size: %i",
			     dev->current_block_size_samples);
	}
	if (err == ASE_OK) {
		dev->buffers_created = true;
		info("Buffers created successfully");
		/* allocation of input and output buffers */
		free(dev->io_buffer_space);
		dev->io_buffer_space =
			(float *)calloc(dev->current_block_size_samples * total_buffers + 32, sizeof(float));

		/* for devices like decklink having input only or output only, default format set to ASIOSTLastEntry */
		int input_type = ASIOSTLastEntry;
		int output_type = ASIOSTLastEntry;
		dev->current_bit_depth = 16;

		/* Set up input buffers and formats */
		for (int n = 0; n < dev->total_num_input_chans; ++n) {
			dev->in_buffers[n] = dev->io_buffer_space + (dev->current_block_size_samples * n);
			ASIOChannelInfo chinfo = {.channel = n, .isInput = ASIOTrue};
			ASIO_GetChannelInfo(dev, &chinfo);
			if (n == 0)
				input_type = chinfo.type;

			asio_format_init(&dev->input_format[n], chinfo.type);
			dev->current_bit_depth = MAX(dev->input_format[n].bit_depth, dev->current_bit_depth);
		}

		for (int n = 0; n < dev->total_num_output_chans; ++n) {
			dev->out_buffers[n] = dev->io_buffer_space +
					      (dev->current_block_size_samples * (dev->total_num_input_chans + n));
			ASIOChannelInfo chinfo = {.channel = n, .isInput = ASIOFalse};
			ASIO_GetChannelInfo(dev, &chinfo);

			if (n == 0)
				output_type = chinfo.type;

			asio_format_init(&dev->output_format[n], chinfo.type);
			dev->current_bit_depth = MAX(dev->output_format[n].bit_depth, dev->current_bit_depth);
		}

		char in_fmt[32], out_fmt[32];
		asio_device_get_sample_format(input_type, in_fmt);
		asio_device_get_sample_format(output_type, out_fmt);
		info("input sample format: %s, output sample format: %s\n ", in_fmt, out_fmt);

		for (int i = 0; i < dev->total_num_output_chans; ++i) {
			asio_format_clear(&dev->output_format[i],
					  dev->buffer_infos[dev->total_num_input_chans + i].buffers[0],
					  dev->current_block_size_samples);
			asio_format_clear(&dev->output_format[i],
					  dev->buffer_infos[dev->total_num_input_chans + i].buffers[1],
					  dev->current_block_size_samples);
		}

		/* start sequence */
		asio_device_read_latencies(dev);
		asio_device_refresh_buffer_sizes(dev);
		dev->is_open = true;

		info("starting");
		dev->called_back = false;

		ASIOError err = ASIO_Start(dev);

		if (err != ASE_OK) {
			dev->is_open = false;
			error("stop on failure");
			Sleep(10);
			ASIO_Stop(dev);
			error("Can't start device");
			Sleep(10);
			goto fail;
		} else {
			int count = 300;
			while (--count > 0 && !dev->called_back)
				Sleep(10);

			dev->is_started = true;

			if (!dev->called_back) {
				error("Device didn't start correctly\nNo callbacks - stopping.");
				ASIO_Stop(dev);
				goto fail;
			}
			dev->need_to_reset = false;
			return;
		}
	}
fail:
	asio_device_dispose_buffers(dev);
	Sleep(20);
	dev->is_started = false;
	dev->is_open = false;
	asio_device_close(dev);
	dev->need_to_reset = false;
}

void asio_device_close(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return;

	if (dev->asio != NULL && dev->is_open) {

		dev->is_open = false;
		dev->is_started = false;
		os_atomic_set_bool(&dev->need_to_reset, false);

		if (dev->obs_output_client) {
			os_atomic_set_bool(&dev->obs_output_client->stopping, true);
			for (int j = 0; j < MAX_DEVICE_CHANNELS; j++) {
				deque_free(&dev->output_frames[j]);
			}
		}

		info(" asio driver stopping");

		if (dev->asio != NULL) {
			Sleep(20);
			ASIO_Stop(dev);
			Sleep(10);
			asio_device_dispose_buffers(dev);
		}

		Sleep(10);
	}
}

bool asio_device_start(struct asio_device *dev)
{
	if (!dev || !dev->asio)
		return false;

	if (dev->is_started)
		return true;

	if (ASIO_Start(dev) != ASE_OK)
		return false;

	dev->is_started = true;
	return true;
}

void asio_device_stop(struct asio_device *dev)
{
	if (dev && dev->asio && dev->is_started) {
		ASIO_Stop(dev);
		dev->is_started = false;
	}
}

void asio_device_set_output_client(struct asio_device *dev, struct asio_data *client)
{
	if (dev)
		dev->obs_output_client = client;
}

struct asio_data *asio_device_get_output_client(struct asio_device *dev)
{
	return dev ? dev->obs_output_client : NULL;
}

/* message calback */
long asio_device_asio_message_callback(struct asio_device *dev, long selector, long value, void *message, double *opt)
{
	UNUSED_PARAMETER(message);
	UNUSED_PARAMETER(opt);
	switch (selector) {
	case kAsioSelectorSupported:
		if (value == kAsioResetRequest || value == kAsioEngineVersion || value == kAsioResyncRequest ||
		    value == kAsioLatenciesChanged || value == kAsioSupportsInputMonitor || value == kAsioOverload)
			return 1;
		break;

	case kAsioBufferSizeChange:
		info("kAsioBufferSizeChange; new buffer is %ld", value);
		return 0; /* 0 tells the driver to request a reset to the host */

	case kAsioResetRequest:
		info("kAsioResetRequest");
		asio_device_reset_request(dev);
		return 1;

	case kAsioResyncRequest:
		info("kAsioResyncRequest");
		asio_device_reset_request(dev);
		return 1;

	case kAsioLatenciesChanged:
		info("kAsioLatenciesChanged");
		return 1;

	case kAsioEngineVersion:
		return 2;

	case kAsioSupportsTimeInfo:
	case kAsioSupportsTimeCode:
		return 0;

	case kAsioOverload:
		dev->xruns++;
		return 1;
	}

	return 0;
}

/* main audio callback: split in 2 ==> asio_device_callback which implements some signalling and process_buffer which
 * does the actual processing. */
void asio_device_process_buffer(struct asio_device *dev, long buffer_index)
{
	if (!dev || !dev->is_started || buffer_index < 0)
		return;

	if (dev->in_buffers[0] == NULL && dev->out_buffers[0] == NULL)
		return;

	ASIOBufferInfo *infos = dev->buffer_infos;
	int samps = dev->current_block_size_samples;

	/* input: convert ASIO samples to float and feed OBS sources */
	for (int i = 0; i < dev->total_num_input_chans; ++i) {
		const void *src = infos[i].buffers[buffer_index];
		if (dev->in_buffers[i] && src) {
			asio_format_convert_to_float(&dev->input_format[i], infos[i].buffers[buffer_index],
						     dev->in_buffers[i], samps);
		}
	}

	struct obs_audio_info aoi;
	obs_get_audio_info(&aoi);
	int output_channels = (int)audio_output_get_channels(obs_get_audio());

	struct obs_source_audio out = {
		.speakers = aoi.speakers,
		.format = AUDIO_FORMAT_FLOAT_PLANAR,
		.samples_per_sec = (uint32_t)dev->current_sample_rate,
		.frames = samps,
		.timestamp = os_gettime_ns(),
	};
	/* pass audio to obs clients & then to the audio pipeline */
	for (int idx = 0; idx < dev->current_nb_clients; ++idx) {
		struct asio_data *client = dev->obs_clients[idx];
		if (!client || !client->device_name || !client->active)
			continue;

		for (int j = 0; j < output_channels; ++j) {
			int mix_idx = client->mix_channels[j];
			out.data[j] = (mix_idx >= 0 && !os_atomic_load_bool(&client->stopping))
					      ? (uint8_t *)dev->in_buffers[mix_idx]
					      : (uint8_t *)dev->silentBuffers8;
		}
		if (!os_atomic_load_bool(&client->stopping) && client->source && os_atomic_load_bool(&client->active))
			obs_source_output_audio(client->source, &out);
	}

	/* output: feed ASIO buffers with OBS output, ch per ch; the obs_output selects track channels */
	if (dev->obs_output_client) {
		for (int outchan = 0; outchan < dev->total_num_output_chans; ++outchan) {
			void *dst = infos[dev->total_num_input_chans + outchan].buffers[buffer_index];
			bool play = os_atomic_load_bool(&dev->capture_started) && dev->obs_track[outchan] >= 0 &&
				    dev->obs_track_channel[outchan] >= 0 &&
				    dev->output_frames[outchan].size >= samps * sizeof(float);
			if (play) {
				deque_pop_front(&dev->output_frames[outchan], dev->out_buffers[outchan],
						samps * sizeof(float));
				asio_format_convert_from_float(&dev->output_format[outchan], dev->out_buffers[outchan],
							       dst, samps);
			} else {
				asio_format_clear(&dev->output_format[outchan], dst, samps);
			}
		}
	} else {
		for (int i = 0; i < dev->total_num_output_chans; ++i) {
			void *dst = infos[dev->total_num_input_chans + i].buffers[buffer_index];
			asio_format_clear(&dev->output_format[i], dst, samps);
		}
	}

	if (dev->post_output)
		ASIO_OutputReady(dev);
}

void asio_device_callback(struct asio_device *dev, long index)
{
	if (!dev)
		return;

	if (dev->is_started && index >= 0) {
		if (os_atomic_load_bool(&shutting_down_atomic)) {
			os_atomic_set_bool(&dev->capture_started, false);
			os_event_signal(shutting_down);
			dev->is_started = false;
			return;
		} else {
			asio_device_process_buffer(dev, index);
		}
	} else {
		if (dev->post_output && dev->asio)
			ASIO_OutputReady(dev);
	}
	os_atomic_set_bool(&dev->called_back, true);
}
