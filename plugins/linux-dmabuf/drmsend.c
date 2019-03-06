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

#define MSG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)

typedef struct {
	int width, height;
	uint32_t fourcc;
	int offset, pitch;
} DmaBufMetadata;

void printUsage(const char *name) {
	MSG("usage: %s fb_id socket_filename </dev/dri/card>", name);
}

int main(int argc, const char *argv[]) {

	uint32_t fb_id = 0;

	if (argc < 3) {
		printUsage(argv[0]);
		return 1;
	}

	{
		char *endptr;
		fb_id = strtol(argv[1], &endptr, 0);
		if (*endptr != '\0') {
			MSG("%s is not valid framebuffer id", argv[1]);
			printUsage(argv[0]);
			return 1;
		}
	}

	const char *sockname = argv[2];

	const char *card = (argc > 3) ? argv[3] : "/dev/dri/card0";

	MSG("Opening card %s", card);
	const int drmfd = open(card, O_RDONLY);
	if (drmfd < 0) {
		perror("Cannot open card");
		return 1;
	}

	int dma_buf_fd = -1;
	int sockfd = -1;
	drmModeFBPtr fb = drmModeGetFB(drmfd, fb_id);
	if (!fb) {
		MSG("Cannot open fb %#x", fb_id);
		goto cleanup;
	}

	MSG("fb_id=%#x width=%u height=%u pitch=%u bpp=%u depth=%u handle=%#x",
		fb_id, fb->width, fb->height, fb->pitch, fb->bpp, fb->depth, fb->handle);

	if (!fb->handle) {
		MSG("Not permitted to get fb handles. Run either with sudo, or setcap cap_sys_admin+ep %s", argv[0]);
		goto cleanup;
	}

	DmaBufMetadata img;
	img.width = fb->width;
	img.height = fb->height;
	img.pitch = fb->pitch;
	img.offset = 0;
	img.fourcc = DRM_FORMAT_XRGB8888; // FIXME

	const int ret = drmPrimeHandleToFD(drmfd, fb->handle, 0, &dma_buf_fd);
	MSG("drmPrimeHandleToFD = %d, fd = %d", ret, dma_buf_fd);

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

	for (;;) {
		int connfd = accept(sockfd, NULL, NULL);
		if (connfd < 0) {
			perror("Cannot accept unix socket");
			goto cleanup;
		}

		MSG("accepted socket %d", connfd);

		struct msghdr msg = {0};

		struct iovec io = {
			.iov_base = &img,
			.iov_len = sizeof(img),
		};
		msg.msg_iov = &io;
		msg.msg_iovlen = 1;

		char cmsg_buf[CMSG_SPACE(sizeof(dma_buf_fd))];
		msg.msg_control = cmsg_buf;
		msg.msg_controllen = sizeof(cmsg_buf);
		struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(dma_buf_fd));
		memcpy(CMSG_DATA(cmsg), &dma_buf_fd, sizeof(dma_buf_fd));

		ssize_t sent = sendmsg(connfd, &msg, MSG_CONFIRM);
		if (sent < 0) {
			perror("cannot sendmsg");
			goto cleanup;
		}

		MSG("sent %d", (int)sent);

		close(connfd);
	}

cleanup:
	if (sockfd >= 0)
		close(sockfd);
	if (dma_buf_fd >= 0)
		close(dma_buf_fd);
	if (fb)
		drmModeFreeFB(fb);
	close(drmfd);
	return 0;
}
