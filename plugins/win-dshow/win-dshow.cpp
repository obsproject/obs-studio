#include <objbase.h>

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/util.hpp>
#include <util/platform.h>
#include "libdshowcapture/dshowcapture.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <string>
#include <vector>

/*
 * TODO:
 *   - handle disconnections and reconnections
 *   - if device not present, wait for device to be plugged in
 */

#undef min
#undef max

using namespace std;
using namespace DShow;

/* settings defines that will cause errors if there are typos */
#define VIDEO_DEVICE_ID   "video_device_id"
#define RES_TYPE          "res_type"
#define RESOLUTION        "resolution"
#define FRAME_INTERVAL    "frame_interval"
#define VIDEO_FORMAT      "video_format"
#define LAST_VIDEO_DEV_ID "last_video_device_id"
#define LAST_RESOLUTION   "last_resolution"

#define TEXT_INPUT_NAME     obs_module_text("VideoCaptureDevice")
#define TEXT_DEVICE         obs_module_text("Device")
#define TEXT_CONFIG_VIDEO   obs_module_text("ConfigureVideo")
#define TEXT_CONFIG_XBAR    obs_module_text("ConfigureCrossbar")
#define TEXT_RES_FPS_TYPE   obs_module_text("ResFPSType")
#define TEXT_CUSTOM_RES     obs_module_text("ResFPSType.Custom")
#define TEXT_PREFERRED_RES  obs_module_text("ResFPSType.DevPreferred")
#define TEXT_FPS_MATCHING   obs_module_text("FPS.Matching")
#define TEXT_FPS_HIGHEST    obs_module_text("FPS.Highest")
#define TEXT_RESOLUTION     obs_module_text("Resolution")
#define TEXT_VIDEO_FORMAT   obs_module_text("VideoFormat")
#define TEXT_FORMAT_UNKNOWN obs_module_text("VideoFormat.Unknown")

enum ResType {
	ResType_Preferred,
	ResType_Custom
};

struct DShowInput {
	obs_source_t source;
	Device       device;
	bool         comInitialized;

	VideoConfig  videoConfig;
	AudioConfig  audioConfig;

	obs_source_frame frame;

	inline DShowInput(obs_source_t source_)
		: source         (source_),
		  device         (InitGraph::False),
		  comInitialized (false)
	{}

	static void OnVideoData(DShowInput *input, unsigned char *data,
			size_t size, long long startTime, long long endTime);

	void Update(obs_data_t settings);
};

#define FPS_HIGHEST   0LL
#define FPS_MATCHING -1LL

template <typename T, typename U, typename V>
static bool between(T &&lower, U &&value, V &&upper)
{
	return value >= lower && value <= upper;
}

static bool ResolutionAvailable(const VideoInfo &cap, int cx, int cy)
{
	return between(cap.minCX, cx, cap.maxCX) &&
		between(cap.minCY, cy, cap.maxCY);
}

#define DEVICE_INTERVAL_DIFF_LIMIT 20

static bool FrameRateAvailable(const VideoInfo &cap, long long interval)
{
	return interval == FPS_HIGHEST || interval == FPS_MATCHING ||
		between(cap.minInterval - DEVICE_INTERVAL_DIFF_LIMIT,
				interval,
				cap.maxInterval + DEVICE_INTERVAL_DIFF_LIMIT);
}

static long long FrameRateInterval(const VideoInfo &cap,
		long long desired_interval)
{
	return desired_interval < cap.minInterval ?
		cap.minInterval :
		min(desired_interval, cap.maxInterval);
}

void encode_dstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

void decode_dstr(struct dstr *str)
{
	dstr_replace(str, "#3A", ":");
	dstr_replace(str, "#22", "#");
}

static inline video_format ConvertVideoFormat(VideoFormat format)
{
	switch (format) {
	case VideoFormat::ARGB:  return VIDEO_FORMAT_BGRA;
	case VideoFormat::XRGB:  return VIDEO_FORMAT_BGRX;
	case VideoFormat::I420:  return VIDEO_FORMAT_I420;
	case VideoFormat::NV12:  return VIDEO_FORMAT_NV12;
	case VideoFormat::YVYU:  return VIDEO_FORMAT_UYVY;
	case VideoFormat::YUY2:  return VIDEO_FORMAT_YUY2;
	case VideoFormat::UYVY:  return VIDEO_FORMAT_YVYU;
	case VideoFormat::MJPEG: return VIDEO_FORMAT_YUY2;
	default:                 return VIDEO_FORMAT_NONE;
	}
}

void DShowInput::OnVideoData(DShowInput *input, unsigned char *data,
		size_t size, long long startTime, long long endTime)
{
	const int cx = input->videoConfig.cx;
	const int cy = input->videoConfig.cy;

	input->frame.timestamp = (uint64_t)startTime * 100;

	if (input->videoConfig.format == VideoFormat::XRGB ||
	    input->videoConfig.format == VideoFormat::ARGB) {
		input->frame.data[0]     = data;
		input->frame.linesize[0] = cx * 4;

	} else if (input->videoConfig.format == VideoFormat::YVYU ||
	           input->videoConfig.format == VideoFormat::YUY2 ||
	           input->videoConfig.format == VideoFormat::UYVY) {
		input->frame.data[0]     = data;
		input->frame.linesize[0] = cx * 2;

	} else if (input->videoConfig.format == VideoFormat::I420) {
		input->frame.data[0] = data;
		input->frame.data[1] = input->frame.data[0] + (cx * cy);
		input->frame.data[2] = input->frame.data[1] + (cx * cy / 4);
		input->frame.linesize[0] = cx;
		input->frame.linesize[1] = cx / 2;
		input->frame.linesize[2] = cx / 2;

	} else {
		/* TODO: other formats */
		return;
	}

	obs_source_output_video(input->source, &input->frame);

	UNUSED_PARAMETER(endTime); /* it's the enndd tiimmes! */
	UNUSED_PARAMETER(size);
}

static bool DecodeDeviceId(DStr &name, DStr &path, const char *device_id)
{
	const char *path_str;

	if (!device_id || !*device_id)
		return false;

	path_str = strchr(device_id, ':');
	if (!path_str)
		return false;

	dstr_copy(path, path_str+1);
	dstr_copy(name, device_id);

	size_t len = path_str - device_id;
	name->array[len] = 0;
	name->len        = len;

	decode_dstr(name);
	decode_dstr(path);

	return true;
}

static bool DecodeDeviceId(DeviceId &out, const char *device_id)
{
	DStr name, path;

	if (!DecodeDeviceId(name, path, device_id))
		return false;

	BPtr<wchar_t> wname = dstr_to_wcs(name);
	out.name = wname;

	if (!dstr_is_empty(path)) {
		BPtr<wchar_t> wpath = dstr_to_wcs(path);
		out.path = wpath;
	}

	return true;
}

struct PropertiesData {
	vector<VideoDevice> devices;

	bool GetDevice(VideoDevice &device, const char *encoded_id) const
	{
		DeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const VideoDevice &curDevice : devices) {
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

static bool FormatMatches(VideoFormat left, VideoFormat right)
{
	return left == VideoFormat::Any || right == VideoFormat::Any ||
		left == right;
}

static bool ResolutionValid(string res, int &cx, int &cy)
{
	if (!res.size())
		return false;

	return ConvertRes(cx, cy, res.c_str());
}

template <typename F, typename ... Fs>
static bool CapsMatch(const VideoInfo &info, F&& f, Fs ... fs)
{
	return f(info) && CapsMatch(info, fs ...);
}

static bool CapsMatch(const VideoInfo&)
{
	return true;
}

template <typename ... F>
static bool CapsMatch(const VideoDevice &dev, F ... fs)
{
	auto matcher = [&](const VideoInfo &info)
	{
		return CapsMatch(info, fs ...);
	};

	return any_of(begin(dev.caps), end(dev.caps), matcher);
}

bool MatcherMatchVideoFormat(VideoFormat format, bool &did_match,
		const VideoInfo &info)
{
	bool match = FormatMatches(format, info.format);
	did_match = did_match || match;
	return match;
}

bool MatcherClosestFrameRateSelector(long long interval, long long &best_match,
		const VideoInfo &info)
{
	long long current = FrameRateInterval(info, interval);
	if (llabs(interval - best_match) > llabs(interval - current))
		best_match = current;
	return true;
}

#if 0
auto ResolutionMatcher = [](int cx, int cy)
{
	return [cx, cy](const VideoInfo &info)
	{
		return ResolutionAvailable(info, cx, cy);
	};
};

auto FrameRateMatcher = [](long long interval)
{
	return [interval](const VideoInfo &info)
	{
		return FrameRateAvailable(info, interval);
	};
};

auto VideoFormatMatcher = [](VideoFormat format, bool &did_match)
{
	return [format, &did_match](const VideoInfo &info)
	{
		return MatcherMatchVideoFormat(format, did_match, info);
	};
};

auto ClosestFrameRateSelector = [](long long interval, long long &best_match)
{
	return [interval, &best_match](const VideoInfo &info) mutable -> bool
	{
		MatcherClosestFrameRateSelector(interval, best_match, info);
	};
}
#else
#define ResolutionMatcher(cx, cy) \
	[cx, cy](const VideoInfo &info) -> bool \
	{ return ResolutionAvailable(info, cx, cy); }
#define FrameRateMatcher(interval) \
	[interval](const VideoInfo &info) -> bool \
	{ return FrameRateAvailable(info, interval); }
#define VideoFormatMatcher(format, did_match) \
	[format, &did_match](const VideoInfo &info) mutable -> bool \
	{ return MatcherMatchVideoFormat(format, did_match, info); }
#define ClosestFrameRateSelector(interval, best_match) \
	[interval, &best_match](const VideoInfo &info) mutable -> bool \
	{ return MatcherClosestFrameRateSelector(interval, best_match, info); }
#endif

static bool ResolutionAvailable(const VideoDevice &dev, int cx, int cy)
{
	return CapsMatch(dev, ResolutionMatcher(cx, cy));
}

static bool DetermineResolution(int &cx, int &cy, obs_data_t settings,
		VideoDevice dev)
{
	const char *res = obs_data_get_autoselect_string(settings, RESOLUTION);
	if (obs_data_has_autoselect_value(settings, RESOLUTION) &&
			ConvertRes(cx, cy, res) &&
			ResolutionAvailable(dev, cx, cy))
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

void DShowInput::Update(obs_data_t settings)
{
	string video_device_id = obs_data_get_string(settings, VIDEO_DEVICE_ID);

	if (!comInitialized) {
		CoInitialize(nullptr);
		comInitialized = true;
	}

	if (!device.ResetGraph())
		return;

	DeviceId id;
	if (!DecodeDeviceId(id, video_device_id.c_str()))
		return;

	PropertiesData data;
	Device::EnumVideoDevices(data.devices);
	VideoDevice dev;
	if (!data.GetDevice(dev, video_device_id.c_str()))
		return;

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	int cx = 0, cy = 0;
	long long interval = 0;
	VideoFormat format = VideoFormat::Any;

	if (resType == ResType_Custom) {
		bool has_autosel_val;
		string resolution = obs_data_get_string(settings, RESOLUTION);
		if (!ResolutionValid(resolution, cx, cy))
			return;

		has_autosel_val = obs_data_has_autoselect_value(settings,
				FRAME_INTERVAL);
		interval = has_autosel_val ?
			obs_data_get_autoselect_int(settings, FRAME_INTERVAL) :
			obs_data_get_int(settings, FRAME_INTERVAL);

		if (interval == FPS_MATCHING)
			interval = GetOBSFPS();

		format = (VideoFormat)obs_data_get_int(settings, VIDEO_FORMAT);

		long long best_interval = numeric_limits<long long>::max();
		bool video_format_match = false;
		if (!CapsMatch(dev,
			ResolutionMatcher(cx, cy),
			VideoFormatMatcher(format, video_format_match),
			ClosestFrameRateSelector(interval, best_interval),
			FrameRateMatcher(interval)) && !video_format_match)
			return;

		interval = best_interval;
		blog(LOG_INFO, "%s: Using interval %lld",
				obs_source_get_name(source), interval);
	}

	videoConfig.name             = id.name.c_str();
	videoConfig.path             = id.path.c_str();
	videoConfig.callback         = CaptureProc(DShowInput::OnVideoData);
	videoConfig.param            = this;
	videoConfig.useDefaultConfig = resType == ResType_Preferred;
	videoConfig.cx               = cx;
	videoConfig.cy               = cy;
	videoConfig.frameInterval    = interval;
	videoConfig.internalFormat   = format;

	if (videoConfig.internalFormat != VideoFormat::MJPEG)
		videoConfig.format = videoConfig.internalFormat;

	device.SetVideoConfig(&videoConfig);

	if (videoConfig.internalFormat == VideoFormat::MJPEG) {
		videoConfig.format   = VideoFormat::XRGB;
		device.SetVideoConfig(&videoConfig);
	}

	if (!device.ConnectFilters())
		return;

	if (device.Start() != Result::Success)
		return;

	frame.width      = videoConfig.cx;
	frame.height     = videoConfig.cy;
	frame.format     = ConvertVideoFormat(videoConfig.format);
	frame.full_range = false;
	frame.flip       = (videoConfig.format == VideoFormat::XRGB ||
	                    videoConfig.format == VideoFormat::ARGB);

	if (!video_format_get_parameters(VIDEO_CS_601, VIDEO_RANGE_PARTIAL,
			frame.color_matrix,
			frame.color_range_min,
			frame.color_range_max)) {
		blog(LOG_ERROR, "Failed to get video format parameters for " \
		                "video format %u", VIDEO_CS_601);
	}
}

/* ------------------------------------------------------------------------- */

static const char *GetDShowInputName(void)
{
	return TEXT_INPUT_NAME;
}

static void *CreateDShowInput(obs_data_t settings, obs_source_t source)
{
	DShowInput *dshow = new DShowInput(source);

	/* causes a deferred update in the video thread */
	obs_source_update(source, nullptr);

	UNUSED_PARAMETER(settings);
	return dshow;
}

static void DestroyDShowInput(void *data)
{
	delete reinterpret_cast<DShowInput*>(data);
}

static uint32_t GetDShowWidth(void *data)
{
	return reinterpret_cast<DShowInput*>(data)->videoConfig.cx;
}

static uint32_t GetDShowHeight(void *data)
{
	return reinterpret_cast<DShowInput*>(data)->videoConfig.cy;
}

static void UpdateDShowInput(void *data, obs_data_t settings)
{
	reinterpret_cast<DShowInput*>(data)->Update(settings);
}

static void GetDShowDefaults(obs_data_t settings)
{
	obs_data_set_default_int(settings, FRAME_INTERVAL, FPS_MATCHING);
	obs_data_set_default_int(settings, RES_TYPE, ResType_Preferred);
	obs_data_set_default_int(settings, VIDEO_FORMAT, (int)VideoFormat::Any);
}

struct Resolution {
	int cx, cy;

	inline Resolution(int cx, int cy) : cx(cx), cy(cy) {}
};

static void InsertResolution(vector<Resolution> &resolutions, int cx, int cy)
{
	int    bestCY = 0;
	size_t idx    = 0;

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

static inline void AddCap(vector<Resolution> &resolutions, const VideoInfo &cap)
{
	InsertResolution(resolutions, cap.minCX, cap.minCY);
	InsertResolution(resolutions, cap.maxCX, cap.maxCY);
}

#define MAKE_DSHOW_FPS(fps)                 (10000000LL/(fps))
#define MAKE_DSHOW_FRACTIONAL_FPS(den, num) ((num)*10000000LL/(den))

static long long GetOBSFPS()
{
	obs_video_info ovi;
	if (!obs_get_video_info(&ovi))
		return 0;

	return MAKE_DSHOW_FRACTIONAL_FPS(ovi.fps_num, ovi.fps_den);
}

struct FPSFormat {
	const char *text;
	long long  interval;
};

static const FPSFormat validFPSFormats[] = {
	{"60",         MAKE_DSHOW_FPS(60)},
	{"59.94 NTSC", MAKE_DSHOW_FRACTIONAL_FPS(60000, 1001)},
	{"50",         MAKE_DSHOW_FPS(50)},
	{"48 film",    MAKE_DSHOW_FRACTIONAL_FPS(48000, 1001)},
	{"40",         MAKE_DSHOW_FPS(40)},
	{"30",         MAKE_DSHOW_FPS(30)},
	{"29.97 NTSC", MAKE_DSHOW_FRACTIONAL_FPS(30000, 1001)},
	{"25",         MAKE_DSHOW_FPS(25)},
	{"24 film",    MAKE_DSHOW_FRACTIONAL_FPS(24000, 1001)},
	{"20",         MAKE_DSHOW_FPS(20)},
	{"15",         MAKE_DSHOW_FPS(15)},
	{"10",         MAKE_DSHOW_FPS(10)},
	{"5",          MAKE_DSHOW_FPS(5)},
	{"4",          MAKE_DSHOW_FPS(4)},
	{"3",          MAKE_DSHOW_FPS(3)},
	{"2",          MAKE_DSHOW_FPS(2)},
	{"1",          MAKE_DSHOW_FPS(1)},
};

static bool DeviceIntervalChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings);

static bool TryResolution(VideoDevice &dev, string res)
{
	int cx, cy;
	if (!ConvertRes(cx, cy, res.c_str()))
		return false;

	return ResolutionAvailable(dev, cx, cy);
}

static bool SetResolution(obs_properties_t props, obs_data_t settings,
		string res, bool autoselect=false)
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

static bool DeviceResolutionChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	UNUSED_PARAMETER(p);

	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *id;
	VideoDevice device;

	id       = obs_data_get_string(settings, VIDEO_DEVICE_ID);
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

struct VideoFormatName {
	VideoFormat format;
	const char  *name;
};

static const VideoFormatName videoFormatNames[] = {
	/* autoselect format*/
	{VideoFormat::Any,   "VideoFormat.Any"},

	/* raw formats */
	{VideoFormat::ARGB,  "ARGB"},
	{VideoFormat::XRGB,  "XRGB"},

	/* planar YUV formats */
	{VideoFormat::I420,  "I420"},
	{VideoFormat::NV12,  "NV12"},

	/* packed YUV formats */
	{VideoFormat::YVYU,  "YVYU"},
	{VideoFormat::YUY2,  "YUY2"},
	{VideoFormat::UYVY,  "UYVY"},
	{VideoFormat::HDYC,  "HDYV"},

	/* encoded formats */
	{VideoFormat::MPEG2, "MPEG2"},
	{VideoFormat::MJPEG, "MJPEG"},
	{VideoFormat::H264,  "H264"}
};

static bool ResTypeChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings);

static size_t AddDevice(obs_property_t device_list, const string &id)
{
	DStr name, path;
	if (!DecodeDeviceId(name, path, id.c_str()))
		return numeric_limits<size_t>::max();

	return obs_property_list_add_string(device_list, name, id.c_str());
}

static bool UpdateDeviceList(obs_property_t list, const string &id)
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

static bool DeviceSelectionChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	VideoDevice device;

	string id     = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	string old_id = obs_data_get_string(settings, LAST_VIDEO_DEV_ID);

	bool device_list_updated = UpdateDeviceList(p, id);

	if (!data->GetDevice(device, id.c_str()))
		return !device_list_updated;

	vector<Resolution> resolutions;
	for (const VideoInfo &cap : device.caps)
		AddCap(resolutions, cap);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_list_clear(p);

	for (size_t idx = resolutions.size(); idx > 0; idx--) {
		const Resolution &res = resolutions[idx-1];

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

static bool VideoConfigClicked(obs_properties_t props, obs_property_t p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->device.OpenDialog(nullptr, DialogType::ConfigVideo);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}

/*static bool AudioConfigClicked(obs_properties_t props, obs_property_t p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->device.OpenDialog(nullptr, DialogType::ConfigAudio);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}*/

static bool CrossbarConfigClicked(obs_properties_t props, obs_property_t p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->device.OpenDialog(nullptr, DialogType::ConfigCrossbar);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}

/*static bool Crossbar2ConfigClicked(obs_properties_t props, obs_property_t p,
		void *data)
{
	DShowInput *input = reinterpret_cast<DShowInput*>(data);
	input->device.OpenDialog(nullptr, DialogType::ConfigCrossbar2);

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	return false;
}*/

static bool AddDevice(obs_property_t device_list, const VideoDevice &device)
{
	DStr name, path, device_id;

	dstr_from_wcs(name, device.name.c_str());
	dstr_from_wcs(path, device.path.c_str());

	encode_dstr(path);

	dstr_copy_dstr(device_id, name);
	encode_dstr(device_id);
	dstr_cat(device_id, ":");
	dstr_cat_dstr(device_id, path);

	obs_property_list_add_string(device_list, name, device_id);

	return true;
}

static void PropertiesDataDestroy(void *data)
{
	delete reinterpret_cast<PropertiesData*>(data);
}

static bool ResTypeChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	int  val     = (int)obs_data_get_int(settings, RES_TYPE);
	bool enabled = (val != ResType_Preferred);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, FRAME_INTERVAL);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, VIDEO_FORMAT);
	obs_property_set_enabled(p, enabled);

	if (val == ResType_Custom) {
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

static void UpdateFPS(VideoDevice &device, VideoFormat format,
		long long interval, int cx, int cy, obs_properties_t props)
{
	obs_property_t list = obs_properties_get(props, FRAME_INTERVAL);

	obs_property_list_clear(list);

	obs_property_list_add_int(list, TEXT_FPS_MATCHING, FPS_MATCHING);
	obs_property_list_add_int(list, TEXT_FPS_HIGHEST,  FPS_HIGHEST);

	bool interval_added = interval == FPS_HIGHEST ||
				interval == FPS_MATCHING;
	for (const FPSFormat &fps_format : validFPSFormats) {
		bool video_format_match = false;
		long long format_interval = fps_format.interval;

		bool available = CapsMatch(device,
				ResolutionMatcher(cx, cy),
				VideoFormatMatcher(format, video_format_match),
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

	size_t idx = obs_property_list_add_int(list, GetFPSName(interval),
			interval);
	obs_property_list_item_disable(list, idx, true);
}

static DStr GetVideoFormatName(VideoFormat format)
{
	DStr name;
	for (const VideoFormatName &format_ : videoFormatNames) {
		if (format_.format == format) {
			dstr_cat(name, obs_module_text(format_.name));
			return name;
		}
	}

	dstr_cat(name, TEXT_FORMAT_UNKNOWN);
	dstr_replace(name, "%1", std::to_string((long long)format).c_str());
	return name;
}

static void UpdateVideoFormats(VideoDevice &device, VideoFormat format_,
		int cx, int cy, long long interval, obs_properties_t props)
{
	set<VideoFormat> formats = { VideoFormat::Any };
	auto format_gatherer = [&formats](const VideoInfo &info) mutable -> bool
	{
		formats.insert(info.format);
		return false;
	};

	CapsMatch(device,
			ResolutionMatcher(cx, cy),
			FrameRateMatcher(interval),
			format_gatherer);

	obs_property_t list = obs_properties_get(props, VIDEO_FORMAT);
	obs_property_list_clear(list);

	bool format_added = false;
	for (const VideoFormatName &format : videoFormatNames) {
		bool available = formats.find(format.format) != end(formats);

		if (!available && format.format != format_)
			continue;

		if (format.format == format_)
			format_added = true;

		size_t idx = obs_property_list_add_int(list,
				obs_module_text(format.name),
				(long long)format.format);
		obs_property_list_item_disable(list, idx, !available);
	}

	if (format_added)
		return;

	size_t idx = obs_property_list_add_int(list,
			GetVideoFormatName(format_), (long long)format_);
	obs_property_list_item_disable(list, idx, true);
}

static bool UpdateFPS(long long interval, obs_property_t list)
{
	size_t size = obs_property_list_item_count(list);
	bool fps_found = false;
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if (obs_property_list_item_int(list, i) != interval)
			continue;

		obs_property_list_item_disable(list, i, true);
		if (size == 1)
			return false;

		dstr_cat(name, obs_property_list_item_name(list, i));
		fps_found = true;
		break;
	}

	obs_property_list_clear(list);

	if (!name->len)
		name = GetFPSName(interval);

	obs_property_list_add_int(list, name, interval);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

static bool DeviceIntervalChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	long long val = obs_data_get_int(settings, FRAME_INTERVAL);

	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	VideoDevice device;

	if (!data->GetDevice(device, id))
		return UpdateFPS(val, p);

	int cx = 0, cy = 0;
	if (!DetermineResolution(cx, cy, settings, device)) {
		UpdateVideoFormats(device, VideoFormat::Any, 0, 0, 0, props);
		UpdateFPS(device, VideoFormat::Any, 0, 0, 0, props);
		return true;
	}

	int resType = (int)obs_data_get_int(settings, RES_TYPE);
	if (resType != ResType_Custom)
		return true;

	if (val == FPS_MATCHING)
		val = GetOBSFPS();

	VideoFormat format = (VideoFormat)obs_data_get_int(settings,
							VIDEO_FORMAT);

	bool video_format_matches = false;
	long long best_interval = numeric_limits<long long>::max();
	bool frameRateSupported = CapsMatch(device,
			ResolutionMatcher(cx, cy),
			VideoFormatMatcher(format, video_format_matches),
			ClosestFrameRateSelector(val, best_interval),
			FrameRateMatcher(val));

	if (video_format_matches &&
			!frameRateSupported &&
			best_interval != val) {
		long long listed_val = 0;
		for (const FPSFormat &format : validFPSFormats) {
			long long diff = llabs(format.interval - best_interval);
			if (diff < DEVICE_INTERVAL_DIFF_LIMIT) {
				listed_val = format.interval;
				break;
			}
		}

		if (listed_val != val)
			obs_data_set_autoselect_int(settings, FRAME_INTERVAL,
					listed_val);

	} else {
		obs_data_unset_autoselect_value(settings, FRAME_INTERVAL);
	}

	UpdateVideoFormats(device, format, cx, cy, val, props);
	UpdateFPS(device, format, val, cx, cy, props);

	UNUSED_PARAMETER(p);
	return true;
}

static bool UpdateVideoFormats(VideoFormat format, obs_property_t list)
{
	size_t size = obs_property_list_item_count(list);
	DStr name;

	for (size_t i = 0; i < size; i++) {
		if ((VideoFormat)obs_property_list_item_int(list, i) != format)
			continue;

		if (size == 1)
			return false;

		dstr_cat(name, obs_property_list_item_name(list, i));
		break;
	}

	obs_property_list_clear(list);

	if (!name->len)
		name = GetVideoFormatName(format);

	obs_property_list_add_int(list, name, (long long)format);
	obs_property_list_item_disable(list, 0, true);

	return true;
}

static bool VideoFormatChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *id = obs_data_get_string(settings, VIDEO_DEVICE_ID);
	VideoDevice device;

	VideoFormat curFormat =
		(VideoFormat)obs_data_get_int(settings, VIDEO_FORMAT);

	if (!data->GetDevice(device, id))
		return UpdateVideoFormats(curFormat, p);

	int cx, cy;
	if (!DetermineResolution(cx, cy, settings, device)) {
		UpdateVideoFormats(device, VideoFormat::Any, cx, cy, 0, props);
		UpdateFPS(device, VideoFormat::Any, 0, 0, 0, props);
		return true;
	}

	long long interval = obs_data_get_int(settings, FRAME_INTERVAL);

	UpdateVideoFormats(device, curFormat, cx, cy, interval, props);
	UpdateFPS(device, curFormat, interval, cx, cy, props);
	return true;
}

static obs_properties_t GetDShowProperties(void)
{
	obs_properties_t ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	obs_property_t p = obs_properties_add_list(ppts,
			VIDEO_DEVICE_ID, TEXT_DEVICE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceSelectionChanged);

	Device::EnumVideoDevices(data->devices);
	for (const VideoDevice &device : data->devices)
		AddDevice(p, device);

	obs_properties_add_button(ppts, "video_config", TEXT_CONFIG_VIDEO,
			VideoConfigClicked);
	obs_properties_add_button(ppts, "xbar_config", TEXT_CONFIG_XBAR,
			CrossbarConfigClicked);

	/* ------------------------------------- */

	p = obs_properties_add_list(ppts, RES_TYPE, TEXT_RES_FPS_TYPE,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, ResTypeChanged);

	obs_property_list_add_int(p, TEXT_PREFERRED_RES, ResType_Preferred);
	obs_property_list_add_int(p, TEXT_CUSTOM_RES, ResType_Custom);

	p = obs_properties_add_list(ppts, RESOLUTION, TEXT_RESOLUTION,
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceResolutionChanged);

	p = obs_properties_add_list(ppts, FRAME_INTERVAL, "FPS",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, DeviceIntervalChanged);

	p = obs_properties_add_list(ppts, VIDEO_FORMAT, TEXT_VIDEO_FORMAT,
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, VideoFormatChanged);

	return ppts;
}

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-dshow", "en-US")

void DShowModuleLogCallback(LogType type, const wchar_t *msg, void *param)
{
	int obs_type = LOG_DEBUG;

	switch (type) {
	case LogType::Error:   obs_type = LOG_ERROR;   break;
	case LogType::Warning: obs_type = LOG_WARNING; break;
	case LogType::Info:    obs_type = LOG_INFO;    break;
	case LogType::Debug:   obs_type = LOG_DEBUG;   break;
	}

	DStr dmsg;

	dstr_from_wcs(dmsg, msg);
	blog(obs_type, "DShow: %s", dmsg->array);

	UNUSED_PARAMETER(param);
}

bool obs_module_load(void)
{
	SetLogCallback(DShowModuleLogCallback, nullptr);

	obs_source_info info = {};
	info.id              = "dshow_input";
	info.type            = OBS_SOURCE_TYPE_INPUT;
	info.output_flags    = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC;
	info.get_name        = GetDShowInputName;
	info.create          = CreateDShowInput;
	info.destroy         = DestroyDShowInput;
	info.update          = UpdateDShowInput;
	info.get_width       = GetDShowWidth;
	info.get_height      = GetDShowHeight;
	info.get_defaults    = GetDShowDefaults;
	info.get_properties  = GetDShowProperties;
	obs_register_source(&info);

	return true;
}
