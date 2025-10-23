/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#pragma once

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <vector>
#include <string>
#include <memory>
#include <optional>

#include <util/base.h>
#include <util/platform.h>
#include <util/windows/win-version.h>
#include <graphics/matrix3.h>
#include <graphics/matrix4.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>
#include <util/dstr.h>
#include <util/util.hpp>

#include <windows.h>
#include <winternl.h>
#include <dxgi1_6.h>
#include <shellscalingapi.h>
#include <d3dkmthk.h>

/* ------------------------------------------------------------------------- */

#define VERT_IN_OUT \
	"\
struct VertInOut { \
	float4 pos : POSITION; \
}; "

#define NV12_Y_PS \
	VERT_IN_OUT "\
float main(VertInOut vert_in) : TARGET \
{ \
	return 1.0; \
}"

#define NV12_UV_PS \
	VERT_IN_OUT "\
float2 main(VertInOut vert_in) : TARGET \
{ \
	return float2(1.0, 1.0); \
}"

#define NV12_VS \
	VERT_IN_OUT "\
VertInOut main(VertInOut vert_in) \
{ \
	VertInOut vert_out; \
	vert_out.pos = float4(vert_in.pos.xyz, 1.0); \
	return vert_out; \
} "

/* ------------------------------------------------------------------------- */

#define NV12_CX 128
#define NV12_CY 128

struct mat4float {
	float mat[16];
};

static inline uint32_t GetWinVer()
{
	struct win_version_info ver;
	get_win_ver(&ver);

	return (ver.major << 8) | ver.minor;
}

static inline DXGI_FORMAT ConvertGSTextureFormatResource(gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
	case GS_A8:
		return DXGI_FORMAT_A8_UNORM;
	case GS_R8:
		return DXGI_FORMAT_R8_UNORM;
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case GS_R10G10B10A2:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case GS_RGBA16:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case GS_R16:
		return DXGI_FORMAT_R16_UNORM;
	case GS_RGBA16F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case GS_RGBA32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case GS_RG16F:
		return DXGI_FORMAT_R16G16_FLOAT;
	case GS_RG32F:
		return DXGI_FORMAT_R32G32_FLOAT;
	case GS_R16F:
		return DXGI_FORMAT_R16_FLOAT;
	case GS_R32F:
		return DXGI_FORMAT_R32_FLOAT;
	case GS_DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case GS_DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case GS_DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case GS_R8G8:
		return DXGI_FORMAT_R8G8_UNORM;
	case GS_RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case GS_RG16:
		return DXGI_FORMAT_R16G16_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT get_copy_compare_format(gs_color_format format)
{
	switch (format) {
	case GS_RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case GS_BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case GS_BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline DXGI_FORMAT ConvertGSTextureFormatView(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline DXGI_FORMAT ConvertGSTextureFormatViewLinear(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline gs_color_format ConvertDXGITextureFormat(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_A8_UNORM:
		return GS_A8;
	case DXGI_FORMAT_R8_UNORM:
		return GS_R8;
	case DXGI_FORMAT_R8G8_UNORM:
		return GS_R8G8;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return GS_RGBA;
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return GS_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return GS_RGBA16;
	case DXGI_FORMAT_R16_UNORM:
		return GS_R16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return GS_RGBA32F;
	case DXGI_FORMAT_R16G16_FLOAT:
		return GS_RG16F;
	case DXGI_FORMAT_R32G32_FLOAT:
		return GS_RG32F;
	case DXGI_FORMAT_R16_FLOAT:
		return GS_R16F;
	case DXGI_FORMAT_R32_FLOAT:
		return GS_R32F;
	case DXGI_FORMAT_BC1_UNORM:
		return GS_DXT1;
	case DXGI_FORMAT_BC2_UNORM:
		return GS_DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return GS_DXT5;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return GS_RGBA_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return GS_BGRX_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return GS_BGRA_UNORM;
	case DXGI_FORMAT_R16G16_UNORM:
		return GS_RG16;
	}

	return GS_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSZStencilFormat(gs_zstencil_format format)
{
	switch (format) {
	case GS_ZS_NONE:
		return DXGI_FORMAT_UNKNOWN;
	case GS_Z16:
		return DXGI_FORMAT_D16_UNORM;
	case GS_Z24_S8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case GS_Z32F:
		return DXGI_FORMAT_D32_FLOAT;
	case GS_Z32F_S8X24:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static bool FastClearSupported(UINT vendorId, uint64_t version)
{
	/* Always true for non-NVIDIA GPUs */
	if (vendorId != 0x10de)
		return true;

	const uint16_t aa = (version >> 48) & 0xffff;
	const uint16_t bb = (version >> 32) & 0xffff;
	const uint16_t ccccc = (version >> 16) & 0xffff;
	const uint16_t ddddd = version & 0xffff;

	/* Check for NVIDIA driver version >= 31.0.15.2737 */
	return aa >= 31 && bb >= 0 && ccccc >= 15 && ddddd >= 2737;
}

static enum gs_color_format get_swap_format_from_space(gs_color_space space, gs_color_format sdr_format)
{
	gs_color_format format = sdr_format;
	switch (space) {
	case GS_CS_SRGB_16F:
	case GS_CS_709_SCRGB:
		format = GS_RGBA16F;
	}

	return format;
}

struct UnsupportedHWError : HRError {
	inline UnsupportedHWError(const char *str, HRESULT hr) : HRError(str, hr) {}
};

/* exception-safe RAII wrapper for vertex buffer data (NOTE: not copy-safe) */
struct VBDataPtr {
	gs_vb_data *data;

	inline VBDataPtr(gs_vb_data *data) : data(data) {}
	inline ~VBDataPtr() { gs_vbdata_destroy(data); }
};

/* exception-safe RAII wrapper for index buffer data (NOTE: not copy-safe) */
struct DataPtr {
	void *data;

	inline DataPtr(void *data) : data(data) {}
	inline ~DataPtr() { bfree(data); }
};

enum class gs_type {
	gs_vertex_buffer,
	gs_index_buffer,
	gs_texture_2d,
	gs_zstencil_buffer,
	gs_stage_surface,
	gs_sampler_state,
	gs_vertex_shader,
	gs_pixel_shader,
	gs_duplicator,
	gs_swap_chain,
	gs_timer,
	gs_timer_range,
	gs_texture_3d,
};

struct gs_obj {
	gs_device_t *device;
	gs_type obj_type;
	gs_obj *next;
	gs_obj **prev_next;

	inline gs_obj() : device(nullptr), next(nullptr), prev_next(nullptr) {}

	gs_obj(gs_device_t *device, gs_type type);
	virtual ~gs_obj();
};

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

extern std::optional<HagsStatus> GetAdapterHagsStatus(const DXGI_ADAPTER_DESC *desc);

static inline void EnumD3DAdapters(bool (*callback)(void *, const char *, uint32_t), void *param)
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
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

		if (!callback(param, name, i))
			break;
	}
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
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	blog(LOG_INFO, "Available Video Adapters: ");

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
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
