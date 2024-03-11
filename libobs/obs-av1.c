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

// Pass a static 10 byte buffer in. The max size for a leb128.
static inline void encode_uleb128(uint64_t val, uint8_t *out_buf,
				  size_t *len_out)
{
	size_t num_bytes = 0;
	uint8_t b = val & 0x7f;
	val >>= 7;
	while (val > 0) {
		out_buf[num_bytes] = b | 0x80;
		++num_bytes;
		b = val & 0x7f;
		val >>= 7;
	}
	out_buf[num_bytes] = b;
	++num_bytes;
	*len_out = num_bytes;
}

static const uint8_t METADATA_TYPE_ITUT_T35 = 4;
static const uint8_t OBU_METADATA = 5;

// Create a metadata OBU to carry caption information.
void metadata_obu_itu_t35(const uint8_t *itut_t35_buffer, size_t itut_bufsize,
			  uint8_t **out_buffer, size_t *outbuf_size)
{
	/* From the AV1 spec: 5.3.2 OBU Header Syntax
	 * -------------
	 * obu_forbidden_bit   (1)
	 * obu_type            (4)  // In this case OBS_OBU_METADATA
	 * obu_extension_flag  (1)
	 * obu_has_size_field  (1)  // Must be set, size of OBU is variable
	 * obu_reserved_1bit   (1)
	 * if(obu_extension_flag == 1)
	 *   // skip, because we aren't setting this
	 */

	uint8_t obu_header_byte = (OBU_METADATA << 3) | (1 << 1);

	/* From the AV1 spec: 5.3.1 General OBU Syntax
	 * if (obu_has_size_field)
	 * 	obu_size    leb128()
	 * else
	 * 	obu_size = sz - 1 - obu_extension_flag
	 *
	 * 	// Skipping portions unrelated to this OBU type	
	 *
	 * if (obu_type == OBU_METADATA)
	 * 	metdata_obu()
	 * 5.8.1 General metadata OBU Syntax
	 *	// leb128(metadatatype) should always be 1 byte +1 for trailing bits
	 * 	metadata_type                     leb128()
	 * 5.8.2 Metadata ITUT T35 syntax
	 * 	if (metadata_type == METADATA_TYPE_ITUT_T35)
         * 		// add ITUT T35 payload
	 * 5.8.1 General metadata OBU Syntax
	 * 	// trailing bits will always be 0x80 because
	 *	// everything in here is byte aligned
	 * 	trailing_bits( obu_size * 8 - payloadBits )
	 */

	int64_t size_field = 1 + itut_bufsize + 1;
	uint8_t size_buf[10];
	size_t size_buf_size = 0;
	encode_uleb128(size_field, size_buf, &size_buf_size);
	// header + obu_size + metadata_type + metadata_payload + trailing_bits
	*outbuf_size = 1 + size_buf_size + 1 + itut_bufsize + 1;
	*out_buffer = bzalloc(*outbuf_size);
	size_t offset = 0;
	(*out_buffer)[0] = obu_header_byte;
	++offset;
	memcpy((*out_buffer) + offset, size_buf, size_buf_size);
	offset += size_buf_size;
	(*out_buffer)[offset] = METADATA_TYPE_ITUT_T35;
	++offset;
	memcpy((*out_buffer) + offset, itut_t35_buffer, itut_bufsize);
	offset += itut_bufsize;

	/* From AV1 spec: 6.2.1 General OBU semantics
	 * ... Trailing bits are always present, unless the OBU consists of only
	 * the header. Trailing bits achieve byte alignment when the payload of
	 * an OBU is not byte aligned. The trailing bits may also used for
	 * additional byte padding, and if used are taken into account in the
	 * sz value. In all cases, the pattern used for the trailing bits
	 * guarantees that all OBUs (except header-only OBUs) end with the same
	 * pattern: one bit set to one, optionally followed by zeros. */

	(*out_buffer)[offset] = 0x80;
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
