/******************************************************************************
    Copyright (C) 2025 by hongqingwan <hongqingwan@obsproject.com>

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

#include <cassert>
#include <cinttypes>
#include <optional>
#include <util/base.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/util.hpp>
#include <graphics/matrix3.h>
#include <winternl.h>
#include "d3d12-subsystem.hpp"
#include <shellscalingapi.h>
#include <d3dkmthk.h>

struct UnsupportedHWError : HRError {
	inline UnsupportedHWError(const char *str, HRESULT hr) : HRError(str, hr) {}
};

static inline void LogD3D12ErrorDetails(HRError error, gs_device_t *device)
{
	if (error.hr == DXGI_ERROR_DEVICE_REMOVED) {
		HRESULT DeviceRemovedReason = device->device->GetDeviceRemovedReason();
		blog(LOG_ERROR, "  Device Removed Reason: %08lX", DeviceRemovedReason);
	}
}

gs_obj::gs_obj(gs_device_t *device_, gs_type type) : device(device_), obj_type(type)
{
	prev_next = &device->first_obj;
	next = device->first_obj;
	device->first_obj = this;
	if (next)
		next->prev_next = &next;
}

gs_obj::~gs_obj()
{
	if (prev_next)
		*prev_next = next;
	if (next)
		next->prev_next = prev_next;
}



struct HagsStatus {
	enum DriverSupport { ALWAYS_OFF, ALWAYS_ON, EXPERIMENTAL, STABLE, UNKNOWN };

	bool enabled;
	bool enabled_by_default;
	DriverSupport support;

	explicit HagsStatus(const D3DKMT_WDDM_2_7_CAPS *caps)
	{
		enabled = caps->HwSchEnabled;
		enabled_by_default = caps->HwSchEnabledByDefault;
		support = caps->HwSchSupported ? DriverSupport::STABLE : DriverSupport::ALWAYS_OFF;
	}

	void SetDriverSupport(const UINT DXGKVal)
	{
		switch (DXGKVal) {
		case DXGK_FEATURE_SUPPORT_ALWAYS_OFF:
			support = ALWAYS_OFF;
			break;
		case DXGK_FEATURE_SUPPORT_ALWAYS_ON:
			support = ALWAYS_ON;
			break;
		case DXGK_FEATURE_SUPPORT_EXPERIMENTAL:
			support = EXPERIMENTAL;
			break;
		case DXGK_FEATURE_SUPPORT_STABLE:
			support = STABLE;
			break;
		default:
			support = UNKNOWN;
		}
	}

	std::string ToString() const
	{
		std::string status = enabled ? "Enabled" : "Disabled";
		status += " (Default: ";
		status += enabled_by_default ? "Yes" : "No";
		status += ", Driver status: ";
		status += DriverSupportToString();
		status += ")";

		return status;
	}

private:
	const char *DriverSupportToString() const
	{
		switch (support) {
		case ALWAYS_OFF:
			return "Unsupported";
		case ALWAYS_ON:
			return "Always On";
		case EXPERIMENTAL:
			return "Experimental";
		case STABLE:
			return "Supported";
		default:
			return "Unknown";
		}
	}
};

static std::optional<HagsStatus> GetAdapterHagsStatus(const DXGI_ADAPTER_DESC *desc)
{
	std::optional<HagsStatus> ret;
	D3DKMT_OPENADAPTERFROMLUID d3dkmt_openluid{};
	d3dkmt_openluid.AdapterLuid = desc->AdapterLuid;

	NTSTATUS res = D3DKMTOpenAdapterFromLuid(&d3dkmt_openluid);
	if (FAILED(res)) {
		blog(LOG_DEBUG, "Failed opening D3DKMT adapter: %x", res);
		return ret;
	}

	D3DKMT_WDDM_2_7_CAPS caps = {};
	D3DKMT_QUERYADAPTERINFO args = {};
	args.hAdapter = d3dkmt_openluid.hAdapter;
	args.Type = KMTQAITYPE_WDDM_2_7_CAPS;
	args.pPrivateDriverData = &caps;
	args.PrivateDriverDataSize = sizeof(caps);
	res = D3DKMTQueryAdapterInfo(&args);

	/* If this still fails we're likely on Windows 10 pre-2004
	 * where HAGS is not supported anyway. */
	if (SUCCEEDED(res)) {
		HagsStatus status(&caps);

		/* Starting with Windows 10 21H2 we can query more detailed
		 * support information (e.g. experimental status).
		 * This Is optional and failure doesn't matter. */
		D3DKMT_WDDM_2_9_CAPS ext_caps = {};
		args.hAdapter = d3dkmt_openluid.hAdapter;
		args.Type = KMTQAITYPE_WDDM_2_9_CAPS;
		args.pPrivateDriverData = &ext_caps;
		args.PrivateDriverDataSize = sizeof(ext_caps);
		res = D3DKMTQueryAdapterInfo(&args);

		if (SUCCEEDED(res))
			status.SetDriverSupport(ext_caps.HwSchSupportState);

		ret = status;
	} else {
		blog(LOG_WARNING, "Failed querying WDDM 2.7 caps: %x", res);
	}

	D3DKMT_CLOSEADAPTER d3dkmt_close = {d3dkmt_openluid.hAdapter};
	res = D3DKMTCloseAdapter(&d3dkmt_close);
	if (FAILED(res)) {
		blog(LOG_DEBUG, "Failed closing D3DKMT adapter %x: %x", d3dkmt_openluid.hAdapter, res);
	}

	return ret;
}


void gs_device::InitFactory() {
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to create DXGIFactory", hr);
}

void gs_device::InitAdapter(uint32_t adapterIdx) {
	HRESULT hr = factory->EnumAdapterByGpuPreference(
		adapterIdx, DXGI_GPU_PREFERENCE::DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to enumerate DXGIAdapter", hr);
}

void gs_device::InitDevice(uint32_t adapterIdx)
{
	std::wstring adapterName;
	DXGI_ADAPTER_DESC desc;
	D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_12_0;
	LARGE_INTEGER umd;
	uint64_t driverVersion = 0;
	HRESULT hr = 0;

	adpIdx = adapterIdx;

	adapterName = (adapter->GetDesc(&desc) == S_OK) ? desc.Description : L"<unknown>";

	BPtr<char> adapterNameUTF8;
	os_wcs_to_utf8_ptr(adapterName.c_str(), 0, &adapterNameUTF8);
	blog(LOG_INFO, "Loading up D3D12 on adapter %s (%" PRIu32 ")", adapterNameUTF8.Get(), adapterIdx);

	hr = adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd);
	if (SUCCEEDED(hr))
		driverVersion = umd.QuadPart;

	hr = D3D12CreateDevice(adapter, levelUsed, IID_PPV_ARGS(&device));
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to create device", hr);

	blog(LOG_INFO, "D3D12 loaded successfully, feature level used: %x", (unsigned int)levelUsed);

}

void gs_device::UpdateZStencilState() {}
void gs_device::UpdateRasterState() {}
void gs_device::UpdateBlendState() {}

void gs_device::LoadVertexBufferData() {}

void gs_device::UpdateViewProjMatrix() {}

void gs_device::FlushOutputViews() {}

gs_device::gs_device(uint32_t adapterIdx) {
	matrix4_identity(&curProjMatrix);
	matrix4_identity(&curViewMatrix);
	matrix4_identity(&curViewProjMatrix);

	memset(&viewport, 0, sizeof(viewport));

	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		//curTextures[i] = NULL;
		//curSamplers[i] = NULL;
	}

	InitFactory();
	InitAdapter(adapterIdx);
	InitDevice(adapterIdx);
	device_set_render_target(this, NULL, NULL);
}

gs_device::~gs_device() {}

const char *device_get_name(void)
{
	return "Direct3D 12";
}

int device_get_type(void)
{
	return 0;
}

const char *device_preprocessor_name(void)
{
	return "_D3D12";
}


static bool GetMonitorTarget(const MONITORINFOEX &info, DISPLAYCONFIG_TARGET_DEVICE_NAME &target)
{
	bool found = false;

	UINT32 numPath, numMode;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPath, &numMode) == ERROR_SUCCESS) {
		std::vector<DISPLAYCONFIG_PATH_INFO> paths(numPath);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(numMode);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPath, paths.data(), &numMode, modes.data(),
				       nullptr) == ERROR_SUCCESS) {
			paths.resize(numPath);
			for (size_t i = 0; i < numPath; ++i) {
				const DISPLAYCONFIG_PATH_INFO &path = paths[i];

				DISPLAYCONFIG_SOURCE_DEVICE_NAME
				source;
				source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
				source.header.size = sizeof(source);
				source.header.adapterId = path.sourceInfo.adapterId;
				source.header.id = path.sourceInfo.id;
				if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
				    wcscmp(info.szDevice, source.viewGdiDeviceName) == 0) {
					target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
					target.header.size = sizeof(target);
					target.header.adapterId = path.sourceInfo.adapterId;
					target.header.id = path.targetInfo.id;
					found = DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS;
					break;
				}
			}
		}
	}

	return found;
}

static bool GetOutputDesc1(IDXGIOutput *const output, DXGI_OUTPUT_DESC1 *desc1)
{
	ComPtr<IDXGIOutput6> output6;
	HRESULT hr = output->QueryInterface(IID_PPV_ARGS(output6.Assign()));
	bool success = SUCCEEDED(hr);
	if (success) {
		hr = output6->GetDesc1(desc1);
		success = SUCCEEDED(hr);
		if (!success) {
			blog(LOG_WARNING, "IDXGIOutput6::GetDesc1 failed: 0x%08lX", hr);
		}
	}

	return success;
}

// Returns true if this is an integrated display panel e.g. the screen attached to tablets or laptops.
static bool IsInternalVideoOutput(const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType)
{
	switch (VideoOutputTechnologyType) {
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
		return TRUE;

	default:
		return FALSE;
	}
}

// Note: Since an hmon can represent multiple monitors while in clone, this function as written will return
//  the value for the internal monitor if one exists, and otherwise the highest clone-path priority.
static HRESULT GetPathInfo(_In_ PCWSTR pszDeviceName, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo)
{
	HRESULT hr = S_OK;
	UINT32 NumPathArrayElements = 0;
	UINT32 NumModeInfoArrayElements = 0;
	DISPLAYCONFIG_PATH_INFO *PathInfoArray = nullptr;
	DISPLAYCONFIG_MODE_INFO *ModeInfoArray = nullptr;

	do {
		// In case this isn't the first time through the loop, delete the buffers allocated
		delete[] PathInfoArray;
		PathInfoArray = nullptr;

		delete[] ModeInfoArray;
		ModeInfoArray = nullptr;

		hr = HRESULT_FROM_WIN32(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements,
								    &NumModeInfoArrayElements));
		if (FAILED(hr)) {
			break;
		}

		PathInfoArray = new (std::nothrow) DISPLAYCONFIG_PATH_INFO[NumPathArrayElements];
		if (PathInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		ModeInfoArray = new (std::nothrow) DISPLAYCONFIG_MODE_INFO[NumModeInfoArrayElements];
		if (ModeInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		hr = HRESULT_FROM_WIN32(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, PathInfoArray,
							   &NumModeInfoArrayElements, ModeInfoArray, nullptr));
	} while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

	INT DesiredPathIdx = -1;

	if (SUCCEEDED(hr)) {
		// Loop through all sources until the one which matches the 'monitor' is found.
		for (UINT PathIdx = 0; PathIdx < NumPathArrayElements; ++PathIdx) {
			DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
			SourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			SourceName.header.size = sizeof(SourceName);
			SourceName.header.adapterId = PathInfoArray[PathIdx].sourceInfo.adapterId;
			SourceName.header.id = PathInfoArray[PathIdx].sourceInfo.id;

			hr = HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&SourceName.header));
			if (SUCCEEDED(hr)) {
				if (wcscmp(pszDeviceName, SourceName.viewGdiDeviceName) == 0) {
					// Found the source which matches this hmonitor. The paths are given in path-priority order
					// so the first found is the most desired, unless we later find an internal.
					if (DesiredPathIdx == -1 ||
					    IsInternalVideoOutput(PathInfoArray[PathIdx].targetInfo.outputTechnology)) {
						DesiredPathIdx = PathIdx;
					}
				}
			}
		}
	}

	if (DesiredPathIdx != -1) {
		*pPathInfo = PathInfoArray[DesiredPathIdx];
	} else {
		hr = E_INVALIDARG;
	}

	delete[] PathInfoArray;
	PathInfoArray = nullptr;

	delete[] ModeInfoArray;
	ModeInfoArray = nullptr;

	return hr;
}

// Overloaded function accepts an HMONITOR and converts to DeviceName
static HRESULT GetPathInfo(HMONITOR hMonitor, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo)
{
	HRESULT hr = S_OK;

	// Get the name of the 'monitor' being requested
	MONITORINFOEXW ViewInfo;
	RtlZeroMemory(&ViewInfo, sizeof(ViewInfo));
	ViewInfo.cbSize = sizeof(ViewInfo);
	if (!GetMonitorInfoW(hMonitor, &ViewInfo)) {
		// Error condition, likely invalid monitor handle, could log error
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	if (SUCCEEDED(hr)) {
		hr = GetPathInfo(ViewInfo.szDevice, pPathInfo);
	}

	return hr;
}

static ULONG GetSdrMaxNits(HMONITOR monitor)
{
	ULONG nits = 80;

	DISPLAYCONFIG_PATH_INFO info;
	if (SUCCEEDED(GetPathInfo(monitor, &info))) {
		const DISPLAYCONFIG_PATH_TARGET_INFO &targetInfo = info.targetInfo;

		DISPLAYCONFIG_SDR_WHITE_LEVEL level;
		DISPLAYCONFIG_DEVICE_INFO_HEADER &header = level.header;
		header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
		header.size = sizeof(level);
		header.adapterId = targetInfo.adapterId;
		header.id = targetInfo.id;
		if (DisplayConfigGetDeviceInfo(&header) == ERROR_SUCCESS)
			nits = (level.SDRWhiteLevel * 80) / 1000;
	}

	return nits;
}

static void PopulateMonitorIds(HMONITOR handle, char *id, char *alt_id, size_t capacity)
{
	MONITORINFOEXA mi;
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfoA(handle, (LPMONITORINFO)&mi)) {
		strcpy_s(alt_id, capacity, mi.szDevice);
		DISPLAY_DEVICEA device;
		device.cb = sizeof(device);
		if (EnumDisplayDevicesA(mi.szDevice, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME)) {
			strcpy_s(id, capacity, device.DeviceID);
		}
	}
}

static constexpr double DoubleTriangleArea(double ax, double ay, double bx, double by, double cx, double cy)
{
	return ax * (by - cy) + bx * (cy - ay) + cx * (ay - by);
}

static inline void LogAdapterMonitors(IDXGIAdapter1 *adapter)
{
	UINT i;
	ComPtr<IDXGIOutput> output;

	for (i = 0; adapter->EnumOutputs(i, &output) == S_OK; ++i) {
		DXGI_OUTPUT_DESC desc;
		if (FAILED(output->GetDesc(&desc)))
			continue;

		unsigned refresh = 0;

		bool target_found = false;
		DISPLAYCONFIG_TARGET_DEVICE_NAME target;

		constexpr size_t id_capacity = 128;
		char id[id_capacity]{};
		char alt_id[id_capacity]{};
		PopulateMonitorIds(desc.Monitor, id, alt_id, id_capacity);

		MONITORINFOEX info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfo(desc.Monitor, &info)) {
			target_found = GetMonitorTarget(info, target);

			DEVMODE mode;
			mode.dmSize = sizeof(mode);
			mode.dmDriverExtra = 0;
			if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
				refresh = mode.dmDisplayFrequency;
			}
		}

		if (!target_found) {
			target.monitorFriendlyDeviceName[0] = 0;
		}

		UINT bits_per_color = 8;
		DXGI_COLOR_SPACE_TYPE type = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		FLOAT primaries[4][2]{};
		double gamut_size = 0.;
		FLOAT min_luminance = 0.f;
		FLOAT max_luminance = 0.f;
		FLOAT max_full_frame_luminance = 0.f;
		DXGI_OUTPUT_DESC1 desc1;
		if (GetOutputDesc1(output, &desc1)) {
			bits_per_color = desc1.BitsPerColor;
			type = desc1.ColorSpace;
			primaries[0][0] = desc1.RedPrimary[0];
			primaries[0][1] = desc1.RedPrimary[1];
			primaries[1][0] = desc1.GreenPrimary[0];
			primaries[1][1] = desc1.GreenPrimary[1];
			primaries[2][0] = desc1.BluePrimary[0];
			primaries[2][1] = desc1.BluePrimary[1];
			primaries[3][0] = desc1.WhitePoint[0];
			primaries[3][1] = desc1.WhitePoint[1];
			gamut_size = DoubleTriangleArea(desc1.RedPrimary[0], desc1.RedPrimary[1], desc1.GreenPrimary[0],
							desc1.GreenPrimary[1], desc1.BluePrimary[0],
							desc1.BluePrimary[1]);
			min_luminance = desc1.MinLuminance;
			max_luminance = desc1.MaxLuminance;
			max_full_frame_luminance = desc1.MaxFullFrameLuminance;
		}

		const char *space = "Unknown";
		switch (type) {
		case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
			space = "RGB_FULL_G22_NONE_P709";
			break;
		case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
			space = "RGB_FULL_G2084_NONE_P2020";
			break;
		default:
			blog(LOG_WARNING, "Unexpected DXGI_COLOR_SPACE_TYPE: %u", (unsigned)type);
		}

		// These are always identical, but you still have to supply both, thanks Microsoft!
		UINT dpiX, dpiY;
		unsigned scaling = 100;
		if (GetDpiForMonitor(desc.Monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) == S_OK) {
			scaling = (unsigned)(dpiX * 100.0f / 96.0f);
		} else {
			dpiX = 0;
		}

		const RECT &rect = desc.DesktopCoordinates;
		const ULONG sdr_white_nits = GetSdrMaxNits(desc.Monitor);

		char *friendly_name;
		os_wcs_to_utf8_ptr(target.monitorFriendlyDeviceName, 0, &friendly_name);

		blog(LOG_INFO,
		     "\t  output %u:\n"
		     "\t    name=%s\n"
		     "\t    pos={%d, %d}\n"
		     "\t    size={%d, %d}\n"
		     "\t    attached=%s\n"
		     "\t    refresh=%u\n"
		     "\t    bits_per_color=%u\n"
		     "\t    space=%s\n"
		     "\t    primaries=[r=(%f, %f), g=(%f, %f), b=(%f, %f), wp=(%f, %f)]\n"
		     "\t    relative_gamut_area=[709=%f, P3=%f, 2020=%f]\n"
		     "\t    sdr_white_nits=%lu\n"
		     "\t    nit_range=[min=%f, max=%f, max_full_frame=%f]\n"
		     "\t    dpi=%u (%u%%)\n"
		     "\t    id=%s\n"
		     "\t    alt_id=%s",
		     i, friendly_name, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		     desc.AttachedToDesktop ? "true" : "false", refresh, bits_per_color, space, primaries[0][0],
		     primaries[0][1], primaries[1][0], primaries[1][1], primaries[2][0], primaries[2][1],
		     primaries[3][0], primaries[3][1], gamut_size / DoubleTriangleArea(.64, .33, .3, .6, .15, .06),
		     gamut_size / DoubleTriangleArea(.68, .32, .265, .69, .15, .060),
		     gamut_size / DoubleTriangleArea(.708, .292, .17, .797, .131, .046), sdr_white_nits, min_luminance,
		     max_luminance, max_full_frame_luminance, dpiX, scaling, id, alt_id);
		bfree(friendly_name);
	}
}


static inline double to_GiB(size_t bytes)
{
	return static_cast<double>(bytes) / (1 << 30);
}

static inline void LogD3DAdapters()
{
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	blog(LOG_INFO, "Available Video Adapters: ");

	hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	for (i = 0; factory->EnumAdapters1(i, adapter.Assign()) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));
		blog(LOG_INFO, "\tAdapter %u: %s", i, name);
		blog(LOG_INFO, "\t  Dedicated VRAM: %" PRIu64 " (%.01f GiB)", desc.DedicatedVideoMemory,
		     to_GiB(desc.DedicatedVideoMemory));
		blog(LOG_INFO, "\t  Shared VRAM:    %" PRIu64 " (%.01f GiB)", desc.SharedSystemMemory,
		     to_GiB(desc.SharedSystemMemory));
		blog(LOG_INFO, "\t  PCI ID:         %x:%.4x", desc.VendorId, desc.DeviceId);

		if (auto hags_support = GetAdapterHagsStatus(&desc)) {
			blog(LOG_INFO, "\t  HAGS Status:    %s", hags_support->ToString().c_str());
		} else {
			blog(LOG_WARNING, "\t  HAGS Status:    Unknown");
		}

		/* driver version */
		LARGE_INTEGER umd;
		hr = adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd);
		if (SUCCEEDED(hr)) {
			const uint64_t version = umd.QuadPart;
			const uint16_t aa = (version >> 48) & 0xffff;
			const uint16_t bb = (version >> 32) & 0xffff;
			const uint16_t ccccc = (version >> 16) & 0xffff;
			const uint16_t ddddd = version & 0xffff;
			blog(LOG_INFO, "\t  Driver Version: %" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16, aa, bb,
			     ccccc, ddddd);
		} else {
			blog(LOG_INFO, "\t  Driver Version: Unknown (0x%X)", (unsigned)hr);
		}

		LogAdapterMonitors(adapter);
	}
}

static void CreateShaderCacheDirectory()
{
	BPtr cachePath = os_get_program_data_path_ptr("obs-studio/shader-cache");

	if (os_mkdirs(cachePath) == MKDIR_ERROR) {
		blog(LOG_WARNING, "Failed to create shader cache directory, "
				  "cache may not be available.");
	}
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	gs_device *device = NULL;
	int errorcode = GS_SUCCESS;

	try {
		blog(LOG_INFO, "---------------------------------");
		blog(LOG_INFO, "Initializing D3D12...");
		LogD3DAdapters();
		CreateShaderCacheDirectory();

		device = new gs_device(adapter);

	} catch (const UnsupportedHWError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str, error.hr);
		errorcode = GS_ERROR_NOT_SUPPORTED;

	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str, error.hr);
		errorcode = GS_ERROR_FAIL;
	}

	*p_device = device;
	return errorcode;
}

void device_destroy(gs_device_t *device) {}

void device_enter_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void device_leave_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void *device_get_device_obj(gs_device_t *device)
{
	return (void *)device;
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device, const struct gs_init_data *data)
{
	/* not implement */
	return nullptr;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	/* not implement */
}

enum gs_color_space device_get_color_space(gs_device_t *device)
{
	/* not implement */
	return gs_color_space::GS_CS_SRGB;
}

void device_update_color_space(gs_device_t *device)
{
	/* not implement */
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	/* not implement */
}

uint32_t device_get_width(const gs_device_t *device)
{
	/* not implement */
	return 0;
}

uint32_t device_get_height(const gs_device_t *device)
{
	/* not implement */
	return 0;
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width, uint32_t height,
				    enum gs_color_format color_format, uint32_t levels, const uint8_t **data,
				    uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size, enum gs_color_format color_format,
					uint32_t levels, const uint8_t **data, uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
				       enum gs_color_format color_format, uint32_t levels, const uint8_t *const *data,
				       uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width, uint32_t height,
				      enum gs_zstencil_format format)
{
	/* not implement */
	return nullptr;
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width, uint32_t height,
					   enum gs_color_format color_format)
{
	/* not implement */
	return nullptr;
}

gs_samplerstate_t *device_samplerstate_create(gs_device_t *device, const struct gs_sampler_info *info)
{
	/* not implement */
	return nullptr;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device, const char *shader_string, const char *file,
					char **error_string)
{
	gs_vertex_shader *shader = NULL;
	try {
		shader = new gs_vertex_shader(device, file, shader_string);

	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_vertexshader_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);

	} catch (const ShaderError &error) {
		const char *buf = (const char *)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR,
		     "device_vertexshader_create (D3D12): "
		     "Compile warnings/errors for %s:\n%s",
		     file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexshader_create (D3D11): %s", error);
	}

	return shader;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device, const char *shader_string, const char *file,
				       char **error_string)
{
	gs_pixel_shader *shader = NULL;
	try {
		shader = new gs_pixel_shader(device, file, shader_string);

	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_pixelshader_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);

	} catch (const ShaderError &error) {
		const char *buf = (const char *)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR,
		     "device_pixelshader_create (D3D12): "
		     "Compiler warnings/errors for %s:\n%s",
		     file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_pixelshader_create (D3D12): %s", error);
	}

	return shader;
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device, struct gs_vb_data *data, uint32_t flags)
{
	gs_vertex_buffer *buffer = NULL;
	try {
		buffer = new gs_vertex_buffer(device, data, flags);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_vertexbuffer_create (D3D11): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexbuffer_create (D3D11): %s", error);
	}

	return buffer;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device, enum gs_index_type type, void *indices, size_t num,
					    uint32_t flags)
{
	gs_index_buffer *buffer = NULL;
	try {
		buffer = new gs_index_buffer(device, type, indices, num, flags);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_indexbuffer_create (D3D11): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return buffer;
}

gs_timer_t *device_timer_create(gs_device_t *device)
{
	/* not implement */
	return nullptr;
}

gs_timer_range_t *device_timer_range_create(gs_device_t *device)
{
	/* not implement */
	return nullptr;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	/* not implement */
	return gs_texture_type::GS_TEXTURE_2D;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	/* not implement */
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	/* not implement */
}

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	/* not implement */
}

void device_load_samplerstate(gs_device_t *device, gs_samplerstate_t *samplerstate, int unit)
{
	/* not implement */
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	/* not implement */
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	/* not implement */
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

gs_shader_t *device_get_vertex_shader(const gs_device_t *device)
{
	return nullptr;
}

gs_shader_t *device_get_pixel_shader(const gs_device_t *device)
{
	return nullptr;
}

gs_texture_t *device_get_render_target(const gs_device_t *device)
{
	return nullptr;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	return nullptr;
}

void device_set_render_target(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil)
{
	/* not implement */
}

void device_set_render_target_with_color_space(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil,
					       enum gs_color_space space)
{
	/* not implement */
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex, int side, gs_zstencil_t *zstencil)
{
	/* not implement */
}

void device_enable_framebuffer_srgb(gs_device_t *device, bool enable)
{
	/* not implement */
}

bool device_framebuffer_srgb_enabled(gs_device_t *device)
{
	return false;
}
void device_copy_texture_region(gs_device_t *device, gs_texture_t *dst, uint32_t dst_x, uint32_t dst_y,
				gs_texture_t *src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	/* not implement */
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst, gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst, gs_texture_t *src)
{
	/* not implement */
}

void device_begin_frame(gs_device_t *device)
{
	/* not implement */
}

void device_begin_scene(gs_device_t *device)
{
	/* not implement */
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode, uint32_t start_vert, uint32_t num_verts)
{
	/* not implement */
}

void device_end_scene(gs_device_t *device)
{
	/* does nothing in D3D11 */
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	/* not implement */
}

void device_clear(gs_device_t *device, uint32_t clear_flags, const struct vec4 *color, float depth, uint8_t stencil)
{
	/* not implement */
}

bool device_is_present_ready(gs_device_t *device)
{
	/* not implement */
	return false;
}

void device_present(gs_device_t *device)
{
	/* not implement */
}

void device_flush(gs_device_t *device)
{
	/* not implement */
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	/* not implement */
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	/* not implement */
	return gs_cull_mode::GS_NEITHER;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_color(gs_device_t *device, bool red, bool green, bool blue, bool alpha)
{
	/* not implement */
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src, enum gs_blend_type dest)
{
	/* not implement */
}

void device_blend_function_separate(gs_device_t *device, enum gs_blend_type src_c, enum gs_blend_type dest_c,
				    enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	/* not implement */
}

void device_blend_op(gs_device_t *device, enum gs_blend_op_type op)
{
	/* not implement */
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	/* not implement */
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side, enum gs_depth_test test)
{
	/* not implement */
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side, enum gs_stencil_op_type fail,
		       enum gs_stencil_op_type zfail, enum gs_stencil_op_type zpass)
{
	/* not implement */
}

void device_set_viewport(gs_device_t *device, int x, int y, int width, int height)
{
	/* not implement */
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect) {}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	/* not implement */
}

void device_ortho(gs_device_t *device, float left, float right, float top, float bottom, float zNear, float zFar) {}

void device_frustum(gs_device_t *device, float left, float right, float top, float bottom, float zNear, float zFar) {}

void device_projection_push(gs_device_t *device) {}

void device_projection_pop(gs_device_t *device) {}

void gs_swapchain_destroy(gs_swapchain_t *swapchain) {}

void gs_texture_destroy(gs_texture_t *tex) {}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	/* not implement */
	return 0;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	/* not implement */
	return 0;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	return GS_BGRA;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	/* not implement */
	return false;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	/* not implement */
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	/* not implement */
	return nullptr;
}

void gs_cubetexture_destroy(gs_texture_t *cubetex) {}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	/* not implement */
	return 0;
}

enum gs_color_format gs_cubetexture_get_color_format(const gs_texture_t *cubetex)
{
	return GS_BGRA;
}

void gs_voltexture_destroy(gs_texture_t *voltex) {}

uint32_t gs_voltexture_get_width(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_height(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_depth(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

enum gs_color_format gs_voltexture_get_color_format(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return GS_UNKNOWN;
}

void gs_stagesurface_destroy(gs_stagesurf_t *stagesurf) {}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf)
{
	return 0;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf)
{
	return 0;
}

enum gs_color_format gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf)
{
	return GS_BGRA;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data, uint32_t *linesize)
{
	/* not implement */
	return true;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	/* not implement */
}

void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	/* not implement */
}

void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	/* not implement */
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer) {
	if (vertbuffer && vertbuffer->device->lastVertexBuffer == vertbuffer)
		vertbuffer->device->lastVertexBuffer = nullptr;
	delete vertbuffer;
}

static inline void gs_vertexbuffer_flush_internal(gs_vertbuffer_t *vertbuffer, const gs_vb_data *data)
{
	size_t num_tex = data->num_tex < vertbuffer->uvBuffers.size() ? data->num_tex : vertbuffer->uvBuffers.size();

	if (!vertbuffer->dynamic) {
		blog(LOG_ERROR, "gs_vertexbuffer_flush: vertex buffer is "
				"not dynamic");
		return;
	}

	if (data->points)
		vertbuffer->FlushBuffer(vertbuffer->vertexBuffer, data->points, sizeof(vec3));

	if (vertbuffer->normalBuffer && data->normals)
		vertbuffer->FlushBuffer(vertbuffer->normalBuffer, data->normals, sizeof(vec3));

	if (vertbuffer->tangentBuffer && data->tangents)
		vertbuffer->FlushBuffer(vertbuffer->tangentBuffer, data->tangents, sizeof(vec3));

	if (vertbuffer->colorBuffer && data->colors)
		vertbuffer->FlushBuffer(vertbuffer->colorBuffer, data->colors, sizeof(uint32_t));

	for (size_t i = 0; i < num_tex; i++) {
		gs_tvertarray &tv = data->tvarray[i];
		vertbuffer->FlushBuffer(vertbuffer->uvBuffers[i], tv.array, tv.width * sizeof(float));
	}
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	gs_vertexbuffer_flush_internal(vertbuffer, vertbuffer->vbd.data);
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer, const gs_vb_data *data)
{
	gs_vertexbuffer_flush_internal(vertbuffer, data);
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	return vertbuffer->vbd.data;
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	delete indexbuffer;
}

static inline void gs_indexbuffer_flush_internal(gs_indexbuffer_t *indexbuffer, const void *data)
{
	HRESULT hr;

	if (!indexbuffer->dynamic)
		return;

	void *index_resource;
	D3D12_RANGE range;
	memset(&range, 0, sizeof(D3D12_RANGE));

        hr = indexbuffer->indexBuffer->Map(0, &range, &index_resource);
	if (FAILED(hr))
		return;

	memcpy(index_resource, data, indexbuffer->num * indexbuffer->indexSize);

	indexbuffer->indexBuffer->Unmap(0, &range);
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	gs_indexbuffer_flush_internal(indexbuffer, indexbuffer->indices.data);
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer, const void *data)
{
	gs_indexbuffer_flush_internal(indexbuffer, data);
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->indices.data;
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->num;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->type;
}

void gs_timer_destroy(gs_timer_t *timer)
{
	/* not implement */
}

void gs_timer_begin(gs_timer_t *timer)
{
	/* not implement */
}

void gs_timer_end(gs_timer_t *timer)
{
	/* not implement */
}

bool gs_timer_get_data(gs_timer_t *timer, uint64_t *ticks)
{
	/* not implement */
	return false;
}

void gs_timer_range_destroy(gs_timer_range_t *range) {}

void gs_timer_range_begin(gs_timer_range_t *range)
{
	/* not implement */
}

void gs_timer_range_end(gs_timer_range_t *range)
{
	/* not implement */
}

bool gs_timer_range_get_data(gs_timer_range_t *range, bool *disjoint, uint64_t *frequency)
{
	/* not implement */
	return false;
}

extern "C" EXPORT bool device_gdi_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_shared_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_nv12_available(gs_device_t *device)
{
	return false;
}

extern "C" EXPORT bool device_p010_available(gs_device_t *device)
{
	return false;
}

extern "C" EXPORT bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	/* not implement */
	return false;
}

extern "C" EXPORT void device_debug_marker_begin(gs_device_t *, const char *markername, const float color[4])
{
	/* not implement */
}

extern "C" EXPORT void device_debug_marker_end(gs_device_t *)
{
	/* not implement */
}

extern "C" EXPORT gs_texture_t *device_texture_create_gdi(gs_device_t *device, uint32_t width, uint32_t height)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT void *gs_texture_get_dc(gs_texture_t *tex)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT void gs_texture_release_dc(gs_texture_t *tex)
{
	/* not implement */
}

extern "C" EXPORT gs_texture_t *device_texture_open_shared(gs_device_t *device, uint32_t handle)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT gs_texture_t *device_texture_open_nt_shared(gs_device_t *device, uint32_t handle)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT uint32_t device_texture_get_shared_handle(gs_texture_t *tex)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT gs_texture_t *device_texture_wrap_obj(gs_device_t *device, void *obj)
{
	/* not implement */
	return nullptr;
}

int device_texture_acquire_sync(gs_texture_t *tex, uint64_t key, uint32_t ms)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT int device_texture_release_sync(gs_texture_t *tex, uint64_t key)
{
	/* not implement */

	return -1;
}

extern "C" EXPORT bool device_texture_create_nv12(gs_device_t *device, gs_texture_t **p_tex_y, gs_texture_t **p_tex_uv,
						  uint32_t width, uint32_t height, uint32_t flags)
{
	/* not implement */
	return true;
}

extern "C" EXPORT bool device_texture_create_p010(gs_device_t *device, gs_texture_t **p_tex_y, gs_texture_t **p_tex_uv,
						  uint32_t width, uint32_t height, uint32_t flags)
{
	/* not implement */
	return true;
}

extern "C" EXPORT gs_stagesurf_t *device_stagesurface_create_nv12(gs_device_t *device, uint32_t width, uint32_t height)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT gs_stagesurf_t *device_stagesurface_create_p010(gs_device_t *device, uint32_t width, uint32_t height)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT void device_register_loss_callbacks(gs_device_t *device, const gs_device_loss *callbacks)
{
	/* not implement */
}

extern "C" EXPORT void device_unregister_loss_callbacks(gs_device_t *device, void *data)
{
	/* not implement */
}

uint32_t gs_get_adapter_count(void)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT bool device_can_adapter_fast_clear(gs_device_t *device)
{
	/* not implement */
	return false;
}
