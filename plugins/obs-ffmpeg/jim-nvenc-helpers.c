#include "jim-nvenc.h"
#include <util/platform.h>
#include <util/threading.h>
#include <util/config-file.h>
#include <util/windows/device-enum.h>
#include <util/dstr.h>
#include <util/pipe.h>

static void *nvenc_lib = NULL;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
NV_ENCODE_API_FUNCTION_LIST nv = {NV_ENCODE_API_FUNCTION_LIST_VER};
NV_CREATE_INSTANCE_FUNC nv_create_instance = NULL;

#define error(format, ...) blog(LOG_ERROR, "[jim-nvenc] " format, ##__VA_ARGS__)

bool nv_fail2(obs_encoder_t *encoder, void *session, const char *format, ...)
{
	struct dstr message = {0};
	struct dstr error_message = {0};

	va_list args;
	va_start(args, format);
	dstr_vprintf(&message, format, args);
	va_end(args);

	dstr_printf(&error_message, "NVENC Error: %s", message.array);
	obs_encoder_set_last_error(encoder, error_message.array);
	error("%s", error_message.array);

	dstr_free(&error_message);
	dstr_free(&message);

	return true;
}

bool nv_failed2(obs_encoder_t *encoder, void *session, NVENCSTATUS err,
		const char *func, const char *call)
{
	struct dstr error_message = {0};

	switch (err) {
	case NV_ENC_SUCCESS:
		return false;

	case NV_ENC_ERR_OUT_OF_MEMORY:
		obs_encoder_set_last_error(
			encoder, obs_module_text("NVENC.TooManySessions"));
		break;

	case NV_ENC_ERR_UNSUPPORTED_DEVICE:
		obs_encoder_set_last_error(
			encoder, obs_module_text("NVENC.UnsupportedDevice"));
		break;

	case NV_ENC_ERR_INVALID_VERSION:
		obs_encoder_set_last_error(
			encoder, obs_module_text("NVENC.OutdatedDriver"));
		break;

	default:
		dstr_printf(&error_message,
			    "NVENC Error: %s: %s failed: %d (%s)", func, call,
			    (int)err, nv_error_name(err));
		obs_encoder_set_last_error(encoder, error_message.array);
		dstr_free(&error_message);
		break;
	}

	if (session) {
		error("%s: %s failed: %d (%s): %s", func, call, (int)err,
		      nv_error_name(err), nv.nvEncGetLastErrorString(session));
	} else {
		error("%s: %s failed: %d (%s)", func, call, (int)err,
		      nv_error_name(err));
	}
	return true;
}

#define NV_FAILED(e, x) nv_failed2(e, NULL, x, __FUNCTION__, #x)

bool load_nvenc_lib(void)
{
	const char *const file = (sizeof(void *) == 8) ? "nvEncodeAPI64.dll"
						       : "nvEncodeAPI.dll";
	nvenc_lib = os_dlopen(file);
	return nvenc_lib != NULL;
}

static void *load_nv_func(const char *func)
{
	void *func_ptr = os_dlsym(nvenc_lib, func);
	if (!func_ptr) {
		error("Could not load function: %s", func);
	}
	return func_ptr;
}

typedef NVENCSTATUS(NVENCAPI *NV_MAX_VER_FUNC)(uint32_t *);

uint32_t get_nvenc_ver()
{
	static NV_MAX_VER_FUNC nv_max_ver = NULL;
	static bool failed = false;

	if (!nv_max_ver) {
		if (failed)
			return 0;

		nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func(
			"NvEncodeAPIGetMaxSupportedVersion");
		if (!nv_max_ver) {
			failed = true;
			return 0;
		}
	}

	uint32_t ver = 0;
	if (nv_max_ver(&ver) != NV_ENC_SUCCESS) {
		return 0;
	}
	return ver;
}

const char *nv_error_name(NVENCSTATUS err)
{
#define RETURN_CASE(x) \
	case x:        \
		return #x

	switch (err) {
		RETURN_CASE(NV_ENC_SUCCESS);
		RETURN_CASE(NV_ENC_ERR_NO_ENCODE_DEVICE);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_DEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_ENCODERDEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_DEVICE);
		RETURN_CASE(NV_ENC_ERR_DEVICE_NOT_EXIST);
		RETURN_CASE(NV_ENC_ERR_INVALID_PTR);
		RETURN_CASE(NV_ENC_ERR_INVALID_EVENT);
		RETURN_CASE(NV_ENC_ERR_INVALID_PARAM);
		RETURN_CASE(NV_ENC_ERR_INVALID_CALL);
		RETURN_CASE(NV_ENC_ERR_OUT_OF_MEMORY);
		RETURN_CASE(NV_ENC_ERR_ENCODER_NOT_INITIALIZED);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_PARAM);
		RETURN_CASE(NV_ENC_ERR_LOCK_BUSY);
		RETURN_CASE(NV_ENC_ERR_NOT_ENOUGH_BUFFER);
		RETURN_CASE(NV_ENC_ERR_INVALID_VERSION);
		RETURN_CASE(NV_ENC_ERR_MAP_FAILED);
		RETURN_CASE(NV_ENC_ERR_NEED_MORE_INPUT);
		RETURN_CASE(NV_ENC_ERR_ENCODER_BUSY);
		RETURN_CASE(NV_ENC_ERR_EVENT_NOT_REGISTERD);
		RETURN_CASE(NV_ENC_ERR_GENERIC);
		RETURN_CASE(NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY);
		RETURN_CASE(NV_ENC_ERR_UNIMPLEMENTED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_REGISTER_FAILED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_REGISTERED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_MAPPED);
	}
#undef RETURN_CASE

	return "Unknown Error";
}

static inline bool init_nvenc_internal(obs_encoder_t *encoder)
{
	static bool initialized = false;
	static bool success = false;

	if (initialized)
		return success;
	initialized = true;

	uint32_t ver = get_nvenc_ver();
	if (ver == 0) {
		obs_encoder_set_last_error(
			encoder,
			"Missing NvEncodeAPIGetMaxSupportedVersion, check "
			"your video card drivers are up to date.");
		return false;
	}

	uint32_t supported_ver = (NVENC_COMPAT_MAJOR_VER << 4) |
				 NVENC_COMPAT_MINOR_VER;
	if (supported_ver > ver) {
		obs_encoder_set_last_error(
			encoder, obs_module_text("NVENC.OutdatedDriver"));

		error("Current driver version does not support this NVENC "
		      "version, please upgrade your driver");
		return false;
	}

	nv_create_instance = (NV_CREATE_INSTANCE_FUNC)load_nv_func(
		"NvEncodeAPICreateInstance");
	if (!nv_create_instance) {
		obs_encoder_set_last_error(
			encoder, "Missing NvEncodeAPICreateInstance, check "
				 "your video card drivers are up to date.");
		return false;
	}

	if (NV_FAILED(encoder, nv_create_instance(&nv))) {
		return false;
	}

	success = true;
	return true;
}

bool init_nvenc(obs_encoder_t *encoder)
{
	bool success;

	pthread_mutex_lock(&init_mutex);
	success = init_nvenc_internal(encoder);
	pthread_mutex_unlock(&init_mutex);

	return success;
}

extern struct obs_encoder_info h264_nvenc_info;
#ifdef ENABLE_HEVC
extern struct obs_encoder_info hevc_nvenc_info;
#endif
extern struct obs_encoder_info av1_nvenc_info;

static bool enum_luids(void *param, uint32_t idx, uint64_t luid)
{
	struct dstr *cmd = param;
	dstr_catf(cmd, " %llX", luid);
	UNUSED_PARAMETER(idx);
	return true;
}

static bool av1_supported(void)
{
	char *test_exe = os_get_executable_path_ptr("obs-nvenc-test.exe");
	struct dstr cmd = {0};
	struct dstr caps_str = {0};
	bool av1_supported = false;
	config_t *config = NULL;

	dstr_copy(&cmd, test_exe);
	enum_graphics_device_luids(enum_luids, &cmd);

	os_process_pipe_t *pp = os_process_pipe_create(cmd.array, "r");
	if (!pp) {
		blog(LOG_WARNING, "[NVENC] Failed to launch the NVENC "
				  "test process I guess");
		goto fail;
	}

	for (;;) {
		char data[2048];
		size_t len =
			os_process_pipe_read(pp, (uint8_t *)data, sizeof(data));
		if (!len)
			break;

		dstr_ncat(&caps_str, data, len);
	}

	os_process_pipe_destroy(pp);

	if (dstr_is_empty(&caps_str)) {
		blog(LOG_WARNING,
		     "[NVENC] Seems the NVENC test subprocess crashed. "
		     "Better there than here I guess. Let's just "
		     "skip NVENC AV1 detection then I suppose.");
		goto fail;
	}

	if (config_open_string(&config, caps_str.array) != 0) {
		blog(LOG_WARNING, "[NVENC] Failed to open config string");
		goto fail;
	}

	const char *error = config_get_string(config, "error", "string");
	if (error) {
		blog(LOG_WARNING, "[NVENC] AV1 test process failed: %s", error);
		goto fail;
	}

	uint32_t adapter_count = (uint32_t)config_num_sections(config);
	bool avc_supported = false;
	bool hevc_supported = false;

	/* for now, just check AV1 support on device 0 */
	av1_supported = config_get_bool(config, "0", "supports_av1");

fail:
	if (config)
		config_close(config);
	dstr_free(&caps_str);
	dstr_free(&cmd);
	if (test_exe)
		bfree(test_exe);

	return av1_supported;
}

void jim_nvenc_load(bool h264, bool hevc, bool av1)
{
	pthread_mutex_init(&init_mutex, NULL);
	if (h264)
		obs_register_encoder(&h264_nvenc_info);
#ifdef ENABLE_HEVC
	if (hevc)
		obs_register_encoder(&hevc_nvenc_info);
#endif
	if (av1 && av1_supported())
		obs_register_encoder(&av1_nvenc_info);
	else
		blog(LOG_WARNING, "[NVENC] AV1 is not supported");
}

void jim_nvenc_unload(void)
{
	pthread_mutex_destroy(&init_mutex);
}
