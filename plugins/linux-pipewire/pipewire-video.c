/*
 * pipewire-video.c
 *
 * Copyright 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 * Copyright 2024 columbarius <co1umbarius@protonmail.com>
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

#include "pipewire-video.h"
#include "formats.h"

#include <libdrm/drm_fourcc.h>
#include <pipewire/pipewire.h>

#include <obs-module.h>

/************** duplicated from pipewire.c ***************************/
struct obs_pw_version {
	int major;
	int minor;
	int micro;
};

static bool check_pw_version(const struct obs_pw_version *pw_version, int major,
			     int minor, int micro)
{
	if (pw_version->major != major)
		return pw_version->major > major;
	if (pw_version->minor != minor)
		return pw_version->minor > minor;
	return pw_version->micro >= micro;
}
/************** end duplicated from pipewire.c ***************************/

static bool drm_format_available(uint32_t drm_format, uint32_t *drm_formats,
				 size_t n_drm_formats)
{
	for (size_t j = 0; j < n_drm_formats; j++) {
		if (drm_format == drm_formats[j]) {
			return true;
		}
	}
	return false;
}

void init_format_info_with_formats(struct format_info_base *base,
		const uint32_t *formats, uint32_t n_formats, bool query_modifiers)
{
	da_init(base->formats);

	if (query_modifiers)
		obs_enter_graphics();

	enum gs_dmabuf_flags dmabuf_flags;
	uint32_t *drm_formats = NULL;
	size_t n_drm_formats = 0;

	bool capabilities_queried = query_modifiers
		? gs_query_dmabuf_capabilities(&dmabuf_flags, &drm_formats, &n_drm_formats)
		: false;

	for (size_t i = 0; i < n_formats; i++) {
		struct obs_pw_video_format obs_pw_video_format;
		struct format_info *info;
		if (!obs_pw_video_format_from_spa_format(
			    formats[i], &obs_pw_video_format) ||
		    obs_pw_video_format.gs_format == GS_UNKNOWN)
			continue;

		info = da_push_back_new(base->formats);
		da_init(info->modifiers);
		info->spa_format = obs_pw_video_format.spa_format;
		info->drm_format = obs_pw_video_format.drm_format;

		if (!capabilities_queried ||
		    !drm_format_available(obs_pw_video_format.drm_format,
					  drm_formats, n_drm_formats))
			continue;

		size_t n_modifiers;
		uint64_t *modifiers = NULL;
		if (gs_query_dmabuf_modifiers_for_format(
			    obs_pw_video_format.drm_format, &modifiers,
			    &n_modifiers)) {
			da_push_back_array(info->modifiers, modifiers,
					   n_modifiers);
		}
		bfree(modifiers);

		if (dmabuf_flags &
		    GS_DMABUF_FLAG_IMPLICIT_MODIFIERS_SUPPORTED) {
			uint64_t modifier_implicit = DRM_FORMAT_MOD_INVALID;
			da_push_back(info->modifiers, &modifier_implicit);
		}
	}
	if (query_modifiers)
		obs_leave_graphics();

	bfree(drm_formats);
}

void clear_format_info(struct format_info_base *base)
{
	for (size_t i = 0; i < base->formats.num; i++) {
		da_free(base->formats.array[i].modifiers);
	}
	da_free(base->formats);
}

void remove_modifier_from_format(struct format_info_base *base, struct obs_pw_version *server_version,
					uint32_t spa_format, uint64_t modifier)
{
	for (size_t i = 0; i < base->formats.num; i++) {
		if (base->formats.array[i].spa_format !=
		    spa_format)
			continue;

		if (!check_pw_version(server_version, 0, 3, 40)) {
			da_erase_range(
				base->formats.array[i].modifiers,
				0,
				base->formats.array[i]
						.modifiers.num -
					1);
			continue;
		}

		int idx = da_find(base->formats.array[i].modifiers,
				  &modifier, 0);
		while (idx != -1) {
			da_erase(base->formats.array[i].modifiers,
				 idx);
			idx = da_find(
				base->formats.array[i].modifiers,
				&modifier, 0);
		}
	}
}

