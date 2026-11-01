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

#include "obs-hevc.h"

#include "obs.h"
#include "obs-nal.h"
#include "util/array-serializer.h"

bool obs_hevc_keyframe(const uint8_t *data, size_t size)
{
	const uint8_t *nal_start, *nal_end;
	const uint8_t *end = data + size;

	nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		const uint8_t type = (nal_start[0] & 0x7F) >> 1;

		if (type <= OBS_HEVC_NAL_RSV_IRAP_VCL23)
			return type >= OBS_HEVC_NAL_BLA_W_LP;

		nal_end = obs_nal_find_startcode(nal_start, end);
		nal_start = nal_end;
	}

	return false;
}

static int compute_hevc_keyframe_priority(const uint8_t *nal_start, bool *is_keyframe, int priority)
{
	int new_priority;
	// HEVC contains NAL unit specifier at [6..1] bits of
	// the byte next to the startcode 0x000001
	const int type = (nal_start[0] & 0x7F) >> 1;

	switch (type) {
	case OBS_HEVC_NAL_BLA_W_LP:
	case OBS_HEVC_NAL_BLA_W_RADL:
	case OBS_HEVC_NAL_BLA_N_LP:
	case OBS_HEVC_NAL_IDR_W_RADL:
	case OBS_HEVC_NAL_IDR_N_LP:
	case OBS_HEVC_NAL_CRA_NUT:
	case OBS_HEVC_NAL_RSV_IRAP_VCL22:
	case OBS_HEVC_NAL_RSV_IRAP_VCL23:
		/* intra random access point (IRAP) picture, keyframe and highest priority */
		*is_keyframe = true;
		new_priority = OBS_NAL_PRIORITY_HIGHEST;
		break;
	case OBS_HEVC_NAL_TRAIL_R:
	case OBS_HEVC_NAL_TSA_R:
	case OBS_HEVC_NAL_STSA_R:
	case OBS_HEVC_NAL_RADL_R:
	case OBS_HEVC_NAL_RASL_R:
		/* sub-layer reference picture (mainly P-frames), high priority */
		new_priority = OBS_NAL_PRIORITY_HIGH;
		break;
	case OBS_HEVC_NAL_TRAIL_N:
	case OBS_HEVC_NAL_TSA_N:
	case OBS_HEVC_NAL_STSA_N:
	case OBS_HEVC_NAL_RADL_N:
	case OBS_HEVC_NAL_RASL_N:
		/* sub-layer non-reference (SLNR) picture (mainly B-frames), disposable */
		new_priority = OBS_NAL_PRIORITY_DISPOSABLE;
		break;
	default:
		new_priority = OBS_NAL_PRIORITY_DISPOSABLE;
	}

	return priority > new_priority ? priority : new_priority;
}

static void serialize_hevc_data(struct serializer *s, const uint8_t *data, size_t size, bool *is_keyframe,
				int *priority)
{
	const uint8_t *const end = data + size;
	const uint8_t *nal_start = obs_nal_find_startcode(data, end);
	while (true) {
		while (nal_start < end && !*(nal_start++))
			;

		if (nal_start == end)
			break;

		*priority = compute_hevc_keyframe_priority(nal_start, is_keyframe, *priority);

		const uint8_t *const nal_end = obs_nal_find_startcode(nal_start, end);
		const size_t nal_size = nal_end - nal_start;
		s_wb32(s, (uint32_t)nal_size);
		s_write(s, nal_start, nal_size);
		nal_start = nal_end;
	}
}

void obs_parse_hevc_packet(struct encoder_packet *hevc_packet, const struct encoder_packet *src)
{
	struct array_output_data output;
	struct serializer s;
	long ref = 1;

	array_output_serializer_init(&s, &output);
	*hevc_packet = *src;

	serialize(&s, &ref, sizeof(ref));
	serialize_hevc_data(&s, src->data, src->size, &hevc_packet->keyframe, &hevc_packet->priority);

	hevc_packet->data = output.bytes.array + sizeof(ref);
	hevc_packet->size = output.bytes.num - sizeof(ref);
	hevc_packet->drop_priority = hevc_packet->priority;
}

int obs_parse_hevc_packet_priority(const struct encoder_packet *packet)
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
		priority = compute_hevc_keyframe_priority(nal_start, &unused, priority);

		nal_start = obs_nal_find_startcode(nal_start, end);
	}

	return priority;
}

void obs_extract_hevc_headers(const uint8_t *packet, size_t size, uint8_t **new_packet_data, size_t *new_packet_size,
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

		const uint8_t type = (nal_start[0] & 0x7F) >> 1;

		nal_end = obs_nal_find_startcode(nal_start, end);
		if (!nal_end)
			nal_end = end;

		if (type == OBS_HEVC_NAL_VPS || type == OBS_HEVC_NAL_SPS || type == OBS_HEVC_NAL_PPS) {
			da_push_back_array(header, nal_codestart, nal_end - nal_codestart);
		} else if (type == OBS_HEVC_NAL_SEI_PREFIX || type == OBS_HEVC_NAL_SEI_SUFFIX) {
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
