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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct asio_sample_format {
	int bit_depth;
	int byte_stride;
	bool format_is_float;
} asio_sample_format;

bool asio_format_init(asio_sample_format *fmt, long type);

void asio_format_convert_to_float(const asio_sample_format *fmt, const void *src, float *dst, int samps);

void asio_format_convert_from_float(const asio_sample_format *fmt, const float *src, void *dst, int samps);

void asio_format_clear(const asio_sample_format *fmt, void *dst, int samps);

static inline int16_t ByteOrder_littleEndianShort(const void *p)
{
	uint16_t v;
	memcpy(&v, p, sizeof(v));
	return (int16_t)v;
}

static inline int32_t ByteOrder_littleEndianInt(const void *p)
{
	uint32_t v;
	memcpy(&v, p, sizeof(v));
	return (int32_t)v;
}

static inline int32_t ByteOrder_littleEndian24Bit(const void *p)
{
	const uint8_t *b = (const uint8_t *)p;
	const int8_t *signed_bytes = (const int8_t *)p;

	return (int32_t)(((uint32_t)signed_bytes[2] << 16) | ((uint32_t)b[1] << 8) | (uint32_t)b[0]);
}

static inline void ByteOrder_littleEndian24BitToChars(uint32_t val, void *p)
{
	uint8_t *b = (uint8_t *)p;
	b[0] = (uint8_t)(val & 0xFF);
	b[1] = (uint8_t)((val >> 8) & 0xFF);
	b[2] = (uint8_t)((val >> 16) & 0xFF);
}
