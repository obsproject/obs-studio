#include <obs.h>
#include <graphics/graphics.h>
#include <util/dstr.h>
#include <graphics/image-file.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>

extern "C" {

// Text lookup for plugins
struct text_lookup {
	int dummy;
};
// obs_module_text removed
char *obs_module_get_config_path(obs_module_t *module, const char *file)
{
	return nullptr;
}
char *obs_module_get_config_path_arr(obs_module_t *module, const char *file)
{
	return nullptr;
}
void text_lookup_destroy(text_lookup *lookup) {}
bool text_lookup_getstr(text_lookup *lookup, const char *lookup_val, const char **out)
{
	*out = lookup_val;
	return true;
}
// obs_current_module removed
lookup_t *obs_module_load_locale(obs_module_t *module, const char *default_locale, const char *locale)
{
	return nullptr;
}

// Graphics Subsystem Stubs
// In a real engine, these would map to Vulkan resources.

gs_texture_t *gs_texture_create_from_file(const char *file)
{
	std::cout << "[GraphicsCompat] Stub: gs_texture_create_from_file(" << file << ")" << std::endl;
	// Return dummy non-null pointer to pretend success
	return (gs_texture_t *)0x1234;
}

void gs_texture_destroy(gs_texture_t *tex)
{
	std::cout << "[GraphicsCompat] Stub: gs_texture_destroy" << std::endl;
}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	return 1920;
}
uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	return 1080;
}

// Color Format
enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	return GS_RGBA;
}

// Drawing (Immediate Mode / Draw calls)
void gs_draw_sprite(gs_texture_t *tex, uint32_t flip, uint32_t width, uint32_t height)
{
	// This is where we would submit a Quad to the SceneManager in "Spatial Reality" mode
}

void gs_matrix_push(void) {}
void gs_matrix_pop(void) {}
void gs_matrix_scale3f(float x, float y, float z) {}
void gs_matrix_translate3f(float x, float y, float z) {}

// Effects (Shaders)
gs_effect_t *obs_get_base_effect(enum obs_base_effect effect)
{
	return (gs_effect_t *)0x5678;
}

gs_eparam_t *gs_effect_get_param_by_name(const gs_effect_t *effect, const char *name)
{
	return (gs_eparam_t *)0x1;
}

void gs_effect_set_texture(gs_eparam_t *param, gs_texture_t *val) {}
void gs_effect_set_vec4(gs_eparam_t *param, const struct vec4 *val) {}

// Sampler
gs_samplerstate_t *gs_samplerstate_create(const struct gs_sampler_info *info)
{
	return (gs_samplerstate_t *)0x2;
}
void gs_samplerstate_destroy(gs_samplerstate_t *sampler) {}

// Vertex Buffer
gs_vertbuffer_t *gs_vertbuffer_create(const struct gs_vb_data *data, uint32_t flags)
{
	return (gs_vertbuffer_t *)0x3;
}
void gs_vertbuffer_destroy(gs_vertbuffer_t *vb) {}

// Missing stubs from linker error
void gs_blend_state_push() {}
void gs_blend_state_pop() {}
void gs_blend_function(enum gs_blend_type src, enum gs_blend_type dest) {}
bool gs_framebuffer_srgb_enabled()
{
	return false;
}
void gs_enable_framebuffer_srgb(bool enable) {}

// Technique
gs_technique_t *gs_effect_get_technique(const gs_effect_t *effect, const char *name)
{
	return (gs_technique_t *)0x4;
}
size_t gs_technique_begin(gs_technique_t *tech)
{
	return 1;
}
void gs_technique_end_pass(gs_technique_t *tech) {}
void gs_technique_end(gs_technique_t *tech) {}
bool gs_technique_begin_pass(gs_technique_t *tech, size_t pass_idx)
{
	return true;
}

// Misc
void gs_effect_set_texture_srgb(gs_eparam_t *param, gs_texture_t *val) {}
uint32_t gs_get_height(void)
{
	return 1080;
}
uint32_t gs_get_width(void)
{
	return 1920;
}
enum gs_color_space gs_get_color_space(void)
{
	return GS_CS_SRGB;
}
void gs_image_file_free(struct gs_image_file *image) {}
void gs_image_file_init(struct gs_image_file *image, const char *file) {}
void gs_image_file_init_texture(struct gs_image_file *image) {}
bool gs_image_file_tick(struct gs_image_file *image, uint64_t elapsed_time_ns)
{
	return false;
}
void gs_image_file_update_texture(struct gs_image_file *image) {}
bool gs_get_linear_srgb()
{
	return false;
}

// OBS Core Stubs used by plugins
void obs_enter_graphics(void) {}
void obs_leave_graphics(void) {}
bool obs_source_showing(const obs_source_t *source)
{
	return true;
}
const char *obs_source_get_name(const obs_source_t *source)
{
	return "test_source";
}
// obs_properties_create etc are in ObsCompat.cpp
// obs_module_text etc are in image-source.c (via OBS_DECLARE_MODULE)

/* REMOVED:
obs_current_module
obs_module_text
obs_properties_create
obs_data_release
...
*/

// obs_source_* might be unique here though?
// Let's check linker errors again...
// It listed: obs_properties_create, obs_data_release, obs_data_get_string, obs_data_get_bool, obs_data_set_default_bool, obs_data_set_string.

// I will keep obs_source_* and others that were NOT listed as duplicates.
// But some of them might be in ObsCompat too.
// I will blindly remove the ones listed in linker error.

// obs_module_text, obs_current_module defined in image-source.c
// obs_properties_*, obs_data_* defined in ObsCompat.cpp

obs_data_t *obs_source_get_settings(const obs_source_t *source)
{
	return nullptr;
}
void obs_source_update(obs_source_t *source, obs_data_t *settings) {}

obs_property_t *obs_properties_add_path(obs_properties_t *props, const char *name, const char *description,
					enum obs_path_type type, const char *filter, const char *default_path)
{
	return (obs_property_t *)0x2;
}
obs_property_t *obs_properties_add_bool(obs_properties_t *props, const char *name, const char *description)
{
	return (obs_property_t *)0x2;
}
obs_property_t *obs_properties_add_color_alpha(obs_properties_t *props, const char *name, const char *description)
{
	return (obs_property_t *)0x2;
}
obs_property_t *obs_properties_add_int(obs_properties_t *props, const char *name, const char *description, int min,
				       int max, int step)
{
	return (obs_property_t *)0x2;
}
obs_missing_files_t *obs_missing_files_create(void)
{
	return nullptr;
}
obs_missing_file_t *obs_missing_file_create(const char *path, obs_missing_file_cb callback, int src_idx, void *data,
					    void *data2)
{
	return nullptr;
}
void obs_missing_files_add_file(obs_missing_files_t *files, obs_missing_file_t *file) {}
uint64_t obs_get_video_frame_time(void)
{
	return 0;
}

// Slideshow Stubs (referenced by image-source.c even if we don't compile slideshow)
struct obs_source_info slideshow_info = {};
struct obs_source_info slideshow_info_mk2 = {};

// Startup/Shutdown Stubs (missing in test_plugin_image)
bool obs_startup(const char *locale, const char *module_config_path, profiler_name_store_t *store)
{
	return true;
}
void obs_shutdown(void) {}
// Basic Registry for verification
static std::map<std::string, uint32_t> g_registered_sources;

uint32_t obs_get_source_output_flags(const char *id)
{
	if (g_registered_sources.count(id))
		return g_registered_sources[id];
	if (__builtin_strcmp(id, "image_source") == 0)
		return OBS_SOURCE_VIDEO;
	if (__builtin_strcmp(id, "color_source") == 0)
		return OBS_SOURCE_VIDEO;
	return 0; // Not found
}

// OS Stubs
bool os_file_exists(const char *path)
{
	return true;
}

void obs_register_source_s(const struct obs_source_info *info, size_t size)
{
	if (info && info->id) {
		std::cout << "[GraphicsCompat] Registered Source: " << info->id << " (Flags: " << info->output_flags
			  << ")" << std::endl;
		g_registered_sources[info->id] = info->output_flags;
	}
}

// GS Image File Stubs
// init_texture and free are static inline in header, so we don't define them.
// The others are external symbols.

void gs_image_file4_init(struct gs_image_file4 *if4, const char *file, enum gs_image_alpha_mode mode) {}
// gs_image_file4_init_texture(struct gs_image_file4 *if4) REMOVED
// gs_image_file4_free(struct gs_image_file4 *if4) REMOVED
bool gs_image_file4_tick(struct gs_image_file4 *if4, uint64_t elapsed)
{
	return false;
}
void gs_image_file4_update_texture(struct gs_image_file4 *if4) {}

} // extern "C"
