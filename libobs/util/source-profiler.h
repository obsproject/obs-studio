/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

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

#include "obs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct profiler_result {
	/* Tick times in ns */
	uint64_t tick_avg;
	uint64_t tick_max;

	/* Average and max render times for CPU and GPU in ns */
	uint64_t render_avg;
	uint64_t render_max;
	uint64_t render_gpu_avg;
	uint64_t render_gpu_max;

	/* Average of the sum of all render passes in a frame in ns
	 * (a source can be rendered more than once per frame). */
	uint64_t render_sum;
	uint64_t render_gpu_sum;

	/* FPS of submitted async input */
	double async_input;
	/* Actually rendered async frames */
	double async_rendered;

	/* Best and worst frame times of input/output in ns */
	uint64_t async_input_best;
	uint64_t async_input_worst;
	uint64_t async_rendered_best;
	uint64_t async_rendered_worst;
} profiler_result_t;

/* Enable/disable profiler (applied on next frame) */
EXPORT void source_profiler_enable(bool enable);
/* Enable/disable GPU profiling (applied on next frame) */
EXPORT void source_profiler_gpu_enable(bool enable);

/* Get latest profiling results for source (must be freed by user) */
EXPORT profiler_result_t *source_profiler_get_result(obs_source_t *source);
/* Update existing profiler results object for source */
EXPORT bool source_profiler_fill_result(obs_source_t *source,
					profiler_result_t *result);

#ifdef __cplusplus
}
#endif
