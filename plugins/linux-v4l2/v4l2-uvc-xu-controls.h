/*
Copyright (C) 2022 by Grzegorz Godlewski

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
*/

#pragma once

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <obs-module.h>
#include <media-io/video-io.h>

#ifdef __cplusplus
extern "C" {
#endif

struct menu_item_pos {
	const char *label;
	int queries_cnt;
	struct uvc_xu_control_query *queries;
};

struct menu_item {
	const char *id;
	const char *label;
	int positions_cnt;
	struct menu_item_pos *positions;
};

/**
 * Update devices properties to include Extension Unit controls.
 *
 * @param device_path path device within /dev
 * @param props properties for the device
 * @param settings the settings for the source
 *
 * @return negative on failure
 */
int_fast32_t v4l2_update_uvc_xu_controls(const char *device_path, obs_properties_t *props);

#ifdef __cplusplus
}
#endif
