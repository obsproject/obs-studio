/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "darray.h"
#include "array-serializer.h"

static size_t array_output_write(void *param, const void *data, size_t size)
{
	struct array_output_data *output = param;

	if (output->cur_pos < output->bytes.num) {
		size_t new_size = output->cur_pos + size;

		if (new_size > output->bytes.num) {
			darray_ensure_capacity(sizeof(uint8_t),
					       &output->bytes.da, new_size);
			output->bytes.num = new_size;
		}

		memcpy(output->bytes.array + output->cur_pos, data, size);
		output->cur_pos += size;
	} else {
		da_push_back_array(output->bytes, (uint8_t *)data, size);
		output->cur_pos += size;
	}

	return size;
}

static int64_t array_output_get_pos(void *param)
{
	struct array_output_data *data = param;
	return (int64_t)data->bytes.num;
}

static int64_t array_output_seek(void *param, int64_t offset,
				 enum serialize_seek_type seek_type)
{
	struct array_output_data *output = param;

	size_t new_pos = 0;

	switch (seek_type) {
	case SERIALIZE_SEEK_START:
		new_pos = offset;
		break;
	case SERIALIZE_SEEK_CURRENT:
		new_pos = output->cur_pos + offset;
		break;
	case SERIALIZE_SEEK_END:
		new_pos = output->bytes.num - offset;
		break;
	}

	if (new_pos > output->bytes.num)
		return -1;

	output->cur_pos = new_pos;

	return (int64_t)new_pos;
}

void array_output_serializer_init(struct serializer *s,
				  struct array_output_data *data)
{
	memset(s, 0, sizeof(struct serializer));
	memset(data, 0, sizeof(struct array_output_data));
	s->data = data;
	s->write = array_output_write;
	s->get_pos = array_output_get_pos;
	s->seek = array_output_seek;
}

void array_output_serializer_free(struct array_output_data *data)
{
	da_free(data->bytes);
}

void array_output_serializer_reset(struct array_output_data *data)
{
	da_clear(data->bytes);
	data->cur_pos = 0;
}
