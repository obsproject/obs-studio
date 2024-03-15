#include <objbase.h>

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/windows/WinHandle.hpp>
#include <util/threading.h>
#include <util/util.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "mfcapture.hpp"
#include <mfapi.h>

#include <dxcore.h>
#include <winrt/base.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

#undef min
#undef max

using namespace std;

/* settings defines that will cause errors if there are typos */
#define VIDEO_DEVICE_ID "video_device_id"
#define RES_TYPE "res_type"
#define RESOLUTION "resolution"
#define FRAME_INTERVAL "frame_interval"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION "last_resolution"
#define BUFFERING_VAL "buffering"
#define FLIP_IMAGE "flip_vertically"
#define COLOR_SPACE "color_space"
#define COLOR_RANGE "color_range"
#define DEACTIVATE_WNS "deactivate_when_not_showing"
#define AUTOROTATION "autorotation"

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

enum ResType {
	ResTypePreferred,
	ResTypeCustom,
};

enum class BufferingType : int64_t {
	Auto,
	On,
	Off,
};

class CriticalSection {
	CRITICAL_SECTION mutex;

public:
	inline CriticalSection() { InitializeCriticalSection(&mutex); }
	inline ~CriticalSection() { DeleteCriticalSection(&mutex); }

	inline operator CRITICAL_SECTION *() { return &mutex; }
};

class CriticalScope {
	CriticalSection &mutex;

	CriticalScope() = delete;
	CriticalScope &operator=(CriticalScope &cs) = delete;

public:
	inline CriticalScope(CriticalSection &mutex_) : mutex(mutex_)
	{
		EnterCriticalSection(mutex);
	}

	inline ~CriticalScope() { LeaveCriticalSection(mutex); }
};

enum class Action {
	None,
	Activate,
	ActivateBlock,
	Deactivate,
	Shutdown,
	SaveSettings,
	RestoreSettings
};

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

static inline void encodeDstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

static inline void decodeDstr(struct dstr *str)
{
	dstr_replace(str, "#3A", ":");
	dstr_replace(str, "#22", "#");
}

static inline void EncodeDeviceId(struct dstr *encodedStr,
				  const wchar_t *nameStr,
				  const wchar_t *pathStr)
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

static inline bool DecodeDeviceDStr(DStr &name, DStr &path,
				    const char *deviceId)
{
	const char *pathStr;

	if (!deviceId || !*deviceId)
		return false;

	pathStr = strchr(deviceId, ':');
	if (!pathStr)
		return false;

	dstr_copy(path, pathStr + 1);
	dstr_copy(name, deviceId);

	size_t len = pathStr - deviceId;
	name->array[len] = 0;
	name->len = len;

	decodeDstr(name);
	decodeDstr(path);

	return true;
}

static inline bool DecodeDeviceId(MediaFoundationDeviceId &out,
				  const char *deviceId)
{
	DStr name, path;

	if (!DecodeDeviceDStr(name, path, deviceId))
		return false;

	BPtr<wchar_t> wname = dstr_to_wcs(name);
	out.name = wname;

	if (!dstr_is_empty(path)) {
		BPtr<wchar_t> wpath = dstr_to_wcs(path);
		out.path = wpath;
	}

	return true;
}

static DWORD CALLBACK MediaFoundationSourceThread(LPVOID ptr);

// ============================libmfcapture===============================

class DeviceEnumerator {

	std::vector<MediaFoundationVideoDevice> _devices = {};

public:
	DeviceEnumerator() {}
	~DeviceEnumerator() {}

	void static __stdcall EnumerateCameraCallback(const wchar_t *Name,
						      const wchar_t *DevId,
						      void *pUserData)
	{
		DeviceEnumerator *pThis = (DeviceEnumerator *)pUserData;

		printf("Name: %ws, DevId: %ws\n", Name, DevId);
		MediaFoundationVideoDevice vd;
		vd.name = Name;
		vd.path = DevId;

		CAPTURE_DEVICE_HANDLE h = MF_Create(DevId);
		if (h) {
			HRESULT hr = MF_EnumerateStreamCapabilities(
				h, EnumerateStreamCapabilitiesCallback, &vd);
			if (SUCCEEDED(hr)) {
				pThis->_devices.push_back(vd);
			}
			MF_Destroy(h);
		}
	}

	void static __stdcall EnumerateStreamCapabilitiesCallback(
		UINT32 Width, UINT32 Height, UINT32 FpsN, UINT32 FpsD,
		void *pUserData)
	{
		MediaFoundationVideoDevice *vd =
			(MediaFoundationVideoDevice *)pUserData;
		MediaFoundationVideoInfo vInfo;
		vInfo.minCX = Width;
		vInfo.maxCX = Width;
		vInfo.minCY = Height;
		vInfo.maxCY = Height;
		vInfo.granularityCX = 1;
		vInfo.granularityCY = 1;
		vInfo.minInterval = 10000000ll * FpsD / FpsN;
		vInfo.maxInterval = vInfo.minInterval;
		vd->caps.push_back(vInfo);
	}

	HRESULT Enumerate(std::vector<MediaFoundationVideoDevice> &devices)
	{
		devices.clear();
		_devices.clear();
		HRESULT hr = MF_EnumerateCameras(EnumerateCameraCallback, this);
		if (SUCCEEDED(hr)) {
			devices = _devices;
		}
		return hr;
	}
};

// ============================libmfcapture===============================

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
	vector<Action> _actions;

	inline void QueueAction(Action action)
	{
		CriticalScope scope(_mutex);
		_actions.push_back(action);
		ReleaseSemaphore(_semaphore, 1, nullptr);
	}

	inline void QueueActivate(obs_data_t *settings)
	{
		bool block =
			obs_data_get_bool(settings, "synchronous_activate");
		QueueAction(block ? Action::ActivateBlock : Action::Activate);
		if (block) {
			obs_data_erase(settings, "synchronous_activate");
			WaitForSingleObject(_activated_event, INFINITE);
		}
	}

	inline MediaFoundationSourceInput(obs_source_t *source_,
					  obs_data_t *settings)
		: source(source_)
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

		_thread = CreateThread(nullptr, 0, MediaFoundationSourceThread,
				       this, 0, nullptr);
		if (!_thread)
			throw "Failed to create thread";

		_deactivateWhenNotShowing =
			obs_data_get_bool(settings, DEACTIVATE_WNS);

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

	void static __stdcall _OnVideoData(void *pData, int Size,
					   long long llTimestamp,
					   void *pUserData);
	void OnVideoData(void *pData, int Size, long long llTimestamp);
};

static DWORD CALLBACK MediaFoundationSourceThread(LPVOID ptr)
{
	MediaFoundationSourceInput *input = (MediaFoundationSourceInput *)ptr;
	os_set_thread_name("win-MediaFoundation: MediaFoundationSourceThread");
	if (SUCCEEDED(CoInitialize(nullptr))) {
		if (SUCCEEDED(MFStartup(MF_VERSION))) {
			input->MediaFoundationSourceLoop();
			MFShutdown();
		}
		CoUninitialize();
	}
	return 0;
}

static inline void ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void MediaFoundationSourceInput::MediaFoundationSourceLoop()
{
	while (true) {
		DWORD ret = MsgWaitForMultipleObjects(1, &_semaphore, false,
						      INFINITE, QS_ALLINPUT);
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

#define FPS_HIGHEST 0LL
#define FPS_MATCHING -1LL

template<typename T, typename U, typename V>
static bool between(T &&lower, U &&value, V &&upper)
{
	return value >= lower && value <= upper;
}

static bool ResolutionAvailable(const MediaFoundationVideoInfo &cap, int cx,
				int cy)
{
	return between(cap.minCX, cx, cap.maxCX) &&
	       between(cap.minCY, cy, cap.maxCY);
}

#define DEVICE_INTERVAL_DIFF_LIMIT 20

static bool FrameRateAvailable(const MediaFoundationVideoInfo &cap,
			       long long interval)
{
	return interval == FPS_HIGHEST || interval == FPS_MATCHING ||
	       between(cap.minInterval - DEVICE_INTERVAL_DIFF_LIMIT, interval,
		       cap.maxInterval + DEVICE_INTERVAL_DIFF_LIMIT);
}

static long long FrameRateInterval(const MediaFoundationVideoInfo &cap,
				   long long desired_interval)
{
	return desired_interval < cap.minInterval
		       ? cap.minInterval
		       : min(desired_interval, cap.maxInterval);
}

void MediaFoundationSourceInput::OnReactivate()
{
	SetActive(true);
}

void MediaFoundationSourceInput::OnVideoData(void *pData, int Size,
					     long long llTimestamp)
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

struct PropertiesData {
	MediaFoundationSourceInput *input;
	vector<MediaFoundationVideoDevice> devices;

	bool GetDevice(MediaFoundationVideoDevice &device,
		       const char *encoded_id) const
	{
		MediaFoundationDeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const MediaFoundationVideoDevice &curDevice : devices) {
			if (deviceId.name == curDevice.name &&
			    deviceId.path == curDevice.path) {
				device = curDevice;
				return true;
			}
		}

		return false;
	}
};

static inline bool ConvertRes(int &cx, int &cy, const char *res)
{
	return sscanf(res, "%dx%d", &cx, &cy) == 2;
}

static inline bool ResolutionValid(const string &res, int &cx, int &cy)
{
	if (!res.size())
		return false;

	return ConvertRes(cx, cy, res.c_str());
}

static inline bool CapsMatch(const MediaFoundationVideoInfo &)
{
	return true;
}

template<typename... F>
static bool CapsMatch(const MediaFoundationVideoDevice &dev, F... fs);

template<typename F, typename... Fs>
static inline bool CapsMatch(const MediaFoundationVideoInfo &info, F &&f,
			     Fs... fs)
{
	return f(info) && CapsMatch(info, fs...);
}

template<typename... F>
static bool CapsMatch(const MediaFoundationVideoDevice &dev, F... fs)
{
	// no early exit, trigger all side effects.
	bool match = false;
	for (const MediaFoundationVideoInfo &info : dev.caps)
		if (CapsMatch(info, fs...))
			match = true;
	return match;
}

static inline bool
MatcherClosestFrameRateSelector(long long interval, long long &best_match,
				const MediaFoundationVideoInfo &info)
{
	long long current = FrameRateInterval(info, interval);
	if (llabs(interval - best_match) > llabs(interval - current))
		best_match = current;
	return true;
}

#define ResolutionMatcher(cx, cy)                                \
	[cx, cy](const MediaFoundationVideoInfo &info) -> bool { \
		return ResolutionAvailable(info, cx, cy);        \
	}
#define FrameRateMatcher(interval)                                 \
	[interval](const MediaFoundationVideoInfo &info) -> bool { \
		return FrameRateAvailable(info, interval);         \
	}
#define ClosestFrameRateSelector(interval, best_match)                        \
	[interval,                                                            \
	 &best_match](const MediaFoundationVideoInfo &info) mutable -> bool { \
		return MatcherClosestFrameRateSelector(interval, best_match,  \
						       info);                 \
	}

static bool ResolutionAvailable(const MediaFoundationVideoDevice &dev, int cx,
				int cy)
{
	return CapsMatch(dev, ResolutionMatcher(cx, cy));
}

static bool DetermineResolution(int &cx, int &cy, obs_data_t *settings,
				MediaFoundationVideoDevice &dev)
{
	const char *res = obs_data_get_autoselect_string(settings, RESOLUTION);
	if (obs_data_has_autoselect_value(settings, RESOLUTION) &&
	    ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	res = obs_data_get_string(settings, RESOLUTION);
	if (ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	res = obs_data_get_string(settings, LAST_RESOLUTION);
	if (ConvertRes(cx, cy, res) && ResolutionAvailable(dev, cx, cy))
		return true;

	return false;
}

static long long GetOBSFPS();

static inline bool IsDecoupled(const MediaFoundationVideoConfig &config)
{
	return wstrstri(config.name.c_str(), L"GV-USB2") != NULL;
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

void MediaFoundationSourceInput::_OnVideoData(void *pData, int Size,
					      long long llTimestamp,
					      void *pUserData)
{
	MediaFoundationSourceInput *pThis =
		(MediaFoundationSourceInput *)pUserData;

	if (pThis) {
		pThis->OnVideoData(pData, Size, llTimestamp);
	}
}

bool MediaFoundationSourceInput::UpdateVideoConfig(obs_data_t *settings)
{
	string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	_deactivateWhenNotShowing = obs_data_get_bool(settings, DEACTIVATE_WNS);
	_flip = obs_data_get_bool(settings, FLIP_IMAGE);
	_autorotation = obs_data_get_bool(settings, AUTOROTATION);

	MediaFoundationDeviceId id;
	if (!DecodeDeviceId(id, video_device_id.c_str())) {
		blog(LOG_WARNING, "%s: DecodeDeviceId failed",
		     obs_source_get_name(source));
		return false;
	}

	PropertiesData data;
	DeviceEnumerator mfenum;
	mfenum.Enumerate(data.devices);

	MediaFoundationVideoDevice dev;
	if (!data.GetDevice(dev, video_device_id.c_str())) {
		blog(LOG_WARNING, "%s: data.GetDevice failed",
		     obs_source_get_name(source));
		return false;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	int cx = 0, cy = 0;
	long long interval = 0;

	if (resType == ResTypeCustom) {
		bool has_autosel_val;
		string resolution = obs_data_get_string(settings, RESOLUTION);
		if (!ResolutionValid(resolution, cx, cy)) {
			blog(LOG_WARNING, "%s: ResolutionValid failed",
			     obs_source_get_name(source));
			return false;
		}

		has_autosel_val =
			obs_data_has_autoselect_value(settings, FRAME_INTERVAL);
		interval = has_autosel_val
				   ? obs_data_get_autoselect_int(settings,
								 FRAME_INTERVAL)
				   : obs_data_get_int(settings, FRAME_INTERVAL);

		if (interval == FPS_MATCHING)
			interval = GetOBSFPS();

		long long best_interval = numeric_limits<long long>::max();
		CapsMatch(dev, ResolutionMatcher(cx, cy),
			  ClosestFrameRateSelector(interval, best_interval),
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
				hr = MF_GetOutputResolution(
					_mfcaptureDevice, &w, &h, &fpsn, &fpsd);
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
			hr = MF_SetOutputResolution(
				_mfcaptureDevice, w, h, interval,
				MF_COLOR_FORMAT::MF_COLOR_FORMAT_ARGB);
			_videoConfig.cx = w;
			_videoConfig.cyAbs = h;
			_videoConfig.frameInterval = interval;
		}
		if (SUCCEEDED(hr)) {
			_firstframe = true;
			hr = MF_Start(_mfcaptureDevice, _OnVideoData, this);
		}

		QueueAction(Action::NpuControl);
	}

	SetupBuffering(settings);

	return true;
}

bool MediaFoundationSourceInput::UpdateVideoProperties(obs_data_t *settings)
{
	OBSDataArrayAutoRelease cca =
		obs_data_get_array(settings, "CameraControl");

	if (cca) {
		std::vector<MediaFoundationVideoDeviceProperty> properties;
		const auto count = obs_data_array_count(cca);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease item = obs_data_array_item(cca, i);
			if (!item)
				continue;

			MediaFoundationVideoDeviceProperty prop{};
			prop.property =
				(long)obs_data_get_int(item, "property");
			prop.flags = (long)obs_data_get_int(item, "flags");
			prop.val = (long)obs_data_get_int(item, "val");
			properties.push_back(prop);
		}

		if (!properties.empty()) {
		}
	}

	OBSDataArrayAutoRelease vpaa =
		obs_data_get_array(settings, "VideoProcAmp");

	if (vpaa) {
		std::vector<MediaFoundationVideoDeviceProperty> properties;
		const auto count = obs_data_array_count(vpaa);

		for (size_t i = 0; i < count; i++) {
			OBSDataAutoRelease item = obs_data_array_item(vpaa, i);
			if (!item)
				continue;

			MediaFoundationVideoDeviceProperty prop{};
			prop.property =
				(long)obs_data_get_int(item, "property");
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

inline enum video_colorspace
MediaFoundationSourceInput::GetColorSpace(obs_data_t *settings) const
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

inline enum video_range_type
MediaFoundationSourceInput::GetColorRange(obs_data_t *settings) const
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
		blog(LOG_WARNING, "%s: Video configuration failed",
		     obs_source_get_name(source));
		return false;
	}

	if (!UpdateVideoProperties(settings)) {
		blog(LOG_WARNING, "%s: Setting video device properties failed",
		     obs_source_get_name(source));
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

	bool success = video_format_get_parameters_for_format(
		cs, range, VIDEO_FORMAT_BGRA, _frame.color_matrix,
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

/* ------------------------------------------------------------------------- */

static const char *GetMediaFoundationSourceInputName(void *)
{
	return TEXT_INPUT_NAME;
}

static void proc_activate(void *data, calldata_t *cd)
{
	bool activate = calldata_bool(cd, "active");
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);
	input->SetActive(activate);
}

static void *CreateMediaFoundationSourceInput(obs_data_t *settings,
					      obs_source_t *source)
{
	MediaFoundationSourceInput *input = nullptr;

	try {
		input = new MediaFoundationSourceInput(source, settings);
		proc_handler_t *ph = obs_source_get_proc_handler(source);
		proc_handler_add(ph, "void activate(bool active)",
				 proc_activate, input);
	} catch (const char *error) {
		blog(LOG_ERROR, "Could not create device '%s': %s",
		     obs_source_get_name(source), error);
	}

	return input;
}

static void DestroyMediaFoundationSourceInput(void *data)
{
	delete reinterpret_cast<MediaFoundationSourceInput *>(data);
}

static void UpdateMediaFoundationSourceInput(void *data, obs_data_t *settings)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);
	if (input->_active)
		input->QueueActivate(settings);
}

static void SaveMediaFoundationSourceInput(void *data, obs_data_t *settings)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);
	if (!input->_active)
		return;

	input->QueueAction(Action::SaveSettings);
	WaitForSingleObject(input->_saved_event, INFINITE);
}

static void GetMediaFoundationSourceDefaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, FRAME_INTERVAL, FPS_MATCHING);
	obs_data_set_default_int(settings, RES_TYPE, ResTypePreferred);
	obs_data_set_default_bool(settings, "active", true);
	obs_data_set_default_string(settings, COLOR_SPACE, "default");
	obs_data_set_default_string(settings, COLOR_RANGE, "default");
	obs_data_set_default_bool(settings, AUTOROTATION, true);
}

struct Resolution {
	int cx, cy;

	inline Resolution(int cx, int cy) : cx(cx), cy(cy) {}
};

static void InsertResolution(vector<Resolution> &resolutions, int cx, int cy)
{
	int bestCY = 0;
	size_t idx = 0;

	for (; idx < resolutions.size(); idx++) {
		const Resolution &res = resolutions[idx];
		if (res.cx > cx)
			break;

		if (res.cx == cx) {
			if (res.cy == cy)
				return;

			if (!bestCY)
				bestCY = res.cy;
			else if (res.cy > bestCY)
				break;
		}
	}

	resolutions.insert(resolutions.begin() + idx, Resolution(cx, cy));
}

static inline void AddCap(vector<Resolution> &resolutions,
			  const MediaFoundationVideoInfo &cap)
{
	InsertResolution(resolutions, cap.minCX, cap.minCY);
	InsertResolution(resolutions, cap.maxCX, cap.maxCY);
}

#define MAKE_MEDIAFOUNDATION_FPS(fps) (10000000LL / (fps))
#define MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(den, num) \
	((num) * 10000000LL / (den))

static long long GetOBSFPS()
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return 0;

	return MAKE_MEDIAFOUNDATION_FRACTIONAL_FPS(ovi.fps_num, ovi.fps_den);
}

struct FPSFormat {
	const char *text;
	long long interval;
};

static const FPSFormat validFPSFormats[] = {
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

static bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p,
				  obs_data_t *settings);

static bool TryResolution(const MediaFoundationVideoDevice &dev,
			  const string &res)
{
	int cx, cy;
	if (!ConvertRes(cx, cy, res.c_str()))
		return false;

	return ResolutionAvailable(dev, cx, cy);
}

static bool SetResolution(obs_properties_t *props, obs_data_t *settings,
			  const string &res, bool autoselect = false)
{
	if (autoselect)
		obs_data_set_autoselect_string(settings, RESOLUTION,
					       res.c_str());
	else
		obs_data_unset_autoselect_value(settings, RESOLUTION);

	DeviceIntervalChanged(props, obs_properties_get(props, FRAME_INTERVAL),
			      settings);

	if (!autoselect)
		obs_data_set_string(settings, LAST_RESOLUTION, res.c_str());
	return true;
}

static bool DeviceResolutionChanged(obs_properties_t *props, obs_property_t *p,
				    obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	const char *id;
	MediaFoundationVideoDevice device;

	id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string res = obs_data_get_string(settings, RESOLUTION);
	string last_res = obs_data_get_string(settings, LAST_RESOLUTION);

	if (!data->GetDevice(device, id))
		return false;

	if (TryResolution(device, res))
		return SetResolution(props, settings, res);

	if (TryResolution(device, last_res))
		return SetResolution(props, settings, last_res, true);

	return false;
}

static bool ResTypeChanged(obs_properties_t *props, obs_property_t *p,
			   obs_data_t *settings);

static size_t AddDevice(obs_property_t *device_list, const string &id)
{
	DStr name, path;
	if (!DecodeDeviceDStr(name, path, id.c_str()))
		return numeric_limits<size_t>::max();

	return obs_property_list_add_string(device_list, name, id.c_str());
}

static bool UpdateDeviceList(obs_property_t *list, const string &id)
{
	size_t size = obs_property_list_item_count(list);
	bool found = false;
	bool disabled_unknown_found = false;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_string(list, i) == id) {
			found = true;
			continue;
		}
		if (obs_property_list_item_disabled(list, i))
			disabled_unknown_found = true;
	}

	if (!found && !disabled_unknown_found) {
		size_t idx = AddDevice(list, id);
		obs_property_list_item_disable(list, idx, true);
		return true;
	}

	if (found && !disabled_unknown_found)
		return false;

	for (size_t i = 0; i < size;) {
		if (obs_property_list_item_disabled(list, i)) {
			obs_property_list_item_remove(list, i);
			continue;
		}
		i += 1;
	}

	return true;
}

static bool DeviceSelectionChanged(obs_properties_t *props, obs_property_t *p,
				   obs_data_t *settings)
{
	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	MediaFoundationVideoDevice device;

	string id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string old_id = obs_data_get_string(settings, LAST_VIDEO_DEV_ID);

	bool device_list_updated = UpdateDeviceList(p, id);

	if (!data->GetDevice(device, id.c_str()))
		return !device_list_updated;

	vector<Resolution> resolutions;
	for (const MediaFoundationVideoInfo &cap : device.caps)
		AddCap(resolutions, cap);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_list_clear(p);

	for (size_t idx = resolutions.size(); idx > 0; idx--) {
		const Resolution &res = resolutions[idx - 1];

		string strRes;
		strRes += to_string(res.cx);
		strRes += "x";
		strRes += to_string(res.cy);

		obs_property_list_add_string(p, strRes.c_str(), strRes.c_str());
	}

	/* only refresh properties if device legitimately changed */
	if (!id.size() || !old_id.size() || id != old_id) {
		p = obs_properties_get(props, RES_TYPE);
		ResTypeChanged(props, p, settings);
		obs_data_set_string(settings, LAST_VIDEO_DEV_ID, id.c_str());
	}

	return true;
}

static bool AddDevice(obs_property_t *device_list,
		      const MediaFoundationVideoDevice &device)
{
	DStr name, path, device_id;

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

static void PropertiesDataDestroy(void *data)
{
	delete reinterpret_cast<PropertiesData *>(data);
}

static bool ResTypeChanged(obs_properties_t *props, obs_property_t *p,
			   obs_data_t *settings)
{
	int val = (int)obs_data_get_int(settings, RES_TYPE);
	bool enabled = (val != ResTypePreferred);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, FRAME_INTERVAL);
	obs_property_set_enabled(p, enabled);

	if (val == ResTypeCustom) {
		p = obs_properties_get(props, RESOLUTION);
		DeviceResolutionChanged(props, p, settings);
	} else {
		obs_data_unset_autoselect_value(settings, FRAME_INTERVAL);
	}

	return true;
}

static DStr GetFPSName(long long interval)
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
		if (format.interval != interval)
			continue;

		dstr_cat(name, format.text);
		return name;
	}

	dstr_cat(name, to_string(10000000. / interval).c_str());
	return name;
}

static void UpdateFPS(MediaFoundationVideoDevice &device, long long interval,
		      int cx, int cy, obs_properties_t *props)
{
	obs_property_t *list = obs_properties_get(props, FRAME_INTERVAL);

	obs_property_list_clear(list);

	obs_property_list_add_int(list, TEXT_FPS_MATCHING, FPS_MATCHING);
	obs_property_list_add_int(list, TEXT_FPS_HIGHEST, FPS_HIGHEST);

	bool interval_added = interval == FPS_HIGHEST ||
			      interval == FPS_MATCHING;
	for (const FPSFormat &fps_format : validFPSFormats) {
		long long format_interval = fps_format.interval;

		bool available = CapsMatch(device, ResolutionMatcher(cx, cy),
					   FrameRateMatcher(format_interval));

		if (!available && interval != fps_format.interval)
			continue;

		if (interval == fps_format.interval)
			interval_added = true;

		size_t idx = obs_property_list_add_int(list, fps_format.text,
						       fps_format.interval);
		obs_property_list_item_disable(list, idx, !available);
	}

	if (interval_added)
		return;

	size_t idx =
		obs_property_list_add_int(list, GetFPSName(interval), interval);
	obs_property_list_item_disable(list, idx, true);
}

static bool UpdateFPS(long long interval, obs_property_t *list)
{
	size_t size = obs_property_list_item_count(list);
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_int(list, i) != interval)
			continue;

		obs_property_list_item_disable(list, i, true);
		if (size == 1)
			return false;

		dstr_cat(name, obs_property_list_item_name(list, i));
		break;
	}

	obs_property_list_clear(list);

	if (!name->len)
		name = GetFPSName(interval);

	obs_property_list_add_int(list, name, interval);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

static bool DeviceIntervalChanged(obs_properties_t *props, obs_property_t *p,
				  obs_data_t *settings)
{
	long long val = obs_data_get_int(settings, FRAME_INTERVAL);

	PropertiesData *data =
		(PropertiesData *)obs_properties_get_param(props);
	const char *id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	MediaFoundationVideoDevice device;

	if (!data->GetDevice(device, id))
		return UpdateFPS(val, p);

	int cx = 0, cy = 0;
	if (!DetermineResolution(cx, cy, settings, device)) {
		UpdateFPS(device, 0, 0, 0, props);
		return true;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	if (resType != ResTypeCustom)
		return true;

	if (val == FPS_MATCHING)
		val = GetOBSFPS();

	long long best_interval = numeric_limits<long long>::max();
	bool frameRateSupported =
		CapsMatch(device, ResolutionMatcher(cx, cy),
			  ClosestFrameRateSelector(val, best_interval),
			  FrameRateMatcher(val));

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
			obs_data_set_autoselect_int(settings, FRAME_INTERVAL,
						    listed_val);
			val = listed_val;
		}

	} else {
		obs_data_unset_autoselect_value(settings, FRAME_INTERVAL);
	}

	UpdateFPS(device, val, cx, cy, props);

	return true;
}

static bool ActivateClicked(obs_properties_t *, obs_property_t *p, void *data)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);

	if (input->_active) {
		input->SetActive(false);
		obs_property_set_description(p, TEXT_ACTIVATE);
	} else {
		input->SetActive(true);
		obs_property_set_description(p, TEXT_DEACTIVATE);
	}

	return true;
}

static obs_properties_t *GetMediaFoundationSourceProperties(void *obj)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(obj);
	obs_properties_t *ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	data->input = input;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	obs_property_t *p = obs_properties_add_list(ppts, VIDEO_DEVICE_ID,
						    TEXT_DEVICE,
						    OBS_COMBO_TYPE_LIST,
						    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceSelectionChanged);

	DeviceEnumerator mfenum;
	mfenum.Enumerate(data->devices);

	for (const MediaFoundationVideoDevice &device : data->devices)
		AddDevice(p, device);

	const char *activateText = TEXT_ACTIVATE;
	if (input) {
		if (input->_active)
			activateText = TEXT_DEACTIVATE;
	}

	obs_properties_add_button(ppts, "activate", activateText,
				  ActivateClicked);

	obs_properties_add_bool(ppts, DEACTIVATE_WNS, TEXT_DWNS);

	/* ------------------------------------- */
	/* video settings */

	p = obs_properties_add_list(ppts, RES_TYPE, TEXT_RES_FPS_TYPE,
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, ResTypeChanged);

	obs_property_list_add_int(p, TEXT_PREFERRED_RES, ResTypePreferred);
	obs_property_list_add_int(p, TEXT_CUSTOM_RES, ResTypeCustom);

	p = obs_properties_add_list(ppts, RESOLUTION, TEXT_RESOLUTION,
				    OBS_COMBO_TYPE_EDITABLE,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceResolutionChanged);

	p = obs_properties_add_list(ppts, FRAME_INTERVAL, "FPS",
				    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, DeviceIntervalChanged);

	obs_property_set_long_description(p,
					  obs_module_text("Buffering.ToolTip"));

	obs_properties_add_bool(ppts, FLIP_IMAGE, TEXT_FLIP_IMAGE);

	return ppts;
}

static void HideMediaFoundationSourceInput(void *data)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);

	if (input->_deactivateWhenNotShowing && input->_active)
		input->QueueAction(Action::Deactivate);
}

static void ShowMediaFoundationSourceInput(void *data)
{
	MediaFoundationSourceInput *input =
		reinterpret_cast<MediaFoundationSourceInput *>(data);

	if (input->_deactivateWhenNotShowing && input->_active)
		input->QueueAction(Action::Activate);
}

void RegisterMediaFoundationSource()
{
	obs_source_info info = {};
	info.id = "MediaFoundationsource_input";
	info.type = OBS_SOURCE_TYPE_INPUT;

	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC |
			    OBS_SOURCE_DO_NOT_DUPLICATE;

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
