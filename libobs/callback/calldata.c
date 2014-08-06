/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
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

#include <string.h>

#include "../util/bmem.h"

#include "calldata.h"

/*
 *   Uses a data stack.  Probably more complex than it should be, but reduces
 * fetching.
 *
 *   Stack format is:
 *     [size_t    param1_name_size]
 *     [char[]    param1_name]
 *     [size_t    param1_data_size]
 *     [uint8_t[] param1_data]
 *     [size_t    param2_name_size]
 *     [char[]    param2_name]
 *     [size_t    param2_data_size]
 *     [uint8_t[] param2_data]
 *     [...]
 *     [size_t    0]
 *
 *   Strings and string sizes always include the null terminator to allow for
 * direct referencing.
 */

static inline void cd_serialize(uint8_t **pos, void *ptr, size_t size)
{
	memcpy(ptr, *pos, size);
	*pos += size;
}

static inline size_t cd_serialize_size(uint8_t **pos)
{
	size_t size = *(size_t*)*pos;
	*pos += sizeof(size_t);
	return size;
}

static inline const char *cd_serialize_string(uint8_t **pos)
{
	size_t size = cd_serialize_size(pos);
	const char *str = (const char *)*pos;

	*pos += size;

	return (size != 0) ? str : NULL;
}

static bool cd_getparam(calldata_t data, const char *name,
		uint8_t **pos)
{
	size_t name_size;

	if (!data->size)
		return false;

	*pos = data->stack;

	name_size = cd_serialize_size(pos);
	while (name_size != 0) {
		const char *param_name = (const char *)*pos;
		size_t param_size;

		*pos += name_size;
		if (strcmp(param_name, name) == 0)
			return true;

		param_size = cd_serialize_size(pos);
		*pos += param_size;

		name_size = cd_serialize_size(pos);
	}

	*pos -= sizeof(size_t);
	return false;
}

static inline void cd_copy_string(uint8_t **pos, const char *str, size_t len)
{
	if (!len)
		len = strlen(str)+1;

	*(size_t*)*pos = len;
	*pos += sizeof(size_t);
	memcpy(*pos, str, len);
	*pos += len;
}

static inline void cd_copy_data(uint8_t **pos, const void *in, size_t size)
{
	*(size_t*)*pos = size;
	*pos += sizeof(size_t);

	if (size) {
		memcpy(*pos, in, size);
		*pos += size;
	}
}

static inline void cd_set_first_param(calldata_t data, const char *name,
		const void *in, size_t size)
{
	uint8_t *pos;
	size_t capacity;
	size_t name_len = strlen(name)+1;

	capacity = sizeof(size_t)*3 + name_len + size;
	data->size = capacity;

	if (capacity < 128)
		capacity = 128;

	data->capacity = capacity;
	data->stack    = bmalloc(capacity);

	pos = data->stack;
	cd_copy_string(&pos, name, name_len);
	cd_copy_data(&pos, in, size);
	*(size_t*)pos = 0;
}

static inline void cd_ensure_capacity(calldata_t data, uint8_t **pos,
		size_t new_size)
{
	size_t offset;
	size_t new_capacity;

	if (new_size < data->capacity)
		return;

	offset = *pos - data->stack;

	new_capacity = data->capacity * 2;
	if (new_capacity < new_size)
		new_capacity = new_size;

	data->stack    = brealloc(data->stack, new_capacity);
	data->capacity = new_capacity;

	*pos = data->stack + offset;
}

/* ------------------------------------------------------------------------- */

bool calldata_get_data(calldata_t data, const char *name, void *out,
		size_t size)
{
	uint8_t *pos;
	size_t data_size;

	if (!data || !name || !*name)
		return false;

	if (!cd_getparam(data, name, &pos))
		return false;

	data_size = cd_serialize_size(&pos);
	if (data_size != size)
		return false;

	memcpy(out, pos, size);
	return true;
}

void calldata_set_data(calldata_t data, const char *name, const void *in,
		size_t size)
{
	uint8_t *pos = NULL;

	if (!data || !name || !*name)
		return;

	if (!data->stack) {
		cd_set_first_param(data, name, in, size);
		return;
	}

	if (cd_getparam(data, name, &pos)) {
		size_t cur_size = *(size_t*)pos;

		if (cur_size < size) {
			size_t offset = size - cur_size;
			size_t bytes = data->size;

			cd_ensure_capacity(data, &pos, bytes + offset);
			memmove(pos+offset, pos, bytes - (pos - data->stack));
			data->size += offset;

		} else if (cur_size > size) {
			size_t offset = cur_size - size;
			size_t bytes = data->size - offset;

			memmove(pos, pos+offset, bytes - (pos - data->stack));
			data->size -= offset;
		}

		cd_copy_data(&pos, in, size);

	} else {
		size_t name_len = strlen(name)+1;
		size_t offset = name_len + size + sizeof(size_t)*2;
		cd_ensure_capacity(data, &pos, data->size + offset);
		data->size += offset;

		cd_copy_string(&pos, name, 0);
		cd_copy_data(&pos, in, size);
		*(size_t*)pos = 0;
	}
}

bool calldata_get_string(calldata_t data, const char *name, const char **str)
{
	uint8_t *pos;
	if (!data || !name || !*name)
		return false;

	if (!cd_getparam(data, name, &pos))
		return false;

	*str = cd_serialize_string(&pos);
	return true;
}
