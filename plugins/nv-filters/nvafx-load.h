#pragma once
#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <util/platform.h>
#include <util/windows/win-version.h>
#include "nv_sdk_versions.h"

#define NVAFX_API

#ifdef LIBNVAFX_ENABLED
static HMODULE nv_audiofx = NULL;
static HMODULE nv_denoiserfx = NULL;
static HMODULE nv_dereverbfx = NULL;
static HMODULE nv_dereverbdenoiserfx = NULL;

/** Effects @ref NvAFX_EffectSelector  */
#define NVAFX_EFFECT_DENOISER "denoiser"
#define NVAFX_EFFECT_DEREVERB "dereverb"
#define NVAFX_EFFECT_DEREVERB_DENOISER "dereverb_denoiser"

/** Model paths */
#define NVAFX_EFFECT_DENOISER_MODEL "\\models\\denoiser_48k.trtpkg"
#define NVAFX_EFFECT_DEREVERB_MODEL "\\models\\dereverb_48k.trtpkg"
#define NVAFX_EFFECT_DEREVERB_DENOISER_MODEL "\\models\\dereverb_denoiser_48k.trtpkg"

/** Parameter selectors */
#define NVAFX_PARAM_NUM_STREAMS "num_streams"
#define NVAFX_PARAM_USE_DEFAULT_GPU "use_default_gpu"
#define NVAFX_PARAM_USER_CUDA_CONTEXT "user_cuda_context"
#define NVAFX_PARAM_DISABLE_CUDA_GRAPH "disable_cuda_graph"

/** Effect parameters. @ref NvAFX_ParameterSelector */
#define NVAFX_PARAM_MODEL_PATH "model_path"
#define NVAFX_PARAM_INPUT_SAMPLE_RATE "input_sample_rate"
#define NVAFX_PARAM_OUTPUT_SAMPLE_RATE "output_sample_rate"

#define NVAFX_PARAM_NUM_INPUT_SAMPLES_PER_FRAME "num_input_samples_per_frame"
#define NVAFX_PARAM_NUM_OUTPUT_SAMPLES_PER_FRAME "num_output_samples_per_frame"
/* SDK >= 3.0.0.49: replaces the 2 previous define... */
#define NVAFX_PARAM_NUM_SAMPLES_PER_INPUT_FRAME "num_samples_per_input_frame"
#define NVAFX_PARAM_NUM_SAMPLES_PER_OUTPUT_FRAME "num_samples_per_output_frame"

#define NVAFX_PARAM_NUM_INPUT_CHANNELS "num_input_channels"
#define NVAFX_PARAM_NUM_OUTPUT_CHANNELS "num_output_channels"
#define NVAFX_PARAM_INTENSITY_RATIO "intensity_ratio"
#define NVAFX_PARAM_ENABLE_VAD "enable_vad"
/** SDK >= 3.0.0.49: Voice activity status (boolean) is an immutable parameter. */
#define NVAFX_PARAM_VAD_RESULT "vad_result"

#pragma deprecated(NVAFX_PARAM_DENOISER_MODEL_PATH)
#define NVAFX_PARAM_DENOISER_MODEL_PATH NVAFX_PARAM_MODEL_PATH
#pragma deprecated(NVAFX_PARAM_DENOISER_SAMPLE_RATE)
#define NVAFX_PARAM_DENOISER_SAMPLE_RATE NVAFX_PARAM_SAMPLE_RATE
#pragma deprecated(NVAFX_PARAM_DENOISER_NUM_SAMPLES_PER_FRAME)
#define NVAFX_PARAM_DENOISER_NUM_SAMPLES_PER_FRAME NVAFX_PARAM_NUM_SAMPLES_PER_FRAME
#pragma deprecated(NVAFX_PARAM_DENOISER_NUM_CHANNELS)
#define NVAFX_PARAM_DENOISER_NUM_CHANNELS NVAFX_PARAM_NUM_CHANNELS
#pragma deprecated(NVAFX_PARAM_DENOISER_INTENSITY_RATIO)
#define NVAFX_PARAM_DENOISER_INTENSITY_RATIO NVAFX_PARAM_INTENSITY_RATIO
/** Number of audio channels **/
#pragma deprecated(NVAFX_PARAM_NUM_CHANNELS)
#define NVAFX_PARAM_NUM_CHANNELS "num_channels"
/** Sample rate (unsigned int). Currently supported sample rate(s): 48000, 16000 */
#pragma deprecated(NVAFX_PARAM_SAMPLE_RATE)
#define NVAFX_PARAM_SAMPLE_RATE "sample_rate"
/** Number of samples per frame (unsigned int). This is immutable parameter */
#pragma deprecated(NVAFX_PARAM_NUM_SAMPLES_PER_FRAME)
#define NVAFX_PARAM_NUM_SAMPLES_PER_FRAME "num_samples_per_frame"

typedef enum {
	/** Success */
	NVAFX_STATUS_SUCCESS = 0,
	/** Failure */
	NVAFX_STATUS_FAILED = 1,
	/** Handle invalid */
	NVAFX_STATUS_INVALID_HANDLE = 2,
	/** Parameter value invalid */
	NVAFX_STATUS_INVALID_PARAM = 3,
	/** Parameter value immutable */
	NVAFX_STATUS_IMMUTABLE_PARAM = 4,
	/** Insufficient data to process */
	NVAFX_STATUS_INSUFFICIENT_DATA = 5,
	/** Effect not supported */
	NVAFX_STATUS_EFFECT_NOT_AVAILABLE = 6,
	/** Given buffer length too small to hold requested data */
	NVAFX_STATUS_OUTPUT_BUFFER_TOO_SMALL = 7,
	/** Model file could not be loaded */
	NVAFX_STATUS_MODEL_LOAD_FAILED = 8,

	/** Model is not loaded, it needs to be loaded for this operation */
	NVAFX_STATUS_MODEL_NOT_LOADED = 9,
	/** Selected model is incompatible */
	NVAFX_STATUS_INCOMPATIBLE_MODEL = 10,
	/** GPU supported. The SDK requires Turing and above GPU with Tensor cores */
	NVAFX_STATUS_GPU_UNSUPPORTED = 11,
	/** No supported GPU found on the system */
	NVAFX_STATUS_NO_SUPPORTED_GPU_FOUND = 12,
	/** Current GPU is not the one selected */
	NVAFX_STATUS_WRONG_GPU = 13,
	/** Cuda operation failure */
	NVAFX_STATUS_CUDA_ERROR = 14,
	/** Invalid operation performed **/
	NVAFX_STATUS_INVALID_OPERATION = 15,
	/** CUDA runtime is less than supported version*/
	NVAFX_UNSUPPORTED_RUNTIME = 16,
	/** (32 bit SDK only) COM server was not registered, please see user manual for details */
	NVAFX_STATUS_32_SERVER_NOT_REGISTERED = 17,
	/** (32 bit SDK only) COM operation failed */
	NVAFX_STATUS_32_COM_ERROR = 18,
	/** Cuda Context Failure Error */
	NVAFX_STATUS_CUDA_CONTEXT_CREATION_FAILED = 19,
	/** Dynamic Load Library Error */
	NVAFX_STATUS_LIBRARY_ERROR = 20,
	/** Dynamic Load Library out of memory error */
	NVAFX_STATUS_OUT_OF_MEMORY = 21,
} NvAFX_Status;

#define NVAFX_TRUE 1
#define NVAFX_FALSE 0
typedef char NvAFX_Bool;

/** Logging level to enable, each level is inclusive of the level preceding it */
typedef enum LoggingSeverity_t {
	NVAFX_LOG_LEVEL_NONE = -1,
	NVAFX_LOG_LEVEL_FATAL = 0,
	NVAFX_LOG_LEVEL_ERROR = 1,
	NVAFX_LOG_LEVEL_WARNING = 2,
	NVAFX_LOG_LEVEL_INFO = 3,
} LoggingSeverity;

typedef enum LoggingTarget_t {
	// No logging.
	LOG_TARGET_NONE = -1,
	// Log to stderr.
	LOG_TARGET_STDERR = 0,
	// Log to specified file.
	LOG_TARGET_FILE = 1,
	// Log through invocation of a user-specified callback.
	LOG_TARGET_CALLBACK = 2,
} LoggingTarget;

/** Function used for logging callback */
/// @param[in,out]  user_data   a pointer to data needed by the specific logger.
/// @param[in]      msg         a C-string to add to the log.
typedef void (*logging_cb_t)(void *user_data, const char *msg);

typedef const char *NvAFX_EffectSelector;
typedef const char *NvAFX_ParameterSelector;
typedef void *NvAFX_Handle;

typedef NvAFX_Status NVAFX_API (*NvAFX_CreateEffect_t)(NvAFX_EffectSelector code, NvAFX_Handle *effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_DestroyEffect_t)(NvAFX_Handle effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetU32_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						 unsigned int val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetString_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						    const char *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetFloat_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float val);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetU32_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						 unsigned int *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetString_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, char *val,
						    int max_length);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetFloat_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_Load_t)(NvAFX_Handle effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetSupportedDevices_t)(NvAFX_Handle effect, int *num, int *devices);
typedef NvAFX_Status NVAFX_API (*NvAFX_Run_t)(NvAFX_Handle effect, const float **input, float **output,
					      unsigned num_samples, unsigned num_channels);
typedef NvAFX_Status NVAFX_API (*NvAFX_Reset_t)(NvAFX_Handle effect);

/* SDK >= 1.6.0 */
typedef NvAFX_Status NVAFX_API (*NvAFX_InitializeLogger_t)(LoggingSeverity level, LoggingTarget target,
							   const char *filename, logging_cb_t cb, void *userdata);
typedef NvAFX_Status NVAFX_API (*NvAFX_UninitializeLogger_t)();

static NvAFX_CreateEffect_t NvAFX_CreateEffect = NULL;
static NvAFX_DestroyEffect_t NvAFX_DestroyEffect = NULL;
static NvAFX_SetU32_t NvAFX_SetU32 = NULL;
static NvAFX_SetString_t NvAFX_SetString = NULL;
static NvAFX_SetFloat_t NvAFX_SetFloat = NULL;
static NvAFX_GetU32_t NvAFX_GetU32 = NULL;
static NvAFX_GetString_t NvAFX_GetString = NULL;
static NvAFX_GetFloat_t NvAFX_GetFloat = NULL;
static NvAFX_Load_t NvAFX_Load = NULL;
static NvAFX_GetSupportedDevices_t NvAFX_GetSupportedDevices = NULL;
static NvAFX_Run_t NvAFX_Run = NULL;
static NvAFX_Reset_t NvAFX_Reset = NULL;
/* SDK >= 1.6.0 */
static NvAFX_InitializeLogger_t NvAFX_InitializeLogger = NULL;
static NvAFX_UninitializeLogger_t NvAFX_UninitializeLogger = NULL;

static inline void release_afx_lib(void)
{
	NvAFX_CreateEffect = NULL;
	NvAFX_DestroyEffect = NULL;
	NvAFX_SetU32 = NULL;
	NvAFX_SetString = NULL;
	NvAFX_SetFloat = NULL;
	NvAFX_GetU32 = NULL;
	NvAFX_GetString = NULL;
	NvAFX_GetFloat = NULL;
	NvAFX_Load = NULL;
	NvAFX_GetSupportedDevices = NULL;
	NvAFX_Run = NULL;
	NvAFX_Reset = NULL;
	/* SDK >= 1.6.0 */
	NvAFX_InitializeLogger = NULL;
	NvAFX_UninitializeLogger = NULL;

	if (nv_audiofx) {
		FreeLibrary(nv_audiofx);
		nv_audiofx = NULL;
	}
	if (nv_denoiserfx) {
		FreeLibrary(nv_denoiserfx);
		nv_denoiserfx = NULL;
	}
	if (nv_dereverbfx) {
		FreeLibrary(nv_dereverbfx);
		nv_dereverbfx = NULL;
	}
	if (nv_dereverbdenoiserfx) {
		FreeLibrary(nv_dereverbdenoiserfx);
		nv_dereverbdenoiserfx = NULL;
	}
}

static inline bool nvafx_get_sdk_path(char *buffer, const size_t len)
{
	DWORD ret = GetEnvironmentVariableA("NVAFX_SDK_DIR", buffer, (DWORD)len);

	if (!ret || ret >= len - 1) {
		ret = GetEnvironmentVariableA("AFX_SDK_DIR", buffer, (DWORD)len);
	}
	if (!ret || ret >= len - 1) {
		char path[MAX_PATH];
		if (!GetEnvironmentVariableA("ProgramFiles", path, MAX_PATH)) {
			buffer[0] = 0;
			return false;
		}

		if (_snprintf_s(buffer, len, _TRUNCATE, "%s\\NVIDIA Corporation\\NVIDIA Audio Effects SDK", path) > 0) {
			return true;
		}

		return false;
	}

	return true;
}

static unsigned int get_lib_version(void)
{
	static unsigned int version = 0;
	static bool version_checked = false;

	if (version_checked)
		return version;

	version_checked = true;

	char sdkPath[MAX_PATH];
	wchar_t dllPath[MAX_PATH];

	if (!nvafx_get_sdk_path(sdkPath, MAX_PATH)) {
		return version;
	}

	if (_snwprintf_s(dllPath, _countof(dllPath), _TRUNCATE, L"%S\\NVAudioEffects.dll", sdkPath) == -1) {
		return version;
	}

	struct win_version_info nto_ver = {0};
	if (get_dll_ver(dllPath, &nto_ver))
		version = nto_ver.major << 24 | nto_ver.minor << 16 | nto_ver.build << 8 | nto_ver.revis << 0;

	return version;
}

static inline bool load_afx_module(HMODULE *module, const char *sdk_path, const char *filename)
{
	char path[MAX_PATH];

	if (*module) {
		return true;
	}

	if (_snprintf_s(path, _countof(path), _TRUNCATE, "%s\\%s", sdk_path, filename) < 0) {
		return false;
	}

	*module = LoadLibraryExA(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	return *module != NULL;
}

static inline bool load_lib(unsigned int version)
{
	char sdk_path[MAX_PATH];

	if (!nvafx_get_sdk_path(sdk_path, _countof(sdk_path)))
		return false;

	if (!load_afx_module(&nv_audiofx, sdk_path, "NVAudioEffects.dll"))
		return false;

	if (version < MIN_ARM_AFX_SDK_VERSION)
		return true;

	if (!load_afx_module(&nv_denoiserfx, sdk_path, "nvafxdenoiser.dll") ||
	    !load_afx_module(&nv_dereverbfx, sdk_path, "nvafxdereverb.dll") ||
	    !load_afx_module(&nv_dereverbdenoiserfx, sdk_path, "nvafxdereverbdenoiser.dll")) {
		return false;
	}

	return true;
}

#endif
