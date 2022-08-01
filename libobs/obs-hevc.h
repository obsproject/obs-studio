/******************************************************************************
    Copyright (C) 2022 by Hugh Bailey <obs.jim@gmail.com>

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

#pragma once

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct encoder_packet;

EXPORT bool obs_hevc_keyframe(const uint8_t *data, size_t size);
EXPORT void obs_parse_hevc_packet(struct encoder_packet *hevc_packet,
				  const struct encoder_packet *src);
EXPORT void obs_extract_hevc_headers(const uint8_t *packet, size_t size,
				     uint8_t **new_packet_data,
				     size_t *new_packet_size,
				     uint8_t **header_data, size_t *header_size,
				     uint8_t **sei_data, size_t *sei_size);

#ifdef __cplusplus
}
#endif
