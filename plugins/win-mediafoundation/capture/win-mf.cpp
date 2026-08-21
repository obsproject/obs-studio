/*

This is provided under a dual MIT/GPLv2+ license.  When using or
redistributing this, you may do so under either license.

GPL LICENSE SUMMARY

Copyright(c) 2025 Intel Corporation.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

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

#include "MediaFoundationSourceInput.hpp"
#include "DeviceEnumerator.h"

#define TEXT_INPUT_NAME obs_module_text("VideoCaptureDevice")
#define TEXT_DEVICE obs_module_text("Device")
#define TEXT_CONFIG_VIDEO obs_module_text("ConfigureVideo")
#define TEXT_RES_FPS_TYPE obs_module_text("ResFPSType")
#define TEXT_CUSTOM_RES obs_module_text("ResFPSType.Custom")
#define TEXT_PREFERRED_RES obs_module_text("ResFPSType.DevPreferred")
#define TEXT_FPS_MATCHING obs_module_text("FPS.Matching")
#define TEXT_FPS_HIGHEST obs_module_text("FPS.Highest")
#define TEXT_RESOLUTION obs_module_text("Resolution")
#define TEXT_BUFFERING obs_module_text("Buffering")
#define TEXT_BUFFERING_AUTO obs_module_text("Buffering.AutoDetect")
#define TEXT_BUFFERING_ON obs_module_text("Buffering.Enable")
#define TEXT_BUFFERING_OFF obs_module_text("Buffering.Disable")
#define TEXT_FLIP_IMAGE obs_module_text("FlipVertically")
#define TEXT_AUTOROTATION obs_module_text("Autorotation")
#define TEXT_ACTIVATE obs_module_text("Activate")
#define TEXT_DEACTIVATE obs_module_text("Deactivate")
#define TEXT_COLOR_SPACE obs_module_text("ColorSpace")
#define TEXT_COLOR_DEFAULT obs_module_text("ColorSpace.Default")
#define TEXT_COLOR_709 obs_module_text("ColorSpace.709")
#define TEXT_COLOR_601 obs_module_text("ColorSpace.601")
#define TEXT_COLOR_2100PQ obs_module_text("ColorSpace.2100PQ")
#define TEXT_COLOR_2100HLG obs_module_text("ColorSpace.2100HLG")
#define TEXT_COLOR_RANGE obs_module_text("ColorRange")
#define TEXT_RANGE_DEFAULT obs_module_text("ColorRange.Default")
#define TEXT_RANGE_PARTIAL obs_module_text("ColorRange.Partial")
#define TEXT_RANGE_FULL obs_module_text("ColorRange.Full")
#define TEXT_DWNS obs_module_text("DeactivateWhenNotShowing")

#define INTELNPU_BLUR_TYPE "intelnpu_blur_type"
#define TEXT_INTELNPU_BLUR_TYPE obs_module_text("IntelNPU.BackgroundBlur")
#define TEXT_INTELNPU_BLUR_NONE obs_module_text("IntelNPU.BlurNone")
#define TEXT_INTELNPU_BLUR_STANDARD obs_module_text("IntelNPU.BlurStandard")
#define TEXT_INTELNPU_BLUR_PORTRAIT obs_module_text("IntelNPU.Portrait")

#define INTELNPU_BACKGROUND_REMOVAL "intelnpu_background_removal"
#define TEXT_INTELNPU_BACKGROUND_REMOVAL obs_module_text("IntelNPU.BackgroundRemoval")

#define INTELNPU_AUTO_FRAMING "intelnpu_auto_framing"
#define TEXT_INTELNPU_AUTO_FRAMING obs_module_text("IntelNPU.AutoFraming")

#define INTELNPU_EYEGAZE_CORRECTION "intelnpu_eyegaze_correction"
#define TEXT_INTELNPU_EYEGAZE_CORRECTION obs_module_text("IntelNPU.EyeGazeCorrection")

#define MAKE_MEDIAFOUNDATION_FPS(fps) (10000000LL / (fps))
#define MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(den, num) ((num) * 10000000LL / (den))
#define DEVICE_INTERVAL_DIFF_LIMIT 20

struct FPSFormat {
	const char *text;
	long long interval;
};

const FPSFormat validFPSFormats[] = {
	{"60", MAKE_MEDIAFOUNDATION_FPS(60)},
	{"59.94 NTSC", MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(60000, 1001)},
	{"50", MAKE_MEDIAFOUNDATION_FPS(50)},
	{"48 film", MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(48000, 1001)},
	{"40", MAKE_MEDIAFOUNDATION_FPS(40)},
	{"30", MAKE_MEDIAFOUNDATION_FPS(30)},
	{"29.97 NTSC", MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(30000, 1001)},
	{"25", MAKE_MEDIAFOUNDATION_FPS(25)},
	{"24 film", MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(24000, 1001)},
	{"20", MAKE_MEDIAFOUNDATION_FPS(20)},
	{"15", MAKE_MEDIAFOUNDATION_FPS(15)},
	{"10", MAKE_MEDIAFOUNDATION_FPS(10)},
	{"5", MAKE_MEDIAFOUNDATION_FPS(5)},
	{"4", MAKE_MEDIAFOUNDATION_FPS(4)},
	{"3", MAKE_MEDIAFOUNDATION_FPS(3)},
	{"2", MAKE_MEDIAFOUNDATION_FPS(2)},
	{"1", MAKE_MEDIAFOUNDATION_FPS(1)},
};

class Resolution {
public:
	int cx;
	int cy;

	inline Resolution(int cx, int cy) : cx(cx), cy(cy) {}
};

namespace {
void encodeDstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

void decodeDstr(struct dstr *str)
{
	dstr_replace(str, "#3A", ":");
	dstr_replace(str, "#22", "#");
}

void EncodeDeviceId(struct dstr *encodedStr, const wchar_t *nameStr, const wchar_t *pathStr)
{
	DStr name;
	DStr path;

	dstr_from_wcs(name, nameStr);
	dstr_from_wcs(path, pathStr);

	encodeDstr(name);
	encodeDstr(path);

	dstr_copy_dstr(encodedStr, name);
	dstr_cat(encodedStr, ":");
	dstr_cat_dstr(encodedStr, path);
}

bool DecodeDeviceDStr(DStr &name, DStr &path, const char *deviceId)
{
	const char *pathStr;

	if (!deviceId || !*deviceId) {
		return false;
	}

	pathStr = strchr(deviceId, ':');
	if (!pathStr) {
		return false;
	}

	dstr_copy(path, pathStr + 1);
	dstr_copy(name, deviceId);

	size_t len = pathStr - deviceId;
	name->array[len] = 0;
	name->len = len;

	decodeDstr(name);
	decodeDstr(path);

	return true;
}

template<typename T, typename U, typename V> bool between(T &&lower, U &&value, V &&upper)
{
	return value >= lower && value <= upper;
}

long long FrameRateInterval(const MediaFoundationVideoInfo &cap, long long desired_interval)
{
	return desired_interval < cap.minInterval ? cap.minInterval : std::min(desired_interval, cap.maxInterval);
}

bool ResolutionAvailable(const MediaFoundationVideoDevice &dev, FrameSize size)
{
	return CapsMatch(dev, ResolutionMatcher(size));
}

bool DetermineResolution(FrameSize &size, obs_data_t *settings, MediaFoundationVideoDevice &dev)
{
	PRAGMA_WARN_PUSH
	PRAGMA_WARN_DEPRECATION
	const char *res = obs_data_get_autoselect_string(settings, MFCaptureConstants::RESOLUTION);
	if (obs_data_has_autoselect_value(settings, MFCaptureConstants::RESOLUTION) && ConvertRes(size, res) &&
	    ResolutionAvailable(dev, size)) {
		return true;
	}
	PRAGMA_WARN_POP

	res = obs_data_get_string(settings, MFCaptureConstants::RESOLUTION);
	if (ConvertRes(size, res) && ResolutionAvailable(dev, size)) {
		return true;
	}

	res = obs_data_get_string(settings, MFCaptureConstants::LAST_RESOLUTION);
	if (ConvertRes(size, res) && ResolutionAvailable(dev, size)) {
		return true;
	}

	return false;
}

const char *GetMediaFoundationSourceInputName(void *)
{
	return TEXT_INPUT_NAME;
}

void proc_activate(void *data, calldata_t *cd)
{
	bool activate = calldata_bool(cd, "active");
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);
	input->SetActive(activate);
}

void *CreateMediaFoundationSourceInput(obs_data_t *settings, obs_source_t *source)
{
	std::unique_ptr<MediaFoundationSourceInput> input;

	try {
		input = std::make_unique<MediaFoundationSourceInput>(source, settings);
		proc_handler_t *ph = obs_source_get_proc_handler(source);
		proc_handler_add(ph, "void activate(bool active)", proc_activate, input.get());
	} catch (const char *error) {
		blog(LOG_ERROR, "Could not create device '%s': %s", obs_source_get_name(source), error);
	}

	return input.release();
}

void DestroyMediaFoundationSourceInput(void *data)
{
	delete static_cast<MediaFoundationSourceInput *>(data);
}

void UpdateMediaFoundationSourceInput(void *data, obs_data_t *settings)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);
	if (input->active) {
		input->QueueActivate(settings);
	}
}

void SaveMediaFoundationSourceInput(void *data, obs_data_t *settings)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);
	if (!input->active) {
		return;
	}

	input->QueueAction(Action::SaveSettings);
	WaitForSingleObject(input->saved_event, INFINITE);
}

void GetMediaFoundationSourceDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, MFCaptureConstants::FRAME_INTERVAL, FPS_MATCHING);
	obs_data_set_default_int(settings, MFCaptureConstants::RES_TYPE, ResTypePreferred);
	obs_data_set_default_bool(settings, "active", true);
	obs_data_set_default_string(settings, MFCaptureConstants::COLOR_SPACE, "default");
	obs_data_set_default_string(settings, MFCaptureConstants::COLOR_RANGE, "default");
	obs_data_set_default_bool(settings, MFCaptureConstants::AUTOROTATION, true);
}

void InsertResolution(std::vector<Resolution> &resolutions, int cx, int cy)
{
	int bestCY = 0;
	size_t idx = 0;

	for (; idx < resolutions.size(); idx++) {
		const Resolution &res = resolutions[idx];
		if (res.cx > cx) {
			break;
		}

		if (res.cx == cx) {
			if (res.cy == cy) {
				return;
			}

			if (!bestCY) {
				bestCY = res.cy;
			} else if (res.cy > bestCY) {
				break;
			}
		}
	}

	resolutions.insert(resolutions.begin() + idx, Resolution(cx, cy));
}

void AddCap(std::vector<Resolution> &resolutions, const MediaFoundationVideoInfo &cap)
{
	InsertResolution(resolutions, cap.minCX, cap.minCY);
	InsertResolution(resolutions, cap.maxCX, cap.maxCY);
}

bool TryResolution(const MediaFoundationVideoDevice &dev, const std::string &res)
{
	FrameSize size;
	if (!ConvertRes(size, res.c_str())) {
		return false;
	}

	return ResolutionAvailable(dev, size);
}

bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

bool ResTypeChanged(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

bool SetResolution(obs_properties_t *props, obs_data_t *settings, const std::string &res, bool autoselect = false)
{
	PRAGMA_WARN_PUSH
	PRAGMA_WARN_DEPRECATION
	if (autoselect) {
		obs_data_set_autoselect_string(settings, MFCaptureConstants::RESOLUTION, res.c_str());
	} else {
		obs_data_unset_autoselect_value(settings, MFCaptureConstants::RESOLUTION);
	}
	PRAGMA_WARN_POP

	DeviceIntervalChanged(props, obs_properties_get(props, MFCaptureConstants::FRAME_INTERVAL), settings);

	if (!autoselect) {
		obs_data_set_string(settings, MFCaptureConstants::LAST_RESOLUTION, res.c_str());
	}
	return true;
}

bool DeviceResolutionChanged(obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	PropertiesData *data = static_cast<PropertiesData *>(obs_properties_get_param(props));
	const char *id;
	MediaFoundationVideoDevice device;

	id = obs_data_get_string(settings, MFCaptureConstants::VIDEO_DEVICE_ID);
	std::string res = obs_data_get_string(settings, MFCaptureConstants::RESOLUTION);
	std::string last_res = obs_data_get_string(settings, MFCaptureConstants::LAST_RESOLUTION);

	if (!data->GetDevice(device, id)) {
		return false;
	}

	if (TryResolution(device, res)) {
		return SetResolution(props, settings, res);
	}

	if (TryResolution(device, last_res)) {
		return SetResolution(props, settings, last_res, true);
	}

	return false;
}

size_t AddDevice(obs_property_t *device_list, const std::string &id)
{
	DStr name;
	DStr path;
	if (!DecodeDeviceDStr(name, path, id.c_str())) {
		return std::numeric_limits<size_t>::max();
	}

	return obs_property_list_add_string(device_list, name, id.c_str());
}

bool UpdateDeviceList(obs_property_t *list, const std::string &id)
{
	size_t size = obs_property_list_item_count(list);
	bool found = false;
	bool disabled_unknown_found = false;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_string(list, i) == id) {
			found = true;
			continue;
		}
		if (obs_property_list_item_disabled(list, i)) {
			disabled_unknown_found = true;
		}
	}

	if (!found && !disabled_unknown_found) {
		size_t idx = AddDevice(list, id);
		obs_property_list_item_disable(list, idx, true);
		return true;
	}

	if (found && !disabled_unknown_found) {
		return false;
	}

	for (size_t i = 0; i < size;) {
		if (obs_property_list_item_disabled(list, i)) {
			obs_property_list_item_remove(list, i);
			continue;
		}
		i += 1;
	}

	return true;
}

bool DeviceSelectionChanged(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	PropertiesData *data = static_cast<PropertiesData *>(obs_properties_get_param(props));
	MediaFoundationVideoDevice device;

	std::string id = obs_data_get_string(settings, MFCaptureConstants::VIDEO_DEVICE_ID);
	std::string old_id = obs_data_get_string(settings, MFCaptureConstants::LAST_VIDEO_DEV_ID);

	bool device_list_updated = UpdateDeviceList(p, id);

	if (!data->GetDevice(device, id.c_str())) {
		return !device_list_updated;
	}

	std::vector<Resolution> resolutions;
	for (const MediaFoundationVideoInfo &cap : device.caps) {
		AddCap(resolutions, cap);
	}

	p = obs_properties_get(props, MFCaptureConstants::RESOLUTION);
	obs_property_list_clear(p);

	for (size_t idx = resolutions.size(); idx > 0; --idx) {
		const Resolution &res = resolutions[idx - 1];

		std::string strRes;
		strRes += std::to_string(res.cx);
		strRes += "x";
		strRes += std::to_string(res.cy);

		obs_property_list_add_string(p, strRes.c_str(), strRes.c_str());
	}

	/* only refresh properties if device legitimately changed */
	if (!id.size() || !old_id.size() || id != old_id) {
		p = obs_properties_get(props, MFCaptureConstants::RES_TYPE);
		ResTypeChanged(props, p, settings);
		obs_data_set_string(settings, MFCaptureConstants::LAST_VIDEO_DEV_ID, id.c_str());
	}

	return true;
}

bool AddDevice(obs_property_t *device_list, const MediaFoundationVideoDevice &device)
{
	DStr name;
	DStr path;
	DStr device_id;

	dstr_from_wcs(name, device.name.c_str());
	dstr_from_wcs(path, device.path.c_str());

	encodeDstr(path);

	dstr_copy_dstr(device_id, name);
	encodeDstr(device_id);
	dstr_cat(device_id, ":");
	dstr_cat_dstr(device_id, path);

	obs_property_list_add_string(device_list, name, device_id);

	return true;
}

void PropertiesDataDestroy(void *data)
{
	delete static_cast<PropertiesData *>(data);
}

bool ResTypeChanged(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	int val = (int)obs_data_get_int(settings, MFCaptureConstants::RES_TYPE);
	bool enabled = (val != ResTypePreferred);

	p = obs_properties_get(props, MFCaptureConstants::RESOLUTION);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, MFCaptureConstants::FRAME_INTERVAL);
	obs_property_set_enabled(p, enabled);

	if (val == ResTypeCustom) {
		p = obs_properties_get(props, MFCaptureConstants::RESOLUTION);
		DeviceResolutionChanged(props, p, settings);
	} else {
		PRAGMA_WARN_PUSH
		PRAGMA_WARN_DEPRECATION
		obs_data_unset_autoselect_value(settings, MFCaptureConstants::FRAME_INTERVAL);
		PRAGMA_WARN_POP
	}

	return true;
}

DStr GetFPSName(long long interval)
{
	DStr name;

	if (interval == FPS_MATCHING) {
		dstr_cat(name, TEXT_FPS_MATCHING);
		return name;
	}

	if (interval == FPS_HIGHEST) {
		dstr_cat(name, TEXT_FPS_HIGHEST);
		return name;
	}

	for (const FPSFormat &format : validFPSFormats) {
		if (format.interval != interval) {
			continue;
		}

		dstr_cat(name, format.text);
		return name;
	}

	dstr_cat(name, std::to_string(10000000. / interval).c_str());
	return name;
}

void UpdateFPS(MediaFoundationVideoDevice &device, long long interval, FrameSize size, obs_properties_t *props)
{
	obs_property_t *list = obs_properties_get(props, MFCaptureConstants::FRAME_INTERVAL);

	obs_property_list_clear(list);

	obs_property_list_add_int(list, TEXT_FPS_MATCHING, FPS_MATCHING);
	obs_property_list_add_int(list, TEXT_FPS_HIGHEST, FPS_HIGHEST);

	bool interval_added = interval == FPS_HIGHEST || interval == FPS_MATCHING;
	for (const FPSFormat &fps_format : validFPSFormats) {
		long long format_interval = fps_format.interval;

		bool available = CapsMatch(device, ResolutionMatcher(size), FrameRateMatcher(format_interval));

		if (!available && interval != fps_format.interval) {
			continue;
		}

		if (interval == fps_format.interval) {
			interval_added = true;
		}

		size_t idx = obs_property_list_add_int(list, fps_format.text, fps_format.interval);
		obs_property_list_item_disable(list, idx, !available);
	}

	if (interval_added) {
		return;
	}

	size_t idx = obs_property_list_add_int(list, GetFPSName(interval), interval);
	obs_property_list_item_disable(list, idx, true);
}

bool UpdateFPS(long long interval, obs_property_t *list)
{
	size_t size = obs_property_list_item_count(list);
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_int(list, i) != interval) {
			continue;
		}

		obs_property_list_item_disable(list, i, true);
		if (size == 1) {
			return false;
		}

		dstr_cat(name, obs_property_list_item_name(list, i));
		break;
	}

	obs_property_list_clear(list);

	if (!name->len) {
		name = GetFPSName(interval);
	}

	obs_property_list_add_int(list, name, interval);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	long long val = obs_data_get_int(settings, MFCaptureConstants::FRAME_INTERVAL);

	PropertiesData *data = static_cast<PropertiesData *>(obs_properties_get_param(props));
	const char *id = obs_data_get_string(settings, MFCaptureConstants::VIDEO_DEVICE_ID);
	MediaFoundationVideoDevice device;

	if (!data->GetDevice(device, id)) {
		return UpdateFPS(val, p);
	}

	FrameSize size = {0};
	if (!DetermineResolution(size, settings, device)) {
		UpdateFPS(device, 0, size, props);
		return true;
	}

	int resType = (int)obs_data_get_int(settings, MFCaptureConstants::RES_TYPE);
	if (resType != ResTypeCustom) {
		return true;
	}

	if (val == FPS_MATCHING) {
		val = GetOBSFPS();
	}

	long long best_interval = std::numeric_limits<long long>::max();
	bool frameRateSupported = CapsMatch(device, ResolutionMatcher(size),
					    ClosestFrameRateSelector(val, best_interval), FrameRateMatcher(val));

	if (!frameRateSupported && best_interval != val) {
		long long listed_val = 0;
		for (const FPSFormat &format : validFPSFormats) {
			long long diff = llabs(format.interval - best_interval);
			if (diff < DEVICE_INTERVAL_DIFF_LIMIT) {
				listed_val = format.interval;
				break;
			}
		}

		if (listed_val != val) {
			PRAGMA_WARN_PUSH
			PRAGMA_WARN_DEPRECATION
			obs_data_set_autoselect_int(settings, MFCaptureConstants::FRAME_INTERVAL, listed_val);
			val = listed_val;
			PRAGMA_WARN_POP
		}

	} else {
		PRAGMA_WARN_PUSH
		PRAGMA_WARN_DEPRECATION
		obs_data_unset_autoselect_value(settings, MFCaptureConstants::FRAME_INTERVAL);
		PRAGMA_WARN_POP
	}

	UpdateFPS(device, val, size, props);

	return true;
}

bool ActivateClicked(obs_properties_t *, obs_property_t *p, void *data)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);

	if (input->active) {
		input->SetActive(false);
		obs_property_set_description(p, TEXT_ACTIVATE);
	} else {
		input->SetActive(true);
		obs_property_set_description(p, TEXT_DEACTIVATE);
	}

	return true;
}

void HideMediaFoundationSourceInput(void *data)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);

	if (input->deactivateWhenNotShowing && input->active) {
		input->QueueAction(Action::Deactivate);
	}
}

void ShowMediaFoundationSourceInput(void *data)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(data);

	if (input->deactivateWhenNotShowing && input->active) {
		input->QueueAction(Action::Activate);
	}
}
} // namespace

/* ------------------------------------------------------------------------- */

bool DecodeDeviceId(MediaFoundationDeviceId &out, const char *deviceId)
{
	DStr name;
	DStr path;

	if (!DecodeDeviceDStr(name, path, deviceId)) {
		return false;
	}

	BPtr<wchar_t> wname = dstr_to_wcs(name);
	out.name = wname;

	if (!dstr_is_empty(path)) {
		BPtr<wchar_t> wpath = dstr_to_wcs(path);
		out.path = wpath;
	}

	return true;
}

void ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool ResolutionAvailable(const MediaFoundationVideoInfo &cap, FrameSize size)
{
	return between(cap.minCX, size.width, cap.maxCX) && between(cap.minCY, size.height, cap.maxCY);
}

inline bool ConvertRes(FrameSize &size, const char *res)
{
	return sscanf(res, "%dx%d", &size.width, &size.height) == 2;
}

bool FrameRateAvailable(const MediaFoundationVideoInfo &cap, long long interval)
{
	return interval == FPS_HIGHEST || interval == FPS_MATCHING ||
	       between(cap.minInterval - DEVICE_INTERVAL_DIFF_LIMIT, interval,
		       cap.maxInterval + DEVICE_INTERVAL_DIFF_LIMIT);
}

bool ResolutionValid(const std::string &res, FrameSize &size)
{
	if (!res.size()) {
		return false;
	}

	return ConvertRes(size, res.c_str());
}

bool MatcherClosestFrameRateSelector(long long interval, long long best_match, const MediaFoundationVideoInfo &info)
{
	long long current = FrameRateInterval(info, interval);
	if (llabs(interval - best_match) > llabs(interval - current)) {
		best_match = current;
	}
	return true;
}

bool IsDecoupled(const MediaFoundationVideoConfig &config)
{
	return wstrstri(config.name.c_str(), L"GV-USB2") != NULL;
}

long long GetOBSFPS()
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi)) {
		return 0;
	}

	return MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(ovi.fps_num, ovi.fps_den);
}

DEFINE_GUID(OBS_DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU, 0xd46140c4, 0xadd7, 0x451b, 0x9e, 0x56, 0x6, 0xfe, 0x8c, 0x3b, 0x58,
	    0xed);

bool NPUDetection()
{
	// You begin DXCore adapter enumeration by creating an adapter factory.
	winrt::com_ptr<IDXCoreAdapterFactory> adapterFactory;
	winrt::check_hresult(::DXCoreCreateAdapterFactory(adapterFactory.put()));

	// From the factory, retrieve a list of all the Direct3D 12 Core Compute adapters.
	winrt::com_ptr<IDXCoreAdapterList> d3D12CoreComputeAdapters;
	GUID attributes[]{OBS_DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU};
	winrt::check_hresult(
		adapterFactory->CreateAdapterList(_countof(attributes), attributes, d3D12CoreComputeAdapters.put()));

	const uint32_t count{d3D12CoreComputeAdapters->GetAdapterCount()};

	bool npuDetected = false;

	for (uint32_t i = 0; i < count; ++i) {
		winrt::com_ptr<IDXCoreAdapter> candidateAdapter;
		winrt::check_hresult(d3D12CoreComputeAdapters->GetAdapter(i, candidateAdapter.put()));

		char description[256];
		winrt::check_hresult(
			candidateAdapter->GetProperty(DXCoreAdapterProperty::DriverDescription, &description));

		char npu[256] = "Intel(R) AI Boost";
		if (strcmp(description, npu) == 0) {
			npuDetected = true;
		}
	}

	return npuDetected;
}

obs_properties_t *GetMediaFoundationSourceProperties(void *obj)
{
	MediaFoundationSourceInput *input = static_cast<MediaFoundationSourceInput *>(obj);
	obs_properties_t *ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	data->input = input;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	obs_property_t *p = obs_properties_add_list(ppts, MFCaptureConstants::VIDEO_DEVICE_ID, TEXT_DEVICE,
						    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceSelectionChanged);

	DeviceEnumerator mfenum;
	mfenum.Enumerate(data->devices);

	for (const MediaFoundationVideoDevice &device : data->devices) {
		AddDevice(p, device);
	}

	const char *activateText = TEXT_ACTIVATE;
	if (input) {
		if (input->active) {
			activateText = TEXT_DEACTIVATE;
		}
	}

	obs_properties_add_button2(ppts, "activate", activateText, ActivateClicked, input);

	obs_properties_add_bool(ppts, MFCaptureConstants::DEACTIVATE_WNS, TEXT_DWNS);

	/* ------------------------------------- */
	/* Intel NPU AI effect settings */

	if (NPUDetection()) {
		p = obs_properties_add_list(ppts, INTELNPU_BLUR_TYPE, TEXT_INTELNPU_BLUR_TYPE, OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_INT);

		obs_property_list_add_int(p, TEXT_INTELNPU_BLUR_NONE, NpuBlurType_None);
		obs_property_list_add_int(p, TEXT_INTELNPU_BLUR_STANDARD, NpuBlurType_Standard);
		obs_property_list_add_int(p, TEXT_INTELNPU_BLUR_PORTRAIT, NpuBlurType_Portrait);

		obs_properties_add_bool(ppts, INTELNPU_BACKGROUND_REMOVAL, TEXT_INTELNPU_BACKGROUND_REMOVAL);
		obs_properties_add_bool(ppts, INTELNPU_AUTO_FRAMING, TEXT_INTELNPU_AUTO_FRAMING);
		obs_properties_add_bool(ppts, INTELNPU_EYEGAZE_CORRECTION, TEXT_INTELNPU_EYEGAZE_CORRECTION);
	}

	/* ------------------------------------- */
	/* video settings */

	p = obs_properties_add_list(ppts, MFCaptureConstants::RES_TYPE, TEXT_RES_FPS_TYPE, OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, ResTypeChanged);

	obs_property_list_add_int(p, TEXT_PREFERRED_RES, ResTypePreferred);
	obs_property_list_add_int(p, TEXT_CUSTOM_RES, ResTypeCustom);

	p = obs_properties_add_list(ppts, MFCaptureConstants::RESOLUTION, TEXT_RESOLUTION, OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceResolutionChanged);

	p = obs_properties_add_list(ppts, MFCaptureConstants::FRAME_INTERVAL, "FPS", OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, DeviceIntervalChanged);

	p = obs_properties_add_list(ppts, MFCaptureConstants::COLOR_RANGE, TEXT_COLOR_RANGE, OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, TEXT_RANGE_DEFAULT, "default");
	obs_property_list_add_string(p, TEXT_RANGE_PARTIAL, "partial");
	obs_property_list_add_string(p, TEXT_RANGE_FULL, "full");

	obs_properties_add_bool(ppts, MFCaptureConstants::FLIP_IMAGE, TEXT_FLIP_IMAGE);

	return ppts;
}

void RegisterMediaFoundationSource()
{
	obs_source_info info = {};
	info.id = "MediaFoundationsource_input";
	info.type = OBS_SOURCE_TYPE_INPUT;

	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC | OBS_SOURCE_DO_NOT_DUPLICATE;

	info.show = ShowMediaFoundationSourceInput;
	info.hide = HideMediaFoundationSourceInput;
	info.get_name = GetMediaFoundationSourceInputName;
	info.create = CreateMediaFoundationSourceInput;
	info.destroy = DestroyMediaFoundationSourceInput;
	info.update = UpdateMediaFoundationSourceInput;
	info.get_defaults = GetMediaFoundationSourceDefaults;
	info.get_properties = GetMediaFoundationSourceProperties;
	info.save = SaveMediaFoundationSourceInput;
	info.icon_type = OBS_ICON_TYPE_CAMERA;
	obs_register_source(&info);
}
