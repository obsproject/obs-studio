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

#include "../util/threading.h"
#include "../util/darray.h"
#include "../util/bmem.h"

#include "media-io.h"

/* TODO: Incomplete */

struct media_input {
	struct media_input_info info;
	struct media_output *connection;
};

struct media_output {
	struct media_output_info info;
	DARRAY(media_input_t) connections;
	pthread_mutex_t mutex;
};

struct media_data {
	DARRAY(media_output_t) outputs;
};

/* ------------------------------------------------------------------------- */

media_input_t media_input_create(struct media_input_info *info)
{
	struct media_input *input;

	if (!info || !info->format || !info->on_input)
		return NULL;
       
	input = bmalloc(sizeof(struct media_input));
	input->connection = NULL;
	memcpy(&input->info, info, sizeof(struct media_input_info));

	return input;
}

void media_input_destroy(media_input_t input)
{
	if (input)
		bfree(input);
}

/* ------------------------------------------------------------------------- */

media_output_t media_output_create(struct media_output_info *info)
{
	struct media_output *output;

	/* TODO: handle format */
	if (!info) // || !info->format)
		return NULL;

	output = bmalloc(sizeof(struct media_output));
	da_init(output->connections);
	memcpy(&output->info, info, sizeof(struct media_output_info));

	if (pthread_mutex_init(&output->mutex, NULL) != 0) {
		bfree(output);
		return NULL;
	}

	return output;
}

void media_output_data(media_output_t output, const void *data)
{
	size_t i;

	pthread_mutex_lock(&output->mutex);

	for (i = 0; i < output->connections.num; i++) {
		media_input_t input = output->connections.array[i];
		input->info.on_input(input->info.obj, data);
	}

	pthread_mutex_unlock(&output->mutex);
}

void media_output_destroy(media_output_t output)
{
	if (output) {
		da_free(output->connections);
		pthread_mutex_destroy(&output->mutex);
		bfree(output);
	}
}

/* ------------------------------------------------------------------------- */

media_t media_open(void)
{
	struct media_data *media = bmalloc(sizeof(struct media_data));
	da_init(media->outputs);

	return media;
}

bool media_add_input(media_t media, media_input_t input)
{
	media_output_t *outputs = media->outputs.array;
	size_t i;

	for (i = 0; i < media->outputs.num; i++) {
		media_output_t output = outputs[i];

		if (strcmp(output->info.format, input->info.format) == 0) {
			da_push_back(output->connections, input);
			return true;
		}
	}

	return false;
}

void media_add_output(media_t media, media_output_t output)
{
	da_push_back(media->outputs, output);
}

void media_remove_input(media_t media, media_input_t input)
{
	if (!input->connection)
		return;

	da_erase_item(input->connection->connections, input);
}

void media_remove_output(media_t media, media_output_t output)
{
	da_erase_item(media->outputs, output);
}

void media_close(media_t media)
{
	if (media) {
		da_free(media->outputs);
		bfree(media);
	}
}
