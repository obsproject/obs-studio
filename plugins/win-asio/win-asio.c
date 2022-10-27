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

#include "win-asio.h"

#include "asio-device-list.h"

#include <util/threading.h>

#include <stdbool.h>
#include <string.h>

extern os_event_t *shutting_down;
extern volatile bool shutting_down_atomic;
struct asio_data *global_output_asio_data = NULL;
obs_data_t *global_output_settings = NULL;
struct asio_device_list *list = NULL;

/*==================================== DEVICE SCAN =====================================*/
void retrieve_device_list()
{
	list = asio_device_list_create();
}

void free_device_list()
{
	asio_device_list_destroy(list);
}

/*============================== COMMON TO INPUT & OUTPUT ==============================*/

#define ASIODATA_LOG(level, fmt, ...) \
    blog(level, "[%s '%s']: " fmt, \
         (data)->is_output ? "asio_output" : "asio_input", \
         (data)->is_output \
             ? ((data)->device_name ? (data)->device_name : "(none)") \
             : ((data)->source ? obs_source_get_name((data)->source) : "(null)"), \
         ##__VA_ARGS__)

#define debugdata(fmt, ...) ASIODATA_LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define infodata(fmt, ...)  ASIODATA_LOG(LOG_INFO,  fmt, ##__VA_ARGS__)
#define errordata(fmt, ...) ASIODATA_LOG(LOG_ERROR, fmt, ##__VA_ARGS__)
#define warndata(fmt, ...)  ASIODATA_LOG(LOG_WARNING, fmt, ##__VA_ARGS__)

bool attach_device(struct asio_data *data, const char *name)
{
	if (!name || !*name) {
		return false;
	}

	data->device_index = asio_device_list_get_index_from_driver_name(list, name);
	if (data->device_index < 0) {
		errordata("This driver was not found in the registry: %s", name);
		data->driver_loaded = false;
		data->device_name = NULL;
		return false;
	}

	/* We set the name even if the driver fails to load in order to be able to deal with disconnected devices. */
	if (data->device_name) {
		bfree((void *)data->device_name);
	}
	data->device_name = bstrdup(name);

	/* Retrieve the struct asio_device ptr and create the asio_device ptr if NULL. */
	data->asio_device = asio_device_list_attach_device(list, name);
	struct asio_device *dev = data->asio_device;
	if (!dev) {
		errordata("Failed to create device %s", name);
		return false;
	} else if (!dev->asio) {
		errordata(
			"Driver could not find a connected device or device might already be in use by another host.");
		data->asio_device = NULL;
		return false;
	}

	if (!data->is_output) {
		/* Update the device client list if the src was never a client and add src ptr as a client of ASIO device. */
		int client_slot = -1;

		for (int idx = 0; idx < MAX_ASIO_CLIENTS; ++idx) {
			if (!dev->obs_clients[idx]) {
				client_slot = idx;
				break;
			}
		}

		if (client_slot < 0) {
			errordata("Maximum number of ASIO clients reached");
			return false;
		}

		data->asio_client_index[data->device_index] = client_slot;
		dev->obs_clients[client_slot] = data;
		dev->current_nb_clients++;
	} else {
		dev->obs_output_client = data;
	}

	return true;
}

static void detach_device(struct asio_data *data, obs_data_t *settings)
{
	if (!data) {
		return;
	}
	const bool is_output = data->is_output;
	if (is_output) {

		for (int i = 0; i < MAX_DEVICE_CHANNELS; ++i) {
			data->output_routing[i] = -1;
			if (settings) {
				char key[32];
				snprintf(key, sizeof(key), "device_ch%d", i);
				obs_data_set_int(settings, key, -1);
			}
		}
	} else {
		for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			char key[32];
			snprintf(key, sizeof(key), "OBS.Channels.%d", i);

			data->mix_channels[i] = -1;
			if (settings) {
				obs_data_set_int(settings, key, -1);
			}
		}
	}

	data->device_index = -1;

	if (!data->asio_device) {
		return;
	}

	struct asio_device *dev = data->asio_device;

	if (is_output) {
		debugdata("Detached device %s", data->device_name);
		infodata("Device removed; xruns=%d. (-1 means device doesn't report xruns)", dev->xruns);
		os_atomic_set_bool(&dev->capture_started, false);
		/* Close device if no clients remain */
		if (dev->current_nb_clients == 0) {
			asio_device_close(dev);
		}
		for (int i = 0; i < MAX_DEVICE_CHANNELS; ++i) {
			dev->output_routes[i].mix_index = -1;
			dev->output_routes[i].channel_index = -1;
		}

		dev->obs_output_client = NULL;

		return;
	}

	int prev_dev_idx = asio_device_list_get_index_from_driver_name(list, data->device_name);
	if (prev_dev_idx < 0) {
		return;
	}

	int client_slot = data->asio_client_index[prev_dev_idx];
	if (client_slot < 0 || client_slot >= MAX_ASIO_CLIENTS || dev->obs_clients[client_slot] != data) {
		errordata("Invalid ASIO client slot %d", client_slot);
		return;
	}

	dev->obs_clients[client_slot] = NULL;
	data->asio_client_index[prev_dev_idx] = -1;
	dev->current_nb_clients--;

	if (dev->current_nb_clients == 0 && !dev->obs_output_client) {
		infodata("Device %s removed; xruns=%d. (-1 means xruns not reported). "
			 "Increase buffer if you hear cracks/pops.",
			 data->device_name, dev->xruns);
		if (dev->xruns > 0) {
			infodata("XRuns detected: %d. Increase your buffer size if needed.", dev->xruns);
		}
		asio_device_close(dev);
	}
}

static inline bool strdiff(const char *a, const char *b)
{
	if (!a && !b) {
		return false;
	}
	if (!a || !b) {
		return true;
	}
	return strcmp(a, b) != 0;
}

static struct asio_output_route decode_output_route(int64_t encoded_route, int channel_count)
{
	struct asio_output_route route = {
		.mix_index = -1,
		.channel_index = -1,
	};

	if (encoded_route < 0) {
		return route;
	}

	for (int mix_index = 0; mix_index < (MAX_AUDIO_MIXES + 1); ++mix_index) {
		const int64_t first_route = 1LL << (mix_index + 4);
		const int64_t channel_index = encoded_route - first_route;

		if (channel_index >= 0 && channel_index < channel_count) {
			route.mix_index = mix_index;
			route.channel_index = (int)channel_index;
			break;
		}
	}

	return route;
}

static void asio_input_update(void *vptr, obs_data_t *settings);
static void asio_output_update(void *vptr, obs_data_t *settings);
static void asio_update(void *vptr, obs_data_t *settings, bool is_output)
{
	struct asio_data *data = NULL;

	if (is_output && !global_output_settings) {
		global_output_settings = settings;
		obs_data_addref(global_output_settings);
	}

	if (!is_output) {
		data = (struct asio_data *)vptr;
	} else {
		UNUSED_PARAMETER(vptr);
		data = global_output_asio_data;
	}

	const char *new_device = obs_data_get_string(settings, "device_name");
	/* if new device is "" (== no device), return */
	if (!new_device || !*new_device) {
		detach_device(data, settings);
		/* we might actually be swapping from a 'device' to "" */
		if (data->device_name && *data->device_name) {
			bfree((void *)data->device_name);
			data->device_name = NULL;
		}
		data->driver_loaded = false;
		data->update_channels = false;
		data->asio_device = NULL;
		data->initial_update = false;
		return;
	}

	/* we are loading an asio_data which had already settings */
	if (data->initial_update && new_device) {
		data->driver_loaded = attach_device(data, new_device);
		/* If the driver fails to load, we keep the name but the driver will be greyed out on next properties call. */
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			data->initial_update = false;
			return;
		}
	}

	const bool device_changed = strdiff(data->device_name, new_device);

	/* we swap from a 'device' to a 'new device' */
	if (!data->initial_update && (device_changed || !data->driver_loaded || !data->asio_device)) {
		if (device_changed) {
			detach_device(data, settings);
		}

		data->driver_loaded = attach_device(data, new_device);
		data->update_channels = data->driver_loaded;
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			return;
		}
	}

	struct asio_device *dev = data->asio_device;
	if (!dev) {
		data->initial_update = false;
		return;
	}

	if (!dev->is_open) {
		double obs_sample_rate = audio_output_get_sample_rate(obs_get_audio());

		int buffer_size = asio_device_get_preferred_buffer_size(dev);
		if (buffer_size < 16) {
			buffer_size = 512;
		}

		asio_device_open(dev, obs_sample_rate, buffer_size);
		if (dev->is_open) {
			infodata("\nconnected to device %s;"
				 "\n\tcurrent sample rate: %f,"
				 "\n\tcurrent buffer: %i,"
				 "\n\tinput latency: %f ms\n",
				 data->device_name, dev->current_sample_rate, dev->current_buffer_size,
				 1000.0f * (float)dev->input_latency / dev->current_sample_rate);
		}
	}
	data->initial_update = false;

	/*--- Input path ---*/
	if (!is_output) {
		/* update the routing */
		for (int i = 0; i < data->out_channels; i++) {
			char key[32];
			snprintf(key, sizeof(key), "OBS.Channels.%d", i);

			long long mix_channel = obs_data_get_int(settings, key);

			if (mix_channel >= 0 && mix_channel < dev->total_num_input_chans) {
				data->mix_channels[i] = (int)mix_channel;
			} else {
				data->mix_channels[i] = -1;

				if (mix_channel != -1) {
					obs_data_set_int(settings, key, -1);
				}
			}
		}
		return;
	}

	/*--- Output path ---*/
	data->out_channels = (uint8_t)data->asio_device->total_num_output_chans;

	/* update the routing data for each output device channels & pass the info to the device */
	for (int i = 0; i < data->out_channels; ++i) {
		char key[32];
		snprintf(key, sizeof(key), "device_ch%d", i);
		const int64_t encoded_route = obs_data_get_int(settings, key);
		if (data->output_routing[i] != encoded_route) {
			data->output_routing[i] = encoded_route;
			const struct asio_output_route route =
				decode_output_route(encoded_route, data->obs_track_channels);
			data->asio_device->output_routes[i] = route;
			if (route.mix_index >= 0) {
				blog(LOG_DEBUG, "[asio_output]:\nDevice output channel n° %i: Track %i, Channel %i", i,
				     route.mix_index + 1, route.channel_index + 1);
			}
		}
	}
}

static void *asio_create_internal(obs_data_t *settings, void *owner, bool is_output)
{
	struct asio_data *data = bzalloc(sizeof(struct asio_data));
	if (!data) {
		return NULL;
	}
	data->asio_device = NULL;
	data->device_name = NULL;
	data->update_channels = false;
	data->driver_loaded = false;
	data->initial_update = true;
	data->is_output = is_output;
	os_atomic_set_bool(&data->stopping, false);
	os_atomic_set_bool(&data->active, true);

	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; i++) {
		data->asio_client_index[i] = -1; // not a client if negative;
	}

	if (is_output) {
		data->source = NULL;
		data->output = (obs_output_t *)owner;
		data->obs_track_channels = (uint8_t)audio_output_get_channels(obs_get_audio());

		/* default value is negative, which implies no processing */
		for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
			data->output_routing[i] = -1;
		}

		/* allow all tracks for ASIO output */
		obs_output_set_mixers(data->output, (1 << MAX_AUDIO_MIXES) - 1);
		global_output_asio_data = data;
	} else {
		data->output = NULL;
		data->source = (obs_source_t *)owner;
		data->out_channels = (uint8_t)audio_output_get_channels(obs_get_audio());

		for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			data->mix_channels[i] = -1;
		}

		infodata("Source created successfully.");
	}
	is_output ? asio_output_update(data, settings) : asio_input_update(data, settings);

	return data;
}

static void asio_destroy(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;

	if (!data) {
		return;
	}

	os_atomic_set_bool(&data->stopping, true);

	if (!os_atomic_load_bool(&shutting_down_atomic)) {
		if (data->asio_device) {
			detach_device(data, NULL);
		}
	}

	if (data->device_name) {
		bfree((void *)data->device_name);
	}

	data->asio_device = NULL;

	if (data->is_output && global_output_asio_data == data) {
		obs_data_release(global_output_settings);
		global_output_asio_data = NULL;
		global_output_settings = NULL;
	}

	bfree(data);
}

static bool display_control_panel_input(obs_properties_t *props, obs_property_t *property, void *vptr);
static bool display_control_panel_output(obs_properties_t *props, obs_property_t *property, void *vptr);
static bool display_control_panel(obs_properties_t *props, obs_property_t *property, void *vptr, bool is_output)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct asio_data *data = NULL;
	struct asio_device *dev = NULL;

	if (!is_output) {
		if (!vptr) {
			return false;
		}

		data = (struct asio_data *)vptr;
	} else {
		UNUSED_PARAMETER(vptr);
		data = global_output_asio_data;
	}

	dev = data->asio_device;
	if (dev) {
		asio_device_show_control_panel(dev);
	} else {
		return false;
	}

	return true;
}

static bool on_reset_input_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr);
static bool on_reset_output_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr);
static bool on_reset_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr, bool is_output)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct asio_data *data = NULL;
	struct asio_device *dev = NULL;

	if (!is_output) {
		if (!vptr) {
			return false;
		}

		data = (struct asio_data *)vptr;
	} else {
		UNUSED_PARAMETER(vptr);
		data = global_output_asio_data;
	}

	dev = data->asio_device;

	if (dev) {
		asio_device_reset_request(dev);
		return true;
	}

	obs_data_t *settings = NULL;

	if (is_output) {
		settings = global_output_settings;

	} else if (data->source) {
		settings = obs_source_get_settings(data->source);
	}

	if (!settings) {
		return false;
	}

	asio_update(data, settings, is_output);

	if (!is_output) {
		obs_data_release(settings);
	}
	if (is_output && data->asio_device && data->asio_device->is_open &&
	    os_atomic_load_bool(&data->asio_device->is_started) && !obs_output_active(data->output)) {
		obs_output_start(data->output);
	}

	return data->asio_device != NULL;
}

static void asio_defaults(obs_data_t *settings)
{
	if (!settings) {
		return;
	}

	obs_data_set_default_string(settings, "device_name", NULL);

	/* Set default channel routing (-1 means unassigned/muted) */
	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
		char key[32];
		snprintf(key, sizeof(key), "OBS.Channels.%d", i);
		obs_data_set_default_int(settings, key, -1);
	}

	for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
		char key[32];
		snprintf(key, sizeof(key), "device_ch%d", i);
		obs_data_set_default_int(settings, key, -1);
	}
}

static bool on_asio_device_changed(void *priv, obs_properties_t *props, obs_property_t *list_prop,
				   obs_data_t *cur_settings)
{
	struct asio_data *data = (struct asio_data *)priv;
	const bool is_output = data->is_output;
	obs_data_t *settings = cur_settings;

	if (is_output) {
		if (!global_output_settings) {
			return false;
		}
		settings = global_output_settings;
	}

	const char *device_name = obs_data_get_string(settings, "device_name");
	if (!device_name) {
		return false;
	}

	struct asio_device *dev = asio_device_find_by_name(device_name);
	const size_t count = list ? list->count : 0;

	if (!dev || !dev->is_open) {
		const int max_channels = is_output ? MAX_DEVICE_CHANNELS : MAX_AUDIO_CHANNELS;

		for (int i = 0; i < max_channels; i++) {
			char key[64];
			snprintf(key, sizeof(key), is_output ? "device_ch%d" : "OBS.Channels.%d", i);
			obs_data_set_int(settings, key, -1);
			obs_property_t *p = obs_properties_get(props, key);
			if (!p) {
				continue;
			}

			if (is_output) {
				obs_properties_remove_by_name(props, key);
			} else {
				obs_property_list_clear(p);
				obs_property_list_add_int(p, obs_module_text("Mute"), -1);
			}
		}
		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, (count && *device_name));

		asio_update(data, settings, is_output);
		return true;
	}

	if (data->update_channels) {
		if (is_output) {
			for (int i = 0; i < MAX_DEVICE_CHANNELS; ++i) {
				char key[32];
				snprintf(key, sizeof(key), "device_ch%d", i);
				obs_data_set_int(settings, key, -1);
				obs_properties_remove_by_name(props, key);
			}

			for (int i = 0; i < dev->total_num_output_chans; ++i) {
				char key[32];
				snprintf(key, sizeof(key), "device_ch%d", i);
				obs_property_t *p = obs_properties_add_list(props, key, dev->output_channel_names[i],
									    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

				obs_property_list_add_int(p, obs_module_text("Mute"), -1);

				for (int j = 0; j < MAX_AUDIO_MIXES; j++) {
					for (int k = 0; k < global_output_asio_data->obs_track_channels; k++) {
						long long idx = k + (1ULL << (j + 4));
						char label[32];
						snprintf(label, sizeof(label), "Track%d.%d", j, k);
						obs_property_list_add_int(p, obs_module_text(label), idx);
					}
				}
			}
		} else {
			for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
				char key[32];
				snprintf(key, sizeof(key), "OBS.Channels.%d", i);
				obs_data_set_int(settings, key, -1);
				obs_property_t *p = obs_properties_get(props, key);
				if (!p) {
					continue;
				}

				obs_property_list_clear(p);
				obs_property_list_add_int(p, obs_module_text("Mute"), -1);
				for (int j = 0; j < dev->total_num_input_chans; ++j) {
					obs_property_list_add_int(p, dev->input_channel_names[j], j);
				}
			}
		}

		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, false);

		data->update_channels = false;
		asio_update(data, settings, is_output); // required to update to the mute values !
	}

	return true;
}

static obs_properties_t *asio_properties_internal(void *vptr, bool is_output)
{
	obs_properties_t *props = obs_properties_create();
	size_t count = list ? list->count : 0;

	struct asio_data *data = NULL;
	if (is_output) {
		UNUSED_PARAMETER(vptr);
		data = global_output_asio_data;
	} else if (!is_output) {
		data = (struct asio_data *)vptr;
	}

	struct asio_device *dev = data ? data->asio_device : NULL;

	obs_property_t *device_property = obs_properties_add_list(props, "device_name", obs_module_text("ASIO.Driver"),
								  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(device_property, obs_module_text("Select.Device"), "");

	for (size_t i = 0; i < count; ++i) {
		const char *name = asio_device_list_get_name(list, i);
		const size_t idx = obs_property_list_add_string(device_property, name, name);
		if (!list->drivers[i].loadable) {
			obs_property_list_item_disable(device_property, idx, true);
		}
	}

	obs_property_set_modified_callback2(device_property, on_asio_device_changed, data);

	obs_property_t *panel = obs_properties_add_button2(
		props, "ctrl", obs_module_text("Control.Panel"),
		is_output ? display_control_panel_output : display_control_panel_input, vptr);
	obs_property_set_long_description(panel, obs_module_text("Control.Panel.Hint"));

	obs_property_t *reset = obs_properties_add_button2(
		props, "reset_button", obs_module_text("Reset.Device"),
		is_output ? on_reset_output_device_clicked : on_reset_input_device_clicked, vptr);
	obs_property_set_long_description(reset, obs_module_text("Reset.Device.Hint"));

	if (is_output) {
		obs_property_t *warning = obs_properties_add_text(props, "hint", NULL, OBS_TEXT_INFO);
		obs_property_text_set_info_type(warning, OBS_TEXT_INFO_WARNING);
		obs_property_set_long_description(warning, obs_module_text("ASIO.Output.Hint"));
	}

	if (!is_output) {
		int obs_channels = (int)audio_output_get_channels(obs_get_audio());
		for (int i = 0; i < obs_channels; ++i) {
			char key[64];
			snprintf(key, sizeof(key), "OBS.Channels.%d", i);
			obs_property_t *lp = obs_properties_add_list(props, key, obs_module_text(key),
								     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_list_add_int(lp, obs_module_text("Mute"), -1);
			if (dev) {
				for (int j = 0; j < dev->total_num_input_chans; ++j) {
					obs_property_list_add_int(lp, dev->input_channel_names[j], j);
				}
			}
		}
	} else {
		int dev_out_channels = dev ? dev->total_num_output_chans : -1;
		if (dev_out_channels >= 0) {
			for (int i = 0; i < dev_out_channels; i++) {
				char key[32];
				snprintf(key, sizeof(key), "device_ch%d", i);
				obs_property_t *lp = obs_properties_add_list(props, key, dev->output_channel_names[i],
									     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
				obs_property_list_add_int(lp, obs_module_text("Mute"), -1);

				for (int j = 0; j < MAX_AUDIO_MIXES; j++) {
					for (int k = 0; k < global_output_asio_data->obs_track_channels; k++) {
						long long idx = k + (1ULL << (j + 4));
						char label[32];
						snprintf(label, sizeof(label), "Track%d.%d", j, k);
						obs_property_list_add_int(lp, obs_module_text(label), idx);
					}
				}
			}
		}
	}

	obs_property_t *error = obs_properties_add_text(props, "error", NULL, OBS_TEXT_INFO);
	obs_property_text_set_info_type(error, OBS_TEXT_INFO_ERROR);
	obs_property_set_long_description(error, obs_module_text("ASIO.Driver.Error"));
	obs_property_set_visible(error, false);

	obs_property_t *no_asio = obs_properties_add_text(props, "noasio", NULL, OBS_TEXT_INFO);
	obs_property_text_set_info_type(no_asio, OBS_TEXT_INFO_ERROR);
	obs_property_set_long_description(no_asio, obs_module_text("ASIO.Driver.None"));
	obs_property_set_visible(no_asio, !count);

	return props;
}

/*===================================== ASIO INPUT =====================================*/
static const char *asio_input_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ASIO.Input.Capture");
}

static void asio_input_update(void *vptr, obs_data_t *settings)
{

	asio_update(vptr, settings, false);
}

static void *asio_input_create(obs_data_t *settings, obs_source_t *source)
{
	return asio_create_internal(settings, source, false);
}

static bool display_control_panel_input(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	return display_control_panel(props, property, vptr, false);
}

static bool on_reset_input_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	return on_reset_device_clicked(props, property, vptr, false);
}

static obs_properties_t *asio_input_properties(void *vptr)
{
	return asio_properties_internal(vptr, false);
}

static void asio_input_activate(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;
	os_atomic_set_bool(&data->active, true);
}

static void asio_input_deactivate(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;
	os_atomic_set_bool(&data->active, false);
}

struct obs_source_info asio_input_capture = {
	.id = "asio_input_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = asio_input_getname,
	.create = asio_input_create,
	.destroy = asio_destroy,
	.update = asio_input_update,
	.get_defaults = asio_defaults,
	.get_properties = asio_input_properties,
	.icon_type = OBS_ICON_TYPE_AUDIO_INPUT,
	.activate = asio_input_activate,
	.deactivate = asio_input_deactivate,
};

/*==================================== ASIO OUTPUT =====================================*/

static const char *asio_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ASIO.Output");
}

static void asio_output_update(void *vptr, obs_data_t *settings)
{

	asio_update(vptr, settings, true);
}

static void *asio_output_create(obs_data_t *settings, obs_output_t *output)
{
	if (global_output_asio_data) {
		blog(LOG_ERROR, "[asio_output] Only one ASIO output instance is supported");
		return NULL;
	}
	return asio_create_internal(settings, output, true);
}

bool asio_output_start(void *vptr)
{
	struct asio_data *data = vptr;

	if (!data || !data->asio_device) {
		return false;
	}

	struct asio_device *dev = data->asio_device;

	if (!dev->is_open || !os_atomic_load_bool(&dev->is_started) || !(dev->current_sample_rate > 0.0)) {
		return false;
	}

	if (!obs_output_can_begin_data_capture(data->output, 0)) {
		return false;
	}

	os_atomic_set_bool(&dev->capture_started, false);

	if (!asio_device_clear_output_fifos(dev)) {
		return false;
	}

	struct obs_audio_info oai;
	obs_get_audio_info(&oai);

	/* Audio is always planar for asio so we need obs to convert to planar format. */
	struct audio_convert_info aci = {.format = AUDIO_FORMAT_FLOAT_PLANAR,
					 .speakers = oai.speakers,
					 .samples_per_sec = (uint32_t)data->asio_device->current_sample_rate};

	obs_output_set_audio_conversion(data->output, &aci);

	if (!obs_output_begin_data_capture(data->output, 0)) {
		return false;
	}
	os_atomic_set_bool(&data->stopping, false);
	os_atomic_set_bool(&dev->capture_started, true);
	return true;
}

static void asio_output_stop(void *vptr, uint64_t ts)
{
	UNUSED_PARAMETER(ts);

	struct asio_data *data = vptr;
	if (!data) {
		return;
	}

	if (data->asio_device) {
		os_atomic_set_bool(&data->asio_device->capture_started, false);
	}

	obs_output_end_data_capture(data->output);
}

static void asio_receive_audio(void *vptr, size_t mix_idx, struct audio_data *frame)
{
	UNUSED_PARAMETER(vptr);
	if (!frame || !global_output_asio_data) {
		return;
	}

	struct asio_data *data = global_output_asio_data;
	struct audio_data in = *frame;

	if (!data->asio_device) {
		return;
	}
	struct asio_device *dev = data->asio_device;

	if (os_atomic_load_bool(&shutting_down_atomic) || os_atomic_load_bool(&data->stopping) ||
	    !os_atomic_load_bool(&dev->capture_started)) {
		return;
	}

	for (int i = 0; i < dev->total_num_output_chans; ++i) {
		if (dev->output_routes[i].mix_index == (int)mix_idx) {
			long written_frames = (long)in.frames;
			long writable_frames = spsc_fifo_writable_frames(&dev->output_frames[i]);
			if (written_frames > writable_frames) {
				written_frames = writable_frames;
			}
			int channel_index = dev->output_routes[i].channel_index;
			spsc_fifo_write(&dev->output_frames[i], (const float *)in.data[channel_index], written_frames);
		}
	}
}

static bool display_control_panel_output(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	return display_control_panel(props, property, vptr, true);
}

static bool on_reset_output_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	return on_reset_device_clicked(props, property, vptr, true);
}

static obs_properties_t *asio_output_properties(void *vptr)
{
	return asio_properties_internal(vptr, true);
}

struct obs_output_info asio_output = {
	.id = "asio_output",
	.flags = OBS_OUTPUT_AUDIO | OBS_OUTPUT_MULTI_TRACK,
	.get_name = asio_output_getname,
	.create = asio_output_create,
	.destroy = asio_destroy,
	.start = asio_output_start,
	.stop = asio_output_stop,
	.update = asio_output_update,
	.get_defaults = asio_defaults,
	.get_properties = asio_output_properties,
	.raw_audio2 = asio_receive_audio,
};
