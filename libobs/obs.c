/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <inttypes.h>

#include "graphics/matrix4.h"
#include "callback/calldata.h"

#include "obs.h"
#include "obs-internal.h"

struct obs_core *obs = NULL;

static THREAD_LOCAL bool is_ui_thread = false;

extern void add_default_module_paths(void);
extern char *find_libobs_data_file(const char *file);

static inline void make_video_info(struct video_output_info *vi, struct obs_video_info *ovi)
{
	vi->name = "video";
	vi->format = ovi->output_format;
	vi->fps_num = ovi->fps_num;
	vi->fps_den = ovi->fps_den;
	vi->width = ovi->output_width;
	vi->height = ovi->output_height;
	vi->range = ovi->range;
	vi->colorspace = ovi->colorspace;
	vi->cache_size = 6;
}

static inline void calc_gpu_conversion_sizes(struct obs_core_video_mix *video)
{
	const struct video_output_info *info = video_output_get_info(video->video);

	video->conversion_needed = false;
	video->conversion_techs[0] = NULL;
	video->conversion_techs[1] = NULL;
	video->conversion_techs[2] = NULL;
	video->conversion_width_i = 0.f;
	video->conversion_height_i = 0.f;

	switch (info->format) {
	case VIDEO_FORMAT_I420:
		video->conversion_needed = true;
		video->conversion_techs[0] = "Planar_Y";
		video->conversion_techs[1] = "Planar_U_Left";
		video->conversion_techs[2] = "Planar_V_Left";
		video->conversion_width_i = 1.f / (float)info->width;
		break;
	case VIDEO_FORMAT_NV12:
		video->conversion_needed = true;
		video->conversion_techs[0] = "NV12_Y";
		video->conversion_techs[1] = "NV12_UV";
		video->conversion_width_i = 1.f / (float)info->width;
		break;
	case VIDEO_FORMAT_I444:
		video->conversion_needed = true;
		video->conversion_techs[0] = "Planar_Y";
		video->conversion_techs[1] = "Planar_U";
		video->conversion_techs[2] = "Planar_V";
		break;
	case VIDEO_FORMAT_I010:
		video->conversion_needed = true;
		video->conversion_width_i = 1.f / (float)info->width;
		video->conversion_height_i = 1.f / (float)info->height;
		if (info->colorspace == VIDEO_CS_2100_PQ) {
			video->conversion_techs[0] = "I010_PQ_Y";
			video->conversion_techs[1] = "I010_PQ_U";
			video->conversion_techs[2] = "I010_PQ_V";
		} else if (info->colorspace == VIDEO_CS_2100_HLG) {
			video->conversion_techs[0] = "I010_HLG_Y";
			video->conversion_techs[1] = "I010_HLG_U";
			video->conversion_techs[2] = "I010_HLG_V";
		} else {
			video->conversion_techs[0] = "I010_SRGB_Y";
			video->conversion_techs[1] = "I010_SRGB_U";
			video->conversion_techs[2] = "I010_SRGB_V";
		}
		break;
	case VIDEO_FORMAT_P010:
		video->conversion_needed = true;
		video->conversion_width_i = 1.f / (float)info->width;
		video->conversion_height_i = 1.f / (float)info->height;
		if (info->colorspace == VIDEO_CS_2100_PQ) {
			video->conversion_techs[0] = "P010_PQ_Y";
			video->conversion_techs[1] = "P010_PQ_UV";
		} else if (info->colorspace == VIDEO_CS_2100_HLG) {
			video->conversion_techs[0] = "P010_HLG_Y";
			video->conversion_techs[1] = "P010_HLG_UV";
		} else {
			video->conversion_techs[0] = "P010_SRGB_Y";
			video->conversion_techs[1] = "P010_SRGB_UV";
		}
		break;
	case VIDEO_FORMAT_P216:
		video->conversion_needed = true;
		video->conversion_width_i = 1.f / (float)info->width;
		video->conversion_height_i = 1.f / (float)info->height;
		if (info->colorspace == VIDEO_CS_2100_PQ) {
			video->conversion_techs[0] = "P216_PQ_Y";
			video->conversion_techs[1] = "P216_PQ_UV";
		} else if (info->colorspace == VIDEO_CS_2100_HLG) {
			video->conversion_techs[0] = "P216_HLG_Y";
			video->conversion_techs[1] = "P216_HLG_UV";
		} else {
			video->conversion_techs[0] = "P216_SRGB_Y";
			video->conversion_techs[1] = "P216_SRGB_UV";
		}
		break;
	case VIDEO_FORMAT_P416:
		video->conversion_needed = true;
		video->conversion_width_i = 1.f / (float)info->width;
		video->conversion_height_i = 1.f / (float)info->height;
		if (info->colorspace == VIDEO_CS_2100_PQ) {
			video->conversion_techs[0] = "P416_PQ_Y";
			video->conversion_techs[1] = "P416_PQ_UV";
		} else if (info->colorspace == VIDEO_CS_2100_HLG) {
			video->conversion_techs[0] = "P416_HLG_Y";
			video->conversion_techs[1] = "P416_HLG_UV";
		} else {
			video->conversion_techs[0] = "P416_SRGB_Y";
			video->conversion_techs[1] = "P416_SRGB_UV";
		}
		break;
	default:
		break;
	}
}

static bool obs_init_gpu_conversion(struct obs_core_video_mix *video)
{
	const struct video_output_info *info = video_output_get_info(video->video);

	calc_gpu_conversion_sizes(video);

	video->using_nv12_tex = info->format == VIDEO_FORMAT_NV12 ? gs_nv12_available() : false;
	video->using_p010_tex = info->format == VIDEO_FORMAT_P010 ? gs_p010_available() : false;

	if (!video->conversion_needed) {
		blog(LOG_INFO, "GPU conversion not available for format: %u", (unsigned int)info->format);
		video->gpu_conversion = false;
		video->using_nv12_tex = false;
		video->using_p010_tex = false;
		blog(LOG_INFO, "NV12 texture support not available");
		return true;
	}

	if (video->using_nv12_tex)
		blog(LOG_INFO, "NV12 texture support enabled");
	else
		blog(LOG_INFO, "NV12 texture support not available");

	if (video->using_p010_tex)
		blog(LOG_INFO, "P010 texture support enabled");
	else
		blog(LOG_INFO, "P010 texture support not available");

	video->convert_textures[0] = NULL;
	video->convert_textures[1] = NULL;
	video->convert_textures[2] = NULL;
	video->convert_textures_encode[0] = NULL;
	video->convert_textures_encode[1] = NULL;
	video->convert_textures_encode[2] = NULL;
	if (video->using_nv12_tex) {
		if (!gs_texture_create_nv12(&video->convert_textures_encode[0], &video->convert_textures_encode[1],
					    info->width, info->height, GS_RENDER_TARGET | GS_SHARED_KM_TEX)) {
			return false;
		}
	} else if (video->using_p010_tex) {
		if (!gs_texture_create_p010(&video->convert_textures_encode[0], &video->convert_textures_encode[1],
					    info->width, info->height, GS_RENDER_TARGET | GS_SHARED_KM_TEX)) {
			return false;
		}
	}

	bool success = true;

	switch (info->format) {
	case VIDEO_FORMAT_I420:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R8, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width / 2, info->height / 2, GS_R8, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[2] =
			gs_texture_create(info->width / 2, info->height / 2, GS_R8, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1] || !video->convert_textures[2])
			success = false;
		break;
	case VIDEO_FORMAT_NV12:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R8, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width / 2, info->height / 2, GS_R8G8, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1])
			success = false;
		break;
	case VIDEO_FORMAT_I444:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R8, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width, info->height, GS_R8, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[2] =
			gs_texture_create(info->width, info->height, GS_R8, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1] || !video->convert_textures[2])
			success = false;
		break;
	case VIDEO_FORMAT_I010:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R16, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width / 2, info->height / 2, GS_R16, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[2] =
			gs_texture_create(info->width / 2, info->height / 2, GS_R16, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1] || !video->convert_textures[2])
			success = false;
		break;
	case VIDEO_FORMAT_P010:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R16, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width / 2, info->height / 2, GS_RG16, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1])
			success = false;
		break;
	case VIDEO_FORMAT_P216:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R16, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width / 2, info->height, GS_RG16, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1])
			success = false;
		break;
	case VIDEO_FORMAT_P416:
		video->convert_textures[0] =
			gs_texture_create(info->width, info->height, GS_R16, 1, NULL, GS_RENDER_TARGET);
		video->convert_textures[1] =
			gs_texture_create(info->width, info->height, GS_RG16, 1, NULL, GS_RENDER_TARGET);
		if (!video->convert_textures[0] || !video->convert_textures[1])
			success = false;
		break;
	default:
		break;
	}

	if (!success) {
		for (size_t c = 0; c < NUM_CHANNELS; c++) {
			if (video->convert_textures[c]) {
				gs_texture_destroy(video->convert_textures[c]);
				video->convert_textures[c] = NULL;
			}
			if (video->convert_textures_encode[c]) {
				gs_texture_destroy(video->convert_textures_encode[c]);
				video->convert_textures_encode[c] = NULL;
			}
		}
	}

	return success;
}

static bool obs_init_gpu_copy_surfaces(struct obs_core_video_mix *video, size_t i)
{
	const struct video_output_info *info = video_output_get_info(video->video);
	switch (info->format) {
	case VIDEO_FORMAT_I420:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R8);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_R8);
		if (!video->copy_surfaces[i][1])
			return false;
		video->copy_surfaces[i][2] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_R8);
		if (!video->copy_surfaces[i][2])
			return false;
		break;
	case VIDEO_FORMAT_NV12:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R8);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_R8G8);
		if (!video->copy_surfaces[i][1])
			return false;
		break;
	case VIDEO_FORMAT_I444:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R8);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width, info->height, GS_R8);
		if (!video->copy_surfaces[i][1])
			return false;
		video->copy_surfaces[i][2] = gs_stagesurface_create(info->width, info->height, GS_R8);
		if (!video->copy_surfaces[i][2])
			return false;
		break;
	case VIDEO_FORMAT_I010:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R16);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_R16);
		if (!video->copy_surfaces[i][1])
			return false;
		video->copy_surfaces[i][2] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_R16);
		if (!video->copy_surfaces[i][2])
			return false;
		break;
	case VIDEO_FORMAT_P010:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R16);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width / 2, info->height / 2, GS_RG16);
		if (!video->copy_surfaces[i][1])
			return false;
		break;
	case VIDEO_FORMAT_P216:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R16);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width / 2, info->height, GS_RG16);
		if (!video->copy_surfaces[i][1])
			return false;
		break;
	case VIDEO_FORMAT_P416:
		video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, GS_R16);
		if (!video->copy_surfaces[i][0])
			return false;
		video->copy_surfaces[i][1] = gs_stagesurface_create(info->width, info->height, GS_RG16);
		if (!video->copy_surfaces[i][1])
			return false;
		break;
	default:
		break;
	}

	return true;
}

static bool obs_init_textures(struct obs_core_video_mix *video)
{
	const struct video_output_info *info = video_output_get_info(video->video);

	bool success = true;

	enum gs_color_format format = GS_BGRA;
	switch (info->format) {
	case VIDEO_FORMAT_I010:
	case VIDEO_FORMAT_P010:
	case VIDEO_FORMAT_I210:
	case VIDEO_FORMAT_I412:
	case VIDEO_FORMAT_YA2L:
	case VIDEO_FORMAT_P216:
	case VIDEO_FORMAT_P416:
		format = GS_RGBA16F;
		break;
	default:
		break;
	}

	for (size_t i = 0; i < NUM_TEXTURES; i++) {
#ifdef _WIN32
		if (video->using_nv12_tex) {
			video->copy_surfaces_encode[i] = gs_stagesurface_create_nv12(info->width, info->height);
			if (!video->copy_surfaces_encode[i]) {
				success = false;
				break;
			}
		} else if (video->using_p010_tex) {
			video->copy_surfaces_encode[i] = gs_stagesurface_create_p010(info->width, info->height);
			if (!video->copy_surfaces_encode[i]) {
				success = false;
				break;
			}
		}
#endif

		if (video->gpu_conversion) {
			if (!obs_init_gpu_copy_surfaces(video, i)) {
				success = false;
				break;
			}
		} else {
			video->copy_surfaces[i][0] = gs_stagesurface_create(info->width, info->height, format);
			if (!video->copy_surfaces[i][0]) {
				success = false;
				break;
			}
		}
	}

	enum gs_color_space space = GS_CS_SRGB;
	switch (info->colorspace) {
	case VIDEO_CS_2100_PQ:
	case VIDEO_CS_2100_HLG:
		space = GS_CS_709_EXTENDED;
		break;
	default:
		switch (info->format) {
		case VIDEO_FORMAT_I010:
		case VIDEO_FORMAT_P010:
		case VIDEO_FORMAT_P216:
		case VIDEO_FORMAT_P416:
			space = GS_CS_SRGB_16F;
			break;
		default:
			space = GS_CS_SRGB;
			break;
		}
		break;
	}

	video->render_texture =
		gs_texture_create(video->ovi.base_width, video->ovi.base_height, format, 1, NULL, GS_RENDER_TARGET);
	if (!video->render_texture)
		success = false;

	video->output_texture = gs_texture_create(info->width, info->height, format, 1, NULL, GS_RENDER_TARGET);
	if (!video->output_texture)
		success = false;

	if (success) {
		video->render_space = space;
	} else {
		for (size_t i = 0; i < NUM_TEXTURES; i++) {
			for (size_t c = 0; c < NUM_CHANNELS; c++) {
				if (video->copy_surfaces[i][c]) {
					gs_stagesurface_destroy(video->copy_surfaces[i][c]);
					video->copy_surfaces[i][c] = NULL;
				}
			}
#ifdef _WIN32
			if (video->copy_surfaces_encode[i]) {
				gs_stagesurface_destroy(video->copy_surfaces_encode[i]);
				video->copy_surfaces_encode[i] = NULL;
			}
#endif
		}

		if (video->render_texture) {
			gs_texture_destroy(video->render_texture);
			video->render_texture = NULL;
		}

		if (video->output_texture) {
			gs_texture_destroy(video->output_texture);
			video->output_texture = NULL;
		}
	}

	return success;
}

gs_effect_t *obs_load_effect(gs_effect_t **effect, const char *file)
{
	if (!*effect) {
		char *filename = obs_find_data_file(file);
		*effect = gs_effect_create_from_file(filename, NULL);
		bfree(filename);
	}

	return *effect;
}

static const char *shader_comp_name = "shader compilation";
static const char *obs_init_graphics_name = "obs_init_graphics";
static int obs_init_graphics(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;
	uint8_t transparent_tex_data[2 * 2 * 4] = {0};
	const uint8_t *transparent_tex = transparent_tex_data;
	struct gs_sampler_info point_sampler = {0};
	bool success = true;
	int errorcode;

	profile_start(obs_init_graphics_name);

	errorcode = gs_create(&video->graphics, ovi->graphics_module, ovi->adapter);
	if (errorcode != GS_SUCCESS) {
		profile_end(obs_init_graphics_name);

		switch (errorcode) {
		case GS_ERROR_MODULE_NOT_FOUND:
			return OBS_VIDEO_MODULE_NOT_FOUND;
		case GS_ERROR_NOT_SUPPORTED:
			return OBS_VIDEO_NOT_SUPPORTED;
		default:
			return OBS_VIDEO_FAIL;
		}
	}

	profile_start(shader_comp_name);
	gs_enter_context(video->graphics);

	char *filename = obs_find_data_file("default.effect");
	video->default_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	if (gs_get_device_type() == GS_DEVICE_OPENGL) {
		filename = obs_find_data_file("default_rect.effect");
		video->default_rect_effect = gs_effect_create_from_file(filename, NULL);
		bfree(filename);
	}

	filename = obs_find_data_file("opaque.effect");
	video->opaque_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("solid.effect");
	video->solid_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("repeat.effect");
	video->repeat_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("format_conversion.effect");
	video->conversion_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("bicubic_scale.effect");
	video->bicubic_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("lanczos_scale.effect");
	video->lanczos_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("area.effect");
	video->area_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("bilinear_lowres_scale.effect");
	video->bilinear_lowres_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	filename = obs_find_data_file("premultiplied_alpha.effect");
	video->premultiplied_alpha_effect = gs_effect_create_from_file(filename, NULL);
	bfree(filename);

	point_sampler.max_anisotropy = 1;
	video->point_sampler = gs_samplerstate_create(&point_sampler);

	obs->video.transparent_texture = gs_texture_create(2, 2, GS_RGBA, 1, &transparent_tex, 0);

	if (!video->default_effect)
		success = false;
	if (gs_get_device_type() == GS_DEVICE_OPENGL) {
		if (!video->default_rect_effect)
			success = false;
	}
	if (!video->opaque_effect)
		success = false;
	if (!video->solid_effect)
		success = false;
	if (!video->conversion_effect)
		success = false;
	if (!video->premultiplied_alpha_effect)
		success = false;
	if (!video->transparent_texture)
		success = false;
	if (!video->point_sampler)
		success = false;

	gs_leave_context();
	profile_end(shader_comp_name);
	profile_end(obs_init_graphics_name);

	return success ? OBS_VIDEO_SUCCESS : OBS_VIDEO_FAIL;
}

static inline void set_video_matrix(struct obs_core_video_mix *video, struct video_output_info *info)
{
	struct matrix4 mat;
	struct vec4 r_row;

	if (format_is_yuv(info->format)) {
		video_format_get_parameters_for_format(info->colorspace, info->range, info->format, (float *)&mat, NULL,
						       NULL);
		matrix4_inv(&mat, &mat);

		/* swap R and G */
		r_row = mat.x;
		mat.x = mat.y;
		mat.y = r_row;
	} else {
		matrix4_identity(&mat);
	}

	memcpy(video->color_matrix, &mat, sizeof(float) * 16);
}

static int obs_init_video_mix(struct obs_video_info *ovi, struct obs_core_video_mix *video)
{
	struct video_output_info vi;

	pthread_mutex_init_value(&video->gpu_encoder_mutex);

	make_video_info(&vi, ovi);
	video->ovi = *ovi;

	/* main view graphics thread drives all frame output,
	 * so share FPS settings for aux views */
	pthread_mutex_lock(&obs->video.mixes_mutex);
	size_t num = obs->video.mixes.num;
	if (num && obs->video.main_mix) {
		struct obs_video_info main_ovi = obs->video.main_mix->ovi;
		video->ovi.fps_num = main_ovi.fps_num;
		video->ovi.fps_den = main_ovi.fps_den;
	}
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	video->gpu_conversion = ovi->gpu_conversion;
	video->gpu_was_active = false;
	video->raw_was_active = false;
	video->was_active = false;

	set_video_matrix(video, &vi);

	int errorcode = video_output_open(&video->video, &vi);
	if (errorcode != VIDEO_OUTPUT_SUCCESS) {
		if (errorcode == VIDEO_OUTPUT_INVALIDPARAM) {
			blog(LOG_ERROR, "Invalid video parameters specified");
			return OBS_VIDEO_INVALID_PARAM;
		} else {
			blog(LOG_ERROR, "Could not open video output");
		}
		return OBS_VIDEO_FAIL;
	}

	if (pthread_mutex_init(&video->gpu_encoder_mutex, NULL) < 0)
		return OBS_VIDEO_FAIL;

	gs_enter_context(obs->video.graphics);

	if (video->gpu_conversion && !obs_init_gpu_conversion(video))
		return OBS_VIDEO_FAIL;
	if (!obs_init_textures(video))
		return OBS_VIDEO_FAIL;

	gs_leave_context();

	return OBS_VIDEO_SUCCESS;
}

struct obs_core_video_mix *obs_create_video_mix(struct obs_video_info *ovi)
{
	struct obs_core_video_mix *video = bzalloc(sizeof(struct obs_core_video_mix));
	if (obs_init_video_mix(ovi, video) != OBS_VIDEO_SUCCESS) {
		bfree(video);
		video = NULL;
	}
	return video;
}

static int obs_init_video(struct obs_video_info *ovi)
{
	struct obs_core_video *video = &obs->video;
	video->video_frame_interval_ns = util_mul_div64(1000000000ULL, ovi->fps_den, ovi->fps_num);
	video->video_half_frame_interval_ns = util_mul_div64(500000000ULL, ovi->fps_den, ovi->fps_num);

	if (pthread_mutex_init(&video->task_mutex, NULL) < 0)
		return OBS_VIDEO_FAIL;
	if (pthread_mutex_init(&video->encoder_group_mutex, NULL) < 0)
		return OBS_VIDEO_FAIL;
	if (pthread_mutex_init(&video->mixes_mutex, NULL) < 0)
		return OBS_VIDEO_FAIL;

	if (!obs_view_add2(&obs->data.main_view, ovi))
		return OBS_VIDEO_FAIL;

	int errorcode;
#ifdef __APPLE__
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_set_qos_class_np(&attr, QOS_CLASS_USER_INTERACTIVE, 0);
	errorcode = pthread_create(&video->video_thread, &attr, obs_graphics_thread_autorelease, obs);
#else
	errorcode = pthread_create(&video->video_thread, NULL, obs_graphics_thread, obs);
#endif
	if (errorcode != 0)
		return OBS_VIDEO_FAIL;

	video->thread_initialized = true;

	return OBS_VIDEO_SUCCESS;
}

static void stop_video(void)
{
	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++)
		video_output_stop(obs->video.mixes.array[i]->video);
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	struct obs_core_video *video = &obs->video;
	void *thread_retval;

	if (video->thread_initialized) {
		pthread_join(video->video_thread, &thread_retval);
		video->thread_initialized = false;
	}
}

static void obs_free_render_textures(struct obs_core_video_mix *video)
{
	if (!obs->video.graphics)
		return;

	gs_enter_context(obs->video.graphics);

	for (size_t c = 0; c < NUM_CHANNELS; c++) {
		if (video->mapped_surfaces[c]) {
			gs_stagesurface_unmap(video->mapped_surfaces[c]);
			video->mapped_surfaces[c] = NULL;
		}
	}

	for (size_t i = 0; i < NUM_TEXTURES; i++) {
		for (size_t c = 0; c < NUM_CHANNELS; c++) {
			if (video->copy_surfaces[i][c]) {
				gs_stagesurface_destroy(video->copy_surfaces[i][c]);
				video->copy_surfaces[i][c] = NULL;
			}

			video->active_copy_surfaces[i][c] = NULL;
		}
#ifdef _WIN32
		if (video->copy_surfaces_encode[i]) {
			gs_stagesurface_destroy(video->copy_surfaces_encode[i]);
			video->copy_surfaces_encode[i] = NULL;
		}
#endif
	}

	gs_texture_destroy(video->render_texture);

	for (size_t c = 0; c < NUM_CHANNELS; c++) {
		if (video->convert_textures[c]) {
			gs_texture_destroy(video->convert_textures[c]);
			video->convert_textures[c] = NULL;
		}
		if (video->convert_textures_encode[c]) {
			gs_texture_destroy(video->convert_textures_encode[c]);
			video->convert_textures_encode[c] = NULL;
		}
	}

	gs_texture_destroy(video->output_texture);
	video->render_texture = NULL;
	video->output_texture = NULL;

	gs_leave_context();
}

void obs_free_video_mix(struct obs_core_video_mix *video)
{
	if (video->video) {
		video_output_close(video->video);
		video->video = NULL;

		obs_free_render_textures(video);

		deque_free(&video->vframe_info_buffer);
		deque_free(&video->vframe_info_buffer_gpu);

		video->texture_rendered = false;
		memset(video->textures_copied, 0, sizeof(video->textures_copied));
		video->texture_converted = false;

		pthread_mutex_destroy(&video->gpu_encoder_mutex);
		pthread_mutex_init_value(&video->gpu_encoder_mutex);
		da_free(video->gpu_encoders);

		video->gpu_encoder_active = 0;
		video->cur_texture = 0;
	}
	bfree(video);
}

static void obs_free_video(void)
{
	pthread_mutex_lock(&obs->video.mixes_mutex);
	size_t num_views = 0;
	for (size_t i = 0; i < obs->video.mixes.num; i++) {
		struct obs_core_video_mix *video = obs->video.mixes.array[i];
		if (video && video->view)
			num_views++;
		obs_free_video_mix(video);
		obs->video.mixes.array[i] = NULL;
	}
	da_free(obs->video.mixes);
	if (num_views > 0)
		blog(LOG_WARNING, "Number of remaining views: %ld", num_views);
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	pthread_mutex_destroy(&obs->video.mixes_mutex);
	pthread_mutex_init_value(&obs->video.mixes_mutex);

	for (size_t i = 0; i < obs->video.ready_encoder_groups.num; i++) {
		obs_weak_encoder_release(obs->video.ready_encoder_groups.array[i]);
	}
	da_free(obs->video.ready_encoder_groups);

	pthread_mutex_destroy(&obs->video.encoder_group_mutex);
	pthread_mutex_init_value(&obs->video.encoder_group_mutex);

	pthread_mutex_destroy(&obs->video.task_mutex);
	pthread_mutex_init_value(&obs->video.task_mutex);
	deque_free(&obs->video.tasks);
}

static void obs_free_graphics(void)
{
	struct obs_core_video *video = &obs->video;

	if (video->graphics) {
		gs_enter_context(video->graphics);

		gs_texture_destroy(video->transparent_texture);

		gs_samplerstate_destroy(video->point_sampler);

		gs_effect_destroy(video->default_effect);
		gs_effect_destroy(video->default_rect_effect);
		gs_effect_destroy(video->opaque_effect);
		gs_effect_destroy(video->solid_effect);
		gs_effect_destroy(video->conversion_effect);
		gs_effect_destroy(video->bicubic_effect);
		gs_effect_destroy(video->repeat_effect);
		gs_effect_destroy(video->lanczos_effect);
		gs_effect_destroy(video->area_effect);
		gs_effect_destroy(video->bilinear_lowres_effect);
		video->default_effect = NULL;

		gs_leave_context();

		gs_destroy(video->graphics);
		video->graphics = NULL;
	}
}

static void set_audio_thread(void *unused);

static bool obs_init_audio(struct audio_output_info *ai)
{
	struct obs_core_audio *audio = &obs->audio;
	int errorcode;

	pthread_mutex_init_value(&audio->monitoring_mutex);

	if (pthread_mutex_init_recursive(&audio->monitoring_mutex) != 0)
		return false;
	if (pthread_mutex_init(&audio->task_mutex, NULL) != 0)
		return false;

	struct obs_task_info audio_init = {.task = set_audio_thread};
	deque_push_back(&audio->tasks, &audio_init, sizeof(audio_init));

	audio->monitoring_device_name = bstrdup("Default");
	audio->monitoring_device_id = bstrdup("default");

	errorcode = audio_output_open(&audio->audio, ai);
	if (errorcode == AUDIO_OUTPUT_SUCCESS)
		return true;
	else if (errorcode == AUDIO_OUTPUT_INVALIDPARAM)
		blog(LOG_ERROR, "Invalid audio parameters specified");
	else
		blog(LOG_ERROR, "Could not open audio output");

	return false;
}

static void stop_audio(void)
{
	struct obs_core_audio *audio = &obs->audio;

	if (audio->audio) {
		audio_output_close(audio->audio);
		audio->audio = NULL;
	}
}

static void obs_free_audio(void)
{
	struct obs_core_audio *audio = &obs->audio;
	if (audio->audio)
		audio_output_close(audio->audio);

	deque_free(&audio->buffered_timestamps);
	da_free(audio->render_order);
	da_free(audio->root_nodes);

	da_free(audio->monitors);
	bfree(audio->monitoring_device_name);
	bfree(audio->monitoring_device_id);
	deque_free(&audio->tasks);
	pthread_mutex_destroy(&audio->task_mutex);
	pthread_mutex_destroy(&audio->monitoring_mutex);

	memset(audio, 0, sizeof(struct obs_core_audio));
}

static bool obs_init_data(void)
{
	struct obs_core_data *data = &obs->data;

	assert(data != NULL);

	pthread_mutex_init_value(&obs->data.displays_mutex);
	pthread_mutex_init_value(&obs->data.draw_callbacks_mutex);

	if (pthread_mutex_init_recursive(&data->sources_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&data->audio_sources_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&data->displays_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&data->outputs_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&data->encoders_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&data->services_mutex) != 0)
		goto fail;
	if (pthread_mutex_init_recursive(&obs->data.draw_callbacks_mutex) != 0)
		goto fail;

	if (!obs_view_init(&data->main_view))
		goto fail;

	data->sources = NULL;
	data->public_sources = NULL;
	data->private_data = obs_data_create();
	data->valid = true;

fail:
	return data->valid;
}

void obs_main_view_free(struct obs_view *view)
{
	if (!view)
		return;

	for (size_t i = 0; i < MAX_CHANNELS; i++)
		obs_source_release(view->channels[i]);

	memset(view->channels, 0, sizeof(view->channels));
	pthread_mutex_destroy(&view->channels_mutex);
}

#define FREE_OBS_HASH_TABLE(handle, table, type)                                     \
	do {                                                                         \
		struct obs_context_data *ctx, *tmp;                                  \
		int unfreed = 0;                                                     \
		HASH_ITER (handle, *(struct obs_context_data **)table, ctx, tmp) {   \
			obs_##type##_destroy((obs_##type##_t *)ctx);                 \
			unfreed++;                                                   \
		}                                                                    \
		if (unfreed)                                                         \
			blog(LOG_INFO, "\t%d " #type "(s) were remaining", unfreed); \
	} while (false)

#define FREE_OBS_LINKED_LIST(type)                                                   \
	do {                                                                         \
		int unfreed = 0;                                                     \
		while (data->first_##type) {                                         \
			obs_##type##_destroy(data->first_##type);                    \
			unfreed++;                                                   \
		}                                                                    \
		if (unfreed)                                                         \
			blog(LOG_INFO, "\t%d " #type "(s) were remaining", unfreed); \
	} while (false)

static void obs_free_data(void)
{
	struct obs_core_data *data = &obs->data;

	data->valid = false;

	obs_view_remove(&data->main_view);
	obs_main_view_free(&data->main_view);

	blog(LOG_INFO, "Freeing OBS context data");

	FREE_OBS_LINKED_LIST(output);
	FREE_OBS_LINKED_LIST(encoder);
	FREE_OBS_LINKED_LIST(display);
	FREE_OBS_LINKED_LIST(service);

	FREE_OBS_HASH_TABLE(hh, &data->public_sources, source);
	FREE_OBS_HASH_TABLE(hh_uuid, &data->sources, source);

	os_task_queue_wait(obs->destruction_task_thread);

	pthread_mutex_destroy(&data->sources_mutex);
	pthread_mutex_destroy(&data->audio_sources_mutex);
	pthread_mutex_destroy(&data->displays_mutex);
	pthread_mutex_destroy(&data->outputs_mutex);
	pthread_mutex_destroy(&data->encoders_mutex);
	pthread_mutex_destroy(&data->services_mutex);
	pthread_mutex_destroy(&data->draw_callbacks_mutex);
	da_free(data->draw_callbacks);
	da_free(data->rendered_callbacks);
	da_free(data->tick_callbacks);
	obs_data_release(data->private_data);

	for (size_t i = 0; i < data->protocols.num; i++)
		bfree(data->protocols.array[i]);
	da_free(data->protocols);
	da_free(data->sources_to_tick);
}

static const char *obs_signals[] = {
	"void source_create(ptr source)",
	"void source_destroy(ptr source)",
	"void source_remove(ptr source)",
	"void source_update(ptr source)",
	"void source_save(ptr source)",
	"void source_load(ptr source)",
	"void source_activate(ptr source)",
	"void source_deactivate(ptr source)",
	"void source_show(ptr source)",
	"void source_hide(ptr source)",
	"void source_audio_activate(ptr source)",
	"void source_audio_deactivate(ptr source)",
	"void source_filter_add(ptr source, ptr filter)",
	"void source_filter_remove(ptr source, ptr filter)",
	"void source_rename(ptr source, string new_name, string prev_name)",
	"void source_volume(ptr source, in out float volume)",
	"void source_volume_level(ptr source, float level, float magnitude, "
	"float peak)",
	"void source_transition_start(ptr source)",
	"void source_transition_video_stop(ptr source)",
	"void source_transition_stop(ptr source)",

	"void channel_change(int channel, in out ptr source, ptr prev_source)",

	"void hotkey_layout_change()",
	"void hotkey_register(ptr hotkey)",
	"void hotkey_unregister(ptr hotkey)",
	"void hotkey_bindings_changed(ptr hotkey)",

	NULL,
};

static inline bool obs_init_handlers(void)
{
	obs->signals = signal_handler_create();
	if (!obs->signals)
		return false;

	obs->procs = proc_handler_create();
	if (!obs->procs)
		return false;

	return signal_handler_add_array(obs->signals, obs_signals);
}

static pthread_once_t obs_pthread_once_init_token = PTHREAD_ONCE_INIT;
static inline bool obs_init_hotkeys(void)
{
	struct obs_core_hotkeys *hotkeys = &obs->hotkeys;
	bool success = false;

	assert(hotkeys != NULL);

	hotkeys->hotkeys = NULL;
	hotkeys->hotkey_pairs = NULL;
	hotkeys->signals = obs->signals;
	hotkeys->name_map_init_token = obs_pthread_once_init_token;
	hotkeys->mute = bstrdup("Mute");
	hotkeys->unmute = bstrdup("Unmute");
	hotkeys->push_to_mute = bstrdup("Push-to-mute");
	hotkeys->push_to_talk = bstrdup("Push-to-talk");
	hotkeys->sceneitem_show = bstrdup("Show '%1'");
	hotkeys->sceneitem_hide = bstrdup("Hide '%1'");

	if (!obs_hotkeys_platform_init(hotkeys))
		return false;

	if (pthread_mutex_init_recursive(&hotkeys->mutex) != 0)
		goto fail;

	if (os_event_init(&hotkeys->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;
	if (pthread_create(&hotkeys->hotkey_thread, NULL, obs_hotkey_thread, NULL))
		goto fail;

	hotkeys->strict_modifiers = true;
	hotkeys->hotkey_thread_initialized = true;

	success = true;

fail:
	return success;
}

static inline void stop_hotkeys(void)
{
	struct obs_core_hotkeys *hotkeys = &obs->hotkeys;
	void *thread_ret;

	if (hotkeys->hotkey_thread_initialized) {
		os_event_signal(hotkeys->stop_event);
		pthread_join(hotkeys->hotkey_thread, &thread_ret);
		hotkeys->hotkey_thread_initialized = false;
	}

	os_event_destroy(hotkeys->stop_event);
	obs_hotkeys_free();
}

static inline void obs_free_hotkeys(void)
{
	struct obs_core_hotkeys *hotkeys = &obs->hotkeys;

	bfree(hotkeys->mute);
	bfree(hotkeys->unmute);
	bfree(hotkeys->push_to_mute);
	bfree(hotkeys->push_to_talk);
	bfree(hotkeys->sceneitem_show);
	bfree(hotkeys->sceneitem_hide);

	obs_hotkey_name_map_free();

	obs_hotkeys_platform_free(hotkeys);
	pthread_mutex_destroy(&hotkeys->mutex);
}

extern const struct obs_source_info scene_info;
extern const struct obs_source_info group_info;

static const char *submix_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Audio line (internal use only)";
}

const struct obs_source_info audio_line_info = {
	.id = "audio_line",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_CAP_DISABLED | OBS_SOURCE_SUBMIX,
	.get_name = submix_name,
};

extern void log_system_info(void);

static bool obs_init(const char *locale, const char *module_config_path, profiler_name_store_t *store)
{
	obs = bzalloc(sizeof(struct obs_core));

	pthread_mutex_init_value(&obs->audio.monitoring_mutex);
	pthread_mutex_init_value(&obs->audio.task_mutex);
	pthread_mutex_init_value(&obs->video.task_mutex);
	pthread_mutex_init_value(&obs->video.encoder_group_mutex);
	pthread_mutex_init_value(&obs->video.mixes_mutex);

	obs->name_store_owned = !store;
	obs->name_store = store ? store : profiler_name_store_create();
	if (!obs->name_store) {
		blog(LOG_ERROR, "Couldn't create profiler name store");
		return false;
	}

	log_system_info();

	if (!obs_init_data())
		return false;
	if (!obs_init_handlers())
		return false;
	if (!obs_init_hotkeys())
		return false;

	obs->destruction_task_thread = os_task_queue_create();
	if (!obs->destruction_task_thread)
		return false;

	if (module_config_path)
		obs->module_config_path = bstrdup(module_config_path);
	obs->locale = bstrdup(locale);
	obs_register_source(&scene_info);
	obs_register_source(&group_info);
	obs_register_source(&audio_line_info);
	add_default_module_paths();
	return true;
}

#ifdef _WIN32
extern bool initialize_com(void);
extern void uninitialize_com(void);
static bool com_initialized = false;
#endif

/* Separate from actual context initialization
 * since this can be set before startup and persist
 * after shutdown. */
static DARRAY(struct dstr) core_module_paths = {0};

char *obs_find_data_file(const char *file)
{
	struct dstr path = {0};

	char *result = find_libobs_data_file(file);
	if (result)
		return result;

	for (size_t i = 0; i < core_module_paths.num; ++i) {
		if (check_path(file, core_module_paths.array[i].array, &path))
			return path.array;
	}

	blog(LOG_ERROR, "Failed to find file '%s' in libobs data directory", file);

	dstr_free(&path);
	return NULL;
}

void obs_add_data_path(const char *path)
{
	struct dstr *new_path = da_push_back_new(core_module_paths);
	dstr_init_copy(new_path, path);
}

bool obs_remove_data_path(const char *path)
{
	for (size_t i = 0; i < core_module_paths.num; ++i) {
		int result = dstr_cmp(&core_module_paths.array[i], path);

		if (result == 0) {
			dstr_free(&core_module_paths.array[i]);
			da_erase(core_module_paths, i);
			return true;
		}
	}

	return false;
}

static const char *obs_startup_name = "obs_startup";
bool obs_startup(const char *locale, const char *module_config_path, profiler_name_store_t *store)
{
	bool success;

	profile_start(obs_startup_name);

	if (obs) {
		blog(LOG_WARNING, "Tried to call obs_startup more than once");
		return false;
	}

#ifdef _WIN32
	com_initialized = initialize_com();
#endif

	success = obs_init(locale, module_config_path, store);
	profile_end(obs_startup_name);
	if (!success)
		obs_shutdown();

	return success;
}

static struct obs_cmdline_args cmdline_args = {0, NULL};
void obs_set_cmdline_args(int argc, const char *const *argv)
{
	char *data;
	size_t len;
	int i;

	/* Once argc is set (non-zero) we shouldn't call again */
	if (cmdline_args.argc)
		return;

	cmdline_args.argc = argc;

	/* Safely copy over argv */
	len = 0;
	for (i = 0; i < argc; i++)
		len += strlen(argv[i]) + 1;

	cmdline_args.argv = bmalloc(sizeof(char *) * (argc + 1) + len);
	data = (char *)cmdline_args.argv + sizeof(char *) * (argc + 1);

	for (i = 0; i < argc; i++) {
		cmdline_args.argv[i] = data;
		len = strlen(argv[i]) + 1;
		memcpy(data, argv[i], len);
		data += len;
	}

	cmdline_args.argv[argc] = NULL;
}

struct obs_cmdline_args obs_get_cmdline_args(void)
{
	return cmdline_args;
}

void obs_shutdown(void)
{
	struct obs_module *module;

	obs_wait_for_destroy_queue();

	for (size_t i = 0; i < obs->source_types.num; i++) {
		struct obs_source_info *item = &obs->source_types.array[i];
		if (item->type_data && item->free_type_data)
			item->free_type_data(item->type_data);
		if (item->id)
			bfree((void *)item->id);
	}
	da_free(obs->source_types);

#define FREE_REGISTERED_TYPES(structure, list)                         \
	do {                                                           \
		for (size_t i = 0; i < list.num; i++) {                \
			struct structure *item = &list.array[i];       \
			if (item->type_data && item->free_type_data)   \
				item->free_type_data(item->type_data); \
		}                                                      \
		da_free(list);                                         \
	} while (false)

	FREE_REGISTERED_TYPES(obs_output_info, obs->output_types);
	FREE_REGISTERED_TYPES(obs_encoder_info, obs->encoder_types);
	FREE_REGISTERED_TYPES(obs_service_info, obs->service_types);

#undef FREE_REGISTERED_TYPES

	da_free(obs->input_types);
	da_free(obs->filter_types);
	da_free(obs->transition_types);

	stop_video();
	stop_audio();
	stop_hotkeys();

	module = obs->first_module;
	while (module) {
		struct obs_module *next = module->next;
		free_module(module);
		module = next;
	}
	obs->first_module = NULL;

	obs_free_data();
	obs_free_audio();
	obs_free_video();
	os_task_queue_destroy(obs->destruction_task_thread);
	obs_free_hotkeys();
	obs_free_graphics();
	proc_handler_destroy(obs->procs);
	signal_handler_destroy(obs->signals);
	obs->procs = NULL;
	obs->signals = NULL;

	for (size_t i = 0; i < obs->module_paths.num; i++)
		free_module_path(obs->module_paths.array + i);
	da_free(obs->module_paths);

	for (size_t i = 0; i < obs->safe_modules.num; i++)
		bfree(obs->safe_modules.array[i]);
	da_free(obs->safe_modules);

	if (obs->name_store_owned)
		profiler_name_store_free(obs->name_store);

	bfree(obs->module_config_path);
	bfree(obs->locale);
	bfree(obs);
	obs = NULL;
	bfree(cmdline_args.argv);

#ifdef _WIN32
	if (com_initialized)
		uninitialize_com();
#endif
}

bool obs_initialized(void)
{
	return obs != NULL;
}

uint32_t obs_get_version(void)
{
	return LIBOBS_API_VER;
}

const char *obs_get_version_string(void)
{
	return OBS_VERSION;
}

void obs_set_locale(const char *locale)
{
	struct obs_module *module;

	if (obs->locale)
		bfree(obs->locale);
	obs->locale = bstrdup(locale);

	module = obs->first_module;
	while (module) {
		if (module->set_locale)
			module->set_locale(locale);

		module = module->next;
	}
}

const char *obs_get_locale(void)
{
	return obs->locale;
}

#define OBS_SIZE_MIN 2
#define OBS_SIZE_MAX (32 * 1024)

static inline bool size_valid(uint32_t width, uint32_t height)
{
	return (width >= OBS_SIZE_MIN && height >= OBS_SIZE_MIN && width <= OBS_SIZE_MAX && height <= OBS_SIZE_MAX);
}

int obs_reset_video(struct obs_video_info *ovi)
{
	if (!obs)
		return OBS_VIDEO_FAIL;

	/* don't allow changing of video settings if active. */
	if (obs_video_active())
		return OBS_VIDEO_CURRENTLY_ACTIVE;

	if (!size_valid(ovi->output_width, ovi->output_height) || !size_valid(ovi->base_width, ovi->base_height))
		return OBS_VIDEO_INVALID_PARAM;

	stop_video();
	obs_free_video();

	/* align to multiple-of-two and SSE alignment sizes */
	ovi->output_width &= 0xFFFFFFFC;
	ovi->output_height &= 0xFFFFFFFE;

	if (!obs->video.graphics) {
		int errorcode = obs_init_graphics(ovi);
		if (errorcode != OBS_VIDEO_SUCCESS) {
			obs_free_graphics();
			return errorcode;
		}
	}

	const char *scale_type_name = "";
	switch (ovi->scale_type) {
	case OBS_SCALE_DISABLE:
		scale_type_name = "Disabled";
		break;
	case OBS_SCALE_POINT:
		scale_type_name = "Point";
		break;
	case OBS_SCALE_BICUBIC:
		scale_type_name = "Bicubic";
		break;
	case OBS_SCALE_BILINEAR:
		scale_type_name = "Bilinear";
		break;
	case OBS_SCALE_LANCZOS:
		scale_type_name = "Lanczos";
		break;
	case OBS_SCALE_AREA:
		scale_type_name = "Area";
		break;
	}

	bool yuv = format_is_yuv(ovi->output_format);
	const char *yuv_format = get_video_colorspace_name(ovi->colorspace);
	const char *yuv_range = get_video_range_name(ovi->output_format, ovi->range);

	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO,
	     "video settings reset:\n"
	     "\tbase resolution:   %dx%d\n"
	     "\toutput resolution: %dx%d\n"
	     "\tdownscale filter:  %s\n"
	     "\tfps:               %d/%d\n"
	     "\tformat:            %s\n"
	     "\tYUV mode:          %s%s%s",
	     ovi->base_width, ovi->base_height, ovi->output_width, ovi->output_height, scale_type_name, ovi->fps_num,
	     ovi->fps_den, get_video_format_name(ovi->output_format), yuv ? yuv_format : "None", yuv ? "/" : "",
	     yuv ? yuv_range : "");

	source_profiler_reset_video(ovi);

	return obs_init_video(ovi);
}

#ifndef SEC_TO_MSEC
#define SEC_TO_MSEC 1000
#endif

bool obs_reset_audio2(const struct obs_audio_info2 *oai)
{
	struct obs_core_audio *audio = &obs->audio;
	struct audio_output_info ai;

	/* don't allow changing of audio settings if active. */
	if (!obs || (audio->audio && audio_output_active(audio->audio)))
		return false;

	obs_free_audio();
	if (!oai)
		return true;

	if (oai->max_buffering_ms) {
		uint32_t max_frames = oai->max_buffering_ms * oai->samples_per_sec / SEC_TO_MSEC;
		max_frames += (AUDIO_OUTPUT_FRAMES - 1);
		audio->max_buffering_ticks = max_frames / AUDIO_OUTPUT_FRAMES;
	} else {
		audio->max_buffering_ticks = 45;
	}
	audio->fixed_buffer = oai->fixed_buffering;

	int max_buffering_ms =
		audio->max_buffering_ticks * AUDIO_OUTPUT_FRAMES * SEC_TO_MSEC / (int)oai->samples_per_sec;

	ai.name = "Audio";
	ai.samples_per_sec = oai->samples_per_sec;
	ai.format = AUDIO_FORMAT_FLOAT_PLANAR;
	ai.speakers = oai->speakers;
	ai.input_callback = audio_callback;

	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO,
	     "audio settings reset:\n"
	     "\tsamples per sec: %d\n"
	     "\tspeakers:        %d\n"
	     "\tmax buffering:   %d milliseconds\n"
	     "\tbuffering type:  %s",
	     (int)ai.samples_per_sec, (int)ai.speakers, max_buffering_ms,
	     oai->fixed_buffering ? "fixed" : "dynamically increasing");

	return obs_init_audio(&ai);
}

bool obs_reset_audio(const struct obs_audio_info *oai)
{
	struct obs_audio_info2 oai2 = {
		.samples_per_sec = oai->samples_per_sec,
		.speakers = oai->speakers,
	};

	return obs_reset_audio2(&oai2);
}

bool obs_get_video_info(struct obs_video_info *ovi)
{
	if (!obs->video.graphics || !obs->video.main_mix)
		return false;

	*ovi = obs->video.main_mix->ovi;
	return true;
}

float obs_get_video_sdr_white_level(void)
{
	struct obs_core_video *video = &obs->video;
	return video->graphics ? video->sdr_white_level : 300.f;
}

float obs_get_video_hdr_nominal_peak_level(void)
{
	struct obs_core_video *video = &obs->video;
	return video->graphics ? video->hdr_nominal_peak_level : 1000.f;
}

void obs_set_video_levels(float sdr_white_level, float hdr_nominal_peak_level)
{
	struct obs_core_video *video = &obs->video;
	assert(video->graphics);

	video->sdr_white_level = sdr_white_level;
	video->hdr_nominal_peak_level = hdr_nominal_peak_level;
}

bool obs_get_audio_info(struct obs_audio_info *oai)
{
	struct obs_core_audio *audio = &obs->audio;
	const struct audio_output_info *info;

	if (!oai || !audio->audio)
		return false;

	info = audio_output_get_info(audio->audio);

	oai->samples_per_sec = info->samples_per_sec;
	oai->speakers = info->speakers;
	return true;
}

bool obs_get_audio_info2(struct obs_audio_info2 *oai2)
{
	struct obs_core_audio *audio = &obs->audio;
	struct obs_audio_info oai;

	if (!obs_get_audio_info(&oai) || !oai2 || !audio->audio) {
		return false;
	} else {
		oai2->samples_per_sec = oai.samples_per_sec;
		oai2->speakers = oai.speakers;
		oai2->fixed_buffering = audio->fixed_buffer;
		oai2->max_buffering_ms =
			audio->max_buffering_ticks * AUDIO_OUTPUT_FRAMES * SEC_TO_MSEC / (int)oai2->samples_per_sec;
		return true;
	}
}

bool obs_enum_source_types(size_t idx, const char **id)
{
	if (idx >= obs->source_types.num)
		return false;
	*id = obs->source_types.array[idx].id;
	return true;
}

bool obs_enum_input_types(size_t idx, const char **id)
{
	if (idx >= obs->input_types.num)
		return false;
	*id = obs->input_types.array[idx].id;
	return true;
}

bool obs_enum_input_types2(size_t idx, const char **id, const char **unversioned_id)
{
	if (idx >= obs->input_types.num)
		return false;
	if (id)
		*id = obs->input_types.array[idx].id;
	if (unversioned_id)
		*unversioned_id = obs->input_types.array[idx].unversioned_id;
	return true;
}

const char *obs_get_latest_input_type_id(const char *unversioned_id)
{
	struct obs_source_info *latest = NULL;
	int version = -1;

	if (!unversioned_id)
		return NULL;

	for (size_t i = 0; i < obs->source_types.num; i++) {
		struct obs_source_info *info = &obs->source_types.array[i];
		if (strcmp(info->unversioned_id, unversioned_id) == 0 && (int)info->version > version) {
			latest = info;
			version = info->version;
		}
	}

	assert(!!latest);
	if (!latest)
		return NULL;

	return latest->id;
}

bool obs_enum_filter_types(size_t idx, const char **id)
{
	if (idx >= obs->filter_types.num)
		return false;
	*id = obs->filter_types.array[idx].id;
	return true;
}

bool obs_enum_transition_types(size_t idx, const char **id)
{
	if (idx >= obs->transition_types.num)
		return false;
	*id = obs->transition_types.array[idx].id;
	return true;
}

bool obs_enum_output_types(size_t idx, const char **id)
{
	if (idx >= obs->output_types.num)
		return false;
	*id = obs->output_types.array[idx].id;
	return true;
}

bool obs_enum_encoder_types(size_t idx, const char **id)
{
	if (idx >= obs->encoder_types.num)
		return false;
	*id = obs->encoder_types.array[idx].id;
	return true;
}

bool obs_enum_service_types(size_t idx, const char **id)
{
	if (idx >= obs->service_types.num)
		return false;
	*id = obs->service_types.array[idx].id;
	return true;
}

void obs_enter_graphics(void)
{
	if (obs->video.graphics)
		gs_enter_context(obs->video.graphics);
}

void obs_leave_graphics(void)
{
	if (obs->video.graphics)
		gs_leave_context();
}

audio_t *obs_get_audio(void)
{
	return obs->audio.audio;
}

video_t *obs_get_video(void)
{
	return obs->video.main_mix->video;
}

obs_source_t *obs_get_output_source(uint32_t channel)
{
	return obs_view_get_source(&obs->data.main_view, channel);
}

void obs_set_output_source(uint32_t channel, obs_source_t *source)
{
	assert(channel < MAX_CHANNELS);

	if (channel >= MAX_CHANNELS)
		return;

	struct obs_source *prev_source;
	struct obs_view *view = &obs->data.main_view;
	struct calldata params = {0};

	pthread_mutex_lock(&view->channels_mutex);

	source = obs_source_get_ref(source);

	prev_source = view->channels[channel];

	calldata_set_int(&params, "channel", channel);
	calldata_set_ptr(&params, "prev_source", prev_source);
	calldata_set_ptr(&params, "source", source);
	signal_handler_signal(obs->signals, "channel_change", &params);
	calldata_get_ptr(&params, "source", &source);
	calldata_free(&params);

	view->channels[channel] = source;

	pthread_mutex_unlock(&view->channels_mutex);

	if (source)
		obs_source_activate(source, MAIN_VIEW);

	if (prev_source) {
		obs_source_deactivate(prev_source, MAIN_VIEW);
		obs_source_release(prev_source);
	}
}

void obs_enum_sources(bool (*enum_proc)(void *, obs_source_t *), void *param)
{
	obs_source_t *source;

	pthread_mutex_lock(&obs->data.sources_mutex);
	source = obs->data.public_sources;

	while (source) {
		obs_source_t *s = obs_source_get_ref(source);
		if (s) {
			if (s->info.type == OBS_SOURCE_TYPE_INPUT && !enum_proc(param, s)) {
				obs_source_release(s);
				break;
			} else if (strcmp(s->info.id, group_info.id) == 0 && !enum_proc(param, s)) {
				obs_source_release(s);
				break;
			}
			obs_source_release(s);
		}

		source = (obs_source_t *)source->context.hh.next;
	}

	pthread_mutex_unlock(&obs->data.sources_mutex);
}

void obs_enum_scenes(bool (*enum_proc)(void *, obs_source_t *), void *param)
{
	obs_source_t *source;

	pthread_mutex_lock(&obs->data.sources_mutex);

	source = obs->data.public_sources;
	while (source) {
		obs_source_t *s = obs_source_get_ref(source);
		if (s) {
			if (source->info.type == OBS_SOURCE_TYPE_SCENE && !enum_proc(param, s)) {
				obs_source_release(s);
				break;
			}
			obs_source_release(s);
		}

		source = (obs_source_t *)source->context.hh.next;
	}

	pthread_mutex_unlock(&obs->data.sources_mutex);
}

static inline void obs_enum(void *pstart, pthread_mutex_t *mutex, void *proc, void *param)
{
	struct obs_context_data **start = pstart, *context;
	bool (*enum_proc)(void *, void *) = proc;

	assert(start);
	assert(mutex);
	assert(enum_proc);

	pthread_mutex_lock(mutex);

	context = *start;
	while (context) {
		if (!enum_proc(param, context))
			break;

		context = context->next;
	}

	pthread_mutex_unlock(mutex);
}

static inline void obs_enum_uuid(void *pstart, pthread_mutex_t *mutex, void *proc, void *param)
{
	struct obs_context_data **start = pstart, *context, *tmp;
	bool (*enum_proc)(void *, void *) = proc;

	assert(start);
	assert(mutex);
	assert(enum_proc);

	pthread_mutex_lock(mutex);

	HASH_ITER (hh_uuid, *start, context, tmp) {
		if (!enum_proc(param, context))
			break;
	}

	pthread_mutex_unlock(mutex);
}

void obs_enum_all_sources(bool (*enum_proc)(void *, obs_source_t *), void *param)
{
	obs_enum_uuid(&obs->data.sources, &obs->data.sources_mutex, enum_proc, param);
}

void obs_enum_outputs(bool (*enum_proc)(void *, obs_output_t *), void *param)
{
	obs_enum(&obs->data.first_output, &obs->data.outputs_mutex, enum_proc, param);
}

void obs_enum_encoders(bool (*enum_proc)(void *, obs_encoder_t *), void *param)
{
	obs_enum(&obs->data.first_encoder, &obs->data.encoders_mutex, enum_proc, param);
}

void obs_enum_services(bool (*enum_proc)(void *, obs_service_t *), void *param)
{
	obs_enum(&obs->data.first_service, &obs->data.services_mutex, enum_proc, param);
}

static inline void *get_context_by_name(void *vfirst, const char *name, pthread_mutex_t *mutex, void *(*addref)(void *))
{
	struct obs_context_data **first = vfirst;
	struct obs_context_data *context;

	pthread_mutex_lock(mutex);

	/* If context list head has a hash table, look the name up in there */
	if (*first && (*first)->hh.tbl) {
		HASH_FIND_STR(*first, name, context);
	} else {
		context = *first;
		while (context) {
			if (!context->private && strcmp(context->name, name) == 0) {
				break;
			}

			context = context->next;
		}
	}

	if (context)
		addref(context);

	pthread_mutex_unlock(mutex);
	return context;
}

static void *get_context_by_uuid(void *ptable, const char *uuid, pthread_mutex_t *mutex, void *(*addref)(void *))
{
	struct obs_context_data **ht = ptable;
	struct obs_context_data *context;

	pthread_mutex_lock(mutex);

	HASH_FIND_UUID(*ht, uuid, context);
	if (context)
		addref(context);

	pthread_mutex_unlock(mutex);
	return context;
}

static inline void *obs_source_addref_safe_(void *ref)
{
	return obs_source_get_ref(ref);
}

static inline void *obs_output_addref_safe_(void *ref)
{
	return obs_output_get_ref(ref);
}

static inline void *obs_encoder_addref_safe_(void *ref)
{
	return obs_encoder_get_ref(ref);
}

static inline void *obs_service_addref_safe_(void *ref)
{
	return obs_service_get_ref(ref);
}

obs_source_t *obs_get_source_by_name(const char *name)
{
	return get_context_by_name(&obs->data.public_sources, name, &obs->data.sources_mutex, obs_source_addref_safe_);
}

obs_source_t *obs_get_source_by_uuid(const char *uuid)
{
	return get_context_by_uuid(&obs->data.sources, uuid, &obs->data.sources_mutex, obs_source_addref_safe_);
}

obs_source_t *obs_get_transition_by_name(const char *name)
{
	struct obs_source **first = &obs->data.sources;
	struct obs_source *source;

	pthread_mutex_lock(&obs->data.sources_mutex);

	/* Transitions are private but can be found via this method, so we
	 * can't look them up by name in the public_sources hash table. */
	source = *first;
	while (source) {
		if (source->info.type == OBS_SOURCE_TYPE_TRANSITION && strcmp(source->context.name, name) == 0) {
			source = obs_source_addref_safe_(source);
			break;
		}
		source = (void *)source->context.hh_uuid.next;
	}

	pthread_mutex_unlock(&obs->data.sources_mutex);
	return source;
}

obs_source_t *obs_get_transition_by_uuid(const char *uuid)
{
	obs_source_t *source = obs_get_source_by_uuid(uuid);

	if (source && source->info.type == OBS_SOURCE_TYPE_TRANSITION)
		return source;
	else if (source)
		obs_source_release(source);

	return NULL;
}

obs_output_t *obs_get_output_by_name(const char *name)
{
	return get_context_by_name(&obs->data.first_output, name, &obs->data.outputs_mutex, obs_output_addref_safe_);
}

obs_encoder_t *obs_get_encoder_by_name(const char *name)
{
	return get_context_by_name(&obs->data.first_encoder, name, &obs->data.encoders_mutex, obs_encoder_addref_safe_);
}

obs_service_t *obs_get_service_by_name(const char *name)
{
	return get_context_by_name(&obs->data.first_service, name, &obs->data.services_mutex, obs_service_addref_safe_);
}

gs_effect_t *obs_get_base_effect(enum obs_base_effect effect)
{
	switch (effect) {
	case OBS_EFFECT_DEFAULT:
		return obs->video.default_effect;
	case OBS_EFFECT_DEFAULT_RECT:
		return obs->video.default_rect_effect;
	case OBS_EFFECT_OPAQUE:
		return obs->video.opaque_effect;
	case OBS_EFFECT_SOLID:
		return obs->video.solid_effect;
	case OBS_EFFECT_REPEAT:
		return obs->video.repeat_effect;
	case OBS_EFFECT_BICUBIC:
		return obs->video.bicubic_effect;
	case OBS_EFFECT_LANCZOS:
		return obs->video.lanczos_effect;
	case OBS_EFFECT_AREA:
		return obs->video.area_effect;
	case OBS_EFFECT_BILINEAR_LOWRES:
		return obs->video.bilinear_lowres_effect;
	case OBS_EFFECT_PREMULTIPLIED_ALPHA:
		return obs->video.premultiplied_alpha_effect;
	}

	return NULL;
}

signal_handler_t *obs_get_signal_handler(void)
{
	return obs->signals;
}

proc_handler_t *obs_get_proc_handler(void)
{
	return obs->procs;
}

static void obs_render_main_texture_internal(enum gs_blend_type src_c, enum gs_blend_type dest_c,
					     enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	struct obs_core_video_mix *video;
	gs_texture_t *tex;
	gs_effect_t *effect;
	gs_eparam_t *param;

	video = obs->video.main_mix;
	if (!video->texture_rendered)
		return;

	const enum gs_color_space source_space = video->render_space;
	const enum gs_color_space current_space = gs_get_color_space();
	const char *tech_name = "Draw";
	float multiplier = 1.f;
	switch (current_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
		if (source_space == GS_CS_709_EXTENDED)
			tech_name = "DrawTonemap";
		break;
	case GS_CS_709_SCRGB:
		tech_name = "DrawMultiply";
		multiplier = obs_get_video_sdr_white_level() / 80.f;
		break;
	case GS_CS_709_EXTENDED:
		break;
	}

	const bool previous = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	tex = video->render_texture;
	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	param = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture_srgb(param, tex);
	param = gs_effect_get_param_by_name(effect, "multiplier");
	gs_effect_set_float(param, multiplier);

	gs_blend_state_push();
	gs_blend_function_separate(src_c, dest_c, src_a, dest_a);

	while (gs_effect_loop(effect, tech_name))
		gs_draw_sprite(tex, 0, 0, 0);

	gs_blend_state_pop();

	gs_enable_framebuffer_srgb(previous);
}

void obs_render_main_texture(void)
{
	obs_render_main_texture_internal(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
}

void obs_render_main_texture_src_color_only(void)
{
	obs_render_main_texture_internal(GS_BLEND_ONE, GS_BLEND_ZERO, GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
}

gs_texture_t *obs_get_main_texture(void)
{
	struct obs_core_video_mix *video;

	video = obs->video.main_mix;
	if (!video->texture_rendered)
		return NULL;

	return video->render_texture;
}

static obs_source_t *obs_load_source_type(obs_data_t *source_data, bool is_private)
{
	obs_data_array_t *filters = obs_data_get_array(source_data, "filters");
	obs_source_t *source;
	const char *name = obs_data_get_string(source_data, "name");
	const char *uuid = obs_data_get_string(source_data, "uuid");
	const char *id = obs_data_get_string(source_data, "id");
	const char *v_id = obs_data_get_string(source_data, "versioned_id");
	obs_data_t *settings = obs_data_get_obj(source_data, "settings");
	obs_data_t *hotkeys = obs_data_get_obj(source_data, "hotkeys");
	double volume;
	double balance;
	int64_t sync;
	uint32_t prev_ver;
	uint32_t caps;
	uint32_t flags;
	uint32_t mixers;
	int di_order;
	int di_mode;
	int monitoring_type;

	prev_ver = (uint32_t)obs_data_get_int(source_data, "prev_ver");

	if (!*v_id)
		v_id = id;

	source = obs_source_create_set_last_ver(v_id, name, uuid, settings, hotkeys, prev_ver, is_private);

	if (source->owns_info_id) {
		bfree((void *)source->info.unversioned_id);
		source->info.unversioned_id = bstrdup(id);
	}

	obs_data_release(hotkeys);

	caps = obs_source_get_output_flags(source);

	obs_data_set_default_double(source_data, "volume", 1.0);
	volume = obs_data_get_double(source_data, "volume");
	obs_source_set_volume(source, (float)volume);

	obs_data_set_default_double(source_data, "balance", 0.5);
	balance = obs_data_get_double(source_data, "balance");
	obs_source_set_balance_value(source, (float)balance);

	sync = obs_data_get_int(source_data, "sync");
	obs_source_set_sync_offset(source, sync);

	obs_data_set_default_int(source_data, "mixers", 0x3F);
	mixers = (uint32_t)obs_data_get_int(source_data, "mixers");
	obs_source_set_audio_mixers(source, mixers);

	obs_data_set_default_int(source_data, "flags", source->default_flags);
	flags = (uint32_t)obs_data_get_int(source_data, "flags");
	obs_source_set_flags(source, flags);

	obs_data_set_default_bool(source_data, "enabled", true);
	obs_source_set_enabled(source, obs_data_get_bool(source_data, "enabled"));

	obs_data_set_default_bool(source_data, "muted", false);
	obs_source_set_muted(source, obs_data_get_bool(source_data, "muted"));

	obs_data_set_default_bool(source_data, "push-to-mute", false);
	obs_source_enable_push_to_mute(source, obs_data_get_bool(source_data, "push-to-mute"));

	obs_data_set_default_int(source_data, "push-to-mute-delay", 0);
	obs_source_set_push_to_mute_delay(source, obs_data_get_int(source_data, "push-to-mute-delay"));

	obs_data_set_default_bool(source_data, "push-to-talk", false);
	obs_source_enable_push_to_talk(source, obs_data_get_bool(source_data, "push-to-talk"));

	obs_data_set_default_int(source_data, "push-to-talk-delay", 0);
	obs_source_set_push_to_talk_delay(source, obs_data_get_int(source_data, "push-to-talk-delay"));

	di_mode = (int)obs_data_get_int(source_data, "deinterlace_mode");
	obs_source_set_deinterlace_mode(source, (enum obs_deinterlace_mode)di_mode);

	di_order = (int)obs_data_get_int(source_data, "deinterlace_field_order");
	obs_source_set_deinterlace_field_order(source, (enum obs_deinterlace_field_order)di_order);

	monitoring_type = (int)obs_data_get_int(source_data, "monitoring_type");
	if (prev_ver < MAKE_SEMANTIC_VERSION(23, 2, 2)) {
		if ((caps & OBS_SOURCE_MONITOR_BY_DEFAULT) != 0) {
			/* updates older sources to enable monitoring
			 * automatically if they added monitoring by default in
			 * version 24 */
			monitoring_type = OBS_MONITORING_TYPE_MONITOR_ONLY;
			obs_source_set_audio_mixers(source, 0x3F);
		}
	}
	obs_source_set_monitoring_type(source, (enum obs_monitoring_type)monitoring_type);

	obs_data_release(source->private_settings);
	source->private_settings = obs_data_get_obj(source_data, "private_settings");
	if (!source->private_settings)
		source->private_settings = obs_data_create();

	if (filters) {
		size_t count = obs_data_array_count(filters);

		for (size_t i = 0; i < count; i++) {
			obs_data_t *filter_data = obs_data_array_item(filters, i);

			obs_source_t *filter = obs_load_source_type(filter_data, true);
			if (filter) {
				obs_source_filter_add(source, filter);
				obs_source_release(filter);
			}

			obs_data_release(filter_data);
		}

		obs_data_array_release(filters);
	}

	obs_data_release(settings);

	return source;
}

obs_source_t *obs_load_source(obs_data_t *source_data)
{
	return obs_load_source_type(source_data, false);
}

obs_source_t *obs_load_private_source(obs_data_t *source_data)
{
	return obs_load_source_type(source_data, true);
}

void obs_load_sources(obs_data_array_t *array, obs_load_source_cb cb, void *private_data)
{
	struct obs_core_data *data = &obs->data;
	DARRAY(obs_source_t *) sources;
	size_t count;
	size_t i;

	da_init(sources);

	count = obs_data_array_count(array);
	da_reserve(sources, count);

	pthread_mutex_lock(&data->sources_mutex);

	for (i = 0; i < count; i++) {
		obs_data_t *source_data = obs_data_array_item(array, i);
		obs_source_t *source = obs_load_source(source_data);

		da_push_back(sources, &source);

		obs_data_release(source_data);
	}

	/* tell sources that we want to load */
	for (i = 0; i < sources.num; i++) {
		obs_source_t *source = sources.array[i];
		obs_data_t *source_data = obs_data_array_item(array, i);
		if (source) {
			if (source->info.type == OBS_SOURCE_TYPE_TRANSITION)
				obs_transition_load(source, source_data);
			obs_source_load2(source);
			if (cb)
				cb(private_data, source);
		}
		obs_data_release(source_data);
	}

	for (i = 0; i < sources.num; i++)
		obs_source_release(sources.array[i]);

	pthread_mutex_unlock(&data->sources_mutex);

	da_free(sources);
}

obs_data_t *obs_save_source(obs_source_t *source)
{
	obs_data_array_t *filters = obs_data_array_create();
	obs_data_t *source_data = obs_data_create();
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_t *hotkey_data = source->context.hotkey_data;
	obs_data_t *hotkeys;
	float volume = obs_source_get_volume(source);
	float balance = obs_source_get_balance_value(source);
	uint32_t mixers = obs_source_get_audio_mixers(source);
	int64_t sync = obs_source_get_sync_offset(source);
	uint32_t flags = obs_source_get_flags(source);
	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);
	const char *id = source->info.unversioned_id;
	const char *v_id = source->info.id;
	bool enabled = obs_source_enabled(source);
	bool muted = obs_source_muted(source);
	bool push_to_mute = obs_source_push_to_mute_enabled(source);
	uint64_t ptm_delay = obs_source_get_push_to_mute_delay(source);
	bool push_to_talk = obs_source_push_to_talk_enabled(source);
	uint64_t ptt_delay = obs_source_get_push_to_talk_delay(source);
	int m_type = (int)obs_source_get_monitoring_type(source);
	int di_mode = (int)obs_source_get_deinterlace_mode(source);
	int di_order = (int)obs_source_get_deinterlace_field_order(source);
	DARRAY(obs_source_t *) filters_copy;

	obs_source_save(source);
	hotkeys = obs_hotkeys_save_source(source);

	if (hotkeys) {
		obs_data_release(hotkey_data);
		source->context.hotkey_data = hotkeys;
		hotkey_data = hotkeys;
	}

	obs_data_set_int(source_data, "prev_ver", LIBOBS_API_VER);

	obs_data_set_string(source_data, "name", name);
	obs_data_set_string(source_data, "uuid", uuid);
	obs_data_set_string(source_data, "id", id);
	obs_data_set_string(source_data, "versioned_id", v_id);
	obs_data_set_obj(source_data, "settings", settings);
	obs_data_set_int(source_data, "mixers", mixers);
	obs_data_set_int(source_data, "sync", sync);
	obs_data_set_int(source_data, "flags", flags);
	obs_data_set_double(source_data, "volume", volume);
	obs_data_set_double(source_data, "balance", balance);
	obs_data_set_bool(source_data, "enabled", enabled);
	obs_data_set_bool(source_data, "muted", muted);
	obs_data_set_bool(source_data, "push-to-mute", push_to_mute);
	obs_data_set_int(source_data, "push-to-mute-delay", ptm_delay);
	obs_data_set_bool(source_data, "push-to-talk", push_to_talk);
	obs_data_set_int(source_data, "push-to-talk-delay", ptt_delay);
	obs_data_set_obj(source_data, "hotkeys", hotkey_data);
	obs_data_set_int(source_data, "deinterlace_mode", di_mode);
	obs_data_set_int(source_data, "deinterlace_field_order", di_order);
	obs_data_set_int(source_data, "monitoring_type", m_type);

	obs_data_set_obj(source_data, "private_settings", source->private_settings);

	if (source->info.type == OBS_SOURCE_TYPE_TRANSITION)
		obs_transition_save(source, source_data);

	pthread_mutex_lock(&source->filter_mutex);
	da_init(filters_copy);
	da_reserve(filters_copy, source->filters.num);

	for (size_t i = 0; i < source->filters.num; i++) {
		obs_source_t *filter = obs_source_get_ref(source->filters.array[i]);
		if (filter)
			da_push_back(filters_copy, &filter);
	}

	pthread_mutex_unlock(&source->filter_mutex);

	if (filters_copy.num) {
		for (size_t i = filters_copy.num; i > 0; i--) {
			obs_source_t *filter = filters_copy.array[i - 1];
			obs_data_t *filter_data = obs_save_source(filter);
			obs_data_array_push_back(filters, filter_data);
			obs_data_release(filter_data);
			obs_source_release(filter);
		}

		obs_data_set_array(source_data, "filters", filters);
	}

	da_free(filters_copy);

	obs_data_release(settings);
	obs_data_array_release(filters);

	return source_data;
}

obs_data_array_t *obs_save_sources_filtered(obs_save_source_filter_cb cb, void *data_)
{
	struct obs_core_data *data = &obs->data;
	obs_data_array_t *array;
	obs_source_t *source;

	array = obs_data_array_create();

	pthread_mutex_lock(&data->sources_mutex);

	source = data->public_sources;

	while (source) {
		if ((source->info.type != OBS_SOURCE_TYPE_FILTER) != 0 && !source->removed && !source->temp_removed &&
		    cb(data_, source)) {
			obs_data_t *source_data = obs_save_source(source);

			obs_data_array_push_back(array, source_data);
			obs_data_release(source_data);
		}

		source = (obs_source_t *)source->context.hh.next;
	}

	pthread_mutex_unlock(&data->sources_mutex);

	return array;
}

static bool save_source_filter(void *data, obs_source_t *source)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(source);
	return true;
}

obs_data_array_t *obs_save_sources(void)
{
	return obs_save_sources_filtered(save_source_filter, NULL);
}

void obs_reset_source_uuids()
{
	pthread_mutex_lock(&obs->data.sources_mutex);

	/* Move all sources to a new hash table */
	struct obs_context_data *ht = (struct obs_context_data *)obs->data.sources;
	struct obs_context_data *new_ht = NULL;

	struct obs_context_data *ctx, *tmp;
	HASH_ITER (hh_uuid, ht, ctx, tmp) {
		HASH_DELETE(hh_uuid, ht, ctx);

		bfree((void *)ctx->uuid);
		ctx->uuid = os_generate_uuid();

		HASH_ADD_UUID(new_ht, uuid, ctx);
	}

	/* The old table will be automatically freed once the last element has
	 * been removed, so we can simply overwrite the pointer. */
	obs->data.sources = (struct obs_source *)new_ht;

	pthread_mutex_unlock(&obs->data.sources_mutex);
}

/* ensures that names are never blank */
static inline char *dup_name(const char *name, bool private)
{
	if (private && !name)
		return NULL;

	if (!name || !*name) {
		struct dstr unnamed = {0};
		dstr_printf(&unnamed, "__unnamed%04lld", obs->data.unnamed_index++);

		return unnamed.array;
	} else {
		return bstrdup(name);
	}
}

static inline bool obs_context_data_init_wrap(struct obs_context_data *context, enum obs_obj_type type,
					      obs_data_t *settings, const char *name, const char *uuid,
					      obs_data_t *hotkey_data, bool private)
{
	assert(context);
	memset(context, 0, sizeof(*context));
	context->private = private;
	context->type = type;

	pthread_mutex_init_value(&context->rename_cache_mutex);
	if (pthread_mutex_init(&context->rename_cache_mutex, NULL) < 0)
		return false;

	context->signals = signal_handler_create();
	if (!context->signals)
		return false;

	context->procs = proc_handler_create();
	if (!context->procs)
		return false;

	if (uuid && strlen(uuid) == UUID_STR_LENGTH)
		context->uuid = bstrdup(uuid);
	/* Only automatically generate UUIDs for sources */
	else if (type == OBS_OBJ_TYPE_SOURCE)
		context->uuid = os_generate_uuid();

	context->name = dup_name(name, private);
	context->settings = obs_data_newref(settings);
	context->hotkey_data = obs_data_newref(hotkey_data);
	return true;
}

bool obs_context_data_init(struct obs_context_data *context, enum obs_obj_type type, obs_data_t *settings,
			   const char *name, const char *uuid, obs_data_t *hotkey_data, bool private)
{
	if (obs_context_data_init_wrap(context, type, settings, name, uuid, hotkey_data, private)) {
		return true;
	} else {
		obs_context_data_free(context);
		return false;
	}
}

void obs_context_data_free(struct obs_context_data *context)
{
	obs_hotkeys_context_release(context);
	signal_handler_destroy(context->signals);
	proc_handler_destroy(context->procs);
	obs_data_release(context->settings);
	obs_context_data_remove(context);
	pthread_mutex_destroy(&context->rename_cache_mutex);
	bfree(context->name);
	bfree((void *)context->uuid);

	for (size_t i = 0; i < context->rename_cache.num; i++)
		bfree(context->rename_cache.array[i]);
	da_free(context->rename_cache);

	memset(context, 0, sizeof(*context));
}

void obs_context_init_control(struct obs_context_data *context, void *object, obs_destroy_cb destroy)
{
	context->control = bzalloc(sizeof(obs_weak_object_t));
	context->control->object = object;
	context->destroy = destroy;
}

void obs_context_data_insert(struct obs_context_data *context, pthread_mutex_t *mutex, void *pfirst)
{
	struct obs_context_data **first = pfirst;

	assert(context);
	assert(mutex);
	assert(first);

	context->mutex = mutex;

	pthread_mutex_lock(mutex);
	context->prev_next = first;
	context->next = *first;
	*first = context;
	if (context->next)
		context->next->prev_next = &context->next;
	pthread_mutex_unlock(mutex);
}

static inline char *obs_context_deduplicate_name(void *phash, const char *name)
{
	struct obs_context_data *head = phash;
	struct obs_context_data *item = NULL;

	HASH_FIND_STR(head, name, item);
	if (!item)
		return NULL;

	struct dstr new_name = {0};
	int suffix = 2;

	while (item) {
		dstr_printf(&new_name, "%s %d", name, suffix++);
		HASH_FIND_STR(head, new_name.array, item);
	}

	return new_name.array;
}

void obs_context_data_insert_name(struct obs_context_data *context, pthread_mutex_t *mutex, void *pfirst)
{
	struct obs_context_data **first = pfirst;
	char *new_name;

	assert(context);
	assert(mutex);
	assert(first);

	context->mutex = mutex;

	pthread_mutex_lock(mutex);

	/* Ensure name is not a duplicate. */
	new_name = obs_context_deduplicate_name(*first, context->name);
	if (new_name) {
		blog(LOG_WARNING,
		     "Attempted to insert context with duplicate name \"%s\"!"
		     " Name has been changed to \"%s\"",
		     context->name, new_name);
		/* Since this happens before the context creation finishes,
		 * do not bother to add it to the rename cache. */
		bfree(context->name);
		context->name = new_name;
	}

	HASH_ADD_STR(*first, name, context);

	pthread_mutex_unlock(mutex);
}

void obs_context_data_insert_uuid(struct obs_context_data *context, pthread_mutex_t *mutex, void *pfirst_uuid)
{
	struct obs_context_data **first_uuid = pfirst_uuid;
	struct obs_context_data *item = NULL;

	assert(context);
	assert(mutex);
	assert(first_uuid);

	context->mutex = mutex;

	pthread_mutex_lock(mutex);

	/* Ensure UUID is not a duplicate.
	 * This should only ever happen if a scene collection file has been
	 * manually edited and an entry has been duplicated without removing
	 * or regenerating the UUID. */
	HASH_FIND_UUID(*first_uuid, context->uuid, item);
	if (item) {
		blog(LOG_WARNING, "Attempted to insert context with duplicate UUID \"%s\"!", context->uuid);
		/* It is practically impossible for the new UUID to be a
		 * duplicate, so don't bother checking again. */
		bfree((void *)context->uuid);
		context->uuid = os_generate_uuid();
	}

	HASH_ADD_UUID(*first_uuid, uuid, context);
	pthread_mutex_unlock(mutex);
}

void obs_context_data_remove(struct obs_context_data *context)
{
	if (context && context->prev_next) {
		pthread_mutex_lock(context->mutex);
		*context->prev_next = context->next;
		if (context->next)
			context->next->prev_next = context->prev_next;
		context->prev_next = NULL;
		pthread_mutex_unlock(context->mutex);
	}
}

void obs_context_data_remove_name(struct obs_context_data *context, void *phead)
{
	struct obs_context_data **head = phead;

	assert(head);

	if (!context)
		return;

	pthread_mutex_lock(context->mutex);
	HASH_DELETE(hh, *head, context);
	pthread_mutex_unlock(context->mutex);
}

void obs_context_data_remove_uuid(struct obs_context_data *context, void *puuid_head)
{
	struct obs_context_data **uuid_head = puuid_head;

	assert(uuid_head);

	if (!context || !context->uuid || !uuid_head)
		return;

	pthread_mutex_lock(context->mutex);
	HASH_DELETE(hh_uuid, *uuid_head, context);
	pthread_mutex_unlock(context->mutex);
}

void obs_context_wait(struct obs_context_data *context)
{
	pthread_mutex_lock(context->mutex);
	pthread_mutex_unlock(context->mutex);
}

void obs_context_data_setname(struct obs_context_data *context, const char *name)
{
	pthread_mutex_lock(&context->rename_cache_mutex);

	if (context->name)
		da_push_back(context->rename_cache, &context->name);
	context->name = dup_name(name, context->private);

	pthread_mutex_unlock(&context->rename_cache_mutex);
}

void obs_context_data_setname_ht(struct obs_context_data *context, const char *name, void *phead)
{
	struct obs_context_data **head = phead;
	char *new_name;

	pthread_mutex_lock(context->mutex);
	pthread_mutex_lock(&context->rename_cache_mutex);

	HASH_DEL(*head, context);
	if (context->name)
		da_push_back(context->rename_cache, &context->name);

	/* Ensure new name is not a duplicate. */
	new_name = obs_context_deduplicate_name(*head, name);
	if (new_name) {
		blog(LOG_WARNING,
		     "Attempted to rename context to duplicate name \"%s\"!"
		     " New name has been changed to \"%s\"",
		     context->name, new_name);
		context->name = new_name;
	} else {
		context->name = dup_name(name, context->private);
	}

	HASH_ADD_STR(*head, name, context);

	pthread_mutex_unlock(&context->rename_cache_mutex);
	pthread_mutex_unlock(context->mutex);
}

profiler_name_store_t *obs_get_profiler_name_store(void)
{
	return obs->name_store;
}

uint64_t obs_get_video_frame_time(void)
{
	return obs->video.video_time;
}

double obs_get_active_fps(void)
{
	return obs->video.video_fps;
}

uint64_t obs_get_average_frame_time_ns(void)
{
	return obs->video.video_avg_frame_time_ns;
}

uint64_t obs_get_frame_interval_ns(void)
{
	return obs->video.video_frame_interval_ns;
}

enum obs_obj_type obs_obj_get_type(void *obj)
{
	struct obs_context_data *context = obj;
	return context ? context->type : OBS_OBJ_TYPE_INVALID;
}

const char *obs_obj_get_id(void *obj)
{
	struct obs_context_data *context = obj;
	if (!context)
		return NULL;

	switch (context->type) {
	case OBS_OBJ_TYPE_SOURCE:
		return ((obs_source_t *)obj)->info.id;
	case OBS_OBJ_TYPE_OUTPUT:
		return ((obs_output_t *)obj)->info.id;
	case OBS_OBJ_TYPE_ENCODER:
		return ((obs_encoder_t *)obj)->info.id;
	case OBS_OBJ_TYPE_SERVICE:
		return ((obs_service_t *)obj)->info.id;
	default:;
	}

	return NULL;
}

bool obs_obj_invalid(void *obj)
{
	struct obs_context_data *context = obj;
	if (!context)
		return true;

	return !context->data;
}

void *obs_obj_get_data(void *obj)
{
	struct obs_context_data *context = obj;
	if (!context)
		return NULL;

	return context->data;
}

bool obs_obj_is_private(void *obj)
{
	struct obs_context_data *context = obj;
	if (!context)
		return false;

	return context->private;
}

void obs_reset_audio_monitoring(void)
{
	if (!obs_audio_monitoring_available())
		return;

	pthread_mutex_lock(&obs->audio.monitoring_mutex);

	for (size_t i = 0; i < obs->audio.monitors.num; i++) {
		struct audio_monitor *monitor = obs->audio.monitors.array[i];
		audio_monitor_reset(monitor);
	}

	pthread_mutex_unlock(&obs->audio.monitoring_mutex);
}

bool obs_set_audio_monitoring_device(const char *name, const char *id)
{
	if (!name || !id || !*name || !*id)
		return false;

	if (!obs_audio_monitoring_available())
		return false;

	pthread_mutex_lock(&obs->audio.monitoring_mutex);

	if (strcmp(id, obs->audio.monitoring_device_id) == 0) {
		pthread_mutex_unlock(&obs->audio.monitoring_mutex);
		return true;
	}

	bfree(obs->audio.monitoring_device_name);
	bfree(obs->audio.monitoring_device_id);

	obs->audio.monitoring_device_name = bstrdup(name);
	obs->audio.monitoring_device_id = bstrdup(id);

	obs_reset_audio_monitoring();

	pthread_mutex_unlock(&obs->audio.monitoring_mutex);
	return true;
}

void obs_get_audio_monitoring_device(const char **name, const char **id)
{
	if (name)
		*name = obs->audio.monitoring_device_name;
	if (id)
		*id = obs->audio.monitoring_device_id;
}

void obs_add_tick_callback(void (*tick)(void *param, float seconds), void *param)
{
	struct tick_callback data = {tick, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_insert(obs->data.tick_callbacks, 0, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

void obs_remove_tick_callback(void (*tick)(void *param, float seconds), void *param)
{
	struct tick_callback data = {tick, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_erase_item(obs->data.tick_callbacks, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

void obs_add_main_render_callback(void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)
{
	struct draw_callback data = {draw, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_insert(obs->data.draw_callbacks, 0, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

void obs_remove_main_render_callback(void (*draw)(void *param, uint32_t cx, uint32_t cy), void *param)
{
	struct draw_callback data = {draw, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_erase_item(obs->data.draw_callbacks, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

void obs_add_main_rendered_callback(void (*rendered)(void *param), void *param)
{
	struct rendered_callback data = {rendered, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_insert(obs->data.rendered_callbacks, 0, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

void obs_remove_main_rendered_callback(void (*rendered)(void *param), void *param)
{
	struct rendered_callback data = {rendered, param};

	pthread_mutex_lock(&obs->data.draw_callbacks_mutex);
	da_erase_item(obs->data.rendered_callbacks, &data);
	pthread_mutex_unlock(&obs->data.draw_callbacks_mutex);
}

uint32_t obs_get_total_frames(void)
{
	return obs->video.total_frames;
}

uint32_t obs_get_lagged_frames(void)
{
	return obs->video.lagged_frames;
}

struct obs_core_video_mix *get_mix_for_video(video_t *v)
{
	struct obs_core_video_mix *result = NULL;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++) {
		struct obs_core_video_mix *mix = obs->video.mixes.array[i];

		if (v == mix->video) {
			result = mix;
			break;
		}
	}
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	return result;
}

void start_raw_video(video_t *v, const struct video_scale_info *conversion, uint32_t frame_rate_divisor,
		     void (*callback)(void *param, struct video_data *frame), void *param)
{
	struct obs_core_video_mix *video = get_mix_for_video(v);
	if (!video)
		return;
	if (video_output_connect2(v, conversion, frame_rate_divisor, callback, param))
		os_atomic_inc_long(&video->raw_active);
}

void stop_raw_video(video_t *v, void (*callback)(void *param, struct video_data *frame), void *param)
{
	struct obs_core_video_mix *video = get_mix_for_video(v);
	if (!video)
		return;
	if (video_output_disconnect2(v, callback, param))
		os_atomic_dec_long(&video->raw_active);
}

void obs_add_raw_video_callback(const struct video_scale_info *conversion,
				void (*callback)(void *param, struct video_data *frame), void *param)
{
	obs_add_raw_video_callback2(conversion, 1, callback, param);
}

void obs_add_raw_video_callback2(const struct video_scale_info *conversion, uint32_t frame_rate_divisor,
				 void (*callback)(void *param, struct video_data *frame), void *param)
{
	struct obs_core_video_mix *video = obs->video.main_mix;
	start_raw_video(video->video, conversion, frame_rate_divisor, callback, param);
}

void obs_remove_raw_video_callback(void (*callback)(void *param, struct video_data *frame), void *param)
{
	struct obs_core_video_mix *video = obs->video.main_mix;
	stop_raw_video(video->video, callback, param);
}

void obs_add_raw_audio_callback(size_t mix_idx, const struct audio_convert_info *conversion,
				audio_output_callback_t callback, void *param)
{
	struct obs_core_audio *audio = &obs->audio;
	audio_output_connect(audio->audio, mix_idx, conversion, callback, param);
}

void obs_remove_raw_audio_callback(size_t mix_idx, audio_output_callback_t callback, void *param)
{
	struct obs_core_audio *audio = &obs->audio;
	audio_output_disconnect(audio->audio, mix_idx, callback, param);
}

void obs_apply_private_data(obs_data_t *settings)
{
	if (!settings)
		return;

	obs_data_apply(obs->data.private_data, settings);
}

void obs_set_private_data(obs_data_t *settings)
{
	obs_data_clear(obs->data.private_data);
	if (settings)
		obs_data_apply(obs->data.private_data, settings);
}

obs_data_t *obs_get_private_data(void)
{
	obs_data_t *private_data = obs->data.private_data;
	obs_data_addref(private_data);
	return private_data;
}

extern bool init_gpu_encoding(struct obs_core_video_mix *video);
extern void stop_gpu_encoding_thread(struct obs_core_video_mix *video);
extern void free_gpu_encoding(struct obs_core_video_mix *video);

bool start_gpu_encode(obs_encoder_t *encoder)
{
	struct obs_core_video_mix *video = get_mix_for_video(encoder->media);
	bool success = true;

	obs_enter_graphics();
	pthread_mutex_lock(&video->gpu_encoder_mutex);

	if (!video->gpu_encoders.num)
		success = init_gpu_encoding(video);
	if (success)
		da_push_back(video->gpu_encoders, &encoder);
	else
		free_gpu_encoding(video);

	pthread_mutex_unlock(&video->gpu_encoder_mutex);
	obs_leave_graphics();

	if (success) {
		os_atomic_inc_long(&video->gpu_encoder_active);
		video_output_inc_texture_encoders(video->video);
	}

	return success;
}

void stop_gpu_encode(obs_encoder_t *encoder)
{
	struct obs_core_video_mix *video = get_mix_for_video(encoder->media);
	bool call_free = false;

	os_atomic_dec_long(&video->gpu_encoder_active);
	video_output_dec_texture_encoders(video->video);

	pthread_mutex_lock(&video->gpu_encoder_mutex);
	da_erase_item(video->gpu_encoders, &encoder);
	if (!video->gpu_encoders.num)
		call_free = true;
	pthread_mutex_unlock(&video->gpu_encoder_mutex);

	os_event_wait(video->gpu_encode_inactive);

	if (call_free) {
		stop_gpu_encoding_thread(video);

		obs_enter_graphics();
		pthread_mutex_lock(&video->gpu_encoder_mutex);
		free_gpu_encoding(video);
		pthread_mutex_unlock(&video->gpu_encoder_mutex);
		obs_leave_graphics();
	}
}

bool obs_video_active(void)
{
	bool result = false;

	pthread_mutex_lock(&obs->video.mixes_mutex);
	for (size_t i = 0, num = obs->video.mixes.num; i < num; i++) {
		struct obs_core_video_mix *video = obs->video.mixes.array[i];

		if (os_atomic_load_long(&video->raw_active) > 0 ||
		    os_atomic_load_long(&video->gpu_encoder_active) > 0) {
			result = true;
			break;
		}
	}
	pthread_mutex_unlock(&obs->video.mixes_mutex);

	return result;
}

bool obs_nv12_tex_active(void)
{
	struct obs_core_video_mix *video = obs->video.main_mix;
	return video->using_nv12_tex;
}

bool obs_p010_tex_active(void)
{
	struct obs_core_video_mix *video = obs->video.main_mix;
	return video->using_p010_tex;
}

/* ------------------------------------------------------------------------- */
/* task stuff                                                                */

struct task_wait_info {
	obs_task_t task;
	void *param;
	os_event_t *event;
};

static void task_wait_callback(void *param)
{
	struct task_wait_info *info = param;
	if (info->task)
		info->task(info->param);
	os_event_signal(info->event);
}

THREAD_LOCAL bool is_graphics_thread = false;
THREAD_LOCAL bool is_audio_thread = false;

static void set_audio_thread(void *unused)
{
	is_audio_thread = true;
	UNUSED_PARAMETER(unused);
}

bool obs_in_task_thread(enum obs_task_type type)
{
	if (type == OBS_TASK_GRAPHICS)
		return is_graphics_thread;
	else if (type == OBS_TASK_AUDIO)
		return is_audio_thread;
	else if (type == OBS_TASK_UI)
		return is_ui_thread;
	else if (type == OBS_TASK_DESTROY)
		return os_task_queue_inside(obs->destruction_task_thread);

	assert(false);
	return false;
}

void obs_queue_task(enum obs_task_type type, obs_task_t task, void *param, bool wait)
{
	if (type == OBS_TASK_UI) {
		if (obs->ui_task_handler) {
			obs->ui_task_handler(task, param, wait);
		} else {
			blog(LOG_ERROR, "UI task could not be queued, "
					"there's no UI task handler!");
		}
	} else {
		if (obs_in_task_thread(type)) {
			task(param);

		} else if (wait) {
			struct task_wait_info info = {
				.task = task,
				.param = param,
			};

			os_event_init(&info.event, OS_EVENT_TYPE_MANUAL);
			obs_queue_task(type, task_wait_callback, &info, false);
			os_event_wait(info.event);
			os_event_destroy(info.event);

		} else if (type == OBS_TASK_GRAPHICS) {
			struct obs_core_video *video = &obs->video;
			struct obs_task_info info = {task, param};

			pthread_mutex_lock(&video->task_mutex);
			deque_push_back(&video->tasks, &info, sizeof(info));
			pthread_mutex_unlock(&video->task_mutex);

		} else if (type == OBS_TASK_AUDIO) {
			struct obs_core_audio *audio = &obs->audio;
			struct obs_task_info info = {task, param};

			pthread_mutex_lock(&audio->task_mutex);
			deque_push_back(&audio->tasks, &info, sizeof(info));
			pthread_mutex_unlock(&audio->task_mutex);

		} else if (type == OBS_TASK_DESTROY) {
			os_task_t os_task = (os_task_t)task;
			os_task_queue_queue_task(obs->destruction_task_thread, os_task, param);
		}
	}
}

bool obs_wait_for_destroy_queue(void)
{
	struct task_wait_info info = {0};

	if (!obs->video.thread_initialized || !obs->audio.audio)
		return false;

	/* allow video and audio threads time to release objects */
	os_event_init(&info.event, OS_EVENT_TYPE_AUTO);
	obs_queue_task(OBS_TASK_GRAPHICS, task_wait_callback, &info, false);
	os_event_wait(info.event);
	obs_queue_task(OBS_TASK_AUDIO, task_wait_callback, &info, false);
	os_event_wait(info.event);
	os_event_destroy(info.event);

	/* wait for destroy task queue */
	return os_task_queue_wait(obs->destruction_task_thread);
}

static void set_ui_thread(void *unused)
{
	is_ui_thread = true;
	UNUSED_PARAMETER(unused);
}

void obs_set_ui_task_handler(obs_task_handler_t handler)
{
	obs->ui_task_handler = handler;
	obs_queue_task(OBS_TASK_UI, set_ui_thread, NULL, false);
}

obs_object_t *obs_object_get_ref(obs_object_t *object)
{
	if (!object)
		return NULL;

	return obs_weak_object_get_object(object->control);
}

void obs_object_release(obs_object_t *object)
{
	if (!obs) {
		blog(LOG_WARNING, "Tried to release an object when the OBS "
				  "core is shut down!");
		return;
	}

	if (!object)
		return;

	obs_weak_object_t *control = object->control;
	if (obs_ref_release(&control->ref)) {
		object->destroy(object);
		obs_weak_object_release(control);
	}
}

void obs_weak_object_addref(obs_weak_object_t *weak)
{
	if (!weak)
		return;

	obs_weak_ref_addref(&weak->ref);
}

void obs_weak_object_release(obs_weak_object_t *weak)
{
	if (!weak)
		return;

	if (obs_weak_ref_release(&weak->ref))
		bfree(weak);
}

obs_weak_object_t *obs_object_get_weak_object(obs_object_t *object)
{
	if (!object)
		return NULL;

	obs_weak_object_t *weak = object->control;
	obs_weak_object_addref(weak);
	return weak;
}

obs_object_t *obs_weak_object_get_object(obs_weak_object_t *weak)
{
	if (!weak)
		return NULL;

	if (obs_weak_ref_get_ref(&weak->ref))
		return weak->object;

	return NULL;
}

bool obs_weak_object_expired(obs_weak_object_t *weak)
{
	return weak ? obs_weak_ref_expired(&weak->ref) : true;
}

bool obs_weak_object_references_object(obs_weak_object_t *weak, obs_object_t *object)
{
	return weak && object && weak->object == object;
}

bool obs_is_output_protocol_registered(const char *protocol)
{
	for (size_t i = 0; i < obs->data.protocols.num; i++) {
		if (strcmp(protocol, obs->data.protocols.array[i]) == 0)
			return true;
	}

	return false;
}

bool obs_enum_output_protocols(size_t idx, char **protocol)
{
	if (idx >= obs->data.protocols.num)
		return false;

	*protocol = obs->data.protocols.array[idx];
	return true;
}
