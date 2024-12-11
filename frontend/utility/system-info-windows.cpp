#include "system-info.hpp"

#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/win-registry.h>
#include <util/windows/win-version.h>

#include <cinttypes>

#include <dxgi.h>
#include <shlobj.h>

static std::optional<std::vector<GoLiveApi::Gpu>> system_gpu_data()
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		return std::nullopt;

	std::vector<GoLiveApi::Gpu> adapter_info;

	for (i = 0; factory->EnumAdapters1(i, adapter.Assign()) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";
		char driver_version[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));

		GoLiveApi::Gpu data;
		data.model = name;

		data.vendor_id = desc.VendorId;
		data.device_id = desc.DeviceId;

		data.dedicated_video_memory = desc.DedicatedVideoMemory;
		data.shared_system_memory = desc.SharedSystemMemory;

		/* driver version */
		LARGE_INTEGER umd;
		hr = adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd);
		if (SUCCEEDED(hr)) {
			const uint64_t version = umd.QuadPart;
			const uint16_t aa = (version >> 48) & 0xffff;
			const uint16_t bb = (version >> 32) & 0xffff;
			const uint16_t ccccc = (version >> 16) & 0xffff;
			const uint16_t ddddd = version & 0xffff;
			snprintf(driver_version, sizeof(driver_version), "%" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16,
				 aa, bb, ccccc, ddddd);
			data.driver_version = driver_version;
		}

		adapter_info.push_back(data);
	}

	return adapter_info;
}

static void get_processor_info(char **name, DWORD *speed)
{
	HKEY key;
	wchar_t data[1024];
	DWORD size;
	LSTATUS status;

	memset(data, 0, sizeof(data));

	status = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", &key);
	if (status != ERROR_SUCCESS)
		return;

	size = sizeof(data);
	status = RegQueryValueExW(key, L"ProcessorNameString", NULL, NULL, (LPBYTE)data, &size);
	if (status == ERROR_SUCCESS) {
		os_wcs_to_utf8_ptr(data, 0, name);
	} else {
		*name = 0;
	}

	size = sizeof(*speed);
	status = RegQueryValueExW(key, L"~MHz", NULL, NULL, (LPBYTE)speed, &size);
	if (status != ERROR_SUCCESS)
		*speed = 0;

	RegCloseKey(key);
}

#define WIN10_GAME_BAR_REG_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"
#define WIN10_GAME_DVR_POLICY_REG_KEY L"SOFTWARE\\Policies\\Microsoft\\Windows\\GameDVR"
#define WIN10_GAME_DVR_REG_KEY L"System\\GameConfigStore"
#define WIN10_GAME_MODE_REG_KEY L"Software\\Microsoft\\GameBar"
#define WIN10_HAGS_REG_KEY L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"

static std::optional<GoLiveApi::GamingFeatures> get_gaming_features_data(const win_version_info &ver)
{
	uint32_t win_ver = (ver.major << 8) | ver.minor;
	if (win_ver < 0xA00)
		return std::nullopt;

	GoLiveApi::GamingFeatures gaming_features;

	struct feature_mapping_s {
		std::optional<bool> *field;
		HKEY hkey;
		LPCWSTR sub_key;
		LPCWSTR value_name;
		LPCWSTR backup_value_name;
		bool non_existence_is_false;
		DWORD disabled_value;
	};
	struct feature_mapping_s features[] = {
		{&gaming_features.game_bar_enabled, HKEY_CURRENT_USER, WIN10_GAME_BAR_REG_KEY, L"AppCaptureEnabled", 0,
		 false, 0},
		{&gaming_features.game_dvr_allowed, HKEY_CURRENT_USER, WIN10_GAME_DVR_POLICY_REG_KEY, L"AllowGameDVR",
		 0, false, 0},
		{&gaming_features.game_dvr_enabled, HKEY_CURRENT_USER, WIN10_GAME_DVR_REG_KEY, L"GameDVR_Enabled", 0,
		 false, 0},
		{&gaming_features.game_dvr_bg_recording, HKEY_CURRENT_USER, WIN10_GAME_BAR_REG_KEY,
		 L"HistoricalCaptureEnabled", 0, false, 0},
		{&gaming_features.game_mode_enabled, HKEY_CURRENT_USER, WIN10_GAME_MODE_REG_KEY, L"AutoGameModeEnabled",
		 L"AllowAutoGameMode", false, 0},
		{&gaming_features.hags_enabled, HKEY_LOCAL_MACHINE, WIN10_HAGS_REG_KEY, L"HwSchMode", 0, true, 1}};

	for (int i = 0; i < sizeof(features) / sizeof(*features); ++i) {
		struct reg_dword info;

		get_reg_dword(features[i].hkey, features[i].sub_key, features[i].value_name, &info);

		if (info.status != ERROR_SUCCESS && features[i].backup_value_name) {
			get_reg_dword(features[i].hkey, features[i].sub_key, features[i].backup_value_name, &info);
		}

		if (info.status == ERROR_SUCCESS) {
			*features[i].field = info.return_value != features[i].disabled_value;
		} else if (features[i].non_existence_is_false) {
			*features[i].field = false;
		}
	}

	return gaming_features;
}

static inline bool get_reg_sz(HKEY key, const wchar_t *val, wchar_t *buf, DWORD size)
{
	const LSTATUS status = RegGetValueW(key, NULL, val, RRF_RT_REG_SZ, NULL, buf, &size);
	return status == ERROR_SUCCESS;
}

#define MAX_SZ_LEN 256
#define WINVER_REG_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"

static char win_release_id[MAX_SZ_LEN] = "unavailable";

static inline void get_reg_ver(struct win_version_info *ver)
{
	HKEY key;
	DWORD size, dw_val;
	LSTATUS status;
	wchar_t str[MAX_SZ_LEN];

	status = RegOpenKeyW(HKEY_LOCAL_MACHINE, WINVER_REG_KEY, &key);
	if (status != ERROR_SUCCESS)
		return;

	size = sizeof(dw_val);

	status = RegQueryValueExW(key, L"CurrentMajorVersionNumber", NULL, NULL, (LPBYTE)&dw_val, &size);
	if (status == ERROR_SUCCESS)
		ver->major = (int)dw_val;

	status = RegQueryValueExW(key, L"CurrentMinorVersionNumber", NULL, NULL, (LPBYTE)&dw_val, &size);
	if (status == ERROR_SUCCESS)
		ver->minor = (int)dw_val;

	status = RegQueryValueExW(key, L"UBR", NULL, NULL, (LPBYTE)&dw_val, &size);
	if (status == ERROR_SUCCESS)
		ver->revis = (int)dw_val;

	if (get_reg_sz(key, L"CurrentBuildNumber", str, sizeof(str))) {
		ver->build = wcstol(str, NULL, 10);
	}

	const wchar_t *release_key = ver->build > 19041 ? L"DisplayVersion" : L"ReleaseId";
	if (get_reg_sz(key, release_key, str, sizeof(str))) {
		os_wcs_to_utf8(str, 0, win_release_id, MAX_SZ_LEN);
	}

	RegCloseKey(key);
}

void system_info(GoLiveApi::Capabilities &capabilities)
{
	char tmpstr[1024];

	capabilities.gpu = system_gpu_data();

	{
		auto &cpu_data = capabilities.cpu;
		cpu_data.physical_cores = os_get_physical_cores();
		cpu_data.logical_cores = os_get_logical_cores();
		DWORD processorSpeed;
		char *processorName;
		get_processor_info(&processorName, &processorSpeed);
		if (processorSpeed)
			cpu_data.speed = processorSpeed;
		if (processorName)
			cpu_data.name = processorName;
		bfree(processorName);
	}

	{
		auto &memory_data = capabilities.memory;
		memory_data.total = os_get_sys_total_size();
		memory_data.free = os_get_sys_free_size();
	}

	struct win_version_info ver;
	get_win_ver(&ver);
	get_reg_ver(&ver);

	// Gaming features
	capabilities.gaming_features = get_gaming_features_data(ver);

	{
		auto &system_data = capabilities.system;

		snprintf(tmpstr, sizeof(tmpstr), "%d.%d", ver.major, ver.minor);

		system_data.version = tmpstr;
		system_data.name = "Windows";
		system_data.build = ver.build;
		system_data.release = win_release_id;
		system_data.revision = ver.revis;
		system_data.bits = is_64_bit_windows() ? 64 : 32;
		system_data.arm = is_arm64_windows();
		system_data.armEmulation = os_get_emulation_status();
	}
}
