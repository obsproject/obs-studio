#include <obs-module.h>
#include <util/platform.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef _WIN32
#define INITGUID
#include <dxgi.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include "vaapi-utils.h"

#define LIBAVUTIL_VAAPI_AVAILABLE
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "FFmpeg based sources/outputs/encoders";
}

extern struct obs_source_info ffmpeg_source;
extern struct obs_output_info ffmpeg_output;
extern struct obs_output_info ffmpeg_muxer;
extern struct obs_output_info ffmpeg_mpegts_muxer;
extern struct obs_output_info replay_buffer;
extern struct obs_output_info ffmpeg_hls_muxer;
extern struct obs_encoder_info aac_encoder_info;
extern struct obs_encoder_info opus_encoder_info;
extern struct obs_encoder_info pcm_encoder_info;
extern struct obs_encoder_info pcm24_encoder_info;
extern struct obs_encoder_info pcm32_encoder_info;
extern struct obs_encoder_info alac_encoder_info;
extern struct obs_encoder_info flac_encoder_info;
extern struct obs_encoder_info oh264_encoder_info;
#ifdef ENABLE_FFMPEG_NVENC
extern struct obs_encoder_info h264_nvenc_encoder_info;
#ifdef ENABLE_HEVC
extern struct obs_encoder_info hevc_nvenc_encoder_info;
#endif
#endif
extern struct obs_encoder_info svt_av1_encoder_info;
extern struct obs_encoder_info aom_av1_encoder_info;

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
extern struct obs_encoder_info h264_vaapi_encoder_info;
extern struct obs_encoder_info h264_vaapi_encoder_tex_info;
extern struct obs_encoder_info av1_vaapi_encoder_info;
extern struct obs_encoder_info av1_vaapi_encoder_tex_info;
#ifdef ENABLE_HEVC
extern struct obs_encoder_info hevc_vaapi_encoder_info;
extern struct obs_encoder_info hevc_vaapi_encoder_tex_info;
#endif
#endif

#ifdef ENABLE_FFMPEG_NVENC

static const char *nvenc_check_name = "nvenc_check";

static const int blacklisted_adapters[] = {
	0x1298, // GK208M [GeForce GT 720M]
	0x1140, // GF117M [GeForce 610M/710M/810M/820M / GT 620M/625M/630M/720M]
	0x1293, // GK208M [GeForce GT 730M]
	0x1290, // GK208M [GeForce GT 730M]
	0x0fe1, // GK107M [GeForce GT 730M]
	0x0fdf, // GK107M [GeForce GT 740M]
	0x1294, // GK208M [GeForce GT 740M]
	0x1292, // GK208M [GeForce GT 740M]
	0x0fe2, // GK107M [GeForce GT 745M]
	0x0fe3, // GK107M [GeForce GT 745M]
	0x1140, // GF117M [GeForce 610M/710M/810M/820M / GT 620M/625M/630M/720M]
	0x0fed, // GK107M [GeForce 820M]
	0x1340, // GM108M [GeForce 830M]
	0x1393, // GM107M [GeForce 840M]
	0x1341, // GM108M [GeForce 840M]
	0x1398, // GM107M [GeForce 845M]
	0x1390, // GM107M [GeForce 845M]
	0x1344, // GM108M [GeForce 845M]
	0x1299, // GK208BM [GeForce 920M]
	0x134f, // GM108M [GeForce 920MX]
	0x134e, // GM108M [GeForce 930MX]
	0x1349, // GM108M [GeForce 930M]
	0x1346, // GM108M [GeForce 930M]
	0x179c, // GM107 [GeForce 940MX]
	0x139c, // GM107M [GeForce 940M]
	0x1347, // GM108M [GeForce 940M]
	0x134d, // GM108M [GeForce 940MX]
	0x134b, // GM108M [GeForce 940MX]
	0x1399, // GM107M [GeForce 945M]
	0x1348, // GM108M [GeForce 945M / 945A]
	0x1d01, // GP108 [GeForce GT 1030]
	0x0fc5, // GK107 [GeForce GT 1030]
	0x174e, // GM108M [GeForce MX110]
	0x174d, // GM108M [GeForce MX130]
	0x1d10, // GP108M [GeForce MX150]
	0x1d12, // GP108M [GeForce MX150]
	0x1d11, // GP108M [GeForce MX230]
	0x1d13, // GP108M [GeForce MX250]
	0x1d52, // GP108BM [GeForce MX250]
	0x1c94, // GP107 [GeForce MX350]
	0x1c96, // GP107 [GeForce MX350]
	0x1f97, // TU117 [GeForce MX450]
	0x1f98, // TU117 [GeForce MX450]
	0x137b, // GM108GLM [Quadro M520 Mobile]
	0x1d33, // GP108GLM [Quadro P500 Mobile]
	0x137a, // GM108GLM [Quadro K620M / Quadro M500M]
};

static const size_t num_blacklisted = sizeof(blacklisted_adapters) / sizeof(blacklisted_adapters[0]);

static bool is_blacklisted(const int device_id)
{
	for (size_t i = 0; i < num_blacklisted; i++) {
		const int blacklisted_adapter = blacklisted_adapters[i];
		if (device_id == blacklisted_adapter) {
			return true;
		}
	}

	return false;
}

#ifdef _WIN32
typedef HRESULT(WINAPI *create_dxgi_proc)(const IID *, IDXGIFactory1 **);

static bool nvenc_device_available(void)
{
	static HMODULE dxgi = NULL;
	static create_dxgi_proc create = NULL;
	IDXGIFactory1 *factory;
	IDXGIAdapter1 *adapter;
	bool available = false;
	HRESULT hr;
	UINT i = 0;

	if (!dxgi) {
		dxgi = GetModuleHandleW(L"dxgi");
		if (!dxgi) {
			dxgi = LoadLibraryW(L"dxgi");
			if (!dxgi) {
				return true;
			}
		}
	}

	if (!create) {
		create = (create_dxgi_proc)GetProcAddress(dxgi, "CreateDXGIFactory1");
		if (!create) {
			return true;
		}
	}

	hr = create(&IID_IDXGIFactory1, &factory);
	if (FAILED(hr)) {
		return true;
	}

	while (factory->lpVtbl->EnumAdapters1(factory, i++, &adapter) == S_OK) {
		DXGI_ADAPTER_DESC desc;

		hr = adapter->lpVtbl->GetDesc(adapter, &desc);
		adapter->lpVtbl->Release(adapter);

		if (FAILED(hr)) {
			continue;
		}

		// 0x10de = NVIDIA Corporation
		if (desc.VendorId == 0x10de && !is_blacklisted(desc.DeviceId)) {
			available = true;
			goto finish;
		}
	}

finish:
	factory->lpVtbl->Release(factory);
	return available;
}
#endif

#if defined(__linux__)
static int get_id_from_sys(char *d_name, char *type)
{
	char file_name[1024];
	char *c;
	int id;

	snprintf(file_name, sizeof(file_name), "/sys/bus/pci/devices/%s/%s", d_name, type);
	if ((c = os_quick_read_utf8_file(file_name)) == NULL) {
		return -1;
	}
	id = strtol(c, NULL, 16);
	bfree(c);

	return id;
}

static bool nvenc_device_available(void)
{
	os_dir_t *dir;
	struct os_dirent *dirent;
	bool available = false;

	if ((dir = os_opendir("/sys/bus/pci/devices")) == NULL) {
		return true;
	}

	while ((dirent = os_readdir(dir)) != NULL) {
		int id;

		if (get_id_from_sys(dirent->d_name, "class") != 0x030000 &&
		    get_id_from_sys(dirent->d_name, "class") != 0x030200) { // 0x030000 = VGA compatible controller
									    // 0x030200 = 3D controller
			continue;
		}

		if (get_id_from_sys(dirent->d_name, "vendor") != 0x10de) { // 0x10de = NVIDIA Corporation
			continue;
		}

		if ((id = get_id_from_sys(dirent->d_name, "device")) > 0 && !is_blacklisted(id)) {
			available = true;
			break;
		}
	}

	os_closedir(dir);
	return available;
}
#endif

static bool nvenc_codec_exists(const char *name, const char *fallback)
{
	const AVCodec *nvenc = avcodec_find_encoder_by_name(name);
	if (!nvenc)
		nvenc = avcodec_find_encoder_by_name(fallback);

	return nvenc != NULL;
}

static bool nvenc_supported(bool *out_h264, bool *out_hevc)
{
	profile_start(nvenc_check_name);

	const bool h264 = nvenc_codec_exists("h264_nvenc", "nvenc_h264");
#ifdef ENABLE_HEVC
	const bool hevc = nvenc_codec_exists("hevc_nvenc", "nvenc_hevc");
#else
	const bool hevc = false;
#endif

	bool success = h264 || hevc;
	if (success) {
#ifdef _WIN32
		success = nvenc_device_available();
#elif defined(__linux__)
		success = nvenc_device_available();
		if (success) {
			void *const lib = os_dlopen("libnvidia-encode.so.1");
			success = lib != NULL;
			if (success)
				os_dlclose(lib);
		}
#else
		void *const lib = os_dlopen("libnvidia-encode.so.1");
		success = lib != NULL;
		if (success)
			os_dlclose(lib);
#endif

		if (success) {
			*out_h264 = h264;
			*out_hevc = hevc;
		}
	}

	profile_end(nvenc_check_name);
	return success;
}

#endif

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
static bool h264_vaapi_supported(void)
{
	const AVCodec *vaenc = avcodec_find_encoder_by_name("h264_vaapi");

	if (!vaenc)
		return false;

	/* NOTE: If default device is NULL, it means there is no device
	 * that support H264. */
	return vaapi_get_h264_default_device() != NULL;
}

static bool av1_vaapi_supported(void)
{
	const AVCodec *vaenc = avcodec_find_encoder_by_name("av1_vaapi");

	if (!vaenc)
		return false;

	/* NOTE: If default device is NULL, it means there is no device
	 * that support AV1. */
	return vaapi_get_av1_default_device() != NULL;
}

#ifdef ENABLE_HEVC
static bool hevc_vaapi_supported(void)
{
	const AVCodec *vaenc = avcodec_find_encoder_by_name("hevc_vaapi");

	if (!vaenc)
		return false;

	/* NOTE: If default device is NULL, it means there is no device
	 * that support HEVC. */
	return vaapi_get_hevc_default_device() != NULL;
}
#endif
#endif

#ifdef _WIN32
extern void amf_load(void);
extern void amf_unload(void);
#endif

#if ENABLE_FFMPEG_LOGGING
extern void obs_ffmpeg_load_logging(void);
extern void obs_ffmpeg_unload_logging(void);
#endif

static void register_encoder_if_available(struct obs_encoder_info *info, const char *id)
{
	const AVCodec *c = avcodec_find_encoder_by_name(id);
	if (c) {
		obs_register_encoder(info);
	}
}

bool obs_module_load(void)
{
	obs_register_source(&ffmpeg_source);
	obs_register_output(&ffmpeg_output);
	obs_register_output(&ffmpeg_muxer);
	obs_register_output(&ffmpeg_mpegts_muxer);
	obs_register_output(&ffmpeg_hls_muxer);
	obs_register_output(&replay_buffer);
	obs_register_encoder(&aac_encoder_info);
	register_encoder_if_available(&oh264_encoder_info, "libopenh264");
	register_encoder_if_available(&svt_av1_encoder_info, "libsvtav1");
	register_encoder_if_available(&aom_av1_encoder_info, "libaom-av1");
	obs_register_encoder(&opus_encoder_info);
	obs_register_encoder(&pcm_encoder_info);
	obs_register_encoder(&pcm24_encoder_info);
	obs_register_encoder(&pcm32_encoder_info);
	obs_register_encoder(&alac_encoder_info);
	obs_register_encoder(&flac_encoder_info);
#ifdef ENABLE_FFMPEG_NVENC
	bool h264 = false;
	bool hevc = false;
	if (nvenc_supported(&h264, &hevc)) {
		blog(LOG_INFO, "NVENC supported");

		if (h264)
			obs_register_encoder(&h264_nvenc_encoder_info);
#ifdef ENABLE_HEVC
		if (hevc)
			obs_register_encoder(&hevc_nvenc_encoder_info);
#endif
	}
#endif

#ifdef _WIN32
	amf_load();
#endif

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
	const char *libva_env = getenv("LIBVA_DRIVER_NAME");
	if (!!libva_env)
		blog(LOG_WARNING, "LIBVA_DRIVER_NAME variable is set,"
				  " this could prevent FFmpeg VAAPI from working correctly");

	if (h264_vaapi_supported()) {
		blog(LOG_INFO, "FFmpeg VAAPI H264 encoding supported");
		obs_register_encoder(&h264_vaapi_encoder_info);
		obs_register_encoder(&h264_vaapi_encoder_tex_info);
	} else {
		blog(LOG_INFO, "FFmpeg VAAPI H264 encoding not supported");
	}

	if (av1_vaapi_supported()) {
		blog(LOG_INFO, "FFmpeg VAAPI AV1 encoding supported");
		obs_register_encoder(&av1_vaapi_encoder_info);
		obs_register_encoder(&av1_vaapi_encoder_tex_info);
	} else {
		blog(LOG_INFO, "FFmpeg VAAPI AV1 encoding not supported");
	}

#ifdef ENABLE_HEVC
	if (hevc_vaapi_supported()) {
		blog(LOG_INFO, "FFmpeg VAAPI HEVC encoding supported");
		obs_register_encoder(&hevc_vaapi_encoder_info);
		obs_register_encoder(&hevc_vaapi_encoder_tex_info);
	} else {
		blog(LOG_INFO, "FFmpeg VAAPI HEVC encoding not supported");
	}
#endif
#endif

#if ENABLE_FFMPEG_LOGGING
	obs_ffmpeg_load_logging();
#endif
	return true;
}

void obs_module_unload(void)
{
#if ENABLE_FFMPEG_LOGGING
	obs_ffmpeg_unload_logging();
#endif

#ifdef _WIN32
	amf_unload();
#endif
}
