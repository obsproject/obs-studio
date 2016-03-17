#include <inttypes.h>
#include <obs-module.h>
#include <util/platform.h>
#include <windows.h>
#include <dxgi.h>
#include <emmintrin.h>
#include <ipc-util/pipe.h>
#include "obfuscate.h"
#include "inject-library.h"
#include "graphics-hook-info.h"
#include "window-helpers.h"
#include "cursor-capture.h"

#define do_log(level, format, ...) \
	blog(level, "[game-capture: '%s'] " format, \
			obs_source_get_name(gc->source), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

#define SETTING_ANY_FULLSCREEN   "capture_any_fullscreen"
#define SETTING_CAPTURE_WINDOW   "window"
#define SETTING_ACTIVE_WINDOW    "active_window"
#define SETTING_WINDOW_PRIORITY  "priority"
#define SETTING_COMPATIBILITY    "sli_compatibility"
#define SETTING_FORCE_SCALING    "force_scaling"
#define SETTING_SCALE_RES        "scale_res"
#define SETTING_CURSOR           "capture_cursor"
#define SETTING_TRANSPARENCY     "allow_transparency"
#define SETTING_LIMIT_FRAMERATE  "limit_framerate"
#define SETTING_CAPTURE_OVERLAYS "capture_overlays"
#define SETTING_ANTI_CHEAT_HOOK  "anti_cheat_hook"

#define TEXT_GAME_CAPTURE        obs_module_text("GameCapture")
#define TEXT_ANY_FULLSCREEN      obs_module_text("GameCapture.AnyFullscreen")
#define TEXT_SLI_COMPATIBILITY   obs_module_text("Compatibility")
#define TEXT_ALLOW_TRANSPARENCY  obs_module_text("AllowTransparency")
#define TEXT_FORCE_SCALING       obs_module_text("GameCapture.ForceScaling")
#define TEXT_SCALE_RES           obs_module_text("GameCapture.ScaleRes")
#define TEXT_WINDOW              obs_module_text("WindowCapture.Window")
#define TEXT_MATCH_PRIORITY      obs_module_text("WindowCapture.Priority")
#define TEXT_MATCH_TITLE         obs_module_text("WindowCapture.Priority.Title")
#define TEXT_MATCH_CLASS         obs_module_text("WindowCapture.Priority.Class")
#define TEXT_MATCH_EXE           obs_module_text("WindowCapture.Priority.Exe")
#define TEXT_CAPTURE_CURSOR      obs_module_text("CaptureCursor")
#define TEXT_LIMIT_FRAMERATE     obs_module_text("GameCapture.LimitFramerate")
#define TEXT_CAPTURE_OVERLAYS    obs_module_text("GameCapture.CaptureOverlays")
#define TEXT_ANTI_CHEAT_HOOK     obs_module_text("GameCapture.AntiCheatHook")

#define DEFAULT_RETRY_INTERVAL 2.0f
#define ERROR_RETRY_INTERVAL 4.0f

struct game_capture_config {
	char                          *title;
	char                          *class;
	char                          *executable;
	enum window_priority          priority;
	uint32_t                      scale_cx;
	uint32_t                      scale_cy;
	bool                          cursor : 1;
	bool                          force_shmem : 1;
	bool                          capture_any_fullscreen : 1;
	bool                          force_scaling : 1;
	bool                          allow_transparency : 1;
	bool                          limit_framerate : 1;
	bool                          capture_overlays : 1;
	bool                          anticheat_hook : 1;
};

struct game_capture {
	obs_source_t                  *source;

	struct cursor_data            cursor_data;
	HANDLE                        injector_process;
	uint32_t                      cx;
	uint32_t                      cy;
	uint32_t                      pitch;
	DWORD                         process_id;
	DWORD                         thread_id;
	HWND                          next_window;
	HWND                          window;
	float                         retry_time;
	float                         fps_reset_time;
	float                         retry_interval;
	bool                          wait_for_target_startup : 1;
	bool                          active : 1;
	bool                          capturing : 1;
	bool                          activate_hook : 1;
	bool                          process_is_64bit : 1;
	bool                          error_acquiring : 1;
	bool                          dwm_capture : 1;
	bool                          initial_config : 1;
	bool                          convert_16bit : 1;

	struct game_capture_config    config;

	ipc_pipe_server_t             pipe;
	gs_texture_t                  *texture;
	struct hook_info              *global_hook_info;
	HANDLE                        keep_alive;
	HANDLE                        hook_restart;
	HANDLE                        hook_stop;
	HANDLE                        hook_ready;
	HANDLE                        hook_exit;
	HANDLE                        hook_data_map;
	HANDLE                        global_hook_info_map;
	HANDLE                        target_process;
	HANDLE                        texture_mutexes[2];

	union {
		struct {
			struct shmem_data *shmem_data;
			uint8_t *texture_buffers[2];
		};

		struct shtex_data *shtex_data;
		void *data;
	};

	void (*copy_texture)(struct game_capture*);
};

struct graphics_offsets offsets32 = {0};
struct graphics_offsets offsets64 = {0};

static inline enum gs_color_format convert_format(uint32_t format)
{
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:     return GS_RGBA;
	case DXGI_FORMAT_B8G8R8X8_UNORM:     return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_UNORM:     return GS_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:  return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM: return GS_RGBA16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT: return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return GS_RGBA32F;
	}

	return GS_UNKNOWN;
}

static void close_handle(HANDLE *p_handle)
{
	HANDLE handle = *p_handle;
	if (handle) {
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
		*p_handle = NULL;
	}
}

static inline HMODULE kernel32(void)
{
	static HMODULE kernel32_handle = NULL;
	if (!kernel32_handle)
		kernel32_handle = GetModuleHandleW(L"kernel32");
	return kernel32_handle;
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
		DWORD process_id)
{
	static HANDLE (WINAPI *open_process_proc)(DWORD, BOOL, DWORD) = NULL;
	if (!open_process_proc)
		open_process_proc = get_obfuscated_func(kernel32(),
				"NuagUykjcxr", 0x1B694B59451ULL);

	return open_process_proc(desired_access, inherit_handle, process_id);
}

static void stop_capture(struct game_capture *gc)
{
	ipc_pipe_server_free(&gc->pipe);

	if (gc->hook_stop) {
		SetEvent(gc->hook_stop);
	}
	if (gc->global_hook_info) {
		UnmapViewOfFile(gc->global_hook_info);
		gc->global_hook_info = NULL;
	}
	if (gc->data) {
		UnmapViewOfFile(gc->data);
		gc->data = NULL;
	}

	close_handle(&gc->keep_alive);
	close_handle(&gc->hook_restart);
	close_handle(&gc->hook_stop);
	close_handle(&gc->hook_ready);
	close_handle(&gc->hook_exit);
	close_handle(&gc->hook_data_map);
	close_handle(&gc->global_hook_info_map);
	close_handle(&gc->target_process);
	close_handle(&gc->texture_mutexes[0]);
	close_handle(&gc->texture_mutexes[1]);

	if (gc->texture) {
		obs_enter_graphics();
		gs_texture_destroy(gc->texture);
		obs_leave_graphics();
		gc->texture = NULL;
	}

	gc->copy_texture = NULL;
	gc->wait_for_target_startup = false;
	gc->active = false;
	gc->capturing = false;
}

static inline void free_config(struct game_capture_config *config)
{
	bfree(config->title);
	bfree(config->class);
	bfree(config->executable);
	memset(config, 0, sizeof(*config));
}

static void game_capture_destroy(void *data)
{
	struct game_capture *gc = data;
	stop_capture(gc);

	obs_enter_graphics();
	cursor_data_free(&gc->cursor_data);
	obs_leave_graphics();

	free_config(&gc->config);
	bfree(gc);
}

static inline void get_config(struct game_capture_config *cfg,
		obs_data_t *settings, const char *window)
{
	int ret;
	const char *scale_str;

	build_window_strings(window, &cfg->class, &cfg->title,
			&cfg->executable);

	cfg->capture_any_fullscreen = obs_data_get_bool(settings,
			SETTING_ANY_FULLSCREEN);
	cfg->priority = (enum window_priority)obs_data_get_int(settings,
			SETTING_WINDOW_PRIORITY);
	cfg->force_shmem = obs_data_get_bool(settings,
			SETTING_COMPATIBILITY);
	cfg->cursor = obs_data_get_bool(settings, SETTING_CURSOR);
	cfg->allow_transparency = obs_data_get_bool(settings,
			SETTING_TRANSPARENCY);
	cfg->force_scaling = obs_data_get_bool(settings,
			SETTING_FORCE_SCALING);
	cfg->limit_framerate = obs_data_get_bool(settings,
			SETTING_LIMIT_FRAMERATE);
	cfg->capture_overlays = obs_data_get_bool(settings,
			SETTING_CAPTURE_OVERLAYS);
	cfg->anticheat_hook = obs_data_get_bool(settings,
			SETTING_ANTI_CHEAT_HOOK);

	scale_str = obs_data_get_string(settings, SETTING_SCALE_RES);
	ret = sscanf(scale_str, "%"PRIu32"x%"PRIu32,
			&cfg->scale_cx, &cfg->scale_cy);

	cfg->scale_cx &= ~2;
	cfg->scale_cy &= ~2;

	if (cfg->force_scaling) {
		if (ret != 2 || cfg->scale_cx == 0 || cfg->scale_cy == 0) {
			cfg->scale_cx = 0;
			cfg->scale_cy = 0;
		}
	}
}

static inline int s_cmp(const char *str1, const char *str2)
{
	if (!str1 || !str2)
		return -1;

	return strcmp(str1, str2);
}

static inline bool capture_needs_reset(struct game_capture_config *cfg1,
		struct game_capture_config *cfg2)
{
	if (cfg1->capture_any_fullscreen != cfg2->capture_any_fullscreen) {
		return true;

	} else if (!cfg1->capture_any_fullscreen &&
			(s_cmp(cfg1->class, cfg2->class) != 0 ||
			 s_cmp(cfg1->title, cfg2->title) != 0 ||
			 s_cmp(cfg1->executable, cfg2->executable) != 0 ||
			 cfg1->priority != cfg2->priority)) {
		return true;

	} else if (cfg1->force_scaling != cfg2->force_scaling) {
		return true;

	} else if (cfg1->force_scaling &&
			(cfg1->scale_cx != cfg2->scale_cx ||
			 cfg1->scale_cy != cfg2->scale_cy)) {
		return true;

	} else if (cfg1->force_shmem != cfg2->force_shmem) {
		return true;

	} else if (cfg1->limit_framerate != cfg2->limit_framerate) {
		return true;

	} else if (cfg1->capture_overlays != cfg2->capture_overlays) {
		return true;
	}

	return false;
}

static void game_capture_update(void *data, obs_data_t *settings)
{
	struct game_capture *gc = data;
	struct game_capture_config cfg;
	bool reset_capture = false;
	const char *window = obs_data_get_string(settings,
			SETTING_CAPTURE_WINDOW);

	get_config(&cfg, settings, window);
	reset_capture = capture_needs_reset(&cfg, &gc->config);

	if (cfg.force_scaling && (cfg.scale_cx == 0 || cfg.scale_cy == 0)) {
		gc->error_acquiring = true;
		warn("error acquiring, scale is bad");
	} else {
		gc->error_acquiring = false;
	}

	free_config(&gc->config);
	gc->config = cfg;
	gc->activate_hook = !!window && !!*window;
	gc->retry_interval = DEFAULT_RETRY_INTERVAL;

	if (!gc->initial_config) {
		if (reset_capture) {
			stop_capture(gc);
		}
	} else {
		gc->initial_config = false;
	}
}

static void *game_capture_create(obs_data_t *settings, obs_source_t *source)
{
	struct game_capture *gc = bzalloc(sizeof(*gc));
	gc->source = source;
	gc->initial_config = true;
	gc->retry_interval = DEFAULT_RETRY_INTERVAL;

	game_capture_update(gc, settings);
	return gc;
}

static inline HANDLE create_event_id(bool manual_reset, bool initial_state,
		const char *name, DWORD process_id)
{
	char new_name[128];
	sprintf(new_name, "%s%lu", name, process_id);
	return CreateEventA(NULL, manual_reset, initial_state, new_name);
}

static inline HANDLE open_event_id(const char *name, DWORD process_id)
{
	char new_name[128];
	sprintf(new_name, "%s%lu", name, process_id);
	return OpenEventA(EVENT_ALL_ACCESS, false, new_name);
}

#define STOP_BEING_BAD \
	"  This is most likely due to security software. Please make sure " \
        "that the OBS installation folder is excluded/ignored in the "      \
        "settings of the security software you are using."

static bool check_file_integrity(struct game_capture *gc, const char *file,
		const char *name)
{
	DWORD error;
	HANDLE handle;
	wchar_t *w_file = NULL;

	if (!file || !*file) {
		warn("Game capture %s not found." STOP_BEING_BAD, name);
		return false;
	}

	if (!os_utf8_to_wcs_ptr(file, 0, &w_file)) {
		warn("Could not convert file name to wide string");
		return false;
	}

	handle = CreateFileW(w_file, GENERIC_READ | GENERIC_EXECUTE,
			FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	bfree(w_file);

	if (handle != INVALID_HANDLE_VALUE) {
		CloseHandle(handle);
		return true;
	}

	error = GetLastError();
	if (error == ERROR_FILE_NOT_FOUND) {
		warn("Game capture file '%s' not found."
				STOP_BEING_BAD, file);
	} else if (error == ERROR_ACCESS_DENIED) {
		warn("Game capture file '%s' could not be loaded."
				STOP_BEING_BAD, file);
	} else {
		warn("Game capture file '%s' could not be loaded: %lu."
				STOP_BEING_BAD, file, error);
	}

	return false;
}

static inline bool is_64bit_windows(void)
{
#ifdef _WIN64
	return true;
#else
	BOOL x86 = false;
	bool success = !!IsWow64Process(GetCurrentProcess(), &x86);
	return success && !!x86;
#endif
}

static inline bool is_64bit_process(HANDLE process)
{
	BOOL x86 = true;
	if (is_64bit_windows()) {
		bool success = !!IsWow64Process(process, &x86);
		if (!success) {
			return false;
		}
	}

	return !x86;
}

static inline bool open_target_process(struct game_capture *gc)
{
	gc->target_process = open_process(
			PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
			false, gc->process_id);
	if (!gc->target_process) {
		warn("could not open process: %s", gc->config.executable);
		return false;
	}

	gc->process_is_64bit = is_64bit_process(gc->target_process);
	return true;
}

static inline bool init_keepalive(struct game_capture *gc)
{
	gc->keep_alive = create_event_id(false, false, EVENT_HOOK_KEEPALIVE,
			gc->process_id);
	if (!gc->keep_alive) {
		warn("failed to create keepalive event");
		return false;
	}

	return true;
}

static inline bool init_texture_mutexes(struct game_capture *gc)
{
	gc->texture_mutexes[0] = get_mutex_plus_id(MUTEX_TEXTURE1,
			gc->process_id);
	gc->texture_mutexes[1] = get_mutex_plus_id(MUTEX_TEXTURE2,
			gc->process_id);

	if (!gc->texture_mutexes[0] || !gc->texture_mutexes[1]) {
		warn("failed to create texture mutexes: %lu", GetLastError());
		return false;
	}

	return true;
}

/* if there's already a hook in the process, then signal and start */
static inline bool attempt_existing_hook(struct game_capture *gc)
{
	gc->hook_restart = open_event_id(EVENT_CAPTURE_RESTART, gc->process_id);
	if (gc->hook_restart) {
		debug("existing hook found, signaling process: %s",
				gc->config.executable);
		SetEvent(gc->hook_restart);
		return true;
	}

	return false;
}

static inline void reset_frame_interval(struct game_capture *gc)
{
	struct obs_video_info ovi;
	uint64_t interval = 0;

	if (obs_get_video_info(&ovi)) {
		interval = ovi.fps_den * 1000000000ULL / ovi.fps_num;

		/* Always limit capture framerate to some extent.  If a game
		 * running at 900 FPS is being captured without some sort of
		 * limited capture interval, it will dramatically reduce
		 * performance. */
		if (!gc->config.limit_framerate)
			interval /= 2;
	}

	gc->global_hook_info->frame_interval = interval;
}

static inline bool init_hook_info(struct game_capture *gc)
{
	gc->global_hook_info_map = get_hook_info(gc->process_id);
	if (!gc->global_hook_info_map) {
		warn("init_hook_info: get_hook_info failed: %lu",
				GetLastError());
		return false;
	}

	gc->global_hook_info = MapViewOfFile(gc->global_hook_info_map,
			FILE_MAP_ALL_ACCESS, 0, 0,
			sizeof(*gc->global_hook_info));
	if (!gc->global_hook_info) {
		warn("init_hook_info: failed to map data view: %lu",
				GetLastError());
		return false;
	}

	gc->global_hook_info->offsets = gc->process_is_64bit ?
		offsets64 : offsets32;
	gc->global_hook_info->capture_overlay = gc->config.capture_overlays;
	gc->global_hook_info->force_shmem = gc->config.force_shmem;
	gc->global_hook_info->use_scale = gc->config.force_scaling;
	gc->global_hook_info->cx = gc->config.scale_cx;
	gc->global_hook_info->cy = gc->config.scale_cy;
	reset_frame_interval(gc);

	obs_enter_graphics();
	if (!gs_shared_texture_available())
		gc->global_hook_info->force_shmem = true;
	obs_leave_graphics();

	obs_enter_graphics();
	if (!gs_shared_texture_available())
		gc->global_hook_info->force_shmem = true;
	obs_leave_graphics();

	return true;
}

static void pipe_log(void *param, uint8_t *data, size_t size)
{
	struct game_capture *gc = param;
	if (data && size)
		info("%s", data);
}

static inline bool init_pipe(struct game_capture *gc)
{
	char name[64];
	sprintf(name, "%s%lu", PIPE_NAME, gc->process_id);

	if (!ipc_pipe_server_start(&gc->pipe, name, pipe_log, gc)) {
		warn("init_pipe: failed to start pipe");
		return false;
	}

	return true;
}

static inline int inject_library(HANDLE process, const wchar_t *dll)
{
	return inject_library_obf(process, dll,
			"D|hkqkW`kl{k\\osofj", 0xa178ef3655e5ade7,
			"[uawaRzbhh{tIdkj~~", 0x561478dbd824387c,
			"[fr}pboIe`dlN}", 0x395bfbc9833590fd,
			"\\`zs}gmOzhhBq", 0x12897dd89168789a,
			"GbfkDaezbp~X", 0x76aff7238788f7db);
}

static inline bool hook_direct(struct game_capture *gc,
		const char *hook_path_rel)
{
	wchar_t hook_path_abs_w[MAX_PATH];
	wchar_t *hook_path_rel_w;
	wchar_t *path_ret;
	HANDLE process;
	int ret;

	os_utf8_to_wcs_ptr(hook_path_rel, 0, &hook_path_rel_w);
	if (!hook_path_rel_w) {
		warn("hook_direct: could not convert string");
		return false;
	}

	path_ret = _wfullpath(hook_path_abs_w, hook_path_rel_w, MAX_PATH);
	bfree(hook_path_rel_w);

	if (path_ret == NULL) {
		warn("hook_direct: could not make absolute path");
		return false;
	}

	process = open_process(PROCESS_ALL_ACCESS, false, gc->process_id);
	if (!process) {
		warn("hook_direct: could not open process: %s (%lu)",
				gc->config.executable, GetLastError());
		return false;
	}

	ret = inject_library(process, hook_path_abs_w);
	CloseHandle(process);

	if (ret != 0) {
		warn("hook_direct: inject failed: %d", ret);
		return false;
	}

	return true;
}

static inline bool create_inject_process(struct game_capture *gc,
		const char *inject_path, const char *hook_dll)
{
	wchar_t *command_line_w = malloc(4096 * sizeof(wchar_t));
	wchar_t *inject_path_w;
	wchar_t *hook_dll_w;
	bool anti_cheat = gc->config.anticheat_hook;
	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
	bool success = false;

	os_utf8_to_wcs_ptr(inject_path, 0, &inject_path_w);
	os_utf8_to_wcs_ptr(hook_dll, 0, &hook_dll_w);

	si.cb = sizeof(si);

	swprintf(command_line_w, 4096, L"\"%s\" \"%s\" %lu %lu",
			inject_path_w, hook_dll_w,
			(unsigned long)anti_cheat,
			anti_cheat ? gc->thread_id : gc->process_id);

	success = !!CreateProcessW(inject_path_w, command_line_w, NULL, NULL,
			false, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (success) {
		CloseHandle(pi.hThread);
		gc->injector_process = pi.hProcess;
	} else {
		warn("Failed to create inject helper process: %lu",
				GetLastError());
	}

	free(command_line_w);
	bfree(inject_path_w);
	bfree(hook_dll_w);
	return success;
}

static inline bool inject_hook(struct game_capture *gc)
{
	bool matching_architecture;
	bool success = false;
	const char *hook_dll;
	char *inject_path;
	char *hook_path;

	if (gc->process_is_64bit) {
		hook_dll = "graphics-hook64.dll";
		inject_path = obs_module_file("inject-helper64.exe");
	} else {
		hook_dll = "graphics-hook32.dll";
		inject_path = obs_module_file("inject-helper32.exe");
	}

	hook_path = obs_module_file(hook_dll);

	if (!check_file_integrity(gc, inject_path, "inject helper")) {
		goto cleanup;
	}
	if (!check_file_integrity(gc, hook_path, "graphics hook")) {
		goto cleanup;
	}

#ifdef _WIN64
	matching_architecture = gc->process_is_64bit;
#else
	matching_architecture = !gc->process_is_64bit;
#endif

	if (matching_architecture && !gc->config.anticheat_hook) {
		info("using direct hook");
		success = hook_direct(gc, hook_path);
	} else {
		info("using helper (%s hook)", gc->config.anticheat_hook ?
				"compatibility" : "direct");
		success = create_inject_process(gc, inject_path, hook_dll);
	}

cleanup:
	bfree(inject_path);
	bfree(hook_path);
	return success;
}

static bool init_hook(struct game_capture *gc)
{
	if (gc->config.capture_any_fullscreen) {
		struct dstr name = {0};
		if (get_window_exe(&name, gc->next_window)) {
			info("attempting to hook fullscreen process: %s",
					name.array);
			dstr_free(&name);
		}
	} else {
		info("attempting to hook process: %s", gc->config.executable);
	}

	if (!open_target_process(gc)) {
		return false;
	}
	if (!init_keepalive(gc)) {
		return false;
	}
	if (!init_texture_mutexes(gc)) {
		return false;
	}
	if (!init_hook_info(gc)) {
		return false;
	}
	if (!init_pipe(gc)) {
		return false;
	}
	if (!attempt_existing_hook(gc)) {
		if (!inject_hook(gc)) {
			return false;
		}
	}

	gc->window = gc->next_window;
	gc->next_window = NULL;
	gc->active = true;
	return true;
}

static void get_fullscreen_window(struct game_capture *gc)
{
	HWND window = GetForegroundWindow();
	MONITORINFO mi = {0};
	HMONITOR monitor;
	DWORD styles;
	RECT rect;

	gc->next_window = NULL;

	if (!window) {
		return;
	}
	if (!GetWindowRect(window, &rect)) {
		return;
	}

	/* ignore regular maximized windows */
	styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	if ((styles & WS_MAXIMIZE) != 0 && (styles & WS_BORDER) != 0) {
		return;
	}

	monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	if (!monitor) {
		return;
	}

	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(monitor, &mi)) {
		return;
	}

	if (rect.left   == mi.rcMonitor.left   &&
	    rect.right  == mi.rcMonitor.right  &&
	    rect.bottom == mi.rcMonitor.bottom &&
	    rect.top    == mi.rcMonitor.top) {

		/* always wait a bit for the target process to start up before
		 * starting the hook process; sometimes they have important
		 * modules to load first or other hooks (such as steam) need a
		 * little bit of time to load.  ultimately this helps prevent
		 * crashes */
		if (gc->wait_for_target_startup) {
			gc->retry_interval = 3.0f;
			gc->wait_for_target_startup = false;
		} else {
			gc->next_window = window;
		}
	} else {
		gc->wait_for_target_startup = true;
	}
}

static void get_selected_window(struct game_capture *gc)
{
	if (strcmpi(gc->config.class, "dwm") == 0) {
		wchar_t class_w[512];
		os_utf8_to_wcs(gc->config.class, 0, class_w, 512);
		gc->next_window = FindWindowW(class_w, NULL);
	} else {
		gc->next_window = find_window(INCLUDE_MINIMIZED,
				gc->config.priority,
				gc->config.class,
				gc->config.title,
				gc->config.executable);
	}
}

static void try_hook(struct game_capture *gc)
{
	if (gc->config.capture_any_fullscreen) {
		get_fullscreen_window(gc);
	} else {
		get_selected_window(gc);
	}

	if (gc->next_window) {
		gc->thread_id = GetWindowThreadProcessId(gc->next_window,
				&gc->process_id);

		if (!gc->thread_id || !gc->process_id) {
			warn("error acquiring, failed to get window "
					"thread/process ids: %lu",
					GetLastError());
			gc->error_acquiring = true;
			return;
		}

		if (!init_hook(gc)) {
			stop_capture(gc);
		}
	} else {
		gc->active = false;
	}
}

static inline bool init_events(struct game_capture *gc)
{
	if (!gc->hook_restart) {
		gc->hook_restart = get_event_plus_id(EVENT_CAPTURE_RESTART,
				gc->process_id);
		if (!gc->hook_restart) {
			warn("init_events: failed to get hook_restart "
			     "event: %lu", GetLastError());
			return false;
		}
	}

	if (!gc->hook_stop) {
		gc->hook_stop = get_event_plus_id(EVENT_CAPTURE_STOP,
				gc->process_id);
		if (!gc->hook_stop) {
			warn("init_events: failed to get hook_stop event: %lu",
					GetLastError());
			return false;
		}
	}

	if (!gc->hook_ready) {
		gc->hook_ready = get_event_plus_id(EVENT_HOOK_READY,
				gc->process_id);
		if (!gc->hook_ready) {
			warn("init_events: failed to get hook_ready event: %lu",
					GetLastError());
			return false;
		}
	}

	if (!gc->hook_exit) {
		gc->hook_exit = get_event_plus_id(EVENT_HOOK_EXIT,
				gc->process_id);
		if (!gc->hook_exit) {
			warn("init_events: failed to get hook_exit event: %lu",
					GetLastError());
			return false;
		}
	}

	return true;
}

enum capture_result {
	CAPTURE_FAIL,
	CAPTURE_RETRY,
	CAPTURE_SUCCESS
};

static inline enum capture_result init_capture_data(struct game_capture *gc)
{
	char name[64];
	sprintf(name, "%s%u", SHMEM_TEXTURE, gc->global_hook_info->map_id);

	gc->cx = gc->global_hook_info->cx;
	gc->cy = gc->global_hook_info->cy;
	gc->pitch = gc->global_hook_info->pitch;

	if (gc->data) {
		UnmapViewOfFile(gc->data);
		gc->data = NULL;
	}

	CloseHandle(gc->hook_data_map);

	gc->hook_data_map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, name);
	if (!gc->hook_data_map) {
		DWORD error = GetLastError();
		if (error == 2) {
			return CAPTURE_RETRY;
		} else {
			warn("init_capture_data: failed to open file "
			     "mapping: %lu", error);
		}
		return CAPTURE_FAIL;
	}

	gc->data = MapViewOfFile(gc->hook_data_map, FILE_MAP_ALL_ACCESS, 0, 0,
			gc->global_hook_info->map_size);
	if (!gc->data) {
		warn("init_capture_data: failed to map data view: %lu",
				GetLastError());
		return CAPTURE_FAIL;
	}

	return CAPTURE_SUCCESS;
}

#define PIXEL_16BIT_SIZE 2
#define PIXEL_32BIT_SIZE 4

static inline uint32_t convert_5_to_8bit(uint16_t val)
{
	return (uint32_t)((double)(val & 0x1F) * (255.0/31.0));
}

static inline uint32_t convert_6_to_8bit(uint16_t val)
{
	return (uint32_t)((double)(val & 0x3F) * (255.0/63.0));
}

static void copy_b5g6r5_tex(struct game_capture *gc, int cur_texture,
		uint8_t *data, uint32_t pitch)
{
	uint8_t *input = gc->texture_buffers[cur_texture];
	uint32_t gc_cx = gc->cx;
	uint32_t gc_cy = gc->cy;
	uint32_t gc_pitch = gc->pitch;

	for (uint32_t y = 0; y < gc_cy; y++) {
		uint8_t *row = input + (gc_pitch * y);
		uint8_t *out = data + (pitch * y);

		for (uint32_t x = 0; x < gc_cx; x += 8) {
			__m128i pixels_blue, pixels_green, pixels_red;
			__m128i pixels_result;
			__m128i *pixels_dest;

			__m128i *pixels_src = (__m128i*)(row + x * sizeof(uint16_t));
			__m128i pixels = _mm_load_si128(pixels_src);

			__m128i zero = _mm_setzero_si128();
			__m128i pixels_low = _mm_unpacklo_epi16(pixels, zero);
			__m128i pixels_high = _mm_unpackhi_epi16(pixels, zero);

			__m128i blue_channel_mask = _mm_set1_epi32(0x0000001F);
			__m128i blue_offset = _mm_set1_epi32(0x00000003);
			__m128i green_channel_mask = _mm_set1_epi32(0x000007E0);
			__m128i green_offset = _mm_set1_epi32(0x00000008);
			__m128i red_channel_mask = _mm_set1_epi32(0x0000F800);
			__m128i red_offset = _mm_set1_epi32(0x00000300);

			pixels_blue = _mm_and_si128(pixels_low, blue_channel_mask);
			pixels_blue = _mm_slli_epi32(pixels_blue, 3);
			pixels_blue = _mm_add_epi32(pixels_blue, blue_offset);

			pixels_green = _mm_and_si128(pixels_low, green_channel_mask);
			pixels_green = _mm_add_epi32(pixels_green, green_offset);
			pixels_green = _mm_slli_epi32(pixels_green, 5);

			pixels_red = _mm_and_si128(pixels_low, red_channel_mask);
			pixels_red = _mm_add_epi32(pixels_red, red_offset);
			pixels_red = _mm_slli_epi32(pixels_red, 8);

			pixels_result = _mm_set1_epi32(0xFF000000);
			pixels_result = _mm_or_si128(pixels_result, pixels_blue);
			pixels_result = _mm_or_si128(pixels_result, pixels_green);
			pixels_result = _mm_or_si128(pixels_result, pixels_red);

			pixels_dest = (__m128i*)(out + x * sizeof(uint32_t));
			_mm_store_si128(pixels_dest, pixels_result);

			pixels_blue = _mm_and_si128(pixels_high, blue_channel_mask);
			pixels_blue = _mm_slli_epi32(pixels_blue, 3);
			pixels_blue = _mm_add_epi32(pixels_blue, blue_offset);

			pixels_green = _mm_and_si128(pixels_high, green_channel_mask);
			pixels_green = _mm_add_epi32(pixels_green, green_offset);
			pixels_green = _mm_slli_epi32(pixels_green, 5);

			pixels_red = _mm_and_si128(pixels_high, red_channel_mask);
			pixels_red = _mm_add_epi32(pixels_red, red_offset);
			pixels_red = _mm_slli_epi32(pixels_red, 8);

			pixels_result = _mm_set1_epi32(0xFF000000);
			pixels_result = _mm_or_si128(pixels_result, pixels_blue);
			pixels_result = _mm_or_si128(pixels_result, pixels_green);
			pixels_result = _mm_or_si128(pixels_result, pixels_red);

			pixels_dest = (__m128i*)(out + (x + 4) * sizeof(uint32_t));
			_mm_store_si128(pixels_dest, pixels_result);
		}
	}
}

static void copy_b5g5r5a1_tex(struct game_capture *gc, int cur_texture,
		uint8_t *data, uint32_t pitch)
{
	uint8_t *input = gc->texture_buffers[cur_texture];
	uint32_t gc_cx = gc->cx;
	uint32_t gc_cy = gc->cy;
	uint32_t gc_pitch = gc->pitch;

	for (uint32_t y = 0; y < gc_cy; y++) {
		uint8_t *row = input + (gc_pitch * y);
		uint8_t *out = data + (pitch * y);

		for (uint32_t x = 0; x < gc_cx; x += 8) {
			__m128i pixels_blue, pixels_green, pixels_red, pixels_alpha;
			__m128i pixels_result;
			__m128i *pixels_dest;

			__m128i *pixels_src = (__m128i*)(row + x * sizeof(uint16_t));
			__m128i pixels = _mm_load_si128(pixels_src);

			__m128i zero = _mm_setzero_si128();
			__m128i pixels_low = _mm_unpacklo_epi16(pixels, zero);
			__m128i pixels_high = _mm_unpackhi_epi16(pixels, zero);

			__m128i blue_channel_mask = _mm_set1_epi32(0x0000001F);
			__m128i blue_offset = _mm_set1_epi32(0x00000003);
			__m128i green_channel_mask = _mm_set1_epi32(0x000003E0);
			__m128i green_offset = _mm_set1_epi32(0x000000C);
			__m128i red_channel_mask = _mm_set1_epi32(0x00007C00);
			__m128i red_offset = _mm_set1_epi32(0x00000180);
			__m128i alpha_channel_mask = _mm_set1_epi32(0x00008000);
			__m128i alpha_offset = _mm_set1_epi32(0x00000001);
			__m128i alpha_mask32 = _mm_set1_epi32(0xFF000000);

			pixels_blue = _mm_and_si128(pixels_low, blue_channel_mask);
			pixels_blue = _mm_slli_epi32(pixels_blue, 3);
			pixels_blue = _mm_add_epi32(pixels_blue, blue_offset);

			pixels_green = _mm_and_si128(pixels_low, green_channel_mask);
			pixels_green = _mm_add_epi32(pixels_green, green_offset);
			pixels_green = _mm_slli_epi32(pixels_green, 6);

			pixels_red = _mm_and_si128(pixels_low, red_channel_mask);
			pixels_red = _mm_add_epi32(pixels_red, red_offset);
			pixels_red = _mm_slli_epi32(pixels_red, 9);

			pixels_alpha = _mm_and_si128(pixels_low, alpha_channel_mask);
			pixels_alpha = _mm_srli_epi32(pixels_alpha, 15);
			pixels_alpha = _mm_sub_epi32(pixels_alpha, alpha_offset);
			pixels_alpha = _mm_andnot_si128(pixels_alpha, alpha_mask32);

			pixels_result = pixels_red;
			pixels_result = _mm_or_si128(pixels_result, pixels_alpha);
			pixels_result = _mm_or_si128(pixels_result, pixels_blue);
			pixels_result = _mm_or_si128(pixels_result, pixels_green);

			pixels_dest = (__m128i*)(out + x * sizeof(uint32_t));
			_mm_store_si128(pixels_dest, pixels_result);

			pixels_blue = _mm_and_si128(pixels_high, blue_channel_mask);
			pixels_blue = _mm_slli_epi32(pixels_blue, 3);
			pixels_blue = _mm_add_epi32(pixels_blue, blue_offset);

			pixels_green = _mm_and_si128(pixels_high, green_channel_mask);
			pixels_green = _mm_add_epi32(pixels_green, green_offset);
			pixels_green = _mm_slli_epi32(pixels_green, 6);

			pixels_red = _mm_and_si128(pixels_high, red_channel_mask);
			pixels_red = _mm_add_epi32(pixels_red, red_offset);
			pixels_red = _mm_slli_epi32(pixels_red, 9);

			pixels_alpha = _mm_and_si128(pixels_high, alpha_channel_mask);
			pixels_alpha = _mm_srli_epi32(pixels_alpha, 15);
			pixels_alpha = _mm_sub_epi32(pixels_alpha, alpha_offset);
			pixels_alpha = _mm_andnot_si128(pixels_alpha, alpha_mask32);

			pixels_result = pixels_red;
			pixels_result = _mm_or_si128(pixels_result, pixels_alpha);
			pixels_result = _mm_or_si128(pixels_result, pixels_blue);
			pixels_result = _mm_or_si128(pixels_result, pixels_green);

			pixels_dest = (__m128i*)(out + (x + 4) * sizeof(uint32_t));
			_mm_store_si128(pixels_dest, pixels_result);
		}
	}
}

static inline void copy_16bit_tex(struct game_capture *gc, int cur_texture,
		uint8_t *data, uint32_t pitch)
{
	if (gc->global_hook_info->format == DXGI_FORMAT_B5G5R5A1_UNORM) {
		copy_b5g5r5a1_tex(gc, cur_texture, data, pitch);

	} else if (gc->global_hook_info->format == DXGI_FORMAT_B5G6R5_UNORM) {
		copy_b5g6r5_tex(gc, cur_texture, data, pitch);
	}
}

static void copy_shmem_tex(struct game_capture *gc)
{
	int cur_texture = gc->shmem_data->last_tex;
	HANDLE mutex = NULL;
	uint32_t pitch;
	int next_texture;
	uint8_t *data;

	if (cur_texture < 0 || cur_texture > 1)
		return;

	next_texture = cur_texture == 1 ? 0 : 1;

	if (object_signalled(gc->texture_mutexes[cur_texture])) {
		mutex = gc->texture_mutexes[cur_texture];

	} else if (object_signalled(gc->texture_mutexes[next_texture])) {
		mutex = gc->texture_mutexes[next_texture];
		cur_texture = next_texture;

	} else {
		return;
	}

	if (gs_texture_map(gc->texture, &data, &pitch)) {
		if (gc->convert_16bit) {
			copy_16bit_tex(gc, cur_texture, data, pitch);

		} else if (pitch == gc->pitch) {
			memcpy(data, gc->texture_buffers[cur_texture],
					pitch * gc->cy);
		} else {
			uint8_t *input = gc->texture_buffers[cur_texture];
			uint32_t best_pitch =
				pitch < gc->pitch ? pitch : gc->pitch;

			for (uint32_t y = 0; y < gc->cy; y++) {
				uint8_t *line_in = input + gc->pitch * y;
				uint8_t *line_out = data + pitch * y;
				memcpy(line_out, line_in, best_pitch);
			}
		}

		gs_texture_unmap(gc->texture);
	}

	ReleaseMutex(mutex);
}

static inline bool is_16bit_format(uint32_t format)
{
	return format == DXGI_FORMAT_B5G5R5A1_UNORM ||
	       format == DXGI_FORMAT_B5G6R5_UNORM;
}

static inline bool init_shmem_capture(struct game_capture *gc)
{
	enum gs_color_format format;

	gc->texture_buffers[0] =
		(uint8_t*)gc->data + gc->shmem_data->tex1_offset;
	gc->texture_buffers[1] =
		(uint8_t*)gc->data + gc->shmem_data->tex2_offset;

	gc->convert_16bit = is_16bit_format(gc->global_hook_info->format);
	format = gc->convert_16bit ?
		GS_BGRA : convert_format(gc->global_hook_info->format);

	obs_enter_graphics();
	gs_texture_destroy(gc->texture);
	gc->texture = gs_texture_create(gc->cx, gc->cy, format, 1, NULL,
			GS_DYNAMIC);
	obs_leave_graphics();

	if (!gc->texture) {
		warn("init_shmem_capture: failed to create texture");
		return false;
	}

	gc->copy_texture = copy_shmem_tex;
	return true;
}

static inline bool init_shtex_capture(struct game_capture *gc)
{
	obs_enter_graphics();
	gs_texture_destroy(gc->texture);
	gc->texture = gs_texture_open_shared(gc->shtex_data->tex_handle);
	obs_leave_graphics();

	if (!gc->texture) {
		warn("init_shtex_capture: failed to open shared handle");
		return false;
	}

	return true;
}

static bool start_capture(struct game_capture *gc)
{
	if (!init_events(gc)) {
		return false;
	}
	if (gc->global_hook_info->type == CAPTURE_TYPE_MEMORY) {
		if (!init_shmem_capture(gc)) {
			return false;
		}
	} else {
		if (!init_shtex_capture(gc)) {
			return false;
		}
	}

	return true;
}

static inline bool capture_valid(struct game_capture *gc)
{
	if (!gc->dwm_capture && !IsWindow(gc->window))
	       return false;
	
	return !object_signalled(gc->target_process);
}

static void game_capture_tick(void *data, float seconds)
{
	struct game_capture *gc = data;

	if (gc->hook_stop && object_signalled(gc->hook_stop)) {
		stop_capture(gc);
	}

	if (gc->active && !gc->hook_ready && gc->process_id) {
		gc->hook_ready = get_event_plus_id(EVENT_HOOK_READY,
				gc->process_id);
	}

	if (gc->injector_process && object_signalled(gc->injector_process)) {
		DWORD exit_code = 0;

		GetExitCodeProcess(gc->injector_process, &exit_code);
		close_handle(&gc->injector_process);

		if (exit_code != 0) {
			warn("inject process failed: %ld", (long)exit_code);
			gc->error_acquiring = true;

		} else if (!gc->capturing) {
			gc->retry_interval = ERROR_RETRY_INTERVAL;
			stop_capture(gc);
		}
	}

	if (gc->hook_ready && object_signalled(gc->hook_ready)) {
		enum capture_result result = init_capture_data(gc);

		if (result == CAPTURE_SUCCESS)
			gc->capturing = start_capture(gc);

		if (result != CAPTURE_RETRY && !gc->capturing) {
			gc->retry_interval = ERROR_RETRY_INTERVAL;
			stop_capture(gc);
		}
	}

	gc->retry_time += seconds;

	if (!gc->active) {
		if (!obs_source_showing(gc->source)) {
			gc->retry_time = 0.0f;
			return;
		}

		if (!gc->error_acquiring &&
		    gc->retry_time > gc->retry_interval) {
			if (gc->config.capture_any_fullscreen ||
			    gc->activate_hook) {
				try_hook(gc);
				gc->retry_time = 0.0f;
			}
		}
	} else {
		if (!capture_valid(gc)) {
			info("capture window no longer exists, "
			     "terminating capture");
			stop_capture(gc);
		} else {
			if (gc->copy_texture) {
				obs_enter_graphics();
				gc->copy_texture(gc);
				obs_leave_graphics();
			}

			if (gc->config.cursor) {
				obs_enter_graphics();
				cursor_capture(&gc->cursor_data);
				obs_leave_graphics();
			}

			gc->fps_reset_time += seconds;
			if (gc->fps_reset_time >= gc->retry_interval) {
				reset_frame_interval(gc);
				gc->fps_reset_time = 0.0f;
			}
		}
	}
}

static inline void game_capture_render_cursor(struct game_capture *gc)
{
	POINT p = {0};

	if (!gc->global_hook_info->window ||
	    !gc->global_hook_info->base_cx ||
	    !gc->global_hook_info->base_cy)
		return;

	ClientToScreen((HWND)(uintptr_t)gc->global_hook_info->window, &p);

	float x_scale = (float)gc->global_hook_info->cx /
		(float)gc->global_hook_info->base_cx;
	float y_scale = (float)gc->global_hook_info->cy /
		(float)gc->global_hook_info->base_cy;

	cursor_draw(&gc->cursor_data, -p.x, -p.y, x_scale, y_scale,
			gc->global_hook_info->base_cx,
			gc->global_hook_info->base_cy);
}

static void game_capture_render(void *data, gs_effect_t *effect)
{
	struct game_capture *gc = data;
	if (!gc->texture)
		return;

	effect = obs_get_base_effect(gc->config.allow_transparency ?
			OBS_EFFECT_DEFAULT : OBS_EFFECT_OPAQUE);

	while (gs_effect_loop(effect, "Draw")) {
		obs_source_draw(gc->texture, 0, 0, 0, 0,
				gc->global_hook_info->flip);

		if (gc->config.allow_transparency && gc->config.cursor) {
			game_capture_render_cursor(gc);
		}
	}

	if (!gc->config.allow_transparency && gc->config.cursor) {
		effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

		while (gs_effect_loop(effect, "Draw")) {
			game_capture_render_cursor(gc);
		}
	}
}

static uint32_t game_capture_width(void *data)
{
	struct game_capture *gc = data;
	return gc->active ? gc->global_hook_info->cx : 0;
}

static uint32_t game_capture_height(void *data)
{
	struct game_capture *gc = data;
	return gc->active ? gc->global_hook_info->cy : 0;
}

static const char *game_capture_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_GAME_CAPTURE;
}

static void game_capture_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, SETTING_ANY_FULLSCREEN, true);
	obs_data_set_default_int(settings, SETTING_WINDOW_PRIORITY,
			(int)WINDOW_PRIORITY_EXE);
	obs_data_set_default_bool(settings, SETTING_COMPATIBILITY, false);
	obs_data_set_default_bool(settings, SETTING_FORCE_SCALING, false);
	obs_data_set_default_bool(settings, SETTING_CURSOR, true);
	obs_data_set_default_bool(settings, SETTING_TRANSPARENCY, false);
	obs_data_set_default_string(settings, SETTING_SCALE_RES, "0x0");
	obs_data_set_default_bool(settings, SETTING_LIMIT_FRAMERATE, false);
	obs_data_set_default_bool(settings, SETTING_CAPTURE_OVERLAYS, false);
	obs_data_set_default_bool(settings, SETTING_ANTI_CHEAT_HOOK, true);
}

static bool any_fullscreen_callback(obs_properties_t *ppts,
		obs_property_t *p, obs_data_t *settings)
{
	bool any_fullscreen = obs_data_get_bool(settings,
			SETTING_ANY_FULLSCREEN);

	p = obs_properties_get(ppts, SETTING_CAPTURE_WINDOW);
	obs_property_set_enabled(p, !any_fullscreen);

	p = obs_properties_get(ppts, SETTING_WINDOW_PRIORITY);
	obs_property_set_enabled(p, !any_fullscreen);

	return true;
}

static bool use_scaling_callback(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	bool use_scale = obs_data_get_bool(settings, SETTING_FORCE_SCALING);

	p = obs_properties_get(ppts, SETTING_SCALE_RES);
	obs_property_set_enabled(p, use_scale);
	return true;
}

static void insert_preserved_val(obs_property_t *p, const char *val)
{
	char *class = NULL;
	char *title = NULL;
	char *executable = NULL;
	struct dstr desc = {0};

	build_window_strings(val, &class, &title, &executable);

	dstr_printf(&desc, "[%s]: %s", executable, title);
	obs_property_list_insert_string(p, 1, desc.array, val);
	obs_property_list_item_disable(p, 1, true);

	dstr_free(&desc);
	bfree(class);
	bfree(title);
	bfree(executable);
}

static bool window_changed_callback(obs_properties_t *ppts, obs_property_t *p,
		obs_data_t *settings)
{
	const char *cur_val;
	bool match = false;
	size_t i = 0;

	cur_val = obs_data_get_string(settings, SETTING_CAPTURE_WINDOW);
	if (!cur_val) {
		return false;
	}

	for (;;) {
		const char *val = obs_property_list_item_string(p, i++);
		if (!val)
			break;

		if (strcmp(val, cur_val) == 0) {
			match = true;
			break;
		}
	}

	if (cur_val && *cur_val && !match) {
		insert_preserved_val(p, cur_val);
		return true;
	}

	UNUSED_PARAMETER(ppts);
	return false;
}

static const double default_scale_vals[] = {
	1.25,
	1.5,
	2.0,
	2.5,
	3.0
};

#define NUM_DEFAULT_SCALE_VALS \
	(sizeof(default_scale_vals) / sizeof(default_scale_vals[0]))

static BOOL CALLBACK EnumFirstMonitor(HMONITOR monitor, HDC hdc,
		LPRECT rc, LPARAM data)
{
	*(HMONITOR*)data = monitor;

	UNUSED_PARAMETER(hdc);
	UNUSED_PARAMETER(rc);
	return false;
}

static obs_properties_t *game_capture_properties(void *data)
{
	HMONITOR monitor;
	uint32_t cx = 1920;
	uint32_t cy = 1080;

	/* scaling is free form, this is mostly just to provide some common
	 * values */
	bool success = !!EnumDisplayMonitors(NULL, NULL, EnumFirstMonitor,
			(LPARAM)&monitor);
	if (success) {
		MONITORINFO mi = {0};
		mi.cbSize = sizeof(mi);

		if (!!GetMonitorInfo(monitor, &mi)) {
			cx = (uint32_t)(mi.rcMonitor.right - mi.rcMonitor.left);
			cy = (uint32_t)(mi.rcMonitor.bottom - mi.rcMonitor.top);
		}
	}

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_bool(ppts, SETTING_ANY_FULLSCREEN,
			TEXT_ANY_FULLSCREEN);

	obs_property_set_modified_callback(p, any_fullscreen_callback);

	p = obs_properties_add_list(ppts, SETTING_CAPTURE_WINDOW, TEXT_WINDOW,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "", "");
	fill_window_list(p, INCLUDE_MINIMIZED);

	obs_property_set_modified_callback(p, window_changed_callback);

	p = obs_properties_add_list(ppts, SETTING_WINDOW_PRIORITY,
			TEXT_MATCH_PRIORITY, OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, TEXT_MATCH_TITLE, WINDOW_PRIORITY_TITLE);
	obs_property_list_add_int(p, TEXT_MATCH_CLASS, WINDOW_PRIORITY_CLASS);
	obs_property_list_add_int(p, TEXT_MATCH_EXE,   WINDOW_PRIORITY_EXE);

	obs_properties_add_bool(ppts, SETTING_COMPATIBILITY,
			TEXT_SLI_COMPATIBILITY);

	p = obs_properties_add_bool(ppts, SETTING_FORCE_SCALING,
			TEXT_FORCE_SCALING);

	obs_property_set_modified_callback(p, use_scaling_callback);

	p = obs_properties_add_list(ppts, SETTING_SCALE_RES, TEXT_SCALE_RES,
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);

	for (size_t i = 0; i < NUM_DEFAULT_SCALE_VALS; i++) {
		char scale_str[64];
		uint32_t new_cx =
			(uint32_t)((double)cx / default_scale_vals[i]) & ~2;
		uint32_t new_cy =
			(uint32_t)((double)cy / default_scale_vals[i]) & ~2;

		sprintf(scale_str, "%"PRIu32"x%"PRIu32, new_cx, new_cy);

		obs_property_list_add_string(p, scale_str, scale_str);
	}

	obs_property_set_enabled(p, false);

	obs_properties_add_bool(ppts, SETTING_TRANSPARENCY,
			TEXT_ALLOW_TRANSPARENCY);

	obs_properties_add_bool(ppts, SETTING_LIMIT_FRAMERATE,
			TEXT_LIMIT_FRAMERATE);

	obs_properties_add_bool(ppts, SETTING_CURSOR, TEXT_CAPTURE_CURSOR);

	obs_properties_add_bool(ppts, SETTING_ANTI_CHEAT_HOOK,
			TEXT_ANTI_CHEAT_HOOK);

	obs_properties_add_bool(ppts, SETTING_CAPTURE_OVERLAYS,
			TEXT_CAPTURE_OVERLAYS);

	UNUSED_PARAMETER(data);
	return ppts;
}


struct obs_source_info game_capture_info = {
	.id = "game_capture",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW |
	                OBS_SOURCE_DO_NOT_DUPLICATE,
	.get_name = game_capture_name,
	.create = game_capture_create,
	.destroy = game_capture_destroy,
	.get_width = game_capture_width,
	.get_height = game_capture_height,
	.get_defaults = game_capture_defaults,
	.get_properties = game_capture_properties,
	.update = game_capture_update,
	.video_tick = game_capture_tick,
	.video_render = game_capture_render
};
