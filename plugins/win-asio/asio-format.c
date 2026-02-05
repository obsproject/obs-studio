/******************************************************************************
    Copyright (C) 2022-2026 pkv <pkv@obsproject.com>

    This file is part of win-asio.
    It uses the Steinberg ASIO SDK, which is licensed under the GNU GPL v3.
    It is partly based on JUCE ASIoSampleFormat struct in juce_ASIO_windows.cpp
    licensed under GPL v3.

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
#include "asio-format.h"
#include "asio.h"
#include "byteorder.h"

#include <string.h>
#include <math.h>
#include <assert.h>

static double jlimit(double lower, double upper, double val)
{
	return val < lower ? lower : (val > upper ? upper : val);
}

void asio_format_init(asio_sample_format *fmt, long type)
{
	*fmt = (asio_sample_format){.bit_depth = 24, .byte_stride = 4, .format_is_float = false, .little_endian = true};

	switch (type) {
	case ASIOSTInt16MSB:
		fmt->bit_depth = 16;
		fmt->byte_stride = 2;
		fmt->little_endian = false;
		break;
	case ASIOSTInt24MSB:
		fmt->bit_depth = 24;
		fmt->byte_stride = 3;
		fmt->little_endian = false;
		break;
	case ASIOSTInt32MSB:
		fmt->bit_depth = 32;
		fmt->little_endian = false;
		break;
	case ASIOSTFloat32MSB:
		fmt->bit_depth = 32;
		fmt->format_is_float = true;
		fmt->little_endian = false;
		break;
	case ASIOSTFloat64MSB:
		fmt->bit_depth = 64;
		fmt->byte_stride = 8;
		fmt->format_is_float = true;
		fmt->little_endian = false;
		break;
	case ASIOSTInt16LSB:
		fmt->bit_depth = 16;
		fmt->byte_stride = 2;
		break;
	case ASIOSTInt24LSB:
		fmt->bit_depth = 24;
		fmt->byte_stride = 3;
		break;
	case ASIOSTInt32LSB:
		fmt->bit_depth = 32;
		break;
	case ASIOSTFloat32LSB:
		fmt->bit_depth = 32;
		fmt->format_is_float = true;
		break;
	case ASIOSTFloat64LSB:
		fmt->bit_depth = 64;
		fmt->byte_stride = 8;
		fmt->format_is_float = true;
		break;
	default:
		break;
	}
}

void asio_format_convert_to_float(const asio_sample_format *fmt, const void *src, float *dst, int samps)
{
	const char *s = (const char *)src;
	if (fmt->format_is_float) {
		memcpy(dst, src, samps * sizeof(float));
		return;
	}

	const double g16 = 1.0 / 32768.0;
	const double g24 = 1.0 / 0x7FFFFF;
	const double g32 = 1.0 / 0x7FFFFFFF;

	switch (fmt->bit_depth) {
	case 16:
		while (--samps >= 0) {
			int16_t val = fmt->little_endian ? ByteOrder_littleEndianShort(s) : ByteOrder_bigEndianShort(s);
			*dst++ = (float)(g16 * val);
			s += fmt->byte_stride;
		}
		break;
	case 24:
		while (--samps >= 0) {
			int32_t val = fmt->little_endian ? ByteOrder_littleEndian24Bit(s) : ByteOrder_bigEndian24Bit(s);
			*dst++ = (float)(g24 * val);
			s += fmt->byte_stride;
		}
		break;
	case 32:
		while (--samps >= 0) {
			int32_t val = fmt->little_endian ? ByteOrder_littleEndianInt(s) : ByteOrder_bigEndianInt(s);
			*dst++ = (float)(g32 * val);
			s += fmt->byte_stride;
		}
		break;
	default:
		break;
	}
}

void asio_format_convert_from_float(const asio_sample_format *fmt, const float *src, void *dst, int samps)
{
	char *d = (char *)dst;
	if (fmt->format_is_float) {
		memcpy(dst, src, samps * sizeof(float));
		return;
	}

	const double max16 = 32767.0;
	const double max24 = 0x7FFFFF;
	const double max32 = 0x7FFFFFFF;

	switch (fmt->bit_depth) {
	case 16:
		while (--samps >= 0) {
			int16_t val = (int16_t)jlimit(-max16, max16, max16 * *src++);
			uint16_t word = (uint16_t)val;
			word = fmt->little_endian ? ByteOrder_swapIfBigEndian16(word)
						  : ByteOrder_swapIfLittleEndian16(word);
			memcpy(d, &word, 2);
			d += fmt->byte_stride;
		}
		break;
	case 24:
		while (--samps >= 0) {
			int32_t val = (int32_t)jlimit(-max24, max24, max24 * *src++);
			uint32_t uval = (uint32_t)val;
			if (fmt->little_endian)
				ByteOrder_littleEndian24BitToChars(uval, d);
			else
				ByteOrder_bigEndian24BitToChars(uval, d);
			d += fmt->byte_stride;
		}
		break;
	case 32:
		while (--samps >= 0) {
			int32_t val = (int32_t)jlimit(-max32, max32, max32 * *src++);
			uint32_t uval = (uint32_t)val;
			uval = fmt->little_endian ? ByteOrder_swapIfBigEndian32(uval)
						  : ByteOrder_swapIfLittleEndian32(uval);
			memcpy(d, &uval, 4);
			d += fmt->byte_stride;
		}
		break;
	default:
		break;
	}
}

void asio_format_clear(const asio_sample_format *fmt, void *dst, int samps)
{
	if (dst)
		memset(dst, 0, samps * fmt->byte_stride);
}
