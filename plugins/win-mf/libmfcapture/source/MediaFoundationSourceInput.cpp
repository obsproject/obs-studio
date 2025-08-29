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

#include "DeviceEnumerator.h"
#include "MediaFoundationSourceInput.h"

void MediaFoundationSourceInput::MediaFoundationSourceLoop()
{
	while (true) {
		DWORD ret = MsgWaitForMultipleObjects(1, &_semaphore, false, INFINITE, QS_ALLINPUT);
		if (ret == (WAIT_OBJECT_0 + 1)) {
			ProcessMessages();
			continue;
		} else if (ret != WAIT_OBJECT_0) {
			break;
		}

		Action action = Action::None;
		{
			CriticalScope scope(_mutex);
			if (_actions.size()) {
				action = _actions.front();
				_actions.erase(_actions.begin());
			}
		}

		switch (action) {
		case Action::Activate:
		case Action::ActivateBlock: {
			bool block = action == Action::ActivateBlock;

			obs_data_t *settings;
			settings = obs_source_get_settings(source);
			if (!Activate(settings)) {
				obs_source_output_video2(source, nullptr);
			}
			if (block)
				SetEvent(_activated_event);
			obs_data_release(settings);
			break;
		}

		case Action::Deactivate:
			Deactivate();
			break;

		case Action::Shutdown:
			if (_mfcaptureDevice) {
				MF_Stop(_mfcaptureDevice);
				MF_Destroy(_mfcaptureDevice);
				_mfcaptureDevice = nullptr;
			}
			return;

		case Action::SaveSettings:
			SaveVideoProperties();
			break;

		case Action::RestoreSettings:
			if (_mfcaptureDevice) {
				MF_RestoreDefaultSettings(_mfcaptureDevice);
			}
			break;
		case Action::None:;
		}
	}
}

void MediaFoundationSourceInput::OnReactivate()
{
	SetActive(true);
}

void MediaFoundationSourceInput::OnVideoData(void *pData, int Size, long long llTimestamp)
{
	if (_firstframe) {
		QueueAction(Action::RestoreSettings);
		_firstframe = false;
		return;
	}

	const int cx = _videoConfig.cx;
	const int cyAbs = _videoConfig.cyAbs;

	_frame.timestamp = (uint64_t)llTimestamp * 100;
	_frame.width = _videoConfig.cx;
	_frame.height = cyAbs;
	_frame.format = VIDEO_FORMAT_BGRA;
	_frame.flip = _flip;
	_frame.flags = OBS_SOURCE_FRAME_LINEAR_ALPHA;

	_frame.data[0] = (unsigned char *)pData;
	_frame.linesize[0] = cx * 4;

	obs_source_output_video2(source, &_frame);
}

inline void MediaFoundationSourceInput::SetupBuffering(obs_data_t *settings)
{
	BufferingType bufType;
	bool useBuffering;

	bufType = (BufferingType)obs_data_get_int(settings, BUFFERING_VAL);

	if (bufType == BufferingType::Auto)
		useBuffering = false;
	else
		useBuffering = bufType == BufferingType::On;

	obs_source_set_async_unbuffered(source, !useBuffering);
	obs_source_set_async_decoupled(source, IsDecoupled(_videoConfig));
}

void MediaFoundationSourceInput::_OnVideoData(void *pData, int Size, long long llTimestamp, void *pUserData)
{
	MediaFoundationSourceInput *pThis = (MediaFoundationSourceInput *)pUserData;

	if (pThis) {
		pThis->OnVideoData(pData, Size, llTimestamp);
	}
}

bool MediaFoundationSourceInput::UpdateVideoConfig(obs_data_t *settings)
{
	std::string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	_deactivateWhenNotShowing = obs_data_get_bool(settings, DEACTIVATE_WNS);
	_flip = obs_data_get_bool(settings, FLIP_IMAGE);
	_autorotation = obs_data_get_bool(settings, AUTOROTATION);

	MediaFoundationDeviceId id;
	if (!DecodeDeviceId(id, video_device_id.c_str())) {
		blog(LOG_WARNING, "%s: DecodeDeviceId failed", obs_source_get_name(source));
		return false;
	}

	PropertiesData data;
	DeviceEnumerator mfenum;
	mfenum.Enumerate(data.devices);

	MediaFoundationVideoDevice dev;
	if (!data.GetDevice(dev, video_device_id.c_str())) {
		blog(LOG_WARNING, "%s: data.GetDevice failed", obs_source_get_name(source));
		return false;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	int cx = 0, cy = 0;
	long long interval = 0;

	if (resType == ResTypeCustom) {
		bool has_autosel_val;
		std::string resolution = obs_data_get_string(settings, RESOLUTION);
		if (!ResolutionValid(resolution, cx, cy)) {
			blog(LOG_WARNING, "%s: ResolutionValid failed", obs_source_get_name(source));
			return false;
		}

		has_autosel_val = obs_data_has_autoselect_value(settings, FRAME_INTERVAL);
		interval = has_autosel_val ? obs_data_get_autoselect_int(settings, FRAME_INTERVAL)
					   : obs_data_get_int(settings, FRAME_INTERVAL);

		if (interval == FPS_MATCHING)
			interval = GetOBSFPS();

		long long best_interval = std::numeric_limits<long long>::max();
		CapsMatch(dev, ResolutionMatcher(cx, cy), ClosestFrameRateSelector(interval, best_interval),
			  FrameRateMatcher(interval));
		interval = best_interval;
	}

	_videoConfig.name = id.name;
	_videoConfig.path = id.path;
	_videoConfig.cx = cx;
	_videoConfig.cyAbs = abs(cy);
	_videoConfig.cyFlip = cy < 0;
	_videoConfig.frameInterval = interval;

	_mfcaptureDevice = MF_Create(dev.path.c_str());
	if (_mfcaptureDevice) {
		HRESULT hr = MF_Prepare(_mfcaptureDevice);
		UINT32 w, h, fpsd, fpsn;
		if (SUCCEEDED(hr)) {
			if (resType != ResTypeCustom) {
				hr = MF_GetOutputResolution(_mfcaptureDevice, &w, &h, &fpsn, &fpsd);
				if (SUCCEEDED(hr)) {
					interval = 10000000ll * fpsd / fpsn;
				}
			} else {
				w = _videoConfig.cx;
				h = _videoConfig.cyAbs;
				interval = _videoConfig.frameInterval;
			}
		}
		if (SUCCEEDED(hr)) {
			hr = MF_SetOutputResolution(_mfcaptureDevice, w, h, interval,
						    MF_COLOR_FORMAT::MF_COLOR_FORMAT_ARGB);
			_videoConfig.cx = w;
			_videoConfig.cyAbs = h;
			_videoConfig.frameInterval = interval;
		}
		if (SUCCEEDED(hr)) {
			_firstframe = true;
			hr = MF_Start(_mfcaptureDevice, _OnVideoData, this);
		}
	}

	SetupBuffering(settings);

	return true;
}

bool MediaFoundationSourceInput::UpdateVideoProperties(obs_data_t *settings)
{
	OBSDataArrayAutoRelease cca = obs_data_get_array(settings, "CameraControl");

	if (cca) {
		std::vector<MediaFoundationVideoDeviceProperty> properties;
		const auto count = obs_data_array_count(cca);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease item = obs_data_array_item(cca, i);
			if (!item)
				continue;

			MediaFoundationVideoDeviceProperty prop{};
			prop.property = (long)obs_data_get_int(item, "property");
			prop.flags = (long)obs_data_get_int(item, "flags");
			prop.val = (long)obs_data_get_int(item, "val");
			properties.push_back(prop);
		}

		if (!properties.empty()) {
		}
	}

	OBSDataArrayAutoRelease vpaa = obs_data_get_array(settings, "VideoProcAmp");

	if (vpaa) {
		std::vector<MediaFoundationVideoDeviceProperty> properties;
		const auto count = obs_data_array_count(vpaa);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease item = obs_data_array_item(vpaa, i);
			if (!item)
				continue;

			MediaFoundationVideoDeviceProperty prop{};
			prop.property = (long)obs_data_get_int(item, "property");
			prop.flags = (long)obs_data_get_int(item, "flags");
			prop.val = (long)obs_data_get_int(item, "val");
			properties.push_back(prop);
		}

		if (!properties.empty()) {
		}
	}

	return true;
}

void MediaFoundationSourceInput::SaveVideoProperties()
{
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (!settings) {
		SetEvent(_saved_event);
		return;
	}

	std::vector<MediaFoundationVideoDeviceProperty> properties;
	OBSDataArrayAutoRelease ccp = obs_data_array_create();

	obs_data_set_array(settings, "CameraControl", ccp);
	properties.clear();

	OBSDataArrayAutoRelease vpap = obs_data_array_create();

	obs_data_set_array(settings, "VideoProcAmp", vpap);

	SetEvent(_saved_event);
}

void MediaFoundationSourceInput::SetActive(bool active_)
{
	obs_data_t *settings = obs_source_get_settings(source);
	QueueAction(active_ ? Action::Activate : Action::Deactivate);
	obs_data_set_bool(settings, "active", active_);
	_active = active_;
	obs_data_release(settings);
}

inline enum video_colorspace MediaFoundationSourceInput::GetColorSpace(obs_data_t *settings) const
{
	const char *space = obs_data_get_string(settings, COLOR_SPACE);

	if (astrcmpi(space, "709") == 0)
		return VIDEO_CS_709;

	if (astrcmpi(space, "601") == 0)
		return VIDEO_CS_601;

	if (astrcmpi(space, "2100PQ") == 0)
		return VIDEO_CS_2100_PQ;

	if (astrcmpi(space, "2100HLG") == 0)
		return VIDEO_CS_2100_HLG;

	return VIDEO_CS_DEFAULT;
}

inline enum video_range_type MediaFoundationSourceInput::GetColorRange(obs_data_t *settings) const
{
	const char *range = obs_data_get_string(settings, COLOR_RANGE);

	if (astrcmpi(range, "full") == 0)
		return VIDEO_RANGE_FULL;
	if (astrcmpi(range, "partial") == 0)
		return VIDEO_RANGE_PARTIAL;
	return VIDEO_RANGE_DEFAULT;
}

inline bool MediaFoundationSourceInput::Activate(obs_data_t *settings)
{

	if (_mfcaptureDevice) {
		MF_Stop(_mfcaptureDevice);
		MF_Destroy(_mfcaptureDevice);
		_mfcaptureDevice = nullptr;
	}

	if (!UpdateVideoConfig(settings)) {
		blog(LOG_WARNING, "%s: Video configuration failed", obs_source_get_name(source));
		return false;
	}

	if (!UpdateVideoProperties(settings)) {
		blog(LOG_WARNING, "%s: Setting video device properties failed", obs_source_get_name(source));
	}

	const enum video_colorspace cs = GetColorSpace(settings);
	const enum video_range_type range = GetColorRange(settings);

	enum video_trc trc = VIDEO_TRC_DEFAULT;
	switch (cs) {
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_601:
	case VIDEO_CS_709:
	case VIDEO_CS_SRGB:
		trc = VIDEO_TRC_SRGB;
		break;
	case VIDEO_CS_2100_PQ:
		trc = VIDEO_TRC_PQ;
		break;
	case VIDEO_CS_2100_HLG:
		trc = VIDEO_TRC_HLG;
	}

	_frame.range = range;
	_frame.trc = trc;

	bool success = video_format_get_parameters_for_format(cs, range, VIDEO_FORMAT_BGRA, _frame.color_matrix,
							      _frame.color_range_min, _frame.color_range_max);
	if (!success) {
		blog(LOG_ERROR,
		     "Failed to get video format parameters for "
		     "video format %u",
		     cs);
	}

	return true;
}

inline void MediaFoundationSourceInput::Deactivate()
{
	if (_mfcaptureDevice) {
		MF_Stop(_mfcaptureDevice);
		MF_Destroy(_mfcaptureDevice);
		_mfcaptureDevice = nullptr;
	}
	obs_source_output_video2(source, nullptr);
}
