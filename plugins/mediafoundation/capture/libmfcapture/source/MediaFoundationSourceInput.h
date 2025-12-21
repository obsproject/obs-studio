/*

This is provided under a dual MIT/GPLv2 license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2025 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Contact Information:

Thy-Lan Gale, thy-lan.gale@intel.com
5000 W Chandler Blvd, Chandler, AZ 85226

MIT License

Copyright (c) 2025 Microsoft Corporation.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE
*/

#pragma once

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>
#include <util/windows/WinHandle.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include <dxcore.h>
#include <mfapi.h>
#include <objbase.h>
#include <winrt/base.h>
#include <initguid.h>

#include "mfcapture.hpp"
#include "win-mf.hpp"

extern DWORD CALLBACK MediaFoundationSourceThread(LPVOID ptr);

struct MediaFoundationSourceInput {
	obs_source_t *source;
	CAPTURE_DEVICE_HANDLE mfcaptureDevice = nullptr;
	HRESULT mfcapturedialog = E_FAIL;

	bool deactivateWhenNotShowing = false;
	bool flip = false;
	bool active = false;
	bool autorotation = true;
	bool firstframe = true;

	MediaFoundationVideoConfig videoConfig;

	obs_source_frame2 frame;

	WinHandle semaphore;
	WinHandle activated_event;
	WinHandle saved_event;
	WinHandle thread;
	CriticalSection mutex;
	std::vector<Action> actions;

	inline void QueueAction(Action action)
	{
		CriticalScope scope(mutex);
		actions.push_back(action);
		ReleaseSemaphore(semaphore, 1, nullptr);
	}

	inline void QueueActivate(obs_data_t *settings)
	{
		bool block = obs_data_get_bool(settings, "synchronous_activate");
		QueueAction(block ? Action::ActivateBlock : Action::Activate);
		if (block) {
			obs_data_erase(settings, "synchronous_activate");
			WaitForSingleObject(activated_event, INFINITE);
		}
	}

	inline MediaFoundationSourceInput(obs_source_t *source_, obs_data_t *settings) : source(source_)
	{
		memset(&frame, 0, sizeof(frame));

		semaphore = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, nullptr);
		if (!semaphore)
			throw "Failed to create semaphore";

		activated_event = CreateEvent(nullptr, false, false, nullptr);
		if (!activated_event)
			throw "Failed to create activated_event";

		saved_event = CreateEvent(nullptr, false, false, nullptr);
		if (!saved_event)
			throw "Failed to create saved_event";

		thread = CreateThread(nullptr, 0, MediaFoundationSourceThread, this, 0, nullptr);
		if (!thread)
			throw "Failed to create thread";

		deactivateWhenNotShowing = obs_data_get_bool(settings, DEACTIVATE_WNS);

		if (obs_data_get_bool(settings, "active")) {
			bool showing = obs_source_showing(source);
			if (!deactivateWhenNotShowing || showing)
				QueueActivate(settings);

			active = true;
		}
	}

	inline ~MediaFoundationSourceInput()
	{
		{
			CriticalScope scope(mutex);
			actions.resize(1);
			actions[0] = Action::Shutdown;
		}

		ReleaseSemaphore(semaphore, 1, nullptr);

		WaitForSingleObject(thread, INFINITE);
	}

	void OnReactivate();
	bool UpdateVideoConfig(obs_data_t *settings);
	bool UpdateVideoProperties(obs_data_t *settings);
	void SaveVideoProperties();
	void SetActive(bool active);
	inline enum video_colorspace GetColorSpace(obs_data_t *settings) const;
	inline enum video_range_type GetColorRange(obs_data_t *settings) const;
	inline bool Activate(obs_data_t *settings);
	inline void Deactivate();

	inline void SetupBuffering(obs_data_t *settings);

	void MediaFoundationSourceLoop();

	void static __stdcall OnVideoDataStatic(void *pData, int Size, long long llTimestamp, void *pUserData);
	void OnVideoData(void *pData, int Size, long long llTimestamp);
};

struct PropertiesData {
	MediaFoundationSourceInput *input;
	std::vector<MediaFoundationVideoDevice> devices;

	bool GetDevice(MediaFoundationVideoDevice &device, const char *encoded_id) const
	{
		MediaFoundationDeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const MediaFoundationVideoDevice &curDevice : devices) {
			if (deviceId.name == curDevice.name && deviceId.path == curDevice.path) {
				device = curDevice;
				return true;
			}
		}

		return false;
	}
};
