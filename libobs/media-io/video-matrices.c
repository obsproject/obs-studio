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

#include "../util/bmem.h"
#include "video-io.h"

//#define COMPUTE_MATRICES

#ifdef COMPUTE_MATRICES
#include "../graphics/matrix3.h"
#endif

static struct {
	enum video_colorspace const color_space;
	float const Kb, Kr;
	int const range_min[3];
	int const range_max[3];
	int const black_levels[2][3];

	float float_range_min[3];
	float float_range_max[3];
	float matrix[2][16];

} format_info[] = {
	{VIDEO_CS_601,
	 0.114f,
	 0.299f,
	 {16, 16, 16},
	 {235, 240, 240},
	 {{16, 128, 128}, {0, 128, 128}},
#ifndef COMPUTE_MATRICES
	 {16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f},
	 {235.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f},
	 {{1.164384f, 0.000000f, 1.596027f, -0.874202f, 1.164384f, -0.391762f,
	   -0.812968f, 0.531668f, 1.164384f, 2.017232f, 0.000000f, -1.085631f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f},
	  {1.000000f, 0.000000f, 1.407520f, -0.706520f, 1.000000f, -0.345491f,
	   -0.716948f, 0.533303f, 1.000000f, 1.778976f, 0.000000f, -0.892976f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f}}
#endif
	},
	{VIDEO_CS_709,
	 0.0722f,
	 0.2126f,
	 {16, 16, 16},
	 {235, 240, 240},
	 {{16, 128, 128}, {0, 128, 128}},
#ifndef COMPUTE_MATRICES
	 {16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f},
	 {235.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f},
	 {{1.164384f, 0.000000f, 1.792741f, -0.972945f, 1.164384f, -0.213249f,
	   -0.532909f, 0.301483f, 1.164384f, 2.112402f, 0.000000f, -1.133402f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f},
	  {1.000000f, 0.000000f, 1.581000f, -0.793600f, 1.000000f, -0.188062f,
	   -0.469967f, 0.330305f, 1.000000f, 1.862906f, 0.000000f, -0.935106f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f}}
#endif
	},
	{VIDEO_CS_2020_PQ,
	 0.0593f,
	 0.2627f,
	 {64, 64, 64},
	 {940, 960, 960},
	 {{64, 512, 512}, {0, 512, 512}},
#ifndef COMPUTE_MATRICES
	 {64.0f / 1023.0f, 64.0f / 1023.0f, 64.0f / 1023.0f},
	 {940.0f / 1023.0f, 960.0f / 1023.0f, 960.0f / 1023.0f},
	 {{1.167808f, 0.000000f, 1.683611f, -0.915688f, 1.167808f, -0.187877f,
	   -0.652337f, 0.347459f, 1.167808f, 2.148072f, 0.000000f, -1.148145f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f},
	  {1.000000f, 0.000000f, 1.476043f, -0.738743f, 1.000000f, -0.164714f,
	   -0.571912f, 0.368673f, 1.000000f, 1.883241f, 0.000000f, -0.942541f,
	   0.000000f, 0.000000f, 0.000000f, 1.000000f}}
#endif
	},
};

#define NUM_FORMATS (sizeof(format_info) / sizeof(format_info[0]))

#ifdef COMPUTE_MATRICES
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

static void initialize_matrix(float const Kb, float const Kr,
			      float bit_range_max, int const range_min[3],
			      int const range_max[3], int const black_levels[3],
			      float matrix[16])
{
	struct matrix3 color_matrix;

	int yvals = range_max[0] - range_min[0];
	int uvals = (range_max[1] - range_min[1]) / 2;
	int vvals = (range_max[2] - range_min[2]) / 2;

	vec3_set(&color_matrix.x, bit_range_max / yvals, 0.,
		 bit_range_max / vvals * (1.f - Kr));
	vec3_set(&color_matrix.y, bit_range_max / yvals,
		 bit_range_max / uvals * (Kb - 1.f) * Kb / (1.f - Kb - Kr),
		 bit_range_max / vvals * (Kr - 1.f) * Kr / (1.f - Kb - Kr));
	vec3_set(&color_matrix.z, bit_range_max / yvals,
		 bit_range_max / uvals * (1.f - Kb), 0.);

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

	log_matrix(matrix);
}

static void initialize_matrices()
{
	static const int range_min[] = {0, 0, 0};
	static const int range_max_8bit[] = {255, 255, 255};
	static const int range_max_10bit[] = {1023, 1023, 1023};

	for (size_t i = 0; i < NUM_FORMATS; i++) {
		const int *range_max =
			(format_info[i].color_space == VIDEO_CS_2020_PQ)
				? range_max_10bit
				: range_max_8bit;
		float f_r_max = (float)range_max[0];
		initialize_matrix(format_info[i].Kb, format_info[i].Kr, f_r_max,
				  range_min, range_max,
				  format_info[i].black_levels[1],
				  format_info[i].matrix[1]);

		initialize_matrix(format_info[i].Kb, format_info[i].Kr, f_r_max,
				  format_info[i].range_min,
				  format_info[i].range_max,
				  format_info[i].black_levels[0],
				  format_info[i].matrix[0]);

		for (int j = 0; j < 3; j++) {
			format_info[i].float_range_min[j] =
				format_info[i].range_min[j] / f_r_max;
			format_info[i].float_range_max[j] =
				format_info[i].range_max[j] / f_r_max;
		}
	}
}

static bool matrices_initialized = false;
#endif

static const float full_min[3] = {0.0f, 0.0f, 0.0f};
static const float full_max[3] = {1.0f, 1.0f, 1.0f};

bool video_format_get_parameters(enum video_colorspace color_space,
				 enum video_range_type range, float matrix[16],
				 float range_min[3], float range_max[3])
{
#ifdef COMPUTE_MATRICES
	if (!matrices_initialized) {
		initialize_matrices();
		matrices_initialized = true;
	}
#endif
	if ((color_space == VIDEO_CS_DEFAULT) || (color_space == VIDEO_CS_SRGB))
		color_space = VIDEO_CS_709;
	else if (color_space == VIDEO_CS_2020_HLG)
		color_space = VIDEO_CS_2020_PQ;

	for (size_t i = 0; i < NUM_FORMATS; i++) {
		if (format_info[i].color_space != color_space)
			continue;

		int full_range = range == VIDEO_RANGE_FULL ? 1 : 0;
		memcpy(matrix, format_info[i].matrix[full_range],
		       sizeof(float) * 16);

		if (range == VIDEO_RANGE_FULL) {
			if (range_min)
				memcpy(range_min, full_min, sizeof(float) * 3);
			if (range_max)
				memcpy(range_max, full_max, sizeof(float) * 3);
			return true;
		}

		if (range_min)
			memcpy(range_min, format_info[i].float_range_min,
			       sizeof(float) * 3);

		if (range_max)
			memcpy(range_max, format_info[i].float_range_max,
			       sizeof(float) * 3);

		return true;
	}
	return false;
}
