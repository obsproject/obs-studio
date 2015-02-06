#define _CRT_SECURE_NO_WARNINGS
#include <obs-module.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/config-file.h>
#include <util/pipe.h>

#include <windows.h>
#include "graphics-hook-info.h"

extern struct graphics_offsets offsets32;
extern struct graphics_offsets offsets64;

static inline bool load_offsets_from_string(struct graphics_offsets *offsets,
		const char *str)
{
	config_t *config;

	if (config_open_string(&config, str) != CONFIG_SUCCESS) {
		return false;
	}

	offsets->d3d8.present =
		(uint32_t)config_get_uint(config, "d3d8", "present");
	offsets->d3d8.reset =
		(uint32_t)config_get_uint(config, "d3d8", "reset");

	offsets->d3d9.present =
		(uint32_t)config_get_uint(config, "d3d9", "present");
	offsets->d3d9.present_ex =
		(uint32_t)config_get_uint(config, "d3d9", "present_ex");
	offsets->d3d9.present_swap =
		(uint32_t)config_get_uint(config, "d3d9", "present_swap");
	offsets->d3d9.reset =
		(uint32_t)config_get_uint(config, "d3d9", "reset");
	offsets->d3d9.reset_ex =
		(uint32_t)config_get_uint(config, "d3d9", "reset_ex");

	offsets->dxgi.present =
		(uint32_t)config_get_uint(config, "dxgi", "present");
	offsets->dxgi.resize =
		(uint32_t)config_get_uint(config, "dxgi", "resize");

	config_close(config);
	return true;
}

bool load_graphics_offsets(bool is32bit)
{
	char *offset_exe_path = NULL;
	struct dstr offset_exe = {0};
	struct dstr str = {0};
	os_process_pipe_t *pp;
	bool success = false;
	char data[128];

	dstr_copy(&offset_exe, "get-graphics-offsets");
	dstr_cat(&offset_exe, is32bit ? "32.exe" : "64.exe");
	offset_exe_path = obs_module_file(offset_exe.array);

	pp = os_process_pipe_create(offset_exe_path, "r");
	if (!pp) {
		blog(LOG_INFO, "load_graphics_offsets: Failed to start '%s'",
				offset_exe.array);
		goto error;
	}

	for (;;) {
		size_t len = os_process_pipe_read(pp, (uint8_t*)data, 128);
		if (!len)
			break;

		dstr_ncat(&str, data, len);
	}

	success = load_offsets_from_string(is32bit ? &offsets32 : &offsets64,
			str.array);
	if (!success) {
		blog(LOG_INFO, "load_graphics_offsets: Failed to load string");
	}

	os_process_pipe_destroy(pp);

error:
	bfree(offset_exe_path);
	dstr_free(&offset_exe);
	dstr_free(&str);
	return success;
}
