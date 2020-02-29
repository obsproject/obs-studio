#include <windows.h>
#include <strsafe.h>
#include <shlobj.h>
#include <obs-module.h>
#include <util/windows/win-version.h>
#include <util/platform.h>
#include <util/c99defs.h>
#include <util/base.h>

/* ------------------------------------------------------------------------- */
/* helper funcs                                                              */

static bool has_elevation()
{
	SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
	PSID sid = NULL;
	BOOL elevated = false;
	BOOL success;

	success = AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID,
					   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0,
					   0, 0, &sid);
	if (success && sid) {
		CheckTokenMembership(NULL, sid, &elevated);
		FreeSid(sid);
	}

	return elevated;
}

static inline bool file_exists(const wchar_t *path)
{
	WIN32_FIND_DATAW wfd;
	HANDLE h = FindFirstFileW(path, &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return false;
	FindClose(h);
	return true;
}

static LSTATUS get_reg(HKEY hkey, LPCWSTR sub_key, LPCWSTR value_name, bool b64)
{
	HKEY key;
	LSTATUS status;
	DWORD flags = b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
	DWORD size = sizeof(DWORD);
	DWORD val;

	status = RegOpenKeyEx(hkey, sub_key, 0, KEY_READ | flags, &key);
	if (status == ERROR_SUCCESS) {
		status = RegQueryValueExW(key, value_name, NULL, NULL,
					  (LPBYTE)&val, &size);
		RegCloseKey(key);
	}
	return status;
}

#define get_programdata_path(path, subpath)                        \
	do {                                                       \
		SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, \
				 SHGFP_TYPE_CURRENT, path);        \
		StringCbCatW(path, sizeof(path), L"\\");           \
		StringCbCatW(path, sizeof(path), subpath);         \
	} while (false)

#define make_filename(str, name, ext)                                \
	do {                                                         \
		StringCbCatW(str, sizeof(str), name);                \
		StringCbCatW(str, sizeof(str), b64 ? L"64" : L"32"); \
		StringCbCatW(str, sizeof(str), ext);                 \
	} while (false)

/* ------------------------------------------------------------------------- */
/* function to get the path to the hook                                      */

static bool programdata64_hook_exists = false;
static bool programdata32_hook_exists = false;

char *get_hook_path(bool b64)
{
	wchar_t path[MAX_PATH];

	get_programdata_path(path, L"obs-studio-hook\\");
	make_filename(path, L"graphics-hook", L".dll");

	if ((b64 && programdata64_hook_exists) ||
	    (!b64 && programdata32_hook_exists)) {
		char *path_utf8 = NULL;
		os_wcs_to_utf8_ptr(path, 0, &path_utf8);
		return path_utf8;
	}

	return obs_module_file(b64 ? "graphics-hook64.dll"
				   : "graphics-hook32.dll");
}

/* ------------------------------------------------------------------------- */
/* initialization                                                            */

#define IMPLICIT_LAYERS L"SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers"

static bool update_hook_file(bool b64)
{
	wchar_t temp[MAX_PATH];
	wchar_t src[MAX_PATH];
	wchar_t dst[MAX_PATH];
	wchar_t src_json[MAX_PATH];
	wchar_t dst_json[MAX_PATH];

	StringCbCopyW(temp, sizeof(temp),
		      L"..\\..\\data\\obs-plugins\\"
		      L"win-capture\\");
	make_filename(temp, L"obs-vulkan", L".json");

	if (_wfullpath(src_json, temp, MAX_PATH) == NULL)
		return false;

	StringCbCopyW(temp, sizeof(temp),
		      L"..\\..\\data\\obs-plugins\\"
		      L"win-capture\\");
	make_filename(temp, L"graphics-hook", L".dll");

	if (_wfullpath(src, temp, MAX_PATH) == NULL)
		return false;

	get_programdata_path(temp, L"obs-studio-hook\\");
	StringCbCopyW(dst_json, sizeof(dst_json), temp);
	StringCbCopyW(dst, sizeof(dst), temp);
	make_filename(dst_json, L"obs-vulkan", L".json");
	make_filename(dst, L"graphics-hook", L".dll");

	if (!file_exists(src)) {
		return false;
	}
	if (!file_exists(src_json)) {
		return false;
	}
	if (!file_exists(dst) || !file_exists(dst_json)) {
		CreateDirectoryW(temp, NULL);
		if (!CopyFileW(src_json, dst_json, false))
			return false;
		if (!CopyFileW(src, dst, false))
			return false;
		return true;
	}

	struct win_version_info ver_src = {0};
	struct win_version_info ver_dst = {0};
	if (!get_dll_ver(src, &ver_src))
		return false;
	if (!get_dll_ver(dst, &ver_dst))
		return false;

	/* if source is greater than dst, overwrite new file  */
	while (win_version_compare(&ver_dst, &ver_src) < 0) {
		if (!CopyFileW(src_json, dst_json, false))
			return false;
		if (!CopyFileW(src, dst, false))
			return false;

		if (!get_dll_ver(dst, &ver_dst))
			return false;
	}

	/* do not use if major version incremented in target compared to
	 * ours */
	if (ver_dst.major > ver_src.major) {
		return false;
	}

	return true;
}

#define warn(format, ...) \
	blog(LOG_WARNING, "%s: " format, "[Vulkan Capture Init]", ##__VA_ARGS__)

/* Sets vulkan layer registry if it doesn't already exist */
static void init_vulkan_registry(bool b64)
{
	HKEY key = NULL;
	LSTATUS s;

	wchar_t path[MAX_PATH];
	get_programdata_path(path, L"obs-studio-hook\\");
	make_filename(path, L"obs-vulkan", L".json");

	s = get_reg(HKEY_LOCAL_MACHINE, IMPLICIT_LAYERS, path, b64);

	if (s == ERROR_FILE_NOT_FOUND) {
		s = get_reg(HKEY_CURRENT_USER, IMPLICIT_LAYERS, path, b64);

		if (s != ERROR_FILE_NOT_FOUND && s != ERROR_SUCCESS) {
			warn("Failed to query registry keys: %d", (int)s);
			goto finish;
		}
	} else if (s != ERROR_SUCCESS) {
		warn("Failed to query registry keys: %d", (int)s);
		goto finish;
	}

	if (s == ERROR_SUCCESS) {
		goto finish;
	}

	HKEY type = has_elevation() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	DWORD flags = b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;
	DWORD temp;

	s = RegCreateKeyExW(type, IMPLICIT_LAYERS, 0, NULL, 0,
			    KEY_WRITE | flags, NULL, &key, &temp);
	if (s != ERROR_SUCCESS) {
		warn("Failed to create registry key");
		goto finish;
	}

	DWORD zero = 0;
	s = RegSetValueExW(key, path, 0, REG_DWORD, (const BYTE *)&zero,
			   sizeof(zero));
	if (s != ERROR_SUCCESS) {
		warn("Failed to set registry value");
	}

finish:
	if (key)
		RegCloseKey(key);
}

void init_hook_files()
{
	if (update_hook_file(true)) {
		programdata64_hook_exists = true;
		init_vulkan_registry(true);
	}
	if (update_hook_file(false)) {
		programdata32_hook_exists = true;
		init_vulkan_registry(false);
	}
}
