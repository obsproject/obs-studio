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

#include "vulkan-subsystem.h"
#include "vulkan-helpers.h"
#include <util/blogging.h>
#include <util/mem.h>
#include <graphics/matrix3.h>

const char *device_get_name(void)
{
	return "Vulkan";
}

int device_get_type(void)
{
	return GS_DEVICE_VULKAN;
}

const char *device_preprocessor_name(void)
{
	return "_VULKAN";
}

const char *gpu_get_driver_version(void)
{
	/* Placeholder - would be filled with actual GPU info */
	return "Vulkan";
}

const char *gpu_get_renderer(void)
{
	/* Placeholder - would be filled with actual GPU info */
	return "Vulkan Device";
}

uint64_t gpu_get_dmem(void)
{
	/* TODO: Query actual dedicated VRAM */
	return 0;
}

uint64_t gpu_get_smem(void)
{
	/* TODO: Query actual shared memory */
	return 0;
}

static bool vk_init_extensions(struct gs_device *device)
{
	blog(LOG_INFO, "Initializing Vulkan extensions...");

	/* Check for required extensions */
	uint32_t extension_count = 0;
	vkEnumerateDeviceExtensionProperties(device->gpu, NULL, &extension_count, NULL);

	VkExtensionProperties *extensions = bmalloc(sizeof(VkExtensionProperties) * extension_count);
	vkEnumerateDeviceExtensionProperties(device->gpu, NULL, &extension_count, extensions);

	blog(LOG_INFO, "Found %u device extensions", extension_count);

	bfree(extensions);
	return true;
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	struct gs_device *device = bzalloc(sizeof(struct gs_device));
	int errorcode = GS_ERROR_FAIL;

	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO, "Initializing Vulkan...");

	/* 初始化 Vulkan 动态加载器 */
	if (!vk_loader_init()) {
		blog(LOG_ERROR, "Failed to initialize Vulkan loader");
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	blog(LOG_INFO, "Vulkan loader initialized");

	/* Create Vulkan Instance */
	VkApplicationInfo app_info = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				       .pApplicationName = "OBS Studio",
				       .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				       .apiVersion = VK_API_VERSION_1_2};

	VkInstanceCreateInfo instance_info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					       .pApplicationInfo = &app_info};

	VkResult result = vkCreateInstance(&instance_info, NULL, &device->instance);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create Vulkan instance: %s", vk_get_error_string(result));
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	/* 加载实例级别函数 */
	if (!vk_loader_load_instance_functions(device->instance)) {
		blog(LOG_ERROR, "Failed to load instance functions");
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	/* Select physical device */
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(device->instance, &gpu_count, NULL);

	if (gpu_count == 0) {
		blog(LOG_ERROR, "No Vulkan-capable GPU found");
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	VkPhysicalDevice *gpus = bmalloc(sizeof(VkPhysicalDevice) * gpu_count);
	vkEnumeratePhysicalDevices(device->instance, &gpu_count, gpus);

	/* Use specified adapter or first available */
	if (adapter >= gpu_count)
		adapter = 0;

	device->gpu = gpus[adapter];
	bfree(gpus);

	VkPhysicalDeviceProperties gpu_props;
	vkGetPhysicalDeviceProperties(device->gpu, &gpu_props);

	blog(LOG_INFO, "Using GPU: %s (Driver: %d.%d.%d)", gpu_props.deviceName, VK_VERSION_MAJOR(gpu_props.driverVersion),
	     VK_VERSION_MINOR(gpu_props.driverVersion), VK_VERSION_PATCH(gpu_props.driverVersion));

	/* Find graphics queue family */
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device->gpu, &queue_family_count, NULL);

	VkQueueFamilyProperties *queue_families = bmalloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device->gpu, &queue_family_count, queue_families);

	for (uint32_t i = 0; i < queue_family_count; i++) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			device->graphics_queue_family_index = i;
			break;
		}
	}

	bfree(queue_families);

	/* Create logical device */
	VkDeviceQueueCreateInfo queue_create_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						      .queueFamilyIndex = device->graphics_queue_family_index,
						      .queueCount = 1};

	float queue_priority = 1.0f;
	queue_create_info.pQueuePriorities = &queue_priority;

	const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	VkDeviceCreateInfo device_create_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
						  .queueCreateInfoCount = 1,
						  .pQueueCreateInfos = &queue_create_info,
						  .enabledExtensionCount = 1,
						  .ppEnabledExtensionNames = device_extensions};

	result = vkCreateDevice(device->gpu, &device_create_info, NULL, &device->device);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create logical device: %s", vk_get_error_string(result));
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	/* 加载设备级别函数 */
	if (!vk_loader_load_device_functions(device->device)) {
		blog(LOG_ERROR, "Failed to load device functions");
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	vkGetDeviceQueue(device->device, device->graphics_queue_family_index, 0, &device->graphics_queue);

	/* Create command pool */
	VkCommandPoolCreateInfo pool_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
					      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
					      .queueFamilyIndex = device->graphics_queue_family_index};

	result = vkCreateCommandPool(device->device, &pool_info, NULL, &device->command_pool);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create command pool: %s", vk_get_error_string(result));
		errorcode = GS_ERROR_FAIL;
		goto fail;
	}

	/* Create command buffer */
	VkCommandBufferAllocateInfo cmd_buffer_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
						        .commandPool = device->command_pool,
						        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
						        .commandBufferCount = 1};

	result = vkAllocateCommandBuffers(device->device, &cmd_buffer_info, &device->command_buffer);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to allocate command buffer: %s", vk_get_error_string(result));
		errorcode = GS_ERROR_FAIL;
		goto fail;
	}

	/* Create descriptor pool */
	if (!vk_create_descriptor_pool(device)) {
		errorcode = GS_ERROR_FAIL;
		goto fail;
	}

	if (!vk_init_extensions(device)) {
		errorcode = GS_ERROR_NOT_SUPPORTED;
		goto fail;
	}

	/* Initialize sampler for raw texture loading */
	struct gs_sampler_info raw_load_info = {
		.filter = GS_FILTER_POINT,
		.address_u = GS_ADDRESS_BORDER,
		.address_v = GS_ADDRESS_BORDER,
		.address_w = GS_ADDRESS_BORDER,
		.max_anisotropy = 1,
		.border_color = 0,
	};
	device->raw_load_sampler = device_samplerstate_create(device, &raw_load_info);

	device->cur_swap = NULL;
	device->cur_color_space = GS_CS_SRGB;
	device->cur_cull_mode = GS_BACK;

	blog(LOG_INFO, "Vulkan loaded successfully");

	*p_device = device;
	return GS_SUCCESS;

fail:
	blog(LOG_ERROR, "device_create (Vulkan) failed");
	device_destroy(device);
	*p_device = NULL;
	return errorcode;
}

void device_destroy(gs_device_t *device)
{
	if (!device)
		return;

	if (device->device) {
		vkDeviceWaitIdle(device->device);

		if (device->descriptor_pool)
			vkDestroyDescriptorPool(device->device, device->descriptor_pool, NULL);

		if (device->command_pool)
			vkDestroyCommandPool(device->device, device->command_pool, NULL);

		if (device->raw_load_sampler)
			samplerstate_release(device->raw_load_sampler);

		while (device->first_program)
			gs_program_destroy(device->first_program);

		vkDestroyDevice(device->device, NULL);
	}

	if (device->instance)
		vkDestroyInstance(device->instance, NULL);

	da_free(device->proj_stack);

	/* 释放 Vulkan 加载器 */
	vk_loader_free();

	bfree(device);
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device, const struct gs_init_data *info)
{
	struct gs_swap_chain *swap = bzalloc(sizeof(struct gs_swap_chain));

	swap->device = device;
	swap->info = *info;
	swap->wi = vk_windowinfo_create(info);
	if (!swap->wi) {
		blog(LOG_ERROR, "device_swapchain_create (Vulkan) failed to create window info");
		gs_swapchain_destroy(swap);
		return NULL;
	}

	if (!vk_platform_init_swapchain(swap)) {
		blog(LOG_ERROR, "vk_platform_init_swapchain failed");
		gs_swapchain_destroy(swap);
		return NULL;
	}

	return swap;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	if (device->cur_swap) {
		device->cur_swap->info.cx = cx;
		device->cur_swap->info.cy = cy;
	} else {
		blog(LOG_WARNING, "device_resize (Vulkan): No active swap");
	}
}

enum gs_color_space device_get_color_space(gs_device_t *device)
{
	return device->cur_color_space;
}

void device_update_color_space(gs_device_t *device)
{
	if (!device->cur_swap)
		blog(LOG_WARNING, "device_update_color_space (Vulkan): No active swap");
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	if (device->cur_swap) {
		*cx = device->cur_swap->info.cx;
		*cy = device->cur_swap->info.cy;
	} else {
		blog(LOG_WARNING, "device_get_size (Vulkan): No active swap");
		*cx = 0;
		*cy = 0;
	}
}

uint32_t device_get_width(const gs_device_t *device)
{
	if (device->cur_swap) {
		return device->cur_swap->info.cx;
	} else {
		blog(LOG_WARNING, "device_get_width (Vulkan): No active swap");
		return 0;
	}
}

uint32_t device_get_height(const gs_device_t *device)
{
	if (device->cur_swap) {
		return device->cur_swap->info.cy;
	} else {
		blog(LOG_WARNING, "device_get_height (Vulkan): No active swap");
		return 0;
	}
}

gs_samplerstate_t *device_samplerstate_create(gs_device_t *device, const struct gs_sampler_info *info)
{
	struct gs_sampler_state *sampler = bzalloc(sizeof(struct gs_sampler_state));
	sampler->device = device;
	sampler->ref = 1;

	/* Convert filter modes */
	switch (info->filter) {
	case GS_FILTER_POINT:
		sampler->min_filter = VK_FILTER_NEAREST;
		sampler->mag_filter = VK_FILTER_NEAREST;
		sampler->mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		break;
	case GS_FILTER_LINEAR:
		sampler->min_filter = VK_FILTER_LINEAR;
		sampler->mag_filter = VK_FILTER_LINEAR;
		sampler->mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	case GS_FILTER_ANISOTROPIC:
		sampler->min_filter = VK_FILTER_LINEAR;
		sampler->mag_filter = VK_FILTER_LINEAR;
		sampler->mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		break;
	default:
		sampler->min_filter = VK_FILTER_NEAREST;
		sampler->mag_filter = VK_FILTER_NEAREST;
		sampler->mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}

	/* Convert address modes */
	switch (info->address_u) {
	case GS_ADDRESS_CLAMP:
		sampler->address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case GS_ADDRESS_WRAP:
		sampler->address_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case GS_ADDRESS_MIRROR:
		sampler->address_u = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	case GS_ADDRESS_BORDER:
		sampler->address_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	}

	switch (info->address_v) {
	case GS_ADDRESS_CLAMP:
		sampler->address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case GS_ADDRESS_WRAP:
		sampler->address_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case GS_ADDRESS_MIRROR:
		sampler->address_v = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	case GS_ADDRESS_BORDER:
		sampler->address_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	}

	switch (info->address_w) {
	case GS_ADDRESS_CLAMP:
		sampler->address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case GS_ADDRESS_WRAP:
		sampler->address_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case GS_ADDRESS_MIRROR:
		sampler->address_w = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	case GS_ADDRESS_BORDER:
		sampler->address_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		break;
	}

	sampler->max_anisotropy = (float)info->max_anisotropy;
	vec4_from_rgba(&sampler->border_color, info->border_color);

	/* Create Vulkan sampler */
	VkSamplerCreateInfo sampler_info = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
					     .magFilter = sampler->mag_filter,
					     .minFilter = sampler->min_filter,
					     .mipmapMode = sampler->mipmap_mode,
					     .addressModeU = sampler->address_u,
					     .addressModeV = sampler->address_v,
					     .addressModeW = sampler->address_w,
					     .mipLodBias = 0.0f,
					     .anisotropyEnable = (sampler->max_anisotropy > 1.0f) ? VK_TRUE : VK_FALSE,
					     .maxAnisotropy = sampler->max_anisotropy,
					     .compareEnable = VK_FALSE,
					     .compareOp = VK_COMPARE_OP_ALWAYS,
					     .minLod = 0.0f,
					     .maxLod = 0.0f,
					     .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
					     .unnormalizedCoordinates = VK_FALSE};

	VkResult result = vkCreateSampler(device->device, &sampler_info, NULL, &sampler->sampler);
	if (result != VK_SUCCESS) {
		blog(LOG_ERROR, "Failed to create sampler: %s", vk_get_error_string(result));
		bfree(sampler);
		return NULL;
	}

	return sampler;
}

gs_timer_t *device_timer_create(gs_device_t *device)
{
	UNUSED_PARAMETER(device);

	/* TODO: Implement Vulkan timer queries */
	struct gs_timer *timer = bzalloc(sizeof(struct gs_timer));
	return timer;
}

gs_timer_range_t *device_timer_range_create(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return NULL;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	return texture->type;
}

void device_begin_frame(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	/* Vulkan-specific frame setup */
}

void device_begin_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	/* Clear any cached state if needed */
}

void device_end_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	/* End of scene tasks */
}

void device_clear(gs_device_t *device, uint32_t clear_flags, const struct vec4 *color, float depth, uint8_t stencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(clear_flags);
	UNUSED_PARAMETER(color);
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(stencil);

	/* TODO: Implement Vulkan clear operations */
}

void device_flush(gs_device_t *device)
{
	if (device->device)
		vkDeviceWaitIdle(device->device);
}

void device_present(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	/* Present current frame to swapchain */
}

bool device_is_present_ready(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return true;
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	device->cur_cull_mode = mode;
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	return device->cur_cull_mode;
}

/* Texture operations */

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width, uint32_t height,
				   enum gs_color_format color_format, uint32_t levels, const uint8_t **data,
				   uint32_t flags)
{
	/* Implemented in vulkan-texture2d.c */
	device_texture_2d_create(device, width, height, color_format, levels, data, flags);
	return NULL;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size, enum gs_color_format color_format,
				       uint32_t levels, const uint8_t **data, uint32_t flags)
{
	/* Implemented in vulkan-texturecube.c */
	device_cubetexture_create(device, size, color_format, levels, data, flags);
	return NULL;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
				      enum gs_color_format color_format, uint32_t levels, const uint8_t *const *data,
				      uint32_t flags)
{
	/* Implemented in vulkan-texture3d.c */
	device_texture_3d_create(device, width, height, depth, color_format, levels, data, flags);
	return NULL;
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	if (!device->cur_pixel_shader)
		return;

	if (unit < 0 || unit >= GS_MAX_TEXTURES)
		return;

	device->cur_textures[unit] = tex;
}

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	/* Vulkan handles sRGB through format selection, not through glTexParameteri */
	device_load_texture(device, tex, unit);
}

void device_load_samplerstate(gs_device_t *device, gs_samplerstate_t *sampler, int unit)
{
	if (!device->cur_pixel_shader)
		sampler = NULL;

	if (unit < 0 || unit >= GS_MAX_TEXTURES)
		return;

	device->cur_samplers[unit] = sampler;
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst, gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_copy_texture_region(gs_device_t *device, gs_texture_t *dst, uint32_t dst_x, uint32_t dst_y,
				gs_texture_t *src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(dst);
	UNUSED_PARAMETER(dst_x);
	UNUSED_PARAMETER(dst_y);
	UNUSED_PARAMETER(src);
	UNUSED_PARAMETER(src_x);
	UNUSED_PARAMETER(src_y);
	UNUSED_PARAMETER(src_w);
	UNUSED_PARAMETER(src_h);

	/* TODO: Implement Vulkan texture copy operations */
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode, uint32_t start_vert, uint32_t num_verts)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(draw_mode);
	UNUSED_PARAMETER(start_vert);
	UNUSED_PARAMETER(num_verts);

	/* TODO: Implement Vulkan draw operations */
}

/* Stub implementations for remaining functions */
void device_set_render_target(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(zstencil);
}

void device_set_viewport(gs_device_t *device, int x, int y, int width, int height)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(x);
	UNUSED_PARAMETER(y);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(vertbuffer);
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(indexbuffer);
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	if (device->cur_vertex_shader == vertshader)
		return;

	if (vertshader && vertshader->type != GS_SHADER_VERTEX) {
		blog(LOG_ERROR, "Specified shader is not a vertex shader");
		return;
	}

	device->cur_vertex_shader = vertshader;
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	if (device->cur_pixel_shader == pixelshader)
		return;

	if (pixelshader && pixelshader->type != GS_SHADER_PIXEL) {
		blog(LOG_ERROR, "Specified shader is not a pixel shader");
		return;
	}

	device->cur_pixel_shader = pixelshader;
}

gs_shader_t *device_get_vertex_shader(const gs_device_t *device)
{
	return device->cur_vertex_shader;
}

gs_shader_t *device_get_pixel_shader(const gs_device_t *device)
{
	return device->cur_pixel_shader;
}

gs_texture_t *device_get_render_target(const gs_device_t *device)
{
	return device->cur_render_target;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	return device->cur_zstencil_buffer;
}

void gs_swapchain_destroy(gs_swapchain_t *swapchain)
{
	if (!swapchain)
		return;

	if (swapchain->device->cur_swap == swapchain)
		device_load_swapchain(swapchain->device, NULL);

	vk_platform_cleanup_swapchain(swapchain);

	if (swapchain->wi)
		vk_windowinfo_destroy(swapchain->wi);

	bfree(swapchain);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	device->cur_swap = swapchain;
}

void device_enter_context(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_leave_context(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void *device_get_device_obj(gs_device_t *device)
{
	return device->device;
}

bool device_enum_adapters(gs_device_t *device, bool (*callback)(void *param, const char *name, uint32_t id),
			  void *param)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(callback);
	UNUSED_PARAMETER(param);

	/* TODO: Implement adapter enumeration */
	return false;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_color(gs_device_t *device, bool red, bool green, bool blue, bool alpha)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(red);
	UNUSED_PARAMETER(green);
	UNUSED_PARAMETER(blue);
	UNUSED_PARAMETER(alpha);
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src, enum gs_blend_type dest)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(src);
	UNUSED_PARAMETER(dest);
}

void device_blend_function_separate(gs_device_t *device, enum gs_blend_type src_c, enum gs_blend_type dest_c,
				    enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(src_c);
	UNUSED_PARAMETER(dest_c);
	UNUSED_PARAMETER(src_a);
	UNUSED_PARAMETER(dest_a);
}

void device_blend_op(gs_device_t *device, enum gs_blend_op_type op)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(op);
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(test);
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side, enum gs_depth_test test)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(test);
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side, enum gs_stencil_op_type fail,
		       enum gs_stencil_op_type zfail, enum gs_stencil_op_type zpass)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(fail);
	UNUSED_PARAMETER(zfail);
	UNUSED_PARAMETER(zpass);
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(rect);
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	if (rect)
		*rect = device->cur_viewport;
}

void device_ortho(gs_device_t *device, float left, float right, float top, float bottom, float near, float far)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(left);
	UNUSED_PARAMETER(right);
	UNUSED_PARAMETER(top);
	UNUSED_PARAMETER(bottom);
	UNUSED_PARAMETER(near);
	UNUSED_PARAMETER(far);
}

void device_frustum(gs_device_t *device, float left, float right, float top, float bottom, float near, float far)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(left);
	UNUSED_PARAMETER(right);
	UNUSED_PARAMETER(top);
	UNUSED_PARAMETER(bottom);
	UNUSED_PARAMETER(near);
	UNUSED_PARAMETER(far);
}

void device_projection_push(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_projection_pop(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_debug_marker_begin(gs_device_t *device, const char *markername, const float color[4])
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(markername);
	UNUSED_PARAMETER(color);
}

void device_debug_marker_end(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst, gs_texture_t *src)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(dst);
	UNUSED_PARAMETER(src);
}

void device_enable_framebuffer_srgb(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

bool device_framebuffer_srgb_enabled(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return false;
}

bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(monitor);
	return false;
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *cubetex, int side, gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(cubetex);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(zstencil);
}

void device_set_render_target_with_color_space(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil,
					       enum gs_color_space space)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(zstencil);
	UNUSED_PARAMETER(space);
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width, uint32_t height,
				      enum gs_zstencil_format format)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(format);

	return NULL;
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width, uint32_t height,
					   enum gs_color_format color_format)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(color_format);

	return NULL;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device, const char *shader, const char *file, char **error_string)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(shader);
	UNUSED_PARAMETER(file);
	UNUSED_PARAMETER(error_string);

	return NULL;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device, const char *shader, const char *file, char **error_string)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(shader);
	UNUSED_PARAMETER(file);
	UNUSED_PARAMETER(error_string);

	return NULL;
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device, struct gs_vb_data *data, uint32_t flags)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);

	return NULL;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device, enum gs_index_type type, void *indices, size_t num,
					   uint32_t flags)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(type);
	UNUSED_PARAMETER(indices);
	UNUSED_PARAMETER(num);
	UNUSED_PARAMETER(flags);

	return NULL;
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

bool device_nv12_available(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return false;
}

bool device_p010_available(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return false;
}

bool device_shared_texture_available(void)
{
	return false;
}
