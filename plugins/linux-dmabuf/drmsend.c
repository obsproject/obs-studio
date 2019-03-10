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

#define MSG(fmt, ...) fprintf(stderr, "obs-drmsend: " fmt "\n", ##__VA_ARGS__)

void printUsage(const char *name) {
	MSG("usage: %s /dev/dri/card socket_filename", name);
}

int main(int argc, const char *argv[]) {
	if (argc < 3) {
		printUsage(argv[0]);
		return 1;
	}

	const char *card = argv[2];
	const char *sockname = argv[3];

	MSG("Opening card %s", card);
	const int drmfd = open(card, O_RDONLY);
	if (drmfd < 0) {
		perror("Cannot open card");
		return 1;
	}

	int sockfd = -1;
	int retval = 2;
	drmsend_response_t response = {0};
	int fb_fds[OBS_DRMSEND_MAX_FRAMEBUFFERS] = {-1};

	{
		drmModePlaneResPtr planes = drmModeGetPlaneResources(drmfd);
		if (!planes) {
			MSG("Cannot get drm planes: %d", errno);
			goto cleanup;
		}

		for (uint32_t i = 0; i < planes->count_planes; ++i) {
			drmModePlanePtr plane = drmModeGetPlane(drmfd, planes->planes[i]);
			if (!plane) {
				MSG("Cannot get drmModePlanePtr for plane %#x", planes->planes[i]);
				continue;
			}

			if (!plane->fb_id)
				goto plane_continue;

			int j = 0;
			for (; j < response.num_framebuffers; ++j) {
				if (response.framebuffers[j].fb_id == plane->fb_id)
					break;
			}

			if (j < response.num_framebuffers)
				goto plane_continue;

			if (j == OBS_DRMSEND_MAX_FRAMEBUFFERS) {
				MSG("Too many framebuffers, max %d", OBS_DRMSEND_MAX_FRAMEBUFFERS);
				goto plane_continue;
			}

			drmModeFBPtr drmfb = drmModeGetFB(drmfd, plane->fb_id);
			if (!drmfb) {
				MSG("Cannot get drmModeFBPtr for fb %#x", plane->fb_id);
			} else {
				if (!drmfb->handle) {
					MSG("Cannot get FB handle for fb %#x", plane->fb_id);
					MSG("Possible reason: not permitted to get fb handles. Run either with sudo, or setcap cap_sys_admin+ep %s", argv[0]);
				} else {
					int fb_fd = -1;
					const int ret = drmPrimeHandleToFD(drmfd, drmfb->handle, 0, &fb_fd);
					if (!ret || fb_fd == -1) {
						MSG("Cannot get fd for fb %#x", plane->fb_id);
					} else {
						const int fb_index = response.num_framebuffers++;
						drmsend_framebuffer_t *fb = response.framebuffers + fb_index;
						fb_fds[fb_index] = fb_fd;
						fb->fb_id = plane->fb_id;
						fb->width = drmfb->width;
						fb->height = drmfb->height;
						fb->pitch = drmfb->pitch;
						fb->offset = 0;
						fb->fourcc = DRM_FORMAT_XRGB8888; // FIXME
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
		if (-1 == bind(sockfd, (const struct sockaddr*)&addr, sizeof(addr))) {
			perror("Cannot bind unix socket");
			goto cleanup;
		}

		if (-1 == listen(sockfd, 1)) {
			perror("Cannot listen on unix socket");
			goto cleanup;
		}
	}

	int connfd = accept(sockfd, NULL, NULL);
	if (connfd < 0) {
		perror("Cannot accept unix socket");
		goto cleanup;
	}

	MSG("accepted socket %d", connfd);

	struct msghdr msg = {0};

	struct iovec io = {
		.iov_base = &response,
		.iov_len = sizeof(response),
	};
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;

	char cmsg_buf[CMSG_SPACE(sizeof(fb_fds))];
	msg.msg_control = cmsg_buf;
	msg.msg_controllen = sizeof(cmsg_buf);
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int) * response.num_framebuffers);
	memcpy(CMSG_DATA(cmsg), fb_fds, sizeof(int) * response.num_framebuffers);

	ssize_t sent = sendmsg(connfd, &msg, MSG_CONFIRM);
	close(connfd);
	if (sent < 0) {
		perror("cannot sendmsg");
		goto cleanup;
	}

	MSG("sent %d", (int)sent);
	retval = 0;

cleanup:
	if (sockfd >= 0)
		close(sockfd);
	close(drmfd);
	return retval;
}
