#include <obs-module.h>
#include <util/threading.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include "nvvfx-load.h"
/* -------------------------------------------------------- */

#define do_log(level, format, ...)                                             \
	blog(level,                                                            \
	     "[NVIDIA RTX AI Greenscreen (Background removal): '%s'] " format, \
	     obs_source_get_name(filter->context), ##__VA_ARGS__)

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

#define MT_ obs_module_text
#define TEXT_MODE MT_("Greenscreen.Mode")
#define TEXT_MODE_QUALITY MT_("Greenscreen.Quality")
#define TEXT_MODE_PERF MT_("Greenscreen.Performance")
#define TEXT_MODE_THRESHOLD MT_("Greenscreen.Threshold")

bool nvvfx_loaded = false;
struct nv_greenscreen_data {
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
	CUstream stream;        // CUDA stream
	int mode;               // 0 = quality, 1 = performance
	NvCVImage *src_img;     // src img in obs format (RGBA ?) on GPU
	NvCVImage *BGR_src_img; // src img in BGR on GPU
	NvCVImage *A_dst_img;   // mask img on GPU
	NvCVImage *dst_img;     // mask texture
	NvCVImage *stage;       // planar stage img used for transfer to texture

	/* alpha mask effect */
	gs_effect_t *effect;
	gs_texrender_t *render;
	gs_texture_t *render_unorm;
	gs_texture_t *alpha_texture;
	uint32_t width;  // width of texture
	uint32_t height; // height of texture
	gs_eparam_t *mask_param;
	gs_eparam_t *src_param;
	gs_eparam_t *threshold_param;
	float threshold;
};

static const char *nv_greenscreen_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("NvidiaGreenscreenFilter");
}

static void nv_greenscreen_filter_update(void *data, obs_data_t *settings)
{
	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;
	NvCV_Status vfxErr;
	int mode = (int)obs_data_get_int(settings, S_MODE);
	if (filter->mode != mode) {
		filter->mode = mode;
		vfxErr = NvVFX_SetU32(filter->handle, NVVFX_MODE, mode);
		vfxErr = NvVFX_Load(filter->handle);
		if (NVCV_SUCCESS != vfxErr)
			error("Error loading AI Greenscreen FX %i", vfxErr);
	}
	filter->threshold = (float)obs_data_get_double(settings, S_THRESHOLDFX);
}

static void nv_greenscreen_filter_actual_destroy(void *data)
{
	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;
	if (!nvvfx_loaded) {
		bfree(filter);
		return;
	}

	os_atomic_set_bool(&filter->processing_stop, true);

	if (filter->images_allocated) {
		obs_enter_graphics();
		gs_texture_destroy(filter->alpha_texture);
		gs_texrender_destroy(filter->render);
		gs_texture_destroy(filter->render_unorm);
		obs_leave_graphics();
		NvCVImage_Destroy(filter->src_img);
		NvCVImage_Destroy(filter->BGR_src_img);
		NvCVImage_Destroy(filter->A_dst_img);
		NvCVImage_Destroy(filter->dst_img);
		NvCVImage_Destroy(filter->stage);
	}
	if (filter->stream) {
		NvVFX_CudaStreamDestroy(filter->stream);
	}
	if (filter->handle) {
		NvVFX_DestroyEffect(filter->handle);
	}
	if (filter->effect) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(filter);
}

static void nv_greenscreen_filter_destroy(void *data)
{
	obs_queue_task(OBS_TASK_GRAPHICS, nv_greenscreen_filter_actual_destroy,
		       data, false);
}

static void nv_greenscreen_filter_reset(void *data, calldata_t *calldata)
{
	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;
	NvCV_Status vfxErr;

	os_atomic_set_bool(&filter->processing_stop, true);
	// first destroy
	if (filter->stream) {
		NvVFX_CudaStreamDestroy(filter->stream);
	}
	if (filter->handle) {
		NvVFX_DestroyEffect(filter->handle);
	}
	// recreate
	/* 1. Create FX */
	vfxErr = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &filter->handle);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error recreating AI Greenscreen FX; error %i: %s",
		      vfxErr, errString);
		nv_greenscreen_filter_destroy(filter);
	}

	/* 2. Set models path & initialize CudaStream */
	char buffer[MAX_PATH];
	char modelDir[MAX_PATH];
	nvvfx_get_sdk_path(buffer, MAX_PATH);
	size_t max_len = sizeof(buffer) / sizeof(char);
	snprintf(modelDir, max_len, "%s\\models", buffer);
	vfxErr = NvVFX_SetString(filter->handle, NVVFX_MODEL_DIRECTORY,
				 modelDir);
	vfxErr = NvVFX_CudaStreamCreate(&filter->stream);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error creating CUDA Stream; error %i: %s", vfxErr,
		      errString);
		nv_greenscreen_filter_destroy(filter);
	}
	vfxErr = NvVFX_SetCudaStream(filter->handle, NVVFX_CUDA_STREAM,
				     filter->stream);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error setting CUDA Stream %i", vfxErr);
		nv_greenscreen_filter_destroy(filter);
	}

	/* 3. load FX */
	vfxErr = NvVFX_SetU32(filter->handle, NVVFX_MODE, filter->mode);
	vfxErr = NvVFX_Load(filter->handle);
	if (NVCV_SUCCESS != vfxErr)
		error("Error loading AI Greenscreen FX %i", vfxErr);

	filter->images_allocated = false;
	os_atomic_set_bool(&filter->processing_stop, false);
}

static void init_images_greenscreen(struct nv_greenscreen_data *filter)
{
	NvCV_Status vfxErr;
	uint32_t width = filter->width;
	uint32_t height = filter->height;

	/* 1. create alpha texture */
	if (filter->alpha_texture) {
		gs_texture_destroy(filter->alpha_texture);
	}
	filter->alpha_texture =
		gs_texture_create(width, height, GS_A8, 1, NULL, 0);
	if (filter->alpha_texture == NULL) {
		error("Alpha texture couldn't be created");
		goto fail;
	}
	struct ID3D11Texture2D *d11texture =
		(struct ID3D11Texture2D *)gs_texture_get_obj(
			filter->alpha_texture);

	/* 2. Create NvCVImage which will hold final alpha texture. */
	if (!filter->dst_img &&
	    (NvCVImage_Create(width, height, NVCV_A, NVCV_U8, NVCV_CHUNKY,
			      NVCV_GPU, 1, &filter->dst_img) != NVCV_SUCCESS)) {
		goto fail;
	}

	vfxErr = NvCVImage_InitFromD3D11Texture(filter->dst_img, d11texture);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error passing dst ID3D11Texture to img; error %i: %s",
		      vfxErr, errString);
		goto fail;
	}

	/* 3. create texrender */
	if (filter->render)
		gs_texrender_destroy(filter->render);
	filter->render = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	if (!filter->render) {
		error("Failed to create render texrenderer", vfxErr);
		goto fail;
	}
	if (filter->render_unorm)
		gs_texture_destroy(filter->render_unorm);
	filter->render_unorm =
		gs_texture_create(width, height, GS_BGRA_UNORM, 1, NULL, 0);
	if (!filter->render_unorm) {
		error("Failed to create render_unorm texture", vfxErr);
		goto fail;
	}

	/* 4. Create and allocate BGR NvCVImage (fx src). */
	if (filter->BGR_src_img) {
		if (NvCVImage_Realloc(filter->BGR_src_img, width, height,
				      NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU,
				      1) != NVCV_SUCCESS) {
			goto fail;
		}
	} else {
		if (NvCVImage_Create(width, height, NVCV_BGR, NVCV_U8,
				     NVCV_CHUNKY, NVCV_GPU, 1,
				     &filter->BGR_src_img) != NVCV_SUCCESS) {
			goto fail;
		}
		if (NvCVImage_Alloc(filter->BGR_src_img, width, height,
				    NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU,
				    1) != NVCV_SUCCESS) {
			goto fail;
		}
	}

	/* 5. Create and allocate Alpha NvCVimage (fx dst). */
	if (filter->A_dst_img) {
		if (NvCVImage_Realloc(filter->A_dst_img, width, height, NVCV_A,
				      NVCV_U8, NVCV_CHUNKY, NVCV_GPU,
				      1) != NVCV_SUCCESS) {
			goto fail;
		}
	} else {
		if (NvCVImage_Create(width, height, NVCV_A, NVCV_U8,
				     NVCV_CHUNKY, NVCV_GPU, 1,
				     &filter->A_dst_img) != NVCV_SUCCESS) {
			goto fail;
		}
		if (NvCVImage_Alloc(filter->A_dst_img, width, height, NVCV_A,
				    NVCV_U8, NVCV_CHUNKY, NVCV_GPU,
				    1) != NVCV_SUCCESS) {
			goto fail;
		}
	}

	/* 6. Create stage NvCVImage which will be used as buffer for transfer */
	if (filter->stage) {
		if (NvCVImage_Realloc(filter->stage, width, height, NVCV_RGBA,
				      NVCV_U8, NVCV_PLANAR, NVCV_GPU,
				      1) != NVCV_SUCCESS) {
			goto fail;
		}
	} else {
		if (NvCVImage_Create(width, height, NVCV_RGBA, NVCV_U8,
				     NVCV_PLANAR, NVCV_GPU, 1,
				     &filter->stage) != NVCV_SUCCESS) {
			goto fail;
		}
		if (NvCVImage_Alloc(filter->stage, width, height, NVCV_RGBA,
				    NVCV_U8, NVCV_PLANAR, NVCV_GPU,
				    1) != NVCV_SUCCESS) {
			goto fail;
		}
	}

	/* 7. Set input & output images for nv FX. */
	if (NvVFX_SetImage(filter->handle, NVVFX_INPUT_IMAGE,
			   filter->BGR_src_img) != NVCV_SUCCESS) {
		goto fail;
	}
	if (NvVFX_SetImage(filter->handle, NVVFX_OUTPUT_IMAGE,
			   filter->A_dst_img) != NVCV_SUCCESS) {
		goto fail;
	}

	filter->images_allocated = true;
	return;
fail:
	error("Error during allocation of images");
	os_atomic_set_bool(&filter->processing_stop, true);
	return;
}

static bool process_texture_greenscreen(struct nv_greenscreen_data *filter)
{
	/* 1. Map src img holding texture. */
	NvCV_Status vfxErr =
		NvCVImage_MapResource(filter->src_img, filter->stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error mapping resource for source texture; error %i : %s",
		      vfxErr, errString);
		goto fail;
	}

	/* 2. Convert to BGR. */
	vfxErr = NvCVImage_Transfer(filter->src_img, filter->BGR_src_img, 1.0f,
				    filter->stream, filter->stage);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error converting src to BGR img; error %i: %s", vfxErr,
		      errString);
		goto fail;
	}
	vfxErr = NvCVImage_UnmapResource(filter->src_img, filter->stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error unmapping resource for src texture; error %i: %s",
		      vfxErr, errString);
		goto fail;
	}

	/*  3. run RTX fx */
	vfxErr = NvVFX_Run(filter->handle, 1);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error running the FX; error %i: %s", vfxErr, errString);
		if (vfxErr == NVCV_ERR_CUDA)
			nv_greenscreen_filter_reset(filter, NULL);
	}

	/* 4. Map dst texture before transfer from dst img provided by FX */
	vfxErr = NvCVImage_MapResource(filter->dst_img, filter->stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error mapping resource for dst texture; error %i: %s",
		      vfxErr, errString);
		goto fail;
	}

	vfxErr = NvCVImage_Transfer(filter->A_dst_img, filter->dst_img, 1.0f,
				    filter->stream, filter->stage);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error transferring mask to alpha texture; error %i: %s ",
		      vfxErr, errString);
		goto fail;
	}

	vfxErr = NvCVImage_UnmapResource(filter->dst_img, filter->stream);
	if (vfxErr != NVCV_SUCCESS) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error unmapping resource for dst texture; error %i: %s",
		      vfxErr, errString);
		goto fail;
	}

	return true;
fail:
	os_atomic_set_bool(&filter->processing_stop, true);
	return false;
}

static void *nv_greenscreen_filter_create(obs_data_t *settings,
					  obs_source_t *context)
{
	struct nv_greenscreen_data *filter =
		(struct nv_greenscreen_data *)bzalloc(sizeof(*filter));
	if (!nvvfx_loaded) {
		nv_greenscreen_filter_destroy(filter);
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

	/* 1. Create FX */
	vfxErr = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &filter->handle);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error creating AI Greenscreen FX; error %i: %s", vfxErr,
		      errString);
		nv_greenscreen_filter_destroy(filter);
		return NULL;
	}

	/* 2. Set models path & initialize CudaStream */
	char buffer[MAX_PATH];
	char modelDir[MAX_PATH];
	nvvfx_get_sdk_path(buffer, MAX_PATH);
	size_t max_len = sizeof(buffer) / sizeof(char);
	snprintf(modelDir, max_len, "%s\\models", buffer);
	vfxErr = NvVFX_SetString(filter->handle, NVVFX_MODEL_DIRECTORY,
				 modelDir);
	vfxErr = NvVFX_CudaStreamCreate(&filter->stream);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error creating CUDA Stream; error %i: %s", vfxErr,
		      errString);
		nv_greenscreen_filter_destroy(filter);
		return NULL;
	}
	vfxErr = NvVFX_SetCudaStream(filter->handle, NVVFX_CUDA_STREAM,
				     filter->stream);
	if (NVCV_SUCCESS != vfxErr) {
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
		error("Error setting CUDA Stream %i", vfxErr);
		nv_greenscreen_filter_destroy(filter);
		return NULL;
	}
	/* log sdk version */
	unsigned int version;
	if (NvVFX_GetVersion(&version) == NVCV_SUCCESS) {
		uint8_t major = (version >> 24) & 0xff;
		uint8_t minor = (version >> 16) & 0x00ff;
		uint8_t build = (version >> 8) & 0x0000ff;
		info("RTX VIDEO FX version: %i.%i.%i", major, minor, build);
	}

	/* 3. Load alpha mask effect. */
	char *effect_path = obs_module_file("rtx_greenscreen.effect");

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, NULL);
	bfree(effect_path);
	if (filter->effect) {
		filter->mask_param =
			gs_effect_get_param_by_name(filter->effect, "mask");
		filter->src_param =
			gs_effect_get_param_by_name(filter->effect, "image");
		filter->threshold_param = gs_effect_get_param_by_name(
			filter->effect, "threshold");
	}
	obs_leave_graphics();

	if (!filter->effect) {
		nv_greenscreen_filter_destroy(filter);
		return NULL;
	}

	/*---------------------------------------- */

	nv_greenscreen_filter_update(filter, settings);

	return filter;
}

static obs_properties_t *nv_greenscreen_filter_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	obs_property_t *mode = obs_properties_add_list(props, S_MODE, TEXT_MODE,
						       OBS_COMBO_TYPE_LIST,
						       OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(mode, TEXT_MODE_QUALITY, S_MODE_QUALITY);
	obs_property_list_add_int(mode, TEXT_MODE_PERF, S_MODE_PERF);
	obs_property_t *threshold = obs_properties_add_float_slider(
		props, S_THRESHOLDFX, TEXT_MODE_THRESHOLD, 0, 1, 0.05);

	UNUSED_PARAMETER(data);
	return props;
}

static void nv_greenscreen_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_MODE, S_MODE_QUALITY);
	obs_data_set_default_double(settings, S_THRESHOLDFX,
				    S_THRESHOLDFX_DEFAULT);
}
static struct obs_source_frame *
nv_greenscreen_filter_video(void *data, struct obs_source_frame *frame)
{
	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;
	filter->got_new_frame = true;
	return frame;
}

static void nv_greenscreen_filter_tick(void *data, float t)
{
	UNUSED_PARAMETER(t);

	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;

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
	filter->target_valid = cx >= 512 && cy >= 288;
	if (!filter->target_valid) {
		error("Size must be larger than (512,288)");
		return;
	}
	if (cx != filter->width && cy != filter->height) {
		filter->images_allocated = false;
		filter->width = cx;
		filter->height = cy;
	}
	if (!filter->images_allocated) {
		obs_enter_graphics();
		init_images_greenscreen(filter);
		obs_leave_graphics();
		filter->initial_render = false;
	}

	filter->processed_frame = false;
}

static void draw_greenscreen(struct nv_greenscreen_data *filter)
{
	/* Render alpha mask */
	if (obs_source_process_filter_begin(filter->context, GS_RGBA,
					    OBS_ALLOW_DIRECT_RENDERING)) {
		gs_effect_set_texture(filter->mask_param,
				      filter->alpha_texture);
		gs_effect_set_texture_srgb(
			filter->src_param,
			gs_texrender_get_texture(filter->render));
		gs_effect_set_float(filter->threshold_param, filter->threshold);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		obs_source_process_filter_end(filter->context, filter->effect,
					      0, 0);

		gs_blend_state_pop();
	}
}

static void nv_greenscreen_filter_render(void *data, gs_effect_t *effect)
{
	NvCV_Status vfxErr;
	struct nv_greenscreen_data *filter = (struct nv_greenscreen_data *)data;

	if (filter->processing_stop) {
		obs_source_skip_video_filter(filter->context);
		return;
	}
	obs_source_t *target = obs_filter_get_target(filter->context);
	obs_source_t *parent = obs_filter_get_parent(filter->context);

	/* Skip if processing of a frame hasn't yet started */
	if (!filter->target_valid || !target || !parent) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	/* Render processed image from earlier in the frame */
	if (filter->processed_frame) {
		draw_greenscreen(filter);
		return;
	}

	if (parent && !filter->handler) {
		filter->handler = obs_source_get_signal_handler(parent);
		signal_handler_connect(filter->handler, "update_properties",
				       nv_greenscreen_filter_reset, filter);
	}
	/* 1. Render to retrieve texture. */
	gs_texrender_t *const render = filter->render;
	if (!render) {
		obs_source_skip_video_filter(filter->context);
		return;
	}
	uint32_t target_flags = obs_source_get_output_flags(target);
	uint32_t parent_flags = obs_source_get_output_flags(parent);

	bool custom_draw = (target_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
	bool async = (target_flags & OBS_SOURCE_ASYNC) != 0;

	gs_texrender_reset(render);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	if (gs_texrender_begin(render, filter->width, filter->height)) {
		struct vec4 clear_color;

		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
		gs_ortho(0.0f, (float)filter->width, 0.0f,
			 (float)filter->height, -100.0f, 100.0f);

		if (target == parent && !custom_draw && !async)
			obs_source_default_render(target);
		else
			obs_source_video_render(target);

		gs_texrender_end(render);

		gs_copy_texture(filter->render_unorm,
				gs_texrender_get_texture(filter->render));
	}
	gs_blend_state_pop();

	/* 2. Initialize src_texture (only at startup or reset) */
	if (!filter->initial_render) {
		struct ID3D11Texture2D *d11texture2 =
			(struct ID3D11Texture2D *)gs_texture_get_obj(
				filter->render_unorm);
		if (!d11texture2) {
			error("Couldn't retrieve d3d11texture2d.");
			return;
		}

		if (!filter->src_img) {
			vfxErr = NvCVImage_Create(filter->width, filter->height,
						  NVCV_BGRA, NVCV_U8,
						  NVCV_CHUNKY, NVCV_GPU, 1,
						  &filter->src_img);
			if (vfxErr != NVCV_SUCCESS) {
				const char *errString =
					NvCV_GetErrorStringFromCode(vfxErr);
				error("Error creating src img; error %i: %s",
				      vfxErr, errString);
				os_atomic_set_bool(&filter->processing_stop,
						   true);
				return;
			}
		}

		vfxErr = NvCVImage_InitFromD3D11Texture(filter->src_img,
							d11texture2);
		if (vfxErr != NVCV_SUCCESS) {
			const char *errString =
				NvCV_GetErrorStringFromCode(vfxErr);
			error("Error passing src ID3D11Texture to img; error %i: %s",
			      vfxErr, errString);
			os_atomic_set_bool(&filter->processing_stop, true);
			return;
		}

		filter->initial_render = true;
	}

	/* 3. Process FX (outputs a mask) & draw. */
	if (filter->initial_render && filter->images_allocated) {
		bool draw = true;
		if (!async || filter->got_new_frame) {
			draw = process_texture_greenscreen(filter);
			filter->got_new_frame = false;
		}

		if (draw) {
			draw_greenscreen(filter);
			filter->processed_frame = true;
		}
	} else {
		obs_source_skip_video_filter(filter->context);
	}
	UNUSED_PARAMETER(effect);
}

bool load_nvvfx(void)
{
	if (!load_nv_vfx_libs()) {
		blog(LOG_INFO,
		     "[NVIDIA RTX VIDEO FX]: FX disabled, redistributable not found.");
		return false;
	}

#define LOAD_SYM_FROM_LIB(sym, lib, dll)                            \
	if (!(sym = (sym##_t)GetProcAddress(lib, #sym))) {          \
		DWORD err = GetLastError();                         \
		printf("[NVIDIA RTX VIDEO FX]: Couldn't load " #sym \
		       " from " dll ": %lu (0x%lx)",                \
		       err, err);                                   \
		release_nv_vfx();                                   \
		goto unload_everything;                             \
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
#define LOAD_SYM(sym) LOAD_SYM_FROM_LIB(sym, nv_cudart, "cudart64_110.dll")
	LOAD_SYM(cudaMalloc);
	LOAD_SYM(cudaStreamSynchronize);
	LOAD_SYM(cudaFree);
	LOAD_SYM(cudaMemcpy);
	LOAD_SYM(cudaMemsetAsync);
#undef LOAD_SYM
	int err;
	NvVFX_Handle h = NULL;

	/* load the effect to check if the GPU is supported */
	err = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &h);
	if (err != NVCV_SUCCESS) {
		if (err == NVCV_ERR_UNSUPPORTEDGPU) {
			blog(LOG_INFO,
			     "[NVIDIA RTX VIDEO FX]: disabled, unsupported GPU");
		} else {
			blog(LOG_ERROR,
			     "[NVIDIA RTX VIDEO FX]: disabled, error %i", err);
		}
		goto unload_everything;
	}
	NvVFX_DestroyEffect(h);
	nvvfx_loaded = true;
	blog(LOG_INFO, "[NVIDIA RTX VIDEO FX]: enabled, redistributable found");
	return true;

unload_everything:
	nvvfx_loaded = false;
	release_nv_vfx();
	return false;
}

#ifdef LIBNVVFX_ENABLED
void unload_nvvfx(void)
{
	release_nv_vfx();
}
#endif

struct obs_source_info nvidia_greenscreen_filter_info = {
	.id = "nv_greenscreen_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = nv_greenscreen_filter_name,
	.create = nv_greenscreen_filter_create,
	.destroy = nv_greenscreen_filter_destroy,
	.get_defaults = nv_greenscreen_filter_defaults,
	.get_properties = nv_greenscreen_filter_properties,
	.update = nv_greenscreen_filter_update,
	.filter_video = nv_greenscreen_filter_video,
	.video_render = nv_greenscreen_filter_render,
	.video_tick = nv_greenscreen_filter_tick,
};
