/******************************************************************************
    Copyright (C) 2023 by Hugh Bailey <obs.jim@gmail.com>

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

#include "rtmp-av1.h"
#include "utils.h"

#include <obs.h>
#include <util/array-serializer.h>

/* Adapted from FFmpeg's libavformat/av1.c for our FLV muxer. */

#define AV1_OBU_SEQUENCE_HEADER 1
#define AV1_OBU_TEMPORAL_DELIMITER 2
#define AV1_OBU_REDUNDANT_FRAME_HEADER 7
#define AV1_OBU_TILE_LIST 8
#define AV1_OBU_PADDING 15
#define AV1_OBU_METADATA 5

#define AV1_OBU_TILE_GROUP 4
#define AV1_OBU_TILE_LIST 8
#define AV1_OBU_FRAME 6

#define FF_PROFILE_AV1_MAIN 0
#define FF_PROFILE_AV1_HIGH 1
#define FF_PROFILE_AV1_PROFESSIONAL 2

typedef struct AV1SequenceParameters {
	uint8_t profile;
	uint8_t level;
	uint8_t tier;
	uint8_t bitdepth;
	uint8_t monochrome;
	uint8_t chroma_subsampling_x;
	uint8_t chroma_subsampling_y;
	uint8_t chroma_sample_position;
	uint8_t color_description_present_flag;
	uint8_t color_primaries;
	uint8_t transfer_characteristics;
	uint8_t matrix_coefficients;
	uint8_t color_range;
} AV1SequenceParameters;

#define MAX_OBU_HEADER_SIZE (2 + 8)

typedef struct Av1GetBitContext {
	const uint8_t *buffer, *buffer_end;
	int index;
	int size_in_bits;
	int size_in_bits_plus8;
} Av1GetBitContext;

static inline int init_get_bits_xe(Av1GetBitContext *s, const uint8_t *buffer,
				   int bit_size)
{
	int buffer_size;
	int ret = 0;

	if (bit_size >= INT_MAX - 64 * 8 || bit_size < 0 || !buffer) {
		bit_size = 0;
		buffer = NULL;
		ret = -1;
	}

	buffer_size = (bit_size + 7) >> 3;

	s->buffer = buffer;
	s->size_in_bits = bit_size;
	s->size_in_bits_plus8 = bit_size + 8;
	s->buffer_end = buffer + buffer_size;
	s->index = 0;

	return ret;
}

static inline int init_get_bits(Av1GetBitContext *s, const uint8_t *buffer,
				int bit_size)
{
	return init_get_bits_xe(s, buffer, bit_size);
}

static inline int init_get_bits8(Av1GetBitContext *s, const uint8_t *buffer,
				 int byte_size)
{
	if (byte_size > INT_MAX / 8 || byte_size < 0)
		byte_size = -1;
	return init_get_bits(s, buffer, byte_size * 8);
}

static inline unsigned int get_bit1(Av1GetBitContext *s)
{
	unsigned int index = s->index;
	uint8_t result = s->buffer[index >> 3];

	result <<= index & 7;
	result >>= 8 - 1;

	if (s->index < s->size_in_bits_plus8)
		index++;
	s->index = index;

	return result;
}

static inline unsigned int get_bits(Av1GetBitContext *s, unsigned int n)
{
	unsigned int out = 0;
	for (unsigned int i = 0; i < n; i++)
		out = (out << 1) | get_bit1(s);
	return out;
}

#define skip_bits get_bits

static inline int get_bits_count(Av1GetBitContext *s)
{
	return s->index;
}

static inline int get_bits_left(Av1GetBitContext *gb)
{
	return gb->size_in_bits - get_bits_count(gb);
}

#define get_bits_long get_bits
#define skip_bits_long get_bits_long

static inline int64_t leb128(Av1GetBitContext *gb)
{
	int64_t ret = 0;
	int i;

	for (i = 0; i < 8; i++) {
		int byte = get_bits(gb, 8);
		ret |= (int64_t)(byte & 0x7f) << (i * 7);
		if (!(byte & 0x80))
			break;
	}
	return ret;
}

static inline void uvlc(Av1GetBitContext *gb)
{
	int leading_zeros = 0;

	while (get_bits_left(gb)) {
		if (get_bits(gb, 1))
			break;
		leading_zeros++;
	}

	if (leading_zeros >= 32)
		return;

	skip_bits_long(gb, leading_zeros);
}

static inline int parse_obu_header(const uint8_t *buf, int buf_size,
				   int64_t *obu_size, int *start_pos, int *type,
				   int *temporal_id, int *spatial_id)
{
	Av1GetBitContext gb;
	int ret, extension_flag, has_size_flag;
	size_t size;

	ret = init_get_bits8(&gb, buf, min_i32(buf_size, MAX_OBU_HEADER_SIZE));
	if (ret < 0)
		return ret;

	if (get_bits(&gb, 1) != 0) // obu_forbidden_bit
		return -1;

	*type = get_bits(&gb, 4);
	extension_flag = get_bits(&gb, 1);
	has_size_flag = get_bits(&gb, 1);
	skip_bits(&gb, 1); // obu_reserved_1bit

	if (extension_flag) {
		*temporal_id = get_bits(&gb, 3);
		*spatial_id = get_bits(&gb, 2);
		skip_bits(&gb, 3); // extension_header_reserved_3bits
	} else {
		*temporal_id = *spatial_id = 0;
	}

	*obu_size = has_size_flag ? leb128(&gb) : buf_size - 1 - extension_flag;

	if (get_bits_left(&gb) < 0)
		return -1;

	*start_pos = get_bits_count(&gb) / 8;

	size = (size_t)(*obu_size + *start_pos);

	if (size > (size_t)buf_size)
		return -1;

	assert(size <= INT_MAX);
	return (int)size;
}

static inline int get_obu_bit_length(const uint8_t *buf, int size, int type)
{
	int v;

	/* There are no trailing bits on these */
	if (type == AV1_OBU_TILE_GROUP || type == AV1_OBU_TILE_LIST ||
	    type == AV1_OBU_FRAME) {
		if (size > INT_MAX / 8)
			return -1;
		else
			return size * 8;
	}

	while (size > 0 && buf[size - 1] == 0)
		size--;

	if (!size)
		return 0;

	v = buf[size - 1];

	if (size > INT_MAX / 8)
		return -1;
	size *= 8;

	/* Remove the trailing_one_bit and following trailing zeros */
	if (v)
		size -= ctz32(v) + 1;

	return size;
}

static int parse_color_config(AV1SequenceParameters *seq_params,
			      Av1GetBitContext *gb)
{
	int twelve_bit = 0;
	int high_bitdepth = get_bits(gb, 1);
	if (seq_params->profile == FF_PROFILE_AV1_PROFESSIONAL && high_bitdepth)
		twelve_bit = get_bits(gb, 1);

	seq_params->bitdepth = 8 + (high_bitdepth * 2) + (twelve_bit * 2);

	if (seq_params->profile == FF_PROFILE_AV1_HIGH)
		seq_params->monochrome = 0;
	else
		seq_params->monochrome = get_bits(gb, 1);

	seq_params->color_description_present_flag = get_bits(gb, 1);
	if (seq_params->color_description_present_flag) {
		seq_params->color_primaries = get_bits(gb, 8);
		seq_params->transfer_characteristics = get_bits(gb, 8);
		seq_params->matrix_coefficients = get_bits(gb, 8);
	} else {
		seq_params->color_primaries = 2;
		seq_params->transfer_characteristics = 2;
		seq_params->matrix_coefficients = 2;
	}

	if (seq_params->monochrome) {
		seq_params->color_range = get_bits(gb, 1);
		seq_params->chroma_subsampling_x = 1;
		seq_params->chroma_subsampling_y = 1;
		seq_params->chroma_sample_position = 0;
		return 0;
	} else if (seq_params->color_primaries == 1 &&
		   seq_params->transfer_characteristics == 13 &&
		   seq_params->matrix_coefficients == 0) {
		seq_params->chroma_subsampling_x = 0;
		seq_params->chroma_subsampling_y = 0;
	} else {
		seq_params->color_range = get_bits(gb, 1);

		if (seq_params->profile == FF_PROFILE_AV1_MAIN) {
			seq_params->chroma_subsampling_x = 1;
			seq_params->chroma_subsampling_y = 1;
		} else if (seq_params->profile == FF_PROFILE_AV1_HIGH) {
			seq_params->chroma_subsampling_x = 0;
			seq_params->chroma_subsampling_y = 0;
		} else {
			if (twelve_bit) {
				seq_params->chroma_subsampling_x =
					get_bits(gb, 1);
				if (seq_params->chroma_subsampling_x)
					seq_params->chroma_subsampling_y =
						get_bits(gb, 1);
				else
					seq_params->chroma_subsampling_y = 0;
			} else {
				seq_params->chroma_subsampling_x = 1;
				seq_params->chroma_subsampling_y = 0;
			}
		}
		if (seq_params->chroma_subsampling_x &&
		    seq_params->chroma_subsampling_y)
			seq_params->chroma_sample_position = get_bits(gb, 2);
	}

	skip_bits(gb, 1); // separate_uv_delta_q

	return 0;
}

static int parse_sequence_header(AV1SequenceParameters *seq_params,
				 const uint8_t *buf, int size)
{
	Av1GetBitContext gb;
	int reduced_still_picture_header;
	int frame_width_bits_minus_1, frame_height_bits_minus_1;
	int size_bits, ret;

	size_bits = get_obu_bit_length(buf, size, AV1_OBU_SEQUENCE_HEADER);
	if (size_bits < 0)
		return size_bits;

	ret = init_get_bits(&gb, buf, size_bits);
	if (ret < 0)
		return ret;

	memset(seq_params, 0, sizeof(*seq_params));

	seq_params->profile = get_bits(&gb, 3);

	skip_bits(&gb, 1); // still_picture
	reduced_still_picture_header = get_bits(&gb, 1);

	if (reduced_still_picture_header) {
		seq_params->level = get_bits(&gb, 5);
		seq_params->tier = 0;
	} else {
		int initial_display_delay_present_flag,
			operating_points_cnt_minus_1;
		int decoder_model_info_present_flag,
			buffer_delay_length_minus_1;

		if (get_bits(&gb, 1)) {          // timing_info_present_flag
			skip_bits_long(&gb, 32); // num_units_in_display_tick
			skip_bits_long(&gb, 32); // time_scale

			if (get_bits(&gb, 1)) // equal_picture_interval
				uvlc(&gb);    // num_ticks_per_picture_minus_1

			decoder_model_info_present_flag = get_bits(&gb, 1);
			if (decoder_model_info_present_flag) {
				buffer_delay_length_minus_1 = get_bits(&gb, 5);
				skip_bits_long(&gb, 32);
				skip_bits(&gb, 10);
			}
		} else
			decoder_model_info_present_flag = 0;

		initial_display_delay_present_flag = get_bits(&gb, 1);

		operating_points_cnt_minus_1 = get_bits(&gb, 5);
		for (int i = 0; i <= operating_points_cnt_minus_1; i++) {
			int seq_level_idx, seq_tier;

			skip_bits(&gb, 12);
			seq_level_idx = get_bits(&gb, 5);

			if (seq_level_idx > 7)
				seq_tier = get_bits(&gb, 1);
			else
				seq_tier = 0;

			if (decoder_model_info_present_flag) {
				if (get_bits(&gb, 1)) {
					skip_bits_long(
						&gb,
						buffer_delay_length_minus_1 +
							1);
					skip_bits_long(
						&gb,
						buffer_delay_length_minus_1 +
							1);
					skip_bits(&gb, 1);
				}
			}

			if (initial_display_delay_present_flag) {
				if (get_bits(&gb, 1))
					skip_bits(&gb, 4);
			}

			if (i == 0) {
				seq_params->level = seq_level_idx;
				seq_params->tier = seq_tier;
			}
		}
	}

	frame_width_bits_minus_1 = get_bits(&gb, 4);
	frame_height_bits_minus_1 = get_bits(&gb, 4);

	skip_bits(&gb, frame_width_bits_minus_1 + 1); // max_frame_width_minus_1
	skip_bits(&gb,
		  frame_height_bits_minus_1 + 1); // max_frame_height_minus_1

	if (!reduced_still_picture_header) {
		if (get_bits(&gb, 1)) // frame_id_numbers_present_flag
			skip_bits(&gb, 7);
	}

	skip_bits(
		&gb,
		3); // use_128x128_superblock (1), enable_filter_intra (1), enable_intra_edge_filter (1)

	if (!reduced_still_picture_header) {
		int enable_order_hint, seq_force_screen_content_tools;

		skip_bits(&gb, 4);

		enable_order_hint = get_bits(&gb, 1);
		if (enable_order_hint)
			skip_bits(&gb, 2);

		if (get_bits(&gb, 1)) // seq_choose_screen_content_tools
			seq_force_screen_content_tools = 2;
		else
			seq_force_screen_content_tools = get_bits(&gb, 1);

		if (seq_force_screen_content_tools) {
			if (!get_bits(&gb, 1))     // seq_choose_integer_mv
				skip_bits(&gb, 1); // seq_force_integer_mv
		}

		if (enable_order_hint)
			skip_bits(&gb, 3); // order_hint_bits_minus_1
	}

	skip_bits(&gb, 3);

	parse_color_config(seq_params, &gb);

	skip_bits(&gb, 1); // film_grain_params_present

	if (get_bits_left(&gb))
		return -1;

	return 0;
}

size_t obs_parse_av1_header(uint8_t **header, const uint8_t *data, size_t size)
{
	if (data[0] & 0x80) {
		int config_record_version = data[0] & 0x7f;
		if (config_record_version != 1 || size < 4)
			return 0;

		*header = bmemdup(data, size);
		return size;
	}

	// AV1S init
	AV1SequenceParameters seq_params;
	int nb_seq = 0, seq_size = 0, meta_size = 0;
	const uint8_t *seq = 0, *meta = 0;

	uint8_t *buf = (uint8_t *)data;
	while (size > 0) {
		int64_t obu_size;
		int start_pos, type, temporal_id, spatial_id;
		assert(size <= INT_MAX);
		int len = parse_obu_header(buf, (int)size, &obu_size,
					   &start_pos, &type, &temporal_id,
					   &spatial_id);
		if (len < 0)
			return 0;

		switch (type) {
		case AV1_OBU_SEQUENCE_HEADER:
			nb_seq++;
			if (!obu_size || nb_seq > 1) {
				return 0;
			}
			assert(obu_size <= INT_MAX);
			if (parse_sequence_header(&seq_params, buf + start_pos,
						  (int)obu_size) < 0)
				return 0;

			seq = buf;
			seq_size = len;
			break;
		case AV1_OBU_METADATA:
			if (!obu_size)
				return 0;
			meta = buf;
			meta_size = len;
			break;
		default:
			break;
		}
		size -= len;
		buf += len;
	}

	if (!nb_seq)
		return 0;

	uint8_t av1header[4];
	av1header[0] = (1 << 7) | 1; // marker and version
	av1header[1] = (seq_params.profile << 5) | (seq_params.level);
	av1header[2] = (seq_params.tier << 7) | (seq_params.bitdepth > 8) << 6 |
		       (seq_params.bitdepth == 12) << 5 |
		       (seq_params.monochrome) << 4 |
		       (seq_params.chroma_subsampling_x) << 3 |
		       (seq_params.chroma_subsampling_y) << 2 |
		       (seq_params.chroma_sample_position);
	av1header[3] = 0;

	struct array_output_data output;
	struct serializer s;

	array_output_serializer_init(&s, &output);

	s_write(&s, av1header, sizeof(av1header));
	if (seq_size)
		s_write(&s, seq, seq_size);
	if (meta_size)
		s_write(&s, meta, meta_size);

	*header = output.bytes.array;
	return output.bytes.num;
}

static void serialize_av1_data(struct serializer *s, const uint8_t *data,
			       size_t size, bool *is_keyframe, int *priority)
{
	(void)is_keyframe;
	(void)priority;
	uint8_t *buf = (uint8_t *)data;
	uint8_t *end = (uint8_t *)data + size;
	enum {
		START_NOT_FOUND,
		START_FOUND,
		END_FOUND,
		OFFSET_IMPOSSIBLE,
	} state = START_NOT_FOUND;

	while (buf < end) {
		int64_t obu_size;
		int start_pos, type, temporal_id, spatial_id;
		assert(end - buf <= INT_MAX);
		int len = parse_obu_header(buf, (int)(end - buf), &obu_size,
					   &start_pos, &type, &temporal_id,
					   &spatial_id);
		if (len < 0)
			return;

		switch (type) {
		case AV1_OBU_TEMPORAL_DELIMITER:
		case AV1_OBU_REDUNDANT_FRAME_HEADER:
		case AV1_OBU_TILE_LIST:
			if (state == START_FOUND)
				state = END_FOUND;
			break;
		default:
			if (state == START_NOT_FOUND) {
				state = START_FOUND;
			} else if (state == END_FOUND) {
				state = OFFSET_IMPOSSIBLE;
			}
			s_write(s, buf, len);
			size += len;
			break;
		}
		buf += len;
	}
}

void obs_parse_av1_packet(struct encoder_packet *av1_packet,
			  const struct encoder_packet *src)
{
	struct array_output_data output;
	struct serializer s;
	long ref = 1;

	array_output_serializer_init(&s, &output);
	serialize(&s, &ref, sizeof(ref));

	*av1_packet = *src;
	serialize_av1_data(&s, src->data, src->size, &av1_packet->keyframe,
			   &av1_packet->priority);

	av1_packet->data = output.bytes.array + sizeof(ref);
	av1_packet->size = output.bytes.num - sizeof(ref);
	av1_packet->drop_priority = av1_packet->priority;
}
