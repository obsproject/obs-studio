#include <util/threading.h>
#include <opts-parser.h>
#include <obs-module.h>
#include <obs-avc.h>

#include <unordered_map>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vector>
#include <mutex>
#include <deque>
#include <map>

#include <AMF/components/VideoEncoderHEVC.h>
#include <AMF/components/VideoEncoderVCE.h>
#include <AMF/components/VideoEncoderAV1.h>
#include <AMF/core/Factory.h>
#include <AMF/core/Trace.h>

#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <util/windows/device-enum.h>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>
#include <util/platform.h>
#include <util/util.hpp>
#include <util/pipe.h>
#include <util/dstr.h>

using namespace amf;

/* ========================================================================= */
/* Junk                                                                      */

#define do_log(level, format, ...) \
	blog(level, "[%s: '%s'] " format, enc->encoder_str, obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

struct amf_error {
	const char *str;
	AMF_RESULT res;

	inline amf_error(const char *str, AMF_RESULT res) : str(str), res(res) {}
};

struct handle_tex {
	uint32_t handle;
	ComPtr<ID3D11Texture2D> tex;
	ComPtr<IDXGIKeyedMutex> km;
};

struct adapter_caps {
	bool is_amd = false;
	bool supports_avc = false;
	bool supports_hevc = false;
	bool supports_av1 = false;
};

/* ------------------------------------------------------------------------- */

static std::map<uint32_t, adapter_caps> caps;
static bool h264_supported = false;
static AMFFactory *amf_factory = nullptr;
static AMFTrace *amf_trace = nullptr;
static HMODULE amf_module = nullptr;
static uint64_t amf_version = 0;

/* ========================================================================= */
/* Main Implementation                                                       */

enum class amf_codec_type {
	AVC,
	HEVC,
	AV1,
};

struct amf_base {
	obs_encoder_t *encoder;
	const char *encoder_str;
	amf_codec_type codec;
	bool fallback;

	AMFContextPtr amf_context;
	AMFComponentPtr amf_encoder;
	AMFBufferPtr packet_data;
	AMFRate amf_frame_rate;
	AMFBufferPtr header;
	AMFSurfacePtr roi_map;

	std::deque<AMFDataPtr> queued_packets;

	AMF_VIDEO_CONVERTER_COLOR_PROFILE_ENUM amf_color_profile;
	AMF_COLOR_TRANSFER_CHARACTERISTIC_ENUM amf_characteristic;
	AMF_COLOR_PRIMARIES_ENUM amf_primaries;
	AMF_SURFACE_FORMAT amf_format;

	amf_int64 max_throughput = 0;
	amf_int64 requested_throughput = 0;
	amf_int64 throughput = 0;
	int64_t dts_offset = 0;
	uint32_t cx;
	uint32_t cy;
	uint32_t linesize = 0;
	uint32_t roi_increment = 0;
	int fps_num;
	int fps_den;
	bool full_range;
	bool bframes_supported = false;
	bool first_update = true;
	bool roi_supported = false;

	inline amf_base(bool fallback) : fallback(fallback) {}
	virtual ~amf_base() = default;
	virtual void init() = 0;
};

using d3dtex_t = ComPtr<ID3D11Texture2D>;
using buf_t = std::vector<uint8_t>;

struct amf_texencode : amf_base, public AMFSurfaceObserver {
	volatile bool destroying = false;

	std::vector<handle_tex> input_textures;

	std::mutex textures_mutex;
	std::vector<d3dtex_t> available_textures;
	std::unordered_map<AMFSurface *, d3dtex_t> active_textures;

	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	inline amf_texencode() : amf_base(false) {}
	~amf_texencode() { os_atomic_set_bool(&destroying, true); }

	void AMF_STD_CALL OnSurfaceDataRelease(amf::AMFSurface *surf) override
	{
		if (os_atomic_load_bool(&destroying))
			return;

		std::scoped_lock lock(textures_mutex);

		auto it = active_textures.find(surf);
		if (it != active_textures.end()) {
			available_textures.push_back(it->second);
			active_textures.erase(it);
		}
	}

	void init() override
	{
		AMF_RESULT res = amf_context->InitDX11(device, AMF_DX11_1);
		if (res != AMF_OK)
			throw amf_error("InitDX11 failed", res);
	}
};

struct amf_fallback : amf_base, public AMFSurfaceObserver {
	volatile bool destroying = false;

	std::mutex buffers_mutex;
	std::vector<buf_t> available_buffers;
	std::unordered_map<AMFSurface *, buf_t> active_buffers;

	inline amf_fallback() : amf_base(true) {}
	~amf_fallback() { os_atomic_set_bool(&destroying, true); }

	void AMF_STD_CALL OnSurfaceDataRelease(amf::AMFSurface *surf) override
	{
		if (os_atomic_load_bool(&destroying))
			return;

		std::scoped_lock lock(buffers_mutex);

		auto it = active_buffers.find(surf);
		if (it != active_buffers.end()) {
			available_buffers.push_back(std::move(it->second));
			active_buffers.erase(it);
		}
	}

	void init() override
	{
		AMF_RESULT res = amf_context->InitDX11(nullptr, AMF_DX11_1);
		if (res != AMF_OK)
			throw amf_error("InitDX11 failed", res);
	}
};

/* ------------------------------------------------------------------------- */
/* More garbage                                                              */

template<typename T> static bool get_amf_property(amf_base *enc, const wchar_t *name, T *value)
{
	AMF_RESULT res = enc->amf_encoder->GetProperty(name, value);
	return res == AMF_OK;
}

template<typename T> static void set_amf_property(amf_base *enc, const wchar_t *name, const T &value)
{
	AMF_RESULT res = enc->amf_encoder->SetProperty(name, value);
	if (res != AMF_OK)
		error("Failed to set property '%ls': %ls", name, amf_trace->GetResultText(res));
}

#define set_avc_property(enc, name, value) set_amf_property(enc, AMF_VIDEO_ENCODER_##name, value)
#define set_hevc_property(enc, name, value) set_amf_property(enc, AMF_VIDEO_ENCODER_HEVC_##name, value)
#define set_av1_property(enc, name, value) set_amf_property(enc, AMF_VIDEO_ENCODER_AV1_##name, value)

#define get_avc_property(enc, name, value) get_amf_property(enc, AMF_VIDEO_ENCODER_##name, value)
#define get_hevc_property(enc, name, value) get_amf_property(enc, AMF_VIDEO_ENCODER_HEVC_##name, value)
#define get_av1_property(enc, name, value) get_amf_property(enc, AMF_VIDEO_ENCODER_AV1_##name, value)

#define get_opt_name(name)                                                      \
	((enc->codec == amf_codec_type::AVC)    ? AMF_VIDEO_ENCODER_##name      \
	 : (enc->codec == amf_codec_type::HEVC) ? AMF_VIDEO_ENCODER_HEVC_##name \
						: AMF_VIDEO_ENCODER_AV1_##name)
#define set_opt(name, value) set_amf_property(enc, get_opt_name(name), value)
#define get_opt(name, value) get_amf_property(enc, get_opt_name(name), value)
#define set_avc_opt(name, value) set_avc_property(enc, name, value)
#define set_hevc_opt(name, value) set_hevc_property(enc, name, value)
#define set_av1_opt(name, value) set_av1_property(enc, name, value)
#define set_enum_opt(name, value) set_amf_property(enc, get_opt_name(name), get_opt_name(name##_##value))
#define set_avc_enum(name, value) set_avc_property(enc, name, AMF_VIDEO_ENCODER_##name##_##value)
#define set_hevc_enum(name, value) set_hevc_property(enc, name, AMF_VIDEO_ENCODER_HEVC_##name##_##value)
#define set_av1_enum(name, value) set_av1_property(enc, name, AMF_VIDEO_ENCODER_AV1_##name##_##value)

/* ------------------------------------------------------------------------- */
/* Implementation                                                            */

static HMODULE get_lib(const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
		return mod;

	return LoadLibraryA(lib);
}

#define AMD_VENDOR_ID 0x1002

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);

static bool amf_init_d3d11(amf_texencode *enc)
try {
	HMODULE dxgi = get_lib("DXGI.dll");
	HMODULE d3d11 = get_lib("D3D11.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D11_CREATE_DEVICE create_device;
	ComPtr<IDXGIFactory> factory;
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	ComPtr<IDXGIAdapter> adapter;
	DXGI_ADAPTER_DESC desc;
	HRESULT hr;

	if (!dxgi || !d3d11)
		throw "Couldn't get D3D11/DXGI libraries? "
		      "That definitely shouldn't be possible.";

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress(dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11, "D3D11CreateDevice");

	if (!create_dxgi || !create_device)
		throw "Failed to load D3D11/DXGI procedures";

	hr = create_dxgi(__uuidof(IDXGIFactory2), (void **)&factory);
	if (FAILED(hr))
		throw HRError("CreateDXGIFactory1 failed", hr);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	hr = factory->EnumAdapters(ovi.adapter, &adapter);
	if (FAILED(hr))
		throw HRError("EnumAdapters failed", hr);

	adapter->GetDesc(&desc);
	if (desc.VendorId != AMD_VENDOR_ID)
		throw "Seems somehow AMF is trying to initialize "
		      "on a non-AMD adapter";

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device,
			   nullptr, &context);
	if (FAILED(hr))
		throw HRError("D3D11CreateDevice failed", hr);

	enc->device = device;
	enc->context = context;
	return true;

} catch (const HRError &err) {
	error("%s: %s: 0x%lX", __FUNCTION__, err.str, err.hr);
	return false;

} catch (const char *err) {
	error("%s: %s", __FUNCTION__, err);
	return false;
}

static void add_output_tex(amf_texencode *enc, ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from)
{
	ID3D11Device *device = enc->device;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc;
	from->GetDesc(&desc);
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = 0;

	hr = device->CreateTexture2D(&desc, nullptr, &output_tex);
	if (FAILED(hr))
		throw HRError("Failed to create texture", hr);
}

static inline bool get_available_tex(amf_texencode *enc, ComPtr<ID3D11Texture2D> &output_tex)
{
	std::scoped_lock lock(enc->textures_mutex);
	if (enc->available_textures.size()) {
		output_tex = enc->available_textures.back();
		enc->available_textures.pop_back();
		return true;
	}

	return false;
}

static inline void get_output_tex(amf_texencode *enc, ComPtr<ID3D11Texture2D> &output_tex, ID3D11Texture2D *from)
{
	if (!get_available_tex(enc, output_tex))
		add_output_tex(enc, output_tex, from);
}

static void get_tex_from_handle(amf_texencode *enc, uint32_t handle, IDXGIKeyedMutex **km_out,
				ID3D11Texture2D **tex_out)
{
	ID3D11Device *device = enc->device;
	ComPtr<ID3D11Texture2D> tex;
	HRESULT hr;

	for (size_t i = 0; i < enc->input_textures.size(); i++) {
		struct handle_tex &ht = enc->input_textures[i];
		if (ht.handle == handle) {
			ht.km.CopyTo(km_out);
			ht.tex.CopyTo(tex_out);
			return;
		}
	}

	hr = device->OpenSharedResource((HANDLE)(uintptr_t)handle, __uuidof(ID3D11Resource), (void **)&tex);
	if (FAILED(hr))
		throw HRError("OpenSharedResource failed", hr);

	ComQIPtr<IDXGIKeyedMutex> km(tex);
	if (!km)
		throw "QueryInterface(IDXGIKeyedMutex) failed";

	tex->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);

	struct handle_tex new_ht = {handle, tex, km};
	enc->input_textures.push_back(std::move(new_ht));

	*km_out = km.Detach();
	*tex_out = tex.Detach();
}

static constexpr amf_int64 macroblock_size = 16;

static inline void calc_throughput(amf_base *enc)
{
	amf_int64 mb_cx = ((amf_int64)enc->cx + (macroblock_size - 1)) / macroblock_size;
	amf_int64 mb_cy = ((amf_int64)enc->cy + (macroblock_size - 1)) / macroblock_size;
	amf_int64 mb_frame = mb_cx * mb_cy;

	enc->throughput = mb_frame * (amf_int64)enc->fps_num / (amf_int64)enc->fps_den;
}

static inline int get_avc_preset(amf_base *enc, const char *preset);
#if ENABLE_HEVC
static inline int get_hevc_preset(amf_base *enc, const char *preset);
#endif
static inline int get_av1_preset(amf_base *enc, const char *preset);

static inline int get_preset(amf_base *enc, const char *preset)
{
	if (enc->codec == amf_codec_type::AVC)
		return get_avc_preset(enc, preset);

#if ENABLE_HEVC
	else if (enc->codec == amf_codec_type::HEVC)
		return get_hevc_preset(enc, preset);

#endif
	else if (enc->codec == amf_codec_type::AV1)
		return get_av1_preset(enc, preset);

	return 0;
}

static inline void refresh_throughput_caps(amf_base *enc, const char *&preset)
{
	AMF_RESULT res = AMF_OK;
	AMFCapsPtr caps;

	set_opt(QUALITY_PRESET, get_preset(enc, preset));
	res = enc->amf_encoder->GetCaps(&caps);
	if (res == AMF_OK) {
		caps->GetProperty(get_opt_name(CAP_MAX_THROUGHPUT), &enc->max_throughput);
		caps->GetProperty(get_opt_name(CAP_REQUESTED_THROUGHPUT), &enc->requested_throughput);
	}
}

static inline void check_preset_compatibility(amf_base *enc, const char *&preset)
{
	/* The throughput depends on the current preset and the other static
	 * encoder properties. If the throughput is lower than the max
	 * throughput, switch to a lower preset. */

	if (astrcmpi(preset, "highQuality") == 0) {
		if (!enc->max_throughput) {
			preset = "quality";
			set_opt(QUALITY_PRESET, get_preset(enc, preset));
		} else {
			if (enc->max_throughput - enc->requested_throughput < enc->throughput) {
				preset = "quality";
				refresh_throughput_caps(enc, preset);
			}
		}
	}

	if (astrcmpi(preset, "quality") == 0) {
		if (!enc->max_throughput) {
			preset = "balanced";
			set_opt(QUALITY_PRESET, get_preset(enc, preset));
		} else {
			if (enc->max_throughput - enc->requested_throughput < enc->throughput) {
				preset = "balanced";
				refresh_throughput_caps(enc, preset);
			}
		}
	}

	if (astrcmpi(preset, "balanced") == 0) {
		if (enc->max_throughput && enc->max_throughput - enc->requested_throughput < enc->throughput) {
			preset = "speed";
			refresh_throughput_caps(enc, preset);
		}
	}
}

static inline int64_t convert_to_amf_ts(amf_base *enc, int64_t ts)
{
	constexpr int64_t amf_timebase = AMF_SECOND;
	return ts * amf_timebase / (int64_t)enc->fps_den;
}

static inline int64_t convert_to_obs_ts(amf_base *enc, int64_t ts)
{
	constexpr int64_t amf_timebase = AMF_SECOND;
	return ts * (int64_t)enc->fps_den / amf_timebase;
}

static void convert_to_encoder_packet(amf_base *enc, AMFDataPtr &data, encoder_packet *packet)
{
	if (!data)
		return;

	enc->packet_data = AMFBufferPtr(data);
	data->GetProperty(L"PTS", &packet->pts);

	const wchar_t *get_output_type;
	switch (enc->codec) {
	case amf_codec_type::AVC:
		get_output_type = AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE;
		break;
	case amf_codec_type::HEVC:
		get_output_type = AMF_VIDEO_ENCODER_HEVC_OUTPUT_DATA_TYPE;
		break;
	case amf_codec_type::AV1:
		get_output_type = AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE;
		break;
	}

	uint64_t type = 0;
	AMF_RESULT res = data->GetProperty(get_output_type, &type);
	if (res != AMF_OK)
		throw amf_error("Failed to GetProperty(): encoder output "
				"data type",
				res);

	if (enc->codec == amf_codec_type::AVC || enc->codec == amf_codec_type::HEVC) {
		switch (type) {
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR:
			packet->priority = OBS_NAL_PRIORITY_HIGHEST;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_I:
			packet->priority = OBS_NAL_PRIORITY_HIGH;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_P:
			packet->priority = OBS_NAL_PRIORITY_LOW;
			break;
		case AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_B:
			packet->priority = OBS_NAL_PRIORITY_DISPOSABLE;
			break;
		}
	} else if (enc->codec == amf_codec_type::AV1) {
		switch (type) {
		case AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_KEY:
			packet->priority = OBS_NAL_PRIORITY_HIGHEST;
			break;
		case AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_INTRA_ONLY:
			packet->priority = OBS_NAL_PRIORITY_HIGH;
			break;
		case AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_INTER:
			packet->priority = OBS_NAL_PRIORITY_LOW;
			break;
		case AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_SWITCH:
			packet->priority = OBS_NAL_PRIORITY_DISPOSABLE;
			break;
		case AMF_VIDEO_ENCODER_AV1_OUTPUT_FRAME_TYPE_SHOW_EXISTING:
			packet->priority = OBS_NAL_PRIORITY_DISPOSABLE;
			break;
		}
	}

	packet->data = (uint8_t *)enc->packet_data->GetNative();
	packet->size = enc->packet_data->GetSize();
	packet->type = OBS_ENCODER_VIDEO;
	packet->dts = convert_to_obs_ts(enc, data->GetPts());
	packet->keyframe = type == AMF_VIDEO_ENCODER_OUTPUT_DATA_TYPE_IDR;

	if (enc->dts_offset && enc->codec != amf_codec_type::AV1)
		packet->dts -= enc->dts_offset;
}

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000ULL
#endif

struct roi_params {
	uint32_t mb_width;
	uint32_t mb_height;
	amf_int32 pitch;
	bool h264;
	amf_uint32 *buf;
};

static void roi_cb(void *param, obs_encoder_roi *roi)
{
	const roi_params *rp = static_cast<roi_params *>(param);

	/* AMF does not support negative priority */
	if (roi->priority < 0)
		return;

	const uint32_t mb_size = rp->h264 ? 16 : 64;
	const uint32_t roi_left = roi->left / mb_size;
	const uint32_t roi_top = roi->top / mb_size;
	const uint32_t roi_right = (roi->right - 1) / mb_size;
	const uint32_t roi_bottom = (roi->bottom - 1) / mb_size;
	/* Importance value range is 0..10 */
	const amf_uint32 priority = (amf_uint32)(10.0f * roi->priority);

	for (uint32_t mb_y = 0; mb_y < rp->mb_height; mb_y++) {
		if (mb_y < roi_top || mb_y > roi_bottom)
			continue;

		for (uint32_t mb_x = 0; mb_x < rp->mb_width; mb_x++) {
			if (mb_x < roi_left || mb_x > roi_right)
				continue;

			rp->buf[mb_y * rp->pitch / sizeof(amf_uint32) + mb_x] = priority;
		}
	}
}

static void create_roi(amf_base *enc, AMFSurface *amf_surf)
{
	uint32_t mb_size = 16; /* H.264 is always 16x16 */
	if (enc->codec == amf_codec_type::HEVC || enc->codec == amf_codec_type::AV1)
		mb_size = 64; /* AMF HEVC & AV1 use 64x64 blocks */

	const uint32_t mb_width = (enc->cx + mb_size - 1) / mb_size;
	const uint32_t mb_height = (enc->cy + mb_size - 1) / mb_size;

	if (!enc->roi_map) {
		AMFContext1Ptr context1(enc->amf_context);
		AMF_RESULT res = context1->AllocSurfaceEx(AMF_MEMORY_HOST, AMF_SURFACE_GRAY32, mb_width, mb_height,
							  AMF_SURFACE_USAGE_DEFAULT | AMF_SURFACE_USAGE_LINEAR,
							  AMF_MEMORY_CPU_DEFAULT, &enc->roi_map);

		if (res != AMF_OK) {
			warn("Failed allocating surface for ROI map!");
			/* Clear ROI to prevent failure the next frame */
			obs_encoder_clear_roi(enc->encoder);
			return;
		}
	}

	/* This is just following the SimpleROI example. */
	amf_uint32 *pBuf = (amf_uint32 *)enc->roi_map->GetPlaneAt(0)->GetNative();
	amf_int32 pitch = enc->roi_map->GetPlaneAt(0)->GetHPitch();
	memset(pBuf, 0, pitch * mb_height);

	roi_params par{mb_width, mb_height, pitch, enc->codec == amf_codec_type::AVC, pBuf};
	obs_encoder_enum_roi(enc->encoder, roi_cb, &par);

	enc->roi_increment = obs_encoder_get_roi_increment(enc->encoder);
}

static void add_roi(amf_base *enc, AMFSurface *amf_surf)
{
	const uint32_t increment = obs_encoder_get_roi_increment(enc->encoder);

	if (increment != enc->roi_increment || !enc->roi_increment)
		create_roi(enc, amf_surf);

	if (enc->codec == amf_codec_type::AVC)
		amf_surf->SetProperty(AMF_VIDEO_ENCODER_ROI_DATA, enc->roi_map);
	else if (enc->codec == amf_codec_type::HEVC)
		amf_surf->SetProperty(AMF_VIDEO_ENCODER_HEVC_ROI_DATA, enc->roi_map);
	else if (enc->codec == amf_codec_type::AV1)
		amf_surf->SetProperty(AMF_VIDEO_ENCODER_AV1_ROI_DATA, enc->roi_map);
}

static void amf_encode_base(amf_base *enc, AMFSurface *amf_surf, encoder_packet *packet, bool *received_packet)
{
	auto &queued_packets = enc->queued_packets;
	uint64_t ts_start = os_gettime_ns();
	AMF_RESULT res;

	*received_packet = false;

	bool waiting = true;
	while (waiting) {
		/* ----------------------------------- */
		/* add ROI data (if any)               */
		if (enc->roi_supported && obs_encoder_has_roi(enc->encoder))
			add_roi(enc, amf_surf);

		/* ----------------------------------- */
		/* submit frame                        */

		res = enc->amf_encoder->SubmitInput(amf_surf);

		if (res == AMF_OK || res == AMF_NEED_MORE_INPUT) {
			waiting = false;

		} else if (res == AMF_INPUT_FULL) {
			os_sleep_ms(1);

			uint64_t duration = os_gettime_ns() - ts_start;
			constexpr uint64_t timeout = 5 * SEC_TO_NSEC;

			if (duration >= timeout) {
				throw amf_error("SubmitInput timed out", res);
			}
		} else {
			throw amf_error("SubmitInput failed", res);
		}

		/* ----------------------------------- */
		/* query as many packets as possible   */

		AMFDataPtr new_packet;
		do {
			res = enc->amf_encoder->QueryOutput(&new_packet);
			if (new_packet)
				queued_packets.push_back(new_packet);

			if (res != AMF_REPEAT && res != AMF_OK) {
				throw amf_error("QueryOutput failed", res);
			}
		} while (!!new_packet);
	}

	/* ----------------------------------- */
	/* return a packet if available        */

	if (queued_packets.size()) {
		AMFDataPtr amf_out;

		amf_out = queued_packets.front();
		queued_packets.pop_front();

		*received_packet = true;
		convert_to_encoder_packet(enc, amf_out, packet);
	}
}

static bool amf_encode_tex(void *data, uint32_t handle, int64_t pts, uint64_t lock_key, uint64_t *next_key,
			   encoder_packet *packet, bool *received_packet)
try {
	amf_texencode *enc = (amf_texencode *)data;
	ID3D11DeviceContext *context = enc->context;
	ComPtr<ID3D11Texture2D> output_tex;
	ComPtr<ID3D11Texture2D> input_tex;
	ComPtr<IDXGIKeyedMutex> km;
	AMFSurfacePtr amf_surf;
	AMF_RESULT res;

	if (handle == GS_INVALID_HANDLE) {
		*next_key = lock_key;
		throw "Encode failed: bad texture handle";
	}

	/* ------------------------------------ */
	/* get the input tex                    */

	get_tex_from_handle(enc, handle, &km, &input_tex);

	/* ------------------------------------ */
	/* get an output tex                    */

	get_output_tex(enc, output_tex, input_tex);

	/* ------------------------------------ */
	/* copy to output tex                   */

	km->AcquireSync(lock_key, INFINITE);
	context->CopyResource((ID3D11Resource *)output_tex.Get(), (ID3D11Resource *)input_tex.Get());
	context->Flush();
	km->ReleaseSync(*next_key);

	/* ------------------------------------ */
	/* map output tex to amf surface        */

	res = enc->amf_context->CreateSurfaceFromDX11Native(output_tex, &amf_surf, enc);
	if (res != AMF_OK)
		throw amf_error("CreateSurfaceFromDX11Native failed", res);

	int64_t last_ts = convert_to_amf_ts(enc, pts - 1);
	int64_t cur_ts = convert_to_amf_ts(enc, pts);

	amf_surf->SetPts(cur_ts);
	amf_surf->SetProperty(L"PTS", pts);

	{
		std::scoped_lock lock(enc->textures_mutex);
		enc->active_textures[amf_surf.GetPtr()] = output_tex;
	}

	/* ------------------------------------ */
	/* do actual encode                     */

	amf_encode_base(enc, amf_surf, packet, received_packet);
	return true;

} catch (const char *err) {
	amf_texencode *enc = (amf_texencode *)data;
	error("%s: %s", __FUNCTION__, err);
	return false;

} catch (const amf_error &err) {
	amf_texencode *enc = (amf_texencode *)data;
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	*received_packet = false;
	return false;

} catch (const HRError &err) {
	amf_texencode *enc = (amf_texencode *)data;
	error("%s: %s: 0x%lX", __FUNCTION__, err.str, err.hr);
	*received_packet = false;
	return false;
}

static buf_t alloc_buf(amf_fallback *enc)
{
	buf_t buf;
	size_t size;

	if (enc->amf_format == AMF_SURFACE_NV12) {
		size = enc->linesize * enc->cy * 2;
	} else if (enc->amf_format == AMF_SURFACE_RGBA) {
		size = enc->linesize * enc->cy * 4;
	} else if (enc->amf_format == AMF_SURFACE_P010) {
		size = enc->linesize * enc->cy * 2 * 2;
	} else {
		throw "Invalid amf_format";
	}

	buf.resize(size);
	return buf;
}

static buf_t get_buf(amf_fallback *enc)
{
	std::scoped_lock lock(enc->buffers_mutex);
	buf_t buf;

	if (enc->available_buffers.size()) {
		buf = std::move(enc->available_buffers.back());
		enc->available_buffers.pop_back();
	} else {
		buf = alloc_buf(enc);
	}

	return buf;
}

static inline void copy_frame_data(amf_fallback *enc, buf_t &buf, struct encoder_frame *frame)
{
	uint8_t *dst = &buf[0];

	if (enc->amf_format == AMF_SURFACE_NV12 || enc->amf_format == AMF_SURFACE_P010) {
		size_t size = enc->linesize * enc->cy;
		memcpy(&buf[0], frame->data[0], size);
		memcpy(&buf[size], frame->data[1], size / 2);

	} else if (enc->amf_format == AMF_SURFACE_RGBA) {
		memcpy(dst, frame->data[0], enc->linesize * enc->cy);
	}
}

static bool amf_encode_fallback(void *data, struct encoder_frame *frame, struct encoder_packet *packet,
				bool *received_packet)
try {
	amf_fallback *enc = (amf_fallback *)data;
	AMFSurfacePtr amf_surf;
	AMF_RESULT res;
	buf_t buf;

	if (!enc->linesize)
		enc->linesize = frame->linesize[0];

	buf = get_buf(enc);

	copy_frame_data(enc, buf, frame);

	res = enc->amf_context->CreateSurfaceFromHostNative(enc->amf_format, enc->cx, enc->cy, enc->linesize, 0,
							    &buf[0], &amf_surf, enc);
	if (res != AMF_OK)
		throw amf_error("CreateSurfaceFromHostNative failed", res);

	int64_t last_ts = convert_to_amf_ts(enc, frame->pts - 1);
	int64_t cur_ts = convert_to_amf_ts(enc, frame->pts);

	amf_surf->SetPts(cur_ts);
	amf_surf->SetProperty(L"PTS", frame->pts);

	{
		std::scoped_lock lock(enc->buffers_mutex);
		enc->active_buffers[amf_surf.GetPtr()] = std::move(buf);
	}

	/* ------------------------------------ */
	/* do actual encode                     */

	amf_encode_base(enc, amf_surf, packet, received_packet);
	return true;

} catch (const amf_error &err) {
	amf_fallback *enc = (amf_fallback *)data;
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	*received_packet = false;
	return false;
} catch (const char *err) {
	amf_fallback *enc = (amf_fallback *)data;
	error("%s: %s", __FUNCTION__, err);
	*received_packet = false;
	return false;
}

static bool amf_extra_data(void *data, uint8_t **header, size_t *size)
{
	amf_base *enc = (amf_base *)data;
	if (!enc->header)
		return false;

	*header = (uint8_t *)enc->header->GetNative();
	*size = enc->header->GetSize();
	return true;
}

static void h264_video_info_fallback(void *, struct video_scale_info *info)
{
	switch (info->format) {
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		info->format = VIDEO_FORMAT_RGBA;
		break;
	default:
		info->format = VIDEO_FORMAT_NV12;
		break;
	}
}

static void h265_video_info_fallback(void *, struct video_scale_info *info)
{
	switch (info->format) {
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		info->format = VIDEO_FORMAT_RGBA;
		break;
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		info->format = VIDEO_FORMAT_P010;
		break;
	default:
		info->format = VIDEO_FORMAT_NV12;
	}
}

static void av1_video_info_fallback(void *, struct video_scale_info *info)
{
	switch (info->format) {
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
		info->format = VIDEO_FORMAT_RGBA;
		break;
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		info->format = VIDEO_FORMAT_P010;
		break;
	default:
		info->format = VIDEO_FORMAT_NV12;
	}
}

static bool amf_create_encoder(amf_base *enc)
try {
	AMF_RESULT res;

	/* ------------------------------------ */
	/* get video info                       */

	struct video_scale_info info;
	video_t *video = obs_encoder_video(enc->encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	info.format = voi->format;
	info.colorspace = voi->colorspace;
	info.range = voi->range;

	if (enc->fallback) {
		if (enc->codec == amf_codec_type::AVC)
			h264_video_info_fallback(NULL, &info);
		else if (enc->codec == amf_codec_type::HEVC)
			h265_video_info_fallback(NULL, &info);
		else
			av1_video_info_fallback(NULL, &info);
	}

	enc->cx = obs_encoder_get_width(enc->encoder);
	enc->cy = obs_encoder_get_height(enc->encoder);
	enc->amf_frame_rate = AMFConstructRate(voi->fps_num, voi->fps_den);
	enc->fps_num = (int)voi->fps_num;
	enc->fps_den = (int)voi->fps_den;
	enc->full_range = info.range == VIDEO_RANGE_FULL;

	switch (info.colorspace) {
	case VIDEO_CS_601:
		enc->amf_color_profile = enc->full_range ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_601
							 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_601;
		enc->amf_primaries = AMF_COLOR_PRIMARIES_SMPTE170M;
		enc->amf_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->amf_color_profile = enc->full_range ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709
							 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
		enc->amf_primaries = AMF_COLOR_PRIMARIES_BT709;
		enc->amf_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->amf_color_profile = enc->full_range ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_709
							 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_709;
		enc->amf_primaries = AMF_COLOR_PRIMARIES_BT709;
		enc->amf_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_IEC61966_2_1;
		break;
	case VIDEO_CS_2100_HLG:
		enc->amf_color_profile = enc->full_range ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_2020
							 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_2020;
		enc->amf_primaries = AMF_COLOR_PRIMARIES_BT2020;
		enc->amf_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_ARIB_STD_B67;
		break;
	case VIDEO_CS_2100_PQ:
		enc->amf_color_profile = enc->full_range ? AMF_VIDEO_CONVERTER_COLOR_PROFILE_FULL_2020
							 : AMF_VIDEO_CONVERTER_COLOR_PROFILE_2020;
		enc->amf_primaries = AMF_COLOR_PRIMARIES_BT2020;
		enc->amf_characteristic = AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE2084;
		break;
	}

	switch (info.format) {
	case VIDEO_FORMAT_NV12:
		enc->amf_format = AMF_SURFACE_NV12;
		break;
	case VIDEO_FORMAT_P010:
		enc->amf_format = AMF_SURFACE_P010;
		break;
	case VIDEO_FORMAT_RGBA:
		enc->amf_format = AMF_SURFACE_RGBA;
		break;
	}

	/* ------------------------------------ */
	/* create encoder                       */

	res = amf_factory->CreateContext(&enc->amf_context);
	if (res != AMF_OK)
		throw amf_error("CreateContext failed", res);

	enc->init();

	const wchar_t *codec = nullptr;
	switch (enc->codec) {
	case (amf_codec_type::AVC):
		codec = AMFVideoEncoderVCE_AVC;
		break;
	case (amf_codec_type::HEVC):
		codec = AMFVideoEncoder_HEVC;
		break;
	case (amf_codec_type::AV1):
		codec = AMFVideoEncoder_AV1;
		break;
	default:
		codec = AMFVideoEncoder_HEVC;
	}
	res = amf_factory->CreateComponent(enc->amf_context, codec, &enc->amf_encoder);
	if (res != AMF_OK)
		throw amf_error("CreateComponent failed", res);

	calc_throughput(enc);
	return true;

} catch (const amf_error &err) {
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return false;
}

static void amf_destroy(void *data)
{
	amf_base *enc = (amf_base *)data;
	delete enc;
}

static void check_texture_encode_capability(obs_encoder_t *encoder, amf_codec_type codec)
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);
	bool avc = amf_codec_type::AVC == codec;
	bool hevc = amf_codec_type::HEVC == codec;
	bool av1 = amf_codec_type::AV1 == codec;

	if (obs_encoder_scaling_enabled(encoder) && !obs_encoder_gpu_scaling_enabled(encoder))
		throw "Encoder scaling is active";

	if (hevc || av1) {
		if (!obs_encoder_video_tex_active(encoder, VIDEO_FORMAT_NV12) &&
		    !obs_encoder_video_tex_active(encoder, VIDEO_FORMAT_P010))
			throw "NV12/P010 textures aren't active";
	} else if (!obs_encoder_video_tex_active(encoder, VIDEO_FORMAT_NV12)) {
		throw "NV12 textures aren't active";
	}

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG:
			throw "OBS does not support 8-bit output of Rec. 2100";
		}
	}

	if ((avc && !caps[ovi.adapter].supports_avc) || (hevc && !caps[ovi.adapter].supports_hevc) ||
	    (av1 && !caps[ovi.adapter].supports_av1))
		throw "Wrong adapter";
}

#include "texture-amf-opts.hpp"

static void amf_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "cqp", 20);
	obs_data_set_default_string(settings, "rate_control", "CBR");
	obs_data_set_default_string(settings, "preset", "quality");
	obs_data_set_default_string(settings, "profile", "high");
	obs_data_set_default_int(settings, "bf", 3);
}

static bool rate_control_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	const char *rc = obs_data_get_string(settings, "rate_control");
	bool cqp = astrcmpi(rc, "CQP") == 0;
	bool qvbr = astrcmpi(rc, "QVBR") == 0;

	p = obs_properties_get(ppts, "bitrate");
	obs_property_set_visible(p, !cqp && !qvbr);
	p = obs_properties_get(ppts, "cqp");
	obs_property_set_visible(p, cqp || qvbr);
	return true;
}

static obs_properties_t *amf_properties_internal(amf_codec_type codec)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_list(props, "rate_control", obs_module_text("RateControl"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, "CBR", "CBR");
	obs_property_list_add_string(p, "CQP", "CQP");
	obs_property_list_add_string(p, "VBR", "VBR");
	obs_property_list_add_string(p, "VBR_LAT", "VBR_LAT");
	obs_property_list_add_string(p, "QVBR", "QVBR");
	obs_property_list_add_string(p, "HQVBR", "HQVBR");
	obs_property_list_add_string(p, "HQCBR", "HQCBR");

	obs_property_set_modified_callback(p, rate_control_modified);

	p = obs_properties_add_int(props, "bitrate", obs_module_text("Bitrate"), 50, 100000, 50);
	obs_property_int_set_suffix(p, " Kbps");

	obs_properties_add_int(props, "cqp", obs_module_text("NVENC.CQLevel"), 0,
			       codec == amf_codec_type::AV1 ? 63 : 51, 1);

	p = obs_properties_add_int(props, "keyint_sec", obs_module_text("KeyframeIntervalSec"), 0, 10, 1);
	obs_property_int_set_suffix(p, " s");

	p = obs_properties_add_list(props, "preset", obs_module_text("Preset"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

#define add_preset(val) obs_property_list_add_string(p, obs_module_text("AMF.Preset." val), val)
	if (amf_codec_type::AV1 == codec) {
		add_preset("highQuality");
	}
	add_preset("quality");
	add_preset("balanced");
	add_preset("speed");
#undef add_preset

	if (amf_codec_type::AVC == codec || amf_codec_type::AV1 == codec) {
		p = obs_properties_add_list(props, "profile", obs_module_text("Profile"), OBS_COMBO_TYPE_LIST,
					    OBS_COMBO_FORMAT_STRING);

#define add_profile(val) obs_property_list_add_string(p, val, val)
		if (amf_codec_type::AVC == codec)
			add_profile("high");
		add_profile("main");
		if (amf_codec_type::AVC == codec)
			add_profile("baseline");
#undef add_profile
	}

	if (amf_codec_type::AVC == codec) {
		obs_properties_add_int(props, "bf", obs_module_text("BFrames"), 0, 5, 1);
	}

	p = obs_properties_add_text(props, "ffmpeg_opts", obs_module_text("AMFOpts"), OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, obs_module_text("AMFOpts.ToolTip"));

	return props;
}

static obs_properties_t *amf_avc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return amf_properties_internal(amf_codec_type::AVC);
}

static obs_properties_t *amf_hevc_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return amf_properties_internal(amf_codec_type::HEVC);
}

static obs_properties_t *amf_av1_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	return amf_properties_internal(amf_codec_type::AV1);
}

/* ========================================================================= */
/* AVC Implementation                                                        */

static const char *amf_avc_get_name(void *)
{
	return "AMD HW H.264 (AVC)";
}

static inline int get_avc_preset(amf_base *enc, const char *preset)
{
	if (astrcmpi(preset, "quality") == 0)
		return AMF_VIDEO_ENCODER_QUALITY_PRESET_QUALITY;
	else if (astrcmpi(preset, "speed") == 0)
		return AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED;

	return AMF_VIDEO_ENCODER_QUALITY_PRESET_BALANCED;
}

static inline int get_avc_rate_control(const char *rc_str)
{
	if (astrcmpi(rc_str, "cqp") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CONSTANT_QP;
	else if (astrcmpi(rc_str, "cbr") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CBR;
	else if (astrcmpi(rc_str, "vbr") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "vbr_lat") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "qvbr") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqvbr") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_HIGH_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqcbr") == 0)
		return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_HIGH_QUALITY_CBR;

	return AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CBR;
}

static inline int get_avc_profile(obs_data_t *settings)
{
	const char *profile = obs_data_get_string(settings, "profile");

	if (astrcmpi(profile, "baseline") == 0)
		return AMF_VIDEO_ENCODER_PROFILE_BASELINE;
	else if (astrcmpi(profile, "main") == 0)
		return AMF_VIDEO_ENCODER_PROFILE_MAIN;
	else if (astrcmpi(profile, "constrained_baseline") == 0)
		return AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_BASELINE;
	else if (astrcmpi(profile, "constrained_high") == 0)
		return AMF_VIDEO_ENCODER_PROFILE_CONSTRAINED_HIGH;

	return AMF_VIDEO_ENCODER_PROFILE_HIGH;
}

static void amf_avc_update_data(amf_base *enc, int rc, int64_t bitrate, int64_t qp)
{
	if (rc != AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CONSTANT_QP &&
	    rc != AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_QUALITY_VBR) {
		set_avc_property(enc, TARGET_BITRATE, bitrate);
		set_avc_property(enc, PEAK_BITRATE, bitrate);
		set_avc_property(enc, VBV_BUFFER_SIZE, bitrate);

		if (rc == AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CBR) {
			set_avc_property(enc, FILLER_DATA_ENABLE, true);
		}
	} else {
		set_avc_property(enc, QP_I, qp);
		set_avc_property(enc, QP_P, qp);
		set_avc_property(enc, QP_B, qp);
		set_avc_property(enc, QVBR_QUALITY_LEVEL, qp);
	}
}

static bool amf_avc_update(void *data, obs_data_t *settings)
try {
	amf_base *enc = (amf_base *)data;

	if (enc->first_update) {
		enc->first_update = false;
		return true;
	}

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t qp = obs_data_get_int(settings, "cqp");
	const char *rc_str = obs_data_get_string(settings, "rate_control");
	int rc = get_avc_rate_control(rc_str);
	AMF_RESULT res = AMF_OK;

	amf_avc_update_data(enc, rc, bitrate * 1000, qp);

	res = enc->amf_encoder->Flush();
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Flush failed", res);

	res = enc->amf_encoder->ReInit(enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::ReInit failed", res);

	return true;

} catch (const amf_error &err) {
	amf_base *enc = (amf_base *)data;
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return false;
}

static bool amf_avc_init(void *data, obs_data_t *settings)
{
	amf_base *enc = (amf_base *)data;

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t qp = obs_data_get_int(settings, "cqp");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *profile = obs_data_get_string(settings, "profile");
	const char *rc_str = obs_data_get_string(settings, "rate_control");
	int64_t bf = obs_data_get_int(settings, "bf");

	if (enc->bframes_supported) {
		set_avc_property(enc, MAX_CONSECUTIVE_BPICTURES, bf);
		set_avc_property(enc, B_PIC_PATTERN, bf);

		/* AdaptiveMiniGOP is suggested for some types of content such
		 * as those with high motion. This only takes effect if
		 * Pre-Analysis is enabled.
		 */
		if (bf > 0) {
			set_avc_property(enc, ADAPTIVE_MINIGOP, true);
		}

	} else if (bf != 0) {
		warn("B-Frames set to %lld but b-frames are not "
		     "supported by this device",
		     bf);
		bf = 0;
	}

	int rc = get_avc_rate_control(rc_str);

	set_avc_property(enc, RATE_CONTROL_METHOD, rc);
	if (rc != AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CONSTANT_QP)
		set_avc_property(enc, ENABLE_VBAQ, true);

	amf_avc_update_data(enc, rc, bitrate * 1000, qp);

	set_avc_property(enc, ENFORCE_HRD, true);
	set_avc_property(enc, HIGH_MOTION_QUALITY_BOOST_ENABLE, false);

	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int gop_size = (keyint_sec) ? keyint_sec * enc->fps_num / enc->fps_den : 250;

	set_avc_property(enc, IDR_PERIOD, gop_size);

	bool repeat_headers = obs_data_get_bool(settings, "repeat_headers");
	if (repeat_headers)
		set_avc_property(enc, HEADER_INSERTION_SPACING, gop_size);

	set_avc_property(enc, DE_BLOCKING_FILTER, true);

	check_preset_compatibility(enc, preset);

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	if (ffmpeg_opts && *ffmpeg_opts) {
		struct obs_options opts = obs_parse_options(ffmpeg_opts);
		for (size_t i = 0; i < opts.count; i++) {
			amf_apply_opt(enc, &opts.options[i]);
		}
		obs_free_options(opts);
	}

	if (!ffmpeg_opts || !*ffmpeg_opts)
		ffmpeg_opts = "(none)";

	info("settings:\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\tprofile:      %s\n"
	     "\tb-frames:     %d\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tparams:       %s",
	     rc_str, bitrate, qp, gop_size, preset, profile, bf, enc->cx, enc->cy, ffmpeg_opts);

	return true;
}

static void amf_avc_create_internal(amf_base *enc, obs_data_t *settings)
{
	AMF_RESULT res;
	AMFVariant p;

	enc->codec = amf_codec_type::AVC;

	if (!amf_create_encoder(enc))
		throw "Failed to create encoder";

	AMFCapsPtr caps;
	res = enc->amf_encoder->GetCaps(&caps);
	if (res == AMF_OK) {
		caps->GetProperty(AMF_VIDEO_ENCODER_CAP_BFRAMES, &enc->bframes_supported);
		caps->GetProperty(AMF_VIDEO_ENCODER_CAP_MAX_THROUGHPUT, &enc->max_throughput);
		caps->GetProperty(AMF_VIDEO_ENCODER_CAP_REQUESTED_THROUGHPUT, &enc->requested_throughput);
		caps->GetProperty(AMF_VIDEO_ENCODER_CAP_ROI, &enc->roi_supported);
	}

	const char *preset = obs_data_get_string(settings, "preset");

	set_avc_property(enc, FRAMESIZE, AMFConstructSize(enc->cx, enc->cy));
	set_avc_property(enc, USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCODING);
	set_avc_property(enc, QUALITY_PRESET, get_avc_preset(enc, preset));
	set_avc_property(enc, PROFILE, get_avc_profile(settings));
	set_avc_property(enc, LOWLATENCY_MODE, false);
	set_avc_property(enc, CABAC_ENABLE, AMF_VIDEO_ENCODER_UNDEFINED);
	set_avc_property(enc, PREENCODE_ENABLE, true);
	set_avc_property(enc, OUTPUT_COLOR_PROFILE, enc->amf_color_profile);
	set_avc_property(enc, OUTPUT_TRANSFER_CHARACTERISTIC, enc->amf_characteristic);
	set_avc_property(enc, OUTPUT_COLOR_PRIMARIES, enc->amf_primaries);
	set_avc_property(enc, FULL_RANGE_COLOR, enc->full_range);
	set_avc_property(enc, FRAMERATE, enc->amf_frame_rate);

	amf_avc_init(enc, settings);

	res = enc->amf_encoder->Init(enc->amf_format, enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Init failed", res);

	res = enc->amf_encoder->GetProperty(AMF_VIDEO_ENCODER_EXTRADATA, &p);
	if (res == AMF_OK && p.type == AMF_VARIANT_INTERFACE)
		enc->header = AMFBufferPtr(p.pInterface);

	if (enc->bframes_supported) {
		amf_int64 b_frames = 0;
		amf_int64 b_max = 0;

		if (get_avc_property(enc, B_PIC_PATTERN, &b_frames) &&
		    get_avc_property(enc, MAX_CONSECUTIVE_BPICTURES, &b_max))
			enc->dts_offset = b_frames + 1;
		else
			enc->dts_offset = 0;
	}
}

static void *amf_avc_create_texencode(obs_data_t *settings, obs_encoder_t *encoder)
try {
	check_texture_encode_capability(encoder, amf_codec_type::AVC);

	std::unique_ptr<amf_texencode> enc = std::make_unique<amf_texencode>();
	enc->encoder = encoder;
	enc->encoder_str = "texture-amf-h264";

	if (!amf_init_d3d11(enc.get()))
		throw "Failed to create D3D11";

	amf_avc_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[texture-amf-h264] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return obs_encoder_create_rerouted(encoder, "h264_fallback_amf");

} catch (const char *err) {
	blog(LOG_ERROR, "[texture-amf-h264] %s: %s", __FUNCTION__, err);
	return obs_encoder_create_rerouted(encoder, "h264_fallback_amf");
}

static void *amf_avc_create_fallback(obs_data_t *settings, obs_encoder_t *encoder)
try {
	std::unique_ptr<amf_fallback> enc = std::make_unique<amf_fallback>();
	enc->encoder = encoder;
	enc->encoder_str = "fallback-amf-h264";

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010: {
		const char *const text = obs_module_text("AMF.10bitUnsupportedAvc");
		obs_encoder_set_last_error(encoder, text);
		throw text;
	}
	case VIDEO_FORMAT_P216:
	case VIDEO_FORMAT_P416: {
		const char *const text = obs_module_text("AMF.16bitUnsupported");
		obs_encoder_set_last_error(encoder, text);
		throw text;
	}
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text = obs_module_text("AMF.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			throw text;
		}
		}
	}

	amf_avc_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[fallback-amf-h264] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return nullptr;

} catch (const char *err) {
	blog(LOG_ERROR, "[fallback-amf-h264] %s: %s", __FUNCTION__, err);
	return nullptr;
}

static void register_avc()
{
	struct obs_encoder_info amf_encoder_info = {};
	amf_encoder_info.id = "h264_texture_amf";
	amf_encoder_info.type = OBS_ENCODER_VIDEO;
	amf_encoder_info.codec = "h264";
	amf_encoder_info.get_name = amf_avc_get_name;
	amf_encoder_info.create = amf_avc_create_texencode;
	amf_encoder_info.destroy = amf_destroy;
	amf_encoder_info.update = amf_avc_update;
	amf_encoder_info.encode_texture = amf_encode_tex;
	amf_encoder_info.get_defaults = amf_defaults;
	amf_encoder_info.get_properties = amf_avc_properties;
	amf_encoder_info.get_extra_data = amf_extra_data;
	amf_encoder_info.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;

	obs_register_encoder(&amf_encoder_info);

	amf_encoder_info.id = "h264_fallback_amf";
	amf_encoder_info.caps = OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;
	amf_encoder_info.encode_texture = nullptr;
	amf_encoder_info.create = amf_avc_create_fallback;
	amf_encoder_info.encode = amf_encode_fallback;
	amf_encoder_info.get_video_info = h264_video_info_fallback;

	obs_register_encoder(&amf_encoder_info);
}

/* ========================================================================= */
/* HEVC Implementation                                                       */

#if ENABLE_HEVC

static const char *amf_hevc_get_name(void *)
{
	return "AMD HW H.265 (HEVC)";
}

static inline int get_hevc_preset(amf_base *enc, const char *preset)
{
	if (astrcmpi(preset, "balanced") == 0)
		return AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_BALANCED;
	else if (astrcmpi(preset, "speed") == 0)
		return AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_SPEED;

	return AMF_VIDEO_ENCODER_HEVC_QUALITY_PRESET_QUALITY;
}

static inline int get_hevc_rate_control(const char *rc_str)
{
	if (astrcmpi(rc_str, "cqp") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CONSTANT_QP;
	else if (astrcmpi(rc_str, "vbr_lat") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "vbr") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "cbr") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CBR;
	else if (astrcmpi(rc_str, "qvbr") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqvbr") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_HIGH_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqcbr") == 0)
		return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_HIGH_QUALITY_CBR;

	return AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CBR;
}

static void amf_hevc_update_data(amf_base *enc, int rc, int64_t bitrate, int64_t qp)
{
	if (rc != AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CONSTANT_QP &&
	    rc != AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_QUALITY_VBR) {
		set_hevc_property(enc, TARGET_BITRATE, bitrate);
		set_hevc_property(enc, PEAK_BITRATE, bitrate);
		set_hevc_property(enc, VBV_BUFFER_SIZE, bitrate);

		if (rc == AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CBR) {
			set_hevc_property(enc, FILLER_DATA_ENABLE, true);
		}
	} else {
		set_hevc_property(enc, QP_I, qp);
		set_hevc_property(enc, QP_P, qp);
		set_hevc_property(enc, QVBR_QUALITY_LEVEL, qp);
	}
}

static bool amf_hevc_update(void *data, obs_data_t *settings)
try {
	amf_base *enc = (amf_base *)data;

	if (enc->first_update) {
		enc->first_update = false;
		return true;
	}

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t qp = obs_data_get_int(settings, "cqp");
	const char *rc_str = obs_data_get_string(settings, "rate_control");
	int rc = get_hevc_rate_control(rc_str);
	AMF_RESULT res = AMF_OK;

	amf_hevc_update_data(enc, rc, bitrate * 1000, qp);

	res = enc->amf_encoder->Flush();
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Flush failed", res);

	res = enc->amf_encoder->ReInit(enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::ReInit failed", res);

	return true;

} catch (const amf_error &err) {
	amf_base *enc = (amf_base *)data;
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return false;
}

static bool amf_hevc_init(void *data, obs_data_t *settings)
{
	amf_base *enc = (amf_base *)data;

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t qp = obs_data_get_int(settings, "cqp");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *profile = obs_data_get_string(settings, "profile");
	const char *rc_str = obs_data_get_string(settings, "rate_control");
	int rc = get_hevc_rate_control(rc_str);

	set_hevc_property(enc, RATE_CONTROL_METHOD, rc);
	if (rc != AMF_VIDEO_ENCODER_HEVC_RATE_CONTROL_METHOD_CONSTANT_QP)
		set_hevc_property(enc, ENABLE_VBAQ, true);

	amf_hevc_update_data(enc, rc, bitrate * 1000, qp);

	set_hevc_property(enc, ENFORCE_HRD, true);
	set_hevc_property(enc, HIGH_MOTION_QUALITY_BOOST_ENABLE, false);

	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int gop_size = (keyint_sec) ? keyint_sec * enc->fps_num / enc->fps_den : 250;

	set_hevc_property(enc, GOP_SIZE, gop_size);

	check_preset_compatibility(enc, preset);

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	if (ffmpeg_opts && *ffmpeg_opts) {
		struct obs_options opts = obs_parse_options(ffmpeg_opts);
		for (size_t i = 0; i < opts.count; i++) {
			amf_apply_opt(enc, &opts.options[i]);
		}
		obs_free_options(opts);
	}

	if (!ffmpeg_opts || !*ffmpeg_opts)
		ffmpeg_opts = "(none)";

	info("settings:\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tparams:       %s",
	     rc_str, bitrate, qp, gop_size, preset, profile, enc->cx, enc->cy, ffmpeg_opts);

	return true;
}

static inline bool is_hlg(amf_base *enc)
{
	return enc->amf_characteristic == AMF_COLOR_TRANSFER_CHARACTERISTIC_ARIB_STD_B67;
}

static inline bool is_pq(amf_base *enc)
{
	return enc->amf_characteristic == AMF_COLOR_TRANSFER_CHARACTERISTIC_SMPTE2084;
}

constexpr amf_uint16 amf_hdr_primary(uint32_t num, uint32_t den)
{
	return (amf_uint16)(num * 50000 / den);
}

constexpr amf_uint32 lum_mul = 10000;

constexpr amf_uint32 amf_make_lum(amf_uint32 val)
{
	return val * lum_mul;
}

static void amf_hevc_create_internal(amf_base *enc, obs_data_t *settings)
{
	AMF_RESULT res;
	AMFVariant p;

	enc->codec = amf_codec_type::HEVC;

	if (!amf_create_encoder(enc))
		throw "Failed to create encoder";

	AMFCapsPtr caps;
	res = enc->amf_encoder->GetCaps(&caps);
	if (res == AMF_OK) {
		caps->GetProperty(AMF_VIDEO_ENCODER_HEVC_CAP_MAX_THROUGHPUT, &enc->max_throughput);
		caps->GetProperty(AMF_VIDEO_ENCODER_HEVC_CAP_REQUESTED_THROUGHPUT, &enc->requested_throughput);
		caps->GetProperty(AMF_VIDEO_ENCODER_HEVC_CAP_ROI, &enc->roi_supported);
	}

	const bool is10bit = enc->amf_format == AMF_SURFACE_P010;
	const bool pq = is_pq(enc);
	const bool hlg = is_hlg(enc);
	const bool is_hdr = pq || hlg;
	const char *preset = obs_data_get_string(settings, "preset");

	set_hevc_property(enc, FRAMESIZE, AMFConstructSize(enc->cx, enc->cy));
	set_hevc_property(enc, USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCODING);
	set_hevc_property(enc, QUALITY_PRESET, get_hevc_preset(enc, preset));
	set_hevc_property(enc, COLOR_BIT_DEPTH, is10bit ? AMF_COLOR_BIT_DEPTH_10 : AMF_COLOR_BIT_DEPTH_8);
	set_hevc_property(enc, PROFILE,
			  is10bit ? AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN_10 : AMF_VIDEO_ENCODER_HEVC_PROFILE_MAIN);
	set_hevc_property(enc, LOWLATENCY_MODE, false);
	set_hevc_property(enc, OUTPUT_COLOR_PROFILE, enc->amf_color_profile);
	set_hevc_property(enc, OUTPUT_TRANSFER_CHARACTERISTIC, enc->amf_characteristic);
	set_hevc_property(enc, OUTPUT_COLOR_PRIMARIES, enc->amf_primaries);
	set_hevc_property(enc, NOMINAL_RANGE, enc->full_range);
	set_hevc_property(enc, FRAMERATE, enc->amf_frame_rate);

	if (is_hdr) {
		const int hdr_nominal_peak_level = pq ? (int)obs_get_video_hdr_nominal_peak_level() : (hlg ? 1000 : 0);

		AMFBufferPtr buf;
		enc->amf_context->AllocBuffer(AMF_MEMORY_HOST, sizeof(AMFHDRMetadata), &buf);
		AMFHDRMetadata *md = (AMFHDRMetadata *)buf->GetNative();
		md->redPrimary[0] = amf_hdr_primary(17, 25);
		md->redPrimary[1] = amf_hdr_primary(8, 25);
		md->greenPrimary[0] = amf_hdr_primary(53, 200);
		md->greenPrimary[1] = amf_hdr_primary(69, 100);
		md->bluePrimary[0] = amf_hdr_primary(3, 20);
		md->bluePrimary[1] = amf_hdr_primary(3, 50);
		md->whitePoint[0] = amf_hdr_primary(3127, 10000);
		md->whitePoint[1] = amf_hdr_primary(329, 1000);
		md->minMasteringLuminance = 0;
		md->maxMasteringLuminance = amf_make_lum(hdr_nominal_peak_level);
		md->maxContentLightLevel = hdr_nominal_peak_level;
		md->maxFrameAverageLightLevel = hdr_nominal_peak_level;
		set_hevc_property(enc, INPUT_HDR_METADATA, buf);
	}

	amf_hevc_init(enc, settings);

	res = enc->amf_encoder->Init(enc->amf_format, enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Init failed", res);

	res = enc->amf_encoder->GetProperty(AMF_VIDEO_ENCODER_HEVC_EXTRADATA, &p);
	if (res == AMF_OK && p.type == AMF_VARIANT_INTERFACE)
		enc->header = AMFBufferPtr(p.pInterface);
}

static void *amf_hevc_create_texencode(obs_data_t *settings, obs_encoder_t *encoder)
try {
	check_texture_encode_capability(encoder, amf_codec_type::HEVC);

	std::unique_ptr<amf_texencode> enc = std::make_unique<amf_texencode>();
	enc->encoder = encoder;
	enc->encoder_str = "texture-amf-h265";

	if (!amf_init_d3d11(enc.get()))
		throw "Failed to create D3D11";

	amf_hevc_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[texture-amf-h265] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return obs_encoder_create_rerouted(encoder, "h265_fallback_amf");

} catch (const char *err) {
	blog(LOG_ERROR, "[texture-amf-h265] %s: %s", __FUNCTION__, err);
	return obs_encoder_create_rerouted(encoder, "h265_fallback_amf");
}

static void *amf_hevc_create_fallback(obs_data_t *settings, obs_encoder_t *encoder)
try {
	std::unique_ptr<amf_fallback> enc = std::make_unique<amf_fallback>();
	enc->encoder = encoder;
	enc->encoder_str = "fallback-amf-h265";

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
		break;
	case VIDEO_FORMAT_P216:
	case VIDEO_FORMAT_P416: {
		const char *const text = obs_module_text("AMF.16bitUnsupported");
		obs_encoder_set_last_error(encoder, text);
		throw text;
	}
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text = obs_module_text("AMF.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			throw text;
		}
		}
	}

	amf_hevc_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[fallback-amf-h265] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return nullptr;

} catch (const char *err) {
	blog(LOG_ERROR, "[fallback-amf-h265] %s: %s", __FUNCTION__, err);
	return nullptr;
}

static void register_hevc()
{
	struct obs_encoder_info amf_encoder_info = {};
	amf_encoder_info.id = "h265_texture_amf";
	amf_encoder_info.type = OBS_ENCODER_VIDEO;
	amf_encoder_info.codec = "hevc";
	amf_encoder_info.get_name = amf_hevc_get_name;
	amf_encoder_info.create = amf_hevc_create_texencode;
	amf_encoder_info.destroy = amf_destroy;
	amf_encoder_info.update = amf_hevc_update;
	amf_encoder_info.encode_texture = amf_encode_tex;
	amf_encoder_info.get_defaults = amf_defaults;
	amf_encoder_info.get_properties = amf_hevc_properties;
	amf_encoder_info.get_extra_data = amf_extra_data;
	amf_encoder_info.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;

	obs_register_encoder(&amf_encoder_info);

	amf_encoder_info.id = "h265_fallback_amf";
	amf_encoder_info.caps = OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;
	amf_encoder_info.encode_texture = nullptr;
	amf_encoder_info.create = amf_hevc_create_fallback;
	amf_encoder_info.encode = amf_encode_fallback;
	amf_encoder_info.get_video_info = h265_video_info_fallback;

	obs_register_encoder(&amf_encoder_info);
}

#endif //ENABLE_HEVC

/* ========================================================================= */
/* AV1 Implementation                                                        */

static const char *amf_av1_get_name(void *)
{
	return "AMD HW AV1";
}

static inline int get_av1_preset(amf_base *enc, const char *preset)
{
	if (astrcmpi(preset, "highquality") == 0)
		return AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_HIGH_QUALITY;
	else if (astrcmpi(preset, "quality") == 0)
		return AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_QUALITY;
	else if (astrcmpi(preset, "balanced") == 0)
		return AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_BALANCED;
	else if (astrcmpi(preset, "speed") == 0)
		return AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_SPEED;

	return AMF_VIDEO_ENCODER_AV1_QUALITY_PRESET_BALANCED;
}

static inline int get_av1_rate_control(const char *rc_str)
{
	if (astrcmpi(rc_str, "cqp") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CONSTANT_QP;
	else if (astrcmpi(rc_str, "vbr_lat") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_LATENCY_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "vbr") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR;
	else if (astrcmpi(rc_str, "cbr") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CBR;
	else if (astrcmpi(rc_str, "qvbr") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqvbr") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_HIGH_QUALITY_VBR;
	else if (astrcmpi(rc_str, "hqcbr") == 0)
		return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_HIGH_QUALITY_CBR;

	return AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CBR;
}

static inline int get_av1_profile(obs_data_t *settings)
{
	const char *profile = obs_data_get_string(settings, "profile");

	if (astrcmpi(profile, "main") == 0)
		return AMF_VIDEO_ENCODER_AV1_PROFILE_MAIN;

	return AMF_VIDEO_ENCODER_AV1_PROFILE_MAIN;
}

static void amf_av1_update_data(amf_base *enc, int rc, int64_t bitrate, int64_t cq_value)
{
	if (rc != AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_CONSTANT_QP &&
	    rc != AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_QUALITY_VBR) {
		set_av1_property(enc, TARGET_BITRATE, bitrate);
		set_av1_property(enc, PEAK_BITRATE, bitrate);
		set_av1_property(enc, VBV_BUFFER_SIZE, bitrate);

		if (rc == AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_CBR) {
			set_av1_property(enc, FILLER_DATA, true);
		} else if (rc == AMF_VIDEO_ENCODER_RATE_CONTROL_METHOD_PEAK_CONSTRAINED_VBR ||
			   rc == AMF_VIDEO_ENCODER_AV1_RATE_CONTROL_METHOD_HIGH_QUALITY_VBR) {
			set_av1_property(enc, PEAK_BITRATE, bitrate * 1.5);
		}
	} else {
		int64_t qp = cq_value * 4;
		set_av1_property(enc, QVBR_QUALITY_LEVEL, qp / 4);
		set_av1_property(enc, Q_INDEX_INTRA, qp);
		set_av1_property(enc, Q_INDEX_INTER, qp);
	}
}

static bool amf_av1_update(void *data, obs_data_t *settings)
try {
	amf_base *enc = (amf_base *)data;

	if (enc->first_update) {
		enc->first_update = false;
		return true;
	}

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t cq_level = obs_data_get_int(settings, "cqp");
	const char *rc_str = obs_data_get_string(settings, "rate_control");
	int rc = get_av1_rate_control(rc_str);
	AMF_RESULT res = AMF_OK;

	amf_av1_update_data(enc, rc, bitrate * 1000, cq_level);

	res = enc->amf_encoder->Flush();
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Flush failed", res);

	res = enc->amf_encoder->ReInit(enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::ReInit failed", res);

	return true;

} catch (const amf_error &err) {
	amf_base *enc = (amf_base *)data;
	error("%s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return false;
}

static bool amf_av1_init(void *data, obs_data_t *settings)
{
	amf_base *enc = (amf_base *)data;

	int64_t bitrate = obs_data_get_int(settings, "bitrate");
	int64_t qp = obs_data_get_int(settings, "cqp");
	const char *preset = obs_data_get_string(settings, "preset");
	const char *profile = obs_data_get_string(settings, "profile");
	const char *rc_str = obs_data_get_string(settings, "rate_control");

	int rc = get_av1_rate_control(rc_str);
	set_av1_property(enc, RATE_CONTROL_METHOD, rc);

	amf_av1_update_data(enc, rc, bitrate * 1000, qp);

	set_av1_property(enc, ENFORCE_HRD, true);

	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	int gop_size = (keyint_sec) ? keyint_sec * enc->fps_num / enc->fps_den : 250;
	set_av1_property(enc, GOP_SIZE, gop_size);

	const char *ffmpeg_opts = obs_data_get_string(settings, "ffmpeg_opts");
	if (ffmpeg_opts && *ffmpeg_opts) {
		struct obs_options opts = obs_parse_options(ffmpeg_opts);
		for (size_t i = 0; i < opts.count; i++) {
			amf_apply_opt(enc, &opts.options[i]);
		}
		obs_free_options(opts);
	}

	check_preset_compatibility(enc, preset);

	if (!ffmpeg_opts || !*ffmpeg_opts)
		ffmpeg_opts = "(none)";

	info("settings:\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\tparams:       %s",
	     rc_str, bitrate, qp, gop_size, preset, profile, enc->cx, enc->cy, ffmpeg_opts);

	return true;
}

static void amf_av1_create_internal(amf_base *enc, obs_data_t *settings)
{
	enc->codec = amf_codec_type::AV1;

	if (!amf_create_encoder(enc))
		throw "Failed to create encoder";

	AMFCapsPtr caps;
	AMF_RESULT res = enc->amf_encoder->GetCaps(&caps);
	if (res == AMF_OK) {
		caps->GetProperty(AMF_VIDEO_ENCODER_AV1_CAP_MAX_THROUGHPUT, &enc->max_throughput);
		caps->GetProperty(AMF_VIDEO_ENCODER_AV1_CAP_REQUESTED_THROUGHPUT, &enc->requested_throughput);
		/* For some reason there's no specific CAP for AV1, but should always be supported */
		enc->roi_supported = true;
	}

	const bool is10bit = enc->amf_format == AMF_SURFACE_P010;
	const char *preset = obs_data_get_string(settings, "preset");

	set_av1_property(enc, FRAMESIZE, AMFConstructSize(enc->cx, enc->cy));
	set_av1_property(enc, USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCODING);
	set_av1_property(enc, ALIGNMENT_MODE, AMF_VIDEO_ENCODER_AV1_ALIGNMENT_MODE_NO_RESTRICTIONS);
	set_av1_property(enc, QUALITY_PRESET, get_av1_preset(enc, preset));
	set_av1_property(enc, COLOR_BIT_DEPTH, is10bit ? AMF_COLOR_BIT_DEPTH_10 : AMF_COLOR_BIT_DEPTH_8);
	set_av1_property(enc, PROFILE, get_av1_profile(settings));
	set_av1_property(enc, ENCODING_LATENCY_MODE, AMF_VIDEO_ENCODER_AV1_ENCODING_LATENCY_MODE_NONE);
	// set_av1_property(enc, RATE_CONTROL_PREENCODE, true);
	set_av1_property(enc, OUTPUT_COLOR_PROFILE, enc->amf_color_profile);
	set_av1_property(enc, OUTPUT_TRANSFER_CHARACTERISTIC, enc->amf_characteristic);
	set_av1_property(enc, OUTPUT_COLOR_PRIMARIES, enc->amf_primaries);
	set_av1_property(enc, FRAMERATE, enc->amf_frame_rate);

	amf_av1_init(enc, settings);

	res = enc->amf_encoder->Init(enc->amf_format, enc->cx, enc->cy);
	if (res != AMF_OK)
		throw amf_error("AMFComponent::Init failed", res);

	AMFVariant p;
	res = enc->amf_encoder->GetProperty(AMF_VIDEO_ENCODER_AV1_EXTRA_DATA, &p);
	if (res == AMF_OK && p.type == AMF_VARIANT_INTERFACE)
		enc->header = AMFBufferPtr(p.pInterface);
}

static void *amf_av1_create_texencode(obs_data_t *settings, obs_encoder_t *encoder)
try {
	check_texture_encode_capability(encoder, amf_codec_type::AV1);

	std::unique_ptr<amf_texencode> enc = std::make_unique<amf_texencode>();
	enc->encoder = encoder;
	enc->encoder_str = "texture-amf-av1";

	if (!amf_init_d3d11(enc.get()))
		throw "Failed to create D3D11";

	amf_av1_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[texture-amf-av1] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return obs_encoder_create_rerouted(encoder, "av1_fallback_amf");

} catch (const char *err) {
	blog(LOG_ERROR, "[texture-amf-av1] %s: %s", __FUNCTION__, err);
	return obs_encoder_create_rerouted(encoder, "av1_fallback_amf");
}

static void *amf_av1_create_fallback(obs_data_t *settings, obs_encoder_t *encoder)
try {
	std::unique_ptr<amf_fallback> enc = std::make_unique<amf_fallback>();
	enc->encoder = encoder;
	enc->encoder_str = "fallback-amf-av1";

	video_t *video = obs_encoder_video(encoder);
	const struct video_output_info *voi = video_output_get_info(video);
	switch (voi->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010: {
		break;
	}
	case VIDEO_FORMAT_P216:
	case VIDEO_FORMAT_P416: {
		const char *const text = obs_module_text("AMF.16bitUnsupported");
		obs_encoder_set_last_error(encoder, text);
		throw text;
	}
	default:
		switch (voi->colorspace) {
		case VIDEO_CS_2100_PQ:
		case VIDEO_CS_2100_HLG: {
			const char *const text = obs_module_text("AMF.8bitUnsupportedHdr");
			obs_encoder_set_last_error(encoder, text);
			throw text;
		}
		}
	}

	amf_av1_create_internal(enc.get(), settings);
	return enc.release();

} catch (const amf_error &err) {
	blog(LOG_ERROR, "[fallback-amf-av1] %s: %s: %ls", __FUNCTION__, err.str, amf_trace->GetResultText(err.res));
	return nullptr;

} catch (const char *err) {
	blog(LOG_ERROR, "[fallback-amf-av1] %s: %s", __FUNCTION__, err);
	return nullptr;
}

static void amf_av1_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "bitrate", 2500);
	obs_data_set_default_int(settings, "cqp", 20);
	obs_data_set_default_string(settings, "rate_control", "CBR");
	obs_data_set_default_string(settings, "preset", "quality");
	obs_data_set_default_string(settings, "profile", "high");
}

static void register_av1()
{
	struct obs_encoder_info amf_encoder_info = {};
	amf_encoder_info.id = "av1_texture_amf";
	amf_encoder_info.type = OBS_ENCODER_VIDEO;
	amf_encoder_info.codec = "av1";
	amf_encoder_info.get_name = amf_av1_get_name;
	amf_encoder_info.create = amf_av1_create_texencode;
	amf_encoder_info.destroy = amf_destroy;
	amf_encoder_info.update = amf_av1_update;
	amf_encoder_info.encode_texture = amf_encode_tex;
	amf_encoder_info.get_defaults = amf_av1_defaults;
	amf_encoder_info.get_properties = amf_av1_properties;
	amf_encoder_info.get_extra_data = amf_extra_data;
	amf_encoder_info.caps = OBS_ENCODER_CAP_PASS_TEXTURE | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;

	obs_register_encoder(&amf_encoder_info);

	amf_encoder_info.id = "av1_fallback_amf";
	amf_encoder_info.caps = OBS_ENCODER_CAP_INTERNAL | OBS_ENCODER_CAP_DYN_BITRATE | OBS_ENCODER_CAP_ROI;
	amf_encoder_info.encode_texture = nullptr;
	amf_encoder_info.create = amf_av1_create_fallback;
	amf_encoder_info.encode = amf_encode_fallback;
	amf_encoder_info.get_video_info = av1_video_info_fallback;

	obs_register_encoder(&amf_encoder_info);
}

/* ========================================================================= */
/* Global Stuff                                                              */

static bool enum_luids(void *param, uint32_t idx, uint64_t luid)
{
	std::stringstream &cmd = *(std::stringstream *)param;
	cmd << " " << std::hex << luid;
	UNUSED_PARAMETER(idx);
	return true;
}

extern "C" void amf_load(void)
try {
	AMF_RESULT res;
	HMODULE amf_module_test;

	/* Check if the DLL is present before running the more expensive */
	/* obs-amf-test.exe, but load it as data so it can't crash us    */
	amf_module_test = LoadLibraryExW(AMF_DLL_NAME, nullptr, LOAD_LIBRARY_AS_DATAFILE);
	if (!amf_module_test)
		throw "No AMF library";
	FreeLibrary(amf_module_test);

	/* ----------------------------------- */
	/* Check for supported codecs          */

	BPtr<char> test_exe = os_get_executable_path_ptr("obs-amf-test.exe");
	std::stringstream cmd;
	std::string caps_str;

	cmd << '"';
	cmd << test_exe;
	cmd << '"';
	enum_graphics_device_luids(enum_luids, &cmd);

	os_process_pipe_t *pp = os_process_pipe_create(cmd.str().c_str(), "r");
	if (!pp)
		throw "Failed to launch the AMF test process I guess";

	for (;;) {
		char data[2048];
		size_t len = os_process_pipe_read(pp, (uint8_t *)data, sizeof(data));
		if (!len)
			break;

		caps_str.append(data, len);
	}

	os_process_pipe_destroy(pp);

	if (caps_str.empty())
		throw "Seems the AMF test subprocess crashed. "
		      "Better there than here I guess. "
		      "Let's just skip loading AMF then I suppose.";

	ConfigFile config;
	if (config.OpenString(caps_str.c_str()) != 0)
		throw "Failed to open config string";

	const char *error = config_get_string(config, "error", "string");
	if (error)
		throw std::string(error);

	uint32_t adapter_count = (uint32_t)config_num_sections(config);
	bool avc_supported = false;
	bool hevc_supported = false;
	bool av1_supported = false;

	for (uint32_t i = 0; i < adapter_count; i++) {
		std::string section = std::to_string(i);
		adapter_caps &info = caps[i];

		info.is_amd = config_get_bool(config, section.c_str(), "is_amd");
		info.supports_avc = config_get_bool(config, section.c_str(), "supports_avc");
		info.supports_hevc = config_get_bool(config, section.c_str(), "supports_hevc");
		info.supports_av1 = config_get_bool(config, section.c_str(), "supports_av1");

		avc_supported |= info.supports_avc;
		hevc_supported |= info.supports_hevc;
		av1_supported |= info.supports_av1;
	}

	if (!avc_supported && !hevc_supported && !av1_supported)
		throw "Neither AVC, HEVC, nor AV1 are supported by any devices";

	/* ----------------------------------- */
	/* Init AMF                            */

	amf_module = LoadLibraryW(AMF_DLL_NAME);
	if (!amf_module)
		throw "AMF library failed to load";

	AMFInit_Fn init = (AMFInit_Fn)GetProcAddress(amf_module, AMF_INIT_FUNCTION_NAME);
	if (!init)
		throw "Failed to get AMFInit address";

	res = init(AMF_FULL_VERSION, &amf_factory);
	if (res != AMF_OK)
		throw amf_error("AMFInit failed", res);

	res = amf_factory->GetTrace(&amf_trace);
	if (res != AMF_OK)
		throw amf_error("GetTrace failed", res);

	AMFQueryVersion_Fn get_ver = (AMFQueryVersion_Fn)GetProcAddress(amf_module, AMF_QUERY_VERSION_FUNCTION_NAME);
	if (!get_ver)
		throw "Failed to get AMFQueryVersion address";

	res = get_ver(&amf_version);
	if (res != AMF_OK)
		throw amf_error("AMFQueryVersion failed", res);

#ifndef DEBUG_AMF_STUFF
	amf_trace->EnableWriter(AMF_TRACE_WRITER_DEBUG_OUTPUT, false);
	amf_trace->EnableWriter(AMF_TRACE_WRITER_CONSOLE, false);
#endif

	/* ----------------------------------- */
	/* Register encoders                   */

	if (avc_supported)
		register_avc();
#if ENABLE_HEVC
	if (hevc_supported)
		register_hevc();
#endif
	if (av1_supported)
		register_av1();

} catch (const std::string &str) {
	/* doing debug here because string exceptions indicate the user is
	 * probably not using AMD */
	blog(LOG_DEBUG, "%s: %s", __FUNCTION__, str.c_str());

} catch (const char *str) {
	/* doing debug here because string exceptions indicate the user is
	 * probably not using AMD */
	blog(LOG_DEBUG, "%s: %s", __FUNCTION__, str);

} catch (const amf_error &err) {
	/* doing an error here because it means at least the library has loaded
	 * successfully, so they probably have AMD at this point */
	blog(LOG_ERROR, "%s: %s: 0x%lX", __FUNCTION__, err.str, (uint32_t)err.res);
}

extern "C" void amf_unload(void)
{
	if (amf_module && amf_trace) {
		amf_trace->TraceFlush();
		amf_trace->UnregisterWriter(L"obs_amf_trace_writer");
	}
}
