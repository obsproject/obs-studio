/*
 * formats.h
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <obs-module.h>

#include <spa/param/video/format-utils.h>

struct obs_pw_video_format {
	uint32_t spa_format;
	uint32_t drm_format;
	enum gs_color_format gs_format;
	enum video_format video_format;
	bool swap_red_blue;
	uint32_t bpp;
	const char *pretty_name;
};

bool obs_pw_video_format_from_spa_format(
	uint32_t spa_format, struct obs_pw_video_format *out_format_info);
