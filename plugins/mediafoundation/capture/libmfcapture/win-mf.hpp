/*

This is provided under a dual MIT/BSD/GPLv2 license.  When using or
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

BSD LICENSE

Copyright(c) 2025 Intel Corporation.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in
the documentation and/or other materials provided with the
distribution.

* Neither the name of Intel Corporation nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include "mfcapture.hpp"
#include "CriticalSection.cpp"

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

#define FPS_HIGHEST 0LL
#define FPS_MATCHING -1LL

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

namespace {
constexpr const char *VIDEO_DEVICE_ID = "video_device_id";
constexpr const char *RES_TYPE = "res_type";
constexpr const char *RESOLUTION = "resolution";
constexpr const char *FRAME_INTERVAL = "frame_interval";
constexpr const char *LAST_VIDEO_DEV_ID = "last_video_device_id";
constexpr const char *LAST_RESOLUTION = "last_resolution";
constexpr const char *BUFFERING_VAL = "buffering";
constexpr const char *FLIP_IMAGE = "flip_vertically";
constexpr const char *COLOR_SPACE = "color_space";
constexpr const char *COLOR_RANGE = "color_range";
constexpr const char *DEACTIVATE_WNS = "deactivate_when_not_showing";
constexpr const char *AUTOROTATION = "autorotation";

const char *TEXT_INPUT_NAME = obs_module_text("VideoCaptureDevice");
const char *TEXT_DEVICE = obs_module_text("Device");
const std::string_view TEXT_CONFIG_VIDEO = obs_module_text("ConfigureVideo");
const char *TEXT_RES_FPS_TYPE = obs_module_text("ResFPSType");
const char *TEXT_CUSTOM_RES = obs_module_text("ResFPSType.Custom");
const char *TEXT_PREFERRED_RES = obs_module_text("ResFPSType.DevPreferred");
const char *TEXT_FPS_MATCHING = obs_module_text("FPS.Matching");
const char *TEXT_FPS_HIGHEST = obs_module_text("FPS.Highest");
const char *TEXT_RESOLUTION = obs_module_text("Resolution");
const std::string_view TEXT_BUFFERING = obs_module_text("Buffering");
const std::string_view TEXT_BUFFERING_AUTO = obs_module_text("Buffering.AutoDetect");
const std::string_view TEXT_BUFFERING_ON = obs_module_text("Buffering.Enable");
const std::string_view TEXT_BUFFERING_OFF = obs_module_text("Buffering.Disable");
const char *TEXT_FLIP_IMAGE = obs_module_text("FlipVertically");
const std::string_view TEXT_AUTOROTATION = obs_module_text("Autorotation");
const char *TEXT_ACTIVATE = obs_module_text("Activate");
const char *TEXT_DEACTIVATE = obs_module_text("Deactivate");
const std::string_view TEXT_COLOR_SPACE = obs_module_text("ColorSpace");
const std::string_view TEXT_COLOR_DEFAULT = obs_module_text("ColorSpace.Default");
const std::string_view TEXT_COLOR_709 = obs_module_text("ColorSpace.709");
const std::string_view TEXT_COLOR_601 = obs_module_text("ColorSpace.601");
const std::string_view TEXT_COLOR_2100PQ = obs_module_text("ColorSpace.2100PQ");
const std::string_view TEXT_COLOR_2100HLG = obs_module_text("ColorSpace.2100HLG");
const std::string_view TEXT_COLOR_RANGE = obs_module_text("ColorRange");
const std::string_view TEXT_RANGE_DEFAULT = obs_module_text("ColorRange.Default");
const std::string_view TEXT_RANGE_PARTIAL = obs_module_text("ColorRange.Partial");
const std::string_view TEXT_RANGE_FULL = obs_module_text("ColorRange.Full");
const char *TEXT_DWNS = obs_module_text("DeactivateWhenNotShowing");

constexpr const char *INTELNPU_BLUR_TYPE = "intelnpu_blur_type";
const char *TEXT_INTELNPU_BLUR_TYPE = obs_module_text("IntelNPU.BackgroundBlur");
const char *TEXT_INTELNPU_BLUR_NONE = obs_module_text("IntelNPU.BlurNone");
const char *TEXT_INTELNPU_BLUR_STANDARD = obs_module_text("IntelNPU.BlurStandard");
const char *TEXT_INTELNPU_BLUR_PORTRAIT = obs_module_text("IntelNPU.Portrait");

constexpr const char *INTELNPU_BACKGROUND_REMOVAL = "intelnpu_background_removal";
const char *TEXT_INTELNPU_BACKGROUND_REMOVAL = obs_module_text("IntelNPU.BackgroundRemoval");

constexpr const char *INTELNPU_AUTO_FRAMING = "intelnpu_auto_framing";
const char *TEXT_INTELNPU_AUTO_FRAMING = obs_module_text("IntelNPU.AutoFraming");

constexpr const char *INTELNPU_EYEGAZE_CORRECTION = "intelnpu_eyegaze_correction";
const char *TEXT_INTELNPU_EYEGAZE_CORRECTION = obs_module_text("IntelNPU.EyeGazeCorrection");
} // namespace

enum BlurType {
	NpuBlurType_None,
	NpuBlurType_Standard,
	NpuBlurType_Portrait,
};

enum ResType {
	ResTypePreferred,
	ResTypeCustom,
};

enum class BufferingType : int64_t {
	Auto,
	On,
	Off,
};

enum class Action { None, Activate, ActivateBlock, Deactivate, Shutdown, SaveSettings, RestoreSettings, NpuControl };

struct MediaFoundationVideoInfo {
	int minCX;
	int minCY;
	int maxCX;
	int maxCY;
	int granularityCX;
	int granularityCY;
	long long minInterval;
	long long maxInterval;
};

struct MediaFoundationDeviceId {
	std::wstring name;
	std::wstring path;
};

struct MediaFoundationVideoDevice : MediaFoundationDeviceId {
	std::vector<MediaFoundationVideoInfo> caps;
};

struct MediaFoundationVideoConfig {
	std::wstring name;
	std::wstring path;

	/* Desired width/height of video. */
	int cx = 0;
	int cyAbs = 0;

	/* Whether or not cy was negative. */
	bool cyFlip = false;

	/* Desired frame interval (in 100-nanosecond units) */
	long long frameInterval = 0;
};

struct MediaFoundationVideoDeviceProperty {
	long property;
	long flags;
	long val;
	long min;
	long max;
	long step;
	long def;
};

bool DecodeDeviceId(MediaFoundationDeviceId &out, const char *deviceId);
void ProcessMessages();
bool IsDecoupled(const MediaFoundationVideoConfig &config);
bool ResolutionValid(const std::string &res, int &cx, int &cy);
long long GetOBSFPS();
bool MatcherClosestFrameRateSelector(long long interval, long long &best_match, const MediaFoundationVideoInfo &info);
bool ConvertRes(int &cx, int &cy, const char *res);
bool ResolutionAvailable(const MediaFoundationVideoInfo &cap, int cx, int cy);
bool FrameRateAvailable(const MediaFoundationVideoInfo &cap, long long interval);

static inline bool CapsMatch(const MediaFoundationVideoInfo &)
{
	return true;
}

template<typename... F> static bool CapsMatch(const MediaFoundationVideoDevice &dev, F... fs);

template<typename F, typename... Fs> static inline bool CapsMatch(const MediaFoundationVideoInfo &info, F &&f, Fs... fs)
{
	return f(info) && CapsMatch(info, fs...);
}

template<typename... F> static bool CapsMatch(const MediaFoundationVideoDevice &dev, F... fs)
{
	// no early exit, trigger all side effects.
	bool match = false;
	for (const MediaFoundationVideoInfo &info : dev.caps)
		if (CapsMatch(info, fs...))
			match = true;
	return match;
}

#define ResolutionMatcher(cx, cy)                                \
	[cx, cy](const MediaFoundationVideoInfo &info) -> bool { \
		return ResolutionAvailable(info, cx, cy);        \
	}
#define FrameRateMatcher(interval)                                 \
	[interval](const MediaFoundationVideoInfo &info) -> bool { \
		return FrameRateAvailable(info, interval);         \
	}
#define ClosestFrameRateSelector(interval, best_match)                                  \
	[interval, &best_match](const MediaFoundationVideoInfo &info) mutable -> bool { \
		return MatcherClosestFrameRateSelector(interval, best_match, info);     \
	}
