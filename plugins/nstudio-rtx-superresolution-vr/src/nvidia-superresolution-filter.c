/*
obs-rtx_superresolution
Copyright (C) 2023 Ben Jolley

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>
#include <util/threading.h>
#ifdef _WIN32
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include "include/nvTransferD3D11.h"
#else
#include <GL/gl.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#endif
#ifdef _WIN32
#include <tchar.h>
#endif
#include <graphics/image-file.h>
#include "include/nvvfx.h"



#define do_log(level, format, ...) obs_log(level, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)

#if defined(_DEBUG) || defined(DEBUG)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)
#else
#define debug(format, ...)
#endif



/* -------------------------------------------------------- */



#define S_TYPE "type"
#define S_TYPE_NONE 0
#define S_TYPE_SR 1
#define S_TYPE_UP 2
#define S_TYPE_DEFAULT S_TYPE_NONE

#define S_ENABLE_AR "ar"

#ifndef _WIN32
#include <string.h>
#define strnlen_s(str, maxlen) strnlen(str, maxlen)
#endif

#ifndef _WIN32
#include <string.h>
#define strnlen_s(str, maxlen) strnlen(str, maxlen)
#endif

#ifndef _WIN32
#include <string.h>
#define strnlen_s(str, maxlen) strnlen(str, maxlen)
#endif
#define S_MODE_AR "ar_mode"
#define S_MODE_SR "sr_mode"
#define S_MODE_WEAK 0
#define S_MODE_STRONG 1
#define S_MODE_DEFAULT S_MODE_STRONG

#define S_SR_SCALE "srscale"
#define S_UP_SCALE "upscale"
#define S_SCALE_NONE 0
#define S_SCALE_133x 1
#define S_SCALE_15x 2
#define S_SCALE_2x 3
#define S_SCALE_3x 4
#define S_SCALE_4x 5
#define S_SCALE_N 6
#define S_SCALE_AR 0
#define S_SCALE_DEFAULT S_SCALE_15x

#define S_STRENGTH "strength"
#define S_STRENGTH_DEFAULT 0.4f

#define S_VALID_TARGET "target_valid"
#define S_FATAL_ERROR "error_fatal"
#define S_INVALID_ERROR "error_invalid"
#define S_INVALID_WARNING_AR "warning_ar"
#define S_INVALID_WARNING_SR "warning_sr"
#define S_PROPS_VERIFY "verify"

#define MT_ obs_module_text
#define TEXT_OBS_FILTER_NAME MT_("NVIDIASuperResolutionFilter")
#define TEXT_FILTER MT_("SuperResolution.Filter")
#define TEXT_FILTER_NONE MT_("SuperResolution.Filter.None")
#define TEXT_FILTER_SR MT_("SuperResolution.Filter.SuperRes")
#define TEXT_FILTER_UP MT_("SuperResolution.Filter.Upscaling")
#define TEXT_FILTER_DESC MT_("SuperResolution.Filter.Desc")
#define TEXT_SR_MODE MT_("SuperResolution.SRMode")
#define TEXT_SR_MODE_WEAK MT_("Standard (Mode 0)")
#define TEXT_SR_MODE_STRONG MT_("High Quality / Lossless (Mode 1)")
#define TEXT_UPSCALE_MODE_DESC MT_("SuperResoltuion.SRMode.Desc")
#define TEXT_AR MT_("SuperResolution.AR")
#define TEXT_AR_DESC MT_("SuperResolution.ARDesc")
#define TEXT_AR_MODE MT_("SuperResolution.ARMode")
#define TEXT_AR_MODE_WEAK MT_("SuperResolution.ARMode.Weak")
#define TEXT_AR_MODE_STRONG MT_("SuperResolution.ARMode.Strong")
#define TEXT_AR_MODE_DESC MT_("SuperResolution.ARMode.Desc")
#define TEXT_UP_STRENGTH MT_("SuperResolution.Strength")
#define TEXT_SCALE MT_("SuperResolution.Scale")
#define TEXT_SCALE_DESC MT_("SuperResolution.Scale.Desc")
#define TEXT_SRSCALE_SIZE_133x MT_("SuperResolution.SRScale.133")
#define TEXT_SRSCALE_SIZE_15x MT_("SuperResolution.SRScale.15")
#define TEXT_SRSCALE_SIZE_2x MT_("SuperResolution.SRScale.2")
#define TEXT_SRSCALE_SIZE_3x MT_("SuperResolution.SRScale.3")
#define TEXT_SRSCALE_SIZE_4x MT_("SuperResolution.SRScale.4")
#define TEXT_UPSCALE_SIZE_133x MT_("SuperResolution.Upscale.133")
#define TEXT_UPSCALE_SIZE_15x MT_("SuperResolution.Upscale.15")
#define TEXT_UPSCALE_SIZE_2x MT_("SuperResolution.Upscale.2")
#define TEXT_UPSCALE_SIZE_3x MT_("SuperResolution.Upscale.3")
#define TEXT_UPSCALE_SIZE_4x MT_("SuperResolution.Upscale.4")
#define TEXT_VALID_TARGET MT_("SuperResolution.Valid")
#define TEXT_FATAL_ERROR MT_("SuperResolution.FatalError")
#define TEXT_INVALID_ERROR MT_("SuperResolution.Invalid")
#define TEXT_INVALID_WARNING_AR MT_("SuperResolution.InvalidAR")
#define TEXT_INVALID_WARNING_SR MT_("SuperResolution.InvalidSR")
#define TEXT_BUTTON_VERIFY MT_("SuperResolution.Verify")


/* Set at module load time, checks to see if the NvVFX SDK is loaded, and what the users GPU and drivers supports */
/* Usable everywhere except load_nv_superresolution_filter */
static bool nvvfx_loaded = false;
static bool nvvfx_supports_ar = false;
static bool nvvfx_supports_sr = false;
static bool nvvfx_supports_up = false;

/* New strings for UI */
#define S_ENABLE_STITCH "enable_stitching"
#define S_STMAP_PATH "stmap_path"
#define S_CUSTOM_SHADER "custom_shader_path"
#define TEXT_ENABLE_STITCH MT_("Enable Stitching (VR180)")
#define TEXT_STMAP_PATH MT_("STMap Image Path (*.tiff, *.png)")
#define TEXT_CUSTOM_SHADER MT_("Custom Stitching Shader (*.effect) [Optional]")


/* while the filter allows for non 16:9 aspect ratios, these 16:9 values are used to validate input source sizes
* so even though a 4:3 source may be provided that has the same pixel count as a 16:9 source -
* if the resolution is outside these bounds it will be deemed invalid for processing
* see https://docs.nvidia.com/deeplearning/maxine/vfx-sdk-programming-guide/index.html#super-res-filter
*/
static const uint32_t nv_type_resolutions[S_SCALE_N][2][2] =
{
	{{160, 90}, {8192, 4320}}, // S_SCALE_NONE, Pass-Through / AR - ALLOW 8K
	{{160, 90}, {8192, 4320}}, // S_SCALE_133x - ALLOW 8K
	{{160, 90}, {8192, 4320}}, // S_SCALE_15x - ALLOW 8K
	{{160, 90}, {8192, 4320}}, // S_SCALE_2x - ALLOW 8K
	{{160, 90}, {1280, 720}},  // S_SCALE_3x
	{{160, 90}, {960, 540}}    // S_SCALE_4x
};



struct nv_superresolution_data
{
	/* OBS and other vars */
	volatile bool processing_stopped; // essentially a mutex to use, as a signal to stop processing of things outside of our block
																		// or to signal a catastrophic failure has occured
	obs_source_t *context;
	bool processed_frame;
	bool done_initial_render;
	bool is_target_valid;
	bool invalid_ar_size;
	bool invalid_sr_size;
	bool show_size_error;
	bool got_new_frame;
	signal_handler_t *handler;
	bool reload_ar_fx;
	bool reload_sr_fx;
	uint32_t target_width;	// The current real width of our source target, may be 0
	uint32_t target_height;	// The current real height of our source target, may be 0
	bool apply_ar;
	bool are_images_allocated;
	bool destroy_ar;
	bool destroy_sr;
	bool destroying;

	/* RTX SDK vars */
	unsigned int version;
	NvVFX_Handle sr_handle;
	NvVFX_Handle ar_handle;
	CUstream stream;	// CUDA stream
	int ar_mode;		// filter mode, should be one of S_MODE_AR
	int sr_mode;		// filter mode, should be one of S_MODE_SR
	int type;			// filter type, should be one of S_TYPE_
	int scale;			// sr_scale mode, should be one of S_SCALE_
	float strength;		// effect strength, only effects upscaling filter?

	/* OBS render buffers for NvVFX */
	NvCVImage *src_img; // src img in obs format (RGBA) on GPU pointing to a live d3d11 gs_texture used by obs
	NvCVImage *dst_img; // the final processed image, in obs format (RGBA)  pointing to a live d3d11 gs_texture used by obs

	/* Artifact Reduction Buffers in BGRf32 Planar format */
	NvCVImage *gpu_ar_src_img;
	NvCVImage *gpu_ar_dst_img;

	/* Super Resolution buffers in either BGRf32 Planar or Upscaling buffers in RGBAu8 Chunky format */
	NvCVImage *gpu_sr_src_img; // src img in appropriate filter format on GPU
	NvCVImage *gpu_sr_dst_img; // final processed image in appropriate filter format on gpu
	
	/* A staging buffer that is the maximal size for the selected filters to avoid allocations during transfers */
	NvCVImage *gpu_staging_img; // RGBAu8 Chunky if Upscaling only, BGRf32 otherwise

	/* Intermediate buffer between final destination image, and dst_img. Should be removed if nvidia ever fixes this issue.
	* Not used with Upscaling filter as this is only needed with BGRf32 -> D3D RGBAu8 transfers
	* See Table 4, Pixel Conversions https://docs.nvidia.com/deeplearning/maxine/nvcvimage-api-guide/index.html#nvcvimage-transfer__section_wgp_qtd_xpb
	* See this for a very basic and non-informational official response from 2021 as to why this is needed
	* https://forums.developer.nvidia.com/t/no-transfer-conversion-from-planar-ncv-bgr-nvcv-f32-to-dx11-textures/183964/2
	*/
	NvCVImage *gpu_dst_tmp_img; // RGBAu8 chunky Format

	/* upscaling effect vars */
	gs_effect_t *effect;
	gs_texrender_t *render;
	gs_texrender_t *render_unorm; // the converted RGBA U8 render of our source
	gs_texture_t *scaled_texture; // the final RGBA U8 processed texture of the filter
	uint32_t width;         // width of source
	uint32_t height;        // height of source
	uint32_t out_width;     // output width determined by filter
	uint32_t out_height;	// output height determined by filter
	enum gs_color_space space;
	gs_eparam_t *image_param;
	gs_eparam_t *upscaled_param;
	gs_eparam_t *multiplier_param;

    /* STMap Stitching vars */
    bool enable_stitching;
    char *stmap_path;
    char *custom_shader_path;
    gs_image_file_t stmap_image;
    gs_texture_t *stmap_texture;
    gs_effect_t *stitch_effect;
    gs_eparam_t *stitch_image_param;
    gs_eparam_t *stitch_remap_param;
    gs_texrender_t *stitch_render; 
    bool stmap_loaded;
    bool stitching_bound;

#ifndef _WIN32
    cudaGraphicsResource_t src_gl_cuda_res;
    cudaGraphicsResource_t dst_gl_cuda_res;
    bool src_gl_registered;
    bool dst_gl_registered;
    uint32_t last_src_tex_id;
    uint32_t last_dst_tex_id;
#endif
};



typedef struct img_create_params
{
	NvCVImage **buffer;
	uint32_t width, height;
	uint32_t width2, height2;
	uint32_t layout;
	uint32_t alignment;
	NvCVImage_PixelFormat pixel_fmt;
	NvCVImage_ComponentType comp_type;
} img_create_params_t;



#ifdef _WIN32
static void nv_sdk_path(TCHAR *buffer, size_t len)
{
	/* Currently hardcoded to find windows install directory, as that is the only supported OS supported by NvVFX */
	TCHAR path[MAX_PATH];

	// There can be multiple apps on the system,
	// some might include the SDK in the app package and
	// others might expect the SDK to be installed in Program Files
	GetEnvironmentVariable(TEXT("NV_VIDEO_EFFECTS_PATH"), path, MAX_PATH);

	if (_tcscmp(path, TEXT("USE_APP_PATH")))
	{
		// App has not set environment variable to "USE_APP_PATH"
		// So pick up the SDK dll and dependencies from Program Files
		GetEnvironmentVariable(TEXT("ProgramFiles"), path, MAX_PATH);
		_stprintf_s(buffer, len, TEXT("%s\\NVIDIA Corporation\\NVIDIA Video Effects\\"), path);
	}
    else {
        // Use local app path? Logic seems incomplete in original but keeping straight port
        _tcscpy_s(buffer, len, path);
    }
}

static void get_nvfx_sdk_path(char *buffer, size_t len)
{
	TCHAR tbuffer[MAX_PATH];
	TCHAR tmodelDir[MAX_PATH];

	nv_sdk_path(tbuffer, MAX_PATH);

	size_t max_len = sizeof(tbuffer) / sizeof(TCHAR);
	_snwprintf_s(tmodelDir, max_len, max_len, TEXT("%s\\models"), tbuffer);

	wcstombs_s(0, buffer, len, tmodelDir, MAX_PATH);
}
#else
#include <unistd.h>
#include <linux/limits.h>
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

static void nv_sdk_path(char *buffer, size_t len)
{
    const char* env_path = getenv("NV_VIDEO_EFFECTS_PATH");
    if (env_path) {
        snprintf(buffer, len, "%s", env_path);
    } else {
        // Fallback or error
        snprintf(buffer, len, "/usr/local/NVIDIA/VideoEffects");
    }
}

static void get_nvfx_sdk_path(char *buffer, size_t len)
{
    char sdk_path[MAX_PATH];
    nv_sdk_path(sdk_path, MAX_PATH);
    snprintf(buffer, len, "%s/models", sdk_path);
}
#endif



/*
* Scales the input dimensions by the given sr_scale enum, giving the output
* param sr_scale - sr_scale enum, should be one of S_SCALE_133x, S_SCALE_15x, S_SCALE_2x, S_SCALE_3x, S_SCALE_4x
* param in_x - input width
* param in_y - input height
* param out_x - scaled width output
* param out_y - scaled height output
*/
static inline void get_scale_factor(uint32_t s_scale, uint32_t in_x, uint32_t in_y, uint32_t *out_x, uint32_t *out_y)
{
	const float scale_133 = 4.0f / 3.0f;
	float scale = 1.0f;

	switch (s_scale)
	{
	case S_SCALE_133x:
		scale = scale_133;
		break;
	case S_SCALE_15x:
		scale = 1.5f;
		break;
	case S_SCALE_2x:
		scale = 2.0f;
		break;
	case S_SCALE_3x:
		scale = 3.0f;
		break;
	case S_SCALE_4x:
		scale = 4.0f;
		break;
	}

	*out_x = (uint32_t)(in_x * scale + 0.5f);
	*out_y = (uint32_t)(in_y * scale + 0.5f);
}




static inline bool validate_scaling_aspect(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
	return (x1 * y2) == (y1 * x2);
}



/*
* Scales the input dimensions by the given sr_scale enum, giving the output
* param sr_scale - sr_scale enum, should be one of S_SCALE_133x, S_SCALE_15x, S_SCALE_2x, S_SCALE_3x, S_SCALE_4x
* param in_x - input width
* param in_y - input height
* param out_x - scaled width output
* param out_y - scaled height output
* 
* return - True if the input resolution is valid and falls within the bounds defined by nVidia,
						and the output resolution is properly valid and falls within the bounds defined by nVidia
						and if the aspect ratio of the input and output resolutions matches
*/
static inline bool validate_source_size(uint32_t scale, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
	if (scale < 0 || scale >= S_SCALE_N)
		return false;

	uint32_t min_width = nv_type_resolutions[scale][0][0];
	uint32_t max_width = nv_type_resolutions[scale][1][0];
	uint32_t min_height = nv_type_resolutions[scale][0][1];
	uint32_t max_height = nv_type_resolutions[scale][1][1];

	return (x1 >= min_width && x1 <= max_width && y1 >= min_height && y1 <= max_height);
}



/*
* Properly destroys the supplied fx and images, and nulls them out.
* 
* param fx - the NvFX handle of the effect to destroy
* param src - the source image of the fx to destroy
* param dst - the destination image of the fx to destroy
*/
static void nv_destroy_fx_filter(NvVFX_Handle *fx, NvCVImage **src, NvCVImage **dst)
{
	if (src && *src)
	{
		debug("nv_destroy_fx_filter: destorying src %X", *src);
		NvCVImage_Destroy(*src);
		*src = NULL;
	}

	if (dst && *dst)
	{
		debug("nv_destroy_fx_filter: destorying dst %X", *dst);
		NvCVImage_Destroy(*dst);
		*dst = NULL;
	}

	if (fx && *fx)
	{
		debug("nv_destroy_fx_filter: destorying fx %X", *fx);
		NvVFX_DestroyEffect(*fx);
		*fx = NULL;
	}
}



/*
* The real destroy method, destroys and frees all memory we've allocated to the Fx filters and image buffers
* param data - The OBS supplied data, should be a pointer to our filter struct
* 
* NOTE: For no particular reason, this function can be called multiple times
*/ 
static void nv_superres_filter_actual_destroy(void *data)
{
	debug("nv_superres_filter_actual_destroy: entered");

	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;

	if (!nvvfx_loaded)
	{
		bfree(filter);
		return;
	}

	os_atomic_set_bool(&filter->processing_stopped, true);

	nv_destroy_fx_filter(&filter->ar_handle, &filter->gpu_ar_src_img, &filter->gpu_ar_dst_img);
	nv_destroy_fx_filter(&filter->sr_handle, &filter->gpu_sr_src_img, &filter->gpu_sr_dst_img);
	nv_destroy_fx_filter(NULL, &filter->src_img, &filter->dst_img);
	nv_destroy_fx_filter(NULL, &filter->gpu_dst_tmp_img, &filter->gpu_staging_img);

	if (filter->stream)
	{
		NvVFX_CudaStreamDestroy(filter->stream);
		filter->stream = NULL;
	}

	obs_enter_graphics();

	if (filter->scaled_texture)
	{
		gs_texture_destroy(filter->scaled_texture);
		filter->scaled_texture = NULL;
	}
	if (filter->render)
	{
		gs_texrender_destroy(filter->render);
		filter->render = NULL;
	}
	if (filter->render_unorm)
	{
		gs_texrender_destroy(filter->render_unorm);
		filter->render_unorm = NULL;
	}

	if (filter->effect)
	{
		gs_effect_destroy(filter->effect);
		filter->effect = NULL;
	}

    /* Destroy Stitching Resources */
    if (filter->stitch_effect) {
        gs_effect_destroy(filter->stitch_effect);
        filter->stitch_effect = NULL;
    }
    if (filter->stmap_texture) {
        gs_texture_destroy(filter->stmap_texture);
        filter->stmap_texture = NULL;
    }
    if (filter->stitch_render) {
        gs_texrender_destroy(filter->stitch_render);
        filter->stitch_render = NULL;
    }
    gs_image_file_free(&filter->stmap_image);
    bfree(filter->stmap_path);
    bfree(filter->custom_shader_path);

#ifndef _WIN32
    if (filter->src_gl_registered) {
        cudaGraphicsUnregisterResource(filter->src_gl_cuda_res);
        filter->src_gl_registered = false;
        filter->src_gl_cuda_res = NULL;
    }
    if (filter->dst_gl_registered) {
        cudaGraphicsUnregisterResource(filter->dst_gl_cuda_res);
        filter->dst_gl_registered = false;
        filter->dst_gl_cuda_res = NULL;
    }
#endif

	obs_leave_graphics();

	bfree(filter);

	debug("nv_superres_filter_actual_destroy: exiting");
}



static void set_nvcv_colorspace(NvCVImage *img, enum gs_color_space space)
{
    if (!img) return;
    
    switch (space) {
        // HDR Enums disabled for compatibility with older libis on Linux
        /*
        case GS_CS_2100_PQ:
        case GS_CS_2100_HLG:
            img->colorspace = NVCV_2020 | NVCV_VIDEO_RANGE; // HDR is typ. Rec.2020
            break;
        */
        case GS_CS_709_EXTENDED:
        case GS_CS_709_SCRGB:
            img->colorspace = NVCV_709 | NVCV_VIDEO_RANGE;
            break;
        case GS_CS_SRGB:
        case GS_CS_SRGB_16F:
        default:
            img->colorspace = NVCV_709 | NVCV_VIDEO_RANGE; // NvCV treats sRGB/709 similarly for primaries
            break;
    }
}


/**
 * Helper function to determine SR scale factorur filter to be destroyed through OBS's task queue
* param data - should be a pointer to our OBS filter struct
*/
static void nv_superres_filter_destroy(void *data)
{
	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;
	if (!filter->destroying)
	{
		filter->destroying = true;
		obs_queue_task(OBS_TASK_GRAPHICS, nv_superres_filter_actual_destroy, data, false);
	}
}



/* Macro shenanigans to deal with variadic arguments to the error */
#define kill_on_error(cnd, msg, filter, ...) {	\
	if (!cnd) \
	{\
		obs_log(LOG_ERROR, msg, ##__VA_ARGS__);	\
		os_atomic_set_bool(&filter->processing_stopped, true); \
		return false;	\
	}\
}

/* Check the value of vfxErr, if it's anything other than NVCV_SUCCESS this macro will
* log the error, set the processing_stopped flag on filter, and return false from whatever function it's in
*/
#define nv_error(vfxErr, msg, filter, destroy_filter, ...) {	\
	if (NVCV_SUCCESS != vfxErr)\
	{\
		filter = (struct nv_superresolution_data*)filter;			\
		const char *errString = NvCV_GetErrorStringFromCode(vfxErr);\
		obs_log(LOG_ERROR, msg, ##__VA_ARGS__);						\
		obs_log(LOG_ERROR, "NvVFX Error %i: %s", vfxErr, errString);\
		if (destroy_filter) {nv_superres_filter_destroy(filter);}	\
		else {os_atomic_set_bool(&filter->processing_stopped, true);}\
		return false;\
	}\
}

/* no return variation of the above macro */
#define nv_error_nr(vfxErr, msg, filter, destroy_filter, ...)\
{															\
	if (NVCV_SUCCESS != vfxErr) {                           \
		filter = (struct nv_superresolution_data *)filter;	\
		const char *errString =                             \
			NvCV_GetErrorStringFromCode(vfxErr);			\
		obs_log(LOG_ERROR, msg, ##__VA_ARGS__);             \
		obs_log(LOG_ERROR, "NvVFX Error %i: %s", vfxErr,    \
			errString);										\
		if (destroy_filter) {                               \
			nv_superres_filter_destroy(filter);				\
		} else {                                            \
			os_atomic_set_bool(								\
				&filter->processing_stopped, true);			\
		}                                                   \
	}                                                       \
}



/*
* initializes the Fx Handle with the given FX selector, and optionally set the model directory parameter for the given FX
* note: if the FX handle is initialized and exists, it will be destroyed and re-initizliaed
* 
* filter - Our OBS filter structure
* handle - the fx handle to initialize
* fx - the fx type to initialize, these are NV filter constants, prefixed with NVVFX_FX_
*/
static bool create_nvfx(struct nv_superresolution_data *filter, NvVFX_Handle *handle, NvVFX_EffectSelector fx)
{
	if (*handle)
	{
		NvVFX_DestroyEffect(*handle);
	}

	NvCV_Status vfxErr = NvVFX_CreateEffect(fx, handle);
	nv_error(vfxErr, "Error creating nVidia RTX Upscaling FX", filter, true);

	bool set_model_dir =
		(strncmp(fx, NVVFX_FX_ARTIFACT_REDUCTION, sizeof(NVVFX_FX_ARTIFACT_REDUCTION) / sizeof(char)) ==0) ||
		(strncmp(fx, NVVFX_FX_SUPER_RES, sizeof(NVVFX_FX_SUPER_RES) / sizeof(char)) == 0);

	if (set_model_dir)
	{
		char model_dir[MAX_PATH];
		get_nvfx_sdk_path(model_dir, MAX_PATH);

		vfxErr = NvVFX_SetString(*handle, NVVFX_MODEL_DIRECTORY, model_dir);
		nv_error(vfxErr, "Error seting Super Resolution model directory: [%s]", filter, true, model_dir);
	}

	vfxErr = NvVFX_SetCudaStream(*handle, NVVFX_CUDA_STREAM, filter->stream);
	nv_error(vfxErr, "Error seting Super Resolution CUDA stream", filter, true);

	return true;
}



/* Loads the AR NVFX filter effect. Ensures any necessary parameters have been set.
* 
* returns: False if there is any error, true otherwise
*/
static bool load_ar_fx(struct nv_superresolution_data *filter)
{
	debug("load_ar_fx: entering");

	NvCV_Status vfxErr = NvVFX_SetU32(filter->ar_handle, NVVFX_MODE, filter->ar_mode);
	nv_error_nr(vfxErr, "Failed to set AR mode", filter, false);

	vfxErr = NvVFX_SetImage(filter->ar_handle, NVVFX_INPUT_IMAGE, filter->gpu_ar_src_img);
	nv_error(vfxErr, "Failed to set input image for Artifact Reduction filter", filter, false);

	vfxErr = NvVFX_SetImage(filter->ar_handle, NVVFX_OUTPUT_IMAGE, filter->gpu_ar_dst_img);
	nv_error(vfxErr, "Failed to set output image for Artifact Reduction filter", filter, false);

	vfxErr = NvVFX_Load(filter->ar_handle);

	bool success = NVCV_SUCCESS == vfxErr;

	if (!success)
	{
		if (NVCV_ERR_RESOLUTION != vfxErr)
		{
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Failed to load NvVFX AR effect %i: %s", vfxErr, errString);
			os_atomic_set_bool(&filter->processing_stopped, true);
		}
	}
	filter->invalid_ar_size = !success;
	filter->reload_ar_fx = false;

	debug("load_ar_fx: exiting");
	return success;
}


/* Loads the SR NVFX filter effect. Ensures any necessary parameters have been set.
* 
* returns: False if there is any error, true otherwise
*/
static bool load_sr_fx(struct nv_superresolution_data *filter)
{
	debug("load_sr_fx: entering");
	NvCV_Status vfxErr;

	if (filter->type == S_TYPE_UP)
	{
		vfxErr = NvVFX_SetF32(filter->sr_handle, NVVFX_STRENGTH, filter->strength);
		nv_error_nr(vfxErr, "Failed to set upscaling sharpening strength", filter, false);
	}
	else if (filter->type == S_TYPE_SR)
	{
		vfxErr = NvVFX_SetU32(filter->sr_handle, NVVFX_MODE, filter->sr_mode);
		nv_error_nr(vfxErr, "Failed to set SR mode", filter, false);
	}

	vfxErr = NvVFX_SetImage(filter->sr_handle, NVVFX_INPUT_IMAGE, filter->gpu_sr_src_img);
	nv_error(vfxErr, "Error setting SuperRes input image", filter, false);

	vfxErr = NvVFX_SetImage(filter->sr_handle, NVVFX_OUTPUT_IMAGE, filter->gpu_sr_dst_img);
	nv_error(vfxErr, "Error setting SuperRes output image", filter, false);

	vfxErr = NvVFX_Load(filter->sr_handle);

	bool success = NVCV_SUCCESS == vfxErr;

	if (!success)
	{
		if (NVCV_ERR_RESOLUTION != vfxErr)
		{
			const char *errString = NvCV_GetErrorStringFromCode(vfxErr);
			error("Failed to load NvVFX SR effect %i: %s", vfxErr, errString);
			os_atomic_set_bool(&filter->processing_stopped, true);
		}
	}

	filter->invalid_sr_size = !success;

	filter->reload_sr_fx = false;

	debug("load_sr_fx: exiting");
	return success;
}



/* Creates a new CUDA stream for the filter, desytroying the previous one if it exists.
* returns: False if there is an error, true otherwise
*/
static bool create_cuda(struct nv_superresolution_data *filter)
{
	debug("create_cuda: entering");

	if (filter->stream)
	{
		NvVFX_CudaStreamDestroy(filter->stream);
		filter->stream = NULL;
	}

	NvCV_Status vfxErr = NvVFX_CudaStreamCreate(&filter->stream);
	nv_error(vfxErr, "Failed to create NvVFX CUDA Stream: %i", filter, true);

	debug("create_cuda: exiting");
	return true;
}



/* Creates the NvFX filters that are needed
* This has the side consequence of requiring NvCVImage buffers to be created or reallocated
* return - True if there are no errors or if nothing was created. False if there was an error.
*/
static bool initialize_fx(struct nv_superresolution_data *filter)
{
	bool success = true;

	if (success && filter->apply_ar && !filter->ar_handle)
	{
		debug("initialize_fx: creating AR fx");
		success = create_nvfx(filter, &filter->ar_handle, NVVFX_FX_ARTIFACT_REDUCTION);
		filter->are_images_allocated = false;
	}

	if (success && filter->type != S_TYPE_NONE && !filter->sr_handle)
	{
		debug("initialize_fx: creating SR fx");
		const char *FX = filter->type == S_TYPE_SR ? NVVFX_FX_SUPER_RES : NVVFX_FX_SR_UPSCALE;
		success = create_nvfx(filter, &filter->sr_handle, FX);
		filter->are_images_allocated = false;
	}

	return success;
}



/* Called when the user changes any property in our OBS property window
* Applies user settings changes to the filter, setting any necessary update/creation flags to be properly handled later.
*/
static void nv_superres_filter_update(void *data, obs_data_t *settings)
{
	struct nv_superresolution_data *filter =(struct nv_superresolution_data *)data;

	bool enable_stitching = obs_data_get_bool(settings, S_ENABLE_STITCH);
	const char *stmap_path = obs_data_get_string(settings, S_STMAP_PATH);
	const char *custom_shader = obs_data_get_string(settings, S_CUSTOM_SHADER);

	if (filter->enable_stitching != enable_stitching) {
		filter->enable_stitching = enable_stitching;
		filter->stitching_bound = false; // Reset binding
		debug("Update: Stitching toggled");
	}

    /* Check for Custom Shader Update */
    if (custom_shader && (!filter->custom_shader_path || strcmp(filter->custom_shader_path, custom_shader) != 0)) {
        bfree(filter->custom_shader_path);
        filter->custom_shader_path = bstrdup(custom_shader);
        
        obs_enter_graphics();
        if (filter->stitch_effect) {
             gs_effect_destroy(filter->stitch_effect);
             filter->stitch_effect = NULL;
        }
        
        char *effect_path = NULL;
        if (filter->custom_shader_path && *filter->custom_shader_path) {
             effect_path = bstrdup(filter->custom_shader_path);
             debug("Loading custom shader: %s", effect_path);
        } else {
             effect_path = obs_module_file("stmap_stitch.effect");
             debug("Loading default shader");
        }
        
        char *load_err = NULL;
        filter->stitch_effect = gs_effect_create_from_file(effect_path, &load_err);
        bfree(effect_path);
        
        if (filter->stitch_effect) {
             filter->stitch_image_param = gs_effect_get_param_by_name(filter->stitch_effect, "image");
             filter->stitch_remap_param = gs_effect_get_param_by_name(filter->stitch_effect, "remap_texture");
        } else {
             error("Failed to reload stitch effect: %s", load_err);
             bfree(load_err);
        }
        obs_leave_graphics();
    }

	if (stmap_path && (!filter->stmap_path || strcmp(filter->stmap_path, stmap_path) != 0)) {
		bfree(filter->stmap_path);
		filter->stmap_path = bstrdup(stmap_path);
		
		/* Load STMap Image */
		obs_enter_graphics();
		if (filter->stmap_texture) {
			gs_texture_destroy(filter->stmap_texture);
			filter->stmap_texture = NULL;
		}
		gs_image_file_free(&filter->stmap_image);
		
		gs_image_file_init(&filter->stmap_image, stmap_path);
		if (filter->stmap_image.loaded) {
			filter->stmap_texture = gs_texture_create_from_gs_image_file(&filter->stmap_image);
			filter->stmap_loaded = true;
		} else {
			error("Failed to load STMap image: %s", stmap_path);
			filter->stmap_loaded = false;
		}
		obs_leave_graphics();
	}

	int type = (int)obs_data_get_int(settings, S_TYPE);
	int sr_mode = (int)obs_data_get_int(settings, S_MODE_SR);
	bool apply_ar = obs_data_get_bool(settings, S_ENABLE_AR);
	filter->scale = (int)obs_data_get_int(settings, S_SR_SCALE);

	if (filter->type != type)
	{
		filter->type = type;
		filter->destroy_sr = true;
		filter->reload_sr_fx = true;
		debug("Update: Filter type changed");
	}

	if (filter->type == S_TYPE_SR)
	{
        int new_scale = (int)obs_data_get_int(settings, S_SR_SCALE);
        if (filter->scale != new_scale) {
             filter->scale = new_scale;
             /* Trigger property update to force graph rebuild for size change */
             obs_source_update_properties(filter->context);
        }
	}

	if (filter->sr_mode != sr_mode)
	{
		filter->sr_mode = sr_mode;
		filter->reload_sr_fx = true;
		debug("Update: Super Res mode changed");
	}

	if (filter->apply_ar != apply_ar)
	{
		filter->apply_ar = apply_ar;
		debug("Update: AR changed");

		if (!apply_ar)
		{
			debug("Update: AR disabled");
			filter->destroy_ar = true;
		}
		else
		{
			debug("Update: AR enabled");
		}

		filter->reload_ar_fx = true;
	}

	int ar_mode = (int)obs_data_get_int(settings, S_MODE_AR);

	if (filter->apply_ar && filter->ar_mode != ar_mode)
	{
			filter->ar_mode = ar_mode;
			filter->reload_ar_fx = true;
			debug("Update: AR mode changed");
	}

	if (type == S_TYPE_UP)
	{
		float strength = (float)obs_data_get_double(settings, S_STRENGTH);
        
        int new_scale = (int)obs_data_get_int(settings, S_UP_SCALE);
        if (filter->scale != new_scale) {
             filter->scale = new_scale;
             obs_source_update_properties(filter->context);
        }

		if (fabsf(strength - filter->strength) > EPSILON)
		{
			filter->strength = strength;
			filter->reload_sr_fx = true;
			debug("Update: Upscaling strength changed");
		}
	}
}



static bool alloc_image_from_texture(struct nv_superresolution_data *filter, img_create_params_t *params, gs_texture_t *texture)
{
	debug("alloc_image_from_texture: entered");

#ifdef _WIN32
	struct ID3D11Texture2D *d11texture = (struct ID3D11Texture2D *)gs_texture_get_obj(texture);

	if (!d11texture)
	{
		error("Couldn't retrieve d3d11texture2d from gs_texture");
		return false;
	}

	NvCV_Status vfxErr;

	if (*(params->buffer) != NULL)
	{
		NvCVImage_Destroy(*(params->buffer));
	}

	vfxErr = NvCVImage_Create(params->width, params->height,
					params->pixel_fmt, params->comp_type,
					params->layout, NVCV_GPU,
					params->alignment, params->buffer);
	nv_error(vfxErr, "Error creating source NvCVImage", filter, false);

	vfxErr = NvCVImage_InitFromD3D11Texture(*(params->buffer), d11texture);
	nv_error(vfxErr, "Error allocating NvCVImage from ID3D11Texture", filter, false);

#else
    // Linux / OpenGL Path
    uint32_t tex_id = (uint32_t)(uintptr_t)gs_texture_get_obj(texture);
    if (!tex_id) {
        error("Couldn't retrieve GL texture ID from gs_texture");
        return false;
    }

    cudaGraphicsResource_t *res_ptr = NULL;
    uint32_t *last_id_ptr = NULL;
    bool *reg_ptr = NULL;

    // Identify if this is src or dst image based on pointers
    if (params->buffer == &filter->src_img) {
        res_ptr = &filter->src_gl_cuda_res;
        last_id_ptr = &filter->last_src_tex_id;
        reg_ptr = &filter->src_gl_registered;
    } else if (params->buffer == &filter->dst_img) {
        res_ptr = &filter->dst_gl_cuda_res;
        last_id_ptr = &filter->last_dst_tex_id;
        reg_ptr = &filter->dst_gl_registered;
    } else {
        error("Unknown buffer pointer in alloc_image_from_texture");
        return false;
    }

    // Register / Re-register if changed
    if (*reg_ptr && *last_id_ptr != tex_id) {
        cudaGraphicsUnregisterResource(*res_ptr);
        *reg_ptr = false;
    }

    if (!*reg_ptr) {
        cudaError_t err = cudaGraphicsGLRegisterImage(res_ptr, tex_id, GL_TEXTURE_2D, 
            (params->buffer == &filter->src_img) ? cudaGraphicsRegisterFlagsReadOnly : cudaGraphicsRegisterFlagsNone);
        if (err != cudaSuccess) {
            error("CUDA Graphics GL Register Image failed: %d", err);
            return false;
        }
        *reg_ptr = true;
        *last_id_ptr = tex_id;
    }

    NvCV_Status vfxErr;
    if (*(params->buffer) != NULL) {
        NvCVImage_Destroy(*(params->buffer));
    }

    // Create a container image (NVCV_CUDA_ARRAY)
    // We use NVCV_CPU to avoid allocating a massive linear buffer, as we will inject the cudaArray later.
    vfxErr = NvCVImage_Create(params->width, params->height,
                    params->pixel_fmt, params->comp_type,
                    params->layout, NVCV_CPU, 
                    params->alignment, params->buffer);
    nv_error(vfxErr, "Error creating shell NvCVImage", filter, false);
    
    // Manually set the info to CUDA Array mode so SDK knows how to treat it
    if (*(params->buffer)) {
        (*(params->buffer))->gpuMem = NVCV_CUDA_ARRAY;
    }

#endif

	debug("alloc_image_from_texture: exiting");
	return true;
}



/*
* Simple wrapper method around alloc_image_from_texture to accept textrender parameter
* 
* param filter
* param params - 
* param texture - OBS texrender texture to bind to the buffer in params
* 
* return - True if there is no error, False otherwise
*/
static bool alloc_image_from_texrender(struct nv_superresolution_data *filter, img_create_params_t *params, gs_texrender_t *texture)
{
	debug("alloc_image_from_texrender");
	return alloc_image_from_texture(filter, params, gs_texrender_get_texture(texture));
}



/* Allocates or reallocates the NvCVImage buffer provided in the param struct
* If width2 or height2 are > 0, the image buffer will have memory allocated to fit the maximum size between 
* but be sized to width X height. This is used to allocate intermediary staging buffers
* 
* returns - True if there is no error, False otherwise
*/
static bool alloc_image(struct nv_superresolution_data* filter, img_create_params_t *params)
{
	debug("alloc_image: entered for buffer %X", *(params->buffer));

	uint32_t create_width = params->width2 > 0 ? params->width2 : params->width;
	uint32_t create_height = params->height2 > 0 ? params->height2 : params->height;
	NvCV_Status vfx_err;

	// If our NVFX Image exists, resize and reformat it
	if (*(params->buffer) != NULL)
	{
		debug("alloc_image: realloc buffer %X", *(params->buffer));

		vfx_err = NvCVImage_Realloc(
			     *(params->buffer), create_width, create_height,
			     params->pixel_fmt, params->comp_type,
			     params->layout, NVCV_GPU, params->alignment);

		nv_error(vfx_err, "Failed to re-allocate image buffer", filter, false);
	}
	else
	{
		debug("alloc_image: creating new NvCVImage buffer for %X", *(params->buffer));

		vfx_err = NvCVImage_Create(
			     create_width, create_height, params->pixel_fmt,
			     params->comp_type, params->layout, NVCV_GPU,
			     params->alignment, params->buffer);

		nv_error(vfx_err, "Failed to create image buffer", filter, false);

		debug("alloc_image: alloc buffer %X", *(params->buffer));
		vfx_err = NvCVImage_Alloc(
			     *(params->buffer), create_width, create_height,
			     params->pixel_fmt, params->comp_type,
			     params->layout, NVCV_GPU, params->alignment);

		nv_error(vfx_err, "Failed to allocate image buffer", filter, false);

		// We create our image at the given secondary size, and then resize it down to the original size we want
		// This is the recommended method from the nVidia video effects SDK for allocating staging buffers
		if (create_height != params->height || create_width != params->width)
		{
			debug("alloc_image: two-sized re-alloc buffer %X", *(params->buffer));

			vfx_err = NvCVImage_Realloc(
				     *(params->buffer), params->width,
				     params->height,
				     params->pixel_fmt, params->comp_type,
				     params->layout, NVCV_GPU,
				     params->alignment);

			nv_error(vfx_err, "Failed to resize image buffer", filter, false);
		}
	}

	debug("alloc_image: exiting for buffer %X", *(params->buffer));

	return true;
}



/*
* Allocates and binds Artifact Reduction images, the source and destination images required for this NVFX Filter to work
* 
* param filter - our OBS filter structure
* returns - True if there were no errors, False otherwise
*/
static bool alloc_ar_images(struct nv_superresolution_data* filter)
{
	debug("alloc_ar_images: entering");

	img_create_params_t ar_img =
	{
		.buffer = &filter->gpu_ar_src_img,
		.width = filter->width,
		.height = filter->height,
		.pixel_fmt = NVCV_BGR,
		.comp_type = NVCV_F32,
		.layout = NVCV_PLANAR,
		.alignment = 1,
	};

	if (!alloc_image(filter, &ar_img))
	{
		error("Failed to allocate AR source buffer");
		return false;
	}

	ar_img.buffer = &filter->gpu_ar_dst_img;

	if (!alloc_image(filter, &ar_img))
	{
		error("Failed to allocate AR dest buffer");
		return false;
	}

	filter->reload_ar_fx = true;

	debug("alloc_ar_images: entering");
	return true;
}



/*
* Allocates required textures for the OBS source our filter is applied to
* 
* param filter - our OBS filter structure
* return - True if there is no error, False otherwise
*/
static bool alloc_obs_textures(struct nv_superresolution_data* filter)
{
	debug("alloc_obs_textures: entering");

	/* 3. create texrenders */
	if (filter->render)
	{
		debug("alloc_obs_textures: destroying existing render texture");
		gs_texrender_destroy(filter->render);
	}

	debug("alloc_obs_textures: creating render texture");
	filter->render = gs_texrender_create(gs_get_format_from_space(filter->space), GS_ZS_NONE);

	kill_on_error(filter->render, "Failed to create render texrenderer", filter);

	if (filter->render_unorm)
	{
		debug("alloc_obs_textures: destroying existing render unorm texture");
		gs_texrender_destroy(filter->render_unorm);
	}

	debug("alloc_obs_textures: creating render unorm texture");
	filter->render_unorm = gs_texrender_create(GS_BGRA_UNORM, GS_ZS_NONE);

	kill_on_error(filter->render_unorm, "Failed to create render_unorm texrenderer", filter);

	filter->done_initial_render = false;

	debug("alloc_obs_textures: exiting");
	return true;
}



/* Allocates the Super Resolution source images, these are allocated anytime the target is resized, or the filter type changed */
static bool alloc_sr_source_images(struct nv_superresolution_data *filter)
{
	debug("alloc_sr_source_images: entering");

	if (!filter->is_target_valid)
	{
		return true;
	}

	img_create_params_t img = {
		.buffer = &filter->gpu_sr_src_img,
		.width = filter->width,
		.height = filter->height
	};

	if (filter->type == S_TYPE_SR)
	{
		img.pixel_fmt = NVCV_BGR;
		img.comp_type = NVCV_F32;
		img.layout = NVCV_PLANAR;
		img.alignment = 1;
	}
	else if (filter->type == S_TYPE_UP)
	{
		img.pixel_fmt = NVCV_RGBA;
		img.comp_type = NVCV_U8;
		img.layout = NVCV_CHUNKY;
		img.alignment = 32;
	}
	else
	{
		error("Attempted to allocate source image buffer for No Upscaler");
	}

    /* 8K OPTIMIZATION NOTE: 
       For 8K (8192x4320), a BGR f32 buffer is approx 405MB.
       RGBA u8 is 135MB. 
       Ensure system has adequate VRAM.
    */

	if (!alloc_image(filter, &img))
	{
		error("Failed to allocate SuperRes source buffer");
		return false;
	}

	filter->reload_sr_fx = true;

    /* Set Colorspace for HDR support */
    set_nvcv_colorspace(filter->gpu_sr_src_img, filter->space);

	debug("alloc_sr_source_images: exiting");
	return true;
}



/* Allocates the Super Resolution source images, these are allocated anytime the target is resized, the filter type changed, or the sr_scale changed */
static bool alloc_sr_dest_images(struct nv_superresolution_data* filter)
{
	debug("alloc_sr_dest_images: entering");

	if (!filter->is_target_valid)
	{
		return true;
	}

	img_create_params_t img = {
		.buffer = &filter->gpu_sr_dst_img,
		.width = filter->out_width,
		.height = filter->out_height
	};

	if (filter->type == S_TYPE_SR)
	{
		img.pixel_fmt = NVCV_BGR;
		img.comp_type = NVCV_F32;
		img.layout = NVCV_PLANAR;
		img.alignment = 1;
	}
	else if (filter->type == S_TYPE_UP)
	{
		img.pixel_fmt = NVCV_RGBA;
		img.comp_type = NVCV_U8;
		img.layout = NVCV_CHUNKY;
		img.alignment = 32;
	}
	else
	{
		error("Attempted to allocate destination image buffer for No Upscaler");
	}

	if (!alloc_image(filter, &img))
	{
		error("Failed to allocate NvCVImage SR dest buffer");
		return false;
	}

	/* Allocate the staging buffer next to set it's size */
	img.buffer = &filter->gpu_staging_img;
	img.width = filter->width;
	img.height = filter->height;
	img.width2 = filter->out_width;
	img.height2 = filter->out_height;

	if (!alloc_image(filter, &img))
	{
		error("Failed to allocate NvCVImage FX staging buffer");
		return false;
	}

	filter->reload_sr_fx = true;
    
    /* Set Colorspace for HDR Support */
     set_nvcv_colorspace(filter->gpu_sr_dst_img, filter->space);
     if (filter->gpu_staging_img) {
         set_nvcv_colorspace(filter->gpu_staging_img, filter->space);
     }

	debug("alloc_sr_dest_images: exiting");
	return true;
}



/* (Re)allocates any images that are pending (re)allocation
/* @return - false if there's any error, true otherwise */
static bool alloc_nvfx_images(struct nv_superresolution_data *filter)
{
	debug("alloc_nvfx_images: entering");

	if (filter->ar_handle)
	{
		if (!alloc_ar_images(filter))
		{
			error("Failed to allocate AR NvFXImages");
			return false;
		}
	}

	if (filter->sr_handle)
	{
		if (!alloc_sr_source_images(filter))
		{
			error("Failed to allocate SR Source NvFXImages");
			return false;
		}

		if (!alloc_sr_dest_images(filter))
		{
			error("Failed to allocate SR Dest NvFXImages");
			return false;
		}
	}

	/* 
    * Legacy Workaround: This temporary image is not required if using Upscaling.
	* If using only AR, or using SR at all this IS required as an intermediate step for BGRf32 -> D3D11 RGBAu8 conversion
    * UNLESS we are on modern hardware/drivers where this restriction is lifted.
    */
#ifdef LEGACY_BUFFER_WORKAROUND
	if (filter->type != S_TYPE_UP)
#else
    if (0) // Disabled for optimization
#endif
	{
		debug("alloc_nvfx_images: creaing dst_tmp_img");
		img_create_params_t img = {
			.buffer = &filter->gpu_dst_tmp_img,
			.pixel_fmt = NVCV_RGBA,
			.comp_type = NVCV_U8,
			.layout = NVCV_CHUNKY,
			.alignment = 0,
			.width = filter->out_width,
			.height = filter->out_height,
			.width2 = 0,
			.height2 = 0
		};

		if (!alloc_image(filter, &img))
		{
			error("Failed to allocate conversion buffer for AR or SR pass");
			return false;
		}
	}

	debug("alloc_nvfx_images: exiting");

	return true;
}



/*
* Initializes and binds the final destination NVFX Image to the output texture intended for OBS
* note: the internal texture, and nvfx image will be destroyed and recreated if they already exist
* param filter - Our OBS data structure
*/
static bool alloc_destination_image(struct nv_superresolution_data* filter)
{
	if (filter->scaled_texture)
	{
		gs_texture_destroy(filter->scaled_texture);
	}

	filter->scaled_texture = gs_texture_create(filter->out_width, filter->out_height, GS_RGBA_UNORM, 1, NULL, 0);

	kill_on_error(filter->scaled_texture, "Final output texture couldn't be created", filter);

	img_create_params_t params = {
		.buffer = &filter->dst_img,
		.width = filter->out_width,
		.height = filter->out_height,
		.pixel_fmt = NVCV_RGBA,
		.comp_type = NVCV_U8,
		.layout = NVCV_CHUNKY,
		.alignment = 0
	};

	if (!alloc_image_from_texture(filter, &params, filter->scaled_texture))
	{
		error("Failed to create dest NvCVImage from OBS output texture");
		return false;
	}

	return true;
}



/* Allocates any textures or images that have been flagged for allocation
* Used in both initialization and render tick to ensure things are created before use */
static bool init_images(struct nv_superresolution_data* filter)
{
	debug("init_images: entering");

	if (!alloc_obs_textures(filter))
	{
		return false;
	}

	if (!alloc_nvfx_images(filter))
	{
		return false;
	}

	if ((filter->apply_ar || filter->type != S_TYPE_NONE) && !alloc_destination_image(filter))
	{
		return false;
	}

	filter->are_images_allocated = true;

	debug("init_images: exiting");

	return true;
}



/*
* Called when the source, or this filter itself needs to be reinitialized for some reason.
*/
static void nv_superres_filter_reset(void *data, calldata_t *calldata)
{
	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;

	debug("nv_superres_filter_reset: Entering");

	if (!filter)
	{
		error("Attempted to reset filter, but filter structure is invalid!");
		return;
	}

	os_atomic_set_bool(&filter->processing_stopped, true);

	debug("nv_superres_filter_reset: Source reset recreate CUDA stream");
	if (!create_cuda(filter))
	{
		return;
	}

	filter->destroy_ar = true;
	filter->destroy_sr = true;
	filter->reload_ar_fx = true;
	filter->reload_sr_fx = true;
	filter->are_images_allocated = false;

	os_atomic_set_bool(&filter->processing_stopped, false);

	debug("nv_superres_filter_reset: Entering");
}



/*
* Runs the NVFX filter pipeline on the current source frame.
* The final destination NVFX buffer in fitler will be updated with the output from this pipeline
* 
* param filter - our OBS filter structure
* return - False if there was an error. True otherwise.
*/
static NvCV_Status map_resources(struct nv_superresolution_data *filter, NvCVImage *img)
{
#ifdef _WIN32
    return NvCVImage_MapResource(img, filter->stream);
#else
    cudaGraphicsResource_t res = NULL; 
    if (img == filter->src_img) res = filter->src_gl_cuda_res;
    else if (img == filter->dst_img) res = filter->dst_gl_cuda_res;
    else return NVCV_ERR_GENERAL;

    if (!res) return NVCV_ERR_INITIALIZATION;

    cudaError_t err = cudaGraphicsMapResources(1, &res, filter->stream); 
    if (err != cudaSuccess) return NVCV_ERR_CUDA;

    cudaArray_t arr;
    err = cudaGraphicsSubResourceGetMappedArray(&arr, res, 0, 0);
    if (err != cudaSuccess) {
         cudaGraphicsUnmapResources(1, &res, filter->stream);
         return NVCV_ERR_CUDA;
    }

    img->pixels = (void*)arr; 
    
    return NVCV_SUCCESS;
#endif
}

static NvCV_Status unmap_resources(struct nv_superresolution_data *filter, NvCVImage *img)
{
#ifdef _WIN32
    return NvCVImage_UnmapResource(img, filter->stream);
#else
    cudaGraphicsResource_t res = NULL; 
    if (img == filter->src_img) res = filter->src_gl_cuda_res;
    else if (img == filter->dst_img) res = filter->dst_gl_cuda_res;
    else return NVCV_ERR_GENERAL;

    if (!res) return NVCV_ERR_INITIALIZATION;

    cudaError_t err = cudaGraphicsUnmapResources(1, &res, filter->stream);
    if (err != cudaSuccess) return NVCV_ERR_CUDA;
    return NVCV_SUCCESS;
#endif
}

static bool process_texture_superres(struct nv_superresolution_data *filter)
{
	/* From nVidias recommendations here https://docs.nvidia.com/deeplearning/maxine/vfx-sdk-programming-guide/index.html#upscale-filter
	* We have 3 main paths to take
	* A. AR pass only
	* B. Upscaling Pass Only
	* C. AR Pass -> Upscaling Pass
	* So the effect pipeline is
	*	A: src_img -> staging -> AR_src -> Run FX -> AR_dst -> staging -> dst_tmp_img -> staging -> dst_img
	*	B: src_img -> staging -> SR_src -> Run FX -> SR_dst -> staging -> dst_tmp_img -> staging -> dst_img
	*	C: src_img -> staging -> AR_src -> Run FX -> AR_dst -> staging -> SR_src -> Run FX -> SR_dst -> staging -> dst_tmp_img -> staging -> dst_img
	* 
	* The staging -> dst_tmp_img stage is skipped if the AR is not selected, and the upscaling method is standard upscaling
	*/

	NvCVImage *destination = filter->dst_img;

	if (filter->ar_handle)
	{
		destination = filter->gpu_ar_src_img;
	}
	else if (filter->sr_handle)
	{
		destination = filter->gpu_sr_src_img;
	}

	/* Have to map the D3D buffers before your manipulate them, and unmap before D3D is allowed to take over again */
	NvCV_Status vfxErr = map_resources(filter, filter->src_img);
	nv_error(vfxErr, "Error mapping resource for source texture", filter, false);

	vfxErr = NvCVImage_Transfer(filter->src_img, destination, filter->ar_handle ? 1.0f/255.0f : 1.0f, filter->stream, filter->gpu_staging_img);
	nv_error(vfxErr, "Error converting src img for first filter pass", filter, false);

	vfxErr = unmap_resources(filter, filter->src_img);
	nv_error(vfxErr, "Error unmapping resource for src texture", filter, false);

	/* 2. process artifact reduction fx pass, and transfer to upscaling pass, or to final dst_tmp_img */
	if (filter->ar_handle)
	{
		vfxErr = NvVFX_Run(filter->ar_handle, 0);

		if (vfxErr == NVCV_ERR_CUDA)
		{
			nv_superres_filter_reset(filter, NULL);
			return false;
		}

		nv_error(vfxErr, "Error running the AR FX", filter, false);

		destination = (filter->type == S_TYPE_NONE) ? filter->gpu_dst_tmp_img : filter->gpu_sr_src_img;

		vfxErr = NvCVImage_Transfer(filter->gpu_ar_dst_img, destination, 255.0f, filter->stream, filter->gpu_staging_img);
		nv_error(vfxErr, "Error converting src to BGR img for SR pass", filter, false);
	}

	/* 3. Run the image through the upscaling pass */
	if (filter->sr_handle)
	{
		vfxErr = NvVFX_Run(filter->sr_handle, 0);

		if (vfxErr == NVCV_ERR_CUDA)
		{
			nv_superres_filter_reset(filter, NULL);
			return false;
		}

		nv_error(vfxErr, "Error running the NvVFX Super Resolution stage.", filter, false);

		nv_error(vfxErr, "Error running the NvVFX Super Resolution stage.", filter, false);

#ifdef LEGACY_BUFFER_WORKAROUND
		if (!filter->gpu_dst_tmp_img)
		{
			/* Have to map the D3D buffers before your manipulate them, and unmap before D3D is allowed to take over again */
			destination = filter->dst_img;
			vfxErr = map_resources(filter, filter->dst_img);
			nv_error(vfxErr, "Error mapping resource for dst texture", filter, false);
		}
		else
		{
			destination = filter->gpu_dst_tmp_img;
		}
#else
        /* Optimized Path: Logic moved to Transfer step */
#endif

#ifdef LEGACY_BUFFER_WORKAROUND
		/* 3.5 move to a temp buffer, not tied to a bound D3D11 gs_texture_t, or used as an input/output NvCVImage to an effect */
		// This temporary buffer should not be required, but it is
		// see https://forums.developer.nvidia.com/t/no-transfer-conversion-from-planar-ncv-bgr-nvcv-f32-to-dx11-textures/183964/2
		vfxErr = NvCVImage_Transfer(filter->gpu_sr_dst_img, destination, filter->ar_handle && filter->type == S_TYPE_SR ? 255.0f : 1.0f, filter->stream, filter->gpu_staging_img);
		nv_error(vfxErr, "Error transfering super resolution upscaled texture to destination buffer", filter, false);

		if (!filter->gpu_dst_tmp_img)
		{
			vfxErr = unmap_resources(filter, filter->dst_img);
			nv_error(vfxErr, "Error unmapping resource for dst texture", filter, false);
		}
#else
        /* OPTIMIZATION: Direct Transfer for RTX 40/50 Series */
        /* We skip the intermediate legacy buffer and transfer directly to the mapped D3D11 resource. */
        /* This assumes the driver/SDK issue from 2021 is resolved in modern drivers. */
        
        destination = filter->dst_img;
        vfxErr = map_resources(filter, filter->dst_img);
        nv_error(vfxErr, "Error mapping resource for dst texture", filter, false);

        vfxErr = NvCVImage_Transfer(filter->gpu_sr_dst_img, destination, filter->ar_handle && filter->type == S_TYPE_SR ? 255.0f : 1.0f, filter->stream, filter->gpu_staging_img);
        
        // Unmap immediately after transfer
        unmap_resources(filter, filter->dst_img);
        
        nv_error(vfxErr, "Error transfering super resolution upscaled texture to D3D destination", filter, false);
#endif
	}

	/*
	* 4. Do the final dst_tmp_img -> staging -> dst_img transfer
	* This stage is only required when doing BGR/Planar to a D3D11 texture, as GPU->CUDA_ARRAY transfers in that format are not supported
	*/
        if (filter->gpu_dst_tmp_img)
	{
#ifdef LEGACY_BUFFER_WORKAROUND
		/* Have to map the D3D buffers before your manipulate them, and unmap before D3D is allowed to take over again */
		vfxErr = map_resources(filter, filter->dst_img);
		nv_error(vfxErr, "Error mapping resource for dst texture", filter, false);

		vfxErr = NvCVImage_Transfer(filter->gpu_dst_tmp_img, filter->dst_img, 1.0f, filter->stream, filter->gpu_staging_img);
		nv_error(vfxErr, "Error transferring temporary image buffer to final dest buffer", filter, false);

		vfxErr = unmap_resources(filter, filter->dst_img);
		nv_error(vfxErr, "Error unmapping resource for dst texture", filter, false);
#endif
	}

	return true;
}



/* Reload the NVFX filter effects, the filter ar_handle and sr_handle must be allocated
* param filter - the filter structure to validate
*/
static bool reload_fx(struct nv_superresolution_data* filter)
{
	if (nvvfx_supports_ar && filter->ar_handle && filter->reload_ar_fx && !load_ar_fx(filter))
	{
		error("Failed to load the artifact reduction NvVFX");
		return false;
	}

	if ((nvvfx_supports_sr || nvvfx_supports_up) && filter->reload_sr_fx && filter->sr_handle && !load_sr_fx(filter))
	{
		error("Failed to load the selected NvVFX %d", filter->type);
		return false;
	}

	return true;
}



static void* nv_superres_filter_create(obs_data_t* settings, obs_source_t* context)
{
	struct nv_superresolution_data* filter = (struct nv_superresolution_data*)bzalloc(sizeof(*filter));

	debug("nv_superres_filter_create: Entering");

	/* Does this filter exist already on a source, but the vfx sdk libraries weren't found? Let's leave. */
	if (!nvvfx_loaded)
	{
		nv_superres_filter_destroy(filter);
		debug("nv_superres_filter_create: Failed, NvVFX SDK not installed, or RTX GPU not found");
		return NULL;
	}

	filter->context = context;
	filter->sr_mode = S_MODE_DEFAULT;
	filter->type = S_TYPE_DEFAULT;
	filter->show_size_error = true;
	filter->scale = S_SCALE_15x;
	filter->strength = S_STRENGTH_DEFAULT;
	os_atomic_set_bool(&filter->processing_stopped, false);

	/* Load the effect file */
	char* effect_path = obs_module_file("rtx_superresolution.effect");
	char* load_err = NULL;

	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, &load_err);
	bfree(effect_path);

	if (filter->effect)
	{
		filter->image_param = gs_effect_get_param_by_name(filter->effect, "image");
		filter->upscaled_param = gs_effect_get_param_by_name(filter->effect, "image_upcsaled");
		filter->multiplier_param = gs_effect_get_param_by_name(filter->effect, "multiplier");
	}

    /* Load Stitch Effect (Initial) */
    char *stitch_effect_path = NULL;
    const char *custom = obs_data_get_string(settings, S_CUSTOM_SHADER);
    
    if (custom && *custom) {
        filter->custom_shader_path = bstrdup(custom);
        stitch_effect_path = bstrdup(custom);
    } else {
        stitch_effect_path = obs_module_file("stmap_stitch.effect");
    }

    filter->stitch_effect = gs_effect_create_from_file(stitch_effect_path, &load_err);
    bfree(stitch_effect_path);
    if (filter->stitch_effect) {
        filter->stitch_image_param = gs_effect_get_param_by_name(filter->stitch_effect, "image");
        filter->stitch_remap_param = gs_effect_get_param_by_name(filter->stitch_effect, "remap_texture");
    } else {
        error("Failed to load stmap_stitch.effect: %s", load_err);
    }
    // Create stitch render target
    filter->stitch_render = gs_texrender_create(GS_RGBA, GS_ZS_NONE); // Basic RGBA texture

	obs_leave_graphics();

	if (!filter->effect)
	{
		error("Failed to load effect file: %s", load_err);
		nv_superres_filter_destroy(filter);

		if (load_err)
		{
			bfree(load_err);
		}

		return NULL;
	}

	nv_superres_filter_update(filter, settings);

	if (!create_cuda(filter))
	{
		error("Failed to initialize filter, couldn't create FX");
		return NULL;
	}

	debug("nv_superres_filter_create: exiting");

	return filter;
}

static bool stmap_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	bool enabled = obs_data_get_bool(settings, S_ENABLE_STITCH);
	p = obs_properties_get(ppts, S_STMAP_PATH);
	obs_property_set_visible(p, enabled);
    p = obs_properties_get(ppts, S_CUSTOM_SHADER);
    obs_property_set_visible(p, enabled);
	return true;
}



static bool nv_filter_type_modified(obs_properties_t *ppts, obs_property_t *p, obs_data_t *settings)
{
	int type = (int)obs_data_get_int(settings, S_TYPE);

	obs_property_t *p_str = obs_properties_get(ppts, S_STRENGTH);
	obs_property_t *p_mode = obs_properties_get(ppts, S_MODE_SR);
	obs_property_t *p_sr_scale = obs_properties_get(ppts, S_SR_SCALE);
	obs_property_t *p_up_scale = obs_properties_get(ppts, S_UP_SCALE);

	obs_property_set_visible(p_sr_scale, type == S_TYPE_SR);
	obs_property_set_visible(p_up_scale, type == S_TYPE_UP);
	obs_property_set_visible(p_str, type == S_TYPE_UP);
	obs_property_set_visible(p_mode, type == S_TYPE_SR);

	return true;
}



static bool ar_pass_toggled(obs_properties_t *ppts, obs_property_t *p,obs_data_t *settings)
{
	p = obs_properties_get(ppts, S_MODE_AR);
	obs_property_set_visible(p, obs_data_get_bool(settings, S_ENABLE_AR));

	return true;
}



void update_validation_messages(obs_properties_t* ppts, struct nv_superresolution_data* filter)
{
		bool activateSRWarning = filter->type != S_TYPE_NONE && filter->invalid_sr_size;
		bool activateARWarning =  filter->apply_ar && filter->invalid_ar_size;
		bool fatal = filter->processing_stopped;

		obs_property_set_visible(obs_properties_get(ppts, S_VALID_TARGET), !fatal && !activateSRWarning && !activateARWarning);
		obs_property_set_visible(obs_properties_get(ppts, S_FATAL_ERROR), fatal);
		obs_property_set_visible(obs_properties_get(ppts, S_INVALID_ERROR), !fatal && (activateSRWarning || activateARWarning));
		obs_property_set_visible(obs_properties_get(ppts, S_INVALID_WARNING_SR), !fatal && activateSRWarning);
		obs_property_set_visible(obs_properties_get(ppts, S_INVALID_WARNING_AR), !fatal && activateARWarning);
}



bool on_verify_clicked(obs_properties_t* ppts, obs_property_t* p, void* data)
{
	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)obs_properties_get_param(ppts);

	if (filter)
	{
		update_validation_messages(ppts, filter);
	}

	return true;
}



static obs_properties_t *nv_superres_filter_properties(void *data)
{
	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;

	obs_properties_t *properties = obs_properties_create_param(filter, NULL);

	obs_property_t *filter_type = obs_properties_add_list(properties, S_TYPE, TEXT_FILTER, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(filter_type, TEXT_FILTER_DESC);
	obs_property_list_add_int(filter_type, TEXT_FILTER_NONE, S_TYPE_NONE);

    /* Stitching Properties */
	obs_property_t *p_stitch = obs_properties_add_bool(properties, S_ENABLE_STITCH, TEXT_ENABLE_STITCH);
	obs_property_set_modified_callback(p_stitch, stmap_modified);

    obs_property_t *p_stmap = obs_properties_add_path(properties, S_STMAP_PATH, TEXT_STMAP_PATH, OBS_PATH_FILE, "Image Files (*.tiff *.png *.jpg *.jpeg *.bmp *.tga *.gif);;All Files (*.*)", NULL);
    UNUSED_PARAMETER(p_stmap);
    UNUSED_PARAMETER(p_stmap);

    obs_property_t *p_shader = obs_properties_add_path(properties, S_CUSTOM_SHADER, TEXT_CUSTOM_SHADER, OBS_PATH_FILE, "Effect Files (*.effect *.hlsl);;All Files (*.*)", NULL);
    UNUSED_PARAMETER(p_shader);


	if (nvvfx_supports_sr)
	{
		obs_property_list_add_int(filter_type, TEXT_FILTER_SR, S_TYPE_SR);
	}
	if (nvvfx_supports_up)
	{
		obs_property_list_add_int(filter_type, TEXT_FILTER_UP, S_TYPE_UP);
	}

	obs_property_set_modified_callback(filter_type, nv_filter_type_modified);

	obs_property_t *sr_scale = obs_properties_add_list(properties, S_SR_SCALE, TEXT_SCALE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_set_long_description(sr_scale, TEXT_SCALE_DESC);

	// NOTE: This generally gives inaccurate results that will need to be manually fixed if possible when we validate the source input size of things
	//obs_property_list_add_int(sr_scale, TEXT_SRSCALE_SIZE_133x, S_SCALE_133x);
    obs_property_list_add_int(sr_scale, "Pass-Through (1.0x)", S_SCALE_NONE);
	obs_property_list_add_int(sr_scale, TEXT_SRSCALE_SIZE_15x, S_SCALE_15x);
	obs_property_list_add_int(sr_scale, TEXT_SRSCALE_SIZE_2x, S_SCALE_2x);
	obs_property_list_add_int(sr_scale, TEXT_SRSCALE_SIZE_3x, S_SCALE_3x);
	obs_property_list_add_int(sr_scale, TEXT_SRSCALE_SIZE_4x, S_SCALE_4x);

	obs_property_t *up_scale = obs_properties_add_list(properties, S_UP_SCALE, TEXT_SCALE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	// NOTE: This generally gives inaccurate results that will need to be manually fixed if possible when we validate the source input size of things
	//obs_property_list_add_int(up_scale, TEXT_UPSCALE_SIZE_133x, S_SCALE_133x);
    obs_property_list_add_int(up_scale, "Pass-Through (1.0x)", S_SCALE_NONE);
	obs_property_list_add_int(up_scale, TEXT_UPSCALE_SIZE_15x, S_SCALE_15x);
	obs_property_list_add_int(up_scale, TEXT_UPSCALE_SIZE_2x, S_SCALE_2x);
	obs_property_list_add_int(up_scale, TEXT_UPSCALE_SIZE_3x, S_SCALE_3x);
	obs_property_list_add_int(up_scale, TEXT_UPSCALE_SIZE_4x, S_SCALE_4x);

	if (nvvfx_supports_sr)
	{
		obs_property_t *sr_mode = obs_properties_add_list(properties, S_MODE_SR, TEXT_SR_MODE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_list_add_int(sr_mode, TEXT_SR_MODE_WEAK, S_MODE_WEAK);
		obs_property_list_add_int(sr_mode, TEXT_SR_MODE_STRONG, S_MODE_STRONG);
		obs_property_set_long_description(sr_mode, TEXT_UPSCALE_MODE_DESC);
	}

	if (nvvfx_supports_up)
	{
		obs_property_t *strength = obs_properties_add_float_slider(properties, S_STRENGTH, TEXT_UP_STRENGTH, 0.0, 1.0, 0.05);
        UNUSED_PARAMETER(strength);
	}

	if (nvvfx_supports_ar)
	{
		obs_property_t *ar_pass = obs_properties_add_bool(properties, S_ENABLE_AR, TEXT_AR);
		obs_property_set_long_description(ar_pass, TEXT_AR_DESC);
		obs_property_set_modified_callback(ar_pass, ar_pass_toggled);

		obs_property_t *ar_modes = obs_properties_add_list(properties, S_MODE_AR, TEXT_AR_MODE, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
		obs_property_set_long_description(ar_modes, TEXT_AR_MODE_DESC);
		obs_property_list_add_int(ar_modes, TEXT_AR_MODE_WEAK, S_MODE_WEAK);
		obs_property_list_add_int(ar_modes, TEXT_AR_MODE_STRONG, S_MODE_STRONG);
	}

	obs_properties_add_button(properties, S_PROPS_VERIFY, TEXT_BUTTON_VERIFY, on_verify_clicked);

	obs_property_t *prop_source_valid_sr = obs_properties_add_text(properties, S_VALID_TARGET, TEXT_VALID_TARGET, OBS_TEXT_INFO);
    UNUSED_PARAMETER(prop_source_valid_sr);

	obs_property_t *prop_fatal_error = obs_properties_add_text(properties, S_FATAL_ERROR, TEXT_FATAL_ERROR, OBS_TEXT_INFO);
	obs_property_text_set_info_type(prop_fatal_error, OBS_TEXT_INFO_ERROR);

	obs_property_t *prop_invalid_error = obs_properties_add_text(properties, S_INVALID_ERROR, TEXT_INVALID_ERROR, OBS_TEXT_INFO);
	obs_property_text_set_info_type(prop_invalid_error, OBS_TEXT_INFO_ERROR);

	obs_property_t *prop_invalid_warning_sr = obs_properties_add_text(properties, S_INVALID_WARNING_SR, TEXT_INVALID_WARNING_SR, OBS_TEXT_INFO);
	obs_property_text_set_info_type(prop_invalid_warning_sr, OBS_TEXT_INFO_WARNING);

	obs_property_t *prop_invalid_warning_ar = obs_properties_add_text(properties, S_INVALID_WARNING_AR, TEXT_INVALID_WARNING_AR, OBS_TEXT_INFO);
	obs_property_text_set_info_type(prop_invalid_warning_ar, OBS_TEXT_INFO_WARNING);

	update_validation_messages(properties, filter);

	return properties;
}



static void nv_superres_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_TYPE, S_TYPE_DEFAULT);
	
	if (nvvfx_supports_ar)
	{
		obs_data_set_default_bool(settings, S_ENABLE_AR, false);
		obs_data_set_default_int(settings, S_MODE_AR, S_MODE_DEFAULT);
	}

	if (nvvfx_supports_sr)
	{
		obs_data_set_default_int(settings, S_MODE_SR, S_MODE_DEFAULT);
		obs_data_set_default_int(settings, S_SR_SCALE, S_SCALE_DEFAULT);
	}

	if (nvvfx_supports_up)
	{
		obs_data_set_default_double(settings, S_STRENGTH, S_STRENGTH_DEFAULT);
		obs_data_set_default_int(settings, S_UP_SCALE, S_SCALE_DEFAULT);
	}
}



/*
* Called when a video frame available to be processed by the filter
* We don't do our processing here, as this would require copying this raw data from this frame to the NVFX image buffer every single frame
* We instead bind an internal texture to an NVFX image allowing its data to be updated by the OBS rendering process automatically
* 
* This function is purely used to inform us that we have a new frame available to process and our old previously processed frame is now invalid
*/
static struct obs_source_frame *nv_superres_filter_video(void *data, struct obs_source_frame *frame)
{
	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;
	filter->got_new_frame = true;
	return frame;
}



/*
* We check and validate our source size, requested sr_scale size, and color space here incase they change,
* if it does we need to recreate or resize the various image buffers used to accomodate
*/
static void nv_superres_filter_tick(void *data, float t)
{
	UNUSED_PARAMETER(t);

	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;

	if (filter->processing_stopped)
	{
		return;
	}

	obs_source_t *target = obs_filter_get_target(filter->context);

	if (!target)
	{
		return;
	}

	const uint32_t cx = obs_source_get_base_width(target);
	const uint32_t cy = obs_source_get_base_height(target);
	filter->target_width = cx;
	filter->target_height = cy;

	// initially the sizes are 0
	filter->is_target_valid = cx > 0 && cy > 0;

	if (!filter->is_target_valid)
	{
		return;
	}

	uint32_t cx_out;
	uint32_t cy_out;

	if (filter->apply_ar)
	{
		get_scale_factor(S_SCALE_AR, cx, cy, &cx_out, &cy_out);
		filter->is_target_valid = validate_scaling_aspect(cx, cy, cx_out, cy_out);
		filter->is_target_valid = filter->is_target_valid && validate_source_size(S_SCALE_AR, cx, cy, cx_out, cy_out);
		filter->invalid_ar_size = !filter->is_target_valid;
	}

	if (filter->is_target_valid)
	{
		if (filter->type != S_TYPE_NONE)
		{
			get_scale_factor(filter->scale, cx, cy, &cx_out, &cy_out);
			filter->is_target_valid = validate_scaling_aspect(cx, cy, cx_out, cy_out);
			if (filter->is_target_valid && filter->type != S_TYPE_UP)
			{
				filter->is_target_valid = validate_source_size(filter->scale, cx, cy, cx_out, cy_out);
			}
			filter->invalid_sr_size = !filter->is_target_valid;
		}
	}

	if (!filter->is_target_valid)
	{
		if (filter->show_size_error)
		{
			error("Input source is too small or too large for the requested scaling. Please try adding a Scale/Aspect ratio filter before this, or changing the input resolution of the source this filter is attached to!");
			filter->show_size_error = false;
		}
		return;
	}
	else if (!filter->show_size_error)
	{
		filter->show_size_error = true;
	}

	if (cx != filter->width || cy != filter->height || cx_out != filter->out_width || cy_out != filter->out_height)
	{
		debug("nv_superres_filter_tick: source size changed, or scale changed");

		filter->width = cx;
		filter->height = cy;
		filter->out_width = cx_out;
		filter->out_height = cy_out;
		filter->are_images_allocated = false;
        filter->stitching_bound = false;
	}

	filter->processed_frame = false;
}



/*
* Gets the technique name, and color intensity multiplier for the given color space conversion
* Copied from OBS filter sources as this is common boilerplate code
* 
* param current_space - our color space we're displaying
* param source_space - the color space of the source our filter is applied to
* param multiplier - OUTPUT parameter, the color intensity multiplier to be used in shaders
* 
* return - the technique name for the shader to apply
*/
static const char *get_tech_name_and_multiplier(enum gs_color_space current_space, enum gs_color_space source_space, float *multiplier)
{
	const char *tech_name = "Draw";
	*multiplier = 1.0f;

	switch (source_space)
	{
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
		if (current_space == GS_CS_709_SCRGB)
		{
			tech_name = "DrawMultiply";
			*multiplier = obs_get_video_sdr_white_level() / 80.0f;
		}
		break;

	case GS_CS_709_EXTENDED:
		switch (current_space)
		{
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
		switch (current_space)
		{
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			tech_name = "DrawMultiplyTonemap";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
			break;
		case GS_CS_709_EXTENDED:
			tech_name = "DrawMultiply";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
		}
		break;
    /* HDR Enums disabled for build compatibility
    case GS_CS_2100_PQ:
    case GS_CS_2100_HLG:
        switch (current_space)
		{
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
        case GS_CS_709_EXTENDED:
            tech_name = "DrawTonemap"; // Use OBS built-in tonemapper for HDR->SDR
            break;
        default:
            tech_name = "Draw"; // Pass through if destination is also HDR
        }
        break;
    */
	}

	return tech_name;
}





/*
* Draws our final processed texture to the scene
* 
* param - our OBS filter structure
*/
static bool draw_superresolution(struct nv_superresolution_data *filter)
{
	const enum gs_color_space source_space = filter->space;
	float multiplier;
	const char *technique = get_tech_name_and_multiplier(gs_get_color_space(), source_space, &multiplier);
	const enum gs_color_format format = gs_get_format_from_space(source_space);

	if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space, OBS_ALLOW_DIRECT_RENDERING))
	{
		if (source_space != GS_CS_SRGB)
		{
			gs_effect_set_texture(filter->upscaled_param, filter->scaled_texture);
		}
		else
		{
			gs_effect_set_texture_srgb(filter->upscaled_param, filter->scaled_texture);
		}

		gs_effect_set_float(filter->multiplier_param, multiplier);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		obs_source_process_filter_tech_end(filter->context, filter->effect, filter->out_width, filter->out_height, technique);

		gs_blend_state_pop();
		return true;
	}

	return false;
}



static void render_source_to_render_tex(struct nv_superresolution_data *filter, obs_source_t *const target, obs_source_t *const parent)
{
	const uint32_t target_flags = obs_source_get_output_flags(target);
	const uint32_t parent_flags = obs_source_get_output_flags(parent);
    UNUSED_PARAMETER(parent_flags);

	bool custom_draw = (target_flags & OBS_SOURCE_CUSTOM_DRAW) != 0;
	bool async = (target_flags & OBS_SOURCE_ASYNC) != 0;

	const enum gs_color_space preferred_spaces[] =
	{
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
        /*
        GS_CS_2100_PQ,
        GS_CS_2100_HLG,
        */
	};

	const enum gs_color_space source_space = obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	gs_texrender_t *const render = filter->render;
	gs_texrender_reset(render);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	if (gs_texrender_begin_with_color_space(render, filter->width, filter->height, source_space))
	{
		struct vec4 clear_color;
		vec4_zero(&clear_color);
		gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);

		gs_ortho(0.0f, (float)filter->width, 0.0f, (float)filter->height, -100.0f, 100.0f);

		if (target == parent && !custom_draw && !async)
		{
			obs_source_default_render(target);
		}
		else
		{
			obs_source_video_render(target);
		}

		gs_texrender_end(render);

		gs_texrender_t *const render_unorm = filter->render_unorm;
		gs_texrender_reset(render_unorm);

		if (gs_texrender_begin_with_color_space(render_unorm, filter->width, filter->height, GS_CS_SRGB))
		{
			const bool previous = gs_framebuffer_srgb_enabled();
			gs_enable_framebuffer_srgb(true);
			gs_enable_blending(false);

			gs_ortho(0.0f, (float)filter->width, 0.0f, (float)filter->height, -100.0f, 100.0f);

			const char *tech_name = "ConvertUnorm";
			float multiplier = 1.f;

			if (source_space == GS_CS_709_EXTENDED)
			{
				tech_name = "ConvertUnormTonemap";
			}
			else if (source_space == GS_CS_709_SCRGB)
			{
				tech_name = "ConvertUnormMultiplyTonemap";
				multiplier = 80.0f / obs_get_video_sdr_white_level();
			}

			gs_effect_set_texture_srgb(filter->image_param, gs_texrender_get_texture(render));
			gs_effect_set_float(filter->multiplier_param, multiplier);

			while (gs_effect_loop(filter->effect, tech_name))
			{
				gs_draw(GS_TRIS, 0, 3);
			}

			gs_texrender_end(render_unorm);

			gs_enable_blending(true);
			gs_enable_framebuffer_srgb(previous);
		}
	}

	gs_blend_state_pop();

	if (!filter->done_initial_render)
	{
		debug("render_source_to_render_tex: doing initial texture render");

		img_create_params_t params = {
			.buffer = &filter->src_img,
			.width = filter->width,
			.height = filter->height,
			.pixel_fmt = NVCV_RGBA,
			.comp_type = NVCV_U8,
			.layout = NVCV_CHUNKY,
			.alignment = 1
		};

		filter->done_initial_render = alloc_image_from_texrender(filter, &params, filter->render_unorm);
	}

    /* Perform Stitching Pass if Enabled */
    if (filter->enable_stitching && filter->stmap_loaded && filter->stitch_effect && filter->done_initial_render) {
        gs_texrender_reset(filter->stitch_render);
        if (gs_texrender_begin(filter->stitch_render, filter->width, filter->height)) {
            gs_ortho(0.0f, (float)filter->width, 0.0f, (float)filter->height, -100.0f, 100.0f);
            
            gs_effect_set_texture(filter->stitch_image_param, gs_texrender_get_texture(filter->render_unorm));
            gs_effect_set_texture(filter->stitch_remap_param, filter->stmap_texture);
            
            while (gs_effect_loop(filter->stitch_effect, "DrawStitch")) {
                gs_draw(GS_TRIS, 0, 3);
            }
            
            gs_texrender_end(filter->stitch_render);
            
            // Re-allocate src_img from the stitched result
            // OPTIMIZATION: Only re-bind if the texture handle actually changed to avoid massive overhead
            gs_texture_t *new_tex = gs_texrender_get_texture(filter->stitch_render);
            UNUSED_PARAMETER(new_tex);
            // We can't easily check the D3D pointer here without breaking abstraction, but we know src_img wraps IT.
            // Check if we already bound this specific texture object? 
            // Better strategy: Only do this call IF we haven't done it for this resolution/setup.
            // But gs_texrender buffers swap?
            // Safer Optimization: Assume NvCVImage_InitFromD3D11Texture is relatively fast if resource is same?
            // Actually, we should just use a flag since we reset everything on resize.
            
            if (!filter->stitching_bound) {
                 img_create_params_t params = {
                    .buffer = &filter->src_img,
                    .width = filter->width,
                    .height = filter->height,
                    .pixel_fmt = NVCV_RGBA,
                    .comp_type = NVCV_U8,
                    .layout = NVCV_CHUNKY,
                    .alignment = 1
                };
                alloc_image_from_texrender(filter, &params, filter->stitch_render);
                filter->stitching_bound = true;
            }
        }
    }
}



static void nv_superres_filter_render(void *data, gs_effect_t *effect)
{
	// TODO: Consider just using the provided effect to draw the final output instead of our custom superresolution effect
	UNUSED_PARAMETER(effect);

	struct nv_superresolution_data *filter = (struct nv_superresolution_data *)data;

	if (filter->processing_stopped)
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

	obs_source_t *const target = obs_filter_get_target(filter->context);
	obs_source_t *const parent = obs_filter_get_parent(filter->context);

	/* Skip if processing of a frame hasn't yet started */
	if (!filter->is_target_valid || !target || !parent)
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

	/* We've already processed the last frame we got and we haven't seen a new one, just draw what we've already done */
	if (filter->processed_frame)
	{
		draw_superresolution(filter);
		return;
	}

	/* Ensure we've got our signal handlers setup if our source is valid */
	//if (parent && !filter->handler)
	//{
	//	filter->handler = obs_source_get_signal_handler(parent);
	//	signal_handler_connect(filter->handler, "update", nv_superres_filter_reset, filter);
	//}

	if (filter->destroy_ar)
	{
		debug("nv_superres_filter_render: Destroying AR");

		nv_destroy_fx_filter(&filter->ar_handle, &filter->gpu_ar_src_img, &filter->gpu_ar_dst_img);
		filter->destroy_ar = false;
	}

	if (filter->destroy_sr)
	{
		debug("nv_superres_filter_render: Destroying SR");

		if (filter->gpu_dst_tmp_img)
		{
			debug("nv_superres_filter_render: Destroying Upscale texture");

			NvCVImage_Destroy(filter->gpu_dst_tmp_img);
			filter->gpu_dst_tmp_img = NULL;
		}

		nv_destroy_fx_filter(&filter->sr_handle, &filter->gpu_sr_src_img, &filter->gpu_sr_dst_img);
		filter->destroy_sr = false;
	}

	if (!initialize_fx(filter))
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

	/* Skip drawing if the user has turned everything off */
	if (!filter->ar_handle && !filter->sr_handle)
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const enum gs_color_space preferred_spaces[] =
	{
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
        /*
        GS_CS_2100_PQ,
        GS_CS_2100_HLG,
        */
	};

	const enum gs_color_space source_space = obs_source_get_color_space(target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	if (filter->space != source_space || !filter->are_images_allocated)
	{
		debug("nv_superres_filter_render: Initializing Images");

		filter->space = source_space;
		if (!init_images(filter))
		{
			obs_source_skip_video_filter(filter->context);
			return;
		}
	}

	if (!reload_fx(filter))
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

		/* We're waiting for the source to report a valid size for the render textures to be ready. We cannot continue until they are. */
	if (!filter->render)
	{
		obs_source_skip_video_filter(filter->context);
		return;
	}

	const uint32_t target_flags = obs_source_get_output_flags(target);
	bool async = (target_flags & OBS_SOURCE_ASYNC) != 0;

	/* Render our source out to the render texture, getting it ready for the pipeline */
	render_source_to_render_tex(filter, target, parent);

	/* If we actually have a valid texture to render, process it and draw it */
	if (filter->done_initial_render && filter->are_images_allocated)
	{
		bool draw = true;

		/* limit processing of the video frames to the main source instance, and only when there's actually a new frame */
		if (!async || filter->got_new_frame)
		{
			filter->got_new_frame = false;
			draw = process_texture_superres(filter);
		}

		if (draw)
		{
			filter->processed_frame = true;
			draw_superresolution(filter);
		}
	}
	else
	{
		obs_source_skip_video_filter(filter->context);
	}
}



static enum gs_color_space nv_superres_filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] =
	{
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
        /*
        GS_CS_2100_PQ,
        GS_CS_2100_HLG,
        */
	};

	struct nv_superresolution_data *const filter = (struct nv_superresolution_data *)data;

	const enum gs_color_space source_space = obs_source_get_color_space(obs_filter_get_target(filter->context), OBS_COUNTOF(potential_spaces), potential_spaces);

	enum gs_color_space space = source_space;

	for (size_t i = 0; i < count; ++i)
	{
		space = preferred_spaces[i];

		if (space == source_space)
		{
			break;
		}
	}

	return space;
}



static uint32_t nv_superres_filter_width(void *data)
{
	struct nv_superresolution_data *const filter = (struct nv_superresolution_data *)data;

	return (filter->type != S_TYPE_NONE && filter->is_target_valid && !filter->processing_stopped) ? filter->out_width : filter->target_width;
}



static uint32_t nv_superres_filter_height(void *data)
{
	struct nv_superresolution_data *const filter = (struct nv_superresolution_data *)data;
	
	return (filter->type != S_TYPE_NONE && filter->is_target_valid && !filter->processing_stopped) ? filter->out_height : filter->target_height;
}



static const char *nv_superres_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return TEXT_OBS_FILTER_NAME;
}



static void nv_superres_filter_activate(void *data)
{
	struct nv_superresolution_data *const filter = (struct nv_superresolution_data *)data;

	filter->processed_frame = false;
	filter->got_new_frame = true;
}



struct obs_source_info nvidia_superresolution_filter_info = {
	.id = "nv_superresolution_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = nv_superres_filter_name,
	.create = nv_superres_filter_create,
	.destroy = nv_superres_filter_destroy,
	.get_defaults = nv_superres_filter_defaults,
	.get_properties = nv_superres_filter_properties,
	.update = nv_superres_filter_update,
	.filter_video = nv_superres_filter_video,
	.video_render = nv_superres_filter_render,
	.video_tick = nv_superres_filter_tick,
	.video_get_color_space = nv_superres_filter_get_color_space,
	.get_width = nv_superres_filter_width,
	.get_height = nv_superres_filter_height,
	.activate = nv_superres_filter_activate,
	.show = nv_superres_filter_activate,
};



bool load_nv_superresolution_filter(void)
{
	debug("load_nv_superresolution_filter: Entering");
	const char *cstr = NULL;
	NvCV_Status err = NvVFX_GetString(NULL, NVVFX_INFO, &cstr);

	nvvfx_loaded = err == NVCV_SUCCESS;

	if (nvvfx_loaded)
	{
		if (cstr != NULL && strnlen_s(cstr, 3) > 1)
		{
			nvvfx_supports_ar = strstr(cstr, NVVFX_FX_ARTIFACT_REDUCTION) != NULL;
			nvvfx_supports_sr = strstr(cstr, NVVFX_FX_SUPER_RES) != NULL;
			nvvfx_supports_up = strstr(cstr, NVVFX_FX_SR_UPSCALE) != NULL;
		}
		obs_register_source(&nvidia_superresolution_filter_info);
	}
	else
	{
		if (err == NVCV_ERR_LIBRARY)
		{
			info("[NVIDIA VIDEO FX SUPERRES]: Could not load NVVFX Library, please download the video effects SDK for your GPU https://www.nvidia.com/en-us/geforce/broadcasting/broadcast-sdk/resources/");
		}
		else if (err == NVCV_ERR_UNSUPPORTEDGPU)
		{
			info("[NVIDIA VIDEO FX SUPERRES]: Unsupported GPU");
		}
		else
		{
			info("[NVIDIA VIDEO FX SUPERRES]: Error %i", err);
		}
	}

	debug("load_nv_superresolution_filter: exiting");
	return nvvfx_loaded;
}
