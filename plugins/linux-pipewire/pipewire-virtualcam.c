/* pipewire-virtualcam.c
 *
 * Copyright 2021 columbarius <co1umbarius@protonmail.com>
 *
 * This code is heavily inspired by the pipewire-capture in the
 * linux-capture plugin done by
 * Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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

#include <pipewire/pipewire.h>
#include <spa/debug/format.h>

#include <obs-module.h>
#include <obs-output.h>

#include "pipewire.h"

struct obs_pipewire_virtualcam_data {
	obs_output_t *output;

	obs_pipewire *obs_pw;
	obs_pipewire_stream *obs_pw_stream;
};

/****************************************************************************************/

static const char *virtualcam_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("PipeWireVirtualCamera");
}

static void virtualcam_destroy(void *data)
{
	blog(LOG_INFO, "Virtual camera destroyed");
	struct obs_pipewire_virtualcam_data *obs_pwvc =
		(struct obs_pipewire_virtualcam_data *)data;
	bfree(obs_pwvc);
}

static void *virtualcam_create(obs_data_t *settings, obs_output_t *output)
{
	blog(LOG_INFO, "Virtual camera created");

	struct obs_pipewire_virtualcam_data *obs_pwvc =
		(struct obs_pipewire_virtualcam_data *)bzalloc(
			sizeof(struct obs_pipewire_virtualcam_data));
	obs_pwvc->output = output;
	UNUSED_PARAMETER(settings);
	return obs_pwvc;
}

static bool virtualcam_start(void *data)
{
	blog(LOG_INFO, "Virtual camera started");
	struct obs_pipewire_virtualcam_data *obs_pwvc =
		(struct obs_pipewire_virtualcam_data *)data;

	obs_pwvc->obs_pw = obs_pipewire_create(-1, NULL, obs_pwvc);

	obs_pipewire_stream_data stream_data;
	stream_data.type = OBS_PIPEWIRE_STREAM_TYPE_OUTPUT;
	stream_data.output = obs_pwvc->output;
	obs_pwvc->obs_pw_stream = obs_pipewire_connect_stream(
		obs_pwvc->obs_pw, &stream_data, PW_ID_ANY, "OBS Virtual Camera",
		pw_properties_new(PW_KEY_NODE_DESCRIPTION, "OBS Virtual Camera",
				  PW_KEY_MEDIA_CLASS, "Video/Source",
				  PW_KEY_MEDIA_ROLE, "Camera", NULL));

	obs_output_begin_data_capture(obs_pwvc->output, OBS_OUTPUT_VIDEO);
	return true;
}

static void virtualcam_stop(void *data, uint64_t ts)
{
	UNUSED_PARAMETER(ts);
	blog(LOG_INFO, "Virtual camera stopped");
	struct obs_pipewire_virtualcam_data *obs_pwvc =
		(struct obs_pipewire_virtualcam_data *)data;

	obs_output_end_data_capture(obs_pwvc->output);
	// stop pipewire stuff
	obs_pipewire_stream_destroy(obs_pwvc->obs_pw_stream);
	obs_pipewire_destroy(obs_pwvc->obs_pw);
}

static void virtual_video(void *data, struct video_data *frame)
{
	struct obs_pipewire_virtualcam_data *obs_pwvc =
		(struct obs_pipewire_virtualcam_data *)data;

	obs_pipewire_stream_export_frame(obs_pwvc->obs_pw_stream, frame);
}

void virtual_cam_register_output(void)
{
	struct obs_output_info pipewire_virtualcam_info = {
		.id = "pw_vcam_output",
		.flags = OBS_OUTPUT_VIDEO | OBS_OUTPUT_VIRTUALCAM,
		.get_name = virtualcam_name,
		.create = virtualcam_create,
		.destroy = virtualcam_destroy,
		.start = virtualcam_start,
		.stop = virtualcam_stop,
		.raw_video = virtual_video,
	};

	obs_register_output(&pipewire_virtualcam_info);
}
