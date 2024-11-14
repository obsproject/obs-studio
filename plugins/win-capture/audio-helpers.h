#pragma once

#include "obs-module.h"
#include <util/windows/window-helpers.h>

#include "windows.h"

#define SETTING_CAPTURE_AUDIO "capture_audio"
#define TEXT_CAPTURE_AUDIO obs_module_text("CaptureAudio")
#define TEXT_CAPTURE_AUDIO_TT obs_module_text("CaptureAudio.TT")
#define TEXT_CAPTURE_AUDIO_SUFFIX obs_module_text("AudioSuffix")
#define AUDIO_SOURCE_TYPE "wasapi_process_output_capture"

void setup_audio_source(obs_source_t *parent, obs_source_t **child,
			const char *window, bool enabled,
			enum window_priority priority);
void reconfigure_audio_source(obs_source_t *source, HWND window);
void rename_audio_source(void *param, calldata_t *data);

static bool audio_capture_available(void)
{
	return obs_get_latest_input_type_id(AUDIO_SOURCE_TYPE) != NULL;
}

static void destroy_audio_source(obs_source_t *parent, obs_source_t **child)
{
	setup_audio_source(parent, child, NULL, false, 0);
}
