--[[ This function returns a table of core extensions and the versions they were made core in.

The table is indexed by version number (as a string). In each version is an array of extension names.

This list must be manually updated, as there is no equivalent in the spec files. Just add to the list. When a new version comes out with new core extensions, add a new list and add the local variable name to the master table as shown below.
]]

local coreExts1_2 = {
	"ARB_imaging",
};

local coreExts3_0 = {
	"ARB_vertex_array_object",
	"ARB_texture_rg",
	"ARB_texture_compression_rgtc",
	"ARB_map_buffer_range",
	"ARB_half_float_vertex",
	"ARB_framebuffer_sRGB",
	"ARB_framebuffer_object",
	"ARB_depth_buffer_float",
};

local coreExts3_1 = {
	"ARB_uniform_buffer_object",
	"ARB_copy_buffer",
};

local coreExts3_2 = {
	"ARB_depth_clamp",
	"ARB_draw_elements_base_vertex",
	"ARB_fragment_coord_conventions",
	"ARB_provoking_vertex",
	"ARB_seamless_cube_map",
	"ARB_sync",
	"ARB_texture_multisample",
	"ARB_vertex_array_bgra",
};

local coreExts3_3 = {
	"ARB_texture_rgb10_a2ui",
	"ARB_texture_swizzle",
	"ARB_timer_query",
	"ARB_vertex_type_2_10_10_10_rev",
	"ARB_blend_func_extended",
	"ARB_occlusion_query2",
	"ARB_sampler_objects",
};

local coreExts4_0 = {
	"ARB_draw_indirect",
	"ARB_gpu_shader5",
	"ARB_gpu_shader_fp64",
	"ARB_shader_subroutine",
	"ARB_tessellation_shader",
	"ARB_transform_feedback2",
	"ARB_transform_feedback3",
};

local coreExts4_1 = {
	"ARB_ES2_compatibility",
	"ARB_get_program_binary",
	"ARB_separate_shader_objects",
	"ARB_vertex_attrib_64bit",
	"ARB_viewport_array",
};

local coreExts4_2 = {
	"ARB_base_instance",
	"ARB_shading_language_420pack",
	"ARB_transform_feedback_instanced",
	"ARB_compressed_texture_pixel_storage",
	"ARB_conservative_depth",
	"ARB_internalformat_query",
	"ARB_map_buffer_alignment",
	"ARB_shader_atomic_counters",
	"ARB_shader_image_load_store",
	"ARB_shading_language_packing",
	"ARB_texture_storage",
};

local coreExts4_3 = {
	"KHR_debug",
	"ARB_arrays_of_arrays",
	"ARB_clear_buffer_object",
	"ARB_compute_shader",
	"ARB_copy_image",
	"ARB_ES3_compatibility",
	"ARB_explicit_uniform_location",
	"ARB_fragment_layer_viewport",
	"ARB_framebuffer_no_attachments",
	"ARB_internalformat_query2",
	"ARB_invalidate_subdata",
	"ARB_multi_draw_indirect",
	"ARB_program_interface_query",
	"ARB_shader_image_size",
	"ARB_shader_storage_buffer_object",
	"ARB_stencil_texturing",
	"ARB_texture_buffer_range",
	"ARB_texture_query_levels",
	"ARB_texture_storage_multisample",
	"ARB_texture_view",
	"ARB_vertex_attrib_binding",
};

return {
	["1.2"] = coreExts1_2,
	["3.0"] = coreExts3_0,
	["3.1"] = coreExts3_1,
	["3.2"] = coreExts3_2,
	["3.3"] = coreExts3_3,
	["4.0"] = coreExts4_0,
	["4.1"] = coreExts4_1,
	["4.2"] = coreExts4_2,
	["4.3"] = coreExts4_3,
};

