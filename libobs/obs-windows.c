/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
******************************************************************************/

#include "util/windows/win-version.h"
#include "util/platform.h"
#include "util/dstr.h"
#include "obs.h"
#include "obs-internal.h"

#include <windows.h>

static uint32_t win_ver = 0;

const char *get_module_extension(void)
{
	return ".dll";
}

#ifdef _WIN64
#define BIT_STRING "64bit"
#else
#define BIT_STRING "32bit"
#endif

static const char *module_bin[] = {
	"obs-plugins/" BIT_STRING,
	"../../obs-plugins/" BIT_STRING,
};

static const char *module_data[] = {
	"data/%module%",
	"../../data/obs-plugins/%module%"
};

static const int module_patterns_size =
	sizeof(module_bin)/sizeof(module_bin[0]);

void add_default_module_paths(void)
{
	for (int i = 0; i < module_patterns_size; i++)
		obs_add_module_path(module_bin[i], module_data[i]);
}

/* on windows, points to [base directory]/data/libobs */
char *find_libobs_data_file(const char *file)
{
	struct dstr path;
	dstr_init(&path);

	if (check_path(file, "data/libobs/", &path))
		return path.array;

	if (check_path(file, "../../data/libobs/", &path))
		return path.array;

	dstr_free(&path);
	return NULL;
}

static void log_processor_info(void)
{
	HKEY    key;
	wchar_t data[1024];
	char    *str = NULL;
	DWORD   size, speed;
	LSTATUS status;

	memset(data, 0, 1024);

	status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
			L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
			&key);
	if (status != ERROR_SUCCESS)
		return;

	size = 1024;
	status = RegQueryValueExW(key, L"ProcessorNameString", NULL, NULL,
			(LPBYTE)data, &size);
	if (status == ERROR_SUCCESS) {
		os_wcs_to_utf8_ptr(data, 0, &str);
		blog(LOG_INFO, "CPU Name: %s", str);
		bfree(str);
	}

	size = sizeof(speed);
	status = RegQueryValueExW(key, L"~MHz", NULL, NULL, (LPBYTE)&speed,
			&size);
	if (status == ERROR_SUCCESS)
		blog(LOG_INFO, "CPU Speed: %ldMHz", speed);

	RegCloseKey(key);
}

static void log_processor_cores(void)
{
	blog(LOG_INFO, "Physical Cores: %d, Logical Cores: %d",
			os_get_physical_cores(), os_get_logical_cores());
}

static void log_available_memory(void)
{
	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(ms);

	GlobalMemoryStatusEx(&ms);

#ifdef _WIN64
	const char *note = "";
#else
	const char *note = " (NOTE: 32bit programs cannot use more than 3gb)";
#endif

	blog(LOG_INFO, "Physical Memory: %luMB Total, %luMB Free%s",
			(DWORD)(ms.ullTotalPhys / 1048576),
			(DWORD)(ms.ullAvailPhys / 1048576),
			note);
}

static void log_windows_version(void)
{
	struct win_version_info ver;
	get_win_ver(&ver);

	bool b64 = is_64_bit_windows();
	const char *windows_bitness = b64 ? "64" : "32";

	blog(LOG_INFO, "Windows Version: %d.%d Build %d (revision: %d; %s-bit)",
			ver.major, ver.minor, ver.build, ver.revis,
			windows_bitness);
}

static void log_admin_status(void)
{
	SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
	PSID admin_group;
	BOOL success;

	success = AllocateAndInitializeSid(&auth, 2,
			SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0, &admin_group);
	if (success) {
		if (!CheckTokenMembership(NULL, admin_group, &success))
			success = false;
		FreeSid(admin_group);
	}

	blog(LOG_INFO, "Running as administrator: %s",
			success ? "true" : "false");
}

typedef HRESULT (WINAPI *dwm_is_composition_enabled_t)(BOOL*);

static void log_aero(void)
{
	dwm_is_composition_enabled_t composition_enabled = NULL;

	const char *aeroMessage = win_ver >= 0x602 ?
		" (Aero is always on for windows 8 and above)" : "";

	HMODULE dwm = LoadLibraryW(L"dwmapi");
	BOOL bComposition = true;

	if (!dwm) {
		return;
	}

	composition_enabled = (dwm_is_composition_enabled_t)GetProcAddress(dwm,
			"DwmIsCompositionEnabled");
	if (!composition_enabled) {
		return;
	}

	composition_enabled(&bComposition);
	blog(LOG_INFO, "Aero is %s%s", bComposition ? "Enabled" : "Disabled",
			aeroMessage);
}

void log_system_info(void)
{
	struct win_version_info ver;
	get_win_ver(&ver);

	win_ver = (ver.major << 8) | ver.minor;

	log_processor_info();
	log_processor_cores();
	log_available_memory();
	log_windows_version();
	log_admin_status();
	log_aero();
}


struct obs_hotkeys_platform {
	int vk_codes[OBS_KEY_LAST_VALUE];
};

static int get_virtual_key(obs_key_t key)
{
	switch (key) {
	case OBS_KEY_RETURN: return VK_RETURN;
	case OBS_KEY_ESCAPE: return VK_ESCAPE;
	case OBS_KEY_TAB: return VK_TAB;
	case OBS_KEY_BACKTAB: return VK_OEM_BACKTAB;
	case OBS_KEY_BACKSPACE: return VK_BACK;
	case OBS_KEY_INSERT: return VK_INSERT;
	case OBS_KEY_DELETE: return VK_DELETE;
	case OBS_KEY_PAUSE: return VK_PAUSE;
	case OBS_KEY_PRINT: return VK_SNAPSHOT;
	case OBS_KEY_CLEAR: return VK_CLEAR;
	case OBS_KEY_HOME: return VK_HOME;
	case OBS_KEY_END: return VK_END;
	case OBS_KEY_LEFT: return VK_LEFT;
	case OBS_KEY_UP: return VK_UP;
	case OBS_KEY_RIGHT: return VK_RIGHT;
	case OBS_KEY_DOWN: return VK_DOWN;
	case OBS_KEY_PAGEUP: return VK_PRIOR;
	case OBS_KEY_PAGEDOWN: return VK_NEXT;

	case OBS_KEY_SHIFT: return VK_SHIFT;
	case OBS_KEY_CONTROL: return VK_CONTROL;
	case OBS_KEY_ALT: return VK_MENU;
	case OBS_KEY_CAPSLOCK: return VK_CAPITAL;
	case OBS_KEY_NUMLOCK: return VK_NUMLOCK;
	case OBS_KEY_SCROLLLOCK: return VK_SCROLL;

	case OBS_KEY_F1: return VK_F1;
	case OBS_KEY_F2: return VK_F2;
	case OBS_KEY_F3: return VK_F3;
	case OBS_KEY_F4: return VK_F4;
	case OBS_KEY_F5: return VK_F5;
	case OBS_KEY_F6: return VK_F6;
	case OBS_KEY_F7: return VK_F7;
	case OBS_KEY_F8: return VK_F8;
	case OBS_KEY_F9: return VK_F9;
	case OBS_KEY_F10: return VK_F10;
	case OBS_KEY_F11: return VK_F11;
	case OBS_KEY_F12: return VK_F12;
	case OBS_KEY_F13: return VK_F13;
	case OBS_KEY_F14: return VK_F14;
	case OBS_KEY_F15: return VK_F15;
	case OBS_KEY_F16: return VK_F16;
	case OBS_KEY_F17: return VK_F17;
	case OBS_KEY_F18: return VK_F18;
	case OBS_KEY_F19: return VK_F19;
	case OBS_KEY_F20: return VK_F20;
	case OBS_KEY_F21: return VK_F21;
	case OBS_KEY_F22: return VK_F22;
	case OBS_KEY_F23: return VK_F23;
	case OBS_KEY_F24: return VK_F24;

	case OBS_KEY_SPACE: return VK_SPACE;

	case OBS_KEY_APOSTROPHE: return VK_OEM_7;
	case OBS_KEY_PLUS: return VK_OEM_PLUS;
	case OBS_KEY_COMMA: return VK_OEM_COMMA;
	case OBS_KEY_MINUS: return VK_OEM_MINUS;
	case OBS_KEY_PERIOD: return VK_OEM_PERIOD;
	case OBS_KEY_SLASH: return VK_OEM_2;
	case OBS_KEY_0: return '0';
	case OBS_KEY_1: return '1';
	case OBS_KEY_2: return '2';
	case OBS_KEY_3: return '3';
	case OBS_KEY_4: return '4';
	case OBS_KEY_5: return '5';
	case OBS_KEY_6: return '6';
	case OBS_KEY_7: return '7';
	case OBS_KEY_8: return '8';
	case OBS_KEY_9: return '9';
	case OBS_KEY_NUMASTERISK: return VK_MULTIPLY;
	case OBS_KEY_NUMPLUS: return VK_ADD;
	case OBS_KEY_NUMMINUS: return VK_SUBTRACT;
	case OBS_KEY_NUMPERIOD: return VK_DECIMAL;
	case OBS_KEY_NUMSLASH: return VK_DIVIDE;
	case OBS_KEY_NUM0: return VK_NUMPAD0;
	case OBS_KEY_NUM1: return VK_NUMPAD1;
	case OBS_KEY_NUM2: return VK_NUMPAD2;
	case OBS_KEY_NUM3: return VK_NUMPAD3;
	case OBS_KEY_NUM4: return VK_NUMPAD4;
	case OBS_KEY_NUM5: return VK_NUMPAD5;
	case OBS_KEY_NUM6: return VK_NUMPAD6;
	case OBS_KEY_NUM7: return VK_NUMPAD7;
	case OBS_KEY_NUM8: return VK_NUMPAD8;
	case OBS_KEY_NUM9: return VK_NUMPAD9;
	case OBS_KEY_SEMICOLON: return VK_OEM_1;
	case OBS_KEY_A: return 'A';
	case OBS_KEY_B: return 'B';
	case OBS_KEY_C: return 'C';
	case OBS_KEY_D: return 'D';
	case OBS_KEY_E: return 'E';
	case OBS_KEY_F: return 'F';
	case OBS_KEY_G: return 'G';
	case OBS_KEY_H: return 'H';
	case OBS_KEY_I: return 'I';
	case OBS_KEY_J: return 'J';
	case OBS_KEY_K: return 'K';
	case OBS_KEY_L: return 'L';
	case OBS_KEY_M: return 'M';
	case OBS_KEY_N: return 'N';
	case OBS_KEY_O: return 'O';
	case OBS_KEY_P: return 'P';
	case OBS_KEY_Q: return 'Q';
	case OBS_KEY_R: return 'R';
	case OBS_KEY_S: return 'S';
	case OBS_KEY_T: return 'T';
	case OBS_KEY_U: return 'U';
	case OBS_KEY_V: return 'V';
	case OBS_KEY_W: return 'W';
	case OBS_KEY_X: return 'X';
	case OBS_KEY_Y: return 'Y';
	case OBS_KEY_Z: return 'Z';
	case OBS_KEY_BRACKETLEFT: return VK_OEM_4;
	case OBS_KEY_BACKSLASH: return VK_OEM_5;
	case OBS_KEY_BRACKETRIGHT: return VK_OEM_6;
	case OBS_KEY_ASCIITILDE: return VK_OEM_3;

	case OBS_KEY_KANJI: return VK_KANJI;
	case OBS_KEY_TOUROKU: return VK_OEM_FJ_TOUROKU;
	case OBS_KEY_MASSYO: return VK_OEM_FJ_MASSHOU;

	case OBS_KEY_HANGUL: return VK_HANGUL;

	case OBS_KEY_MOUSE1: return VK_LBUTTON;
	case OBS_KEY_MOUSE2: return VK_RBUTTON;
	case OBS_KEY_MOUSE3: return VK_MBUTTON;
	case OBS_KEY_MOUSE4: return VK_XBUTTON1;
	case OBS_KEY_MOUSE5: return VK_XBUTTON2;

	/* TODO: Implement keys for non-US keyboards */
	default:;
	}
	return 0;
}

bool obs_hotkeys_platform_init(struct obs_core_hotkeys *hotkeys)
{
	hotkeys->platform_context = bzalloc(sizeof(obs_hotkeys_platform_t));

	for (size_t i = 0; i < OBS_KEY_LAST_VALUE; i++)
		hotkeys->platform_context->vk_codes[i] = get_virtual_key(i);

	return true;
}

void obs_hotkeys_platform_free(struct obs_core_hotkeys *hotkeys)
{
	bfree(hotkeys->platform_context);
	hotkeys->platform_context = NULL;
}

static bool vk_down(DWORD vk)
{
	short state = GetAsyncKeyState(vk);
	bool down = (state & 0x8000) != 0;

	return down;
}

bool obs_hotkeys_platform_is_pressed(obs_hotkeys_platform_t *context,
		obs_key_t key)
{
	if (key == OBS_KEY_META) {
		return vk_down(VK_LWIN) || vk_down(VK_RWIN);
	}

	UNUSED_PARAMETER(context);
	return vk_down(obs_key_to_virtual_key(key));
}

void obs_key_to_str(obs_key_t key, struct dstr *str)
{
	wchar_t name[128] = L"";
	UINT scan_code;
	int vk;

	if (key == OBS_KEY_NONE) {
		return;

	} else if (key >= OBS_KEY_MOUSE1 && key <= OBS_KEY_MOUSE29) {
		if (obs->hotkeys.translations[key]) {
			dstr_copy(str, obs->hotkeys.translations[key]);
		} else {
			dstr_printf(str, "Mouse %d",
					(int)(key - OBS_KEY_MOUSE1 + 1));
		}
		return;

	} if (key == OBS_KEY_PAUSE) {
		dstr_copy(str, obs_get_hotkey_translation(key, "Pause"));
		return;

	} else if (key == OBS_KEY_META) {
		dstr_copy(str, obs_get_hotkey_translation(key, "Windows"));
		return;
	}

	vk = obs_key_to_virtual_key(key);
	scan_code = MapVirtualKey(vk, 0) << 16;

	switch (vk) {
	case VK_HOME:
	case VK_END:
	case VK_LEFT:
	case VK_UP:
	case VK_RIGHT:
	case VK_DOWN:
	case VK_PRIOR:
	case VK_NEXT:
	case VK_INSERT:
	case VK_DELETE:
	case VK_NUMLOCK:
		scan_code |= 0x01000000;
	}

	if (scan_code != 0 && GetKeyNameTextW(scan_code, name, 128) != 0) {
		dstr_from_wcs(str, name);
	} else if (key != OBS_KEY_NONE) {
		dstr_copy(str, obs_key_to_name(key));
	}
}

obs_key_t obs_key_from_virtual_key(int code)
{
	obs_hotkeys_platform_t *platform = obs->hotkeys.platform_context;

	for (size_t i = 0; i < OBS_KEY_LAST_VALUE; i++) {
		if (platform->vk_codes[i] == code) {
			return (obs_key_t)i;
		}
	}

	return OBS_KEY_NONE;
}

int obs_key_to_virtual_key(obs_key_t key)
{
	if (key == OBS_KEY_META)
		return VK_LWIN;

	return obs->hotkeys.platform_context->vk_codes[(int)key];
}

static inline void add_combo_key(obs_key_t key, struct dstr *str)
{
	struct dstr key_str = {0};

	obs_key_to_str(key, &key_str);

	if (!dstr_is_empty(&key_str)) {
		if (!dstr_is_empty(str)) {
			dstr_cat(str, " + ");
		}
		dstr_cat_dstr(str, &key_str);
	}

	dstr_free(&key_str);
}

void obs_key_combination_to_str(obs_key_combination_t combination,
		struct dstr *str)
{
	if ((combination.modifiers & INTERACT_CONTROL_KEY) != 0) {
		add_combo_key(OBS_KEY_CONTROL, str);
	}
	if ((combination.modifiers & INTERACT_COMMAND_KEY) != 0) {
		add_combo_key(OBS_KEY_META, str);
	}
	if ((combination.modifiers & INTERACT_ALT_KEY) != 0) {
		add_combo_key(OBS_KEY_ALT, str);
	}
	if ((combination.modifiers & INTERACT_SHIFT_KEY) != 0) {
		add_combo_key(OBS_KEY_SHIFT, str);
	}
	if (combination.key != OBS_KEY_NONE) {
		add_combo_key(combination.key, str);
	}
}

bool sym_initialize_called = false;

void reset_win32_symbol_paths(void)
{
	static BOOL (WINAPI *sym_initialize_w)(HANDLE, const wchar_t*, BOOL);
	static BOOL (WINAPI *sym_set_search_path_w)(HANDLE, const wchar_t*);
	static bool funcs_initialized = false;
	static bool initialize_success = false;

	struct obs_module *module = obs->first_module;
	struct dstr path_str = {0};
	DARRAY(char*) paths;
	wchar_t *path_str_w = NULL;
	char *abspath;

	da_init(paths);

	if (!funcs_initialized) {
		HMODULE mod;
		funcs_initialized = true;

		mod = LoadLibraryW(L"DbgHelp");
		if (!mod)
			return;

		sym_initialize_w = (void*)GetProcAddress(mod, "SymInitializeW");
		sym_set_search_path_w = (void*)GetProcAddress(mod,
				"SymSetSearchPathW");
		if (!sym_initialize_w || !sym_set_search_path_w)
			return;

		initialize_success = true;
	}

	if (!initialize_success)
		return;

	abspath = os_get_abs_path_ptr(".");
	if (abspath)
		da_push_back(paths, &abspath);

	while (module) {
		bool found = false;
		struct dstr path = {0};
		char *path_end;

		dstr_copy(&path, module->bin_path);
		dstr_replace(&path, "/", "\\");

		path_end = strrchr(path.array, '\\');
		if (!path_end) {
			module = module->next;
			dstr_free(&path);
			continue;
		}

		*path_end = 0;

		for (size_t i = 0; i < paths.num; i++) {
			const char *existing_path = paths.array[i];
			if (astrcmpi(path.array, existing_path) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			abspath = os_get_abs_path_ptr(path.array);
			if (abspath)
				da_push_back(paths, &abspath);
		}

		dstr_free(&path);

		module = module->next;
	}

	for (size_t i = 0; i < paths.num; i++) {
		const char *path = paths.array[i];
		if (path && *path) {
			if (i != 0)
				dstr_cat(&path_str, ";");
			dstr_cat(&path_str, paths.array[i]);
		}
	}

	if (path_str.array) {
		os_utf8_to_wcs_ptr(path_str.array, path_str.len, &path_str_w);
		if (path_str_w) {
			if (!sym_initialize_called) {
				sym_initialize_w(GetCurrentProcess(),
						path_str_w, false);
				sym_initialize_called = true;
			} else {
				sym_set_search_path_w(GetCurrentProcess(),
						path_str_w);
			}

			bfree(path_str_w);
		}
	}

	for (size_t i = 0; i < paths.num; i++)
		bfree(paths.array[i]);
	dstr_free(&path_str);
	da_free(paths);
}

void initialize_com(void)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
}

void uninitialize_com(void)
{
	CoUninitialize();
}
