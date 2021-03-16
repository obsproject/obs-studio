#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <util/platform.h>

#define NVAFX_API __declspec(dllexport)

#ifdef LIBNVAFX_ENABLED
static HMODULE nv_audiofx = NULL;

/** Denoiser Effect */
#define NVAFX_EFFECT_DENOISER "denoiser"

/** Parameter selectors */

/** Denoiser parameters. @ref NvAFX_ParameterSelector */
/** Denoiser filter model path (char*) */
#define NVAFX_PARAM_DENOISER_MODEL_PATH "denoiser_model_path"
/** Denoiser sample rate (unsigned int). Currently supported sample rate(s): 48000 */
#define NVAFX_PARAM_DENOISER_SAMPLE_RATE "sample_rate"
/** Denoiser number of samples per frame (unsigned int). This is immutable parameter */
#define NVAFX_PARAM_DENOISER_NUM_SAMPLES_PER_FRAME "num_samples_per_frame"
/** Denoiser number of channels in I/O (unsigned int). This is immutable parameter */
#define NVAFX_PARAM_DENOISER_NUM_CHANNELS "num_channels"
/** Denoiser noise suppression factor (float) */
#define NVAFX_PARAM_DENOISER_INTENSITY_RATIO "intensity_ratio"

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

typedef const char *NvAFX_EffectSelector;
typedef const char *NvAFX_ParameterSelector;
typedef void *NvAFX_Handle;

typedef NvAFX_Status
	NVAFX_API (*NvAFX_GetEffectList_t)(int *num_effects,
					   NvAFX_EffectSelector *effects[]);
typedef NvAFX_Status
	NVAFX_API (*NvAFX_CreateEffect_t)(NvAFX_EffectSelector code,
					  NvAFX_Handle *effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_DestroyEffect_t)(NvAFX_Handle effect);
typedef NvAFX_Status
	NVAFX_API (*NvAFX_SetU32_t)(NvAFX_Handle effect,
				    NvAFX_ParameterSelector param_name,
				    unsigned int val);
typedef NvAFX_Status
	NVAFX_API (*NvAFX_SetString_t)(NvAFX_Handle effect,
				       NvAFX_ParameterSelector param_name,
				       const char *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_SetFloat_t)(
	NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float val);
typedef NvAFX_Status
	NVAFX_API (*NvAFX_GetU32_t)(NvAFX_Handle effect,
				    NvAFX_ParameterSelector param_name,
				    unsigned int *val);
typedef NvAFX_Status
	NVAFX_API (*NvAFX_GetString_t)(NvAFX_Handle effect,
				       NvAFX_ParameterSelector param_name,
				       char *val, int max_length);
typedef NvAFX_Status NVAFX_API (*NvAFX_GetFloat_t)(
	NvAFX_Handle effect, NvAFX_ParameterSelector param_name, float *val);
typedef NvAFX_Status NVAFX_API (*NvAFX_Load_t)(NvAFX_Handle effect);
typedef NvAFX_Status NVAFX_API (*NvAFX_Run_t)(NvAFX_Handle effect,
					      const float **input,
					      float **output,
					      unsigned num_samples,
					      unsigned num_channels);

static NvAFX_GetEffectList_t NvAFX_GetEffectList = NULL;
static NvAFX_CreateEffect_t NvAFX_CreateEffect = NULL;
static NvAFX_DestroyEffect_t NvAFX_DestroyEffect = NULL;
static NvAFX_SetU32_t NvAFX_SetU32 = NULL;
static NvAFX_SetString_t NvAFX_SetString = NULL;
static NvAFX_SetFloat_t NvAFX_SetFloat = NULL;
static NvAFX_GetU32_t NvAFX_GetU32 = NULL;
static NvAFX_GetString_t NvAFX_GetString = NULL;
static NvAFX_GetFloat_t NvAFX_GetFloat = NULL;
static NvAFX_Load_t NvAFX_Load = NULL;
static NvAFX_Run_t NvAFX_Run = NULL;

void release_lib(void)
{
	NvAFX_GetEffectList = NULL;
	NvAFX_CreateEffect = NULL;
	NvAFX_DestroyEffect = NULL;
	NvAFX_SetU32 = NULL;
	NvAFX_SetString = NULL;
	NvAFX_SetFloat = NULL;
	NvAFX_GetU32 = NULL;
	NvAFX_GetString = NULL;
	NvAFX_GetFloat = NULL;
	NvAFX_Load = NULL;
	NvAFX_Run = NULL;
	if (nv_audiofx) {
		FreeLibrary(nv_audiofx);
		nv_audiofx = NULL;
	}
}

static bool nvafx_get_sdk_path(char *buffer, const size_t len)
{
	DWORD ret =
		GetEnvironmentVariableA("NVAFX_SDK_DIR", buffer, (DWORD)len);

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

	return !!nv_audiofx;
}
#endif
