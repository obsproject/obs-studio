#include <obs-module.h>
#include <util/platform.h>
#include <util/util.hpp>
#include <strsafe.h>
#include <strmif.h>
#ifdef VIRTUALCAM_AVAILABLE
#include "virtualcam-guid.h"
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-dshow", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows DirectShow source/encoder";
}

extern void RegisterDShowSource();
extern void RegisterDShowEncoders();

#ifdef VIRTUALCAM_AVAILABLE
extern "C" struct obs_output_info virtualcam_info;

static bool vcam_installed(bool b64)
{
	wchar_t cls_str[CHARS_IN_GUID];
	wchar_t temp[MAX_PATH];
	wchar_t path[MAX_PATH];
	DWORD path_size = sizeof(path);
	HKEY key = nullptr;
	const char *bits = b64 ? "64" : "32";

	StringFromGUID2(CLSID_OBS_VirtualVideo, cls_str, CHARS_IN_GUID);
	StringCbPrintf(temp, sizeof(temp), L"CLSID\\%s", cls_str);

	DWORD flags = KEY_READ;
	flags |= b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;

	LSTATUS status = RegOpenKeyExW(HKEY_CLASSES_ROOT, temp, 0, flags, &key);
	if (status != ERROR_SUCCESS) {
		blog(LOG_INFO, "Virtual camera %s-bit filter is not registered", bits);
		return false;
	}

	status = RegGetValueW(key, L"InprocServer32", nullptr, RRF_RT_REG_SZ, nullptr, path, &path_size);
	RegCloseKey(key);

	if (status == ERROR_SUCCESS) {
		BPtr<char> path_utf8;
		os_wcs_to_utf8_ptr(path, 0, &path_utf8);
		blog(LOG_INFO, "Virtual camera %s-bit filter is registered: %s", bits, (const char *)path_utf8);
	} else {
		blog(LOG_INFO, "Virtual camera %s-bit filter is registered but its path could not be read", bits);
	}

	return true;
}
#endif

bool obs_module_load(void)
{
	RegisterDShowSource();
	RegisterDShowEncoders();
#ifdef VIRTUALCAM_AVAILABLE
	bool installed32 = vcam_installed(false);
	bool installed64 = vcam_installed(true);

	if (installed32 || installed64) {
		obs_register_output(&virtualcam_info);
	} else {
		blog(LOG_WARNING, "Virtual camera output is unavailable, no virtual camera filter is registered");
	}
#endif

	return true;
}
