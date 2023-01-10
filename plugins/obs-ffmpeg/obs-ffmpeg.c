#include <util/dstr.h>
#include <obs-module.h>
#include <util/platform.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "obs-ffmpeg-config.h"

#ifdef _WIN32
#include <dxgi.h>
#include <util/windows/win-version.h>

#include "jim-nvenc.h"
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && \
	LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 27, 100)
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
extern struct obs_encoder_info h264_nvenc_encoder_info;
#ifdef ENABLE_HEVC
extern struct obs_encoder_info hevc_nvenc_encoder_info;
#endif
extern struct obs_encoder_info svt_av1_encoder_info;
extern struct obs_encoder_info aom_av1_encoder_info;

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
extern struct obs_encoder_info vaapi_encoder_info;
#endif

#ifndef __APPLE__

static const char *nvenc_check_name = "nvenc_check";

#if defined(_WIN32) || defined(__linux__)
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
	0x1f97, // TU117 [GeForce MX450]
	0x137b, // GM108GLM [Quadro M520 Mobile]
	0x1d33, // GP108GLM [Quadro P500 Mobile]
	0x137a, // GM108GLM [Quadro K620M / Quadro M500M]
};

static const size_t num_blacklisted =
	sizeof(blacklisted_adapters) / sizeof(blacklisted_adapters[0]);

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
#endif

#if defined(_WIN32)
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
		create = (create_dxgi_proc)GetProcAddress(dxgi,
							  "CreateDXGIFactory1");
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

	snprintf(file_name, sizeof(file_name), "/sys/bus/pci/devices/%s/%s",
		 d_name, type);
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
		    get_id_from_sys(dirent->d_name, "class") !=
			    0x030200) { // 0x030000 = VGA compatible controller
					// 0x030200 = 3D controller
			continue;
		}

		if (get_id_from_sys(dirent->d_name, "vendor") !=
		    0x10de) { // 0x10de = NVIDIA Corporation
			continue;
		}

		if ((id = get_id_from_sys(dirent->d_name, "device")) > 0 &&
		    !is_blacklisted(id)) {
			available = true;
			break;
		}
	}

	os_closedir(dir);
	return available;
}
#endif

#ifdef _WIN32
extern bool load_nvenc_lib(void);
extern uint32_t get_nvenc_ver();
#endif

/* please remove this annoying garbage and the associated garbage in
 * obs-ffmpeg-nvenc.c when ubuntu 20.04 is finally gone for good. */

#ifdef __linux__
bool ubuntu_20_04_nvenc_fallback = false;

static void do_nvenc_check_for_ubuntu_20_04(void)
{
	FILE *fp;
	char *line = NULL;
	size_t linecap = 0;

	fp = fopen("/etc/os-release", "r");
	if (!fp) {
		return;
	}

	while (getline(&line, &linecap, fp) != -1) {
		if (strncmp(line, "VERSION_CODENAME=focal", 22) == 0) {
			ubuntu_20_04_nvenc_fallback = true;
		}
	}

	fclose(fp);
	free(line);
}
#endif

static bool nvenc_codec_exists(const char *name, const char *fallback)
{
	const AVCodec *nvenc = avcodec_find_encoder_by_name(name);
	if (!nvenc)
		nvenc = avcodec_find_encoder_by_name(fallback);

	return nvenc != NULL;
}

static bool nvenc_supported(bool *out_h264, bool *out_hevc, bool *out_av1)
{
	profile_start(nvenc_check_name);

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif

	const bool h264 = nvenc_codec_exists("h264_nvenc", "nvenc_h264");
#ifdef ENABLE_HEVC
	const bool hevc = nvenc_codec_exists("hevc_nvenc", "nvenc_hevc");
#else
	const bool hevc = false;
#endif

	bool av1 = false;

	bool success = h264 || hevc;
	if (success) {
#if defined(_WIN32)
		success = nvenc_device_available() && load_nvenc_lib();
		av1 = success && (get_nvenc_ver() >= ((12 << 4) | 0));

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
			*out_av1 = av1;
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
#endif

#ifdef _WIN32
extern void jim_nvenc_load(bool h264, bool hevc, bool av1);
extern void jim_nvenc_unload(void);
extern void amf_load(void);
extern void amf_unload(void);
#endif

#if ENABLE_FFMPEG_LOGGING
extern void obs_ffmpeg_load_logging(void);
extern void obs_ffmpeg_unload_logging(void);
#endif

static void register_encoder_if_available(struct obs_encoder_info *info,
					  const char *id)
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
	register_encoder_if_available(&svt_av1_encoder_info, "libsvtav1");
	register_encoder_if_available(&aom_av1_encoder_info, "libaom-av1");
	obs_register_encoder(&opus_encoder_info);
#ifndef __APPLE__
	bool h264 = false;
	bool hevc = false;
	bool av1 = false;
	if (nvenc_supported(&h264, &hevc, &av1)) {
		blog(LOG_INFO, "NVENC supported");

#ifdef __linux__
		/* why are we here? just to suffer? */
		do_nvenc_check_for_ubuntu_20_04();
#endif

#ifdef _WIN32
		if (get_win_ver_int() > 0x0601) {
			jim_nvenc_load(h264, hevc, av1);
		} else {
			// if on Win 7, new nvenc isn't available so there's
			// no nvenc encoder for the user to select, expose
			// the old encoder directly
			if (h264) {
				h264_nvenc_encoder_info.caps &=
					~OBS_ENCODER_CAP_INTERNAL;
			}
#ifdef ENABLE_HEVC
			if (hevc) {
				hevc_nvenc_encoder_info.caps &=
					~OBS_ENCODER_CAP_INTERNAL;
			}
#endif
		}
#endif
		if (h264)
			obs_register_encoder(&h264_nvenc_encoder_info);
#ifdef ENABLE_HEVC
		if (hevc)
			obs_register_encoder(&hevc_nvenc_encoder_info);
#endif
	}

#ifdef _WIN32
	amf_load();
#endif

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
	const char *libva_env = getenv("LIBVA_DRIVER_NAME");
	if (!!libva_env)
		blog(LOG_WARNING,
		     "LIBVA_DRIVER_NAME variable is set,"
		     " this could prevent FFmpeg VAAPI from working correctly");

	if (h264_vaapi_supported()) {
		blog(LOG_INFO, "FFmpeg VAAPI H264 encoding supported");
		obs_register_encoder(&vaapi_encoder_info);
	} else {
		blog(LOG_INFO, "FFmpeg VAAPI H264 encoding not supported");
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
	jim_nvenc_unload();
#endif
}
