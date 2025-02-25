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
static HMODULE nv_cuda = NULL;

/** Effects @ref NvAFX_EffectSelector  */
#define NVAFX_EFFECT_DENOISER "denoiser"
#define NVAFX_EFFECT_DEREVERB "dereverb"
#define NVAFX_EFFECT_DEREVERB_DENOISER "dereverb_denoiser"
#define NVAFX_EFFECT_AEC "aec"
#define NVAFX_EFFECT_SUPERRES "superres"

/** Model paths */
#define NVAFX_EFFECT_DENOISER_MODEL "\\models\\denoiser_48k.trtpkg"
#define NVAFX_EFFECT_DEREVERB_MODEL "\\models\\dereverb_48k.trtpkg"
#define NVAFX_EFFECT_DEREVERB_DENOISER_MODEL "\\models\\dereverb_denoiser_48k.trtpkg"

#define NVAFX_CHAINED_EFFECT_DENOISER_16k_SUPERRES_16k_TO_48k "denoiser16k_superres16kto48k"
#define NVAFX_CHAINED_EFFECT_DEREVERB_16k_SUPERRES_16k_TO_48k "dereverb16k_superres16kto48k"
#define NVAFX_CHAINED_EFFECT_DEREVERB_DENOISER_16k_SUPERRES_16k_TO_48k "dereverb_denoiser16k_superres16kto48k"
#define NVAFX_CHAINED_EFFECT_SUPERRES_8k_TO_16k_DENOISER_16k "superres8kto16k_denoiser16k"
#define NVAFX_CHAINED_EFFECT_SUPERRES_8k_TO_16k_DEREVERB_16k "superres8kto16k_dereverb16k"
#define NVAFX_CHAINED_EFFECT_SUPERRES_8k_TO_16k_DEREVERB_DENOISER_16k "superres8kto16k_dereverb_denoiser16k"

/** Parameter selectors */

#define NVAFX_PARAM_NUM_STREAMS "num_streams"
#define NVAFX_PARAM_USE_DEFAULT_GPU "use_default_gpu"
#define NVAFX_PARAM_USER_CUDA_CONTEXT "user_cuda_context"
#define NVAFX_PARAM_DISABLE_CUDA_GRAPH "disable_cuda_graph"
#define NVAFX_PARAM_ENABLE_VAD "enable_vad"
/** Effect parameters. @ref NvAFX_ParameterSelector */
#define NVAFX_PARAM_MODEL_PATH "model_path"
#define NVAFX_PARAM_INPUT_SAMPLE_RATE "input_sample_rate"
#define NVAFX_PARAM_OUTPUT_SAMPLE_RATE "output_sample_rate"
#define NVAFX_PARAM_NUM_INPUT_SAMPLES_PER_FRAME "num_input_samples_per_frame"
#define NVAFX_PARAM_NUM_OUTPUT_SAMPLES_PER_FRAME "num_output_samples_per_frame"
#define NVAFX_PARAM_NUM_INPUT_CHANNELS "num_input_channels"
#define NVAFX_PARAM_NUM_OUTPUT_CHANNELS "num_output_channels"
#define NVAFX_PARAM_INTENSITY_RATIO "intensity_ratio"
#define NVAFX_PARAM_ENABLE_VAD "enable_vad"

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

	/** (32 bit SDK only) COM server was not registered, please see user manual for details */
	NVAFX_STATUS_32_SERVER_NOT_REGISTERED = 9,
	/** (32 bit SDK only) COM operation failed */
	NVAFX_STATUS_32_COM_ERROR = 10,
	/** GPU supported. The SDK requires Turing and above GPU with Tensor cores */
	NVAFX_STATUS_GPU_UNSUPPORTED = 11,
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

typedef NvAFX_Status NVAFX_API (*NvAFX_GetEffectList_t)(int *num_effects, NvAFX_EffectSelector *effects[]);
typedef NvAFX_Status NVAFX_API (*NvAFX_CreateEffect_t)(NvAFX_EffectSelector code, NvAFX_Handle *effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_CreateChainedEffect_t)(NvAFX_EffectSelector code, NvAFX_Handle *effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_DestroyEffect_t)(NvAFX_Handle effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetU32_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						 unsigned int val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetU32List_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						     unsigned int *val, unsigned int size);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetString_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						    const char *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetStringList_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
							const char **val, unsigned int size);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetFloat_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetFloatList_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						       float *val, unsigned int size);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetU32_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						 unsigned int *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetString_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, char *val,
						    int max_length);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetStringList_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
							char **val, int *max_length, unsigned int size);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetFloat_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetFloatList_t)(NvAFX_Handle effect, NvAFX_ParameterSelector param_name,
						       float *val, unsigned int size);
typedef NvAFX_Status NVAFX_API (*NvAFX_Load_t)(NvAFX_Handle effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetSupportedDevices_t)(NvAFX_Handle effect, int *num, int *devices);
typedef NvAFX_Status NVAFX_API (*NvAFX_Run_t)(NvAFX_Handle effect, const float **input, float **output,
					      unsigned num_samples, unsigned num_channels);
typedef NvAFX_Status NVAFX_API (*NvAFX_Reset_t)(NvAFX_Handle effect);

/* SDK >= 1.6.0 */
typedef NvAFX_Status NVAFX_API (*NvAFX_InitializeLogger_t)(LoggingSeverity level, LoggingTarget target,
							   const char *filename, logging_cb_t cb, void *userdata);
typedef NvAFX_Status NVAFX_API (*NvAFX_UninitializeLogger_t)();
/* cuda */
typedef enum cudaError_enum {
	CUDA_SUCCESS = 0,
	CUDA_ERROR_INVALID_VALUE = 1,
	CUDA_ERROR_OUT_OF_MEMORY = 2,
	CUDA_ERROR_NOT_INITIALIZED = 3,
	CUDA_ERROR_DEINITIALIZED = 4,
	CUDA_ERROR_PROFILER_DISABLED = 5,
	CUDA_ERROR_PROFILER_NOT_INITIALIZED = 6,
	CUDA_ERROR_PROFILER_ALREADY_STARTED = 7,
	CUDA_ERROR_PROFILER_ALREADY_STOPPED = 8,
	CUDA_ERROR_NO_DEVICE = 100,
	CUDA_ERROR_INVALID_DEVICE = 101,
	CUDA_ERROR_INVALID_IMAGE = 200,
	CUDA_ERROR_INVALID_CONTEXT = 201,
	CUDA_ERROR_CONTEXT_ALREADY_CURRENT = 202,
	CUDA_ERROR_MAP_FAILED = 205,
	CUDA_ERROR_UNMAP_FAILED = 206,
	CUDA_ERROR_ARRAY_IS_MAPPED = 207,
	CUDA_ERROR_ALREADY_MAPPED = 208,
	CUDA_ERROR_NO_BINARY_FOR_GPU = 209,
	CUDA_ERROR_ALREADY_ACQUIRED = 210,
	CUDA_ERROR_NOT_MAPPED = 211,
	CUDA_ERROR_NOT_MAPPED_AS_ARRAY = 212,
	CUDA_ERROR_NOT_MAPPED_AS_POINTER = 213,
	CUDA_ERROR_ECC_UNCORRECTABLE = 214,
	CUDA_ERROR_UNSUPPORTED_LIMIT = 215,
	CUDA_ERROR_CONTEXT_ALREADY_IN_USE = 216,
	CUDA_ERROR_INVALID_SOURCE = 300,
	CUDA_ERROR_FILE_NOT_FOUND = 301,
	CUDA_ERROR_SHARED_OBJECT_SYMBOL_NOT_FOUND = 302,
	CUDA_ERROR_SHARED_OBJECT_INIT_FAILED = 303,
	CUDA_ERROR_OPERATING_SYSTEM = 304,
	CUDA_ERROR_INVALID_HANDLE = 400,
	CUDA_ERROR_NOT_FOUND = 500,
	CUDA_ERROR_NOT_READY = 600,
	CUDA_ERROR_LAUNCH_FAILED = 700,
	CUDA_ERROR_LAUNCH_OUT_OF_RESOURCES = 701,
	CUDA_ERROR_LAUNCH_TIMEOUT = 702,
	CUDA_ERROR_LAUNCH_INCOMPATIBLE_TEXTURING = 703,
	CUDA_ERROR_PEER_ACCESS_ALREADY_ENABLED = 704,
	CUDA_ERROR_PEER_ACCESS_NOT_ENABLED = 705,
	CUDA_ERROR_PRIMARY_CONTEXT_ACTIVE = 708,
	CUDA_ERROR_CONTEXT_IS_DESTROYED = 709,
	CUDA_ERROR_ASSERT = 710,
	CUDA_ERROR_TOO_MANY_PEERS = 711,
	CUDA_ERROR_HOST_MEMORY_ALREADY_REGISTERED = 712,
	CUDA_ERROR_HOST_MEMORY_NOT_REGISTERED = 713,
	CUDA_ERROR_UNKNOWN = 999
} CUresult;
typedef struct CUctx_st *CUcontext;
typedef CUresult(__stdcall *cuCtxGetCurrent_t)(CUcontext *pctx);
typedef CUresult(__stdcall *cuCtxPopCurrent_t)(CUcontext *pctx);
typedef CUresult(__stdcall *cuInit_t)(unsigned int Flags);

static NvAFX_GetEffectList_t NvAFX_GetEffectList = NULL;
static NvAFX_CreateEffect_t NvAFX_CreateEffect = NULL;
static NvAFX_CreateChainedEffect_t NvAFX_CreateChainedEffect = NULL;
static NvAFX_DestroyEffect_t NvAFX_DestroyEffect = NULL;
static NvAFX_SetU32_t NvAFX_SetU32 = NULL;
static NvAFX_SetU32List_t NvAFX_SetU32List = NULL;
static NvAFX_SetString_t NvAFX_SetString = NULL;
static NvAFX_SetStringList_t NvAFX_SetStringList = NULL;
static NvAFX_SetFloat_t NvAFX_SetFloat = NULL;
static NvAFX_SetFloatList_t NvAFX_SetFloatList = NULL;
static NvAFX_GetU32_t NvAFX_GetU32 = NULL;
static NvAFX_GetString_t NvAFX_GetString = NULL;
static NvAFX_GetStringList_t NvAFX_GetStringList = NULL;
static NvAFX_GetFloat_t NvAFX_GetFloat = NULL;
static NvAFX_GetFloatList_t NvAFX_GetFloatList = NULL;
static NvAFX_Load_t NvAFX_Load = NULL;
static NvAFX_GetSupportedDevices_t NvAFX_GetSupportedDevices = NULL;
static NvAFX_Run_t NvAFX_Run = NULL;
static NvAFX_Reset_t NvAFX_Reset = NULL;
/* SDK >= 1.6.0 */
static NvAFX_InitializeLogger_t NvAFX_InitializeLogger = NULL;
static NvAFX_UninitializeLogger_t NvAFX_UninitializeLogger = NULL;
/* cuda */
static cuCtxGetCurrent_t cuCtxGetCurrent = NULL;
static cuCtxPopCurrent_t cuCtxPopCurrent = NULL;
static cuInit_t cuInit = NULL;

void release_lib(void)
{
	NvAFX_GetEffectList = NULL;
	NvAFX_CreateEffect = NULL;
	NvAFX_CreateChainedEffect = NULL;
	NvAFX_DestroyEffect = NULL;
	NvAFX_SetU32 = NULL;
	NvAFX_SetU32List = NULL;
	NvAFX_SetString = NULL;
	NvAFX_SetStringList = NULL;
	NvAFX_SetFloat = NULL;
	NvAFX_SetFloatList = NULL;
	NvAFX_GetU32 = NULL;
	NvAFX_GetString = NULL;
	NvAFX_GetStringList = NULL;
	NvAFX_GetFloat = NULL;
	NvAFX_GetFloatList = NULL;
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
	cuCtxGetCurrent = NULL;
	cuCtxPopCurrent = NULL;
	cuInit = NULL;
	if (nv_cuda) {
		FreeLibrary(nv_cuda);
		nv_cuda = NULL;
	}
}

static bool nvafx_get_sdk_path(char *buffer, const size_t len)
{
	DWORD ret = GetEnvironmentVariableA("NVAFX_SDK_DIR", buffer, (DWORD)len);

	if (!ret || ret >= len - 1)
		return false;

	return true;
}

static bool load_lib(void)
{
	char path[MAX_PATH];
	if (!nvafx_get_sdk_path(path, sizeof(path)))
		return false;

	SetDllDirectoryA(path);
	nv_audiofx = LoadLibrary(L"NVAudioEffects.dll");
	SetDllDirectoryA(NULL);
	nv_cuda = LoadLibrary(L"nvcuda.dll");
	return !!nv_audiofx && !!nv_cuda;
}

static unsigned int get_lib_version(void)
{
	static unsigned int version = 0;
	static bool version_checked = false;

	if (version_checked)
		return version;

	version_checked = true;

	char path[MAX_PATH];
	if (!nvafx_get_sdk_path(path, sizeof(path)))
		return 0;

	SetDllDirectoryA(path);

	struct win_version_info nto_ver = {0};
	if (get_dll_ver(L"NVAudioEffects.dll", &nto_ver))
		version = nto_ver.major << 24 | nto_ver.minor << 16 | nto_ver.build << 8 | nto_ver.revis << 0;

	SetDllDirectoryA(NULL);
	return version;
}

#endif
