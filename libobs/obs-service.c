/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

const struct obs_service_info *find_service(const char *id)
{
	size_t i;
	for (i = 0; i < obs->service_types.num; i++)
		if (strcmp(obs->service_types.array[i].id, id) == 0)
			return obs->service_types.array+i;

	return NULL;
}

const char *obs_service_get_display_name(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	return (info != NULL) ? info->get_name() : NULL;
}

obs_service_t obs_service_create(const char *id, const char *name,
		obs_data_t settings)
{
	const struct obs_service_info *info = find_service(id);
	struct obs_service *service;

	if (!info) {
		blog(LOG_ERROR, "Service '%s' not found", id);
		return NULL;
	}

	service = bzalloc(sizeof(struct obs_service));

	if (!obs_context_data_init(&service->context, settings, name)) {
		bfree(service);
		return NULL;
	}

	service->info = *info;

	service->context.data = service->info.create(service->context.settings,
			service);
	if (!service->context.data) {
		obs_service_destroy(service);
		return NULL;
	}

	obs_context_data_insert(&service->context,
			&obs->data.services_mutex,
			&obs->data.first_service);

	blog(LOG_INFO, "service '%s' (%s) created", name, id);
	return service;
}

static void actually_destroy_service(struct obs_service *service)
{
	if (service->context.data)
		service->info.destroy(service->context.data);

	if (service->output)
		service->output->service = NULL;

	blog(LOG_INFO, "service '%s' destroyed", service->context.name);

	obs_context_data_free(&service->context);
	bfree(service);
}

void obs_service_destroy(obs_service_t service)
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

const char *obs_service_get_name(obs_service_t service)
{
	return service ? service->context.name : NULL;
}

static inline obs_data_t get_defaults(const struct obs_service_info *info)
{
	obs_data_t settings = obs_data_create();
	if (info->get_defaults)
		info->get_defaults(settings);
	return settings;
}

obs_data_t obs_service_defaults(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	return (info) ? get_defaults(info) : NULL;
}

obs_properties_t obs_get_service_properties(const char *id)
{
	const struct obs_service_info *info = find_service(id);
	if (info && info->get_properties) {
		obs_data_t       defaults = get_defaults(info);
		obs_properties_t properties;

		properties = info->get_properties();
		obs_properties_apply_settings(properties, defaults);
		obs_data_release(defaults);
		return properties;
	}
	return NULL;
}

obs_properties_t obs_service_properties(obs_service_t service)
{
	if (service && service->info.get_properties) {
		obs_properties_t props;
		props = service->info.get_properties();
		obs_properties_apply_settings(props, service->context.settings);
		return props;
	}

	return NULL;
}

const char *obs_service_gettype(obs_service_t service)
{
	return service ? service->info.id : NULL;
}

void obs_service_update(obs_service_t service, obs_data_t settings)
{
	if (!service) return;

	obs_data_apply(service->context.settings, settings);

	if (service->info.update)
		service->info.update(service->context.data,
				service->context.settings);
}

obs_data_t obs_service_get_settings(obs_service_t service)
{
	if (!service)
		return NULL;

	obs_data_addref(service->context.settings);
	return service->context.settings;
}

signal_handler_t obs_service_get_signal_handler(obs_service_t service)
{
	return service ? service->context.signals : NULL;
}

proc_handler_t obs_service_get_proc_handler(obs_service_t service)
{
	return service ? service->context.procs : NULL;
}

const char *obs_service_get_url(obs_service_t service)
{
	if (!service || !service->info.get_url) return NULL;
	return service->info.get_url(service->context.data);
}

const char *obs_service_get_key(obs_service_t service)
{
	if (!service || !service->info.get_key) return NULL;
	return service->info.get_key(service->context.data);
}

const char *obs_service_get_username(obs_service_t service)
{
	if (!service || !service->info.get_username) return NULL;
	return service->info.get_username(service->context.data);
}

const char *obs_service_get_password(obs_service_t service)
{
	if (!service || !service->info.get_password) return NULL;
	return service->info.get_password(service->context.data);
}

void obs_service_activate(struct obs_service *service)
{
	if (!service || !service->output || service->active) return;

	if (service->info.activate)
		service->info.activate(service->context.data,
				service->context.settings);
	service->active = true;
}

void obs_service_deactivate(struct obs_service *service, bool remove)
{
	if (!service || !service->output || !service->active) return;

	if (service->info.deactivate)
		service->info.deactivate(service->context.data);
	service->active = false;

	if (service->destroy)
		actually_destroy_service(service);
	else if (remove)
		service->output = NULL;
}

bool obs_service_initialize(struct obs_service *service,
		struct obs_output *output)
{
	if (!service || !output)
		return false;

	if (service->info.initialize)
		return service->info.initialize(service->context.data, output);
	return true;
}
