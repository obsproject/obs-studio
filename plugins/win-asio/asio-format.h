/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef ASIO_FORMAT_H
#define ASIO_FORMAT_H

#include <stdbool.h>

typedef struct asio_sample_format {
	int bit_depth;
	int byte_stride;
	bool format_is_float;
	bool little_endian;
} asio_sample_format;

void asio_format_init(asio_sample_format *fmt, long type);

void asio_format_convert_to_float(const asio_sample_format *fmt, const void *src, float *dst, int samps);

void asio_format_convert_from_float(const asio_sample_format *fmt, const float *src, void *dst, int samps);

void asio_format_clear(const asio_sample_format *fmt, void *dst, int samps);

#endif
