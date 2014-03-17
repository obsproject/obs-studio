/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs.h"
#include "obs-internal.h"

static inline const struct obs_output_info *find_output(const char *id)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].id, id) == 0)
			return obs->output_types.array+i;

	return NULL;
}

obs_output_t obs_output_create(const char *id, const char *name,
		obs_data_t settings)
{
	const struct obs_output_info *info = find_output(id);
	struct obs_output *output;

	if (!info) {
		blog(LOG_ERROR, "Output '%s' not found", id);
		return NULL;
	}

	output = bzalloc(sizeof(struct obs_output));

	output->signals = signal_handler_create();
	if (!output->signals)
		goto fail;

	output->procs = proc_handler_create();
	if (!output->procs)
		goto fail;

	output->info      = *info;
	output->settings  = obs_data_newref(settings);
	if (output->info.defaults)
		output->info.defaults(output->settings);

	output->data      = info->create(output->settings, output);
	if (!output->data)
		goto fail;

	output->name = bstrdup(name);

	pthread_mutex_lock(&obs->data.outputs_mutex);
	da_push_back(obs->data.outputs, &output);
	pthread_mutex_unlock(&obs->data.outputs_mutex);

	output->valid = true;

	return output;

fail:
	obs_output_destroy(output);
	return NULL;
}

void obs_output_destroy(obs_output_t output)
{
	if (output) {
		if (output->valid) {
			if (output->info.active) {
				if (output->info.active(output->data))
					output->info.stop(output->data);
			}

			pthread_mutex_lock(&obs->data.outputs_mutex);
			da_erase_item(obs->data.outputs, &output);
			pthread_mutex_unlock(&obs->data.outputs_mutex);
		}

		if (output->data)
			output->info.destroy(output->data);

		signal_handler_destroy(output->signals);
		proc_handler_destroy(output->procs);

		obs_data_release(output->settings);
		bfree(output->name);
		bfree(output);
	}
}

bool obs_output_start(obs_output_t output)
{
	return (output != NULL) ? output->info.start(output->data) : false;
}

void obs_output_stop(obs_output_t output)
{
	if (output)
		output->info.stop(output->data);
}

bool obs_output_active(obs_output_t output)
{
	return (output != NULL) ? output->info.active(output) : false;
}

obs_data_t obs_output_defaults(const char *id)
{
	const struct obs_output_info *info = find_output(id);
	if (info) {
		obs_data_t settings = obs_data_create();
		if (info->defaults)
			info->defaults(settings);
		return settings;
	}

	return NULL;
}

obs_properties_t obs_output_properties(const char *id, const char *locale)
{
	const struct obs_output_info *info = find_output(id);
	if (info && info->properties)
		return info->properties(locale);
	return NULL;
}

void obs_output_update(obs_output_t output, obs_data_t settings)
{
	if (!output) return;

	obs_data_apply(output->settings, settings);

	if (output->info.update)
		output->info.update(output->data, output->settings);
}

obs_data_t obs_output_get_settings(obs_output_t output)
{
	if (!output)
		return NULL;

	obs_data_addref(output->settings);
	return output->settings;
}

bool obs_output_canpause(obs_output_t output)
{
	return output ? (output->info.pause != NULL) : false;
}

void obs_output_pause(obs_output_t output)
{
	if (output && output->info.pause)
		output->info.pause(output->data);
}

signal_handler_t obs_output_signalhandler(obs_output_t output)
{
	return output ? output->signals : NULL;
}

proc_handler_t obs_output_prochandler(obs_output_t output)
{
	return output ? output->procs : NULL;
}
