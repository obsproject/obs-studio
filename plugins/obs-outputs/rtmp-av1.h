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

#pragma once

#include <stdint.h>
#include <stddef.h>

struct encoder_packet;

extern void obs_parse_av1_packet(struct encoder_packet *avc_packet,
				 const struct encoder_packet *src);
extern size_t obs_parse_av1_header(uint8_t **header, const uint8_t *data,
				   size_t size);
