/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "../util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT void compress_uyvx_to_i420(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y, void **output);

EXPORT void compress_uyvx_to_nv12(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y, void **output);

EXPORT void decompress_nv12(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y, void *output);

EXPORT void decompress_420(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y, void *output);

EXPORT void decompress_422(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y, void *output,
		bool leading_lum);

/* special case for quicksync */
EXPORT void compress_uyvx_to_nv12_aligned(const void *input,
		uint32_t width, uint32_t height, uint32_t row_bytes,
		uint32_t start_y, uint32_t end_y,
		uint32_t row_bytes_out, void **output);

#ifdef __cplusplus
}
#endif
