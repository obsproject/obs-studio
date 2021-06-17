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

#include "d3d11-fence.hpp"
#include <stdexcept>

bool gs_fence::available(gs_device *device)
{
	ComPtr<ID3D11Device5> dev5;
	HRESULT hr = device->device->QueryInterface(&dev5);
	if (hr != S_OK) {
		return false;
	}

	// Most likely supports fences.
	return true;
}

gs_fence::gs_fence(gs_device *device) : gs_obj(device, gs_type::gs_fence)
{
	HRESULT hr = device->device->QueryInterface(&device5);
	if (hr != S_OK) {
		throw std::runtime_error("Failed to get ID3D11Device5 object.");
	}

	hr = device->context->QueryInterface(&context4);
	if (hr != S_OK) {
		throw std::runtime_error(
			"Failed to get ID3D11DeviceContext4 object.");
	}

	hr = device5->CreateFence(0, D3D11_FENCE_FLAG_SHARED,
				  __uuidof(ID3D11Fence),
				  reinterpret_cast<void **>(&fence));
	if (hr != S_OK) {
		throw std::runtime_error("Failed to create Fence.");
	}

	hevent = CreateEventExW(NULL, NULL, CREATE_EVENT_MANUAL_RESET,
				EVENT_ALL_ACCESS);
	if (!hevent || (hevent == INVALID_HANDLE_VALUE)) {
		throw std::runtime_error("Failed to create event for Fence.");
	}

	hr = fence->SetEventOnCompletion(1, hevent);
	if (hr != S_OK) {
		throw std::runtime_error("Failed to assign event to Fence.");
	}
}

gs_fence::~gs_fence()
{
	CloseHandle(hevent);
	fence.Release();
	device5.Release();
}

void gs_fence::signal()
{
	// Enqueue a signal to the Fence, ...
	context4->Signal(fence, 1);
	// ... and flush all scheduled commands to the GPU.
	context4->Flush();
}

void gs_fence::wait()
{
	timed_wait(INFINITE);
}

bool gs_fence::timed_wait(uint32_t ms)
{
	DWORD res = WaitForSingleObject(hevent, ms);
	if (res == WAIT_OBJECT_0)
		return true;
	return false;
}

void gs_fence::reset()
{
	// Set the Fence back to 0 and ...
	context4->Signal(fence, 0);
	// ... flush this to the GPU.
	context4->Flush();
	// Reset the attached event.
	ResetEvent(hevent);
}

extern "C" EXPORT bool device_fence_available(gs_device_t *device)
{
	return gs_fence::available(device);
}

extern "C" EXPORT gs_fence_t *device_fence_create(gs_device_t *device)
{
	try {
		return new gs_fence(device);
	} catch (...) {
		return nullptr;
	}
}

extern "C" EXPORT void gs_fence_destroy(gs_fence_t *fence)
{
	if (!fence)
		return;

	delete reinterpret_cast<gs_fence *>(fence);
}

extern "C" EXPORT void gs_fence_signal(gs_fence_t *fence)
{
	if (!fence)
		return;

	reinterpret_cast<gs_fence *>(fence)->signal();
}

extern "C" EXPORT void gs_fence_wait(gs_fence_t *fence)
{
	if (!fence)
		return;

	reinterpret_cast<gs_fence *>(fence)->wait();
}

extern "C" EXPORT bool gs_fence_timed_wait(gs_fence_t *fence,
					   uint64_t nanoseconds)
{
	if (!fence)
		return false;

	return reinterpret_cast<gs_fence *>(fence)->timed_wait(
		static_cast<uint32_t>(nanoseconds / 1000000ull));
}
