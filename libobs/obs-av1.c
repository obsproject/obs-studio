// SPDX-FileCopyrightText: 2023 David Rosca <nowrep@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "obs-av1.h"

#include "obs.h"

static inline uint64_t leb128(const uint8_t *buf, size_t size, size_t *len)
{
	uint64_t value = 0;
	uint8_t leb128_byte;

	*len = 0;

	for (int i = 0; i < 8; i++) {
		if (size-- < 1)
			break;
		(*len)++;
		leb128_byte = buf[i];
		value |= (leb128_byte & 0x7f) << (i * 7);
		if (!(leb128_byte & 0x80))
			break;
	}

	return value;
}

static inline unsigned int get_bits(uint8_t val, unsigned int n,
				    unsigned int count)
{
	return (val >> (8 - n - count)) & ((1 << (count - 1)) * 2 - 1);
}

static void parse_obu_header(const uint8_t *buf, size_t size, size_t *obu_start,
			     size_t *obu_size, int *obu_type)
{
	int extension_flag, has_size_field;
	size_t size_len = 0;

	*obu_start = 0;
	*obu_size = 0;
	*obu_type = 0;

	if (size < 1)
		return;

	*obu_type = get_bits(*buf, 1, 4);
	extension_flag = get_bits(*buf, 5, 1);
	has_size_field = get_bits(*buf, 6, 1);

	if (extension_flag)
		(*obu_start)++;

	(*obu_start)++;

	if (has_size_field)
		*obu_size = (size_t)leb128(buf + *obu_start, size - *obu_start,
					   &size_len);
	else
		*obu_size = size - 1;

	*obu_start += size_len;
}

bool obs_av1_keyframe(const uint8_t *data, size_t size)
{
	const uint8_t *start = data, *end = data + size;

	while (start < end) {
		size_t obu_start, obu_size;
		int obu_type;
		parse_obu_header(start, end - start, &obu_start, &obu_size,
				 &obu_type);

		if (obu_size) {
			if (obu_type == OBS_OBU_FRAME ||
			    obu_type == OBS_OBU_FRAME_HEADER) {
				uint8_t val = *(start + obu_start);
				if (!get_bits(val, 0, 1)) // show_existing_frame
					return get_bits(val, 1, 2) ==
					       0; // frame_type
				return false;
			}
		}

		start += obu_start + obu_size;
	}

	return false;
}

void obs_extract_av1_headers(const uint8_t *packet, size_t size,
			     uint8_t **new_packet_data, size_t *new_packet_size,
			     uint8_t **header_data, size_t *header_size)
{
	DARRAY(uint8_t) new_packet;
	DARRAY(uint8_t) header;
	const uint8_t *start = packet, *end = packet + size;

	da_init(new_packet);
	da_init(header);

	while (start < end) {
		size_t obu_start, obu_size;
		int obu_type;
		parse_obu_header(start, end - start, &obu_start, &obu_size,
				 &obu_type);

		if (obu_type == OBS_OBU_METADATA ||
		    obu_type == OBS_OBU_SEQUENCE_HEADER) {
			da_push_back_array(header, start, obu_start + obu_size);
		}
		da_push_back_array(new_packet, start, obu_start + obu_size);

		start += obu_start + obu_size;
	}

	*new_packet_data = new_packet.array;
	*new_packet_size = new_packet.num;
	*header_data = header.array;
	*header_size = header.num;
}
