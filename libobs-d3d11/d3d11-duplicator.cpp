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

#include "d3d11-subsystem.hpp"

#include <obs.h>
#include <util/threading.h>

#include <unordered_map>
#include <process.h>

static inline bool get_monitor(gs_device_t *device, int monitor_idx,
			       IDXGIOutput **dxgiOutput)
{
	HRESULT hr;

	hr = device->adapter->EnumOutputs(monitor_idx, dxgiOutput);
	if (FAILED(hr)) {
		if (hr == DXGI_ERROR_NOT_FOUND)
			return false;

		throw HRError("Failed to get output", hr);
	}

	return true;
}

static inline bool copy_texture(gs_duplicator_t *d, ID3D11Texture2D *tex)
{
	D3D11_TEXTURE2D_DESC desc;
	tex->GetDesc(&desc);
	const gs_color_format format = ConvertDXGITextureFormat(desc.Format);
	const gs_color_format general_format = gs_generalize_format(format);

	bool success = true;

	gs_duplicator_frame *const worker_frame = d->worker_frame;
	if (!worker_frame->worker_tex || (worker_frame->width != desc.Width) ||
	    (worker_frame->height != desc.Height) ||
	    (worker_frame->format != general_format)) {
		worker_frame->shared_km.Clear();
		delete worker_frame->shared_tex;
		worker_frame->shared_tex = nullptr;
		worker_frame->worker_km.Clear();
		worker_frame->worker_tex.Clear();
		worker_frame->height = 0;
		worker_frame->width = 0;

		D3D11_TEXTURE2D_DESC shared_texture_desc;
		shared_texture_desc.Width = desc.Width;
		shared_texture_desc.Height = desc.Height;
		shared_texture_desc.MipLevels = 1;
		shared_texture_desc.ArraySize = 1;
		shared_texture_desc.Format =
			ConvertGSTextureFormatResource(general_format);
		shared_texture_desc.SampleDesc.Count = 1;
		shared_texture_desc.SampleDesc.Quality = 0;
		shared_texture_desc.Usage = D3D11_USAGE_DEFAULT;
		shared_texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		shared_texture_desc.CPUAccessFlags = 0;
		shared_texture_desc.MiscFlags =
			D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

		ComPtr<ID3D11Texture2D> worker_tex;
		ComPtr<IDXGIKeyedMutex> worker_km;

		HRESULT hr = d->worker_device->CreateTexture2D(
			&shared_texture_desc, nullptr, worker_tex.Assign());
		if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "copy_texture: Failed to create "
			     "ID3D11Texture2D (%08lX)",
			     hr);
			return false;
		}

		hr = worker_tex->QueryInterface(worker_km.Assign());
		if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "copy_texture: Failed to query "
			     "IDXGIKeyedMutex (%08lX)",
			     hr);
			return false;
		}

		ComPtr<IDXGIResource> dxgi_res;
		hr = worker_tex->QueryInterface(dxgi_res.Assign());
		if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "copy_texture: Failed to query "
			     "IDXGIResource (%08lX)",
			     hr);
			return false;
		}

		hr = dxgi_res->SetEvictionPriority(
			DXGI_RESOURCE_PRIORITY_MAXIMUM);
		if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "copy_texture: Failed to SetEvictionPriority (%08lX)",
			     hr);
			return false;
		}

		HANDLE shared_handle{};
		hr = dxgi_res->GetSharedHandle(&shared_handle);
		if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "copy_texture: Failed to GetSharedHandle (%08lX)",
			     hr);
			return false;
		}

		worker_frame->width = desc.Width;
		worker_frame->height = desc.Height;
		worker_frame->format = general_format;
		worker_frame->worker_tex = std::move(worker_tex);
		worker_frame->worker_km = std::move(worker_km);
		worker_frame->shared_handle =
			(uint32_t)(uintptr_t)shared_handle;
	}

	IDXGIKeyedMutex *const worker_km = worker_frame->worker_km;
	HRESULT hr = worker_km->AcquireSync(0, 0);
	success = SUCCEEDED(hr);
	if (success) {
		d->worker_context->CopyResource(worker_frame->worker_tex, tex);
		hr = worker_km->ReleaseSync(1);
		success = SUCCEEDED(hr);
		if (success) {
			d->written_frames.push(worker_frame);
			d->worker_frame = nullptr;
		} else {
			blog(LOG_ERROR,
			     "copy_texture: Failed to release sync 1 (%08lX)",
			     hr);
		}
	} else {
		blog(LOG_ERROR,
		     "copy_texture: Failed to acquire sync 0 (%08lX)", hr);
	}

	return success;
}

static bool can_make_progress(gs_duplicator *d, bool &kill_thread)
{
	bool progress = false;

	kill_thread = d->kill_thread;
	if (kill_thread) {
		progress = true;
	} else {
		gs_duplicator_frame *const worker_frame =
			d->available_frames.pop();
		progress = worker_frame != nullptr;
		if (progress)
			d->worker_frame = worker_frame;
	}

	return progress;
}

static uint64_t ms_converttime_os(LONGLONG ms_time)
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (uint64_t)((double)ms_time *
			  (1000000000.0 / (double)frequency.QuadPart));
}

static unsigned __stdcall duplicator_worker(void *data)
{
	os_set_thread_name("d3d11 duplicator_worker");

	gs_duplicator *const d = static_cast<gs_duplicator *>(data);

	for (;;) {
		bool kill_thread = d->kill_thread;

		if (!kill_thread && !d->worker_frame) {
			for (;;) {
				if (can_make_progress(d, kill_thread))
					break;

				const unsigned key =
					d->worker_progress_ec.prepare_wait();

				if (can_make_progress(d, kill_thread))
					break;

				d->worker_progress_ec.wait(key);
			}
		}

		if (kill_thread)
			break;

		if (d->acquired) {
			const HRESULT hr = d->duplicator->ReleaseFrame();
			if (SUCCEEDED(hr)) {
				d->acquired = false;
			} else {
				blog(LOG_ERROR,
				     "duplicator_worker: Failed to release "
				     "frame (%08lX)",
				     hr);

				break;
			}
		}

		DXGI_OUTDUPL_FRAME_INFO info;
		ComPtr<IDXGIResource> res;
		HRESULT hr = d->duplicator->AcquireNextFrame(500, &info,
							     res.Assign());
		if (hr == DXGI_ERROR_ACCESS_LOST) {
			blog(LOG_ERROR,
			     "duplicator_worker: DXGI_ERROR_ACCESS_LOST");

			break;
		} else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
			/* no frame to read */

		} else if (FAILED(hr)) {
			blog(LOG_ERROR,
			     "duplicator_worker: Failed to update "
			     "frame (%08lX)",
			     hr);

			break;
		} else {
			d->acquired = true;

			const LONGLONG ms_present_time =
				info.LastPresentTime.QuadPart;
			if ((ms_present_time != 0) &&
			    (info.TotalMetadataBufferSize > 0)) {
				ComPtr<ID3D11Texture2D> tex;
				hr = res->QueryInterface(
					IID_PPV_ARGS(tex.Assign()));
				if (SUCCEEDED(hr)) {
					d->worker_frame->present_time =
						ms_converttime_os(
							ms_present_time);
					if (!copy_texture(d, tex))
						break;
				} else {
					blog(LOG_ERROR,
					     "duplicator_worker: Failed to query "
					     "ID3D11Texture2D (%08lX)",
					     hr);

					break;
				}
			}
		}
	}

	if (d->acquired) {
		const HRESULT hr = d->duplicator->ReleaseFrame();
		if (SUCCEEDED(hr)) {
			d->acquired = false;
		} else {
			blog(LOG_ERROR,
			     "duplicator_worker: Failed to release "
			     "frame (%08lX)",
			     hr);
		}
	}

	return 0;
}

void gs_duplicator::Start()
{
	updated = false;
	acquired = false;
	graphics_frame = nullptr;
	worker_frame = nullptr;

	kill_thread.store(false, std::memory_order_relaxed);

	while (available_frames.pop())
		;

	while (written_frames.pop())
		;

	pending_frame_count = 0;

	for (gs_duplicator_frame_padded &padded_frame : frame_cache) {
		gs_duplicator_frame *const frame = &padded_frame.frame;
		frame->width = 0;
		frame->height = 0;
		frame->worker_tex.Clear();
		frame->worker_km.Clear();
		delete frame->shared_tex;
		frame->shared_tex = nullptr;
		frame->shared_km.Clear();

		available_frames.push(frame);
	}

	ComPtr<ID3D11Device> temp_device;
	ComPtr<ID3D11DeviceContext> temp_context;
	ComPtr<IDXGIOutputDuplication> temp_duplicator;
	ComPtr<IDXGIOutput5> output5;
	ComPtr<IDXGIOutput1> output1;
	ComPtr<IDXGIOutput> output;

	if (!get_monitor(device, idx, output.Assign()))
		throw "Invalid monitor index";

	constexpr D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_10_0;
	HRESULT hr = D3D11CreateDevice(device->adapter, D3D_DRIVER_TYPE_UNKNOWN,
				       NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
				       featureLevels, _countof(featureLevels),
				       D3D11_SDK_VERSION, temp_device.Assign(),
				       &levelUsed, temp_context.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create worker device/context", hr);

	hr = output->QueryInterface(IID_PPV_ARGS(output5.Assign()));
	if (SUCCEEDED(hr)) {
		constexpr DXGI_FORMAT supportedFormats[]{
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			DXGI_FORMAT_R10G10B10A2_UNORM,
			DXGI_FORMAT_B8G8R8A8_UNORM,
		};
		hr = output5->DuplicateOutput1(temp_device, 0,
					       _countof(supportedFormats),
					       supportedFormats,
					       temp_duplicator.Assign());
		if (FAILED(hr))
			throw HRError("Failed to DuplicateOutput1", hr);
	} else {
		hr = output->QueryInterface(IID_PPV_ARGS(output1.Assign()));
		if (FAILED(hr))
			throw HRError("Failed to query IDXGIOutput1", hr);

		hr = output1->DuplicateOutput(temp_device,
					      temp_duplicator.Assign());
		if (FAILED(hr))
			throw HRError("Failed to DuplicateOutput", hr);
	}

	unsigned unused;
	const uintptr_t temp_worker_thread =
		_beginthreadex(NULL, 0, &duplicator_worker, this, 0, &unused);
	if (temp_worker_thread == UINTPTR_MAX)
		throw HRError("Failed to create worker thread", GetLastError());

	worker_device = std::move(temp_device);
	worker_context = std::move(temp_context);
	duplicator = std::move(temp_duplicator);

	worker_thread = (HANDLE)temp_worker_thread;
}

void gs_duplicator::Release()
{
	kill_thread = true;
	worker_progress_ec.notify();
	WaitForSingleObject(worker_thread, INFINITE);
	CloseHandle(worker_thread);
	kill_thread.store(false, std::memory_order_relaxed);

	pending_frame_count = 0;

	while (written_frames.pop())
		;

	while (available_frames.pop())
		;

	worker_frame = nullptr;
	graphics_frame = nullptr;

	/* Do not destroy shared_tex here. */
	/* Don't want to modify device reset list while reset is happening. */
	for (gs_duplicator_frame_padded &padded_frame : frame_cache) {
		gs_duplicator_frame *const frame = &padded_frame.frame;
		frame->shared_km.Clear();
		frame->worker_km.Clear();
		frame->worker_tex.Clear();
		frame->height = 0;
		frame->width = 0;
	}

	duplicator.Clear();
	worker_context.Clear();
	worker_device.Clear();
}

gs_duplicator::gs_duplicator(gs_device_t *device_, int monitor_idx)
	: gs_obj(device_, gs_type::gs_duplicator), idx(monitor_idx), refs(1)
{
	Start();
}

gs_duplicator::~gs_duplicator()
{
	Release();

	/* Do this outside Release because shared_tex is in device reset list */
	for (gs_duplicator_frame_padded &padded_frame : frame_cache) {
		gs_duplicator_frame *const frame = &padded_frame.frame;
		delete frame->shared_tex;
		frame->shared_tex = nullptr;
	}
}

extern "C" {

EXPORT bool device_get_duplicator_monitor_info(gs_device_t *device,
					       int monitor_idx,
					       struct gs_monitor_info *info)
{
	DXGI_OUTPUT_DESC desc;
	HRESULT hr;

	try {
		ComPtr<IDXGIOutput> output;

		if (!get_monitor(device, monitor_idx, output.Assign()))
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

EXPORT int device_duplicator_get_monitor_index(gs_device_t *device,
					       void *monitor)
{
	const HMONITOR handle = (HMONITOR)monitor;

	int index = -1;

	UINT output = 0;
	while (index == -1) {
		IDXGIOutput *pOutput;
		const HRESULT hr =
			device->adapter->EnumOutputs(output, &pOutput);
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

EXPORT gs_duplicator_t *device_duplicator_create(gs_device_t *device,
						 int monitor_idx)
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
		blog(LOG_DEBUG, "device_duplicator_create: %s (%08lX)",
		     error.str, error.hr);
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

static bool make_shared_resources(gs_duplicator_frame *frame)
{
	gs_texture_2d *shared_tex =
		(gs_texture_2d *)gs_texture_open_shared(frame->shared_handle);
	bool success = shared_tex != nullptr;
	if (success) {
		ComPtr<IDXGIKeyedMutex> shared_km;
		HRESULT hr =
			shared_tex->texture->QueryInterface(shared_km.Assign());
		success = SUCCEEDED(hr);
		if (success) {
			delete frame->shared_tex;
			frame->shared_tex = shared_tex;
			frame->shared_km = std::move(shared_km);
		} else {
			blog(LOG_ERROR,
			     "make_shared_resources: Failed to query "
			     "IDXGIKeyedMutex (%08lX)",
			     hr);
			delete shared_tex;
		}
	} else {
		blog(LOG_ERROR,
		     "make_shared_resources: Failed to open shared texture");
	}

	return success;
}

gs_duplicator_frame *const determine_next_frame(gs_duplicator_t *d)
{
	gs_duplicator_frame *next_frame = nullptr;

	const uint64_t interval = obs_get_frame_interval_ns();
	assert(interval > 0);

	/* not scientific */
	const uint64_t buffer_ns = 4 * interval;

	const uint64_t boundary = obs_get_video_frame_time() - buffer_ns;

	gs_duplicator_queue &written_frames = d->written_frames;
	gs_duplicator_frame **const pending_frames = d->pending_frames;
	size_t pending_frame_count = d->pending_frame_count;

	gs_duplicator_frame *written_frame;
	while ((written_frame = written_frames.pop()) != nullptr) {
		pending_frames[pending_frame_count] = written_frame;
		++pending_frame_count;
	}

	if (pending_frame_count > 0) {
		uint64_t previous_slot =
			(pending_frames[0]->present_time - boundary) / interval;
		size_t write_index = 0;
		for (size_t i = 1; i < pending_frame_count; ++i) {
			gs_duplicator_frame *const frame = pending_frames[i];
			const uint64_t slot =
				(frame->present_time - boundary) / interval;
			if (previous_slot == slot) {
				gs_duplicator_frame *previous_frame =
					pending_frames[write_index];
				IDXGIKeyedMutex *shared_km =
					previous_frame->shared_km;
				if (!shared_km) {
					make_shared_resources(previous_frame);
					shared_km = previous_frame->shared_km;
				}
				HRESULT hr = shared_km->AcquireSync(1, 0);
				if (SUCCEEDED(hr)) {
					hr = shared_km->ReleaseSync(0);
					if (FAILED(hr)) {
						blog(LOG_ERROR,
						     "determine_next_frame: Failed to acquire sync 1 (%08lX)",
						     hr);
					}
				} else {
					blog(LOG_ERROR,
					     "determine_next_frame: Failed to release sync 0 (%08lX)",
					     hr);
				}

				d->available_frames.push(previous_frame);
				d->worker_progress_ec.notify();
			} else {
				previous_slot = slot;
				++write_index;
			}

			pending_frames[write_index] = frame;
		}

		pending_frame_count = write_index + 1;
	}

	for (size_t i = 0; i < pending_frame_count; ++i) {
		gs_duplicator_frame *const pending_frame = d->pending_frames[i];
		const uint64_t present_time = pending_frame->present_time;
		if (present_time < boundary) {
			if ((next_frame == nullptr) ||
			    (present_time > next_frame->present_time)) {
				next_frame = pending_frame;
			}
		}
	}

	if (next_frame) {
		size_t write_index = 0;
		for (size_t i = 0; i < pending_frame_count; ++i) {
			gs_duplicator_frame *const pending_frame =
				pending_frames[i];
			if (pending_frame != next_frame) {
				if (pending_frame->present_time <=
				    next_frame->present_time) {
					IDXGIKeyedMutex *shared_km =
						pending_frame->shared_km;
					if (!shared_km) {
						make_shared_resources(
							pending_frame);
						shared_km =
							pending_frame->shared_km;
					}
					HRESULT hr =
						shared_km->AcquireSync(1, 0);
					if (SUCCEEDED(hr)) {
						hr = shared_km->ReleaseSync(0);
						if (FAILED(hr)) {
							blog(LOG_ERROR,
							     "determine_next_frame: Failed to acquire sync 1 (%08lX)",
							     hr);
						}
					} else {
						blog(LOG_ERROR,
						     "determine_next_frame: Failed to release sync 0 (%08lX)",
						     hr);
					}
					d->available_frames.push(pending_frame);
					d->worker_progress_ec.notify();
				} else {
					pending_frames[write_index] =
						pending_frames[i];
					++write_index;
				}
			}
		}

		pending_frame_count = write_index;
	}

	d->pending_frame_count = pending_frame_count;

	return next_frame;
}

EXPORT bool gs_duplicator_update_frame(gs_duplicator_t *d)
{
	if (d->updated)
		return true;

	if (WaitForSingleObject(d->worker_thread, 0) == WAIT_OBJECT_0)
		return false;

	bool success = true;

	gs_duplicator_frame *const next_frame = determine_next_frame(d);
	if (next_frame) {
		if (!next_frame->shared_km) {
			make_shared_resources(next_frame);
		}

		if (success) {
			const HRESULT hr =
				next_frame->shared_km->AcquireSync(1, 0);
			success = SUCCEEDED(hr);
			if (success) {
				gs_duplicator_frame *const available_frame =
					d->graphics_frame;
				d->graphics_frame = next_frame;

				if (available_frame) {
					const HRESULT hr =
						available_frame->shared_km
							->ReleaseSync(0);
					success = SUCCEEDED(hr);
					if (success) {
						d->available_frames.push(
							available_frame);
						d->worker_progress_ec.notify();
					} else {
						blog(LOG_ERROR,
						     "gs_duplicator_update_frame: Failed to release sync 0 (%08lX)",
						     hr);
					}
				}
			} else {
				blog(LOG_ERROR,
				     "gs_duplicator_update_frame: Failed to acquire sync 1 (%08lX)",
				     hr);
			}
		}
	}

	d->updated = success;
	return success;
}

EXPORT gs_texture_t *gs_duplicator_get_texture(gs_duplicator_t *duplicator)
{
	/* Check shared_km, shared_tex might be stale after device reset. */
	gs_duplicator_frame *const graphics_frame = duplicator->graphics_frame;
	return (graphics_frame && graphics_frame->shared_km)
		       ? graphics_frame->shared_tex
		       : nullptr;
}
}
