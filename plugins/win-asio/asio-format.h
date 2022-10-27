/* Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 * This file is part of win-asio.
 *
 * This file uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
 *
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later, to comply with the SDK license.
 */
#ifndef ASIO_FORMAT_H
#define ASIO_FORMAT_H

#include <stdbool.h>
#include <stdint.h>
#include "asio.h"

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
