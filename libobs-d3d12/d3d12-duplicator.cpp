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

#include "d3d12-subsystem.hpp"
#include <unordered_map>

static inline bool get_monitor(IDXGIAdapter1 *adaptor, int monitor_idx, IDXGIOutput **dxgiOutput)
{
	HRESULT hr;

	hr = adaptor->EnumOutputs(monitor_idx, dxgiOutput);
	if (FAILED(hr)) {
		if (hr == DXGI_ERROR_NOT_FOUND)
			return false;

		throw HRError("Failed to get output", hr);
	}

	return true;
}

void gs_duplicator::Start()
{
	ComPtr<IDXGIOutput5> output5;
	ComPtr<IDXGIOutput1> output1;
	ComPtr<IDXGIOutput> output;
	HRESULT hr;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	hr = factory->EnumAdapters1(idx, &adapter);
	if (FAILED(hr))
		throw HRError("Failed to enumerate DXGIAdapter", hr);

	hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
			       device11.Assign(), NULL, context11.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create D3D11 Device", hr);

	if (!get_monitor(adapter, idx, output.Assign()))
		throw "Invalid monitor index";

	hr = output->QueryInterface(IID_PPV_ARGS(output5.Assign()));
	hdr = false;
	sdr_white_nits = 80.f;
	if (SUCCEEDED(hr)) {
		constexpr DXGI_FORMAT supportedFormats[]{
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_B8G8R8A8_UNORM,
		};
		hr = output5->DuplicateOutput1(device11, 0, _countof(supportedFormats), supportedFormats,
					       duplicator.Assign());
		if (FAILED(hr))
			throw HRError("Failed to DuplicateOutput1", hr);
		DXGI_OUTPUT_DESC desc;
		if (SUCCEEDED(output->GetDesc(&desc))) {
			gs_monitor_color_info info = device->GetMonitorColorInfo(desc.Monitor);
			hdr = info.hdr;
			sdr_white_nits = (float)info.sdr_white_nits;
		}
	} else {
		hr = output->QueryInterface(IID_PPV_ARGS(output1.Assign()));
		if (FAILED(hr))
			throw HRError("Failed to query IDXGIOutput1", hr);

		hr = output1->DuplicateOutput(device11, duplicator.Assign());
		if (FAILED(hr))
			throw HRError("Failed to DuplicateOutput", hr);
	}
}

gs_duplicator::gs_duplicator(gs_device_t *device_, int monitor_idx)
	: gs_obj(device_, gs_type::gs_duplicator),
	  texture(nullptr),
	  idx(monitor_idx),
	  refs(1),
	  updated(false)
{
	Start();
}

gs_duplicator::~gs_duplicator()
{
	delete texture;
}

extern "C" {

EXPORT bool device_get_duplicator_monitor_info(gs_device_t *device, int monitor_idx, struct gs_monitor_info *info)
{
	DXGI_OUTPUT_DESC desc;

	try {
		ComPtr<IDXGIOutput> output;
		HRESULT hr;
		ComPtr<IDXGIFactory1> factory;
		ComPtr<IDXGIAdapter1> adapter;
		hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		if (FAILED(hr))
			throw HRError("Failed to create DXGIFactory", hr);

		hr = factory->EnumAdapters1(monitor_idx, &adapter);
		if (FAILED(hr))
			throw HRError("Failed to enumerate DXGIAdapter", hr);
		if (!get_monitor(adapter, monitor_idx, output.Assign()))
			return false;

		hr = output->GetDesc(&desc);
		if (FAILED(hr))
			throw HRError("GetDesc failed", hr);

	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_get_duplicator_monitor_info: "
		     "%s (%08lX)",
		     error.str, error.hr);
		return false;
	}

	switch (desc.Rotation) {
	case DXGI_MODE_ROTATION_UNSPECIFIED:
	case DXGI_MODE_ROTATION_IDENTITY:
		info->rotation_degrees = 0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		info->rotation_degrees = 90;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		info->rotation_degrees = 180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		info->rotation_degrees = 270;
		break;
	}

	info->x = desc.DesktopCoordinates.left;
	info->y = desc.DesktopCoordinates.top;
	info->cx = desc.DesktopCoordinates.right - info->x;
	info->cy = desc.DesktopCoordinates.bottom - info->y;

	return true;
}

EXPORT int device_duplicator_get_monitor_index(gs_device_t *device, void *monitor)
{
	const HMONITOR handle = (HMONITOR)monitor;

	int index = -1;

	UINT output = 0;
	while (index == -1) {
		IDXGIOutput *pOutput;
		const HRESULT hr = device->d3d12Instance->GetAdapter()->EnumOutputs(output, &pOutput);
		if (hr == DXGI_ERROR_NOT_FOUND)
			break;

		if (SUCCEEDED(hr)) {
			DXGI_OUTPUT_DESC desc;
			if (SUCCEEDED(pOutput->GetDesc(&desc))) {
				if (desc.Monitor == handle)
					index = output;
			} else {
				blog(LOG_ERROR,
				     "device_duplicator_get_monitor_index: "
				     "Failed to get desc (%08lX)",
				     hr);
			}

			pOutput->Release();
		} else if (hr == DXGI_ERROR_NOT_FOUND) {
			blog(LOG_ERROR,
			     "device_duplicator_get_monitor_index: "
			     "Failed to get output (%08lX)",
			     hr);
		}

		++output;
	}

	return index;
}

static std::unordered_map<int, gs_duplicator *> instances;

void reset_duplicators(void)
{
	for (std::pair<const int, gs_duplicator *> &pair : instances) {
		pair.second->updated = false;
	}
}

EXPORT gs_duplicator_t *device_duplicator_create(gs_device_t *device, int monitor_idx)
{
	gs_duplicator *duplicator = nullptr;

	const auto it = instances.find(monitor_idx);
	if (it != instances.end()) {
		duplicator = it->second;
		duplicator->refs++;
		return duplicator;
	}

	try {
		duplicator = new gs_duplicator(device, monitor_idx);
		instances[monitor_idx] = duplicator;

	} catch (const char *error) {
		blog(LOG_DEBUG, "device_duplicator_create: %s", error);
		return nullptr;

	} catch (const HRError &error) {
		blog(LOG_DEBUG, "device_duplicator_create: %s (%08lX)", error.str, error.hr);
		return nullptr;
	}

	return duplicator;
}

EXPORT void gs_duplicator_destroy(gs_duplicator_t *duplicator)
{
	if (--duplicator->refs == 0) {
		instances.erase(duplicator->idx);
		delete duplicator;
	}
}

static HANDLE GetSharedHandle(IDXGIResource *dxgi_res)
{
	HANDLE handle;
	HRESULT hr;

	hr = dxgi_res->GetSharedHandle(&handle);
	if (FAILED(hr)) {
		blog(LOG_WARNING,
		     "GetSharedHandle: Failed to "
		     "get shared handle: %08lX",
		     hr);
		return nullptr;
	} else {
		return handle;
	}
}

static void CreateSharedTexture(gs_duplicator_t *d, D3D11_TEXTURE2D_DESC &desc, gs_color_format general_format)
{
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.Format = ConvertGSTextureFormatResource(general_format);
	HRESULT hr = d->device11->CreateTexture2D(&desc, NULL, d->texShared.Assign());

	if (FAILED(hr)) {
		blog(LOG_WARNING,
		     "CreateSharedTexture: Failed to "
		     "create shared texture: %08lX",
		     hr);
		return;
	}
	ComPtr<IDXGIResource> dxgi_res;

	d->texShared->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);

	hr = d->texShared->QueryInterface(__uuidof(IDXGIResource), (void **)&dxgi_res);
	if (FAILED(hr)) {
		blog(LOG_WARNING,
		     "InitTexture: Failed to query "
		     "interface: %08lX",
		     hr);
	} else {
		d->texSharedHandle = GetSharedHandle(dxgi_res);

		hr = d->texShared->QueryInterface(__uuidof(IDXGIKeyedMutex), (void **)&d->km);
		if (FAILED(hr)) {
			throw HRError("Failed to query "
				      "IDXGIKeyedMutex",
				      hr);
		}
	}
}

static inline void copy_texture(gs_duplicator_t *d, ID3D11Texture2D *tex)
{
	D3D11_TEXTURE2D_DESC desc;
	tex->GetDesc(&desc);
	const gs_color_format format = ConvertDXGITextureFormat(desc.Format);
	const gs_color_format general_format = gs_generalize_format(format);

	if (!d->texture || (gs_texture_get_width(d->texture) != desc.Width) ||
	    (gs_texture_get_height(d->texture) != desc.Height) || (d->texture->format != general_format)) {

		gs_texture_destroy(d->texture);
		d->texShared = nullptr;
		if (d->km) {
			d->km->ReleaseSync(0);
			d->km.Release();
			d->km = nullptr;
		}

		if (d->texSharedHandle) {
			CloseHandle(d->texSharedHandle);
			d->texSharedHandle = 0;
		}

		CreateSharedTexture(d, desc, general_format);
		ComPtr<ID3D12Resource> d3d12res;
		HRESULT hr = d->device->d3d12Instance->GetDevice()->OpenSharedHandle(
			d->texSharedHandle, __uuidof(ID3D12Resource), (void **)d3d12res.Assign());
		if (SUCCEEDED(hr)) {
			d->texture = gs_texture_wrap_obj(d3d12res.Get());
		}

		d->color_space =
			d->hdr ? GS_CS_709_SCRGB
			       : ((desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT) ? GS_CS_SRGB_16F : GS_CS_SRGB);
	}

	if (d->texShared) {
		d->km->AcquireSync(0, INFINITE);
		d->context11->CopyResource(d->texShared, tex);
		d->context11->Flush();
		d->km->ReleaseSync(0);
	}
}

EXPORT bool gs_duplicator_update_frame(gs_duplicator_t *d)
{
	DXGI_OUTDUPL_FRAME_INFO info;
	ComPtr<ID3D11Texture2D> tex;
	ComPtr<IDXGIResource> res;
	HRESULT hr;

	if (!d->duplicator) {
		return false;
	}
	if (d->updated) {
		return true;
	}

	hr = d->duplicator->AcquireNextFrame(0, &info, res.Assign());
	if (hr == DXGI_ERROR_ACCESS_LOST) {
		return false;

	} else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return true;

	} else if (FAILED(hr)) {
		blog(LOG_ERROR,
		     "gs_duplicator_update_frame: Failed to update "
		     "frame (%08lX)",
		     hr);
		return true;
	}

	hr = res->QueryInterface(__uuidof(ID3D11Texture2D), (void **)tex.Assign());
	if (FAILED(hr)) {
		blog(LOG_ERROR,
		     "gs_duplicator_update_frame: Failed to query "
		     "ID3D11Texture2D (%08lX)",
		     hr);
		d->duplicator->ReleaseFrame();
		return true;
	}

	copy_texture(d, tex);
	d->duplicator->ReleaseFrame();
	d->updated = true;
	return true;
}

EXPORT gs_texture_t *gs_duplicator_get_texture(gs_duplicator_t *duplicator)
{
	return duplicator->texture;
}

EXPORT enum gs_color_space gs_duplicator_get_color_space(gs_duplicator_t *duplicator)
{
	return duplicator->color_space;
}

EXPORT float gs_duplicator_get_sdr_white_level(gs_duplicator_t *duplicator)
{
	return duplicator->sdr_white_nits;
}
}
