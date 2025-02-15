#include <obs-module.h>
#include <util/threading.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include "nvvfx-load.h"
/* -------------------------------------------------------- */

#define do_log(level, format, ...) \
	blog(level, "[NVIDIA Video Effect: '%s'] " format, obs_source_get_name(filter->context), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)

#ifdef _DEBUG
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif

/* -------------------------------------------------------- */
#define S_MODE "mode"
#define S_MODE_QUALITY 0
#define S_MODE_PERF 1
#define S_THRESHOLDFX "threshold"
#define S_THRESHOLDFX_DEFAULT 1.0
#define S_PROCESSING "processing_interval"

#define MT_ obs_module_text
#define TEXT_MODE MT_("Nvvfx.Method.Greenscreen.Mode")
#define TEXT_MODE_QUALITY MT_("Nvvfx.Method.Greenscreen.Quality")
#define TEXT_MODE_PERF MT_("Nvvfx.Method.Greenscreen.Performance")
#define TEXT_MODE_THRESHOLD MT_("Nvvfx.Method.Greenscreen.Threshold")
#define TEXT_DEPRECATION MT_("Nvvfx.OutdatedSDK")
#define TEXT_PROCESSING MT_("Nvvfx.Method.Greenscreen.Processing")
#define TEXT_PROCESSING_HINT MT_("Nvvfx.Method.Greenscreen.Processing.Hint")

/* Blur & background blur FX */
#define S_STRENGTH "intensity"
#define S_STRENGTH_DEFAULT 0.5
#define TEXT_MODE_BLUR_STRENGTH MT_("Nvvfx.Method.Blur.Strength")

enum nvvfx_filter_id { S_FX_AIGS, S_FX_BLUR, S_FX_BG_BLUR };

bool nvvfx_loaded = false;
bool nvvfx_new_sdk = false;

/* clang-format off */
struct nvvfx_data {
	obs_source_t *context;
	bool images_allocated;
	bool initial_render;
	volatile bool processing_stop;
	bool processed_frame;
	bool target_valid;
	bool got_new_frame;
	signal_handler_t *handler;

	/* RTX SDK vars */
	NvVFX_Handle handle;
	CUstream stream;            // CUDA stream
	int mode;                   // 0 = quality, 1 = performance
	NvCVImage *src_img;         // src img in obs format (RGBA ?) on GPU
	NvCVImage *BGR_src_img;     // src img in BGR on GPU
	NvCVImage *A_dst_img;       // mask img on GPU
	NvCVImage *dst_img;         // Greenscreen: alpha mask texture; blur: texture initialized from d3d11 (RGBA, chunky, u8)
	NvCVImage *stage;           // used for transfer to texture
	unsigned int version;
	NvVFX_StateObjectHandle stateObjectHandle;

	/* alpha mask effect */
	gs_effect_t *effect;
	gs_texrender_t *render;
	gs_texrender_t *render_unorm;
	gs_texture_t *alpha_texture; // either alpha or blur
	uint32_t width;              // width of texture
	uint32_t height;             // height of texture
	enum gs_color_space space;
	gs_eparam_t *mask_param;
	gs_eparam_t *image_param;
	gs_eparam_t *threshold_param;
	gs_eparam_t *multiplier_param;
	float threshold;

	/* Every nth frame is processed through the FX (where n =
	 * processing_interval) so that the same mask is used for n consecutive
	 * frames. This is to alleviate the GPU load. Default: 1 (process every frame).
	 * Only used for AIGS.
	 */
	int processing_interval;
	int processing_counter;

	/* blur specific */
	enum nvvfx_filter_id filter_id;
	NvVFX_Handle handle_blur;
	CUstream stream_blur;        // cuda stream
	float strength;              // from 0 to 1, default = 0.5
	NvCVImage *blur_BGR_dst_img; // dst img of blur FX (BGR, chunky, u8)
	NvCVImage *RGBA_dst_img;     // tmp img used to transfer to texture
	NvCVImage *blur_dst_img;     // dst img initialized from d3d11 texture (RGBA, chunky, u8)
	gs_texture_t *blur_texture;
	gs_eparam_t *blur_param;
};
/* clang-format on */

static const char *nv_greenscreen_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Nvvfx.Method.Greenscreen");
}

static const char *nv_blur_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Nvvfx.Method.BlurFilter");
}

static const char *nv_background_blur_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Nvvfx.Method.BackgroundBlurFilter");
}

static void nvvfx_filter_update(void *data, obs_data_t *settings)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)data;
	NvCV_Status vfxErr;
	enum nvvfx_fx_id id = filter->filter_id;

	filter->threshold = (float)obs_data_get_double(settings, S_THRESHOLDFX);
	filter->processing_interval = (int)obs_data_get_int(settings, S_PROCESSING);
	float strength = (float)obs_data_get_double(settings, S_STRENGTH);
	if (id == S_FX_AIGS || id == S_FX_BG_BLUR) {
		int mode = id == S_FX_BG_BLUR ? (int)S_MODE_QUALITY : (int)obs_data_get_int(settings, S_MODE);
		if (filter->mode != mode) {
			filter->mode = mode;
			vfxErr = NvVFX_SetU32(filter->handle, NVVFX_MODE, mode);
			vfxErr = NvVFX_Load(filter->handle);
			if (NVCV_SUCCESS != vfxErr)
				error("Error loading AI Greenscreen FX %i", vfxErr);
		}
	}
	if (id == S_FX_BLUR || id == S_FX_BG_BLUR) {
		if (filter->strength != strength) {
			filter->strength = strength;
			vfxErr = NvVFX_SetF32(filter->handle_blur, NVVFX_STRENGTH, filter->strength);
		}
		vfxErr = NvVFX_Load(filter->handle_blur);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading blur FX %i", vfxErr);
	}
}

static void nvvfx_filter_actual_destroy(void *data)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)data;
	if (!nvvfx_loaded) {
		bfree(filter);
		return;
	}

	os_atomic_set_bool(&filter->processing_stop, true);

	if (filter->images_allocated) {
		obs_enter_graphics();
		if (filter->filter_id == S_FX_AIGS)
			gs_texture_destroy(filter->alpha_texture);
		else
			gs_texture_destroy(filter->blur_texture);

		gs_texrender_destroy(filter->render);
		gs_texrender_destroy(filter->render_unorm);
		obs_leave_graphics();
		NvCVImage_Destroy(filter->src_img);
		NvCVImage_Destroy(filter->BGR_src_img);
		NvCVImage_Destroy(filter->A_dst_img);
		NvCVImage_Destroy(filter->dst_img);
		NvCVImage_Destroy(filter->stage);
		if (filter->filter_id != S_FX_AIGS) {
			NvCVImage_Destroy(filter->blur_BGR_dst_img);
			NvCVImage_Destroy(filter->RGBA_dst_img);
			NvCVImage_Destroy(filter->blur_dst_img);
		}
	}
	if (filter->stream)
		NvVFX_CudaStreamDestroy(filter->stream);
	if (filter->stream_blur)
		NvVFX_CudaStreamDestroy(filter->stream_blur);

	if (filter->handle) {
		if (filter->stateObjectHandle) {
			NvVFX_DeallocateState(filter->handle, filter->stateObjectHandle);
		}
		NvVFX_DestroyEffect(filter->handle);
	}
	if (filter->handle_blur) {
		NvVFX_DestroyEffect(filter->handle_blur);
	}

	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(filter);
}

static void nvvfx_filter_destroy(void *data)
{
	obs_queue_task(OBS_TASK_GRAPHICS, nvvfx_filter_actual_destroy, data, false);
}

static void *log_nverror_destroy(struct nvvfx_data *filter, NvCV_Status vfxErr)
{
	const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
	error("Error creating NVIDIA Video FX; error %i: %s", vfxErr, errString);
	nvvfx_filter_destroy(filter);
	return NULL;
}

//---------------------------------------------------------------------------//
// filter creation //

static bool nvvfx_filter_create_internal(struct nvvfx_data *filter)
{
	NvCV_Status vfxErr;
	enum nvvfx_fx_id id = filter->filter_id;
	/* 1. Create FX */
	switch (id) {
	case S_FX_AIGS:
		vfxErr = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &filter->handle);
		break;
	case S_FX_BLUR:
		vfxErr = NvVFX_CreateEffect(NVVFX_FX_BGBLUR, &filter->handle_blur);
		break;
	case S_FX_BG_BLUR:
		vfxErr = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &filter->handle);
		if (NVCV_SUCCESS != vfxErr)
			log_nverror_destroy(filter, vfxErr);
		vfxErr = NvVFX_CreateEffect(NVVFX_FX_BGBLUR, &filter->handle_blur);
		break;
	default:
		return false;
	}
	if (NVCV_SUCCESS != vfxErr)
		log_nverror_destroy(filter, vfxErr);
		/* debug */
#ifdef _DEBUG
	const char *info;
	vfxErr = NvVFX_GetString(filter->handle_blur, NVVFX_INFO, &info);
	blog(LOG_DEBUG, "blur fx settings /n/%s", info);
#endif
	/* 2. Set models path & initialize CudaStream */
	if (id == S_FX_AIGS || id == S_FX_BG_BLUR) {
		char buffer[MAX_PATH];
		char modelDir[MAX_PATH];
		nvvfx_get_sdk_path(buffer, MAX_PATH);
		size_t max_len = sizeof(buffer) / sizeof(char);
		snprintf(modelDir, max_len, "%s\\models", buffer);
		vfxErr = NvVFX_SetString(filter->handle, NVVFX_MODEL_DIRECTORY, modelDir);
		vfxErr = NvVFX_CudaStreamCreate(&filter->stream);
		if (NVCV_SUCCESS != vfxErr)
			log_nverror_destroy(filter, vfxErr);
		vfxErr = NvVFX_SetCudaStream(filter->handle, NVVFX_CUDA_STREAM, filter->stream);
		if (NVCV_SUCCESS != vfxErr)
			log_nverror_destroy(filter, vfxErr);
	}
	if (id == S_FX_BLUR || id == S_FX_BG_BLUR) {
		vfxErr = NvVFX_CudaStreamCreate(&filter->stream_blur);
		if (NVCV_SUCCESS != vfxErr)
			log_nverror_destroy(filter, vfxErr);
		vfxErr = NvVFX_SetCudaStream(filter->handle_blur, NVVFX_CUDA_STREAM, filter->stream_blur);
		if (NVCV_SUCCESS != vfxErr)
			log_nverror_destroy(filter, vfxErr);
	}
	return true;
}

static void *nvvfx_filter_create(obs_data_t *settings, obs_source_t *context, enum nvvfx_fx_id id)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)bzalloc(sizeof(*filter));
	if (!nvvfx_loaded) {
		nvvfx_filter_destroy(filter);
		return NULL;
	}

	NvCV_Status vfxErr;
	filter->context = context;
	filter->mode = -1; // should be 0 or 1; -1 triggers an update
	filter->images_allocated = false;
	filter->processed_frame = true; // start processing when false
	filter->width = 0;
	filter->height = 0;
	filter->initial_render = false;
	os_atomic_set_bool(&filter->processing_stop, false);
	filter->handler = NULL;
	filter->processing_interval = 1;
	filter->processing_counter = 0;
	// set nvvfx_fx_id
	filter->filter_id = id;
	// blur specific
	filter->strength = -1;

#ifdef _DEBUG
	/* sanity check of sdk version */
	if (NvVFX_GetVersion(&filter->version) == NVCV_SUCCESS) {
		uint8_t major = (filter->version >> 24) & 0xff;
		uint8_t minor = (filter->version >> 16) & 0x00ff;
		uint8_t build = (filter->version >> 8) & 0x0000ff;
		uint8_t revision = (filter->version >> 0) & 0x000000ff;
		nvvfx_new_sdk = filter->version >= MIN_VFX_SDK_VERSION && nvvfx_new_sdk;
	}
#endif
	/* 1. Create FX */
	/* 2. Set models path & initialize CudaStream */
	if (!nvvfx_filter_create_internal(filter))
		return NULL;

	/* 3. Load effect. */
	char *effect_path = obs_module_file(id != S_FX_AIGS ? "rtx_blur.effect" : "rtx_greenscreen.effect");
	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);
	if (filter->effect) {
		if (id == S_FX_AIGS) {
			filter->mask_param = gs_effect_get_param_by_name(filter->effect, "mask");
			filter->threshold_param = gs_effect_get_param_by_name(filter->effect, "threshold");
		} else {
			filter->blur_param = gs_effect_get_param_by_name(filter->effect, "blurred");
		}
		filter->image_param = gs_effect_get_param_by_name(filter->effect, "image");

		filter->multiplier_param = gs_effect_get_param_by_name(filter->effect, "multiplier");
	}
	obs_leave_graphics();

	/* 4. Allocate state for the AIGS & background blur */
	if (nvvfx_new_sdk && id != S_FX_BLUR) {
		vfxErr = NvVFX_AllocateState(filter->handle, &filter->stateObjectHandle);
		if (NVCV_SUCCESS != vfxErr)
			return log_nverror_destroy(filter, vfxErr);
		vfxErr = NvVFX_SetStateObjectHandleArray(filter->handle, NVVFX_STATE, &filter->stateObjectHandle);
		if (NVCV_SUCCESS != vfxErr)
			return log_nverror_destroy(filter, vfxErr);
	}

	if (!filter->effect) {
		nvvfx_filter_destroy(filter);
		return NULL;
	}

	nvvfx_filter_update(filter, settings);

	return filter;
}

static void *nv_greenscreen_filter_create(obs_data_t *settings, obs_source_t *context)
{
	return nvvfx_filter_create(settings, context, S_FX_AIGS);
}

static void *nv_blur_filter_create(obs_data_t *settings, obs_source_t *context)
{
	return nvvfx_filter_create(settings, context, S_FX_BLUR);
}

static void *nv_background_blur_filter_create(obs_data_t *settings, obs_source_t *context)
{
	return nvvfx_filter_create(settings, context, S_FX_BG_BLUR);
}

static void nvvfx_filter_reset(void *data, calldata_t *calldata)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)data;
	NvCV_Status vfxErr;

	os_atomic_set_bool(&filter->processing_stop, true);
	// [A] first destroy
	if (filter->stream) {
		NvVFX_CudaStreamDestroy(filter->stream);
	}
	if (filter->stream_blur) {
		NvVFX_CudaStreamDestroy(filter->stream_blur);
	}
	if (filter->handle) {
		if (filter->stateObjectHandle) {
			NvVFX_DeallocateState(filter->handle, filter->stateObjectHandle);
		}
		NvVFX_DestroyEffect(filter->handle);
	}
	if (filter->handle_blur) {
		NvVFX_DestroyEffect(filter->handle_blur);
	}
	// [B] recreate
	/* 1. Create FX */
	/* 2. Set models path & initialize CudaStream */
	if (!nvvfx_filter_create_internal(filter))
		return;

	/* 3. load FX */
	if (filter->filter_id != S_FX_BLUR) {
		vfxErr = NvVFX_SetU32(filter->handle, NVVFX_MODE, filter->mode);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading NVIDIA Video FX %i", vfxErr);
		vfxErr = NvVFX_Load(filter->handle);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading NVIDIA Video FX %i", vfxErr);
		vfxErr = NvVFX_ResetState(filter->handle, filter->stateObjectHandle);
	}
	if (filter->filter_id != S_FX_AIGS) {
		vfxErr = NvVFX_SetF32(filter->handle_blur, NVVFX_STRENGTH, filter->strength);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading NVIDIA Video FX %i", vfxErr);
		vfxErr = NvVFX_Load(filter->handle_blur);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading blur FX %i", vfxErr);
	}

	filter->images_allocated = false;
	os_atomic_set_bool(&filter->processing_stop, false);
}

//---------------------------------------------------------------------------//
// intialization of images //

static bool create_alpha_texture(struct nvvfx_data *filter)
{
	NvCV_Status vfxErr;
	uint32_t width = filter->width;
	uint32_t height = filter->height;

	/* 1. create alpha texture */
	if (filter->alpha_texture) {
		gs_texture_destroy(filter->alpha_texture);
	}
	filter->alpha_texture = gs_texture_create(width, height, GS_A8, 1, NULL, 0);
	if (filter->alpha_texture == NULL) {
		error("Alpha texture couldn't be created");
		return false;
	}
	struct ID3D11Texture2D *d11texture = (struct ID3D11Texture2D *)gs_texture_get_obj(filter->alpha_texture);

	/* 2. Create NvCVImage which will hold final alpha texture. */
	if (!filter->dst_img)
		NvCVImage_Create(width, height, NVCV_A, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->dst_img);
	vfxErr = NvCVImage_InitFromD3D11Texture(filter->dst_img, d11texture);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error passing dst ID3D11Texture to img; error %i: %s", vfxErr, errString);
		return false;
	}

	return true;
}

static bool create_blur_texture(struct nvvfx_data *filter)
{
	NvCV_Status vfxErr;
	uint32_t width = filter->width;
	uint32_t height = filter->height;

	/* 1. Create blur texture */
	if (filter->blur_texture) {
		gs_texture_destroy(filter->blur_texture);
	}
	filter->blur_texture = gs_texture_create(width, height, GS_RGBA_UNORM, 1, NULL, 0);
	if (filter->blur_texture == NULL) {
		error("Blur texture couldn't be created");
		return false;
	}
	struct ID3D11Texture2D *d11texture = (struct ID3D11Texture2D *)gs_texture_get_obj(filter->blur_texture);

	/* 2. Create NvCVImage which will hold final blur texture */
	if (!filter->blur_dst_img)
		NvCVImage_Create(width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->blur_dst_img);
	vfxErr = NvCVImage_InitFromD3D11Texture(filter->blur_dst_img, d11texture);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error passing dst ID3D11Texture to img; error %i: %s", vfxErr, errString);
		return false;
	}

	return true;
}

static bool create_texrenders(struct nvvfx_data *filter)
{
	if (filter->render)
		gs_texrender_destroy(filter->render);
	filter->render = gs_texrender_create(gs_get_format_from_space(filter->space), GS_ZS_NONE);
	if (!filter->render) {
		error("Failed to create render texrenderer");
		return false;
	}
	if (filter->render_unorm)
		gs_texrender_destroy(filter->render_unorm);
	filter->render_unorm = gs_texrender_create(GS_BGRA_UNORM, GS_ZS_NONE);
	if (!filter->render_unorm) {
		error("Failed to create render_unorm texrenderer");
		return false;
	}

	return true;
}

static bool init_blur_images(struct nvvfx_data *filter)
{
	uint32_t width = filter->width;
	uint32_t height = filter->height;

	/* 1. Create and allocate Blur BGR NvCVimage (blur FX dst) */
	NvCVImage_Create(width, height, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->blur_BGR_dst_img);
	NvCVImage_Alloc(filter->blur_BGR_dst_img, width, height, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);

	/* 2. Create dst NvCVImage */
	if (filter->RGBA_dst_img) {
		NvCVImage_Realloc(filter->RGBA_dst_img, width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	} else {
		NvCVImage_Create(width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->RGBA_dst_img);
		NvCVImage_Alloc(filter->RGBA_dst_img, width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	}

	/* 3. Set input & output images for nv blur FX */
	NvVFX_SetImage(filter->handle_blur, NVVFX_INPUT_IMAGE, filter->BGR_src_img);
	NvVFX_SetImage(filter->handle_blur, NVVFX_INPUT_IMAGE_1, filter->A_dst_img);
	NvVFX_SetImage(filter->handle_blur, NVVFX_OUTPUT_IMAGE, filter->blur_BGR_dst_img);

	if (NvVFX_Load(filter->handle_blur) != NVCV_SUCCESS) {
		error("Error loading blur FX");
		return false;
	}

	return true;
}

static void init_images(struct nvvfx_data *filter)
{
	NvCV_Status vfxErr;
	uint32_t width = filter->width;
	uint32_t height = filter->height;

	/* 1. Create alpha texture & associated NvCVImage */
	if (filter->filter_id == S_FX_BG_BLUR || filter->filter_id == S_FX_AIGS) {
		if (!create_alpha_texture(filter))
			goto fail;
	}
	/* 2. Create blur texture & associated NvCVImage */
	if (filter->filter_id == S_FX_BG_BLUR || filter->filter_id == S_FX_BLUR) {
		if (!create_blur_texture(filter))
			goto fail;
	}
	/* 3. Create texrenders */
	if (!create_texrenders(filter))
		goto fail;

	/* 4. Create and allocate BGR NvCVImage (alpha mask FX src) */
	if (filter->BGR_src_img) {
		NvCVImage_Realloc(filter->BGR_src_img, width, height, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	} else {
		NvCVImage_Create(width, height, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->BGR_src_img);
		NvCVImage_Alloc(filter->BGR_src_img, width, height, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	}

	/* 5. Create and allocate Alpha NvCVimage (mask fx dst). */
	if (filter->A_dst_img) {
		NvCVImage_Realloc(filter->A_dst_img, width, height, NVCV_A, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	} else {
		NvCVImage_Create(width, height, NVCV_A, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->A_dst_img);
		NvCVImage_Alloc(filter->A_dst_img, width, height, NVCV_A, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	}

	/* 6. Create stage NvCVImage which will be used as buffer for transfer */
	vfxErr = NvCVImage_Create(width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1, &filter->stage);
	vfxErr = NvCVImage_Alloc(filter->stage, width, height, NVCV_RGBA, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
	if (vfxErr != NVCV_SUCCESS) {
		goto fail;
	}

	/* 7. Init blur images */
	if (filter->filter_id == S_FX_BLUR || filter->filter_id == S_FX_BG_BLUR) {
		if (!init_blur_images(filter))
			goto fail;
	}

	/* 8. Pass settings for AIGS (AI Greenscreen) FX */
	if (filter->filter_id == S_FX_BG_BLUR || filter->filter_id == S_FX_AIGS) {
		NvVFX_SetImage(filter->handle, NVVFX_INPUT_IMAGE, filter->BGR_src_img);
		NvVFX_SetImage(filter->handle, NVVFX_OUTPUT_IMAGE, filter->A_dst_img);
		if (filter->width)
			NvVFX_SetU32(filter->handle, NVVFX_MAX_INPUT_WIDTH, filter->width);
		if (filter->height)
			NvVFX_SetU32(filter->handle, NVVFX_MAX_INPUT_HEIGHT, filter->height);
		vfxErr = NvVFX_Load(filter->handle);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading AI Greenscreen FX %i", vfxErr);
	}
	filter->images_allocated = true;
	return;
fail:
	error("Error during allocation of images");
	os_atomic_set_bool(&filter->processing_stop, true);
	return;
}

//---------------------------------------------------------------------------//
// video processing functions //

static bool process_texture(struct nvvfx_data *filter)
{
	enum nvvfx_fx_id id = filter->filter_id;
	CUstream process_stream;

	/* 1. Map src img holding texture. */
	switch (id) {
	case S_FX_AIGS:
	case S_FX_BG_BLUR:
		process_stream = filter->stream;
		break;
	case S_FX_BLUR:
		process_stream = filter->stream_blur;
		break;
	default:
		process_stream = NULL;
	}
	NvCV_Status vfxErr = NvCVImage_MapResource(filter->src_img, process_stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error mapping resource for source texture; error %i : %s", vfxErr, errString);
		goto fail;
	}

	/* 2. Convert to BGR. */
	vfxErr = NvCVImage_Transfer(filter->src_img, filter->BGR_src_img, 1.0f, filter->stream_blur, filter->stage);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error converting src to BGR img; error %i: %s", vfxErr, errString);
		goto fail;
	}
	vfxErr = NvCVImage_UnmapResource(filter->src_img, process_stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error unmapping resource for src texture; error %i: %s", vfxErr, errString);
		goto fail;
	}

	/* 3. Run AIGS (AI Greenscreen) fx */
	if (id != S_FX_BLUR) {
		vfxErr = NvVFX_Run(filter->handle, 1);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error running the AIGS FX; error %i: %s", vfxErr, errString);
			if (vfxErr == NVCV_ERR_CUDA)
				nvvfx_filter_reset(filter, NULL);
		}
	}

	if (id != S_FX_AIGS) {
		/* 4. BLUR FX */
		/* 4a. Run BLUR FX except for AIGS */
		vfxErr = NvVFX_Run(filter->handle_blur, 1);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error running the BLUR FX; error %i: %s", vfxErr, errString);
			if (vfxErr == NVCV_ERR_CUDA)
				nvvfx_filter_reset(filter, NULL);
		}
		/* 4b. Transfer blur result to an intermediate dst [RGBA, chunky, u8] */
		vfxErr = NvCVImage_Transfer(filter->blur_BGR_dst_img, filter->RGBA_dst_img, 1.0f, filter->stream_blur,
					    filter->stage);
		if (vfxErr != NVCV_SUCCESS) {
			error("Error transferring blurred to intermediate img [RGBA, chunky, u8], error %i, ", vfxErr);
			goto fail;
		}
		/* 5. Map blur dst texture before transfer from dst img provided by FX */
		vfxErr = NvCVImage_MapResource(filter->blur_dst_img, filter->stream_blur);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error mapping resource for dst texture; error %i: %s", vfxErr, errString);
			goto fail;
		}

		vfxErr = NvCVImage_Transfer(filter->RGBA_dst_img, filter->blur_dst_img, 1.0f, filter->stream_blur,
					    filter->stage);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error transferring mask to alpha texture; error %i: %s ", vfxErr, errString);
			goto fail;
		}

		vfxErr = NvCVImage_UnmapResource(filter->blur_dst_img, filter->stream_blur);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error unmapping resource for dst texture; error %i: %s", vfxErr, errString);
			goto fail;
		}
	} else {
		/* 4. Map dst texture before transfer from dst img provided by AIGS FX */
		vfxErr = NvCVImage_MapResource(filter->dst_img, filter->stream);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error mapping resource for dst texture; error %i: %s", vfxErr, errString);
			goto fail;
		}

		vfxErr = NvCVImage_Transfer(filter->A_dst_img, filter->dst_img, 1.0f, filter->stream, filter->stage);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error transferring mask to alpha texture; error %i: %s ", vfxErr, errString);
			goto fail;
		}

		vfxErr = NvCVImage_UnmapResource(filter->dst_img, filter->stream);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error unmapping resource for dst texture; error %i: %s", vfxErr, errString);
			goto fail;
		}
	}

	return true;
fail:
	os_atomic_set_bool(&filter->processing_stop, true);
	return false;
}

static struct obs_source_frame *nvvfx_filter_video(void *data, struct obs_source_frame *frame)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)data;
	filter->got_new_frame = true;
	return frame;
}

static void nvvfx_filter_tick(void *data, float t)
{
	UNUSED_PARAMETER(t);

	struct nvvfx_data *filter = (struct nvvfx_data *)data;

	if (filter->processing_stop) {
		return;
	}
	if (!obs_filter_get_target(filter->context)) {
		return;
	}
	obs_source_t *target = obs_filter_get_target(filter->context);

	filter->target_valid = true;

	const uint32_t cx = obs_source_get_base_width(target);
	const uint32_t cy = obs_source_get_base_height(target);

	// initially the sizes are 0
	if (!cx && !cy) {
		filter->target_valid = false;
		return;
	}

	/* minimum size supported by SDK is (512,288) */
	if (filter->filter_id != S_FX_BLUR) {
		filter->target_valid = cx >= 512 && cy >= 288;
		if (!filter->target_valid) {
			error("Size must be larger than (512,288)");
			return;
		}
	}

	if (cx != filter->width && cy != filter->height) {
		filter->images_allocated = false;
		filter->width = cx;
		filter->height = cy;
	}
	if (!filter->images_allocated) {
		obs_enter_graphics();
		init_images(filter);
		obs_leave_graphics();
		filter->initial_render = false;
	}

	filter->processed_frame = false;
}

static const char *get_tech_name_and_multiplier(enum gs_color_space current_space, enum gs_color_space source_space,
						float *multiplier)
{
	const char *tech_name = "Draw";
	*multiplier = 1.f;

	switch (source_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
		if (current_space == GS_CS_709_SCRGB) {
			tech_name = "DrawMultiply";
			*multiplier = obs_get_video_sdr_white_level() / 80.0f;
		}
		break;
	case GS_CS_709_EXTENDED:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			tech_name = "DrawTonemap";
			break;
		case GS_CS_709_SCRGB:
			tech_name = "DrawMultiply";
			*multiplier = obs_get_video_sdr_white_level() / 80.0f;
		}
		break;
	case GS_CS_709_SCRGB:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			tech_name = "DrawMultiplyTonemap";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
			break;
		case GS_CS_709_EXTENDED:
			tech_name = "DrawMultiply";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
		}
	}

	return tech_name;
}

static void draw_greenscreen_blur(struct nvvfx_data *filter, bool has_blur)
{
	const enum gs_color_space source_space = filter->space;
	float multiplier;
	const char *technique = get_tech_name_and_multiplier(gs_get_color_space(), source_space, &multiplier);
	const enum gs_color_format format = gs_get_format_from_space(source_space);
	if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
							     OBS_ALLOW_DIRECT_RENDERING)) {
		if (has_blur) {
			gs_effect_set_texture_srgb(filter->blur_param, filter->blur_texture);
		} else {
			gs_effect_set_texture(filter->mask_param, filter->alpha_texture);
			gs_effect_set_float(filter->threshold_param, filter->threshold);
		}
		gs_effect_set_texture_srgb(filter->image_param, gs_texrender_get_texture(filter->render));

		gs_effect_set_float(filter->multiplier_param, multiplier);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		obs_source_process_filter_tech_end(filter->context, filter->effect, 0, 0, technique);

		gs_blend_state_pop();
	}
}

static void nvvfx_filter_render(void *data, gs_effect_t *effect, bool has_blur)
{
	NvCV_Status vfxErr;
	struct nvvfx_data *filter = (struct nvvfx_data *)data;

	if (filter->processing_stop || (has_blur && filter->strength == 0)) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	obs_source_t *const target = obs_filter_get_target(filter->context);
	obs_source_t *const parent = obs_filter_get_parent(filter->context);

	/* Skip if processing of a frame hasn't yet started */
	if (!filter->target_valid || !target || !parent) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	/* Render processed image from earlier in the frame */
	if (filter->processed_frame) {
		draw_greenscreen_blur(filter, has_blur);
		return;
	}

	if (parent && !filter->handler) {
		filter->handler = obs_source_get_signal_handler(parent);
		signal_handler_connect(filter->handler, "update", nvvfx_filter_reset, filter);
	}

	/* 1. Render to retrieve texture. */
	if (!filter->render) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const uint32_t target_flags = obs_source_get_output_flags(target);
	const uint32_t parent_flags = obs_source_get_output_flags(parent);

	bool custom_draw = (target_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
	bool async = (target_flags & OBS_SOURCE_ASYNC) != 0;

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space =
		obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	if (filter->space != source_space) {
		filter->space = source_space;
		init_images(filter);
		filter->initial_render = false;
	}

	gs_texrender_t *const render = filter->render;
	gs_texrender_reset(render);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	if (gs_texrender_begin_with_color_space(render, filter->width, filter->height, source_space)) {
		struct vec4 clear_color;
		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

		gs_ortho(0.0f, (float)filter->width, 0.0f, (float)filter->height, -100.0f, 100.0f);

		if (target == parent && !custom_draw && !async)
			obs_source_default_render(target);
		else
			obs_source_video_render(target);

		gs_texrender_end(render);

		gs_texrender_t *const render_unorm = filter->render_unorm;
		gs_texrender_reset(render_unorm);
		if (gs_texrender_begin_with_color_space(render_unorm, filter->width, filter->height, GS_CS_SRGB)) {
			const bool previous = gs_framebuffer_srgb_enabled();
			gs_enable_framebuffer_srgb(true);
			gs_enable_blending(false);

			gs_ortho(0.0f, (float)filter->width, 0.0f, (float)filter->height, -100.0f, 100.0f);

			const char *tech_name = "ConvertUnorm";
			float multiplier = 1.f;
			if (!has_blur) {
				switch (source_space) {
				case GS_CS_709_EXTENDED:
					tech_name = "ConvertUnormTonemap";
					break;
				case GS_CS_709_SCRGB:
					tech_name = "ConvertUnormMultiplyTonemap";
					multiplier = 80.0f / obs_get_video_sdr_white_level();
				}
			} else {
				switch (source_space) {
				case GS_CS_709_SCRGB:
					tech_name = "ConvertUnormMultiply";
					multiplier = 80.0f / obs_get_video_sdr_white_level();
				}
			}
			gs_effect_set_texture_srgb(filter->image_param, gs_texrender_get_texture(render));
			gs_effect_set_float(filter->multiplier_param, multiplier);

			while (gs_effect_loop(filter->effect, tech_name)) {
				gs_draw(GS_TRIS, 0, 3);
			}

			gs_texrender_end(render_unorm);

			gs_enable_blending(true);
			gs_enable_framebuffer_srgb(previous);
		}
	}

	gs_blend_state_pop();

	/* 2. Initialize src_texture (only at startup or reset) */
	if (!filter->initial_render) {
		struct ID3D11Texture2D *d11texture2 =
			(struct ID3D11Texture2D *)gs_texture_get_obj(gs_texrender_get_texture(filter->render_unorm));
		if (!d11texture2) {
			error("Couldn't retrieve d3d11texture2d.");
			return;
		}

		if (!filter->src_img) {
			vfxErr = NvCVImage_Create(filter->width, filter->height, NVCV_BGRA, NVCV_U8, NVCV_CHUNKY,
						  NVCV_GPU, 1, &filter->src_img);
			if (vfxErr != NVCV_SUCCESS) {
				const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
				error("Error creating src img; error %i: %s", vfxErr, errString);
				os_atomic_set_bool(&filter->processing_stop, true);
				return;
			}
		}

		vfxErr = NvCVImage_InitFromD3D11Texture(filter->src_img, d11texture2);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Error passing src ID3D11Texture to img; error %i: %s", vfxErr, errString);
			os_atomic_set_bool(&filter->processing_stop, true);
			return;
		}

		filter->initial_render = true;
	}

	/* 3. Process FX (outputs a mask) & draw. */
	if (filter->initial_render && filter->images_allocated) {
		bool draw = true;
		if (!async || filter->got_new_frame) {
			if (!has_blur) {
				if (filter->processing_counter % filter->processing_interval == 0) {
					draw = process_texture(filter);
					filter->processing_counter = 1;
				} else {
					filter->processing_counter++;
				}
			} else {
				draw = process_texture(filter);
			}
			filter->got_new_frame = false;
		}

		if (draw) {
			draw_greenscreen_blur(filter, has_blur);
			filter->processed_frame = true;
		}
	} else {
		obs_source_skip_video_filter(filter->context);
	}
	UNUSED_PARAMETER(effect);
}

static void nv_greenscreen_filter_render(void *data, gs_effect_t *effect)
{
	nvvfx_filter_render(data, effect, false);
}

static void nv_blur_filter_render(void *data, gs_effect_t *effect)
{
	nvvfx_filter_render(data, effect, true);
}

static enum gs_color_space nvvfx_filter_get_color_space(void *data, size_t count,
							const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct nvvfx_data *const filter = data;
	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	enum gs_color_space space = source_space;
	for (size_t i = 0; i < count; ++i) {
		space = preferred_spaces[i];
		if (space == source_space)
			break;
	}

	return space;
}

static obs_properties_t *nvvfx_filter_properties(void *data)
{
	struct nvvfx_data *filter = (struct nvvfx_data *)data;
	obs_properties_t *props = obs_properties_create();
	if (filter->filter_id != S_FX_AIGS) {
		obs_property_t *strength =
			obs_properties_add_float_slider(props, S_STRENGTH, TEXT_MODE_BLUR_STRENGTH, 0, 1, 0.05);
	} else {
		obs_property_t *mode =
			obs_properties_add_list(props, S_MODE, TEXT_MODE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_list_add_int(mode, TEXT_MODE_QUALITY, S_MODE_QUALITY);
		obs_property_list_add_int(mode, TEXT_MODE_PERF, S_MODE_PERF);
		obs_property_t *threshold =
			obs_properties_add_float_slider(props, S_THRESHOLDFX, TEXT_MODE_THRESHOLD, 0, 1, 0.05);
		obs_property_t *partial = obs_properties_add_int_slider(props, S_PROCESSING, TEXT_PROCESSING, 1, 4, 1);
		obs_property_set_long_description(partial, TEXT_PROCESSING_HINT);
	}
	unsigned int version = get_lib_version();
	if (version && version < MIN_VFX_SDK_VERSION) {
		obs_property_t *warning = obs_properties_add_text(props, "deprecation", NULL, OBS_TEXT_INFO);
		obs_property_text_set_info_type(warning, OBS_TEXT_INFO_WARNING);
		obs_property_set_long_description(warning, TEXT_DEPRECATION);
	}

	return props;
}

static void nvvfx_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_MODE, S_MODE_QUALITY);
	obs_data_set_default_double(settings, S_THRESHOLDFX, S_THRESHOLDFX_DEFAULT);
	obs_data_set_default_int(settings, S_PROCESSING, 1);
	obs_data_set_default_double(settings, S_STRENGTH, S_STRENGTH_DEFAULT);
}

bool load_nvidia_vfx(void)
{
	bool old_sdk_loaded = false;
	unsigned int version = get_lib_version();
	uint8_t major = (version >> 24) & 0xff;
	uint8_t minor = (version >> 16) & 0x00ff;
	uint8_t build = (version >> 8) & 0x0000ff;
	uint8_t revision = (version >> 0) & 0x000000ff;
	if (version) {
		blog(LOG_INFO, "[NVIDIA VIDEO FX]: NVIDIA VIDEO FX version: %i.%i.%i.%i", major, minor, build,
		     revision);
		if (version < MIN_VFX_SDK_VERSION) {
			blog(LOG_INFO,
			     "[NVIDIA VIDEO FX]: NVIDIA VIDEO Effects SDK is outdated. Please update both audio & video SDK.");
		}
	}
	if (!load_nv_vfx_libs()) {
		blog(LOG_INFO, "[NVIDIA VIDEO FX]: FX disabled, redistributable not found or could not be loaded.");
		return false;
	}

#define LOAD_SYM_FROM_LIB(sym, lib, dll)                                                                 \
	if (!(sym = (sym##_t)GetProcAddress(lib, #sym))) {                                               \
		DWORD err = GetLastError();                                                              \
		printf("[NVIDIA VIDEO FX]: Couldn't load " #sym " from " dll ": %lu (0x%lx)", err, err); \
		release_nv_vfx();                                                                        \
		goto unload_everything;                                                                  \
	}

#define LOAD_SYM_FROM_LIB2(sym, lib, dll)                                                                \
	if (!(sym = (sym##_t)GetProcAddress(lib, #sym))) {                                               \
		DWORD err = GetLastError();                                                              \
		printf("[NVIDIA VIDEO FX]: Couldn't load " #sym " from " dll ": %lu (0x%lx)", err, err); \
		nvvfx_new_sdk = false;                                                                   \
	} else {                                                                                         \
		nvvfx_new_sdk = true;                                                                    \
	}

#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB(sym, nv_videofx, "NVVideoEffects.dll")
	LOAD_SYM(NvVFX_GetVersion);
	LOAD_SYM(NvVFX_CreateEffect);
	LOAD_SYM(NvVFX_DestroyEffect);
	LOAD_SYM(NvVFX_SetU32);
	LOAD_SYM(NvVFX_SetS32);
	LOAD_SYM(NvVFX_SetF32);
	LOAD_SYM(NvVFX_SetF64);
	LOAD_SYM(NvVFX_SetU64);
	LOAD_SYM(NvVFX_SetObject);
	LOAD_SYM(NvVFX_SetCudaStream);
	LOAD_SYM(NvVFX_SetImage);
	LOAD_SYM(NvVFX_SetString);
	LOAD_SYM(NvVFX_GetU32);
	LOAD_SYM(NvVFX_GetS32);
	LOAD_SYM(NvVFX_GetF32);
	LOAD_SYM(NvVFX_GetF64);
	LOAD_SYM(NvVFX_GetU64);
	LOAD_SYM(NvVFX_GetObject);
	LOAD_SYM(NvVFX_GetCudaStream);
	LOAD_SYM(NvVFX_GetImage);
	LOAD_SYM(NvVFX_GetString);
	LOAD_SYM(NvVFX_Run);
	LOAD_SYM(NvVFX_Load);
	LOAD_SYM(NvVFX_CudaStreamCreate);
	LOAD_SYM(NvVFX_CudaStreamDestroy);
	old_sdk_loaded = true;
#undef LOAD_SYM

#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB(sym, nv_cvimage, "NVCVImage.dll")
	LOAD_SYM(NvCV_GetErrorStringFromCode);
	LOAD_SYM(NvCVImage_Init);
	LOAD_SYM(NvCVImage_InitView);
	LOAD_SYM(NvCVImage_Alloc);
	LOAD_SYM(NvCVImage_Realloc);
	LOAD_SYM(NvCVImage_Dealloc);
	LOAD_SYM(NvCVImage_Create);
	LOAD_SYM(NvCVImage_Destroy);
	LOAD_SYM(NvCVImage_ComponentOffsets);
	LOAD_SYM(NvCVImage_Transfer);
	LOAD_SYM(NvCVImage_TransferRect);
	LOAD_SYM(NvCVImage_TransferFromYUV);
	LOAD_SYM(NvCVImage_TransferToYUV);
	LOAD_SYM(NvCVImage_MapResource);
	LOAD_SYM(NvCVImage_UnmapResource);
	LOAD_SYM(NvCVImage_Composite);
	LOAD_SYM(NvCVImage_CompositeRect);
	LOAD_SYM(NvCVImage_CompositeOverConstant);
	LOAD_SYM(NvCVImage_FlipY);
	LOAD_SYM(NvCVImage_GetYUVPointers);
	LOAD_SYM(NvCVImage_InitFromD3D11Texture);
	LOAD_SYM(NvCVImage_ToD3DFormat);
	LOAD_SYM(NvCVImage_FromD3DFormat);
	LOAD_SYM(NvCVImage_ToD3DColorSpace);
	LOAD_SYM(NvCVImage_FromD3DColorSpace);
#undef LOAD_SYM

#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB2(sym, nv_videofx, "NVVideoEffects.dll")
	LOAD_SYM(NvVFX_SetStateObjectHandleArray);
	LOAD_SYM(NvVFX_AllocateState);
	LOAD_SYM(NvVFX_DeallocateState);
	LOAD_SYM(NvVFX_ResetState);
	if (!nvvfx_new_sdk) {
		blog(LOG_INFO, "[NVIDIA VIDEO FX]: sdk loaded but old redistributable detected; please upgrade.");
	}
#undef LOAD_SYM

	int err;
	NvVFX_Handle h = NULL;

	/* load the effect to check if the GPU is supported */
	err = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &h);
	if (err != NVCV_SUCCESS) {
		if (err == NVCV_ERR_UNSUPPORTEDGPU) {
			blog(LOG_INFO, "[NVIDIA VIDEO FX]: disabled, unsupported GPU");
		} else {
			blog(LOG_ERROR, "[NVIDIA VIDEO FX]: disabled, error %i", err);
		}
		goto unload_everything;
	}
	NvVFX_DestroyEffect(h);
	nvvfx_loaded = true;
	blog(LOG_INFO, "[NVIDIA VIDEO FX]: enabled, redistributable found");
	return true;

unload_everything:
	nvvfx_loaded = false;
	blog(LOG_INFO, "[NVIDIA VIDEO FX]: disabled, redistributable not found");
	release_nv_vfx();
	return false;
}

void unload_nvidia_vfx(void)
{
	release_nv_vfx();
}

struct obs_source_info nvidia_greenscreen_filter_info = {
	.id = "nv_greenscreen_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = nv_greenscreen_filter_name,
	.create = nv_greenscreen_filter_create,
	.destroy = nvvfx_filter_destroy,
	.get_defaults = nvvfx_filter_defaults,
	.get_properties = nvvfx_filter_properties,
	.update = nvvfx_filter_update,
	.filter_video = nvvfx_filter_video,
	.video_render = nv_greenscreen_filter_render,
	.video_tick = nvvfx_filter_tick,
	.video_get_color_space = nvvfx_filter_get_color_space,
};

struct obs_source_info nvidia_blur_filter_info = {
	.id = "nv_blur_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = nv_blur_filter_name,
	.create = nv_blur_filter_create,
	.destroy = nvvfx_filter_destroy,
	.get_defaults = nvvfx_filter_defaults,
	.get_properties = nvvfx_filter_properties,
	.update = nvvfx_filter_update,
	.filter_video = nvvfx_filter_video,
	.video_render = nv_blur_filter_render,
	.video_tick = nvvfx_filter_tick,
	.video_get_color_space = nvvfx_filter_get_color_space,
};

struct obs_source_info nvidia_background_blur_filter_info = {
	.id = "nv_background_blur_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = nv_background_blur_filter_name,
	.create = nv_background_blur_filter_create,
	.destroy = nvvfx_filter_destroy,
	.get_defaults = nvvfx_filter_defaults,
	.get_properties = nvvfx_filter_properties,
	.update = nvvfx_filter_update,
	.filter_video = nvvfx_filter_video,
	.video_render = nv_blur_filter_render,
	.video_tick = nvvfx_filter_tick,
	.video_get_color_space = nvvfx_filter_get_color_space,
};
