#include "drmsend.h"

#include <graphics/graphics-internal.h>

// FIXME needed for gl_platform pointer access
#include <../libobs-opengl/gl-subsystem.h>

#include <obs-module.h>
#include <glad/glad_egl.h>

#include <sys/wait.h>
#include <stdio.h>

// FIXME integrate into glad
typedef void (APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, GLeglImageOES image);
GLAPI PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glad_glEGLImageTargetTexture2DOES;
typedef void (APIENTRYP PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC)(GLenum target, GLeglImageOES image);
GLAPI PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glad_glEGLImageTargetRenderbufferStorageOES;

// FIXME glad
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

// FIXME sync w/ gl-x11-egl.c
struct gl_platform {
	Display *xdisplay;
	EGLDisplay edisplay;
	EGLConfig config;
	EGLContext context;
	EGLSurface pbuffer;
};

typedef struct {
	drmsend_response_t resp;
	int fb_fds[OBS_DRMSEND_MAX_FRAMEBUFFERS];
} dmabuf_source_fblist_t;

typedef struct {
	obs_source_t *source;
	gs_texture_t *texture;
	EGLDisplay edisp;
	EGLImage eimage;

	dmabuf_source_fblist_t fbs;
	int active_fb;
} dmabuf_source_t;

static int dmabuf_source_receive_framebuffers(dmabuf_source_fblist_t *list)
{
	const char *dri_filename = "/dev/dri/card0"; // FIXME
	const char *drmsend_filename = "./obs-drmsend"; // FIXME extract from /proc/self/exe + -drmsend
	const char *sockname = "/tmp/drmsend.sock"; // FIXME mktemp/tempnamp/...

	int retval = 0;
	int sockfd = -1;

	/* 1. create and listen on unix socket */
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	{
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		if (strlen(sockname) >= sizeof(addr.sun_path)) {
			blog(LOG_ERROR, "Socket filename '%s' is too long, max %d",
				sockname, (int)sizeof(addr.sun_path));
			goto socket_cleanup;
		}
		strcpy(addr.sun_path, sockname);

		unlink(sockname);

		if (-1 == bind(sockfd, (const struct sockaddr*)&addr, sizeof(addr))) {
			blog(LOG_ERROR, "Cannot bind unix socket: %d", errno);
			goto socket_cleanup;
		}

		if (-1 == listen(sockfd, 1)) {
			blog(LOG_ERROR, "Cannot listen on unix socket: %d", errno);
			goto socket_cleanup;
		}
	}

	/* 2. run obs-drmsend utility */
	const pid_t drmsend_pid = fork();
	if (drmsend_pid == -1) {
		blog(LOG_ERROR, "Cannot fork(): %d", errno);
		goto socket_cleanup;
	} else if (drmsend_pid == 0) {
		execlp(drmsend_filename, drmsend_filename, dri_filename, sockname, NULL);
		fprintf(stderr, "Cannot execlp(%s): %d\n", drmsend_filename, errno);
		exit(-1);
	}

	/* 3. select() on unix socket w/ timeout */
	// FIXME updating timeout w/ time left is linux-specific, other unices might not do that
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	for (;;) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		const int maxfd = sockfd;
		const int nfds = select(maxfd + 1, &set, NULL, NULL, &timeout);
		if (nfds > 0) {
			if (FD_ISSET(sockfd, &set))
				break;
		}

		if (nfds < 0) {
			if (errno == EINTR)
				continue;
			blog(LOG_ERROR, "Cannot select(): %d", errno);
			goto child_cleanup;
		}

		if (nfds == 0) {
			blog(LOG_ERROR, "Waiting for drmsend timed out");
			goto child_cleanup;
		}
	}

	/* 4. accept() and receive data */
	int connfd = accept(sockfd, NULL, NULL);
	if (connfd < 0) {
		blog(LOG_ERROR, "Cannot accept unix socket: %d", errno);
		goto child_cleanup;
	}

	for (;;) {
		struct msghdr msg = {0};

		struct iovec io = {
			.iov_base = &list->resp,
			.iov_len = sizeof(list->resp),
		};
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

		char cmsg_buf[CMSG_SPACE(sizeof(int) * OBS_DRMSEND_MAX_FRAMEBUFFERS)];
		msg.msg_control = cmsg_buf;
		msg.msg_controllen = sizeof(cmsg_buf);
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * OBS_DRMSEND_MAX_FRAMEBUFFERS);

		// FIXME blocking, may hang if drmsend dies before sending anything
		const ssize_t recvd = recvmsg(connfd, &msg, 0);
		blog(LOG_DEBUG, "recvmsg = %d", (int)recvd);
		if (recvd <= 0) {
			blog(LOG_ERROR, "cannot recvmsg: %d", errno);
			break;
		}

		if (io.iov_len != sizeof(list->resp)) {
			blog(LOG_ERROR, "Received metadata size mismatch: %d received, %d expected",
				(int)io.iov_len, (int)sizeof(list->resp));
			break;
		}

		if (list->resp.tag != OBS_DRMSEND_TAG) {
			blog(LOG_ERROR, "Received metadata tag mismatch: %#x received, %#x expected",
					list->resp.tag, OBS_DRMSEND_TAG);
			break;
		}

		if (cmsg->cmsg_len != CMSG_LEN(sizeof(int) * list->resp.num_framebuffers)) {
			blog(LOG_ERROR, "Received fd size mismatch: %d received, %d expected",
				(int)cmsg->cmsg_len, (int)CMSG_LEN(sizeof(int) * list->resp.num_framebuffers));
			break;
		}

		memcpy(list->fb_fds, CMSG_DATA(cmsg), sizeof(int) * list->resp.num_framebuffers);
		retval = 1;
		break;
	}
	close(connfd);

	if (retval) {
		blog(LOG_INFO, "Received %d framebuffers:", list->resp.num_framebuffers);
		for (int i = 0; i < list->resp.num_framebuffers; ++i) {
			const drmsend_framebuffer_t *fb = list->resp.framebuffers + i;
			blog(LOG_INFO, "Received width=%d height=%d pitch=%u fourcc=%#x fd=%d",
				fb->width, fb->height, fb->pitch, fb->fourcc, list->fb_fds[i]);
		}
	}

// TODO consider using separate thread for waitpid() on drmsend_pid
	/* 5. waitpid() on obs-drmsend w/ timeout (poll) */
	int exited = 0;
child_cleanup:
	for (int i = 0; i < 10; ++i) {
		usleep(500*1000);
		int wstatus = 0;
		const pid_t p = waitpid(drmsend_pid, &wstatus, WNOHANG);
		if (p == drmsend_pid) {
			if (wstatus == 0 || WIFEXITED(wstatus)) {
				exited = 1;
				const int status = WEXITSTATUS(wstatus);
				if (status != 0)
					blog(LOG_ERROR, "%s returned %d", drmsend_filename, status);
				break;
			}
		} else if (-1 == p) {
			const int err = errno;
			blog(LOG_ERROR, "Cannot waitpid() on drmsend: %d", err);
			if (err == ECHILD) {
				exited = 1;
				break;
			}
		}
	}

	if (!exited)
		blog(LOG_ERROR, "Couldn't wait for %s to exit, expect zombies", drmsend_filename);

socket_cleanup:
	close(sockfd);
	return retval;
}

void dmabuf_source_close(dmabuf_source_t *ctx)
{
	if (ctx->eimage != EGL_NO_IMAGE) {
		eglDestroyImage(ctx->edisp, ctx->eimage);
		ctx->eimage = EGL_NO_IMAGE;
	}

	ctx->active_fb = -1;
}

static void dmabuf_source_open(dmabuf_source_t *ctx, uint32_t fb_id)
{
	assert(ctx->active_fb == -1);

	int index;
	for (index = 0; index < ctx->fbs.resp.num_framebuffers; ++index)
		if (fb_id == ctx->fbs.resp.framebuffers[index].fb_id)
			break;

	if (index == ctx->fbs.resp.num_framebuffers) {
		blog(LOG_ERROR, "Framebuffer id=%#x not found", fb_id);
		return;
	}

	blog(LOG_DEBUG, "Using framebuffer id=%#x (index=%d)", fb_id, index);

	const drmsend_framebuffer_t *fb = ctx->fbs.resp.framebuffers + index;

	blog(LOG_DEBUG, "%dx%d %d %d %d", fb->width, fb->height,
		ctx->fbs.fb_fds[index], fb->offset, fb->pitch);

	obs_enter_graphics();

	const graphics_t *const graphics = gs_get_context();
	const EGLDisplay edisp = graphics->device->plat->edisplay;
	ctx->edisp = edisp;

	// FIXME check for EGL_EXT_image_dma_buf_import
	EGLAttrib eimg_attrs[] = {
		EGL_WIDTH, fb->width,
		EGL_HEIGHT, fb->height,
		EGL_LINUX_DRM_FOURCC_EXT, fb->fourcc,
		EGL_DMA_BUF_PLANE0_FD_EXT, ctx->fbs.fb_fds[index],
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, fb->offset,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, fb->pitch,
		EGL_NONE
	};

	ctx->eimage = eglCreateImage(edisp, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, 0,
		eimg_attrs);

	if (!ctx->eimage) {
		// FIXME stringify error
		blog(LOG_ERROR, "Cannot create EGLImage: %d", eglGetError());
		dmabuf_source_close(ctx);
		goto exit;
	}

	// FIXME handle fourcc?
	ctx->texture = gs_texture_create(fb->width, fb->height,
		GS_BGRA, 1, NULL, GS_DYNAMIC);
	const GLuint gltex = *(GLuint*)gs_texture_get_obj(ctx->texture);
	blog(LOG_DEBUG, "gltex = %x", gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);

	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, ctx->eimage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	ctx->active_fb = index;

exit:
	obs_leave_graphics();
	return;
}

static void dmabuf_source_update(void *data, obs_data_t *settings)
{
	dmabuf_source_t *ctx = data;

	dmabuf_source_close(ctx);
	dmabuf_source_open(ctx, obs_data_get_int(settings, "framebuffer"));
}

static void *dmabuf_source_create(obs_data_t *settings, obs_source_t *source)
{
	(void)settings;

	{
		// FIXME is it desirable to enter graphics at create time?
		// Or is it better to postpone all activity to after the module
		// was succesfully and unconditionally created?
		obs_enter_graphics();
		const int device_type = gs_get_device_type();
		obs_leave_graphics();
		if (device_type != GS_DEVICE_OPENGL_EGL) {
			blog(LOG_ERROR, "dmabuf_source requires EGL");
			return NULL;
		}
	}

	// FIXME move to glad
	if (!glEGLImageTargetTexture2DOES) {
		glEGLImageTargetTexture2DOES =
			(PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
				"glEGLImageTargetTexture2DOES");
	}

	if (!glEGLImageTargetTexture2DOES) {
		blog(LOG_ERROR, "GL_OES_EGL_image extension is required");
		return NULL;
	}

	dmabuf_source_t *ctx = bzalloc(sizeof(dmabuf_source_t));
	ctx->source = source;
	ctx->active_fb = -1;

#define COUNTOF(a) (sizeof(a) / sizeof(*a))
	for (int i = 0; i < (int)COUNTOF(ctx->fbs.fb_fds); ++i) {
		ctx->fbs.fb_fds[i] = -1;
	}

	if (!dmabuf_source_receive_framebuffers(&ctx->fbs)) {
		blog(LOG_ERROR, "Unable to enumerate DRM/KMS framebuffers");
		return NULL;
	}

	dmabuf_source_update(ctx, settings);
	return ctx;
}

static void dmabuf_source_close_fds(dmabuf_source_t *ctx)
{
	for (int i = 0; i < ctx->fbs.resp.num_framebuffers; ++i) {
		const int fd = ctx->fbs.fb_fds[i];
		if (fd > 0)
			close(fd);
	}
}

static void dmabuf_source_destroy(void *data)
{
	dmabuf_source_t *ctx = data;
	gs_texture_destroy(ctx->texture);
	dmabuf_source_close(ctx);
	dmabuf_source_close_fds(ctx);
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

static obs_properties_t *dmabuf_source_get_properties(void *data)
{
	dmabuf_source_t *ctx = data;

	dmabuf_source_fblist_t stack_list = {0};

	if (!dmabuf_source_receive_framebuffers(&stack_list)) {
		blog(LOG_ERROR, "Unable to enumerate DRM/KMS framebuffers");
		return NULL;
	}

	obs_properties_t *props = obs_properties_create();

	obs_property_t *fb_list = obs_properties_add_list(props, "framebuffer", "Framebuffer to capture",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	for (int i = 0; i < stack_list.resp.num_framebuffers; ++i) {
		const drmsend_framebuffer_t *fb = stack_list.resp.framebuffers + i;
		char buf[128];
		sprintf(buf, "%dx%d (%#x)", fb->width, fb->height, fb->fb_id);
		obs_property_list_add_int(fb_list, buf, fb->fb_id);
	}

	dmabuf_source_close_fds(ctx);
	memcpy(&ctx->fbs, &stack_list, sizeof(stack_list));

	return props;
}

static const char *dmabuf_source_get_name(void *data)
{
	(void)data;
	return "DMA-BUF source";
}

static uint32_t dmabuf_source_get_width(void *data)
{
	const dmabuf_source_t *ctx = data;
	if (ctx->active_fb < 0)
		return 0;
	return ctx->fbs.resp.framebuffers[ctx->active_fb].width;
}

static uint32_t dmabuf_source_get_height(void *data)
{
	const dmabuf_source_t *ctx = data;
	if (ctx->active_fb < 0)
		return 0;
	return ctx->fbs.resp.framebuffers[ctx->active_fb].height;
}

struct obs_source_info dmabuf_source = {
	.id = "dmabuf-source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = dmabuf_source_get_name,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE,
	.create = dmabuf_source_create,
	.destroy = dmabuf_source_destroy,
	.video_render = dmabuf_source_render,
	.get_width = dmabuf_source_get_width,
	.get_height = dmabuf_source_get_height,
	.get_properties = dmabuf_source_get_properties,
	.update = dmabuf_source_update,
};

MODULE_EXPORT const char *obs_module_description(void)
{
	return "DMA-BUF-based zero-copy screen capture";
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
