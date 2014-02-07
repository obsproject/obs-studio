/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "format-conversion.h"
#include <xmmintrin.h>
#include <emmintrin.h>

static inline uint32_t get_m128_32_0(const __m128i val)
{
	return *(uint32_t* const)&val;
}

static inline uint32_t get_m128_32_1(const __m128i val)
{
	return *(((uint32_t* const)&val)+1);
}

static inline void pack_lum(uint8_t *lum_plane,
		uint32_t lum_pos0, uint32_t lum_pos1,
		const __m128i line1, const __m128i line2,
		const __m128i lum_mask)
{
	__m128i pack_val = _mm_packs_epi32(
			_mm_srli_si128(_mm_and_si128(line1, lum_mask), 1),
			_mm_srli_si128(_mm_and_si128(line2, lum_mask), 1));
	pack_val = _mm_packus_epi16(pack_val, pack_val);

	*(uint32_t*)(lum_plane+lum_pos0) = get_m128_32_0(pack_val);
	*(uint32_t*)(lum_plane+lum_pos1) = get_m128_32_1(pack_val);
}

static inline void pack_chroma_1plane(uint8_t *uv_plane,
		uint32_t chroma_pos,
		const __m128i line1, const __m128i line2,
		const __m128i uv_mask)
{
	__m128i add_val = _mm_add_epi64(
			_mm_and_si128(line1, uv_mask),
			_mm_and_si128(line2, uv_mask));
	__m128i avg_val = _mm_add_epi64(
			add_val,
			_mm_shuffle_epi32(add_val, _MM_SHUFFLE(2, 3, 0, 1)));
	avg_val = _mm_srai_epi16(avg_val, 2);
	avg_val = _mm_shuffle_epi32(avg_val, _MM_SHUFFLE(3, 1, 2, 0));
	avg_val = _mm_packus_epi16(avg_val, avg_val);

	*(uint32_t*)(uv_plane+chroma_pos) = get_m128_32_0(avg_val);
}

static inline void pack_chroma_2plane(uint8_t *u_plane, uint8_t *v_plane,
		uint32_t chroma_pos,
		const __m128i line1, const __m128i line2,
		const __m128i uv_mask)
{
	uint32_t packed_vals;

	__m128i add_val = _mm_add_epi64(
			_mm_and_si128(line1, uv_mask),
			_mm_and_si128(line2, uv_mask));
	__m128i avg_val = _mm_add_epi64(
			add_val,
			_mm_shuffle_epi32(add_val, _MM_SHUFFLE(2, 3, 0, 1)));
	avg_val = _mm_srai_epi16(avg_val, 2);
	avg_val = _mm_shuffle_epi32(avg_val, _MM_SHUFFLE(3, 1, 2, 0));
	avg_val = _mm_shufflelo_epi16(avg_val, _MM_SHUFFLE(3, 1, 2, 0));
	avg_val = _mm_packus_epi16(avg_val, avg_val);

	packed_vals = get_m128_32_0(avg_val);

	*(uint16_t*)(u_plane+chroma_pos) = (uint16_t)(packed_vals);
	*(uint16_t*)(v_plane+chroma_pos) = (uint16_t)(packed_vals>>16);
}

void compress_uyvx_to_i420(
		const uint8_t *input, uint32_t in_row_bytes,
		uint32_t width, uint32_t height,
		uint32_t start_y, uint32_t end_y,
		uint8_t *output[], const uint32_t out_row_bytes[])
{
	uint8_t  *lum_plane   = output[0];
	uint8_t  *u_plane     = output[1];
	uint8_t  *v_plane     = output[2];
	uint32_t y;

	__m128i lum_mask = _mm_set1_epi32(0x0000FF00);
	__m128i uv_mask  = _mm_set1_epi16(0x00FF);

	for (y = start_y; y < end_y; y += 2) {
		uint32_t y_pos        = y      * in_row_bytes;
		uint32_t chroma_y_pos = (y>>1) * out_row_bytes[1];
		uint32_t lum_y_pos    = y      * out_row_bytes[0];
		uint32_t x;

		for (x = 0; x < width; x += 4) {
			const uint8_t *img = input + y_pos + x*4;
			uint32_t lum_pos0  = lum_y_pos + x;
			uint32_t lum_pos1  = lum_pos0 + out_row_bytes[0];

			__m128i line1 = _mm_load_si128((const __m128i*)img);
			__m128i line2 = _mm_load_si128(
					(const __m128i*)(img + in_row_bytes));

			pack_lum(lum_plane, lum_pos0, lum_pos1,
					line1, line2, lum_mask);
			pack_chroma_2plane(u_plane, v_plane,
					chroma_y_pos + (x>>1),
					line1, line2, uv_mask);
		}
	}
}

void compress_uyvx_to_nv12(
		const uint8_t *input, uint32_t in_row_bytes,
		uint32_t width, uint32_t height,
		uint32_t start_y, uint32_t end_y,
		uint8_t *output[], const uint32_t out_row_bytes[])
{
	uint8_t *lum_plane    = output[0];
	uint8_t *chroma_plane = output[1];
	uint32_t y;

	__m128i lum_mask = _mm_set1_epi32(0x0000FF00);
	__m128i uv_mask  = _mm_set1_epi16(0x00FF);

	for (y = start_y; y < end_y; y += 2) {
		uint32_t y_pos        = y      * in_row_bytes;
		uint32_t chroma_y_pos = (y>>1) * out_row_bytes[1];
		uint32_t lum_y_pos    = y      * out_row_bytes[0];
		uint32_t x;

		for (x = 0; x < width; x += 4) {
			const uint8_t *img = input + y_pos + x*4;
			uint32_t lum_pos0  = lum_y_pos + x;
			uint32_t lum_pos1  = lum_pos0 + out_row_bytes[0];

			__m128i line1 = _mm_load_si128((const __m128i*)img);
			__m128i line2 = _mm_load_si128(
					(const __m128i*)(img + in_row_bytes));

			pack_lum(lum_plane, lum_pos0, lum_pos1,
					line1, line2, lum_mask);
			pack_chroma_1plane(chroma_plane, chroma_y_pos + x,
					line1, line2, uv_mask);
		}
	}
}

void decompress_420(
		const uint8_t *const input[], const uint32_t in_row_bytes[],
		uint32_t width, uint32_t height,
		uint32_t start_y, uint32_t end_y,
		uint8_t *output, uint32_t out_row_bytes)
{
	uint32_t start_y_d2 = start_y/2;
	uint32_t width_d2   = width/2;
	uint32_t height_d2  = end_y/2;
	uint32_t y;

	for (y = start_y_d2; y < height_d2; y++) {
		const uint8_t *chroma0 = input[1] + y * in_row_bytes[1];
		const uint8_t *chroma1 = input[2] + y * in_row_bytes[2];
		register const uint8_t *lum0, *lum1;
		register uint32_t *output0, *output1;
		uint32_t x;

		lum0 = input[0] + y * 2*width;
		lum1 = lum0 + width;
		output0 = (uint32_t*)(output + y * 2 * in_row_bytes[0]);
		output1 = (uint32_t*)((uint8_t*)output0 + in_row_bytes[0]);

		for (x = 0; x < width_d2; x++) {
			uint32_t out;
			out = (*(chroma0++) << 8) | (*(chroma1++) << 16);

			*(output0++) = *(lum0++) | out;
			*(output0++) = *(lum0++) | out;

			*(output1++) = *(lum1++) | out;
			*(output1++) = *(lum1++) | out;
		}
	}
}

void decompress_nv12(
		const uint8_t *const input[], const uint32_t in_row_bytes[],
		uint32_t width, uint32_t height,
		uint32_t start_y, uint32_t end_y,
		uint8_t *output, uint32_t out_row_bytes)
{
	uint32_t start_y_d2 = start_y/2;
	uint32_t width_d2   = width/2;
	uint32_t height_d2  = end_y/2;
	uint32_t y;

	for (y = start_y_d2; y < height_d2; y++) {
		const uint16_t *chroma;
		register const uint8_t *lum0, *lum1;
		register uint32_t *output0, *output1;
		uint32_t x;

		chroma = (const uint16_t*)(input[1] + y * in_row_bytes[1]);
		lum0 = input[0] + y*2 * in_row_bytes[0];
		lum1 = lum0 + in_row_bytes[0];
		output0 = (uint32_t*)(output + y*2 * out_row_bytes);
		output1 = (uint32_t*)((uint8_t*)output0 + out_row_bytes);

		for (x = 0; x < width_d2; x++) {
			uint32_t out = *(chroma++) << 8;

			*(output0++) = *(lum0++) | out;
			*(output0++) = *(lum0++) | out;

			*(output1++) = *(lum1++) | out;
			*(output1++) = *(lum1++) | out;
		}
	}
}

void decompress_422(
		const uint8_t *input, uint32_t in_row_bytes,
		uint32_t width, uint32_t height,
		uint32_t start_y, uint32_t end_y,
		uint8_t *output, uint32_t out_row_bytes,
		bool leading_lum)
{
	uint32_t width_d2 = width >> 1;
	uint32_t y;

	register const uint32_t *input32;
	register const uint32_t *input32_end;
	register uint32_t       *output32;

	if (leading_lum) {
		for (y = start_y; y < end_y; y++) {
			input32     = (const uint32_t*)(input + y*in_row_bytes);
			input32_end = input32 + width_d2;
			output32    = (uint32_t*)(output + y*out_row_bytes);

			while(input32 < input32_end) {
				register uint32_t dw = *input32;

				output32[0] = dw;
				dw &= 0xFFFFFF00;
				dw |= (uint8_t)(dw>>16);
				output32[1] = dw;

				output32 += 2;
				input32++;
			}
		}
	} else {
		for (y = start_y; y < end_y; y++) {
			input32     = (const uint32_t*)(input + y*in_row_bytes);
			input32_end = input32 + width_d2;
			output32    = (uint32_t*)(output + y*out_row_bytes);

			while (input32 < input32_end) {
				register uint32_t dw = *input32;

				output32[0] = dw;
				dw &= 0xFFFF00FF;
				dw |= (dw>>16) & 0xFF00;
				output32[1] = dw;

				output32 += 2;
				input32++;
			}
		}
	}
}
