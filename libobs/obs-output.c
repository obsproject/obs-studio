/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
#include "obs-data.h"

bool get_output_info(void *module, const char *module_name,
		const char *output_name, struct output_info *info)
{
	info->getname = load_module_subfunc(module, module_name,
			output_name, "getname", true);
	info->create = load_module_subfunc(module, module_name,
			output_name, "create", true);
	info->destroy = load_module_subfunc(module, module_name,
			output_name, "destroy", true);
	info->start = load_module_subfunc(module, module_name,
			output_name, "start", true);
	info->stop = load_module_subfunc(module, module_name,
			output_name, "stop", true);

	if (!info->getname || !info->create || !info->destroy ||
	    !info->start || !info->stop)
		return false;

	info->config = load_module_subfunc(module, module_name,
			output_name, "config", false);
	info->pause = load_module_subfunc(module, module_name,
			output_name, "pause", false);

	info->name = output_name;
	return true;
}

static inline const struct output_info *find_output(const char *type)
{
	size_t i;
	for (i = 0; i < obs->output_types.num; i++)
		if (strcmp(obs->output_types.array[i].name, type) == 0)
			return obs->output_types.array+i;

	return NULL;
}

obs_output_t obs_output_create(const char *type, const char *settings)
{
	const struct output_info *info = find_output(type);
	struct obs_output *output;

	if (!info) {
		blog(LOG_WARNING, "Output type '%s' not found", type);
		return NULL;
	}

	output = bmalloc(sizeof(struct obs_output));
	output->data = info->create(settings, output);
	if (!output->data) {
		bfree(output);
		return NULL;
	}

	dstr_init_copy(&output->settings, settings);
	memcpy(&output->callbacks, info, sizeof(struct output_info));
	return output;
}

void obs_output_destroy(obs_output_t output)
{
	if (output) {
		output->callbacks.destroy(output->data);
		dstr_free(&output->settings);
		bfree(output);
	}
}

void obs_output_start(obs_output_t output)
{
	output->callbacks.start(output->data);
}

void obs_output_stop(obs_output_t output)
{
	output->callbacks.stop(output->data);
}

bool obs_output_canconfig(obs_output_t output)
{
	return output->callbacks.config != NULL;
}

void obs_output_config(obs_output_t output, void *parent)
{
	if (output->callbacks.config)
		output->callbacks.config(output->data, parent);
}

bool obs_output_canpause(obs_output_t output)
{
	return output->callbacks.pause != NULL;
}

void obs_output_pause(obs_output_t output)
{
	if (output->callbacks.pause)
		output->callbacks.pause(output->data);
}

void obs_output_save_settings(obs_output_t output, const char *settings)
{
	dstr_copy(&output->settings, settings);
}
