#include "drmsend.h"
#include "xcursor-xcb.h"

#include <graphics/graphics-internal.h>

// FIXME needed for gl_platform pointer access
#include <../libobs-opengl/gl-subsystem.h>

#include <obs-module.h>
#include <util/platform.h>
#include <glad/glad_egl.h>

#include <sys/wait.h>
#include <stdio.h>

// FIXME stringify errno

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
#include <limits.h>

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

	xcb_connection_t *xcb;
	xcb_xcursor_t *cursor;
	gs_texture_t *texture;
	EGLDisplay edisp;
	EGLImage eimage;

	dmabuf_source_fblist_t fbs;
	int active_fb;

	bool show_cursor;
} dmabuf_source_t;

static const char obs_drmsend_suffix[] = "-drmsend";
static const int obs_drmsend_suffix_len = sizeof(obs_drmsend_suffix) - 1;
static const char socket_filename[] = "/obs-drmsend.sock";
static const int socket_filename_len = sizeof(socket_filename) - 1;

static int dmabuf_source_receive_framebuffers(dmabuf_source_fblist_t *list)
{
	const char *dri_filename = "/dev/dri/card0"; // FIXME

	blog(LOG_DEBUG, "dmabuf_source_receive_framebuffers");

	int retval = 0;
	int sockfd = -1;


	/* Get socket filename */
	struct sockaddr_un addr = {0};
	addr.sun_family = AF_UNIX;
	{
		const char * const module_path = obs_get_module_data_path(obs_current_module());
		assert(module_path);
		if (!os_file_exists(module_path)) {
			if (MKDIR_ERROR == os_mkdir(module_path)) {
				blog(LOG_ERROR, "Unable to create directory %s", module_path);
				return 0;
			}
		}

		const int module_path_len = strlen(module_path);
		if (module_path_len + socket_filename_len + 1 >= (int)sizeof(addr.sun_path)) {
			blog(LOG_ERROR, "Socket filename is too long, max %d", (int)sizeof(addr.sun_path));
			return 0;
		}
		memcpy(addr.sun_path, module_path, module_path_len);
		memcpy(addr.sun_path + module_path_len, socket_filename, socket_filename_len);

		blog(LOG_DEBUG, "Will bind socket to %s", addr.sun_path);
	}

	/* Find obs-drmsend */
	char drmsend_filename[PATH_MAX + 1];
	{
		const ssize_t drmsend_filename_len = readlink("/proc/self/exe", drmsend_filename, sizeof(drmsend_filename));
		if (drmsend_filename_len < 0) {
				blog(LOG_ERROR, "Unable to retrieve full path to obs binary: %d", errno);
				return 0;
		}

		if (drmsend_filename_len + obs_drmsend_suffix_len + 1 > (int)sizeof(drmsend_filename)) {
				blog(LOG_ERROR, "Full path to obs-drmsend is too long");
				return 0;
		}

		memcpy(drmsend_filename + drmsend_filename_len, obs_drmsend_suffix, obs_drmsend_suffix_len + 1);

		if (!os_file_exists(drmsend_filename)) {
				blog(LOG_ERROR, "%s doesn't exist", drmsend_filename);
				return 0;
		}

		blog(LOG_DEBUG, "Will execute obs-drmsend from %s", drmsend_filename);
	}

	/* 1. create and listen on unix socket */
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	unlink(addr.sun_path);
	if (-1 == bind(sockfd, (const struct sockaddr*)&addr, sizeof(addr))) {
		blog(LOG_ERROR, "Cannot bind unix socket to %s: %d", addr.sun_path, errno);
		goto socket_cleanup;
	}

	if (-1 == listen(sockfd, 1)) {
		blog(LOG_ERROR, "Cannot listen on unix socket bound to %s: %d", addr.sun_path, errno);
		goto socket_cleanup;
	}

	/* 2. run obs-drmsend utility */
	const pid_t drmsend_pid = fork();
	if (drmsend_pid == -1) {
		blog(LOG_ERROR, "Cannot fork(): %d", errno);
		goto socket_cleanup;
	} else if (drmsend_pid == 0) {
		execlp(drmsend_filename, drmsend_filename, dri_filename, addr.sun_path, NULL);
		fprintf(stderr, "Cannot execlp(%s): %d\n", drmsend_filename, errno);
		exit(-1);
	}

	blog(LOG_DEBUG, "Forked obs-drmsend to pid %d", drmsend_pid);

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

	blog(LOG_DEBUG, "Ready to accept");

	/* 4. accept() and receive data */
	int connfd = accept(sockfd, NULL, NULL);
	if (connfd < 0) {
		blog(LOG_ERROR, "Cannot accept unix socket: %d", errno);
		goto child_cleanup;
	}

	blog(LOG_DEBUG, "Receiving message from obs-drmsend");

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
	unlink(addr.sun_path);
	return retval;
}

static void dmabuf_source_close(dmabuf_source_t *ctx)
{
	blog(LOG_DEBUG, "dmabuf_source_close %p", ctx);

	if (ctx->eimage != EGL_NO_IMAGE) {
		eglDestroyImage(ctx->edisp, ctx->eimage);
		ctx->eimage = EGL_NO_IMAGE;
	}

	ctx->active_fb = -1;
}

static void dmabuf_source_open(dmabuf_source_t *ctx, uint32_t fb_id)
{
	blog(LOG_DEBUG, "dmabuf_source_open %p %#x", ctx, fb_id);
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
	blog(LOG_DEBUG, "dmabuf_source_udpate %p", ctx);

	ctx->show_cursor = obs_data_get_bool(settings, "show_cursor");

	dmabuf_source_close(ctx);
	dmabuf_source_open(ctx, obs_data_get_int(settings, "framebuffer"));
}

static void *dmabuf_source_create(obs_data_t *settings, obs_source_t *source)
{
	blog(LOG_DEBUG, "dmabuf_source_create");
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
		bfree(ctx);
		return NULL;
	}

	ctx->xcb = xcb_connect(NULL, NULL);
	if (!ctx->xcb || xcb_connection_has_error(ctx->xcb)) {
		blog(LOG_ERROR, "Unable to open X display, cursor will not be available");
	}

	ctx->cursor = xcb_xcursor_init(ctx->xcb);

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
	blog(LOG_DEBUG, "dmabuf_source_destroy %p", ctx);

	if (ctx->texture)
		gs_texture_destroy(ctx->texture);

	dmabuf_source_close(ctx);
	dmabuf_source_close_fds(ctx);

	if (ctx->cursor)
		xcb_xcursor_destroy(ctx->cursor);

	if (ctx->xcb)
		xcb_disconnect(ctx->xcb);

	bfree(data);
}

static void dmabuf_source_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	dmabuf_source_t *ctx = data;

	if (!ctx->texture)
		return;
	if (!obs_source_showing(ctx->source))
		return;
	if (!ctx->cursor)
		return;

	xcb_xfixes_get_cursor_image_cookie_t cur_c
		= xcb_xfixes_get_cursor_image_unchecked(ctx->xcb);
	xcb_xfixes_get_cursor_image_reply_t *cur_r
		= xcb_xfixes_get_cursor_image_reply(ctx->xcb, cur_c, NULL);

	obs_enter_graphics();
	xcb_xcursor_update(ctx->cursor, cur_r);
	obs_leave_graphics();

	free(cur_r);
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

	if (ctx->show_cursor && ctx->cursor) {
		while (gs_effect_loop(effect, "Draw")) {
			xcb_xcursor_render(ctx->cursor);
		}
	}
}

static void dmabuf_source_get_defaults(obs_data_t *defaults)
{
	obs_data_set_default_bool(defaults, "show_cursor", true);
}

static obs_properties_t *dmabuf_source_get_properties(void *data)
{
	dmabuf_source_t *ctx = data;
	blog(LOG_DEBUG, "dmabuf_source_get_properties %p", ctx);

	dmabuf_source_fblist_t stack_list = {0};

	if (!dmabuf_source_receive_framebuffers(&stack_list)) {
		blog(LOG_ERROR, "Unable to enumerate DRM/KMS framebuffers");
		return NULL;
	}

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, "show_cursor",
			obs_module_text("CaptureCursor"));

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
	blog(LOG_DEBUG, "dmabuf_source_get_name %p", data);
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

struct obs_source_info dmabuf_input = {
	.id = "dmabuf-source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.get_name = dmabuf_source_get_name,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_DO_NOT_DUPLICATE,
	.create = dmabuf_source_create,
	.destroy = dmabuf_source_destroy,
	.video_tick = dmabuf_source_video_tick,
	.video_render = dmabuf_source_render,
	.get_width = dmabuf_source_get_width,
	.get_height = dmabuf_source_get_height,
	.get_defaults = dmabuf_source_get_defaults,
	.get_properties = dmabuf_source_get_properties,
	.update = dmabuf_source_update,
};
