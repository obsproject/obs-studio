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

struct {
	enum video_colorspace const color_space;
	float const Kb, Kr;
	int const range_min[3];
	int const range_max[3];
	int const black_levels[2][3];

	float float_range_min[3];
	float float_range_max[3];
	float matrix[2][16];
} static format_info[] = {
	{VIDEO_CS_601,
		0.114, 0.299, {16, 16, 16}, {235, 240, 240},
		{{16, 128, 128}, {0, 128, 128}},
#ifndef COMPUTE_MATRICES
		{ 16/255.,  16/255.,  16/255.},
		{235/255., 240/255., 240/255.},
		{
			{
				 1.164384,  0.000000,  1.596027, -0.874202,
				 1.164384, -0.391762, -0.812968,  0.531668,
				 1.164384,  2.017232,  0.000000, -1.085631,
				 0.000000,  0.000000,  0.000000,  1.000000
			},
			{
				 1.000000,  0.000000,  1.407520, -0.706520,
				 1.000000, -0.345491, -0.716948,  0.533303,
				 1.000000,  1.778976,  0.000000, -0.892976,
				 0.000000,  0.000000,  0.000000,  1.000000
			}
		}
#endif
	},
	{VIDEO_CS_709,
		0.0722, 0.2126, {16, 16, 16}, {235, 240, 240},
		{{16, 128, 128}, {0, 128, 128}},
#ifndef COMPUTE_MATRICES
		{ 16/255.,  16/255.,  16/255.},
		{235/255., 240/255., 240/255.},
		{
			{
				 1.164384,  0.000000,  1.792741, -0.972945,
				 1.164384, -0.213249, -0.532909,  0.301483,
				 1.164384,  2.112402,  0.000000, -1.133402,
				 0.000000,  0.000000,  0.000000,  1.000000
			},
			{
				 1.000000,  0.000000,  1.581000, -0.793600,
				 1.000000, -0.188062, -0.469967,  0.330305,
				 1.000000,  1.862906,  0.000000, -0.935106,
				 0.000000,  0.000000,  0.000000,  1.000000
			}
		}
#endif
	},
};

#define NUM_FORMATS (sizeof(format_info)/sizeof(format_info[0]))

#ifdef COMPUTE_MATRICES
static void log_matrix(float const matrix[16])
{
	blog(LOG_DEBUG, "\n% f, % f, % f, % f" \
			"\n% f, % f, % f, % f" \
			"\n% f, % f, % f, % f" \
			"\n% f, % f, % f, % f",
			matrix[ 0], matrix[ 1], matrix[ 2], matrix[ 3],
			matrix[ 4], matrix[ 5], matrix[ 6], matrix[ 7],
			matrix[ 8], matrix[ 9], matrix[10], matrix[11],
			matrix[12], matrix[13], matrix[14], matrix[15]);
}

static void initialize_matrix(float const Kb, float const Kr,
		int const range_min[3], int const range_max[3],
		int const black_levels[3], float matrix[16])
{
	struct matrix3 color_matrix;

	int yvals = range_max[0] - range_min[0];
	int uvals = (range_max[1] - range_min[1]) / 2;
	int vvals = (range_max[2] - range_min[2]) / 2;

	vec3_set(&color_matrix.x, 255./yvals,
			0.,
			255./vvals * (1. - Kr));
	vec3_set(&color_matrix.y, 255./yvals,
			255./uvals * (Kb - 1.) * Kb / (1. - Kb - Kr),
			255./vvals * (Kr - 1.) * Kr / (1. - Kb - Kr));
	vec3_set(&color_matrix.z, 255./yvals,
			255./uvals * (1. - Kb),
			0.);

	struct vec3 offsets, multiplied;
	vec3_set(&offsets,
			-black_levels[0]/255.,
			-black_levels[1]/255.,
			-black_levels[2]/255.);
	vec3_rotate(&multiplied, &offsets, &color_matrix);

	matrix[ 0] = color_matrix.x.x;
	matrix[ 1] = color_matrix.x.y;
	matrix[ 2] = color_matrix.x.z;
	matrix[ 3] = multiplied.x;

	matrix[ 4] = color_matrix.y.x;
	matrix[ 5] = color_matrix.y.y;
	matrix[ 6] = color_matrix.y.z;
	matrix[ 7] = multiplied.y;

	matrix[ 8] = color_matrix.z.x;
	matrix[ 9] = color_matrix.z.y;
	matrix[10] = color_matrix.z.z;
	matrix[11] = multiplied.z;

	matrix[12] = matrix[13] = matrix[14] = 0.;
	matrix[15] = 1.;

	log_matrix(matrix);
}

static void initialize_matrices()
{
	static int range_min[] = {  0,   0,   0};
	static int range_max[] = {255, 255, 255};

	for (size_t i = 0; i < NUM_FORMATS; i++) {
		initialize_matrix(format_info[i].Kb, format_info[i].Kr,
				range_min, range_max,
				format_info[i].black_levels[1],
				format_info[i].matrix[1]);

		initialize_matrix(format_info[i].Kb, format_info[i].Kr,
				format_info[i].range_min,
				format_info[i].range_max,
				format_info[i].black_levels[0],
				format_info[i].matrix[0]);

		for (int j = 0; j < 3; j++) {
			format_info[i].float_range_min[j] =
				format_info[i].range_min[j]/255.;
			format_info[i].float_range_max[j] =
				format_info[i].range_max[j]/255.;
		}
	}
}

static bool matrices_initialized = false;
#endif

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

	for (size_t i = 0; i < NUM_FORMATS; i++) {
		if (format_info[i].color_space != color_space)
			continue;

		int full_range = range == VIDEO_RANGE_FULL ? 1 : 0;
		memcpy(matrix, format_info[i].matrix[full_range],
				sizeof(float) * 16);

		if (range == VIDEO_RANGE_FULL)
			return true;

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
