/******************************************************************************
    Copyright (C) 2020 by Georges Basile Stavracas Neto <georges.stavracas@gmail.com>

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

#include "gl-egl-common.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xf86drm.h>

#include <glad/glad_egl.h>

#if defined(__linux__)

#include <linux/types.h>
#include <asm/ioctl.h>
typedef unsigned int drm_handle_t;

#else

#include <stdint.h>
#include <sys/ioccom.h>
#include <sys/types.h>
typedef int8_t __s8;
typedef uint8_t __u8;
typedef int16_t __s16;
typedef uint16_t __u16;
typedef int32_t __s32;
typedef uint32_t __u32;
typedef int64_t __s64;
typedef uint64_t __u64;
typedef size_t __kernel_size_t;
typedef unsigned long drm_handle_t;

#endif

typedef void(APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, GLeglImageOES image);
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

static bool find_gl_extension(const char *extension)
{
	GLint n, i;

	glGetIntegerv(GL_NUM_EXTENSIONS, &n);
	for (i = 0; i < n; i++) {
		const char *e = (char *)glGetStringi(GL_EXTENSIONS, i);
		if (extension && strcmp(e, extension) == 0)
			return true;
	}
	return false;
}

static bool init_egl_image_target_texture_2d_ext(void)
{
	static bool initialized = false;

	if (!initialized) {
		initialized = true;

		if (!find_gl_extension("GL_OES_EGL_image")) {
			blog(LOG_ERROR, "No GL_OES_EGL_image");
			return false;
		}

		glEGLImageTargetTexture2DOES =
			(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
	}

	if (!glEGLImageTargetTexture2DOES)
		return false;

	return true;
}

static EGLImageKHR create_dmabuf_egl_image(EGLDisplay egl_display, unsigned int width, unsigned int height,
					   uint32_t drm_format, uint32_t n_planes, const int *fds,
					   const uint32_t *strides, const uint32_t *offsets, const uint64_t *modifiers)
{
	EGLAttrib attribs[47];
	int atti = 0;

	/* This requires the Mesa commit in
	 * Mesa 10.3 (08264e5dad4df448e7718e782ad9077902089a07) or
	 * Mesa 10.2.7 (55d28925e6109a4afd61f109e845a8a51bd17652).
	 * Otherwise Mesa closes the fd behind our back and re-importing
	 * will fail.
	 * https://bugs.freedesktop.org/show_bug.cgi?id=76188
	 * */

	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = drm_format;

	if (n_planes > 0) {
		attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs[atti++] = fds[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs[atti++] = offsets[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs[atti++] = strides[0];
		if (modifiers) {
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
			attribs[atti++] = modifiers[0] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
			attribs[atti++] = modifiers[0] >> 32;
		}
	}

	if (n_planes > 1) {
		attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
		attribs[atti++] = fds[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
		attribs[atti++] = offsets[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
		attribs[atti++] = strides[1];
		if (modifiers) {
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
			attribs[atti++] = modifiers[1] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
			attribs[atti++] = modifiers[1] >> 32;
		}
	}

	if (n_planes > 2) {
		attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
		attribs[atti++] = fds[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
		attribs[atti++] = offsets[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
		attribs[atti++] = strides[2];
		if (modifiers) {
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
			attribs[atti++] = modifiers[2] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
			attribs[atti++] = modifiers[2] >> 32;
		}
	}

	if (n_planes > 3) {
		attribs[atti++] = EGL_DMA_BUF_PLANE3_FD_EXT;
		attribs[atti++] = fds[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
		attribs[atti++] = offsets[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
		attribs[atti++] = strides[3];
		if (modifiers) {
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
			attribs[atti++] = modifiers[3] & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
			attribs[atti++] = modifiers[3] >> 32;
		}
	}

	attribs[atti++] = EGL_NONE;

	return eglCreateImage(egl_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0, attribs);
}

struct gs_texture *gl_egl_create_texture_from_eglimage(EGLDisplay egl_display, uint32_t width, uint32_t height,
						       enum gs_color_format color_format, EGLint target, EGLImage image)
{
	UNUSED_PARAMETER(egl_display);
	UNUSED_PARAMETER(target);

	struct gs_texture *texture = NULL;
	texture = gs_texture_create(width, height, color_format, 1, NULL, GS_GL_DUMMYTEX | GS_RENDER_TARGET);
	const GLuint gltex = *(GLuint *)gs_texture_get_obj(texture);

	gl_bind_texture(GL_TEXTURE_2D, gltex);
	gl_tex_param_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl_tex_param_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
	if (!gl_success("glEGLImageTargetTexture2DOES")) {
		gs_texture_destroy(texture);
		texture = NULL;
	}

	gl_bind_texture(GL_TEXTURE_2D, 0);

	return texture;
}

bool gl_egl_enum_adapters(EGLDisplay display, bool (*callback)(void *param, const char *name, uint32_t id), void *param)
{
	EGLDeviceEXT display_dev;
	if (eglQueryDisplayAttribEXT(display, EGL_DEVICE_EXT, (EGLAttrib *)&display_dev) &&
	    eglGetError() == EGL_SUCCESS) {
		const char *display_node = eglQueryDeviceStringEXT(display_dev, EGL_DRM_RENDER_NODE_FILE_EXT);
		if (eglGetError() != EGL_SUCCESS || display_node == NULL) {
			display_node = "/Software";
		}
		if (!callback(param, display_node, 0)) {
			return true;
		}
	}

	EGLint num_devices = 0;
	EGLDeviceEXT devices[32];
	if (!eglQueryDevicesEXT(32, devices, &num_devices)) {
		eglGetError();
		return true;
	}

	for (int i = 0; i < num_devices; i++) {
		const char *node = eglQueryDeviceStringEXT(devices[i], EGL_DRM_RENDER_NODE_FILE_EXT);
		if (node == NULL || eglGetError() != EGL_SUCCESS) {
			// Do not enumerate additional software renderers.
			continue;
		}
		if (!callback(param, node, i + 1)) {
			return true;
		}
	}
	return true;
}

uint32_t gs_get_adapter_count()
{
	EGLint num_devices = 0;
	eglQueryDevicesEXT(0, NULL, &num_devices);
	return 1 + num_devices; // Display + devices.
}

struct gs_texture *gl_egl_create_dmabuf_image(EGLDisplay egl_display, unsigned int width, unsigned int height,
					      uint32_t drm_format, enum gs_color_format color_format, uint32_t n_planes,
					      const int *fds, const uint32_t *strides, const uint32_t *offsets,
					      const uint64_t *modifiers)
{
	struct gs_texture *texture = NULL;
	EGLImage egl_image;

	if (!init_egl_image_target_texture_2d_ext())
		return NULL;

	egl_image = create_dmabuf_egl_image(egl_display, width, height, drm_format, n_planes, fds, strides, offsets,
					    modifiers);
	if (egl_image == EGL_NO_IMAGE) {
		blog(LOG_ERROR, "Cannot create EGLImage: %s", gl_egl_error_to_string(eglGetError()));
		return NULL;
	}

	texture =
		gl_egl_create_texture_from_eglimage(egl_display, width, height, color_format, GL_TEXTURE_2D, egl_image);
	if (texture)
		eglDestroyImage(egl_display, egl_image);

	return texture;
}

struct gs_texture *gl_egl_create_texture_from_pixmap(EGLDisplay egl_display, uint32_t width, uint32_t height,
						     enum gs_color_format color_format, EGLint target,
						     EGLClientBuffer pixmap)
{
	if (!init_egl_image_target_texture_2d_ext())
		return NULL;

	const EGLAttrib pixmap_attrs[] = {
		EGL_IMAGE_PRESERVED_KHR,
		EGL_TRUE,
		EGL_NONE,
	};

	EGLImage image = eglCreateImage(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, pixmap, pixmap_attrs);
	if (image == EGL_NO_IMAGE) {
		blog(LOG_ERROR, "Cannot create EGLImage: %s", gl_egl_error_to_string(eglGetError()));
		return NULL;
	}

	struct gs_texture *texture =
		gl_egl_create_texture_from_eglimage(egl_display, width, height, color_format, target, image);
	eglDestroyImage(egl_display, image);

	return texture;
}

static inline bool is_implicit_dmabuf_modifiers_supported(void)
{
	return EGL_EXT_image_dma_buf_import > 0;
}

static inline bool query_dmabuf_formats(EGLDisplay egl_display, EGLint **formats, EGLint *num_formats)
{
	EGLint max_formats = 0;

	if (!glad_eglQueryDmaBufFormatsEXT(egl_display, 0, NULL, &max_formats)) {
		blog(LOG_ERROR, "Cannot query the number of formats: %s", gl_egl_error_to_string(eglGetError()));
		return false;
	}

	if (max_formats != 0) {
		EGLint *format_list = bzalloc(max_formats * sizeof(EGLint));
		if (!format_list) {
			blog(LOG_ERROR, "Unable to allocate memory");
			return false;
		}

		if (!glad_eglQueryDmaBufFormatsEXT(egl_display, max_formats, format_list, &max_formats)) {
			blog(LOG_ERROR, "Cannot query a list of formats: %s", gl_egl_error_to_string(eglGetError()));
			bfree(format_list);
			return false;
		}

		*formats = format_list;
	}

	*num_formats = max_formats;
	return true;
}

bool gl_egl_query_dmabuf_capabilities(EGLDisplay egl_display, enum gs_dmabuf_flags *dmabuf_flags, uint32_t **formats,
				      size_t *n_formats)
{
	bool ret = false;

	if (is_implicit_dmabuf_modifiers_supported()) {
		*dmabuf_flags = GS_DMABUF_FLAG_IMPLICIT_MODIFIERS_SUPPORTED;
		ret = true;
	}

	if (!glad_eglQueryDmaBufFormatsEXT) {
		blog(LOG_ERROR, "Unable to load eglQueryDmaBufFormatsEXT");
		return ret;
	}

	if (!query_dmabuf_formats(egl_display, (EGLint **)formats, (EGLint *)n_formats)) {
		*n_formats = 0;
		*formats = NULL;
	}
	return ret;
}

static inline bool query_dmabuf_modifiers(EGLDisplay egl_display, EGLint drm_format, EGLuint64KHR **modifiers,
					  EGLuint64KHR *n_modifiers)
{
	EGLint max_modifiers;

	if (!glad_eglQueryDmaBufModifiersEXT(egl_display, drm_format, 0, NULL, NULL, &max_modifiers)) {
		blog(LOG_ERROR, "Cannot query the number of modifiers: %s", gl_egl_error_to_string(eglGetError()));
		return false;
	}

	if (max_modifiers != 0) {
		EGLuint64KHR *modifier_list = bzalloc(max_modifiers * sizeof(EGLuint64KHR));
		EGLBoolean *external_only = NULL;
		if (!modifier_list) {
			blog(LOG_ERROR, "Unable to allocate memory");
			return false;
		}

		if (!glad_eglQueryDmaBufModifiersEXT(egl_display, drm_format, max_modifiers, modifier_list,
						     external_only, &max_modifiers)) {
			blog(LOG_ERROR, "Cannot query a list of modifiers: %s", gl_egl_error_to_string(eglGetError()));
			bfree(modifier_list);
			return false;
		}

		*modifiers = modifier_list;
	}

	*n_modifiers = (EGLuint64KHR)max_modifiers;
	return true;
}

bool gl_egl_query_dmabuf_modifiers_for_format(EGLDisplay egl_display, uint32_t drm_format, uint64_t **modifiers,
					      size_t *n_modifiers)
{
	if (!glad_eglQueryDmaBufModifiersEXT) {
		blog(LOG_ERROR, "Unable to load eglQueryDmaBufModifiersEXT");
		return false;
	}
	if (!query_dmabuf_modifiers(egl_display, drm_format, modifiers, n_modifiers)) {
		*n_modifiers = 0;
		*modifiers = NULL;
		return false;
	}
	return true;
}

bool gl_egl_query_sync_capabilities(int drm_fd)
{
	uint64_t syncobjCap = 0;
	uint64_t syncobjTimelineCap = 0;

	drmGetCap(drm_fd, DRM_CAP_SYNCOBJ, &syncobjCap);
	drmGetCap(drm_fd, DRM_CAP_SYNCOBJ_TIMELINE, &syncobjTimelineCap);
	return syncobjCap && syncobjTimelineCap && (EGL_ANDROID_native_fence_sync > 0);
}

gs_sync_t *gl_egl_create_sync(EGLDisplay egl_display)
{
	gs_sync_t *sync = eglCreateSync(egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, NULL);
	glFlush();
	return sync;
}

gs_sync_t *gl_egl_create_sync_from_syncobj_timeline_point(EGLDisplay egl_display, int drm_fd, int syncobj_fd,
							  uint64_t timeline_point)
{
	EGLSync sync;
	int syncfile_fd = -1;
	uint32_t syncobj_handle = 0;
	uint32_t temp_handle = 0;

	drmSyncobjFDToHandle(drm_fd, syncobj_fd, &syncobj_handle);
	drmSyncobjTimelineWait(drm_fd, &syncobj_handle, &timeline_point, 1, INT64_MAX,
			       DRM_SYNCOBJ_WAIT_FLAGS_WAIT_AVAILABLE, NULL);
	drmSyncobjCreate(drm_fd, 0, &temp_handle);
	drmSyncobjTransfer(drm_fd, temp_handle, 0, syncobj_handle, timeline_point, 0);
	drmSyncobjExportSyncFile(drm_fd, temp_handle, &syncfile_fd);
	drmSyncobjDestroy(drm_fd, temp_handle);
	drmSyncobjDestroy(drm_fd, syncobj_handle);

	const EGLAttrib attributes[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, syncfile_fd, EGL_NONE};

	sync = eglCreateSync(egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attributes);
	if (sync == EGL_NO_SYNC) {
		blog(LOG_ERROR, "Unable to create an EGLSync object from a syncfile fd");
		close(syncfile_fd);
	}

	return sync;
}

bool gl_egl_sync_export_syncobj_timeline_point(EGLDisplay egl_display, gs_sync_t *sync, int drm_fd, int syncobj_fd,
					       uint64_t timeline_point)
{
	uint32_t syncobj_handle = 0;
	uint32_t temp_handle = 0;

	int gs_sync_fd = eglDupNativeFenceFDANDROID(egl_display, sync);
	if (gs_sync_fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
		return false;
	}

	drmSyncobjFDToHandle(drm_fd, syncobj_fd, &syncobj_handle);
	drmSyncobjCreate(drm_fd, 0, &temp_handle);
	drmSyncobjImportSyncFile(drm_fd, temp_handle, gs_sync_fd);
	drmSyncobjTransfer(drm_fd, syncobj_handle, timeline_point, temp_handle, 0, 0);
	drmSyncobjDestroy(drm_fd, temp_handle);
	drmSyncobjDestroy(drm_fd, syncobj_handle);
	close(gs_sync_fd);

	return true;
}

bool gl_egl_sync_signal_syncobj_timeline_point(int drm_fd, int syncobj_fd, uint64_t timeline_point)
{
	uint32_t syncobj_handle = 0;
	bool result;

	drmSyncobjFDToHandle(drm_fd, syncobj_fd, &syncobj_handle);
	result = drmSyncobjTimelineSignal(drm_fd, &syncobj_handle, &timeline_point, 1) == 0;
	drmSyncobjDestroy(drm_fd, syncobj_handle);

	return result;
}

void gl_egl_device_sync_destroy(EGLDisplay egl_display, gs_sync_t *sync)
{
	eglDestroySync(egl_display, sync);
}

bool gl_egl_sync_wait(EGLDisplay egl_display, gs_sync_t *sync)
{
	return eglWaitSync(egl_display, sync, 0);
}

const char *gl_egl_error_to_string(EGLint error_number)
{
	switch (error_number) {
	case EGL_SUCCESS:
		return "The last function succeeded without error.";
		break;
	case EGL_NOT_INITIALIZED:
		return "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
		break;
	case EGL_BAD_ACCESS:
		return "EGL cannot access a requested resource (for example a context is bound in another thread).";
		break;
	case EGL_BAD_ALLOC:
		return "EGL failed to allocate resources for the requested operation.";
		break;
	case EGL_BAD_ATTRIBUTE:
		return "An unrecognized attribute or attribute value was passed in the attribute list.";
		break;
	case EGL_BAD_CONTEXT:
		return "An EGLContext argument does not name a valid EGL rendering context.";
		break;
	case EGL_BAD_CONFIG:
		return "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
		break;
	case EGL_BAD_CURRENT_SURFACE:
		return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.";
		break;
	case EGL_BAD_DISPLAY:
		return "An EGLDisplay argument does not name a valid EGL display connection.";
		break;
	case EGL_BAD_SURFACE:
		return "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.";
		break;
	case EGL_BAD_MATCH:
		return "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).";
		break;
	case EGL_BAD_PARAMETER:
		return "One or more argument values are invalid.";
		break;
	case EGL_BAD_NATIVE_PIXMAP:
		return "A NativePixmapType argument does not refer to a valid native pixmap.";
		break;
	case EGL_BAD_NATIVE_WINDOW:
		return "A NativeWindowType argument does not refer to a valid native window.";
		break;
	case EGL_CONTEXT_LOST:
		return "A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering. ";
		break;
	default:
		return "Unknown error";
		break;
	}
}

int get_drm_render_node_fd(EGLDisplay egl_display)
{
	EGLAttrib attrib;
	EGLDeviceEXT device;
	const char *drm_render_node_path;

	if (eglQueryDisplayAttribEXT(egl_display, EGL_DEVICE_EXT, &attrib) != EGL_TRUE) {
		return -1;
	}
	device = (EGLDeviceEXT)attrib;
	drm_render_node_path = eglQueryDeviceStringEXT(device, EGL_DRM_RENDER_NODE_FILE_EXT);
	if (drm_render_node_path == NULL) {
		return -1;
	}

	return open(drm_render_node_path, O_RDWR | O_CLOEXEC);
}

void close_drm_render_node_fd(int fd)
{
	close(fd);
}
