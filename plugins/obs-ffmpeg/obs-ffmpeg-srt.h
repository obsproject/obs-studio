/*
 * The following code is a port of FFmpeg/avformat/libsrt.c for obs-studio.
 * Port by pkv@obsproject.com.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once
#include <obs-module.h>
#include "obs-ffmpeg-url.h"
#include <srt/srt.h>
#include <libavformat/avformat.h>
#ifdef _WIN32
#include <sys/timeb.h>
#endif

#define POLLING_TIME 100 /// Time in milliseconds between interrupt check

/* This is for MPEG-TS (7 TS packets) */
#ifndef SRT_LIVE_DEFAULT_PAYLOAD_SIZE
#define SRT_LIVE_DEFAULT_PAYLOAD_SIZE 1316
#endif

enum SRTMode { SRT_MODE_CALLER = 0, SRT_MODE_LISTENER = 1, SRT_MODE_RENDEZVOUS = 2 };

struct srt_err {
	int obs_output_error_number;
	int srt_error_code;
};

typedef struct SRTContext {
	SRTSOCKET fd;
	int eid;
	int64_t rw_timeout;
	int64_t listen_timeout;
	int recv_buffer_size;
	int send_buffer_size;

	int64_t maxbw;
	int pbkeylen;
	char *passphrase;
#if SRT_VERSION_VALUE >= 0x010302
	int enforced_encryption;
	int kmrefreshrate;
	int kmpreannounce;
	int64_t snddropdelay;
#endif
	int mss;
	int ffs;
	int ipttl;
	int iptos;
	int64_t inputbw;
	int oheadbw;
	int64_t latency;
	int tlpktdrop;
	int nakreport;
	int64_t connect_timeout;
	int payload_size;
	int64_t rcvlatency;
	int64_t peerlatency;
	enum SRTMode mode;
	int sndbuf;
	int rcvbuf;
	int lossmaxttl;
	int minversion;
	char *streamid;
	char *smoother;
	int messageapi;
	SRT_TRANSTYPE transtype;
	char *localip;
	char *localport;
	int linger;
	int tsbpd;
	double time; // time in s in order to post logs at definite intervals
	struct srt_err last_error;
} SRTContext;

#define OBS_OUTPUT_TIMEDOUT -10

static int libsrt_neterrno(URLContext *h)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int os_errno;
	int errj;
	int err = srt_getlasterror(&os_errno);
	blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: %s", srt_getlasterror_str());
	s->last_error.srt_error_code = err;
	switch (err) {
	case SRT_EASYNCRCV:
	case SRT_EASYNCSND:
		return AVERROR(EAGAIN);
	case SRT_ECONNREJ:
		errj = srt_getrejectreason(s->fd);
		if (errj == SRT_REJ_BADSECRET) {
			blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Wrong password");
			s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
		} else {
			blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Connection rejected, %s",
			     srt_rejectreason_str(errj));
			s->last_error.obs_output_error_number = OBS_OUTPUT_CONNECT_FAILED;
		}
		break;

	case SRT_ECONNSOCK:
		s->last_error.obs_output_error_number = OBS_OUTPUT_CONNECT_FAILED;
		break;
	case SRT_ESECFAIL:
		s->last_error.obs_output_error_number = OBS_OUTPUT_INVALID_STREAM;
		break;
	case SRT_ECONNLOST:
	case SRT_ENOCONN:
	case SRT_ECONNFAIL:
		s->last_error.obs_output_error_number = OBS_OUTPUT_DISCONNECTED;
		break;
	case SRT_ENOSERVER:
	case SRT_ETIMEOUT:
		s->last_error.obs_output_error_number = OBS_OUTPUT_TIMEDOUT;
		errj = srt_getrejectreason(s->fd);
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Connection timed out, %s",
		     srt_rejectreason_str(errj));
		break;
	default:
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		break;
	}

	return os_errno ? AVERROR(os_errno) : AVERROR_UNKNOWN;
}

static int libsrt_getsockopt(URLContext *h, SRTSOCKET fd, SRT_SOCKOPT optname, const char *optnamestr, void *optval,
			     int *optlen)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	if (srt_getsockopt(fd, 0, optname, optval, optlen) < 0) {
		blog(LOG_INFO, "[obs-ffmpeg mpegts muxer / libsrt]: Failed to get option %s on socket: %s", optnamestr,
		     srt_getlasterror_str());
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		return AVERROR(EIO);
	}
	return 0;
}

static int libsrt_socket_nonblock(SRTSOCKET socket, int enable)
{
	int ret, blocking = enable ? 0 : 1;
	/* Setting SRTO_{SND,RCV}SYN options to 1 enable blocking mode, setting them to 0 enable non-blocking mode. */
	ret = srt_setsockopt(socket, 0, SRTO_SNDSYN, &blocking, sizeof(blocking));
	if (ret < 0)
		return ret;
	ret = srt_setsockopt(socket, 0, SRTO_RCVSYN, &blocking, sizeof(blocking));
	if (ret < 0)
		return ret;

	return 0;
}

static int libsrt_epoll_create(URLContext *h, SRTSOCKET fd, int write)
{
	int modes = SRT_EPOLL_ERR | (write ? SRT_EPOLL_OUT : SRT_EPOLL_IN);
	int eid = srt_epoll_create();
	if (eid < 0)
		return libsrt_neterrno(h);
	if (srt_epoll_add_usock(eid, fd, &modes) < 0) {
		srt_epoll_release(eid);
		return libsrt_neterrno(h);
	}
	return eid;
}

static int libsrt_network_wait_fd(URLContext *h, int eid, int write)
{
	int ret, len = 1, errlen = 1;
	int err;
	SRTSOCKET ready[1] = {SRT_INVALID_SOCK};
	SRTSOCKET error[1] = {SRT_INVALID_SOCK};

	SRTContext *s = (SRTContext *)h->priv_data;
	if (write) {
		err = srt_epoll_wait(eid, error, &errlen, ready, &len, POLLING_TIME, 0, 0, 0, 0);
	} else {
		err = srt_epoll_wait(eid, ready, &len, error, &errlen, POLLING_TIME, 0, 0, 0, 0);
	}

	if (err < 0) {
		s->last_error.srt_error_code = err;
		if (srt_getlasterror(NULL) == SRT_ETIMEOUT) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_TIMEDOUT;
			ret = AVERROR(EAGAIN);
		} else {
			ret = libsrt_neterrno(h);
		}
	} else {
		ret = 0;
	}
	return ret;
}

int check_interrupt(AVIOInterruptCB *cb)
{
	if (cb && cb->callback)
		return cb->callback(cb->opaque);
	return 0;
}

static int libsrt_network_wait_fd_timeout(URLContext *h, int eid, int write, int64_t timeout, AVIOInterruptCB *int_cb)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int ret;
	int64_t wait_start = 0;

	while (1) {
		if (check_interrupt(int_cb)) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_CONNECT_FAILED;
			return AVERROR_EXIT;
		}
		ret = libsrt_network_wait_fd(h, eid, write);
		if (ret != AVERROR(EAGAIN))
			return ret;
		if (timeout > 0) {
			if (!wait_start) {
				wait_start = av_gettime_relative();
			} else if (av_gettime_relative() - wait_start > timeout) {
				s->last_error.obs_output_error_number = OBS_OUTPUT_TIMEDOUT;
				return AVERROR(ETIMEDOUT);
			}
		}
	}
}

static int libsrt_listen(int eid, SRTSOCKET fd, const struct sockaddr *addr, socklen_t addrlen, URLContext *h,
			 int64_t timeout)
{
	int ret;
	int reuse = 1;
	/* Max streamid length plus an extra space for the terminating null character */
	char streamid[513];
	int streamid_len = sizeof(streamid);

	blog(LOG_INFO, "[obs-ffmpeg mpegts muxer / libsrt]: listening for a connection...");

	if (srt_setsockopt(fd, SOL_SOCKET, SRTO_REUSEADDR, &reuse, sizeof(reuse))) {
		blog(LOG_WARNING, "[obs-ffmpeg mpegts muxer / libsrt]: setsockopt(SRTO_REUSEADDR) failed");
	}
	if (srt_bind(fd, addr, addrlen))
		return libsrt_neterrno(h);

	if (srt_listen(fd, 1))
		return libsrt_neterrno(h);

	ret = libsrt_network_wait_fd_timeout(h, eid, 0, timeout, &h->interrupt_callback);
	if (ret < 0)
		return ret;

	ret = srt_accept(fd, NULL, NULL);
	if (ret < 0)
		return libsrt_neterrno(h);

	if (libsrt_socket_nonblock(ret, 1) < 0)
		blog(LOG_DEBUG, "[obs-ffmpeg mpegts muxer / libsrt]: libsrt_socket_nonblock failed");

	if (!libsrt_getsockopt(h, ret, SRTO_STREAMID, "SRTO_STREAMID", streamid, &streamid_len))
		blog(LOG_INFO, "[obs-ffmpeg mpegts muxer / libsrt]: Accept streamid [%s], length %d", streamid,
		     streamid_len);

	return ret;
}

static int libsrt_listen_connect(int eid, SRTSOCKET fd, const struct sockaddr *addr, socklen_t addrlen, int64_t timeout,
				 URLContext *h, int will_try_next)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int ret;
	if (srt_connect(fd, addr, addrlen) < 0)
		return libsrt_neterrno(h);

	ret = libsrt_network_wait_fd_timeout(h, eid, 1, timeout, &h->interrupt_callback);
	if (ret < 0) {
		if (will_try_next) {
			blog(LOG_WARNING,
			     "[obs-ffmpeg mpegts muxer / libsrt]: Connection to %s failed (%s), trying next address",
			     h->url, av_err2str(ret));
		} else {
			blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Connection to %s failed: %s", h->url,
			     av_err2str(ret));
		}
	} else {
		SRT_SOCKSTATUS state = srt_getsockstate(fd);
		if (state != SRTS_CONNECTED) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_CONNECT_FAILED;
			return AVERROR_EXIT;
		}
	}
	return ret;
}

static int libsrt_setsockopt(URLContext *h, SRTSOCKET fd, SRT_SOCKOPT optname, const char *optnamestr,
			     const void *optval, int optlen)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int ret = srt_setsockopt(fd, 0, optname, optval, optlen);
	if (ret < 0) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Failed to set option %s on socket: %s", optnamestr,
		     srt_getlasterror_str());
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		s->last_error.srt_error_code = ret;
		return AVERROR(EIO);
	}
	return 0;
}

/* - The "POST" options can be altered any time on a connected socket.
     They MAY have also some meaning when set prior to connecting; such
     option is SRTO_RCVSYN, which makes connect/accept call asynchronous.
     Because of that this option is treated special way in this app. */
static int libsrt_set_options_post(URLContext *h, SRTSOCKET fd)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int ret = libsrt_setsockopt(h, fd, SRTO_INPUTBW, "SRTO_INPUTBW", &s->inputbw, sizeof(s->inputbw));
	int ret2 = libsrt_setsockopt(h, fd, SRTO_OHEADBW, "SRTO_OHEADBW", &s->oheadbw, sizeof(s->oheadbw));
	if ((s->inputbw >= 0 && ret < 0) || (s->oheadbw >= 0 && ret2 < 0)) {
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		s->last_error.srt_error_code = srt_getlasterror(NULL);
		return OBS_OUTPUT_ERROR;
	}
	return 0;
}

/* - The "PRE" options must be set prior to connecting and can't be altered
     on a connected socket, however if set on a listening socket, they are
     derived by accept-ed socket. */
static int libsrt_set_options_pre(URLContext *h, SRTSOCKET fd)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int yes = 1;
	int latency = (int)(s->latency / 1000);
	int rcvlatency = (int)(s->rcvlatency / 1000);
	int peerlatency = (int)(s->peerlatency / 1000);
#if SRT_VERSION_VALUE >= 0x010302
	int snddropdelay = s->snddropdelay > 0 ? (int)(s->snddropdelay / 1000) : (int)(s->snddropdelay);
#endif
	int connect_timeout = (int)(s->connect_timeout);

	if ((s->mode == SRT_MODE_RENDEZVOUS &&
	     libsrt_setsockopt(h, fd, SRTO_RENDEZVOUS, "SRTO_RENDEZVOUS", &yes, sizeof(yes)) < 0) ||
	    (s->transtype != SRTT_INVALID &&
	     libsrt_setsockopt(h, fd, SRTO_TRANSTYPE, "SRTO_TRANSTYPE", &s->transtype, sizeof(s->transtype)) < 0) ||
	    (s->maxbw >= 0 && libsrt_setsockopt(h, fd, SRTO_MAXBW, "SRTO_MAXBW", &s->maxbw, sizeof(s->maxbw)) < 0) ||
	    (s->pbkeylen >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_PBKEYLEN, "SRTO_PBKEYLEN", &s->pbkeylen, sizeof(s->pbkeylen)) < 0) ||
	    (s->passphrase && libsrt_setsockopt(h, fd, SRTO_PASSPHRASE, "SRTO_PASSPHRASE", s->passphrase,
						(int)strlen(s->passphrase)) < 0) ||
#if SRT_VERSION_VALUE >= 0x010302
#if SRT_VERSION_VALUE >= 0x010401
	    (s->enforced_encryption >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_ENFORCEDENCRYPTION, "SRTO_ENFORCEDENCRYPTION", &s->enforced_encryption,
			       sizeof(s->enforced_encryption)) < 0) ||
#else
	    /* SRTO_STRICTENC == SRTO_ENFORCEDENCRYPTION (53), but for compatibility, we used SRTO_STRICTENC */
	    (s->enforced_encryption >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_STRICTENC, "SRTO_STRICTENC", &s->enforced_encryption,
			       sizeof(s->enforced_encryption)) < 0) ||
#endif
	    (s->kmrefreshrate >= 0 && libsrt_setsockopt(h, fd, SRTO_KMREFRESHRATE, "SRTO_KMREFRESHRATE",
							&s->kmrefreshrate, sizeof(s->kmrefreshrate)) < 0) ||
	    (s->kmpreannounce >= 0 && libsrt_setsockopt(h, fd, SRTO_KMPREANNOUNCE, "SRTO_KMPREANNOUNCE",
							&s->kmpreannounce, sizeof(s->kmpreannounce)) < 0) ||
	    (s->snddropdelay >= -1 && libsrt_setsockopt(h, fd, SRTO_SNDDROPDELAY, "SRTO_SNDDROPDELAY", &snddropdelay,
							sizeof(snddropdelay)) < 0) ||
#endif
	    (s->mss >= 0 && libsrt_setsockopt(h, fd, SRTO_MSS, "SRTO_MSS", &s->mss, sizeof(s->mss)) < 0) ||
	    (s->ffs >= 0 && libsrt_setsockopt(h, fd, SRTO_FC, "SRTO_FC", &s->ffs, sizeof(s->ffs)) < 0) ||
	    (s->ipttl >= 0 && libsrt_setsockopt(h, fd, SRTO_IPTTL, "SRTO_IPTTL", &s->ipttl, sizeof(s->ipttl)) < 0) ||
	    (s->iptos >= 0 && libsrt_setsockopt(h, fd, SRTO_IPTOS, "SRTO_IPTOS", &s->iptos, sizeof(s->iptos)) < 0) ||
	    (s->latency >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_LATENCY, "SRTO_LATENCY", &latency, sizeof(latency)) < 0) ||
	    (s->rcvlatency >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_RCVLATENCY, "SRTO_RCVLATENCY", &rcvlatency, sizeof(rcvlatency)) < 0) ||
	    (s->peerlatency >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_PEERLATENCY, "SRTO_PEERLATENCY", &peerlatency, sizeof(peerlatency)) < 0) ||
	    (s->tlpktdrop >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_TLPKTDROP, "SRTO_TLPKTDROP", &s->tlpktdrop, sizeof(s->tlpktdrop)) < 0) ||
	    (s->nakreport >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_NAKREPORT, "SRTO_NAKREPORT", &s->nakreport, sizeof(s->nakreport)) < 0) ||
	    (connect_timeout >= 0 && libsrt_setsockopt(h, fd, SRTO_CONNTIMEO, "SRTO_CONNTIMEO", &connect_timeout,
						       sizeof(connect_timeout)) < 0) ||
	    (s->sndbuf >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_SNDBUF, "SRTO_SNDBUF", &s->sndbuf, sizeof(s->sndbuf)) < 0) ||
	    (s->rcvbuf >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_RCVBUF, "SRTO_RCVBUF", &s->rcvbuf, sizeof(s->rcvbuf)) < 0) ||
	    (s->lossmaxttl >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_LOSSMAXTTL, "SRTO_LOSSMAXTTL", &s->lossmaxttl, sizeof(s->lossmaxttl)) < 0) ||
	    (s->minversion >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_MINVERSION, "SRTO_MINVERSION", &s->minversion, sizeof(s->minversion)) < 0) ||
	    (s->streamid &&
	     libsrt_setsockopt(h, fd, SRTO_STREAMID, "SRTO_STREAMID", s->streamid, (int)strlen(s->streamid)) < 0) ||
#if SRT_VERSION_VALUE >= 0x010401
	    (s->smoother &&
	     libsrt_setsockopt(h, fd, SRTO_CONGESTION, "SRTO_CONGESTION", s->smoother, (int)strlen(s->smoother)) < 0) ||
#else
	    (s->smoother &&
	     libsrt_setsockopt(h, fd, SRTO_SMOOTHER, "SRTO_SMOOTHER", s->smoother, (int)strlen(s->smoother)) < 0) ||
#endif
	    (s->messageapi >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_MESSAGEAPI, "SRTO_MESSAGEAPI", &s->messageapi, sizeof(s->messageapi)) < 0) ||
	    (s->payload_size >= 0 && libsrt_setsockopt(h, fd, SRTO_PAYLOADSIZE, "SRTO_PAYLOADSIZE", &s->payload_size,
						       sizeof(s->payload_size)) < 0) ||
	    (/*(h->flags & AVIO_FLAG_WRITE) &&*/
	     libsrt_setsockopt(h, fd, SRTO_SENDER, "SRTO_SENDER", &yes, sizeof(yes)) < 0) ||
	    (s->tsbpd >= 0 &&
	     libsrt_setsockopt(h, fd, SRTO_TSBPDMODE, "SRTO_TSBPDMODE", &s->tsbpd, sizeof(s->tsbpd)) < 0)) {
		s->last_error.srt_error_code = srt_getlasterror(NULL);
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		return AVERROR(EIO);
	}

	if (s->linger >= 0) {
		struct linger lin;
		lin.l_linger = s->linger;
		lin.l_onoff = lin.l_linger > 0 ? 1 : 0;
		if (libsrt_setsockopt(h, fd, SRTO_LINGER, "SRTO_LINGER", &lin, sizeof(lin)) < 0) {
			s->last_error.srt_error_code = srt_getlasterror(NULL);
			s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
			return AVERROR(EIO);
		}
	}
	return 0;
}

static int libsrt_setup(URLContext *h, const char *uri)
{
	struct addrinfo hints = {0}, *ai, *cur_ai;
	int port;
	SRTSOCKET fd;
	SRTContext *s = (SRTContext *)h->priv_data;
	const char *p;
	char buf[1024];
	int ret;
	char hostname[1024], proto[1024], path[1024];
	char portstr[10];
	int64_t open_timeout = 0;
	int eid;
	struct sockaddr_in la;

	av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname), &port, path, sizeof(path), uri);
	if (strcmp(proto, "srt")) {
		s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
		return AVERROR(EINVAL);
	}

	if (port <= 0 || port >= 65536) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Port missing in uri");
		s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
		return AVERROR(EINVAL);
	}
	p = strchr(uri, '?');
	if (p) {
		if (av_find_info_tag(buf, sizeof(buf), "timeout", p)) {
			s->rw_timeout = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "listen_timeout", p)) {
			s->listen_timeout = strtoll(buf, NULL, 10);
		}
	}
	if (s->rw_timeout >= 0) {
		open_timeout = h->rw_timeout = s->rw_timeout;
	}
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	snprintf(portstr, sizeof(portstr), "%d", port);
	if (s->mode == SRT_MODE_LISTENER)
		hints.ai_flags |= AI_PASSIVE;
	ret = getaddrinfo(hostname[0] ? hostname : NULL, portstr, &hints, &ai);
	if (ret) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Failed to resolve hostname %s: %s", hostname,
#ifdef _WIN32
		     gai_strerrorA(ret)
#else
		     gai_strerror(ret)
#endif
		);
		s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
		return AVERROR(EIO);
	}

	cur_ai = ai;
	if (s->mode == SRT_MODE_RENDEZVOUS) {
		if (s->localip == NULL || s->localport == NULL) {
			blog(LOG_ERROR, "Invalid adapter configuration\n");
			s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
			return AVERROR(EIO);
		}
		blog(LOG_DEBUG, "[obs-ffmpeg mpegts muxer / libsrt]: Adapter options %s:%s\n", s->localip,
		     s->localport);
		int lp = strtol(s->localport, NULL, 10);
		if (lp <= 0 || lp >= 65536) {
			blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: Local port missing in URL\n");
			s->last_error.obs_output_error_number = OBS_OUTPUT_BAD_PATH;
			return AVERROR(EIO);
		}
		la.sin_family = AF_INET;
		la.sin_port = htons(port);
		inet_pton(AF_INET, s->localip, &la.sin_addr.s_addr);
	}

restart:

	fd = srt_create_socket();
	if (fd < 0) {
		ret = libsrt_neterrno(h);
		goto fail;
	}

	if ((ret = libsrt_set_options_pre(h, fd)) < 0) {
		goto fail;
	}

	/* Set the socket's send or receive buffer sizes, if specified.
           If unspecified or setting fails, system default is used. */
	if (s->recv_buffer_size > 0) {
		srt_setsockopt(fd, SOL_SOCKET, SRTO_UDP_RCVBUF, &s->recv_buffer_size, sizeof(s->recv_buffer_size));
	}
	if (s->send_buffer_size > 0) {
		srt_setsockopt(fd, SOL_SOCKET, SRTO_UDP_SNDBUF, &s->send_buffer_size, sizeof(s->send_buffer_size));
	}
	if (libsrt_socket_nonblock(fd, 1) < 0)
		blog(LOG_DEBUG, "[obs-ffmpeg mpegts muxer / libsrt]: libsrt_socket_nonblock failed");

	if (s->mode == SRT_MODE_LISTENER) {
		int read_eid = ret = libsrt_epoll_create(h, fd, 0);
		if (ret < 0) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
			goto fail1;
		}

		// multi-client
		ret = libsrt_listen(read_eid, fd, cur_ai->ai_addr, (socklen_t)cur_ai->ai_addrlen, h, s->listen_timeout);
		srt_epoll_release(read_eid);
		if (ret < 0) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
			goto fail1;
		}
		srt_close(fd);
		fd = ret;
	} else {
		int write_eid = ret = libsrt_epoll_create(h, fd, 1);
		if (ret < 0) {
			s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
			goto fail1;
		}
		if (s->mode == SRT_MODE_RENDEZVOUS) {
			if (srt_bind(fd, (struct sockaddr *)&la, sizeof(struct sockaddr_in))) {
				ret = libsrt_neterrno(h);
				srt_epoll_release(write_eid);
				goto fail1;
			}
		}

		ret = libsrt_listen_connect(write_eid, fd, cur_ai->ai_addr, (socklen_t)(cur_ai->ai_addrlen),
					    open_timeout, h, !!cur_ai->ai_next);
		srt_epoll_release(write_eid);
		if (ret < 0) {
			s->last_error.srt_error_code = srt_getlasterror(NULL);
			if (ret == AVERROR_EXIT) {
				s->last_error.obs_output_error_number = OBS_OUTPUT_CONNECT_FAILED;
				goto fail1;
			} else {
				s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
				goto fail;
			}
		}
	}
	if ((ret = libsrt_set_options_post(h, fd)) < 0) {
		goto fail;
	}

	int packet_size = 0;
	int optlen = sizeof(packet_size);
	ret = libsrt_getsockopt(h, fd, SRTO_PAYLOADSIZE, "SRTO_PAYLOADSIZE", &packet_size, &optlen);
	if (ret < 0)
		goto fail1;
	if (packet_size > 0)
		h->max_packet_size = packet_size;

	eid = libsrt_epoll_create(h, fd, 1);
	if (eid < 0) {
		ret = eid;
		s->last_error.obs_output_error_number = OBS_OUTPUT_ERROR;
		goto fail1;
	}

	s->fd = fd;
	s->eid = eid;

	freeaddrinfo(ai);
	return 0;

fail:
	if (cur_ai->ai_next) {
		/* Retry with the next sockaddr */
		cur_ai = cur_ai->ai_next;
		if (fd >= 0)
			srt_close(fd);
		ret = 0;
		goto restart;
	}
fail1:
	if (fd >= 0)
		srt_close(fd);
	freeaddrinfo(ai);
	return ret;
}

static void libsrt_set_defaults(SRTContext *s)
{
	s->rw_timeout = -1;
	s->listen_timeout = -1;
	s->send_buffer_size = -1;
	s->recv_buffer_size = -1;
	s->payload_size = SRT_LIVE_DEFAULT_PAYLOAD_SIZE;
	s->maxbw = -1;
	s->pbkeylen = -1;
	s->mss = -1;
	s->ffs = -1;
	s->ipttl = -1;
	s->iptos = -1;
	s->inputbw = 0;
	s->oheadbw = 25;
	s->latency = -1;
	s->rcvlatency = -1;
	s->peerlatency = -1;
	s->tlpktdrop = -1;
	s->nakreport = -1;
	s->connect_timeout = -1;
	s->mode = SRT_MODE_CALLER;
	s->sndbuf = -1;
	s->rcvbuf = -1;
	s->lossmaxttl = -1;
	s->minversion = -1;
	s->smoother = NULL;
	s->messageapi = -1;
	s->transtype = SRTT_LIVE;
	s->linger = -1;
	s->tsbpd = -1;
}

static int libsrt_close(URLContext *h);
static int libsrt_open(URLContext *h, const char *uri)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	const char *p;
	char buf[1024];
	int ret = 0;

	if (srt_startup() < 0) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: libsrt failed to load");
		return OBS_OUTPUT_ERROR;
	} else {
		blog(LOG_INFO, "[obs-ffmpeg mpegts muxer / libsrt]: libsrt version %s loaded", SRT_VERSION_STRING);
	}
	libsrt_set_defaults(s);

	s->last_error.obs_output_error_number = OBS_OUTPUT_SUCCESS;
	s->last_error.srt_error_code = SRT_SUCCESS;

	/* SRT options (srt/srt.h) */
	p = strchr(uri, '?');
	if (p) {
		if (av_find_info_tag(buf, sizeof(buf), "maxbw", p)) {
			s->maxbw = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "pbkeylen", p)) {
			s->pbkeylen = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "passphrase", p)) {
			av_freep(&s->passphrase);
			s->passphrase = av_strndup(buf, strlen(buf));
		}
#if SRT_VERSION_VALUE >= 0x010302
		if (av_find_info_tag(buf, sizeof(buf), "enforced_encryption", p)) {
			s->enforced_encryption = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "kmrefreshrate", p)) {
			s->kmrefreshrate = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "kmpreannounce", p)) {
			s->kmpreannounce = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "snddropdelay", p)) {
			s->snddropdelay = strtoll(buf, NULL, 10);
		}
#endif
		if (av_find_info_tag(buf, sizeof(buf), "mss", p)) {
			s->mss = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "ffs", p)) {
			s->ffs = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "ipttl", p)) {
			s->ipttl = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "iptos", p)) {
			s->iptos = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "inputbw", p)) {
			s->inputbw = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "oheadbw", p)) {
			s->oheadbw = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "latency", p)) {
			s->latency = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "tsbpddelay", p)) {
			s->latency = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "rcvlatency", p)) {
			s->rcvlatency = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "peerlatency", p)) {
			s->peerlatency = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "tlpktdrop", p)) {
			s->tlpktdrop = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "nakreport", p)) {
			s->nakreport = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "connect_timeout", p)) {
			s->connect_timeout = strtoll(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "payload_size", p) ||
		    av_find_info_tag(buf, sizeof(buf), "pkt_size", p)) {
			s->payload_size = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "mode", p)) {
			if (!strcmp(buf, "caller")) {
				s->mode = SRT_MODE_CALLER;
			} else if (!strcmp(buf, "listener")) {
				s->mode = SRT_MODE_LISTENER;
			} else if (!strcmp(buf, "rendezvous")) {
				s->mode = SRT_MODE_RENDEZVOUS;
			} else {
				ret = OBS_OUTPUT_ERROR;
				goto err;
			}
		}
		if (av_find_info_tag(buf, sizeof(buf), "sndbuf", p)) {
			s->sndbuf = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "rcvbuf", p)) {
			s->rcvbuf = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "lossmaxttl", p)) {
			s->lossmaxttl = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "minversion", p)) {
			s->minversion = strtol(buf, NULL, 0);
		}
		if (av_find_info_tag(buf, sizeof(buf), "streamid", p)) {
			av_freep(&s->streamid);
			s->streamid = av_strdup(buf);
			if (!s->streamid) {
				ret = OBS_OUTPUT_ERROR;
				goto err;
			}
		}
		if (av_find_info_tag(buf, sizeof(buf), "smoother", p)) {
			av_freep(&s->smoother);
			s->smoother = av_strdup(buf);
			if (!s->smoother) {
				ret = OBS_OUTPUT_ERROR;
				goto err;
			}
		}
		if (av_find_info_tag(buf, sizeof(buf), "messageapi", p)) {
			s->messageapi = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "transtype", p)) {
			if (!strcmp(buf, "live")) {
				s->transtype = SRTT_LIVE;
			} else if (!strcmp(buf, "file")) {
				s->transtype = SRTT_FILE;
			} else {
				ret = OBS_OUTPUT_ERROR;
				goto err;
			}
		}
		if (av_find_info_tag(buf, sizeof(buf), "linger", p)) {
			s->linger = strtol(buf, NULL, 10);
		}
		if (av_find_info_tag(buf, sizeof(buf), "localip", p)) {
			s->localip = av_strndup(buf, strlen(buf));
		}
		if (av_find_info_tag(buf, sizeof(buf), "localport", p)) {
			s->localport = av_strndup(buf, strlen(buf));
		}
	}
	ret = libsrt_setup(h, uri);
	if (ret < 0) {
		ret = s->last_error.obs_output_error_number;
		goto err;
	}

#ifdef _WIN32
	struct timeb timebuffer;
	ftime(&timebuffer);
	s->time = (double)timebuffer.time + 0.001 * (double)timebuffer.millitm;
#else
	struct timespec timesp;
	clock_gettime(CLOCK_REALTIME, &timesp);
	s->time = (double)timesp.tv_sec + 0.000000001 * (double)timesp.tv_nsec;
#endif

	/* In caller mode, srt_connect won't fail even if there is no established connection in non-blocking mode, so
	 * we explicitly check for the socket status before okaying OBS_OUTPUT_SUCCESS.
	 */
	SRT_SOCKSTATUS state = srt_getsockstate(s->fd);
	if (state != SRTS_CONNECTED) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: SRT socket not connected after open (state=%d)",
		     state);
		libsrt_close(h);
		if (s->last_error.obs_output_error_number != OBS_OUTPUT_SUCCESS)
			return s->last_error.obs_output_error_number;
		else
			return OBS_OUTPUT_CONNECT_FAILED;
	}
	return OBS_OUTPUT_SUCCESS;

err:
	av_freep(&s->smoother);
	av_freep(&s->streamid);
	srt_cleanup();
	return ret;
}

static int libsrt_write(URLContext *h, const uint8_t *buf, int size)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	int ret;
	SRT_TRACEBSTATS perf;

	ret = libsrt_network_wait_fd_timeout(h, s->eid, 1, h->rw_timeout, &h->interrupt_callback);
	if (ret)
		return ret;

	ret = srt_send(s->fd, (char *)buf, size);
	if (ret < 0) {
		ret = libsrt_neterrno(h);
	} else {
		/* log every 60 seconds the rtt and link bandwidth
		 * rtt: round-trip time
		 * link bandwidth: bandwidth from ingest to egress
		*/
#ifdef _WIN32
		struct timeb timebuffer;
		ftime(&timebuffer);
		double time = (double)timebuffer.time + 0.001 * (double)timebuffer.millitm;
#else
		struct timespec timesp;
		clock_gettime(CLOCK_REALTIME, &timesp);
		double time = (double)timesp.tv_sec + 0.000000001 * (double)timesp.tv_nsec;
#endif
		if (time > (s->time + 60.0)) {
			srt_bistats(s->fd, &perf, 0, 1);
			blog(LOG_DEBUG, "[obs-ffmpeg mpegts muxer / libsrt]: RTT [%.2f ms], Link Bandwidth [%.1f Mbps]",
			     perf.msRTT, perf.mbpsBandwidth);
			s->time = time;
		}
	}

	return ret;
}

static int libsrt_close(URLContext *h)
{
	SRTContext *s = (SRTContext *)h->priv_data;
	if (!s)
		return 0;
	if (s->streamid)
		av_freep(&s->streamid);
	if (s->passphrase)
		av_freep(&s->passphrase);
	/* Log stream stats. */
	SRT_TRACEBSTATS perf = {0};
	srt_bstats(s->fd, &perf, 1);
	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO,
	     "[obs-ffmpeg mpegts muxer / libsrt]: Session Summary\n"
	     "\ttime elapsed [%.1f sec]\n"
	     "\tmean speed [%.1f Mbp]\n"
	     "\ttotal bytes sent [%.1f MB]\n"
	     "\tbytes retransmitted [%.1f %%]\n"
	     "\tbytes dropped [%.1f %%]\n",
	     (double)perf.msTimeStamp / 1000.0, perf.mbpsSendRate, (double)perf.byteSentTotal / 1000000.0,
	     perf.byteSentTotal ? perf.byteRetransTotal / perf.byteSentTotal * 100.0 : 0,
	     perf.byteSentTotal ? perf.byteSndDropTotal / perf.byteSentTotal * 100.0 : 0);

	srt_epoll_release(s->eid);
	int err = srt_close(s->fd);
	if (err < 0) {
		blog(LOG_ERROR, "[obs-ffmpeg mpegts muxer / libsrt]: %s", srt_getlasterror_str());
		return -1;
	}

	srt_cleanup();
	blog(LOG_INFO, "[obs-ffmpeg mpegts muxer / libsrt]: SRT connection closed");

	return 0;
}
