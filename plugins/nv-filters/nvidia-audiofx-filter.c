#include <stdint.h>
#include <inttypes.h>
#include <obs-module.h>
#include <util/deque.h>
#include <util/threading.h>
#include <media-io/audio-resampler.h>
#include "nvafx-load.h"
#include <pthread.h>

/* -------------------------------------------------------- */
#define do_log(level, format, ...) \
	blog(level, "[NVIDIA Audio Effects: '%s'] " format, obs_source_get_name(ng->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif
/* -------------------------------------------------------- */

#define S_NVAFX_INTENSITY "intensity"
#define S_METHOD "method"
#define S_METHOD_NVAFX_DENOISER NVAFX_EFFECT_DENOISER
#define S_METHOD_NVAFX_DEREVERB NVAFX_EFFECT_DEREVERB
#define S_METHOD_NVAFX_DEREVERB_DENOISER NVAFX_EFFECT_DEREVERB_DENOISER
#define S_NVAFX_VAD "vad"

#define MT_ obs_module_text
#define TEXT_NVAFX_INTENSITY MT_("Nvafx.Intensity")
#define TEXT_METHOD MT_("Nvafx.Method")
#define TEXT_METHOD_NVAFX_DENOISER MT_("Nvafx.Method.Denoiser")
#define TEXT_METHOD_NVAFX_DEREVERB MT_("Nvafx.Method.Dereverb")
#define TEXT_METHOD_NVAFX_DEREVERB_DENOISER MT_("Nvafx.Method.DenoiserPlusDereverb")
#define TEXT_METHOD_NVAFX_DEPRECATION MT_("Nvafx.OutdatedSDK")
#define TEXT_NVAFX_VAD MT_("Nvafx.VAD")

#define MAX_PREPROC_CHANNELS 8
#define BUFFER_SIZE_MSEC 10

/* NVAFX constants, these can't be changed */
#define NVAFX_SAMPLE_RATE 48000
/* The SDK does not explicitly set this as a constant though it relies on it.*/
#define NVAFX_FRAME_SIZE 480

#ifdef _MSC_VER
#define ssize_t intptr_t
#endif

bool nvafx_new_sdk = false;

struct nvidia_audio_data {
	obs_source_t *context;

	uint64_t last_timestamp;
	uint64_t latency;

	size_t frames;
	size_t channels;

	struct deque info_buffer;
	struct deque input_buffers[MAX_PREPROC_CHANNELS];
	struct deque output_buffers[MAX_PREPROC_CHANNELS];

	/* This bool is quite important but it is easy to get lost. So let's
	 * explain how it's used. One big issue is that the NVIDIA FX takes
	 * ages to load an FX; so its initialization is deferred to a separate
	 * thread.
	 * First stage (creation):
	 * - use_nvafx is set to true at creation of the filter, IF the SDK dir
	 * path is set.
	 * - if initialization fails, the bool is set to false & the filter is
	 * destroyed.
	 * Later stages (running or updating of the FX):
	 * - they are executed ONLY if initialization was successful;
	 * - if at any step, there's an FX failure, the bool is updated to false
	 * & the filter is destroyed.
	 */
	bool use_nvafx;

	/* this tracks if the SDK is found */
	bool nvidia_sdk_dir_found;

	bool has_mono_src;
	volatile bool reinit_done;

	/* NVAFX handle, one per audio channel */
	NvAFX_Handle handle[MAX_PREPROC_CHANNELS];

	uint32_t sample_rate;
	float intensity_ratio;
	unsigned int num_samples_per_frame, num_channels;
	char *model;
	bool nvafx_initialized;
	const char *fx;
	char *sdk_path;

	/* Resampler */
	audio_resampler_t *nvafx_resampler;
	audio_resampler_t *nvafx_resampler_back;

	/* We load the DLL in a separate thread because its loading is very 
	 * long and unnecessarily blocks OBS initial loading.
	 * This bool is true once the thread which side loads the FX DLL is started */
	bool nvafx_loading;
	pthread_t nvafx_thread;
	pthread_mutex_t nvafx_mutex;

	/* PCM buffers */
	float *copy_buffers[MAX_PREPROC_CHANNELS];
	float *nvafx_segment_buffers[MAX_PREPROC_CHANNELS];

	/* output data */
	struct obs_audio_data output_audio;
	DARRAY(float) output_data;

	/* Optimization for Voice Audio Data (VAD) ; requires SDK >= 0.7.6 */
	bool vad;
};

static const char *nvidia_audio_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Nvafx");
}

static void nvidia_audio_destroy(void *data)
{
	struct nvidia_audio_data *ng = data;

	if (!ng)
		return;

	if (ng->nvidia_sdk_dir_found)
		pthread_mutex_lock(&ng->nvafx_mutex);

	if (nvafx_new_sdk)
		NvAFX_UninitializeLogger();

	for (size_t i = 0; i < ng->channels; i++) {
		if (ng->handle[0]) {
			if (NvAFX_DestroyEffect) {
				NvAFX_Status err = NvAFX_DestroyEffect(ng->handle[i]);
				if (err != NVAFX_STATUS_SUCCESS) {
					do_log(LOG_ERROR, "NvAFX_Release() failed");
				}
			}
		}
		deque_free(&ng->input_buffers[i]);
		deque_free(&ng->output_buffers[i]);
	}

	bfree(ng->nvafx_segment_buffers[0]);

	if (ng->nvafx_resampler) {
		audio_resampler_destroy(ng->nvafx_resampler);
		audio_resampler_destroy(ng->nvafx_resampler_back);
	}
	bfree(ng->model);
	bfree(ng->sdk_path);
	bfree((void *)ng->fx);
	if (ng->nvidia_sdk_dir_found) {
		pthread_join(ng->nvafx_thread, NULL);
		pthread_mutex_unlock(&ng->nvafx_mutex);
		pthread_mutex_destroy(&ng->nvafx_mutex);
	}

	bfree(ng->copy_buffers[0]);
	deque_free(&ng->info_buffer);
	da_free(ng->output_data);
	bfree(ng);
}

bool nvidia_afx_initializer_mutex_initialized;
pthread_mutex_t nvidia_afx_initializer_mutex;
bool nvidia_afx_loaded = false;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4706)
#endif
void release_afxlib(void)
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

bool load_nvidia_afx(void)
{
	unsigned int version = get_lib_version();
	uint8_t major = (version >> 24) & 0xff;
	uint8_t minor = (version >> 16) & 0x00ff;
	uint8_t build = (version >> 8) & 0x0000ff;
	uint8_t revision = (version >> 0) & 0x000000ff;
	if (version) {
		blog(LOG_INFO, "[NVIDIA Audio Effects:] version: %i.%i.%i.%i", major, minor, build, revision);
		if (version < MIN_AFX_SDK_VERSION) {
			blog(LOG_INFO,
			     "[NVIDIA Audio Effects:]: SDK is outdated. Please update both audio & video SDK.\nRequired SDK versions, audio: %i.%i.%i; video: %i.%i.%i",
			     (MIN_AFX_SDK_VERSION >> 24) & 0xff, (MIN_AFX_SDK_VERSION >> 16) & 0x00ff,
			     (MIN_AFX_SDK_VERSION >> 8) & 0x0000ff, (MIN_VFX_SDK_VERSION >> 24) & 0xff,
			     (MIN_VFX_SDK_VERSION >> 16) & 0x00ff, (MIN_VFX_SDK_VERSION >> 8) & 0x0000ff);
		}
	}
	if (!load_lib()) {
		blog(LOG_INFO,
		     "[NVIDIA Audio Effects:] NVIDIA denoiser disabled, redistributable not found or could not be loaded.");
		return false;
	}

	nvidia_afx_initializer_mutex_initialized = pthread_mutex_init(&nvidia_afx_initializer_mutex, NULL) == 0;

#define LOAD_SYM_FROM_LIB(sym, lib, dll)                                                                       \
	if (!(sym = (sym##_t)GetProcAddress(lib, #sym))) {                                                     \
		DWORD err = GetLastError();                                                                    \
		printf("[NVIDIA Audio Effects:]: Couldn't load " #sym " from " dll ": %lu (0x%lx)", err, err); \
		goto unload_everything;                                                                        \
	}

#define LOAD_SYM_FROM_LIB2(sym, lib, dll)                                                                      \
	if (!(sym = (sym##_t)GetProcAddress(lib, #sym))) {                                                     \
		DWORD err = GetLastError();                                                                    \
		printf("[NVIDIA Audio Effects:]: Couldn't load " #sym " from " dll ": %lu (0x%lx)", err, err); \
		nvafx_new_sdk = false;                                                                         \
	} else {                                                                                               \
		nvafx_new_sdk = true;                                                                          \
	}

#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB(sym, nv_audiofx, "NVAudioEffects.dll")
	LOAD_SYM(NvAFX_GetEffectList);
	LOAD_SYM(NvAFX_CreateEffect);
	LOAD_SYM(NvAFX_CreateChainedEffect);
	LOAD_SYM(NvAFX_DestroyEffect);
	LOAD_SYM(NvAFX_SetU32);
	LOAD_SYM(NvAFX_SetU32List);
	LOAD_SYM(NvAFX_SetString);
	LOAD_SYM(NvAFX_SetStringList);
	LOAD_SYM(NvAFX_SetFloat);
	LOAD_SYM(NvAFX_SetFloatList);
	LOAD_SYM(NvAFX_GetU32);
	LOAD_SYM(NvAFX_GetString);
	LOAD_SYM(NvAFX_GetStringList);
	LOAD_SYM(NvAFX_GetFloat);
	LOAD_SYM(NvAFX_GetFloatList);
	LOAD_SYM(NvAFX_Load);
	LOAD_SYM(NvAFX_GetSupportedDevices);
	LOAD_SYM(NvAFX_Run);
	LOAD_SYM(NvAFX_Reset);
#undef LOAD_SYM
#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB(sym, nv_cuda, "nvcuda.dll")
	LOAD_SYM(cuCtxGetCurrent);
	LOAD_SYM(cuCtxPopCurrent);
	LOAD_SYM(cuInit);
#undef LOAD_SYM
#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB2(sym, nv_audiofx, "NVAudioEffects.dll")
	LOAD_SYM(NvAFX_InitializeLogger);
	bool new_sdk = nvafx_new_sdk;
	LOAD_SYM(NvAFX_UninitializeLogger);
	if (!nvafx_new_sdk || !new_sdk) {
		blog(LOG_INFO, "[NVIDIA AUDIO FX]: SDK loaded but old redistributable detected. Please upgrade.");
		nvafx_new_sdk = false;
	}
#undef LOAD_SYM
	NvAFX_Status err;
	CUresult cudaerr;

	NvAFX_Handle h = NULL;

	cudaerr = cuInit(0);
	if (cudaerr != CUDA_SUCCESS) {
		goto cuda_errors;
	}
	CUcontext old = {0};
	CUcontext curr = {0};
	cudaerr = cuCtxGetCurrent(&old);
	if (cudaerr != CUDA_SUCCESS) {
		goto cuda_errors;
	}

	err = NvAFX_CreateEffect(NVAFX_EFFECT_DENOISER, &h);
	cudaerr = cuCtxGetCurrent(&curr);
	if (cudaerr != CUDA_SUCCESS) {
		goto cuda_errors;
	}

	if (curr != old) {
		cudaerr = cuCtxPopCurrent(NULL);
		if (cudaerr != CUDA_SUCCESS)
			goto cuda_errors;
	}

	if (err != NVAFX_STATUS_SUCCESS) {
		if (err == NVAFX_STATUS_GPU_UNSUPPORTED) {
			blog(LOG_INFO, "[NVIDIA Audio Effects:] disabled: unsupported GPU");
		} else {
			blog(LOG_ERROR, "[NVIDIA Audio Effects:] disabled, error %i", err);
		}
		goto unload_everything;
	}

	err = NvAFX_DestroyEffect(h);
	if (err != NVAFX_STATUS_SUCCESS) {
		blog(LOG_ERROR, "[NVIDIA Audio Effects:]: disabled, error %i", err);
		goto unload_everything;
	}

	nvidia_afx_loaded = true;
	blog(LOG_INFO, "[NVIDIA Audio Effects:] enabled");
	return true;

cuda_errors:
	blog(LOG_ERROR, "[NVIDIA Audio Effects:] disabled, CUDA error %i", cudaerr);
unload_everything:
	release_afxlib();

	return false;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

void unload_nvidia_afx(void)
{
	release_afxlib();

	if (nvidia_afx_initializer_mutex_initialized) {
		pthread_mutex_destroy(&nvidia_afx_initializer_mutex);
		nvidia_afx_initializer_mutex_initialized = false;
	}
}

static bool nvidia_audio_initialize_internal(void *data)
{
	struct nvidia_audio_data *ng = data;
	NvAFX_Status err;

	if (!ng->handle[0]) {
		ng->sample_rate = NVAFX_SAMPLE_RATE;
		for (size_t i = 0; i < ng->channels; i++) {
			// Create FX
			CUcontext old = {0};
			CUcontext curr = {0};
			if (cuCtxGetCurrent(&old) != CUDA_SUCCESS) {
				goto failure;
			}
			err = NvAFX_CreateEffect(ng->fx, &ng->handle[i]);
			if (err != NVAFX_STATUS_SUCCESS) {
				do_log(LOG_ERROR, "%s FX creation failed, error %i", ng->fx, err);
				goto failure;
			}
			if (cuCtxGetCurrent(&curr) != CUDA_SUCCESS) {
				goto failure;
			}
			if (curr != old) {
				cuCtxPopCurrent(NULL);
			}
			// Set sample rate of FX
			err = NvAFX_SetU32(ng->handle[i], NVAFX_PARAM_INPUT_SAMPLE_RATE, ng->sample_rate);
			if (err != NVAFX_STATUS_SUCCESS) {
				do_log(LOG_ERROR, "NvAFX_SetU32(Sample Rate: %u) failed, error %i", ng->sample_rate,
				       err);
				goto failure;
			}

			// Set intensity of FX
			err = NvAFX_SetFloat(ng->handle[i], NVAFX_PARAM_INTENSITY_RATIO, ng->intensity_ratio);
			if (err != NVAFX_STATUS_SUCCESS) {
				do_log(LOG_ERROR, "NvAFX_SetFloat(Intensity Ratio: %f) failed, error %i",
				       ng->intensity_ratio, err);
				goto failure;
			}

			// Set VAD (Voice Audio Data)
			if (nvafx_new_sdk) {
				err = NvAFX_SetU32(ng->handle[i], NVAFX_PARAM_ENABLE_VAD, ng->vad);
				if (err != NVAFX_STATUS_SUCCESS) {
					do_log(LOG_ERROR, "NvAFX_SetU32(VAD: %i) failed, error %i", ng->vad, err);
					goto failure;
				}
			}

			// Set AI models path
			err = NvAFX_SetString(ng->handle[i], NVAFX_PARAM_MODEL_PATH, ng->model);
			if (err != NVAFX_STATUS_SUCCESS) {
				do_log(LOG_ERROR, "NvAFX_SetString() failed, error %i", err);
				goto failure;
			}

			// Load FX (this is a very long step, about 2 seconds)
			err = NvAFX_Load(ng->handle[i]);
			if (err != NVAFX_STATUS_SUCCESS) {
				do_log(LOG_ERROR, "NvAFX_Load() failed with error %i", err);
				goto failure;
			}
			os_atomic_set_bool(&ng->reinit_done, true);
		}
	}
	return true;

failure:
	ng->use_nvafx = false;
	return false;
}

static void *nvidia_audio_disable(void *data)
{
	struct nvidia_audio_data *ng = data;
	obs_source_t *filter = ng->context;
	obs_source_set_enabled(filter, false);
	info("NVIDIA Audio FX disabled due to an internal error.");
	return NULL;
}

static void *nvidia_audio_initialize(void *data)
{
	struct nvidia_audio_data *ng = data;
	NvAFX_Status err;

	if (!ng->use_nvafx && !nvidia_afx_loaded) {
		return NULL;
	}
	pthread_mutex_lock(&ng->nvafx_mutex);
	pthread_mutex_lock(&nvidia_afx_initializer_mutex);
	if (!nvidia_audio_initialize_internal(data)) {
		goto failure;
	}
	if (ng->use_nvafx) {
		err = NvAFX_GetU32(ng->handle[0], NVAFX_PARAM_NUM_INPUT_CHANNELS, &ng->num_channels);
		if (err != NVAFX_STATUS_SUCCESS) {
			do_log(LOG_ERROR, "NvAFX_GetU32() failed to get the number of channels, error %i", err);
			goto failure;
		}
		if (ng->num_channels != 1) {
			do_log(LOG_ERROR, "The number of channels is not 1 in the sdk any more ==> update code");
			goto failure;
		}
		NvAFX_Status err = NvAFX_GetU32(ng->handle[0], NVAFX_PARAM_NUM_INPUT_SAMPLES_PER_FRAME,
						&ng->num_samples_per_frame);
		if (err != NVAFX_STATUS_SUCCESS) {
			do_log(LOG_ERROR, "NvAFX_GetU32() failed to get the number of samples per frame, error %i",
			       err);
			goto failure;
		}
		if (ng->num_samples_per_frame != NVAFX_FRAME_SIZE) {
			do_log(LOG_ERROR,
			       "The number of samples per frame has changed from 480 (= 10 ms) ==> update code");
			goto failure;
		}
	}
	ng->nvafx_initialized = true;
	ng->nvafx_loading = false;
	pthread_mutex_unlock(&nvidia_afx_initializer_mutex);
	pthread_mutex_unlock(&ng->nvafx_mutex);
	return NULL;

failure:
	ng->use_nvafx = false;
	pthread_mutex_unlock(&nvidia_afx_initializer_mutex);
	pthread_mutex_unlock(&ng->nvafx_mutex);
	nvidia_audio_disable(ng);
	return NULL;
}

static inline enum speaker_layout nv_convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static void set_nv_model(void *data, const char *method)
{
	struct nvidia_audio_data *ng = data;
	const char *file;

	if (strcmp(NVAFX_EFFECT_DEREVERB, method) == 0)
		file = NVAFX_EFFECT_DEREVERB_MODEL;
	else if (strcmp(NVAFX_EFFECT_DEREVERB_DENOISER, method) == 0)
		file = NVAFX_EFFECT_DEREVERB_DENOISER_MODEL;
	else
		file = NVAFX_EFFECT_DENOISER_MODEL;

	size_t size = strlen(ng->sdk_path) + strlen(file) + 1;
	char *buffer = (char *)bmalloc(size);

	strcpy(buffer, ng->sdk_path);
	strcat(buffer, file);
	ng->model = buffer;
}

static void nvidia_audio_update(void *data, obs_data_t *s)
{
	struct nvidia_audio_data *ng = data;

	if (!ng->use_nvafx)
		return;

	const char *method = obs_data_get_string(s, S_METHOD);
	ng->latency = 1000000000LL / (1000 / BUFFER_SIZE_MSEC);

	float intensity = (float)obs_data_get_double(s, S_NVAFX_INTENSITY);
	bool vad = obs_data_get_bool(s, S_NVAFX_VAD);

	/*-------------------------------------------------------------------*/
	/* STAGE 1 : the following is run only when the filter is created. */

	/* If the DLL hasn't been loaded & isn't loading, start the side loading. */
	if (!ng->nvafx_initialized && !ng->nvafx_loading) {
		ng->intensity_ratio = intensity;
		ng->vad = vad;
		ng->nvafx_loading = true;
		pthread_create(&ng->nvafx_thread, NULL, nvidia_audio_initialize, ng);
	}

	/*-------------------------------------------------------------------*/
	/* STAGE 2 : this is executed only after the FX has been initialized */
	if (ng->nvafx_initialized) {
		/* updating the intensity of the FX */
		if (intensity != ng->intensity_ratio && (strcmp(ng->fx, method) == 0)) {
			NvAFX_Status err;
			ng->intensity_ratio = intensity;
			pthread_mutex_lock(&ng->nvafx_mutex);
			for (size_t i = 0; i < ng->channels; i++) {
				err = NvAFX_SetFloat(ng->handle[i], NVAFX_PARAM_INTENSITY_RATIO, ng->intensity_ratio);
				if (err != NVAFX_STATUS_SUCCESS) {
					do_log(LOG_ERROR, "NvAFX_SetFloat(Intensity Ratio: %f) failed, error %i",
					       ng->intensity_ratio, err);
					nvidia_audio_disable(ng);
				}
			}
			pthread_mutex_unlock(&ng->nvafx_mutex);
		}
		/* updating for VAD toggled on or off */
		if (nvafx_new_sdk) {
			if (vad != ng->vad && (strcmp(ng->fx, method) == 0)) {
				NvAFX_Status err;
				ng->vad = vad;
				pthread_mutex_lock(&ng->nvafx_mutex);
				for (size_t i = 0; i < ng->channels; i++) {
					err = NvAFX_SetU32(ng->handle[i], NVAFX_PARAM_ENABLE_VAD, ng->vad);
					if (err != NVAFX_STATUS_SUCCESS) {
						do_log(LOG_ERROR, "NvAFX_SetU32(VAD: %i) failed, error %i", ng->vad,
						       err);
						nvidia_audio_disable(ng);
					}
				}
				pthread_mutex_unlock(&ng->nvafx_mutex);
			}
		}
		/* swapping to a new FX requires a reinitialization */
		if ((strcmp(ng->fx, method) != 0)) {
			pthread_mutex_lock(&ng->nvafx_mutex);
			bfree((void *)ng->fx);
			ng->fx = bstrdup(method);
			ng->intensity_ratio = intensity;
			ng->vad = vad;
			set_nv_model(ng, method);
			os_atomic_set_bool(&ng->reinit_done, false);
			for (int i = 0; i < (int)ng->channels; i++) {
				/* Destroy previous FX */
				if (NvAFX_DestroyEffect(ng->handle[i]) != NVAFX_STATUS_SUCCESS) {
					do_log(LOG_ERROR, "FX failed to be destroyed.");
					nvidia_audio_disable(ng);
				} else {
					ng->handle[i] = NULL;
				}
			}
			if (!nvidia_audio_initialize_internal(data))
				nvidia_audio_disable(ng);

			pthread_mutex_unlock(&ng->nvafx_mutex);
		}
	}
}

static void nvafx_logger_callback(void *data, const char *msg)
{
	UNUSED_PARAMETER(data);
	blog(LOG_ERROR, "[NVIDIA Audio Effects: Error - '%s'] ", msg);
}

static void *nvidia_audio_create(obs_data_t *settings, obs_source_t *filter)
{
	struct nvidia_audio_data *ng = bzalloc(sizeof(struct nvidia_audio_data));

	ng->context = filter;

	char sdk_path[MAX_PATH];

	/* find SDK */
	if (!nvafx_get_sdk_path(sdk_path, sizeof(sdk_path))) {
		ng->nvidia_sdk_dir_found = false;
		do_log(LOG_ERROR, "NVAFX redist is not installed.");
		nvidia_audio_destroy(ng);
		return NULL;
	} else {
		size_t size = sizeof(sdk_path) + 1;
		ng->sdk_path = bmalloc(size);
		strcpy(ng->sdk_path, sdk_path);
		ng->nvidia_sdk_dir_found = true;
		ng->nvafx_initialized = false;
		ng->nvafx_loading = false;
		ng->fx = NULL;

		pthread_mutex_init(&ng->nvafx_mutex, NULL);

		info("NVAFX SDK redist path was found here %s", sdk_path);
		// set FX
		const char *method = obs_data_get_string(settings, S_METHOD);
		set_nv_model(ng, method);
		ng->fx = bstrdup(method);
		ng->use_nvafx = true;
	}

	/* Process 10 millisecond segments to keep latency low. */
	/* At 48kHz, NVAFX processes 480 samples which corresponds to 10 ms.*/
	uint32_t sample_rate = audio_output_get_sample_rate(obs_get_audio());
	size_t channels = audio_output_get_channels(obs_get_audio());
	size_t frames = (size_t)sample_rate / (1000 / BUFFER_SIZE_MSEC);
	ng->frames = frames;
	ng->channels = channels;

	/* allocate buffers */
	ng->copy_buffers[0] = bmalloc(frames * channels * sizeof(float));
	ng->nvafx_segment_buffers[0] = bmalloc(NVAFX_FRAME_SIZE * channels * sizeof(float));
	for (size_t c = 1; c < channels; ++c) {
		ng->copy_buffers[c] = ng->copy_buffers[c - 1] + frames;
		ng->nvafx_segment_buffers[c] = ng->nvafx_segment_buffers[c - 1] + NVAFX_FRAME_SIZE;
	}

	/* reserve circular buffers */
	for (size_t i = 0; i < channels; i++) {
		deque_reserve(&ng->input_buffers[i], frames * sizeof(float));
		deque_reserve(&ng->output_buffers[i], frames * sizeof(float));
	}

	/* create resampler if the source is not at 48 kHz */
	if (sample_rate == NVAFX_SAMPLE_RATE) {
		ng->nvafx_resampler = NULL;
		ng->nvafx_resampler_back = NULL;
	} else {
		struct resample_info src, dst;
		src.samples_per_sec = sample_rate;
		src.format = AUDIO_FORMAT_FLOAT_PLANAR;
		src.speakers = nv_convert_speaker_layout((uint8_t)channels);

		dst.samples_per_sec = NVAFX_SAMPLE_RATE;
		dst.format = AUDIO_FORMAT_FLOAT_PLANAR;
		dst.speakers = nv_convert_speaker_layout((uint8_t)channels);

		ng->nvafx_resampler = audio_resampler_create(&dst, &src);
		ng->nvafx_resampler_back = audio_resampler_create(&src, &dst);
	}

	/* VAD */
	ng->vad = 1;

	nvidia_audio_update(ng, settings);

	/* Setup NVIDIA logger */
	if (nvafx_new_sdk) {
		NvAFX_Status err = NvAFX_InitializeLogger(NVAFX_LOG_LEVEL_ERROR, LOG_TARGET_CALLBACK, NULL,
							  &nvafx_logger_callback, ng);
		if (err != NVAFX_STATUS_SUCCESS) {
			warn("NvAFX logger failed to initialize.");
		}
	}

	return ng;
}

static inline void process_fx(struct nvidia_audio_data *ng)
{
	/* Resample if necessary */
	if (ng->nvafx_resampler) {
		float *output[MAX_PREPROC_CHANNELS];
		uint32_t out_frames;
		uint64_t ts_offset;
		audio_resampler_resample(ng->nvafx_resampler, (uint8_t **)output, &out_frames, &ts_offset,
					 (const uint8_t **)ng->copy_buffers, (uint32_t)ng->frames);

		for (size_t i = 0; i < ng->channels; i++) {
			for (ssize_t j = 0, k = (ssize_t)out_frames - NVAFX_FRAME_SIZE; j < NVAFX_FRAME_SIZE;
			     ++j, ++k) {
				if (k >= 0) {
					ng->nvafx_segment_buffers[i][j] = output[i][k];
				} else {
					ng->nvafx_segment_buffers[i][j] = 0;
				}
			}
		}
	} else {
		for (size_t i = 0; i < ng->channels; i++) {
			for (size_t j = 0; j < NVAFX_FRAME_SIZE; ++j) {
				ng->nvafx_segment_buffers[i][j] = ng->copy_buffers[i][j];
			}
		}
	}

	/* Execute */
	size_t runs = ng->has_mono_src ? 1 : ng->channels;
	if (ng->reinit_done) {
		pthread_mutex_lock(&ng->nvafx_mutex);
		for (size_t i = 0; i < runs; i++) {
			NvAFX_Status err = NvAFX_Run(ng->handle[i], &ng->nvafx_segment_buffers[i],
						     &ng->nvafx_segment_buffers[i], ng->num_samples_per_frame,
						     ng->num_channels);
			if (err != NVAFX_STATUS_SUCCESS) {
				if (err == NVAFX_STATUS_FAILED) {
					do_log(LOG_DEBUG,
					       "NvAFX_Run() failed, error NVAFX_STATUS_FAILED.\n"
					       "This can occur when changing the FX and is not consequential.");
					// stop all processing; this will be reset at new init
					os_atomic_set_bool(&ng->reinit_done, false);
				} else {
					do_log(LOG_ERROR, "NvAFX_Run() failed, error %i.\n", err);
				}
			}
		}
		pthread_mutex_unlock(&ng->nvafx_mutex);
	}
	if (ng->has_mono_src) {
		memcpy(ng->nvafx_segment_buffers[1], ng->nvafx_segment_buffers[0], NVAFX_FRAME_SIZE * sizeof(float));
	}
	/* Revert signal level adjustment, resample back if necessary */
	if (ng->nvafx_resampler) {
		float *output[MAX_PREPROC_CHANNELS];
		uint32_t out_frames;
		uint64_t ts_offset;
		audio_resampler_resample(ng->nvafx_resampler_back, (uint8_t **)output, &out_frames, &ts_offset,
					 (const uint8_t **)ng->nvafx_segment_buffers, NVAFX_FRAME_SIZE);

		for (size_t i = 0; i < ng->channels; i++) {
			for (ssize_t j = 0, k = (ssize_t)out_frames - ng->frames; j < (ssize_t)ng->frames; ++j, ++k) {
				if (k >= 0) {
					ng->copy_buffers[i][j] = output[i][k];
				} else {
					ng->copy_buffers[i][j] = 0;
				}
			}
		}
	} else {
		for (size_t i = 0; i < ng->channels; i++) {
			for (size_t j = 0; j < NVAFX_FRAME_SIZE; ++j) {
				ng->copy_buffers[i][j] = ng->nvafx_segment_buffers[i][j];
			}
		}
	}
}

static inline void process(struct nvidia_audio_data *ng)
{
	/* Pop from input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_pop_front(&ng->input_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));

	if (ng->use_nvafx && nvidia_afx_loaded && ng->nvafx_initialized) {
		process_fx(ng);
	}

	/* Push to output deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->output_buffers[i], ng->copy_buffers[i], ng->frames * sizeof(float));
}

struct nv_audio_info {
	uint32_t frames;
	uint64_t timestamp;
};

static inline void clear_deque(struct deque *buf)
{
	deque_pop_front(buf, NULL, buf->size);
}

static void reset_data(struct nvidia_audio_data *ng)
{
	for (size_t i = 0; i < ng->channels; i++) {
		clear_deque(&ng->input_buffers[i]);
		clear_deque(&ng->output_buffers[i]);
	}

	clear_deque(&ng->info_buffer);
}

static struct obs_audio_data *nvidia_audio_filter_audio(void *data, struct obs_audio_data *audio)
{
	struct nvidia_audio_data *ng = data;
	struct nv_audio_info info;
	size_t segment_size = ng->frames * sizeof(float);
	size_t out_size;
	obs_source_t *parent = obs_filter_get_parent(ng->context);
	if (!parent)
		return NULL;
	enum speaker_layout layout = obs_source_get_speaker_layout(parent);
	ng->has_mono_src = layout == SPEAKERS_MONO && ng->channels == 2;

	/* -----------------------------------------------
	 * If timestamp has dramatically changed, consider it a new stream of
	 * audio data. Clear all circular buffers to prevent old audio data
	 * from being processed as part of the new data. */
	if (ng->last_timestamp) {
		int64_t diff = llabs((int64_t)ng->last_timestamp - (int64_t)audio->timestamp);

		if (diff > 1000000000LL)
			reset_data(ng);
	}

	ng->last_timestamp = audio->timestamp;

	/* -----------------------------------------------
	 * push audio packet info (timestamp/frame count) to info deque */
	info.frames = audio->frames;
	info.timestamp = audio->timestamp;
	deque_push_back(&ng->info_buffer, &info, sizeof(info));

	/* -----------------------------------------------
	 * push back current audio data to input deque */
	for (size_t i = 0; i < ng->channels; i++)
		deque_push_back(&ng->input_buffers[i], audio->data[i], audio->frames * sizeof(float));

	/* -----------------------------------------------
	 * pop/process each 10ms segments, push back to output deque */
	while (ng->input_buffers[0].size >= segment_size)
		process(ng);

	/* -----------------------------------------------
	 * peek front of info deque, check to see if we have enough to
	 * pop the expected packet size, if not, return null */
	memset(&info, 0, sizeof(info));
	deque_peek_front(&ng->info_buffer, &info, sizeof(info));
	out_size = info.frames * sizeof(float);

	if (ng->output_buffers[0].size < out_size)
		return NULL;

	/* -----------------------------------------------
	 * if there's enough audio data buffered in the output deque,
	 * pop and return a packet */
	deque_pop_front(&ng->info_buffer, NULL, sizeof(info));
	da_resize(ng->output_data, out_size * ng->channels);

	for (size_t i = 0; i < ng->channels; i++) {
		ng->output_audio.data[i] = (uint8_t *)&ng->output_data.array[i * out_size];

		deque_pop_front(&ng->output_buffers[i], ng->output_audio.data[i], out_size);
	}

	ng->output_audio.frames = info.frames;
	ng->output_audio.timestamp = info.timestamp - ng->latency;
	return &ng->output_audio;
}

static void nvidia_audio_defaults(obs_data_t *s)
{
	obs_data_set_default_double(s, S_NVAFX_INTENSITY, 1.0);
	obs_data_set_default_string(s, S_METHOD, S_METHOD_NVAFX_DENOISER);
	if (nvafx_new_sdk)
		obs_data_set_default_bool(s, S_NVAFX_VAD, 1);
}

static obs_properties_t *nvidia_audio_properties(void *data)
{
	obs_properties_t *ppts = obs_properties_create();
	struct nvidia_audio_data *ng = (struct nvidia_audio_data *)data;
	obs_property_t *method =
		obs_properties_add_list(ppts, S_METHOD, TEXT_METHOD, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	if (ng->nvidia_sdk_dir_found) {
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DENOISER, S_METHOD_NVAFX_DENOISER);
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DEREVERB, S_METHOD_NVAFX_DEREVERB);
		obs_property_list_add_string(method, TEXT_METHOD_NVAFX_DEREVERB_DENOISER,
					     S_METHOD_NVAFX_DEREVERB_DENOISER);
		obs_property_t *slider = obs_properties_add_float_slider(ppts, S_NVAFX_INTENSITY, TEXT_NVAFX_INTENSITY,
									 0.0f, 1.0f, 0.01f);
		obs_property_t *vad = obs_properties_add_bool(ppts, S_NVAFX_VAD, TEXT_NVAFX_VAD);
		if (!nvafx_new_sdk)
			obs_property_set_visible(vad, 0);

		unsigned int version = get_lib_version();
		obs_property_t *warning = obs_properties_add_text(ppts, "deprecation", NULL, OBS_TEXT_INFO);
		if (version && version < MIN_AFX_SDK_VERSION) {
			obs_property_text_set_info_type(warning, OBS_TEXT_INFO_WARNING);
			obs_property_set_long_description(warning, TEXT_METHOD_NVAFX_DEPRECATION);
		} else {
			obs_property_set_visible(warning, 0);
		}
	}

	return ppts;
}

struct obs_source_info nvidia_audiofx_filter = {
	.id = "nvidia_audiofx_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = nvidia_audio_name,
	.create = nvidia_audio_create,
	.destroy = nvidia_audio_destroy,
	.update = nvidia_audio_update,
	.filter_audio = nvidia_audio_filter_audio,
	.get_defaults = nvidia_audio_defaults,
	.get_properties = nvidia_audio_properties,
};
