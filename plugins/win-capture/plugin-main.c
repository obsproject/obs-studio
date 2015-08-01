#include <windows.h>
#include <obs-module.h>
#include <util/windows/win-version.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-capture", "en-US")

extern struct obs_source_info duplicator_capture_info;
extern struct obs_source_info monitor_capture_info;
extern struct obs_source_info window_capture_info;
extern struct obs_source_info game_capture_info;

extern bool load_graphics_offsets(bool is32bit);

/* temporary, will eventually be erased once we figure out how to create both
 * 32bit and 64bit versions of the helpers/hook */
#ifdef _WIN64
#define IS32BIT false
#else
#define IS32BIT true
#endif

bool obs_module_load(void)
{
	struct win_version_info ver;
	bool win8_or_above = false;

	get_win_ver(&ver);

	win8_or_above = ver.major > 6 || (ver.major == 6 && ver.minor >= 2);

	obs_enter_graphics();

	if (win8_or_above && gs_get_device_type() == GS_DEVICE_DIRECT3D_11)
		obs_register_source(&duplicator_capture_info);
	else
		obs_register_source(&monitor_capture_info);

	obs_leave_graphics();

	obs_register_source(&window_capture_info);

	if (load_graphics_offsets(IS32BIT)) {
		load_graphics_offsets(!IS32BIT);
		obs_register_source(&game_capture_info);
	}

	return true;
}
