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
	CAPTURE_DEVICE_HANDLE _mfcaptureDevice = nullptr;
	HRESULT _mfcapturedialog = E_FAIL;

	bool _deactivateWhenNotShowing = false;
	bool _flip = false;
	bool _active = false;
	bool _autorotation = true;
	bool _firstframe = true;

	MediaFoundationVideoConfig _videoConfig;

	obs_source_frame2 _frame;

	WinHandle _semaphore;
	WinHandle _activated_event;
	WinHandle _saved_event;
	WinHandle _thread;
	CriticalSection _mutex;
	std::vector<Action> _actions;

	inline void QueueAction(Action action)
	{
		CriticalScope scope(_mutex);
		_actions.push_back(action);
		ReleaseSemaphore(_semaphore, 1, nullptr);
	}

	inline void QueueActivate(obs_data_t *settings)
	{
		bool block = obs_data_get_bool(settings, "synchronous_activate");
		QueueAction(block ? Action::ActivateBlock : Action::Activate);
		if (block) {
			obs_data_erase(settings, "synchronous_activate");
			WaitForSingleObject(_activated_event, INFINITE);
		}
	}

	inline MediaFoundationSourceInput(obs_source_t *source_, obs_data_t *settings) : source(source_)
	{
		memset(&_frame, 0, sizeof(_frame));

		_semaphore = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, nullptr);
		if (!_semaphore)
			throw "Failed to create semaphore";

		_activated_event = CreateEvent(nullptr, false, false, nullptr);
		if (!_activated_event)
			throw "Failed to create activated_event";

		_saved_event = CreateEvent(nullptr, false, false, nullptr);
		if (!_saved_event)
			throw "Failed to create saved_event";

		_thread = CreateThread(nullptr, 0, MediaFoundationSourceThread, this, 0, nullptr);
		if (!_thread)
			throw "Failed to create thread";

		_deactivateWhenNotShowing = obs_data_get_bool(settings, DEACTIVATE_WNS);

		if (obs_data_get_bool(settings, "active")) {
			bool showing = obs_source_showing(source);
			if (!_deactivateWhenNotShowing || showing)
				QueueActivate(settings);

			_active = true;
		}
	}

	inline ~MediaFoundationSourceInput()
	{
		{
			CriticalScope scope(_mutex);
			_actions.resize(1);
			_actions[0] = Action::Shutdown;
		}

		ReleaseSemaphore(_semaphore, 1, nullptr);

		WaitForSingleObject(_thread, INFINITE);
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

	void static __stdcall _OnVideoData(void *pData, int Size, long long llTimestamp, void *pUserData);
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
