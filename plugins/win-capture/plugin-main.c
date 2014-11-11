#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-capture", "en-US")

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
	obs_register_source(&monitor_capture_info);
	obs_register_source(&window_capture_info);

	if (load_graphics_offsets(IS32BIT)) {
		load_graphics_offsets(!IS32BIT);
		obs_register_source(&game_capture_info);
	}

	return true;
}
