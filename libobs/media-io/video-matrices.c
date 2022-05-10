/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include "video-io.h"

#include "../util/bmem.h"
#include "../graphics/matrix3.h"

#include <assert.h>

//#define LOG_MATRICES

static struct {
	float range_min[3];
	float range_max[3];
	float black_levels[2][3];

	float float_range_min[3];
	float float_range_max[3];
} bpp_info[9];

static struct {
	enum video_colorspace const color_space;
	float const Kb, Kr;
	float matrix[OBS_COUNTOF(bpp_info)][2][16];
} format_info[] = {
	{
		VIDEO_CS_601,
		0.114f,
		0.299f,
	},
	{
		VIDEO_CS_709,
		0.0722f,
		0.2126f,
	},
	{
		VIDEO_CS_2100_PQ,
		0.0593f,
		0.2627f,
	},
};

#define NUM_FORMATS (sizeof(format_info) / sizeof(format_info[0]))

#ifdef LOG_MATRICES
static void log_matrix(float const matrix[16])
{
	blog(LOG_DEBUG,
	     "\n% f, % f, % f, % f"
	     "\n% f, % f, % f, % f"
	     "\n% f, % f, % f, % f"
	     "\n% f, % f, % f, % f",
	     matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5],
	     matrix[6], matrix[7], matrix[8], matrix[9], matrix[10], matrix[11],
	     matrix[12], matrix[13], matrix[14], matrix[15]);
}
#endif

static void initialize_matrix(float const Kb, float const Kr,
			      float bit_range_max, float const range_min[3],
			      float const range_max[3],
			      float const black_levels[3], float matrix[16])
{
	struct matrix3 color_matrix;

	float const yvals = range_max[0] - range_min[0];
	float const uvals = (range_max[1] - range_min[1]) / 2.f;
	float const vvals = (range_max[2] - range_min[2]) / 2.f;

	float const yscale = bit_range_max / yvals;
	float const uscale = bit_range_max / uvals;
	float const vscale = bit_range_max / vvals;

	float const Kg = (1.f - Kb - Kr);

	vec3_set(&color_matrix.x, yscale, 0.f, vscale * (1.f - Kr));
	vec3_set(&color_matrix.y, yscale, uscale * (Kb - 1.f) * Kb / Kg,
		 vscale * (Kr - 1.f) * Kr / Kg);
	vec3_set(&color_matrix.z, yscale, uscale * (1.f - Kb), 0.f);

	struct vec3 offsets, multiplied;
	vec3_set(&offsets, -black_levels[0] / bit_range_max,
		 -black_levels[1] / bit_range_max,
		 -black_levels[2] / bit_range_max);
	vec3_rotate(&multiplied, &offsets, &color_matrix);

	matrix[0] = color_matrix.x.x;
	matrix[1] = color_matrix.x.y;
	matrix[2] = color_matrix.x.z;
	matrix[3] = multiplied.x;

	matrix[4] = color_matrix.y.x;
	matrix[5] = color_matrix.y.y;
	matrix[6] = color_matrix.y.z;
	matrix[7] = multiplied.y;

	matrix[8] = color_matrix.z.x;
	matrix[9] = color_matrix.z.y;
	matrix[10] = color_matrix.z.z;
	matrix[11] = multiplied.z;

	matrix[12] = matrix[13] = matrix[14] = 0.;
	matrix[15] = 1.;

#ifdef LOG_MATRICES
	log_matrix(matrix);
#endif
}

static void initialize_matrices()
{
	static const float full_range_min3[] = {0, 0, 0};

	float min_value = 16.f;
	float max_luma = 235.f;
	float max_chroma = 240.f;
	float range = 256.f;
	for (uint32_t bpp = 8; bpp <= 16; ++bpp) {
		const uint32_t bpp_index = bpp - 8;
		bpp_info[bpp_index].range_min[0] = min_value;
		bpp_info[bpp_index].range_min[1] = min_value;
		bpp_info[bpp_index].range_min[2] = min_value;
		bpp_info[bpp_index].range_max[0] = max_luma;
		bpp_info[bpp_index].range_max[1] = max_chroma;
		bpp_info[bpp_index].range_max[2] = max_chroma;
		const float mid_chroma = 0.5f * (min_value + max_chroma);
		bpp_info[bpp_index].black_levels[0][0] = min_value;
		bpp_info[bpp_index].black_levels[0][1] = mid_chroma;
		bpp_info[bpp_index].black_levels[0][2] = mid_chroma;
		bpp_info[bpp_index].black_levels[1][0] = 0.f;
		bpp_info[bpp_index].black_levels[1][1] = mid_chroma;
		bpp_info[bpp_index].black_levels[1][2] = mid_chroma;
		const float range_max = range - 1.f;
		bpp_info[bpp_index].float_range_min[0] = min_value / range_max;
		bpp_info[bpp_index].float_range_min[1] = min_value / range_max;
		bpp_info[bpp_index].float_range_min[2] = min_value / range_max;
		bpp_info[bpp_index].float_range_max[0] = max_luma / range_max;
		bpp_info[bpp_index].float_range_max[1] = max_chroma / range_max;
		bpp_info[bpp_index].float_range_max[2] = max_chroma / range_max;

		for (size_t i = 0; i < NUM_FORMATS; i++) {
			float full_range_max3[] = {range_max, range_max,
						   range_max};
			initialize_matrix(format_info[i].Kb, format_info[i].Kr,
					  range_max, full_range_min3,
					  full_range_max3,
					  bpp_info[bpp_index].black_levels[1],
					  format_info[i].matrix[bpp_index][1]);

			initialize_matrix(format_info[i].Kb, format_info[i].Kr,
					  range_max,
					  bpp_info[bpp_index].range_min,
					  bpp_info[bpp_index].range_max,
					  bpp_info[bpp_index].black_levels[0],
					  format_info[i].matrix[bpp_index][0]);
		}

		min_value *= 2.f;
		max_luma *= 2.f;
		max_chroma *= 2.f;
		range *= 2.f;
	}
}

static bool matrices_initialized = false;

static const float full_min[3] = {0.0f, 0.0f, 0.0f};
static const float full_max[3] = {1.0f, 1.0f, 1.0f};

static bool video_format_get_parameters_for_bpc(
	enum video_colorspace color_space, enum video_range_type range,
	float matrix[16], float range_min[3], float range_max[3], uint32_t bpc)
{
	if (!matrices_initialized) {
		initialize_matrices();
		matrices_initialized = true;
	}

	if ((color_space == VIDEO_CS_DEFAULT) || (color_space == VIDEO_CS_SRGB))
		color_space = VIDEO_CS_709;
	else if (color_space == VIDEO_CS_2100_HLG)
		color_space = VIDEO_CS_2100_PQ;

	if (bpc < 8)
		bpc = 8;
	if (bpc > 16)
		bpc = 16;
	const uint32_t bpc_index = bpc - 8;
	assert(bpc_index < OBS_COUNTOF(bpp_info));

	bool success = false;

	for (size_t i = 0; i < NUM_FORMATS; i++) {
		success = format_info[i].color_space == color_space;
		if (success) {
			const bool full_range = range == VIDEO_RANGE_FULL;
			memcpy(matrix,
			       format_info[i].matrix[bpc_index][full_range],
			       sizeof(float) * 16);

			if (range_min) {
				const float *src_range_min =
					full_range ? full_min
						   : bpp_info[bpc_index]
							     .float_range_min;
				memcpy(range_min, src_range_min,
				       sizeof(float) * 3);
			}

			if (range_max) {
				const float *src_range_max =
					full_range ? full_max
						   : bpp_info[bpc_index]
							     .float_range_max;
				memcpy(range_max, src_range_max,
				       sizeof(float) * 3);
			}

			break;
		}
	}

	return success;
}

bool video_format_get_parameters(enum video_colorspace color_space,
				 enum video_range_type range, float matrix[16],
				 float range_min[3], float range_max[3])
{
	uint32_t bpc = 8;
	switch (color_space) {
	case VIDEO_CS_2100_PQ:
	case VIDEO_CS_2100_HLG:
		bpc = 10;
	}

	return video_format_get_parameters_for_bpc(color_space, range, matrix,
						   range_min, range_max, bpc);
}

bool video_format_get_parameters_for_format(enum video_colorspace color_space,
					    enum video_range_type range,
					    enum video_format format,
					    float matrix[16],
					    float range_min[3],
					    float range_max[3])
{
	uint32_t bpc = 8;
	switch (format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
	case VIDEO_FORMAT_I210:
		bpc = 10;
		break;
	case VIDEO_FORMAT_I412:
	case VIDEO_FORMAT_YA2L:
		bpc = 12;
		break;
	}

	return video_format_get_parameters_for_bpc(color_space, range, matrix,
						   range_min, range_max, bpc);
}
