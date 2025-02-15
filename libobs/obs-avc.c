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

#include "obs-avc.h"

#include "obs.h"
#include "obs-nal.h"
#include "util/array-serializer.h"
#include "util/bitstream.h"

bool obs_avc_keyframe(const uint8_t *data, size_t size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;

	nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		const uint8_t type = nal_start[0] & 0x1F;

		if (type == OBS_NAL_SLICE_IDR || type == OBS_NAL_SLICE)
			return type == OBS_NAL_SLICE_IDR;

		nal_end = obs_nal_find_startcode(nal_start, end);
		nal_start = nal_end;
	}

	return false;
}

const uint8_t *obs_avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	return obs_nal_find_startcode(p, end);
}

static int compute_avc_keyframe_priority(const uint8_t *nal_start, bool *is_keyframe, int priority)
{
	const int type = nal_start[0] & 0x1F;
	if (type == OBS_NAL_SLICE_IDR)
		*is_keyframe = true;

	const int new_priority = nal_start[0] >> 5;
	if (priority < new_priority)
		priority = new_priority;

	return priority;
}

static void serialize_avc_data(struct serializer *s, const uint8_t *data, size_t size, bool *is_keyframe, int *priority)
{
	const uint8_t *const end = data + size;
	const uint8_t *nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		*priority = compute_avc_keyframe_priority(nal_start, is_keyframe, *priority);

		const uint8_t *const nal_end = obs_nal_find_startcode(nal_start, end);
		const size_t nal_size = nal_end - nal_start;
		s_wb32(s, (uint32_t)nal_size);
		s_write(s, nal_start, nal_size);
		nal_start = nal_end;
	}
}

void obs_parse_avc_packet(struct encoder_packet *avc_packet, const struct encoder_packet *src)
{
	struct array_output_data output;
	struct serializer s;
	long ref = 1;

	array_output_serializer_init(&s, &output);
	*avc_packet = *src;

	serialize(&s, &ref, sizeof(ref));
	serialize_avc_data(&s, src->data, src->size, &avc_packet->keyframe, &avc_packet->priority);

	avc_packet->data = output.bytes.array + sizeof(ref);
	avc_packet->size = output.bytes.num - sizeof(ref);
	avc_packet->drop_priority = avc_packet->priority;
}

int obs_parse_avc_packet_priority(const struct encoder_packet *packet)
{
	int priority = packet->priority;

	const uint8_t *const data = packet->data;
	const uint8_t *const end = data + packet->size;
	const uint8_t *nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		bool unused;
		priority = compute_avc_keyframe_priority(nal_start, &unused, priority);

		nal_start = obs_nal_find_startcode(nal_start, end);
	}

	return priority;
}

static inline bool has_start_code(const uint8_t *data)
{
	if (data[0] != 0 || data[1] != 0)
		return false;

	return data[2] == 1 || (data[2] == 0 && data[3] == 1);
}

static void get_sps_pps(const uint8_t *data, size_t size, const uint8_t **sps, size_t *sps_size, const uint8_t **pps,
			size_t *pps_size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;
	int type;

	nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		nal_end = obs_nal_find_startcode(nal_start, end);

		type = nal_start[0] & 0x1F;
		if (type == OBS_NAL_SPS) {
			*sps = nal_start;
			*sps_size = nal_end - nal_start;
		} else if (type == OBS_NAL_PPS) {
			*pps = nal_start;
			*pps_size = nal_end - nal_start;
		}

		nal_start = nal_end;
	}
}

static inline uint8_t get_ue_golomb(struct bitstream_reader *gb)
{
	int i = 0;
	while (i < 32 && !bitstream_reader_read_bits(gb, 1))
		i++;

	return bitstream_reader_read_bits(gb, i) + (1 << i) - 1;
}

static void get_sps_high_params(const uint8_t *sps, size_t size, uint8_t *chroma_format_idc, uint8_t *bit_depth_luma,
				uint8_t *bit_depth_chroma)
{
	struct bitstream_reader gb;

	/* Extract RBSP */
	uint8_t *rbsp = bzalloc(size);

	size_t i = 0;
	size_t rbsp_size = 0;

	while (i + 2 < size) {
		if (sps[i] == 0 && sps[i + 1] == 0 && sps[i + 2] == 3) {
			rbsp[rbsp_size++] = sps[i++];
			rbsp[rbsp_size++] = sps[i++];
			// skip emulation_prevention_three_byte
			i++;
		} else {
			rbsp[rbsp_size++] = sps[i++];
		}
	}

	while (i < size)
		rbsp[rbsp_size++] = sps[i++];

	/* Read relevant information from SPS */
	bitstream_reader_init(&gb, rbsp, rbsp_size);

	// skip a whole bunch of stuff we don't care about
	bitstream_reader_read_bits(&gb, 24); // profile, constraint flags, level
	get_ue_golomb(&gb);                  // id

	*chroma_format_idc = get_ue_golomb(&gb);
	// skip separate_colour_plane_flag
	if (*chroma_format_idc == 3)
		bitstream_reader_read_bits(&gb, 1);

	*bit_depth_luma = get_ue_golomb(&gb);
	*bit_depth_chroma = get_ue_golomb(&gb);

	bfree(rbsp);
}

size_t obs_parse_avc_header(uint8_t **header, const uint8_t *data, size_t size)
{
	struct array_output_data output;
	struct serializer s;
	const uint8_t *sps = NULL, *pps = NULL;
	size_t sps_size = 0, pps_size = 0;

	array_output_serializer_init(&s, &output);

	if (size <= 6)
		return 0;

	if (!has_start_code(data)) {
		*header = bmemdup(data, size);
		return size;
	}

	get_sps_pps(data, size, &sps, &sps_size, &pps, &pps_size);
	if (!sps || !pps || sps_size < 4)
		return 0;

	s_w8(&s, 0x01);
	s_write(&s, sps + 1, 3);
	s_w8(&s, 0xff);
	s_w8(&s, 0xe1);

	s_wb16(&s, (uint16_t)sps_size);
	s_write(&s, sps, sps_size);
	s_w8(&s, 0x01);
	s_wb16(&s, (uint16_t)pps_size);
	s_write(&s, pps, pps_size);

	uint8_t profile_idc = sps[1];

	/* Additional data required for high, high10, high422, high444 profiles.
	 * See ISO/IEC 14496-15 Section 5.3.3.1.2. */
	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244) {
		uint8_t chroma_format_idc, bit_depth_luma, bit_depth_chroma;
		get_sps_high_params(sps + 1, sps_size - 1, &chroma_format_idc, &bit_depth_luma, &bit_depth_chroma);

		// reserved + chroma_format
		s_w8(&s, 0xfc | chroma_format_idc);
		// reserved + bit_depth_luma_minus8
		s_w8(&s, 0xf8 | bit_depth_luma);
		// reserved + bit_depth_chroma_minus8
		s_w8(&s, 0xf8 | bit_depth_chroma);
		// numOfSequenceParameterSetExt
		s_w8(&s, 0);
	}

	*header = output.bytes.array;
	return output.bytes.num;
}

void obs_extract_avc_headers(const uint8_t *packet, size_t size, uint8_t **new_packet_data, size_t *new_packet_size,
			     uint8_t **header_data, size_t *header_size, uint8_t **sei_data, size_t *sei_size)
{
	DARRAY(uint8_t) new_packet;
	DARRAY(uint8_t) header;
	DARRAY(uint8_t) sei;
	const uint8_t *nal_start, *nal_end, *nal_codestart;
	const uint8_t *end = packet + size;

	da_init(new_packet);
	da_init(header);
	da_init(sei);

	nal_start = obs_nal_find_startcode(packet, end);
	nal_end = NULL;
	while (nal_end != end) {
		nal_codestart = nal_start;

		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		const uint8_t type = nal_start[0] & 0x1F;

		nal_end = obs_nal_find_startcode(nal_start, end);
		if (!nal_end)
			nal_end = end;

		if (type == OBS_NAL_SPS || type == OBS_NAL_PPS) {
			da_push_back_array(header, nal_codestart, nal_end - nal_codestart);
		} else if (type == OBS_NAL_SEI) {
			da_push_back_array(sei, nal_codestart, nal_end - nal_codestart);

		} else {
			da_push_back_array(new_packet, nal_codestart, nal_end - nal_codestart);
		}

		nal_start = nal_end;
	}

	*new_packet_data = new_packet.array;
	*new_packet_size = new_packet.num;
	*header_data = header.array;
	*header_size = header.num;
	*sei_data = sei.array;
	*sei_size = sei.num;
}
