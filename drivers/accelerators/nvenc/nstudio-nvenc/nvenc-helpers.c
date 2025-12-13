#include "obs-nvenc.h"
#include "nvenc-helpers.h"

#include <util/platform.h>
#include <util/threading.h>
#include <util/config-file.h>
#include <util/dstr.h>
#include <util/pipe.h>

static void *nvenc_lib = NULL;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
NV_ENCODE_API_FUNCTION_LIST nv = {NV_ENCODE_API_FUNCTION_LIST_VER};
NV_CREATE_INSTANCE_FUNC nv_create_instance = NULL;

/* Will be populated with results from obs-nvenc-test */
static struct encoder_caps encoder_capabilities[3];
static bool codec_supported[3];
static int num_devices;
static int driver_version_major;
static int driver_version_minor;

#define error(format, ...) blog(LOG_ERROR, "[obs-nvenc] " format, ##__VA_ARGS__)

bool nv_fail2(obs_encoder_t *encoder, void *session, const char *format, ...)
{
	UNUSED_PARAMETER(session);

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

bool nv_failed2(obs_encoder_t *encoder, void *session, NVENCSTATUS err, const char *func, const char *call)
{
	struct dstr error_message = {0};
	const char *nvenc_error = NULL;

	if (err == NV_ENC_SUCCESS)
		return false;

	if (session) {
		nvenc_error = nv.nvEncGetLastErrorString(session);
		if (nvenc_error) {
			// Some NVENC errors begin with :: which looks
			// odd to users. Strip it off.
			while (*nvenc_error == ':')
				nvenc_error++;
		}
	}

	switch (err) {
	case NV_ENC_ERR_OUT_OF_MEMORY:
	case NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY:
		obs_encoder_set_last_error(encoder, obs_module_text("TooManySessions"));
		break;

	case NV_ENC_ERR_NO_ENCODE_DEVICE:
	case NV_ENC_ERR_UNSUPPORTED_DEVICE:
		obs_encoder_set_last_error(encoder, obs_module_text("UnsupportedDevice"));
		break;

	case NV_ENC_ERR_INVALID_VERSION:
		obs_encoder_set_last_error(encoder, obs_module_text("OutdatedDriver"));
		break;

	default:
		if (nvenc_error && *nvenc_error) {
			dstr_printf(&error_message, "NVENC Error: %s (%s)", nvenc_error, nv_error_name(err));
		} else {

			dstr_printf(&error_message, "NVENC Error: %s: %s failed: %d (%s)", func, call, (int)err,
				    nv_error_name(err));
		}
		obs_encoder_set_last_error(encoder, error_message.array);
		dstr_free(&error_message);
		break;
	}

	if (nvenc_error && *nvenc_error) {
		error("%s: %s failed: %d (%s): %s", func, call, (int)err, nv_error_name(err), nvenc_error);
	} else {
		error("%s: %s failed: %d (%s)", func, call, (int)err, nv_error_name(err));
	}
	return true;
}

#define NV_FAILED(e, x) nv_failed2(e, NULL, x, __FUNCTION__, #x)

bool load_nvenc_lib(void)
{
#ifdef _WIN32
	nvenc_lib = os_dlopen("nvEncodeAPI64.dll");
#else
	nvenc_lib = os_dlopen("libnvidia-encode.so.1");
#endif
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

static uint32_t get_nvenc_ver(void)
{
	static NV_MAX_VER_FUNC nv_max_ver = NULL;
	static bool failed = false;
	static uint32_t ver = 0;

	if (!failed && ver)
		return ver;

	if (!nv_max_ver) {
		if (failed)
			return 0;

		nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func("NvEncodeAPIGetMaxSupportedVersion");
		if (!nv_max_ver) {
			failed = true;
			return 0;
		}
	}

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
		RETURN_CASE(NV_ENC_ERR_NEED_MORE_OUTPUT);
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
		obs_encoder_set_last_error(encoder, "Missing NvEncodeAPIGetMaxSupportedVersion, check "
						    "your video card drivers are up to date.");
		return false;
	}

	if (ver < NVCODEC_CONFIGURED_VERSION) {
		obs_encoder_set_last_error(encoder, obs_module_text("OutdatedDriver"));

		error("Current driver version does not support this NVENC "
		      "version, please upgrade your driver");
		return false;
	}

	nv_create_instance = (NV_CREATE_INSTANCE_FUNC)load_nv_func("NvEncodeAPICreateInstance");
	if (!nv_create_instance) {
		obs_encoder_set_last_error(encoder, "Missing NvEncodeAPICreateInstance, check "
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

struct encoder_caps *get_encoder_caps(enum codec_type codec)
{
	struct encoder_caps *caps = &encoder_capabilities[codec];
	return caps;
}

int num_encoder_devices(void)
{
	return num_devices;
}

bool is_codec_supported(enum codec_type codec)
{
	return codec_supported[codec];
}

bool has_broken_split_encoding(void)
{
	/* CBR padding and tearing artifacts with split encoding are fixed in
	 * driver versions 555+, previous ones should be considered broken. */
	return driver_version_major < 555;
}

static void read_codec_caps(config_t *config, enum codec_type codec, const char *section)
{
	struct encoder_caps *caps = &encoder_capabilities[codec];

	codec_supported[codec] = config_get_bool(config, section, "codec_supported");
	if (!codec_supported[codec])
		return;

	caps->bframes = (int)config_get_int(config, section, "bframes");
	caps->bref_modes = (int)config_get_int(config, section, "bref");
	caps->engines = (int)config_get_int(config, section, "engines");
	caps->max_width = (int)config_get_int(config, section, "max_width");
	caps->max_height = (int)config_get_int(config, section, "max_height");
	caps->temporal_filter = (int)config_get_int(config, section, "temporal_filter");
	caps->lookahead_level = (int)config_get_int(config, section, "lookahead_level");

	caps->dyn_bitrate = config_get_bool(config, section, "dynamic_bitrate");
	caps->lookahead = config_get_bool(config, section, "lookahead");
	caps->lossless = config_get_bool(config, section, "lossless");
	caps->temporal_aq = config_get_bool(config, section, "temporal_aq");
	caps->ten_bit = config_get_bool(config, section, "10bit");
	caps->four_four_four = config_get_bool(config, section, "yuv_444");
	caps->four_two_two = config_get_bool(config, section, "yuv_422");
	caps->uhq = config_get_bool(config, section, "uhq");
}

static bool nvenc_check(void)
{
#ifdef _WIN32
	char *test_exe = os_get_executable_path_ptr("obs-nvenc-test.exe");
#else
	char *test_exe = os_get_executable_path_ptr("obs-nvenc-test");
#endif
	os_process_args_t *args;
	struct dstr caps_str = {0};
	config_t *config = NULL;
	bool success = false;

	args = os_process_args_create(test_exe);

	os_process_pipe_t *pp = os_process_pipe_create2(args, "r");
	if (!pp) {
		blog(LOG_WARNING, "[NVENC] Failed to launch the NVENC "
				  "test process I guess");
		goto fail;
	}

	for (;;) {
		char data[2048];
		size_t len = os_process_pipe_read(pp, (uint8_t *)data, sizeof(data));
		if (!len)
			break;

		dstr_ncat(&caps_str, data, len);
	}

	os_process_pipe_destroy(pp);

	if (dstr_is_empty(&caps_str)) {
		blog(LOG_WARNING, "[NVENC] Seems the NVENC test subprocess crashed. "
				  "Better there than here I guess. ");
		goto fail;
	}

	if (config_open_string(&config, caps_str.array) != 0) {
		blog(LOG_WARNING, "[NVENC] Failed to open config string");
		goto fail;
	}

	success = config_get_bool(config, "general", "nvenc_supported");
	if (!success) {
		const char *error = config_get_string(config, "general", "reason");
		blog(LOG_WARNING, "[NVENC] Test process failed: %s", error ? error : "unknown");
		goto fail;
	}

	num_devices = (int)config_get_int(config, "general", "nvenc_devices");
	read_codec_caps(config, CODEC_H264, "h264");
	read_codec_caps(config, CODEC_HEVC, "hevc");
	read_codec_caps(config, CODEC_AV1, "av1");

	const char *nvenc_ver = config_get_string(config, "general", "nvenc_ver");
	const char *cuda_ver = config_get_string(config, "general", "cuda_ver");
	const char *driver_ver = config_get_string(config, "general", "driver_ver");
	/* Parse out major/minor for some brokenness checks  */
	sscanf(driver_ver, "%d.%d", &driver_version_major, &driver_version_minor);

	blog(LOG_INFO,
	     "[obs-nvenc] NVENC version: %d.%d (compiled) / %s (driver), "
	     "CUDA driver version: %s, AV1 supported: %s",
	     NVCODEC_CONFIGURED_VERSION >> 4, NVCODEC_CONFIGURED_VERSION & 0xf, nvenc_ver, cuda_ver,
	     codec_supported[CODEC_AV1] ? "true" : "false");

fail:
	if (config)
		config_close(config);

	bfree(test_exe);
	dstr_free(&caps_str);
	os_process_args_destroy(args);

	return success;
}

static const char *nvenc_check_name = "nvenc_check";
bool nvenc_supported(void)
{
	bool success;

	profile_start(nvenc_check_name);
	success = load_nvenc_lib() && nvenc_check();
	profile_end(nvenc_check_name);

	return success;
}

void obs_nvenc_load(void)
{
	pthread_mutex_init(&init_mutex, NULL);
	register_encoders();
	register_compat_encoders();
}

void obs_nvenc_unload(void)
{
	pthread_mutex_destroy(&init_mutex);
}
