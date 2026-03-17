/******************************************************************************
    Copyright (C) 2023-2024 by OBS Project

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

#pragma once

#include <util/darray.h>
#include <util/threading.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <graphics/matrix4.h>

#include <vulkan/vulkan.h>
#include "vulkan-loader.h"
#include "vulkan-api.h"

struct vk_platform;
struct vk_windowinfo;

/* Forward declarations */
struct gs_texture;
struct gs_texture_2d;
struct gs_texture_3d;
struct gs_texture_cube;

/* Format conversion helpers */
static inline VkFormat convert_gs_format(enum gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case GS_BGRX:
		return VK_FORMAT_B8G8R8_SRGB;
	case GS_BGRA:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case GS_R10G10B10A2:
		return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case GS_RGBA16:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case GS_RGBA16F:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case GS_RGBA32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case GS_RG16F:
		return VK_FORMAT_R16G16_SFLOAT;
	case GS_RG32F:
		return VK_FORMAT_R32G32_SFLOAT;
	case GS_R8G8:
		return VK_FORMAT_R8G8_UNORM;
	case GS_R16F:
		return VK_FORMAT_R16_SFLOAT;
	case GS_R32F:
		return VK_FORMAT_R32_SFLOAT;
	case GS_RG16:
		return VK_FORMAT_R16G16_UNORM;
	case GS_R8:
		return VK_FORMAT_R8_UNORM;
	case GS_A8:
		return VK_FORMAT_R8_UNORM;
	case GS_RGBA_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX_UNORM:
		return VK_FORMAT_B8G8R8_UNORM;
	case GS_BGRA_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case GS_R16:
		return VK_FORMAT_R16_UNORM;
	case GS_DXT1:
	case GS_DXT3:
	case GS_DXT5:
		/* Compressed formats require BCn extensions */
		return VK_FORMAT_UNDEFINED;
	case GS_UNKNOWN:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

/* Sampler state structure */
struct gs_sampler_state {
	gs_device_t *device;
	int ref;

	VkFilter min_filter;
	VkFilter mag_filter;
	VkSamplerAddressMode address_u;
	VkSamplerAddressMode address_v;
	VkSamplerAddressMode address_w;
	VkSamplerMipmapMode mipmap_mode;
	float max_anisotropy;
	struct vec4 border_color;

	VkSampler sampler;
};

/* Texture structure */
struct gs_texture {
	gs_device_t *device;
	enum gs_texture_type type;
	enum gs_color_format format;
	VkFormat vk_format;
	VkImageType vk_image_type;
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory memory;
	uint32_t levels;
	bool is_dynamic;
	bool is_render_target;
	bool is_dummy;
	bool gen_mipmaps;

	gs_samplerstate_t *cur_sampler;
	struct vk_render_target *render_target;
};

struct gs_texture_2d {
	struct gs_texture base;
	uint32_t width;
	uint32_t height;
	bool gen_mipmaps;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
};

struct gs_texture_3d {
	struct gs_texture base;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	bool gen_mipmaps;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
};

struct gs_texture_cube {
	struct gs_texture base;
	uint32_t size;
};

/* Render target info */
struct vk_render_target {
	VkFramebuffer framebuffer;
	VkRenderPass render_pass;
	uint32_t width;
	uint32_t height;
	enum gs_color_format format;

	gs_texture_t *cur_render_target;
	int cur_render_side;
	gs_zstencil_t *cur_zstencil_buffer;
};

/* Shader parameter */
struct gs_shader_param {
	char *name;
	enum gs_shader_param_type type;
	GLint uniform_location;

	gs_texture_t *texture;
	GLint texture_id;
	size_t sampler_id;
	int array_count;

	DARRAY(uint8_t) cur_value;
	DARRAY(uint8_t) def_value;
	bool changed;
};

/* Shader attribute */
enum attrib_type {
	ATTRIB_POSITION,
	ATTRIB_NORMAL,
	ATTRIB_TANGENT,
	ATTRIB_COLOR,
	ATTRIB_TEXCOORD,
	ATTRIB_TARGET
};

struct shader_attrib {
	char *name;
	size_t index;
	enum attrib_type type;
};

/* Shader structure */
struct gs_shader {
	gs_device_t *device;
	enum gs_shader_type type;
	VkShaderModule module;

	struct gs_shader_param *viewproj;
	struct gs_shader_param *world;

	DARRAY(struct shader_attrib) attribs;
	DARRAY(struct gs_shader_param) params;
	DARRAY(gs_samplerstate_t *) samplers;

	VkPipelineShaderStageCreateInfo stage_info;
	uint8_t *bytecode;
	size_t bytecode_size;
};

/* Shader program */
struct gs_program {
	gs_device_t *device;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	struct gs_shader *vertex_shader;
	struct gs_shader *pixel_shader;

	DARRAY(struct gs_shader_param *) params;
	VkDescriptorSet descriptor_set;
	VkDescriptorSetLayout descriptor_layout;

	struct gs_program **prev_next;
	struct gs_program *next;
};

/* Vertex buffer */
struct gs_vertex_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;

	gs_device_t *device;
	size_t num;
	bool dynamic;
	struct gs_vb_data *data;
};

/* Index buffer */
struct gs_index_buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	enum gs_index_type type;
	VkIndexType vk_type;

	gs_device_t *device;
	void *data;
	size_t num;
	size_t width;
	size_t size;
	bool dynamic;
};

/* Stage surface for readback */
struct gs_stage_surface {
	gs_device_t *device;

	enum gs_color_format format;
	uint32_t width;
	uint32_t height;

	uint32_t bytes_per_pixel;
	VkBuffer buffer;
	VkDeviceMemory memory;
};

/* Z-Stencil buffer */
struct gs_zstencil_buffer {
	gs_device_t *device;
	VkImage image;
	VkImageView image_view;
	VkDeviceMemory memory;
	VkFormat format;
};

/* Swap chain */
struct gs_swap_chain {
	gs_device_t *device;
	struct vk_windowinfo *wi;
	struct gs_init_data info;
	
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	DARRAY(VkImage) images;
	DARRAY(VkImageView) image_views;
	uint32_t current_image;
	VkSemaphore image_acquired_semaphore;
	VkSemaphore render_complete_semaphore;
};

/* Device structure */
struct gs_device {
	struct vk_platform *plat;

	VkInstance instance;
	VkPhysicalDevice gpu;
	VkDevice device;
	VkQueue graphics_queue;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;

	uint32_t graphics_queue_family_index;

	gs_samplerstate_t *raw_load_sampler;

	gs_texture_t *cur_render_target;
	gs_zstencil_t *cur_zstencil_buffer;
	int cur_render_side;
	gs_texture_t *cur_textures[GS_MAX_TEXTURES];
	gs_samplerstate_t *cur_samplers[GS_MAX_TEXTURES];
	gs_vertbuffer_t *cur_vertex_buffer;
	gs_indexbuffer_t *cur_index_buffer;
	gs_shader_t *cur_vertex_shader;
	gs_shader_t *cur_pixel_shader;
	gs_swapchain_t *cur_swap;
	struct gs_program *cur_program;
	enum gs_color_space cur_color_space;

	struct gs_program *first_program;

	enum gs_cull_mode cur_cull_mode;
	struct gs_rect cur_viewport;

	struct matrix4 cur_proj;
	struct matrix4 cur_view;
	struct matrix4 cur_viewproj;
	DARRAY(struct matrix4) proj_stack;

	VkDescriptorPool descriptor_pool;

	/* Vulkan synchronization */
	VkSemaphore present_semaphore;
	VkFence render_fence;

	struct vk_render_target *cur_render_target_info;
};

/* Helper functions */
extern struct gs_program *gs_program_create(struct gs_device *device);
extern void gs_program_destroy(struct gs_program *program);
extern void program_update_params(struct gs_program *program);

extern bool vk_init_extensions(struct gs_device *device);
extern struct vk_platform *vk_platform_create(struct gs_device *device, uint32_t adapter);
extern void vk_platform_destroy(struct vk_platform *plat);

extern struct vk_windowinfo *vk_windowinfo_create(const struct gs_init_data *info);
extern void vk_windowinfo_destroy(struct vk_windowinfo *wi);

extern bool vk_platform_init_swapchain(struct gs_swap_chain *swap);
extern void vk_platform_cleanup_swapchain(struct gs_swap_chain *swap);

extern const char *vk_get_error_string(VkResult result);

#endif
