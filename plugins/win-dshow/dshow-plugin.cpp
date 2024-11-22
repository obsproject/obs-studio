#include <obs-module.h>
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
	HKEY key = nullptr;

	StringFromGUID2(CLSID_OBS_VirtualVideo, cls_str, CHARS_IN_GUID);
	StringCbPrintf(temp, sizeof(temp), L"CLSID\\%s", cls_str);

	DWORD flags = KEY_READ;
	flags |= b64 ? KEY_WOW64_64KEY : KEY_WOW64_32KEY;

	LSTATUS status = RegOpenKeyExW(HKEY_CLASSES_ROOT, temp, 0, flags, &key);
	if (status != ERROR_SUCCESS) {
		return false;
	}

	RegCloseKey(key);
	return true;
}
#endif

bool obs_module_load(void)
{
	RegisterDShowSource();
	RegisterDShowEncoders();
#ifdef VIRTUALCAM_AVAILABLE
	if (vcam_installed(false))
		obs_register_output(&virtualcam_info);
#endif

	return true;
}
