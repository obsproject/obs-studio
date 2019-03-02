#include <obs-module.h>
#include <graphics/graphics-internal.h>

// FIXME ??
#include <../libobs-opengl/gl-subsystem.h>

#include <glad/glad_egl.h>

// FIXME integrate into glad
typedef void (APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, GLeglImageOES image);
GLAPI PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glad_glEGLImageTargetTexture2DOES;
typedef void (APIENTRYP PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)(GLenum target, GLeglImageOES image);
GLAPI PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glad_glEGLImageTargetRenderbufferStorageOES;

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
	int width, height;
	uint32_t fourcc;
	int offset, pitch;
	int fd;
} dmabuf_metadata_t;

typedef struct {
	obs_source_t *source;
	gs_texture_t *texture;
	EGLDisplay edisp;
	EGLImage eimage;
	dmabuf_metadata_t data;
} dmabuf_source_t;

// FIXME sync w/ gl-x11-egl.c
struct gl_platform {
	Display *xdisplay;
	EGLDisplay edisplay;
	EGLConfig config;
	EGLContext context;
	EGLSurface pbuffer;
};

static int dmabuf_receive_socket(const char *sockname, dmabuf_metadata_t *img)
{
	int retval = 0;
	const int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	{
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		if (strlen(sockname) >= sizeof(addr.sun_path)) {
			blog(LOG_ERROR, "Socket filename '%s' is too long, max %d",
				sockname, (int)sizeof(addr.sun_path));
			goto cleanup;
		}

		strcpy(addr.sun_path, sockname);
		if (-1 == connect(sockfd, (const struct sockaddr*)&addr, sizeof(addr))) {
			blog(LOG_ERROR, "Cannot connect to unix socket: %d", errno);
			goto cleanup;
		}

		blog(LOG_DEBUG, "connected, sockfd=%d", sockfd);
	}

	{
		struct msghdr msg = {0};

		struct iovec io = {
			.iov_base = img,
			.iov_len = sizeof(*img),
		};
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

		char cmsg_buf[CMSG_SPACE(sizeof(img->fd))];
		msg.msg_control = cmsg_buf;
		msg.msg_controllen = sizeof(cmsg_buf);
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(img->fd));

		ssize_t recvd = recvmsg(sockfd, &msg, 0);
		blog(LOG_DEBUG, "recvmsg = %d", (int)recvd);
		if (recvd <= 0) {
			blog(LOG_ERROR, "cannot recvmsg: %d", errno);
			goto cleanup;
		}

		if (io.iov_len == sizeof(*img) - sizeof(img->fd)) {
			blog(LOG_ERROR, "Received metadata size mismatch: %d received, %d expected",
				(int)io.iov_len, (int)sizeof(*img) - (int)sizeof(img->fd));
			goto cleanup;
		}

		if (cmsg->cmsg_len != CMSG_LEN(sizeof(img->fd))) {
			blog(LOG_ERROR, "Received fd size mismatch: %d received, %d expected",
				(int)cmsg->cmsg_len, (int)CMSG_LEN(sizeof(img->fd)));
			goto cleanup;
		}

		memcpy(&img->fd, CMSG_DATA(cmsg), sizeof(img->fd));
	}

	blog(LOG_INFO, "Received width=%d height=%d pitch=%u fourcc=%#x fd=%d",
		img->width, img->height, img->pitch, img->fourcc, img->fd);
	retval = 1;

cleanup:
	if (sockfd >= 0)
		close(sockfd);

	return retval;
}

static void *dmabuf_source_create(obs_data_t *settings, obs_source_t *source)
{
	(void)settings;

	struct obs_video_info ovi;
	if (!obs_get_video_info(&ovi)) {
		blog(LOG_ERROR, "Cannot get video info");
		return NULL;
	}

	// FIXME libobs-opengl-egl
	if (strcmp(ovi.graphics_module, "libobs-opengl.so.0.0") != 0) {
		blog(LOG_ERROR, "dmabuf_source requires EGL graphics: %s", ovi.graphics_module);
		return NULL;
	}

	const PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
		(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if (!glEGLImageTargetTexture2DOES) {
		blog(LOG_ERROR, "GL_OES_EGL_image extension is required");
		return NULL;
	}

	dmabuf_metadata_t metadata = {0};
	if (!dmabuf_receive_socket("/home/steaream/tmp/drmsend.sock", &metadata)) {
		blog(LOG_ERROR, "Cannot create dmabuf input");
		return NULL;
	}

	dmabuf_source_t *ctx = bzalloc(sizeof(dmabuf_source_t));
	ctx->source = source;

	obs_enter_graphics();

	const graphics_t *const graphics = gs_get_context();
	const EGLDisplay edisp = graphics->device->plat->edisplay;

	// FIXME check for EGL_EXT_image_dma_buf_import
	EGLAttrib eimg_attrs[] = {
		EGL_WIDTH, metadata.width,
		EGL_HEIGHT, metadata.height,
		EGL_LINUX_DRM_FOURCC_EXT, metadata.fourcc,
		EGL_DMA_BUF_PLANE0_FD_EXT, metadata.fd,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, metadata.offset,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, metadata.pitch,
		EGL_NONE
	};

	ctx->eimage = eglCreateImage(edisp, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0,
		eimg_attrs);

	if (!ctx->eimage) {
		// FIXME stringify error
		blog(LOG_ERROR, "Cannot create EGLImage: %d", eglGetError());
		goto error;
	}

	// FIXME handle fourcc
	ctx->texture = gs_texture_create(metadata.width, metadata.height,
		GS_BGRA, 1, NULL, GS_DYNAMIC);
	const GLuint gltex = *(GLuint*)gs_texture_get_obj(ctx->texture);
	blog(LOG_INFO, "gltex = %x", gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, ctx->eimage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	memcpy(&ctx->data, &metadata, sizeof(metadata));
	ctx->edisp = edisp;

	obs_leave_graphics();
	return ctx;

error:
	obs_leave_graphics();
	if (ctx->eimage)
		eglDestroyImage(edisp, ctx->eimage);
	if (metadata.fd >= 0)
		close(metadata.fd);
	bfree(ctx);
	return NULL;
}

static void dmabuf_source_destroy(void *data)
{
	const dmabuf_source_t *ctx = data;

	gs_texture_destroy(ctx->texture);

	if (ctx->eimage)
		eglDestroyImage(ctx->edisp, ctx->eimage);
	if (ctx->data.fd >= 0)
		close(ctx->data.fd);
	bfree(data);
}

static void dmabuf_source_render(void *data, gs_effect_t *effect)
{
	const dmabuf_source_t *ctx = data;

	if (!ctx->texture)
		return;

	effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, ctx->texture);

	while (gs_effect_loop(effect, "Draw")) {
		gs_draw_sprite(ctx->texture, 0, 0, 0);
	}
}

static const char *dmabuf_source_get_name(void *data)
{
	(void)data;
	return "DMA-BUF source";
}

static uint32_t dmabuf_source_get_width(void *data)
{
	const dmabuf_source_t *ctx = data;
	return ctx->data.width;
}

static uint32_t dmabuf_source_get_height(void *data)
{
	const dmabuf_source_t *ctx = data;
	return ctx->data.height;
}

struct obs_source_info dmabuf_source = {
	.id = "dmabuf-source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = dmabuf_source_get_name,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.create = dmabuf_source_create,
	.destroy = dmabuf_source_destroy,
	.video_render = dmabuf_source_render,
	.get_width = dmabuf_source_get_width,
	.get_height = dmabuf_source_get_height,
};

MODULE_EXPORT const char *obs_module_description(void)
{
	return "DMA-BUF-based zero-copy video capture";
}

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
	obs_register_source(&dmabuf_source);
	return true;
}

void obs_module_unload(void)
{
}
