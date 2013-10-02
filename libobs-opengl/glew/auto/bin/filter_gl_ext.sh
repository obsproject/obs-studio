#!/bin/bash
##
## Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
## Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
##
## This program is distributed under the terms and conditions of the GNU
## General Public License Version 2 as published by the Free Software
## Foundation or, at your option, any later version.
##
## Parameters:
##
##       $1: Extensions directory

set -e

# fix GL_NV_texture_compression_vtc
    grep -v EXT $1/GL_NV_texture_compression_vtc > tmp
    mv tmp $1/GL_NV_texture_compression_vtc

# remove duplicates from GL_ARB_vertex_program and GL_ARB_fragment_program
    grep -v -F -f $1/GL_ARB_vertex_program $1/GL_ARB_fragment_program > tmp
    mv tmp $1/GL_ARB_fragment_program

# remove duplicates from GLX_EXT_visual_rating and GLX_EXT_visual_info
    grep -v -F -f $1/GLX_EXT_visual_info $1/GLX_EXT_visual_rating > tmp
    mv tmp $1/GLX_EXT_visual_rating

# GL_EXT_draw_buffers2 and GL_EXT_transform_feedback both define glGetBooleanIndexedvEXT but with different parameter names
    grep -v glGetBooleanIndexedvEXT $1/GL_EXT_transform_feedback > tmp
    mv tmp $1/GL_EXT_transform_feedback    

# GL_EXT_draw_buffers2 and GL_EXT_transform_feedback both define glGetIntegerIndexedvEXT but with different parameter names
    grep -v glGetIntegerIndexedvEXT $1/GL_EXT_transform_feedback > tmp
    mv tmp $1/GL_EXT_transform_feedback    

# remove duplicates from GL_NV_video_capture and GLX_NV_video_capture
    grep -v glX $1/GL_NV_video_capture > tmp
    mv tmp $1/GL_NV_video_capture

# add missing functions to GL_NV_video_capture
	cat >> $1/GL_NV_video_capture <<EOT
    void glGetVideoCaptureStreamivNV (GLuint video_capture_slot, GLuint stream, GLenum pname, GLint* params)
    void glGetVideoCaptureStreamfvNV (GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat* params)
    void glGetVideoCaptureStreamdvNV (GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble* params)
    void glVideoCaptureStreamParameterivNV (GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint* params)
    void glVideoCaptureStreamParameterfvNV (GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat* params)
    void glVideoCaptureStreamParameterdvNV (GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble* params)
EOT

# fix WGL_NV_video_capture
    cat >> $1/WGL_NV_video_capture <<EOT
    DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
EOT

# fix GLX_NV_video_capture
    cat >> $1/GLX_NV_video_capture <<EOT
    typedef XID GLXVideoCaptureDeviceNV
EOT

# remove duplicates from GL_NV_present_video and GLX_NV_present_video
    grep -v -F -f $1/GLX_NV_present_video $1/GL_NV_present_video > tmp
    mv tmp $1/GL_NV_present_video

# fix WGL_NV_present_video
    cat >> $1/WGL_NV_present_video <<EOT
    DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
EOT

# fix WGL_NV_video_output
    cat >> $1/WGL_NV_video_output <<EOT
    DECLARE_HANDLE(HPVIDEODEV);
EOT

# fix GL_NV_occlusion_query and GL_HP_occlusion_test
    grep -v '_HP' $1/GL_NV_occlusion_query > tmp
    mv tmp $1/GL_NV_occlusion_query
    perl -e's/OCCLUSION_TEST_HP.*/OCCLUSION_TEST_HP 0x8165/' -pi \
	$1/GL_HP_occlusion_test
    perl -e's/OCCLUSION_TEST_RESULT_HP.*/OCCLUSION_TEST_RESULT_HP 0x8166/' -pi \
	$1/GL_HP_occlusion_test

# fix GLvoid in GL_ARB_vertex_buffer_objects
    perl -e 's/ void\*/ GLvoid\*/g' -pi \
        $1/GL_ARB_vertex_buffer_object

# add deprecated constants to GL_ATI_fragment_shader
    cat >> $1/GL_ATI_fragment_shader <<EOT
	GL_NUM_FRAGMENT_REGISTERS_ATI 0x896E
	GL_NUM_FRAGMENT_CONSTANTS_ATI 0x896F
	GL_NUM_PASSES_ATI 0x8970
	GL_NUM_INSTRUCTIONS_PER_PASS_ATI 0x8971
	GL_NUM_INSTRUCTIONS_TOTAL_ATI 0x8972
	GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI 0x8973
	GL_NUM_LOOPBACK_COMPONENTS_ATI 0x8974
	GL_COLOR_ALPHA_PAIRING_ATI 0x8975
	GL_SWIZZLE_STRQ_ATI 0x897A
	GL_SWIZZLE_STRQ_DQ_ATI 0x897B
EOT

# add deprecated constants to GL_NV_texture_shader
    cat >> $1/GL_NV_texture_shader <<EOT
	GL_OFFSET_TEXTURE_2D_MATRIX_NV 0x86E1
	GL_OFFSET_TEXTURE_2D_BIAS_NV 0x86E3
	GL_OFFSET_TEXTURE_2D_SCALE_NV 0x86E2
EOT
	
# fix WGL_ATI_pixel_format_float
    cat >> $1/WGL_ATI_pixel_format_float <<EOT
	GL_RGBA_FLOAT_MODE_ATI 0x8820
	GL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI 0x8835
EOT

# fix WGL_ARB_make_current_read
    cat >> $1/WGL_ARB_make_current_read <<EOT
	ERROR_INVALID_PIXEL_TYPE_ARB 0x2043
	ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
EOT

# fix WGL_EXT_make_current_read
    cat >> $1/WGL_EXT_make_current_read <<EOT
	ERROR_INVALID_PIXEL_TYPE_EXT 0x2043
EOT

# add typedefs to GL_ARB_vertex_buffer_object; (from personal communication
# with Marco Fabbricatore).
#
# Rationale.  The spec says:
#
#   "Both types are defined as signed integers large enough to contain
#   any pointer value [...] The idea of making these types unsigned was
#   considered, but was ultimately rejected ..."
    cat >> $1/GL_ARB_vertex_buffer_object <<EOT
	typedef ptrdiff_t GLsizeiptrARB
	typedef ptrdiff_t GLintptrARB
EOT

# add typedefs to GLX_EXT_import_context
    cat >> $1/GLX_EXT_import_context <<EOT
	typedef XID GLXContextID
EOT

# add tokens to GLX_OML_swap_method
    cat >> $1/GLX_OML_swap_method <<EOT
	GLX_SWAP_EXCHANGE_OML 0x8061
	GLX_SWAP_COPY_OML 0x8062
	GLX_SWAP_UNDEFINED_OML 0x8063
EOT

# add typedefs to GLX_SGIX_fbconfig
    cat >> $1/GLX_SGIX_fbconfig <<EOT
	typedef XID GLXFBConfigIDSGIX
	typedef struct __GLXFBConfigRec *GLXFBConfigSGIX
EOT

# add typedefs to GLX_SGIX_pbuffer
    cat >> $1/GLX_SGIX_pbuffer <<EOT
	typedef XID GLXPbufferSGIX
	typedef struct { int type; unsigned long serial; Bool send_event; Display *display; GLXDrawable drawable; int event_type; int draw_type; unsigned int mask; int x, y; int width, height; int count; } GLXBufferClobberEventSGIX
EOT

# add typedef to GL_NV_half_float
    cat >> $1/GL_NV_half_float <<EOT
	typedef unsigned short GLhalf
EOT

# add handle to WGL_ARB_pbuffer
    cat >> $1/WGL_ARB_pbuffer <<EOT
	DECLARE_HANDLE(HPBUFFERARB);
EOT

# add handle to WGL_EXT_pbuffer
    cat >> $1/WGL_EXT_pbuffer <<EOT
	DECLARE_HANDLE(HPBUFFEREXT);
EOT

# get rid of GL_SUN_multi_draw_arrays
    rm -f $1/GL_SUN_multi_draw_arrays

# change variable names in GL_ARB_vertex_shader
    perl -e 's/v0/x/g' -pi $1/GL_ARB_vertex_shader
    perl -e 's/v1/y/g' -pi $1/GL_ARB_vertex_shader
    perl -e 's/v2/z/g' -pi $1/GL_ARB_vertex_shader
    perl -e 's/v3/w/g' -pi $1/GL_ARB_vertex_shader

# remove triplicates in GL_ARB_shader_objects, GL_ARB_fragment_shader, 
# and GL_ARB_vertex_shader
    grep -v -F -f $1/GL_ARB_shader_objects $1/GL_ARB_fragment_shader > tmp
    mv tmp $1/GL_ARB_fragment_shader
    grep -v -F -f $1/GL_ARB_shader_objects $1/GL_ARB_vertex_shader > tmp
    mv tmp $1/GL_ARB_vertex_shader

# remove duplicates in GL_ARB_vertex_program and GL_ARB_vertex_shader
    grep -v -F -f $1/GL_ARB_vertex_program $1/GL_ARB_vertex_shader > tmp
    mv tmp $1/GL_ARB_vertex_shader

# remove triplicates in GL_ARB_fragment_program, GL_ARB_fragment_shader,
# and GL_ARB_vertex_shader
    grep -v -F -f $1/GL_ARB_fragment_program $1/GL_ARB_fragment_shader > tmp
    mv tmp $1/GL_ARB_fragment_shader
    grep -v -F -f $1/GL_ARB_fragment_program $1/GL_ARB_vertex_shader > tmp
    mv tmp $1/GL_ARB_vertex_shader

# remove duplicates in GL_EXT_direct_state_access
    grep -v "glGetBooleanIndexedvEXT" $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access
    grep -v "glGetIntegerIndexedvEXT" $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access
    grep -v "glDisableIndexedEXT" $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access
    grep -v "glEnableIndexedEXT" $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access
    grep -v "glIsEnabledIndexedEXT" $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access

# remove duplicates in GL_NV_explicit_multisample
    grep -v "glGetBooleanIndexedvEXT" $1/GL_NV_explicit_multisample > tmp
    mv tmp $1/GL_NV_explicit_multisample
    grep -v "glGetIntegerIndexedvEXT" $1/GL_NV_explicit_multisample > tmp
    mv tmp $1/GL_NV_explicit_multisample

# fix bugs in GL_ARB_vertex_shader
    grep -v "GL_FLOAT" $1/GL_ARB_vertex_shader > tmp
    mv tmp $1/GL_ARB_vertex_shader
    perl -e 's/handle /GLhandleARB /g' -pi $1/GL_ARB_vertex_shader

# fix bugs in GL_ARB_shader_objects
    grep -v "GL_FLOAT " $1/GL_ARB_shader_objects > tmp
    mv tmp $1/GL_ARB_shader_objects
    grep -v "GL_INT " $1/GL_ARB_shader_objects > tmp
    mv tmp $1/GL_ARB_shader_objects

# add typedefs to GL_ARB_shader_objects
    cat >> $1/GL_ARB_shader_objects <<EOT
	typedef char GLcharARB
	typedef unsigned int GLhandleARB
EOT

# add missing functions to GL_ARB_transpose_matrix
	cat >> $1/GL_ARB_transpose_matrix <<EOT
	void glLoadTransposeMatrixfARB (GLfloat m[16])
	void glLoadTransposeMatrixdARB (GLdouble m[16])
	void glMultTransposeMatrixfARB (GLfloat m[16])
	void glMultTransposeMatrixdARB (GLdouble m[16])
EOT

# add missing tokens to GL_EXT_framebuffer_multisample
	cat >> $1/GL_EXT_framebuffer_multisample <<EOT
	GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
	GL_MAX_SAMPLES_EXT 0x8D57
EOT

# Filter out GL_NV_gpu_program_fp64 enums and functions
    head -n3 $1/GL_NV_gpu_program_fp64 > tmp
    mv tmp $1/GL_NV_gpu_program_fp64

# Filter glGetUniformui64vNV from GL_NV_shader_buffer_load
    grep -v "glGetUniformui64vNV" $1/GL_NV_shader_buffer_load > tmp
    mv tmp $1/GL_NV_shader_buffer_load

# Filter out profile enumerations from GLX_ARB_create_context
    grep -v "_PROFILE_" $1/GLX_ARB_create_context > tmp
    mv tmp $1/GLX_ARB_create_context

# Filter only profile related enumerations for GLX_ARB_create_context_profile
    head -n3 $1/GLX_ARB_create_context_profile > tmp
    grep "_PROFILE_" $1/GLX_ARB_create_context_profile >> tmp
    mv tmp $1/GLX_ARB_create_context_profile

# Filter out profile enumerations from WGL_ARB_create_context
    grep -v "_PROFILE_" $1/WGL_ARB_create_context > tmp
    mv tmp $1/WGL_ARB_create_context

# Filter only profile related enumerations for WGL_ARB_create_context_profile
    head -n3 $1/WGL_ARB_create_context_profile > tmp
    grep "_PROFILE_" $1/WGL_ARB_create_context_profile >> tmp
    mv tmp $1/WGL_ARB_create_context_profile

# add missing function to GLX_NV_copy_image
	cat >> $1/GLX_NV_copy_image <<EOT
  void glXCopyImageSubDataNV (Display *dpy, GLXContext srcCtx, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLXContext dstCtx, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth)
EOT

# add missing function to WGL_NV_copy_image
	cat >> $1/WGL_NV_copy_image <<EOT
  BOOL wglCopyImageSubDataNV (HGLRC hSrcRC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, HGLRC hDstRC, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth)
EOT

# Filter glProgramParameteri from GL_ARB_separate_shader_objects
#    grep -v "glProgramParameteri" $1/GL_ARB_separate_shader_objects > tmp
#    mv tmp $1/GL_ARB_separate_shader_objects

# Filter out EXT functions from GL_ARB_viewport_array
    grep -v "EXT" $1/GL_ARB_viewport_array > tmp
    mv tmp $1/GL_ARB_viewport_array

# Additional enumerations for GL_NV_vertex_buffer_unified_memory
# These are mentioned in GL_ARB_draw_indirect.txt

    cat >> $1/GL_NV_vertex_buffer_unified_memory <<EOT
	GL_DRAW_INDIRECT_UNIFIED_NV 0x8F40
	GL_DRAW_INDIRECT_ADDRESS_NV 0x8F41
	GL_DRAW_INDIRECT_LENGTH_NV  0x8F42
EOT

# Filter glGetPointerv from GL_ARB_debug_output
# It's part of OpenGL 1.1, after all

    grep -v "glGetPointerv" $1/GL_ARB_debug_output > tmp
    mv tmp $1/GL_ARB_debug_output

# Filter glGetPointerv from GL_EXT_vertex_array
# It's part of OpenGL 1.1, after all

    grep -v "glGetPointerv" $1/GL_EXT_vertex_array > tmp
    mv tmp $1/GL_EXT_vertex_array

# add typedef to GL_AMD_debug_output
# parse_spec.pl can't parse typedefs from New Types section, but ought to
    cat >> $1/GL_AMD_debug_output <<EOT
	typedef void (APIENTRY *GLDEBUGPROCAMD)(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
EOT

# add typedef to GL_ARB_debug_output
# parse_spec.pl can't parse typedefs from New Types section, but ought to
    cat >> $1/GL_ARB_debug_output <<EOT
	typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
EOT

# add typedef to GL_KHR_debug
# parse_spec.pl can't parse typedefs from New Types section, but ought to
    cat >> $1/GL_KHR_debug <<EOT
	typedef void (APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
EOT

# Remove glGetPointerv from GL_KHR_debug
    grep -v "glGetPointerv" $1/GL_KHR_debug > tmp
    mv tmp $1/GL_KHR_debug

# Remove GL_ARB_debug_group, GL_ARB_debug_label and GL_ARB_debug_output2, for now
    rm -f $1/GL_ARB_debug_group
    rm -f $1/GL_ARB_debug_label
    rm -f $1/GL_ARB_debug_output2

# add typedefs to GL_ARB_cl_event
# parse_spec.pl can't parse typedefs from New Types section, but ought to
    cat >> $1/GL_ARB_cl_event <<EOT
	typedef struct _cl_context *cl_context
	typedef struct _cl_event *cl_event
EOT

# Filter out EXT functions from GL_ARB_gpu_shader_fp64
    grep -v 'EXT ' $1/GL_ARB_gpu_shader_fp64 > tmp
    mv tmp $1/GL_ARB_gpu_shader_fp64

# add missing functions to GL_EXT_direct_state_access (GL_ARB_gpu_shader_fp64 related)
	cat >> $1/GL_EXT_direct_state_access <<EOT
    void glProgramUniform1dEXT (GLuint program, GLint location, GLdouble x)
    void glProgramUniform2dEXT (GLuint program, GLint location, GLdouble x, GLdouble y)
    void glProgramUniform3dEXT (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z)
    void glProgramUniform4dEXT (GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
    void glProgramUniform1dvEXT (GLuint program, GLint location, GLsizei count, const GLdouble *value)
    void glProgramUniform2dvEXT (GLuint program, GLint location, GLsizei count, const GLdouble *value)
    void glProgramUniform3dvEXT (GLuint program, GLint location, GLsizei count, const GLdouble *value)
    void glProgramUniform4dvEXT (GLuint program, GLint location, GLsizei count, const GLdouble *value)
    void glProgramUniformMatrix2dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix3dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix4dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix2x3dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix2x4dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix3x2dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix3x4dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix4x2dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
    void glProgramUniformMatrix4x3dvEXT (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value)
EOT

# Filter out GL_UNSIGNED_INT and GL_FLOAT from GL_AMD_performance_monitor
    grep -v 'GL_UNSIGNED_INT ' $1/GL_AMD_performance_monitor > tmp
    mv tmp $1/GL_AMD_performance_monitor
    grep -v 'GL_FLOAT ' $1/GL_AMD_performance_monitor > tmp
    mv tmp $1/GL_AMD_performance_monitor

# Filter out GL_STORAGE_CACHED_APPLE and GL_STORAGE_SHARED_APPLE from GL_APPLE_texture_range
    grep -v 'GL_STORAGE_CACHED_APPLE ' $1/GL_APPLE_texture_range > tmp
    mv tmp $1/GL_APPLE_texture_range
    grep -v 'GL_STORAGE_SHARED_APPLE ' $1/GL_APPLE_texture_range > tmp
    mv tmp $1/GL_APPLE_texture_range

# Filter out GL_RED from GL_ARB_texture_rg
    grep -v 'GL_RED ' $1/GL_ARB_texture_rg > tmp
    mv tmp $1/GL_ARB_texture_rg

# Filter out _EXT enums from GL_ARB_texture_storage
    grep -v '_EXT ' $1/GL_ARB_texture_storage > tmp
    mv tmp $1/GL_ARB_texture_storage

# Filter out TEXTURE_3D enums from GL_EXT_paletted_texture
    grep -v 'TEXTURE_3D' $1/GL_EXT_paletted_texture > tmp
    mv tmp $1/GL_EXT_paletted_texture

# Filter out GL_VERSION_1_1 enums from GL_AMD_stencil_operation_extended
    grep -v '0x150' $1/GL_AMD_stencil_operation_extended > tmp
    mv tmp $1/GL_AMD_stencil_operation_extended

# Filter out from GL_APPLE_ycbcr_422
    grep -v 'GL_UNSIGNED_SHORT_8_8_APPLE' $1/GL_APPLE_ycbcr_422 > tmp
    mv tmp $1/GL_APPLE_ycbcr_422
    grep -v 'GL_UNSIGNED_SHORT_8_8_REV_APPLE' $1/GL_APPLE_ycbcr_422 > tmp
    mv tmp $1/GL_APPLE_ycbcr_422

# Filter out GL_FRAGMENT_DEPTH_EXT from GL_EXT_light_texture
    grep -v 'GL_FRAGMENT_DEPTH_EXT' $1/GL_EXT_light_texture > tmp
    mv tmp $1/GL_EXT_light_texture

# Filter out GL_MULTISAMPLE_BIT_EXT from GL_SGIS_multisample
    grep -v 'GL_MULTISAMPLE_BIT_EXT' $1/GL_SGIS_multisample > tmp
    mv tmp $1/GL_SGIS_multisample

# Filter out GL_COMPRESSED_RGB_S3TC_DXT1_EXT from GL_EXT_texture_compression_dxt1
    grep -v 'GL_COMPRESSED_RGB_S3TC_DXT1_EXT' $1/GL_EXT_texture_compression_dxt1 > tmp
    mv tmp $1/GL_EXT_texture_compression_dxt1

# Filter out GL_COMPRESSED_RGBA_S3TC_DXT1_EXT from GL_EXT_texture_compression_dxt1
    grep -v 'GL_COMPRESSED_RGBA_S3TC_DXT1_EXT' $1/GL_EXT_texture_compression_dxt1 > tmp
    mv tmp $1/GL_EXT_texture_compression_dxt1

# Append GLfixed to GL_ARB_ES2_compatibility
# Probably ought to be explicitly mentioned in the spec language

    cat >> $1/GL_ARB_ES2_compatibility <<EOT
	typedef int GLfixed
EOT

# Append GLclampx to GL_REGAL_ES1_0_compatibility
# Probably ought to be explicitly mentioned in the spec language

    cat >> $1/GL_REGAL_ES1_0_compatibility <<EOT
	typedef int GLclampx
EOT

# Append GLLOGPROCREGAL to GL_REGAL_log
# Probably ought to be explicitly mentioned in the spec language

    cat >> $1/GL_REGAL_log <<EOT
	typedef void (APIENTRY *LOGPROCREGAL)(GLenum stream, GLsizei length, const GLchar *message, GLvoid *context)
EOT

# Fixup LOGPROCREGAL -> GLLOGPROCREGAL
    perl -e 's/LOGPROCREGAL/GLLOGPROCREGAL/g' -pi $1/GL_REGAL_log

# Filter out GL_BYTE from GL_OES_byte_coordinates
    grep -v 'GL_BYTE' $1/GL_OES_byte_coordinates > tmp
    mv tmp $1/GL_OES_byte_coordinates

# Filter out fp64 (not widely supported) from GL_EXT_direct_state_access
    egrep -v 'glProgramUniform.*[1234]d[v]?EXT' $1/GL_EXT_direct_state_access > tmp
    mv tmp $1/GL_EXT_direct_state_access

# Filter out all enums from GL_ANGLE_depth_texture
    grep -v '0x' $1/GL_ANGLE_depth_texture > tmp
    mv tmp $1/GL_ANGLE_depth_texture

# Filter out GL_NONE enum from GL_ANGLE_depth_texture
    grep -v 'GL_NONE' $1/GL_ANGLE_texture_usage > tmp
    mv tmp $1/GL_ANGLE_texture_usage

# Fixup REGAL and ANGLE urls

    for i in $1/GL_REGAL_*; do perl -e 's#http://www.opengl.org/registry/specs/gl/REGAL/.*#https://github.com/p3/regal/tree/master/doc/extensions#g' -pi $i; done
    for i in $1/GL_ANGLE_*; do perl -e 's#http://www.opengl.org/registry/specs/gl/ANGLE/.*#https://code.google.com/p/angleproject/source/browse/\#git%2Fextensions#g' -pi $i; done

# Filter out GL_NV_blend_equation_advanced_coherent enums and functions
    head -n3 $1/GL_NV_blend_equation_advanced_coherent > tmp
    mv tmp $1/GL_NV_blend_equation_advanced_coherent

# clean up
    rm -f $1/*.bak
