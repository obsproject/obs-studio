/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#include "asio-device-list.h"

extern os_event_t *shutting_down;
extern volatile bool shutting_down_atomic;
struct asio_data *global_output_asio_data;
obs_data_t *global_output_settings;
struct asio_device_list *list = NULL;

#define ASIODATA_LOG(level, format, ...) \
	blog(level, "[asio_plugin '%s']: " format, obs_source_get_name(data->source), ##__VA_ARGS__)
#define debugd(format, ...) ASIODATA_LOG(LOG_DEBUG, format, ##__VA_ARGS__)
#define errord(format, ...) ASIODATA_LOG(LOG_ERROR, format, ##__VA_ARGS__)
#define warnd(format, ...) ASIODATA_LOG(LOG_WARNING, format, ##__VA_ARGS__)
#define infod(format, ...) ASIODATA_LOG(LOG_INFO, format, ##__VA_ARGS__)

static const char *asio_input_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ASIO.Input.Capture");
}

bool retrieve_device_list()
{
	list = asio_device_list_create();
	return list->count > 0;
}

void free_device_list()
{
	asio_device_list_destroy(list);
}

/* This creates the device if it hasn't been created by this or another source
  or retrieves its pointer if it already exists. The source is also added as
  a client of the device. */
bool attach_device(struct asio_data *data, const char *name)
{
	if (!name || !*name)
		return false;

	int dev_index = asio_device_list_get_index_from_driver_name(list, name);
	if (dev_index < 0)
		return false;

	// we set the name even if the driver fails to load
	if (data->device_name)
		bfree((void *)data->device_name);

	data->device_name = bstrdup(name);

	data->asio_device = asio_device_list_attach_device(list, name);

	if (!data->asio_device) {
		errord("\nFailed to create device % s ", name);
		return false;
	} else if (!data->asio_device->asio) {
		errord("\nDriver could not find a connected device or device might already be in use by another app.");
		data->asio_device = NULL;
		return false;
	} else {
		struct asio_device *dev = data->asio_device;

		// increment the device client list if the source was never a client
		// & add source ptr as a client of asio device.
		int nb_clients = dev->current_nb_clients;
		data->asio_client_index[dev_index] = nb_clients;
		dev->obs_clients[nb_clients] = data;
		dev->current_nb_clients++;

		// update other fields
		data->device_index = dev_index;

		// log some info about the device
		infod("\nsource added to device %s;\n\tcurrent sample rate: %f,"
		      "\n\tcurrent buffer: %i,"
		      "\n\tinput latency: %f ms\n",
		      obs_source_get_name(data->source), name, dev->current_sample_rate, dev->current_buffer_size,
		      1000.0f * (float)dev->input_latency / dev->current_sample_rate);
	}
	return true;
}

static void detach_device(struct asio_data *data)
{
	if (!data)
		return;

	if (!data->asio_device)
		return;

	struct asio_device *dev = data->asio_device;

	int prev_dev_idx = asio_device_list_get_index_from_driver_name(list, data->device_name);
	if (prev_dev_idx < 0)
		return;

	int prev_client_idx = data->asio_client_index[prev_dev_idx];

	if (dev != NULL) {
		if (!dev->is_open || !dev->current_nb_clients || prev_client_idx < 0)
			return;

		dev->obs_clients[prev_client_idx] = NULL;
		dev->current_nb_clients--;
		data->device_index = -1;

		if (dev->current_nb_clients == 0 && !dev->obs_output_client) {
			infod("\n\tDevice % s removed;\n\tnumber of xruns: % i;\n\t"
			      " -1 means xruns are not reported by your device. Increase your buffer if you get "
			      "a high count & hear cracks, pops or else !\n ",
			      data->device_name, dev->xruns);
			if (dev->xruns > 0) {
				infod("\tnumber of xruns: % i;\n\tIncrease your buffer if you get a high count & hear cracks,"
				      "pops or else !\n ",
				      dev->xruns);
			}
			asio_device_close(dev);
		}
	}
}

static inline bool strdiff(const char *a, const char *b)
{
	if (!a && !b)
		return false; // both null → equal
	if (!a || !b)
		return true; // one null → different
	return strcmp(a, b) != 0;
}

static void asio_input_update(void *vptr, obs_data_t *settings)
{
	struct asio_data *data = (struct asio_data *)vptr;
	const char *new_device = obs_data_get_string(settings, "device_name");

	// new device is 'none' -> return
	if (!new_device || !*new_device) {
		// we are actually swapping from a 'device' -> 'none'
		if (data->device_name && *data->device_name) {
			detach_device(data);
			data->device_name = NULL;
		}
		data->asio_device = NULL;
		data->initial_update = false;
		return;
	}

	// we are loading a source which had already settings
	if (data->initial_update && new_device) {
		data->driver_loaded = attach_device(data, new_device);
		// If the driver fails to load, we keep the name but the driver will be greyed out on next properties call.
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			data->initial_update = false;
			return;
		}
	}

	// we swap from a 'device' to a 'new device'
	if (!data->initial_update && strdiff(data->device_name, new_device)) {
		if (data->device_name && *data->device_name && data->driver_loaded)
			detach_device(data);

		data->driver_loaded = attach_device(data, new_device);
		data->update_channels = data->driver_loaded;
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			return;
		}
	}

	// open the device if it is not already open
	struct asio_device *dev = data->asio_device;
	if (!dev) {
		data->initial_update = false;
		return;
	}

	if (!dev->is_open) {
		double sr = asio_device_get_sample_rate(dev);
		int buffer_size = asio_device_get_preferred_buffer_size(dev);
		if (sr <= 0)
			sr = 48000;

		if (buffer_size < 16)
			buffer_size = 512;

		asio_device_open(dev, sr, buffer_size);
	}

	// update the routing
	for (int i = 0; i < data->out_channels; i++) {
		char key[32];
		snprintf(key, sizeof(key), "OBS.Channels.%d", i);
		data->mix_channels[i] = (int)obs_data_get_int(settings, key);
	}
	data->initial_update = false;
}

static void *asio_input_create(obs_data_t *settings, obs_source_t *source)
{
	struct asio_data *data = (struct asio_data *)bzalloc(sizeof(struct asio_data));
	data->source = source;
	data->asio_device = NULL;
	data->output = NULL;
	data->device_name = NULL;
	data->update_channels = false;
	data->driver_loaded = false;
	data->initial_update = true;
	data->out_channels = (uint8_t)audio_output_get_channels(obs_get_audio());
	os_atomic_set_bool(&data->stopping, false);
	os_atomic_set_bool(&data->active, true);

	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; i++)
		data->asio_client_index[i] = -1; // not a client if negative;

	for (int i = 0; i < MAX_AUDIO_CHANNELS; i++)
		data->mix_channels[i] = -1;

	asio_input_update(data, settings);
	infod(" source created successfully.\n");

	return data;
}

static void asio_input_destroy(void *vptr)
{
	struct asio_data *data = (struct asio_data *)vptr;

	if (!data)
		return;

	/* delete the asio source from clients of asio device */
	if (data->device_name)
		bfree((void *)data->device_name);

	os_atomic_set_bool(&data->stopping, true);
	data->asio_device = NULL;
	bfree(data);
}

static bool display_control_panel(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	if (!vptr)
		return false;

	struct asio_data *data = (struct asio_data *)vptr;
	struct asio_device *dev = data->asio_device;

	if (dev)
		asio_device_show_control_panel(dev);
	else
		return false;

	return true;
}

static bool on_reset_device_clicked(obs_properties_t *props, obs_property_t *property, void *vptr)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	if (!vptr)
		return false;

	struct asio_data *data = (struct asio_data *)vptr;
	struct asio_device *dev = data->asio_device;

	if (dev)
		asio_device_reset_request(dev);
	else
		return false;

	return true;
}

static bool on_asio_input_device_changed(void *vptr, obs_properties_t *props, obs_property_t *property,
					 obs_data_t *settings)
{
	const char *device_name = obs_data_get_string(settings, "device_name");
	if (!device_name || !*device_name)
		return false;

	struct asio_data *data = (struct asio_data *)vptr;

	// Find the device so we can repopulate the channel lists
	struct asio_device *dev = asio_device_find_by_name(device_name);

	if (!dev) {
		for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			char key[32];
			snprintf(key, sizeof(key), "OBS.Channels.%d", i);
			obs_data_set_int(settings, key, -1);
			obs_property_t *p = obs_properties_get(props, key);
			if (!p)
				continue;

			obs_property_list_clear(p);
			obs_property_list_add_int(p, obs_module_text("Mute"), -1);
		}
		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, true);

		/* Ensure routing cleared */
		asio_input_update(data, settings);
		data->update_channels = false;
		return true;
	}

	// For each OBS output channel, update its dropdown list
	if (data->update_channels) {
		for (int i = 0; i < MAX_AUDIO_CHANNELS; i++) {
			char key[32];
			snprintf(key, sizeof(key), "OBS.Channels.%d", i);
			obs_data_set_int(settings, key, -1);
			obs_property_t *p = obs_properties_get(props, key);
			if (!p)
				continue;

			obs_property_list_clear(p);
			obs_property_list_add_int(p, obs_module_text("Mute"), -1);

			for (int j = 0; j < dev->total_num_input_chans; ++j) {
				obs_property_list_add_int(p, dev->input_channel_names[j], j);
			}
		}
		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, false);
		data->update_channels = false;
		asio_input_update(data, settings);
	}

	return true;
}

static obs_properties_t *asio_input_properties(void *vptr)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *panel;
	obs_property_t *reset;
	size_t count = list->count;
	struct asio_device *dev = NULL;
	struct asio_data *data = (struct asio_data *)vptr;
	dev = data->asio_device;

	p = obs_properties_add_list(props, "device_name", obs_module_text("ASIO.Driver"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, obs_module_text("Select.Device"), "");
	obs_property_set_modified_callback2(p, on_asio_input_device_changed, vptr);
	for (size_t i = 0; i < count; ++i) {
		const char *name = asio_device_list_get_name(list, i);
		obs_property_list_add_string(p, name, name);
		if (!list->drivers[i].loadable)
			obs_property_list_item_disable(p, i + 1, true);
	}

	int obs_channels = (int)audio_output_get_channels(obs_get_audio());

	for (int i = 0; i < obs_channels; ++i) {
		char name[64];
		snprintf(name, sizeof(name), "OBS.Channels.%d", i);
		p = obs_properties_add_list(props, name, obs_module_text(name), OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_INT);
		obs_property_list_add_int(p, "Mute", -1);
		if (dev) {
			for (int j = 0; j < dev->total_num_input_chans; ++j) {
				obs_property_list_add_int(p, dev->input_channel_names[j], j);
			}
		}
	}
	panel = obs_properties_add_button2(props, "ctrl", obs_module_text("Control.Panel"), display_control_panel,
					   vptr);
	obs_property_set_long_description(panel, obs_module_text("Control.Panel.Hint"));
	reset = obs_properties_add_button2(props, "reset_button", obs_module_text("Reset.Device"),
					   on_reset_device_clicked, vptr);
	obs_property_set_long_description(panel, obs_module_text("Reset.Device.Hint"));

	obs_property_t *error = obs_properties_add_text(props, "error", NULL, OBS_TEXT_INFO);
	obs_property_text_set_info_type(error, OBS_TEXT_INFO_ERROR);
	obs_property_set_long_description(error, obs_module_text("ASIO.Driver.Error"));
	obs_property_set_visible(error, false);

	return props;
}

static void asio_input_defaults(obs_data_t *settings)
{
	if (!settings || !list || list->count == 0)
		return;

	obs_data_set_default_string(settings, "device_name", NULL);

	// Set default channel routing (-1 means unassigned/muted)
	for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
		char key[32];
		snprintf(key, sizeof(key), "OBS.Channels.%d", i);
		obs_data_set_default_int(settings, key, -1);
	}
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
	.destroy = asio_input_destroy,
	.update = asio_input_update,
	.get_defaults = asio_input_defaults,
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

/* This creates the device if it hasn't been created by this or another output
  or retrieves its pointer if it already exists. The output is also added as
  a client of the device. */
static bool attach_output_device(struct asio_data *data, const char *name)
{
	if (!name || !*name)
		return false;

	data->device_index = asio_device_list_get_index_from_driver_name(list, name);
	if (data->device_index < 0) {
		blog(LOG_ERROR, "[asio_output] This driver was not found in the registry: %s", name);
		return false;
	}

	// we set the name even if the driver fails to load
	if (data->device_name)
		bfree((void *)data->device_name);

	data->device_name = bstrdup(name);

	data->asio_device = asio_device_list_attach_device(list, name);

	if (!data->asio_device) {
		blog(LOG_ERROR, "[asio_output] Failed to attach device: %s", name);
		return false;
	} else if (!data->asio_device->asio) {
		blog(LOG_ERROR,
		     "[asio_output] Failed to load COM Driver: %s \n This can be due to a disconnected device"
		     "or another app using the ASIO driver.",
		     name);
		return false;
	}

	data->asio_device->obs_output_client = data;

	blog(LOG_DEBUG, "[asio_output] Attached device %s to output", name);
	// log some info about the device
	blog(LOG_INFO,
	     "[asio_output]:\nOutput added to device %s;\n\tcurrent sample rate: %f,"
	     "\n\tcurrent buffer: %i,"
	     "\n\toutput latency: %f ms\n",
	     name, data->asio_device->current_sample_rate, data->asio_device->current_buffer_size,
	     1000.0f * (float)data->asio_device->output_latency / data->asio_device->current_sample_rate);
	return true;
}

static void detach_output_device(struct asio_data *data)
{
	if (!data)
		return;

	if (!data->asio_device)
		return;

	struct asio_device *dev = data->asio_device;

	data->device_index = -1;
	blog(LOG_DEBUG, "[asio_output] Detached device %s", data->device_name);
	if (dev) {
		data->asio_device->obs_output_client = NULL;
		blog(LOG_INFO,
		     "[asio_output]:\n\tDevice % s removed;\n"
		     "\tnumber of xruns: % i;\n\tincrease your buffer if you get a high count & hear cracks, pops or else !\n -1 "
		     "means your device doesn't report xruns.",
		     data->device_name, data->asio_device->xruns);
		if (data->asio_device->current_nb_clients == 0)
			asio_device_close(data->asio_device);
	}
}

static void asio_output_update(void *vptr, obs_data_t *settings)
{
	struct asio_data *data = (struct asio_data *)vptr;

	// pass ptr of obs_data_t *settings to global
	if (!global_output_settings)
		global_output_settings = settings;

	const char *new_device = obs_data_get_string(settings, "device_name");

	// new device is 'none' -> return
	if (!new_device || !*new_device) {
		// we are actually swapping from a 'device' -> 'none'
		if (data->device_name && *data->device_name) {
			detach_output_device(data);
			data->device_name = NULL;
		}
		data->asio_device = NULL;
		return;
	}

	// we are loading a source which had already settings
	if (data->initial_update && new_device) {
		data->driver_loaded = attach_output_device(data, new_device);
		// If the driver fails to load, we keep the name but the driver will be greyed out on next properties call.
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			return;
		}
	}

	// we swap from a 'device' to a 'new device'
	if (!data->initial_update && strdiff(data->device_name, new_device)) {
		if (data->device_name && *data->device_name && data->driver_loaded)
			detach_output_device(data);

		data->driver_loaded = attach_output_device(data, new_device);
		data->update_channels = data->driver_loaded;
		if (!data->driver_loaded) {
			data->asio_device = NULL;
			return;
		} else {
			// Reset all mix channels to -1 to avoid leftover routing
			for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
				data->out_mix_channels[i] = -1;
				data->asio_device->obs_track[i] = -1;
				data->asio_device->obs_track_channel[i] = -1;
			}
		}
	}

	struct asio_device *dev = data->asio_device;
	if (!dev)
		return;

	if (!dev->is_open) {
		double sr = asio_device_get_sample_rate(dev);
		int buffer_size = asio_device_get_preferred_buffer_size(dev);
		if (sr <= 0)
			sr = 48000;

		if (buffer_size < 16)
			buffer_size = 512;

		asio_device_open(dev, sr, buffer_size);
	}

	if (dev->is_started)
		os_atomic_set_bool(&dev->capture_started, true);

	data->out_channels = (uint8_t)data->asio_device->total_num_output_chans;

	// update the routing data for each output device channels & pass the info to the device
	for (int i = 0; i < data->out_channels; ++i) {
		char key[32];
		snprintf(key, sizeof(key), "device_ch%d", i);
		if (data->out_mix_channels[i] != (int)obs_data_get_int(settings, key)) {
			data->out_mix_channels[i] = (int)obs_data_get_int(settings, key);
			data->asio_device->obs_track[i] = -1;         // device does not use track i
			data->asio_device->obs_track_channel[i] = -1; // device does not use any channel from track i
			if (data->out_mix_channels[i] >= 0) {
				for (int j = 0; j < MAX_AUDIO_MIXES_NEW; j++) {
					for (int k = 0; k < data->obs_track_channels; k++) {
						int64_t idx = (int64_t)k + (1LL << (j + 4));
						if (data->out_mix_channels[i] == idx) {
							data->asio_device->obs_track[i] = j;
							data->asio_device->obs_track_channel[i] = k;
							blog(LOG_DEBUG,
							     "[asio_output]:\nDevice output channel n° %i: Track %i, Channel %i",
							     i, j + 1, k + 1);
						}
					}
				}
			}
		}
	}
	data->initial_update = false;
}

static void *asio_output_create(obs_data_t *settings, obs_output_t *output)
{
	struct asio_data *data = bzalloc(sizeof(struct asio_data));
	data->asio_device = NULL;
	data->source = NULL;
	data->device_name = NULL;
	data->update_channels = false;
	data->driver_loaded = false;
	data->initial_update = true;

	for (int i = 0; i < MAX_NUM_ASIO_DEVICES; i++)
		data->asio_client_index[i] = -1; // not a client if negative;

	data->output = output;
	data->obs_track_channels = (uint8_t)audio_output_get_channels(obs_get_audio());

	// default value is negative, which implies no processing
	for (int i = 0; i < MAX_DEVICE_CHANNELS; i++)
		data->out_mix_channels[i] = -1;

	// allow all tracks for asio output + extra monitoring track
	obs_output_set_mixers(output, (1 << 7) - 1);

	os_atomic_set_bool(&data->active, true);
	os_atomic_set_bool(&data->stopping, false);

	if (global_output_asio_data)
		blog(LOG_INFO, "issue with asio output code ! you're opening two asio outputs !");

	global_output_asio_data = data;

	asio_output_update(data, settings);

	return data;
}

static bool asio_output_start(void *vptr)
{
	struct asio_data *data = vptr;

	if (!data)
		return false;

	if (!data->asio_device)
		return false;

	if (!obs_output_can_begin_data_capture(data->output, 0))
		return false;

	struct obs_audio_info oai;
	obs_get_audio_info(&oai);
	// Audio is always planar for asio so we ask obs to convert to planar format.
	struct audio_convert_info aci = {.format = AUDIO_FORMAT_FLOAT_PLANAR,
					 .speakers = oai.speakers,
					 .samples_per_sec = (uint32_t)data->asio_device->current_sample_rate};

	obs_output_set_audio_conversion(data->output, &aci);

	struct asio_device *dev = data->asio_device;
	os_atomic_set_bool(&dev->capture_started, true);

	return obs_output_begin_data_capture(data->output, 0);
}

static void asio_output_stop(void *vptr, uint64_t ts)
{
	struct asio_data *data = vptr;
	if (data) {
		obs_output_end_data_capture(data->output);
	}
}

static void asio_output_destroy(void *vptr)
{
	struct asio_data *data = vptr;

	if (!data)
		return;

	os_atomic_set_bool(&data->stopping, true);
	data->asio_device = NULL;

	if (data->device_name)
		bfree((void *)data->device_name);

	obs_data_release(global_output_settings);
	bfree(data);

	global_output_asio_data = NULL;
	global_output_settings = NULL;
}

static void asio_receive_audio(void *vptr, size_t mix_idx, struct audio_data *frame)
{
	struct asio_data *data = vptr;
	struct audio_data in = *frame;
	struct asio_device *dev = data->asio_device;

	if (os_atomic_load_bool(&shutting_down_atomic))
		return;

	if (os_atomic_load_bool(&data->stopping) || !frame)
		return;

	if (!dev)
		return;

	if (!os_atomic_load_bool(&dev->capture_started))
		return;

	for (int i = 0; i < dev->total_num_output_chans; ++i) {
		for (int j = 0; j < data->obs_track_channels; ++j) {
			if (dev->obs_track[i] == (int)mix_idx && dev->obs_track_channel[i] == j) {
				deque_push_back(&dev->excess_frames[i], in.data[j], in.frames * sizeof(float));
			}
		}
	}
}

static uint64_t asio_output_total_bytes(void *data)
{
	return 0;
}

static void asio_output_defaults(obs_data_t *settings)
{
	if (!list || list->count == 0)
		return;

	obs_data_set_default_string(settings, "device_name", "");

	for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
		char key[32];
		snprintf(key, sizeof(key), "device_ch%d", i);
		obs_data_set_default_int(settings, key, -1);
	}
}

static bool on_asio_output_device_changed(void *priv, obs_properties_t *props, obs_property_t *list_prop,
					  obs_data_t *settings)
{
	const char *device_name = obs_data_get_string(settings, "device_name");
	struct asio_data *data = (struct asio_data *)priv;

	if (!device_name || !*device_name)
		return false;

	// Find the device so we can repopulate the channel lists
	struct asio_device *dev = asio_device_find_by_name(device_name);
	if (!dev) {
		/* Driver failed to load or device not found: show only "Mute" for each output channel */
		for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
			char key[32];
			snprintf(key, sizeof(key), "device_ch%d", i);
			obs_data_set_int(settings, key, -1);

			obs_property_t *p = obs_properties_get(props, key);
			if (!p)
				continue;

			obs_property_list_clear(p);
			obs_property_list_add_int(p, obs_module_text("Mute"), -1);
		}

		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, true);

		/* Clear routing and mark update_channels so next attach rebuilds properly */
		asio_output_update(data, settings);
		data->update_channels = false;
		return true;
	}

	if (data->update_channels) {
		for (int i = 0; i < MAX_DEVICE_CHANNELS; ++i) {
			char key[32];
			snprintf(key, sizeof(key), "device_ch%d", i);
			obs_data_set_int(settings, key, -1);
			obs_properties_remove_by_name(props, key);
		}
		// create combo list
		for (int i = 0; i < dev->total_num_output_chans; ++i) {
			char key[32];
			snprintf(key, sizeof(key), "device_ch%d", i);
			obs_property_t *p = obs_properties_add_list(props, key, dev->output_channel_names[i],
								    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_set_long_description(p, obs_module_text("ASIO.Output.Hint"));
			obs_property_list_add_int(p, obs_module_text("Mute"), -1);
			for (int j = 0; j < MAX_AUDIO_MIXES_NEW; j++) {
				for (int k = 0; k < global_output_asio_data->obs_track_channels; k++) {
					long long idx = k + (1ULL << (j + 4));
					char label[32];
					snprintf(label, sizeof(label), "Track%d.%d", j, k);
					obs_property_list_add_int(p, obs_module_text(label), idx);
				}
			}
		}
		obs_property_t *error = obs_properties_get(props, "error");
		obs_property_set_visible(error, false);
		data->update_channels = false;
		asio_output_update(priv, settings);
	}

	return true;
}

static obs_properties_t *asio_output_properties(void *vptr)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;
	obs_property_t *panel;
	obs_property_t *reset;
	size_t count = list->count;
	struct asio_data *data = (struct asio_data *)vptr;
	struct asio_device *dev = NULL;
	int dev_out_channels;

	if (data != global_output_asio_data)
		data = global_output_asio_data;

	dev = data->asio_device;
	dev_out_channels = dev ? dev->total_num_output_chans : -1;

	p = obs_properties_add_list(props, "device_name", obs_module_text("ASIO.Driver"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, obs_module_text("Select.Device"), "");
	for (size_t i = 0; i < count; ++i) {
		const char *name = asio_device_list_get_name(list, i);
		obs_property_list_add_string(p, name, name);
		if (!list->drivers[i].loadable)
			obs_property_list_item_disable(p, i + 1, true);
	}

	obs_property_set_modified_callback2(p, on_asio_output_device_changed, data);

	panel = obs_properties_add_button2(props, "ctrl", obs_module_text("Control.Panel"), display_control_panel,
					   data);
	obs_property_set_long_description(panel, obs_module_text("Control.Panel.Hint"));
	reset = obs_properties_add_button2(props, "reset_button", obs_module_text("Reset.Device"),
					   on_reset_device_clicked, data);
	obs_property_set_long_description(reset, obs_module_text("Reset.Device.Hint"));
	obs_property_t *warning = obs_properties_add_text(props, "hint", NULL, OBS_TEXT_INFO);
	obs_property_text_set_info_type(warning, OBS_TEXT_INFO_WARNING);
	obs_property_set_long_description(warning, obs_module_text("ASIO.Output.Hint"));

	if (dev_out_channels >= 0) {
		for (int i = 0; i < dev_out_channels; i++) {
			char key[32];
			snprintf(key, sizeof(key), "device_ch%d", i);
			obs_property_t *p = obs_properties_add_list(props, key, dev->output_channel_names[i],
								    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
			obs_property_list_add_int(p, obs_module_text("Mute"), -1);
			obs_property_set_long_description(p, obs_module_text("ASIO.Output.Hint"));
			for (int j = 0; j < MAX_AUDIO_MIXES; j++) {
				for (int k = 0; k < global_output_asio_data->obs_track_channels; k++) {
					long long idx = k + (1ULL << (j + 4));
					char label[32];
					snprintf(label, sizeof(label), "Track%d.%d", j, k);
					obs_property_list_add_int(p, obs_module_text(label), idx);
				}
			}
		}
	}

	obs_property_t *error = obs_properties_add_text(props, "error", NULL, OBS_TEXT_INFO);
	obs_property_text_set_info_type(error, OBS_TEXT_INFO_ERROR);
	obs_property_set_long_description(error, obs_module_text("ASIO.Driver.Error"));
	obs_property_set_visible(error, false);

	return props;
}

struct obs_output_info asio_output = {
	.id = "asio_output",
	.flags = OBS_OUTPUT_AUDIO | OBS_OUTPUT_MULTI_TRACK,
	.get_name = asio_output_getname,
	.create = asio_output_create,
	.destroy = asio_output_destroy,
	.start = asio_output_start,
	.stop = asio_output_stop,
	.update = asio_output_update,
	.get_defaults = asio_output_defaults,
	.get_properties = asio_output_properties,
	.raw_audio2 = asio_receive_audio,
};
