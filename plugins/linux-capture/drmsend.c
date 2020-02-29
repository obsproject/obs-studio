#include "drmsend.h"

#include <xf86drm.h>
#include <libdrm/drm_fourcc.h>
#include <xf86drmMode.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LOG_PREFIX "obs-drmsend: "

#define ERR(fmt, ...) fprintf(stderr, LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define MSG(fmt, ...) fprintf(stdout, LOG_PREFIX fmt "\n", ##__VA_ARGS__)

void printUsage(const char *name)
{
	MSG("usage: %s /dev/dri/card socket_filename", name);
}

int main(int argc, const char *argv[])
{
	if (argc < 3) {
		printUsage(argv[0]);
		return 1;
	}

	const char *card = argv[1];
	const char *sockname = argv[2];

	MSG("Opening card %s", card);
	const int drmfd = open(card, O_RDONLY);
	if (drmfd < 0) {
		perror("Cannot open card");
		return 1;
	}

	if (0 != drmSetClientCap(drmfd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) {
		perror("Cannot tell drm to expose all planes; the rest will very likely fail");
	}

	int sockfd = -1;
	int retval = 2;
	drmsend_response_t response = {0};
	int fb_fds[OBS_DRMSEND_MAX_FRAMEBUFFERS] = {-1};

	{
		drmModePlaneResPtr planes = drmModeGetPlaneResources(drmfd);
		if (!planes) {
			ERR("Cannot get drm planes: %s (%d)", strerror(errno),
			    errno);
			goto cleanup;
		}

		MSG("DRM planes %d:", planes->count_planes);
		for (uint32_t i = 0; i < planes->count_planes; ++i) {
			drmModePlanePtr plane =
				drmModeGetPlane(drmfd, planes->planes[i]);
			if (!plane) {
				ERR("Cannot get drmModePlanePtr for plane %#x: %s (%d)",
				    planes->planes[i], strerror(errno), errno);
				continue;
			}

			MSG("\t%d: fb_id=%#x", i, plane->fb_id);

			if (!plane->fb_id)
				goto plane_continue;

			int j = 0;
			for (; j < response.num_framebuffers; ++j) {
				if (response.framebuffers[j].fb_id ==
				    plane->fb_id)
					break;
			}

			if (j < response.num_framebuffers)
				goto plane_continue;

			if (j == OBS_DRMSEND_MAX_FRAMEBUFFERS) {
				ERR("Too many framebuffers, max %d",
				    OBS_DRMSEND_MAX_FRAMEBUFFERS);
				goto plane_continue;
			}

			drmModeFBPtr drmfb = drmModeGetFB(drmfd, plane->fb_id);
			if (!drmfb) {
				ERR("Cannot get drmModeFBPtr for fb %#x: %s (%d)",
				    plane->fb_id, strerror(errno), errno);
			} else {
				if (!drmfb->handle) {
					ERR("\t\tFB handle for fb %#x is NULL",
					    plane->fb_id);
					ERR("\t\tPossible reason: not permitted to get FB handles. Do `sudo setcap cap_sys_admin+ep %s`",
					    argv[0]);
				} else {
					int fb_fd = -1;
					const int ret = drmPrimeHandleToFD(
						drmfd, drmfb->handle, 0,
						&fb_fd);
					if (ret != 0 || fb_fd == -1) {
						ERR("Cannot get fd for fb %#x handle %#x: %s (%d)",
						    plane->fb_id, drmfb->handle,
						    strerror(errno), errno);
					} else {
						const int fb_index =
							response.num_framebuffers++;
						drmsend_framebuffer_t *fb =
							response.framebuffers +
							fb_index;
						fb_fds[fb_index] = fb_fd;
						fb->fb_id = plane->fb_id;
						fb->width = drmfb->width;
						fb->height = drmfb->height;
						fb->pitch = drmfb->pitch;
						fb->offset = 0;
						fb->fourcc =
							DRM_FORMAT_XRGB8888; // FIXME
					}
				}
				drmModeFreeFB(drmfb);
			}

		plane_continue:
			drmModeFreePlane(plane);
		}

		drmModeFreePlaneResources(planes);
	}

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	{
		struct sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		if (strlen(sockname) >= sizeof(addr.sun_path)) {
			MSG("Socket filename '%s' is too long, max %d",
			    sockname, (int)sizeof(addr.sun_path));
			goto cleanup;
		}

		strcpy(addr.sun_path, sockname);
		if (-1 == connect(sockfd, (const struct sockaddr *)&addr,
				  sizeof(addr))) {
			MSG("Cannot connect to unix socket: %d", errno);
			goto cleanup;
		}
	}

	response.tag = OBS_DRMSEND_TAG;

	struct msghdr msg = {0};

	struct iovec io = {
		.iov_base = &response,
		.iov_len = sizeof(response),
	};
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;

	const int fb_size = sizeof(int) * response.num_framebuffers;
	char cmsg_buf[CMSG_SPACE(sizeof(fb_fds))];
	msg.msg_control = cmsg_buf;
	msg.msg_controllen = CMSG_SPACE(fb_size);
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(fb_size);
	memcpy(CMSG_DATA(cmsg), fb_fds, fb_size);

	const ssize_t sent = sendmsg(sockfd, &msg, 0);

	if (sent < 0) {
		perror("cannot sendmsg");
		goto cleanup;
	}

	MSG("sent %d bytes", (int)sent);
	retval = 0;

cleanup:
	if (sockfd >= 0)
		close(sockfd);
	close(drmfd);
	return retval;
}
