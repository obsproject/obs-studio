#include <obs-module.h>
#include <util/darray.h>
#include <util/platform.h>
#include <libavutil/log.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <pthread.h>

#ifndef __APPLE__
#include "dynlink_cuda.h"
#include "nvEncodeAPI.h"
#endif

#define NVENC_CAP 0x30

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-ffmpeg", "en-US")

extern struct obs_source_info  ffmpeg_source;
extern struct obs_output_info  ffmpeg_output;
extern struct obs_output_info  ffmpeg_muxer;
extern struct obs_output_info  replay_buffer;
extern struct obs_encoder_info aac_encoder_info;
extern struct obs_encoder_info opus_encoder_info;
extern struct obs_encoder_info nvenc_encoder_info;

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

static inline bool push_context_(tcuCtxPushCurrent_v2 *cuCtxPushCurrent,
		CUcontext context)
{
	return cuCtxPushCurrent(context) == CUDA_SUCCESS;
}

static inline bool pop_context_(tcuCtxPopCurrent_v2 *cuCtxPopCurrent)
{
	CUcontext dummy;
	return cuCtxPopCurrent(&dummy) == CUDA_SUCCESS;
}

#define push_context(context) push_context_(cuCtxPushCurrent, context)
#define pop_context() pop_context_(cuCtxPopCurrent)

typedef NVENCSTATUS (NVENCAPI *NVENCODEAPICREATEINSTANCE)(
		NV_ENCODE_API_FUNCTION_LIST *functionList);

static bool nvenc_supported(void)
{
	av_register_all();

	profile_start(nvenc_check_name);

	AVCodec *nvenc = avcodec_find_encoder_by_name("nvenc_h264");
	void *lib = NULL;
	void *cudalib = NULL;
	bool success = false;

	if (!nvenc) {
		goto cleanup;
	}

#if defined(_WIN32)
	if (sizeof(void*) == 8) {
		lib = os_dlopen("nvEncodeAPI64.dll");
	} else {
		lib = os_dlopen("nvEncodeAPI.dll");
	}
	cudalib = os_dlopen("nvcuda.dll");
#else
	lib = os_dlopen("libnvidia-encode.so.1");
	cudalib = os_dlopen("libcuda.so.1");
#endif
	if (!lib || !cudalib) {
		goto cleanup;
	}

	/* ------------------------------------------- */

	CUdevice device;
	CUcontext context;
	CUresult cu_result;
	void *nvencoder = NULL;
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {0};
	NV_ENCODE_API_FUNCTION_LIST nv = {0};
	GUID *guids = NULL;
	int nv_result;
	int count;

#define GET_CUDA_FUNC(func) \
	t ## func *func = os_dlsym(cudalib, #func); \
	do { \
		if (!func) { \
			goto cleanup; \
		} \
	} while (false)

	GET_CUDA_FUNC(cuInit);
	GET_CUDA_FUNC(cuDeviceGet);
	GET_CUDA_FUNC(cuDeviceComputeCapability);

#undef GET_CUDA_FUNC

#define GET_CUDA_V2_FUNC(func) \
	t ## func ## _v2 *func = os_dlsym(cudalib, #func "_v2"); \
	do { \
		if (!func) { \
			goto cleanup; \
		} \
	} while (false)

	GET_CUDA_V2_FUNC(cuCtxCreate);
	GET_CUDA_V2_FUNC(cuCtxDestroy);
	GET_CUDA_V2_FUNC(cuCtxPushCurrent);
	GET_CUDA_V2_FUNC(cuCtxPopCurrent);

#undef GET_CUDA_V2_FUNC

	NVENCODEAPICREATEINSTANCE create_instance = os_dlsym(lib,
			"NvEncodeAPICreateInstance");
	if (!create_instance) {
		goto cleanup;
	}

	nv.version = NV_ENCODE_API_FUNCTION_LIST_VER;
	nv_result = create_instance(&nv);
	if (nv_result != NV_ENC_SUCCESS) {
		goto cleanup;
	}

	cu_result = cuInit(0);
	if (cu_result != CUDA_SUCCESS) {
		goto cleanup;
	}

	cu_result = cuDeviceGet(&device, 0);
	if (cu_result != CUDA_SUCCESS) {
		goto cleanup;
	}

	int major, minor;
	cu_result = cuDeviceComputeCapability(&major, &minor, device);
	if (cu_result != CUDA_SUCCESS) {
		goto cleanup;
	}

	if (((major << 4) | minor) < NVENC_CAP) {
		goto cleanup;
	}

	cu_result = cuCtxCreate(&context, 0, device);
	if (cu_result != CUDA_SUCCESS) {
		goto cleanup;
	}

	if (!pop_context()) {
		goto cleanup2;
	}

	params.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
	params.apiVersion = NVENCAPI_VERSION;
	params.device = context;
	params.deviceType = NV_ENC_DEVICE_TYPE_CUDA;

	nv_result = nv.nvEncOpenEncodeSessionEx(&params, &nvencoder);
	if (nv_result != NV_ENC_SUCCESS) {
		nvencoder = NULL;
		goto cleanup2;
	}

	nv_result = nv.nvEncGetEncodeGUIDCount(nvencoder, &count);
	if (nv_result != NV_ENC_SUCCESS || !count) {
		goto cleanup3;
	}

	guids = bzalloc(count * sizeof(GUID));

	nv_result = nv.nvEncGetEncodeGUIDs(nvencoder, guids, count, &count);
	if (nv_result != NV_ENC_SUCCESS || !count) {
		goto cleanup3;
	}

	for (int i = 0; i < count; i++) {
		int ret = memcmp(&guids[i], &NV_ENC_CODEC_H264_GUID,
				sizeof(*guids));
		if (ret == 0) {
			success = true;
			break;
		}
	}

cleanup3:
	bfree(guids);

	if (nvencoder && push_context(context)) {
		nv.nvEncDestroyEncoder(nvencoder);
		pop_context();
	}

cleanup2:
	cuCtxDestroy(context);

cleanup:
	os_dlclose(lib);
	os_dlclose(cudalib);
	profile_end(nvenc_check_name);
	return success;
}

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
		obs_register_encoder(&nvenc_encoder_info);
	}
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
}
