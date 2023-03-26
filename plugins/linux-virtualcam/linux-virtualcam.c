/*
Copyright (C) 2022 DEV47APPS, github.com/dev47apps

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
#include <obs-module.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <unistd.h>

#include "linux-virtualcam.h"

struct virtualcam_data {
	obs_output_t *output;
	void *audio_data;
	void *video_data;
};

static bool is_flatpak_sandbox(void)
{
	return access("/.flatpak-info", F_OK) == 0;
}

int run_command(const char *command)
{
	struct dstr str;
	int result;

	dstr_init_copy(&str, "PATH=\"$PATH:/sbin\" ");

	if (is_flatpak_sandbox())
		dstr_cat(&str, "flatpak-spawn --host ");

	dstr_cat(&str, command);
	result = system(str.array);
	dstr_free(&str);
	return result;
}

bool module_loaded(const char *module)
{
	bool loaded = false;
	char temp[512];

	FILE *fp = fopen("/proc/modules", "r");
	if (!fp)
		return false;

	while (fgets(temp, sizeof(temp), fp)) {
		if (strstr(temp, module)) {
			loaded = true;
			break;
		}
	}

	fclose(fp);

	return loaded;
}

static void *virtualcam_create(obs_data_t *settings, obs_output_t *output)
{
	struct virtualcam_data *vcam =
		(struct virtualcam_data *)bzalloc(sizeof(*vcam));
	vcam->output = output;

	UNUSED_PARAMETER(settings);
	return vcam;
}

static void virtualcam_destroy(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	if (vcam->audio_data)
		audio_stop(vcam->audio_data);

	if (vcam->video_data)
		video_stop(vcam->video_data);

	bfree(data);
}

static bool virtualcam_start(void *data)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;

	vcam->video_data = video_start(vcam->output);
	vcam->audio_data = audio_start(vcam->output);

	if (vcam->video_data || vcam->audio_data) {
		blog(LOG_INFO, "Virtual camera starting: audio=%d video=%d",
		     (vcam->audio_data != NULL), (vcam->video_data != NULL));

		obs_output_begin_data_capture(vcam->output, 0);
		return true;
	}

	return false;
}

static void virtualcam_stop(void *data, uint64_t ts)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	obs_output_end_data_capture(vcam->output);

	if (vcam->audio_data) {
		audio_stop(vcam->audio_data);
		vcam->audio_data = NULL;
	}

	if (vcam->video_data) {
		video_stop(vcam->video_data);
		vcam->video_data = NULL;
	}

	blog(LOG_INFO, "Virtual camera stopped");

	UNUSED_PARAMETER(ts);
}

static const char *virtualcam_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Virtual Camera Output";
}

extern void virtual_video(void *data, struct video_data *frame);
extern void virtual_audio(void *data, struct audio_data *frame);

void raw_audio(void *data, struct audio_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	if (vcam->audio_data) {
		virtual_audio(vcam->audio_data, frame);
	}
}

void raw_video(void *data, struct video_data *frame)
{
	struct virtualcam_data *vcam = (struct virtualcam_data *)data;
	if (vcam->video_data) {
		virtual_video(vcam->video_data, frame);
	}
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("linux-virtualcam", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Linux virtual audio/video output";
}

struct obs_output_info virtualcam_info = {
	.id = "virtualcam_output",
	.flags = OBS_OUTPUT_AV,
	.get_name = virtualcam_name,
	.create = virtualcam_create,
	.destroy = virtualcam_destroy,
	.start = virtualcam_start,
	.stop = virtualcam_stop,
	.raw_audio = raw_audio,
	.raw_video = raw_video,
};

bool obs_module_load(void)
{
	obs_data_t *obs_settings = obs_data_create();

	if (video_possible()) {
		obs_register_output(&virtualcam_info);
		obs_data_set_bool(obs_settings, "vcamEnabled", true);
	} else {
		obs_data_set_bool(obs_settings, "vcamEnabled", false);
		blog(LOG_WARNING,
		     "v4l2loopback not installed, virtual camera disabled");
	}

	obs_apply_private_data(obs_settings);
	obs_data_release(obs_settings);

	return true;
}
