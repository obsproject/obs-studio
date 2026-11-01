/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
******************************************************************************/

#include "obs-internal.h"

#define get_weak(service) ((obs_weak_service_t *)service->context.control)

const struct obs_service_info *find_service(const char *id)
{
	size_t i;
	for (i = 0; i < obs->service_types.num; i++)
		if (strcmp(obs->service_types.array[i].id, id) == 0)
			return obs->service_types.array + i;

	return NULL;
}

const char *obs_service_get_display_name(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	return (info != NULL) ? info->get_name(info->type_data) : NULL;
}

obs_module_t *obs_service_get_module(const char *id)
{
	obs_module_t *module = obs->first_module;
	while (module) {
		for (size_t i = 0; i < module->services.num; i++) {
			if (strcmp(module->services.array[i], id) == 0) {
				return module;
			}
		}
		module = module->next;
	}

	module = obs->first_disabled_module;
	while (module) {
		for (size_t i = 0; i < module->services.num; i++) {
			if (strcmp(module->services.array[i], id) == 0) {
				return module;
			}
		}
		module = module->next;
	}

	return NULL;
}

enum obs_module_load_state obs_service_load_state(const char *id)
{
	obs_module_t *module = obs_service_get_module(id);
	if (!module) {
		return OBS_MODULE_MISSING;
	}
	return module->load_state;
}

static obs_service_t *obs_service_create_internal(const char *id, const char *name, obs_data_t *settings,
						  obs_data_t *hotkey_data, bool private)
{
	const struct obs_service_info *info = find_service(id);
	struct obs_service *service;

	if (!info) {
		blog(LOG_ERROR, "Service '%s' not found", id);
		return NULL;
	}

	service = bzalloc(sizeof(struct obs_service));

	if (!obs_context_data_init(&service->context, OBS_OBJ_TYPE_SERVICE, settings, name, NULL, hotkey_data,
				   private)) {
		bfree(service);
		return NULL;
	}

	service->info = *info;
	service->context.data = service->info.create(service->context.settings, service);
	if (!service->context.data)
		blog(LOG_ERROR, "Failed to create service '%s'!", name);

	obs_context_init_control(&service->context, service, (obs_destroy_cb)obs_service_destroy);
	obs_context_data_insert(&service->context, &obs->data.services_mutex, &obs->data.first_service);

	blog(LOG_DEBUG, "service '%s' (%s) created", name, id);
	return service;
}

obs_service_t *obs_service_create(const char *id, const char *name, obs_data_t *settings, obs_data_t *hotkey_data)
{
	return obs_service_create_internal(id, name, settings, hotkey_data, false);
}

obs_service_t *obs_service_create_private(const char *id, const char *name, obs_data_t *settings)
{
	return obs_service_create_internal(id, name, settings, NULL, true);
}

static void actually_destroy_service(struct obs_service *service)
{
	if (service->context.data)
		service->info.destroy(service->context.data);

	if (service->output)
		service->output->service = NULL;

	blog(LOG_DEBUG, "service '%s' destroyed", service->context.name);

	obs_context_data_free(&service->context);
	if (service->owns_info_id)
		bfree((void *)service->info.id);
	bfree(service);
}

void obs_service_destroy(obs_service_t *service)
{
	if (service) {
		obs_context_data_remove(&service->context);

		service->destroy = true;

		/* do NOT destroy the service until the service is no
		 * longer in use */
		if (!service->active)
			actually_destroy_service(service);
	}
}

const char *obs_service_get_name(const obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_name") ? service->context.name : NULL;
}

static inline obs_data_t *get_defaults(const struct obs_service_info *info)
{
	obs_data_t *settings = obs_data_create();
	if (info->get_defaults)
		info->get_defaults(settings);
	return settings;
}

obs_data_t *obs_service_defaults(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t *obs_get_service_properties(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	if (info && info->get_properties) {
		obs_data_t *defaults = get_defaults(info);
		obs_properties_t *properties;

		properties = info->get_properties(NULL);
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t *obs_service_properties(const obs_service_t *service)
{
	if (!obs_service_valid(service, "obs_service_properties"))
		return NULL;

	if (service->info.get_properties) {
		obs_properties_t *props;
		props = service->info.get_properties(service->context.data);
		obs_properties_apply_settings(props, service->context.settings);
		return props;
	}

	return NULL;
}

const char *obs_service_get_type(const obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_type") ? service->info.id : NULL;
}

void obs_service_update(obs_service_t *service, obs_data_t *settings)
{
	if (!obs_service_valid(service, "obs_service_update"))
		return;

	obs_data_apply(service->context.settings, settings);

	if (service->info.update)
		service->info.update(service->context.data, service->context.settings);
}

obs_data_t *obs_service_get_settings(const obs_service_t *service)
{
	if (!obs_service_valid(service, "obs_service_get_settings"))
		return NULL;

	obs_data_addref(service->context.settings);
	return service->context.settings;
}

signal_handler_t *obs_service_get_signal_handler(const obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_signal_handler") ? service->context.signals : NULL;
}

proc_handler_t *obs_service_get_proc_handler(const obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_proc_handler") ? service->context.procs : NULL;
}

void obs_service_activate(struct obs_service *service)
{
	if (!obs_service_valid(service, "obs_service_activate"))
		return;
	if (!service->output) {
		blog(LOG_WARNING,
		     "obs_service_deactivate: service '%s' "
		     "is not assigned to an output",
		     obs_service_get_name(service));
		return;
	}
	if (service->active)
		return;

	if (service->info.activate)
		service->info.activate(service->context.data, service->context.settings);
	service->active = true;
}

void obs_service_deactivate(struct obs_service *service, bool remove)
{
	if (!obs_service_valid(service, "obs_service_deactivate"))
		return;
	if (!service->output) {
		blog(LOG_WARNING,
		     "obs_service_deactivate: service '%s' "
		     "is not assigned to an output",
		     obs_service_get_name(service));
		return;
	}

	if (!service->active)
		return;

	if (service->info.deactivate)
		service->info.deactivate(service->context.data);
	service->active = false;

	if (service->destroy)
		actually_destroy_service(service);
	else if (remove)
		service->output = NULL;
}

bool obs_service_initialize(struct obs_service *service, struct obs_output *output)
{
	if (!obs_service_valid(service, "obs_service_initialize"))
		return false;
	if (!obs_output_valid(output, "obs_service_initialize"))
		return false;

	if (service->info.initialize)
		return service->info.initialize(service->context.data, output);
	return true;
}

void obs_service_apply_encoder_settings(obs_service_t *service, obs_data_t *video_encoder_settings,
					obs_data_t *audio_encoder_settings)
{
	if (!obs_service_valid(service, "obs_service_apply_encoder_settings"))
		return;
	if (!service->info.apply_encoder_settings)
		return;

	if (video_encoder_settings || audio_encoder_settings)
		service->info.apply_encoder_settings(service->context.data, video_encoder_settings,
						     audio_encoder_settings);
}

void obs_service_release(obs_service_t *service)
{
	if (!service)
		return;

	obs_weak_service_t *control = get_weak(service);
	if (obs_ref_release(&control->ref)) {
		// The order of operations is important here since
		// get_context_by_name in obs.c relies on weak refs
		// being alive while the context is listed
		obs_service_destroy(service);
		obs_weak_service_release(control);
	}
}

void obs_weak_service_addref(obs_weak_service_t *weak)
{
	if (!weak)
		return;

	obs_weak_ref_addref(&weak->ref);
}

void obs_weak_service_release(obs_weak_service_t *weak)
{
	if (!weak)
		return;

	if (obs_weak_ref_release(&weak->ref))
		bfree(weak);
}

obs_service_t *obs_service_get_ref(obs_service_t *service)
{
	if (!service)
		return NULL;

	return obs_weak_service_get_service(get_weak(service));
}

obs_weak_service_t *obs_service_get_weak_service(obs_service_t *service)
{
	if (!service)
		return NULL;

	obs_weak_service_t *weak = get_weak(service);
	obs_weak_service_addref(weak);
	return weak;
}

obs_service_t *obs_weak_service_get_service(obs_weak_service_t *weak)
{
	if (!weak)
		return NULL;

	if (obs_weak_ref_get_ref(&weak->ref))
		return weak->service;

	return NULL;
}

bool obs_weak_service_references_service(obs_weak_service_t *weak, obs_service_t *service)
{
	return weak && service && weak->service == service;
}

void *obs_service_get_type_data(obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_type_data") ? service->info.type_data : NULL;
}

const char *obs_service_get_id(const obs_service_t *service)
{
	return obs_service_valid(service, "obs_service_get_id") ? service->info.id : NULL;
}

void obs_service_get_supported_resolutions(const obs_service_t *service, struct obs_service_resolution **resolutions,
					   size_t *count)
{
	if (!obs_service_valid(service, "obs_service_supported_resolutions"))
		return;
	if (!obs_ptr_valid(resolutions, "obs_service_supported_resolutions"))
		return;
	if (!obs_ptr_valid(count, "obs_service_supported_resolutions"))
		return;

	*resolutions = NULL;
	*count = 0;

	if (service->info.get_supported_resolutions)
		service->info.get_supported_resolutions(service->context.data, resolutions, count);
}

void obs_service_get_max_fps(const obs_service_t *service, int *fps)
{
	if (!obs_service_valid(service, "obs_service_get_max_fps"))
		return;
	if (!obs_ptr_valid(fps, "obs_service_get_max_fps"))
		return;

	*fps = 0;

	if (service->info.get_max_fps)
		service->info.get_max_fps(service->context.data, fps);
}

void obs_service_get_max_bitrate(const obs_service_t *service, int *video_bitrate, int *audio_bitrate)
{
	if (video_bitrate)
		*video_bitrate = 0;
	if (audio_bitrate)
		*audio_bitrate = 0;

	if (!obs_service_valid(service, "obs_service_get_max_bitrate"))
		return;

	if (service->info.get_max_bitrate)
		service->info.get_max_bitrate(service->context.data, video_bitrate, audio_bitrate);
}

const char **obs_service_get_supported_video_codecs(const obs_service_t *service)
{
	if (service->info.get_supported_video_codecs)
		return service->info.get_supported_video_codecs(service->context.data);
	return NULL;
}

const char **obs_service_get_supported_audio_codecs(const obs_service_t *service)
{
	if (service->info.get_supported_audio_codecs)
		return service->info.get_supported_audio_codecs(service->context.data);
	return NULL;
}

const char *obs_service_get_protocol(const obs_service_t *service)
{
	if (!obs_service_valid(service, "obs_service_get_protocol"))
		return NULL;

	return service->info.get_protocol(service->context.data);
}

const char *obs_service_get_preferred_output_type(const obs_service_t *service)
{
	if (!obs_service_valid(service, "obs_service_get_preferred_output_type"))
		return NULL;

	if (service->info.get_output_type)
		return service->info.get_output_type(service->context.data);
	return NULL;
}

const char *obs_service_get_connect_info(const obs_service_t *service, uint32_t type)
{
	if (!obs_service_valid(service, "obs_service_get_info"))
		return NULL;

	if (!service->info.get_connect_info)
		return NULL;
	return service->info.get_connect_info(service->context.data, type);
}

bool obs_service_can_try_to_connect(const obs_service_t *service)
{
	if (!obs_service_valid(service, "obs_service_can_connect"))
		return false;

	if (!service->info.can_try_to_connect)
		return true;
	return service->info.can_try_to_connect(service->context.data);
}
