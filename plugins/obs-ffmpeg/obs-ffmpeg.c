#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>

#ifdef _WIN32
#include <dxgi.h>
#include <util/dstr.h>
#include <util/windows/win-version.h>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "FFmpeg based sources/outputs/encoders";
}

extern struct obs_source_info  ffmpeg_source;
extern struct obs_output_info  ffmpeg_output;
extern struct obs_output_info  ffmpeg_muxer;
extern struct obs_output_info  replay_buffer;
extern struct obs_encoder_info aac_encoder_info;
extern struct obs_encoder_info opus_encoder_info;
extern struct obs_encoder_info nvenc_encoder_info;

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(55, 27, 100)
#define LIBAVUTIL_VAAPI_AVAILABLE
#endif

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
extern struct obs_encoder_info vaapi_encoder_info;
#endif

static DARRAY(struct log_context {
	void *context;
	char str[4096];
	int print_prefix;
} *) active_log_contexts;
static DARRAY(struct log_context *) cached_log_contexts;
pthread_mutex_t log_contexts_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct log_context *create_or_fetch_log_context(void *context)
{
	pthread_mutex_lock(&log_contexts_mutex);
	for (size_t i = 0; i < active_log_contexts.num; i++) {
		if (context == active_log_contexts.array[i]->context) {
			pthread_mutex_unlock(&log_contexts_mutex);
			return active_log_contexts.array[i];
		}
	}

	struct log_context *new_log_context = NULL;

	size_t cnt = cached_log_contexts.num;
	if (!!cnt) {
		new_log_context = cached_log_contexts.array[cnt - 1];
		da_pop_back(cached_log_contexts);
	}

	if (!new_log_context)
		new_log_context = bzalloc(sizeof(struct log_context));

	new_log_context->context = context;
	new_log_context->str[0] = '\0';
	new_log_context->print_prefix = 1;

	da_push_back(active_log_contexts, &new_log_context);

	pthread_mutex_unlock(&log_contexts_mutex);

	return new_log_context;
}

static void destroy_log_context(struct log_context *log_context)
{
	pthread_mutex_lock(&log_contexts_mutex);
	da_erase_item(active_log_contexts, &log_context);
	da_push_back(cached_log_contexts, &log_context);
	pthread_mutex_unlock(&log_contexts_mutex);
}

static void ffmpeg_log_callback(void* context, int level, const char* format,
	va_list args)
{
	if (format == NULL)
		return;

	struct log_context *log_context = create_or_fetch_log_context(context);

	char *str = log_context->str;

	av_log_format_line(context, level, format, args, str + strlen(str),
			(int)(sizeof(log_context->str) - strlen(str)),
			&log_context->print_prefix);

	int obs_level;
	switch (level) {
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
		obs_level = LOG_ERROR;
		break;
	case AV_LOG_ERROR:
	case AV_LOG_WARNING:
		obs_level = LOG_WARNING;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		obs_level = LOG_INFO;
		break;
	case AV_LOG_DEBUG:
	default:
		obs_level = LOG_DEBUG;
	}

	if (!log_context->print_prefix)
		return;

	char *str_end = str + strlen(str) - 1;
	while(str < str_end) {
		if (*str_end != '\n')
			break;
		*str_end-- = '\0';
	}

	if (str_end <= str)
		goto cleanup;

	blog(obs_level, "[ffmpeg] %s", str);

cleanup:
	destroy_log_context(log_context);
}

#ifndef __APPLE__

static const char *nvenc_check_name = "nvenc_check";

#ifdef _WIN32
static const wchar_t *blacklisted_adapters[] = {
	L"720M",
	L"730M",
	L"740M",
	L"745M",
	L"820M",
	L"830M",
	L"840M",
	L"845M",
	L"920M",
	L"930M",
	L"940M",
	L"945M",
	L"1030",
	L"MX110",
	L"MX130",
	L"MX150",
	L"MX230",
	L"MX250",
	L"M520",
	L"M500",
	L"P500",
	L"K620M"
};

static const size_t num_blacklisted =
	sizeof(blacklisted_adapters) / sizeof(blacklisted_adapters[0]);

static bool is_adapter(const wchar_t *name, const wchar_t *adapter)
{
	const wchar_t *find = wstrstri(name, adapter);
	if (!find) {
		return false;
	}

	/* check before string for potential numeric mismatch */
	if (find > name && iswdigit(find[-1]) && iswdigit(find[0])) {
		return false;
	}

	/* check after string for potential numeric mismatch */
	size_t len = wcslen(adapter);
	if (iswdigit(find[len - 1]) && iswdigit(find[len])) {
		return false;
	}

	return true;
}

static bool is_blacklisted(const wchar_t *name)
{
	for (size_t i = 0; i < num_blacklisted; i++) {
		const wchar_t *blacklisted_adapter = blacklisted_adapters[i];
		if (is_adapter(name, blacklisted_adapter)) {
			return true;
		}
	}

	return false;
}

typedef HRESULT (WINAPI *create_dxgi_proc)(const IID *, IDXGIFactory1 **);

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

		if (wstrstri(desc.Description, L"nvidia") &&
		    !is_blacklisted(desc.Description)) {
			available = true;
			goto finish;
		}
	}

finish:
	factory->lpVtbl->Release(factory);
	return available;
}
#endif

#ifdef _WIN32
extern bool load_nvenc_lib(void);
#endif

static bool nvenc_supported(void)
{
	av_register_all();

	profile_start(nvenc_check_name);

	AVCodec *nvenc = avcodec_find_encoder_by_name("nvenc_h264");
	void *lib = NULL;
	bool success = false;

	if (!nvenc) {
		goto cleanup;
	}

#if defined(_WIN32)
	if (!nvenc_device_available()) {
		goto cleanup;
	}
	if (load_nvenc_lib()) {
		success = true;
		goto finish;
	}
#else
	lib = os_dlopen("libnvidia-encode.so.1");
#endif

	/* ------------------------------------------- */

	success = !!lib;

cleanup:
	if (lib)
		os_dlclose(lib);
#if defined(_WIN32)
finish:
#endif
	profile_end(nvenc_check_name);
	return success;
}

#endif

#ifdef LIBAVUTIL_VAAPI_AVAILABLE
static bool vaapi_supported(void)
{
	AVCodec *vaenc = avcodec_find_encoder_by_name("h264_vaapi");
	return !!vaenc;
}
#endif

#ifdef _WIN32
extern void jim_nvenc_load(void);
extern void jim_nvenc_unload(void);
#endif

bool obs_module_load(void)
{
	da_init(active_log_contexts);
	da_init(cached_log_contexts);

	//av_log_set_callback(ffmpeg_log_callback);

	obs_register_source(&ffmpeg_source);
	obs_register_output(&ffmpeg_output);
	obs_register_output(&ffmpeg_muxer);
	obs_register_output(&replay_buffer);
	obs_register_encoder(&aac_encoder_info);
	obs_register_encoder(&opus_encoder_info);
#ifndef __APPLE__
	if (nvenc_supported()) {
		blog(LOG_INFO, "NVENC supported");
#ifdef _WIN32
		if (get_win_ver_int() > 0x0601) {
			jim_nvenc_load();
		}
#endif
		obs_register_encoder(&nvenc_encoder_info);
	}
#if !defined(_WIN32) && defined(LIBAVUTIL_VAAPI_AVAILABLE)
	if (vaapi_supported()) {
		blog(LOG_INFO, "FFMPEG VAAPI supported");
		obs_register_encoder(&vaapi_encoder_info);
	}
#endif
#endif
	return true;
}

void obs_module_unload(void)
{
	av_log_set_callback(av_log_default_callback);

#ifdef _WIN32
	pthread_mutex_destroy(&log_contexts_mutex);
#endif

	for (size_t i = 0; i < active_log_contexts.num; i++) {
		bfree(active_log_contexts.array[i]);
	}
	for (size_t i = 0; i < cached_log_contexts.num; i++) {
		bfree(cached_log_contexts.array[i]);
	}

	da_free(active_log_contexts);
	da_free(cached_log_contexts);

#ifdef _WIN32
	jim_nvenc_unload();
#endif
}
