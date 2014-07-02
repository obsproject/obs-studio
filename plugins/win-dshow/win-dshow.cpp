#include <objbase.h>

#include <obs-module.h>
#include <obs.hpp>
#include <util/dstr.hpp>
#include <util/util.hpp>
#include <util/platform.h>
#include "libdshowcapture/dshowcapture.hpp"

#include <limits>
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
#define LAST_RES_TYPE     "last_res_type"
#define LAST_INTERVAL     "last_interval"

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

	source_frame frame;

	inline DShowInput(obs_source_t source_)
		: source         (source_),
		  device         (InitGraph::False),
		  comInitialized (false)
	{}

	static void OnVideoData(DShowInput *input, unsigned char *data,
			size_t size, long long startTime, long long endTime);

	void Update(obs_data_t settings);
};

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
	return between(cap.minInterval - DEVICE_INTERVAL_DIFF_LIMIT,
				interval,
				cap.maxInterval + DEVICE_INTERVAL_DIFF_LIMIT);
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

static bool DecodeDeviceId(DeviceId &out, const char *device_id)
{
	const char    *path_str;
	DStr          name, path;

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

	BPtr<wchar_t> wname = dstr_to_wcs(name);
	out.name = wname;

	if (!dstr_isempty(path)) {
		BPtr<wchar_t> wpath = dstr_to_wcs(path);
		out.path = wpath;
	}

	return true;
}

static inline bool ConvertRes(int &cx, int &cy, const char *res)
{
	return sscanf(res, "%dx%d", &cx, &cy) == 2;
}

static bool FormatMatches(VideoFormat left, VideoFormat right)
{
	return left == VideoFormat::Any || right == VideoFormat::Any ||
		left == right;
}

void DShowInput::Update(obs_data_t settings)
{
	const char *video_device_id, *res;
	DeviceId id;

	video_device_id    = obs_data_getstring  (settings, VIDEO_DEVICE_ID);
	res                = obs_data_getstring  (settings, RESOLUTION);
	long long interval = obs_data_getint     (settings, FRAME_INTERVAL);
	int resType        = (int)obs_data_getint(settings, RES_TYPE);

	VideoFormat format = (VideoFormat)obs_data_getint(settings,
			VIDEO_FORMAT);

	if (!comInitialized) {
		CoInitialize(nullptr);
		comInitialized = true;
	}

	if (!device.ResetGraph())
		return;

	if (!DecodeDeviceId(id, video_device_id))
		return;

	int cx, cy;
	if (!ConvertRes(cx, cy, res)) {
		blog(LOG_WARNING, "DShowInput::Update: Bad resolution '%s'",
				res);
		return;
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
	/* TODO: locale */
	return "Video Capture Device";
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
	obs_data_set_default_int(settings, RES_TYPE, ResType_Preferred);
	obs_data_set_default_int(settings, VIDEO_FORMAT, (int)VideoFormat::Any);
}

struct PropertiesData {
	vector<VideoDevice> devices;

	const bool GetDevice(VideoDevice &device, const char *encoded_id)
	{
		DeviceId deviceId;
		DecodeDeviceId(deviceId, encoded_id);

		for (const VideoDevice &curDevice : devices) {
			if (deviceId.name.compare(curDevice.name) == 0 &&
			    deviceId.path.compare(curDevice.path) == 0) {
				device = curDevice;
				return true;
			}
		}

		return false;
	}
};

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

static bool AddFPSRate(obs_property_t p, const FPSFormat &format,
		const VideoInfo &cap)
{
	long long interval = format.interval;

	if (!FrameRateAvailable(format, cap))
		return false;

	obs_property_list_add_int(p, format.text, interval);
	return true;
}

static inline bool AddFPSRates(obs_property_t p, const VideoDevice &device,
		int cx, int cy, long long &interval)
{
	long long bestInterval = numeric_limits<long long>::max();
	bool intervalFound = false;

	for (const FPSFormat &format : validFPSFormats) {
		for (const VideoInfo &cap : device.caps) {
			if (ResolutionAvailable(cap, cx, cy)) {
				if (!intervalFound) {
					if (FrameRateAvailable(cap, interval))
						intervalFound = true;
					else if (cap.minInterval < bestInterval)
						bestInterval = cap.minInterval;
				}

				if (AddFPSRate(p, format, cap))
					break;
			}
		}
	}

	if (!intervalFound) {
		interval = bestInterval;
		return false;
	}

	return true;
}

static bool DeviceIntervalChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings);

static bool DeviceResolutionChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *res, *last_res, *id;
	long long interval;
	VideoDevice device;

	id       = obs_data_getstring(settings, VIDEO_DEVICE_ID);
	res      = obs_data_getstring(settings, RESOLUTION);
	last_res = obs_data_getstring(settings, LAST_RESOLUTION);
	interval = obs_data_getint   (settings, FRAME_INTERVAL);

	if (!data->GetDevice(device, id))
		return true;

	int cx, cy;
	if (!ConvertRes(cx, cy, res))
		return true;

	p = obs_properties_get(props, FRAME_INTERVAL);

	obs_property_list_clear(p);
	AddFPSRates(p, device, cx, cy, interval);

	if (res && last_res && strcmp(res, last_res) != 0) {
		DeviceIntervalChanged(props, p, settings);
		obs_data_setstring(settings, LAST_RESOLUTION, res);
	}

	return true;
}

struct VideoFormatName {
	VideoFormat format;
	const char  *name;
};

static const VideoFormatName videoFormatNames[] = {
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

static bool UpdateVideoFormats(obs_properties_t props, VideoDevice &device,
		long long interval, int cx, int cy, VideoFormat format)
{
	bool foundFormat = false;
	obs_property_t p = obs_properties_get(props, VIDEO_FORMAT);

	obs_property_list_clear(p);
	obs_property_list_add_int(p, "Any", (int)VideoFormat::Any);

	for (const VideoFormatName &name : videoFormatNames) {
		for (const VideoInfo &cap : device.caps) {

			if (FrameRateAvailable(cap, interval) &&
			    ResolutionAvailable(cap, cx, cy) &&
			    FormatMatches(cap.format, name.format)) {

				if (format == cap.format)
					foundFormat = true;

				obs_property_list_add_int(p, name.name,
						(int)name.format);
				break;
			}
		}
	}

	return foundFormat;
}

static bool DeviceSelectionChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *id, *old_id;
	VideoDevice device;

	id     = obs_data_getstring(settings, VIDEO_DEVICE_ID);
	old_id = obs_data_getstring(settings, LAST_VIDEO_DEV_ID);

	if (!data->GetDevice(device, id))
		return false;

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

	/* only reset resolution if device legitimately changed */
	if (old_id && id && strcmp(id, old_id) != 0) {
		DeviceResolutionChanged(props, p, settings);
		obs_data_setstring(settings, LAST_VIDEO_DEV_ID, id);
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
	int  val     = (int)obs_data_getint(settings, RES_TYPE);
	int  lastVal = (int)obs_data_getint(settings, LAST_RES_TYPE);
	bool enabled = (val != ResType_Preferred);

	p = obs_properties_get(props, RESOLUTION);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, FRAME_INTERVAL);
	obs_property_set_enabled(p, enabled);

	p = obs_properties_get(props, VIDEO_FORMAT);
	obs_property_set_enabled(p, enabled);

	if (val == ResType_Custom && lastVal != val) {
		SetClosestResFPS(props, settings);
		obs_data_setint(settings, LAST_RES_TYPE, val);
	}

	return true;
}

static bool DeviceIntervalChanged(obs_properties_t props, obs_property_t p,
		obs_data_t settings)
{
	int  val     = (int)obs_data_getint(settings, FRAME_INTERVAL);
	int  lastVal = (int)obs_data_getint(settings, LAST_INTERVAL);

	PropertiesData *data = (PropertiesData*)obs_properties_get_param(props);
	const char *id = obs_data_getstring(settings, VIDEO_DEVICE_ID);
	VideoDevice device;

	if (!data->GetDevice(device, id))
		return false;

	int cx, cy;
	const char *res = obs_data_getstring(settings, RESOLUTION);
	if (!ConvertRes(cx, cy, res))
		return true;

	VideoFormat curFormat =
		(VideoFormat)obs_data_getint(settings, VIDEO_FORMAT);

	bool foundFormat =
		UpdateVideoFormats(props, device, val, cx, cy, curFormat);

	if (val != lastVal) {
		if (!foundFormat)
			obs_data_setint(settings, VIDEO_FORMAT,
					(int)VideoFormat::Any);

		obs_data_setint(settings, LAST_INTERVAL, val);
	}

	UNUSED_PARAMETER(p);
	return true;
}

static obs_properties_t GetDShowProperties(void)
{
	obs_properties_t ppts = obs_properties_create();
	PropertiesData *data = new PropertiesData;

	obs_properties_set_param(ppts, data, PropertiesDataDestroy);

	/* TODO: locale */
	obs_property_t p = obs_properties_add_list(ppts,
		VIDEO_DEVICE_ID, "Device",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceSelectionChanged);

	Device::EnumVideoDevices(data->devices);
	for (const VideoDevice &device : data->devices)
		AddDevice(p, device);

	obs_properties_add_button(ppts, "video_config", "Configure Video",
			VideoConfigClicked);
	obs_properties_add_button(ppts, "xbar_config", "Configure Crossbar",
			CrossbarConfigClicked);

	/* ------------------------------------- */

	p = obs_properties_add_list(ppts, RES_TYPE, "Resolution/FPS Type",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, ResTypeChanged);

	obs_property_list_add_int(p, "Device Preferred", ResType_Preferred);
	obs_property_list_add_int(p, "Custom", ResType_Custom);

	p = obs_properties_add_list(ppts, RESOLUTION, "Resolution",
			OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);

	obs_property_set_modified_callback(p, DeviceResolutionChanged);

	p = obs_properties_add_list(ppts, FRAME_INTERVAL, "FPS",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_set_modified_callback(p, DeviceIntervalChanged);

	obs_properties_add_list(ppts, VIDEO_FORMAT, "Video Format",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	return ppts;
}

OBS_DECLARE_MODULE()

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

bool obs_module_load(uint32_t libobs_ver)
{
	UNUSED_PARAMETER(libobs_ver);

	SetLogCallback(DShowModuleLogCallback, nullptr);

	obs_source_info info = {};
	info.id              = "dshow_input";
	info.type            = OBS_SOURCE_TYPE_INPUT;
	info.output_flags    = OBS_SOURCE_VIDEO |
	                       OBS_SOURCE_ASYNC;
	info.getname         = GetDShowInputName;
	info.create          = CreateDShowInput;
	info.destroy         = DestroyDShowInput;
	info.getwidth        = GetDShowWidth;
	info.getheight       = GetDShowHeight;
	info.update          = UpdateDShowInput;
	info.defaults        = GetDShowDefaults;
	info.properties      = GetDShowProperties;
	obs_register_source(&info);

	return true;
}
