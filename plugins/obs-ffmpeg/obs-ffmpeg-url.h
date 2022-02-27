#pragma once
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/parseutils.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>

#define HTTP_PROTO "http"
#define RIST_PROTO "rist"
#define SRT_PROTO "srt"
#define TCP_PROTO "tcp"
#define UDP_PROTO "udp"

/* lightened version of a struct used by avformat */
typedef struct URLContext {
	void *priv_data; /* SRTContext or RISTContext */
	char *url;       /* URL */
	int max_packet_size;
	AVIOInterruptCB interrupt_callback;
	int64_t rw_timeout; /* max time to wait for write completion in mcs */
} URLContext;

#define UDP_DEFAULT_PAYLOAD_SIZE 1316

/* We need to override libsrt/win/syslog_defs.h due to conflicts w/ some libobs
 * definitions.
 */
#ifndef _WIN32
#define _SYS_SYSLOG_H
#endif

#ifdef _WIN32
#ifndef INC_SRT_WINDOWS_SYSLOG_DEFS_H
#define INC_SRT_WINDOWS_SYSLOG_DEFS_H

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
//#define LOG_WARNING 4 // this creates issues w/ libobs LOG_WARNING = 200
#define LOG_NOTICE 5
//#define LOG_INFO 6 // issue w/ libobs
//#define LOG_DEBUG 7 // issue w/ libobs
#define LOG_PRIMASK 0x07

#define LOG_PRI(p) ((p)&LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri) (((fac) << 3) | (pri))

#define LOG_KERN (0 << 3)
#define LOG_USER (1 << 3)
#define LOG_MAIL (2 << 3)
#define LOG_DAEMON (3 << 3)
#define LOG_AUTH (4 << 3)
#define LOG_SYSLOG (5 << 3)
#define LOG_LPR (6 << 3)
#define LOG_NEWS (7 << 3)
#define LOG_UUCP (8 << 3)
#define LOG_CRON (9 << 3)
#define LOG_AUTHPRIV (10 << 3)
#define LOG_FTP (11 << 3)

/* Codes through 15 are reserved for system use */
#define LOG_LOCAL0 (16 << 3)
#define LOG_LOCAL1 (17 << 3)
#define LOG_LOCAL2 (18 << 3)
#define LOG_LOCAL3 (19 << 3)
#define LOG_LOCAL4 (20 << 3)
#define LOG_LOCAL5 (21 << 3)
#define LOG_LOCAL6 (22 << 3)
#define LOG_LOCAL7 (23 << 3)

#define LOG_NFACILITIES 24
#define LOG_FACMASK 0x03f8
#define LOG_FAC(p) (((p)&LOG_FACMASK) >> 3)
#endif
#endif

/* We need to override libsrt/logging_api.h due to conflicts with some libobs
 * definitions.
 */

#define INC_SRT_LOGGING_API_H

// These are required for access functions:
// - adding FA (requires set)
// - setting a log stream (requires iostream)
#ifdef __cplusplus
#include <set>
#include <iostream>
#endif

#ifndef _WIN32
#include <syslog.h>
#endif

// Syslog is included so that it provides log level names.
// Haivision log standard requires the same names plus extra one:
#ifndef LOG_DEBUG_TRACE
#define LOG_DEBUG_TRACE 8
#endif

// Flags
#define SRT_LOGF_DISABLE_TIME 1
#define SRT_LOGF_DISABLE_THREADNAME 2
#define SRT_LOGF_DISABLE_SEVERITY 4
#define SRT_LOGF_DISABLE_EOL 8

// Handler type
typedef void SRT_LOG_HANDLER_FN(void *opaque, int level, const char *file,
				int line, const char *area,
				const char *message);

#ifdef __cplusplus
namespace srt_logging {

struct LogFA {
private:
	int value;

public:
	operator int() const { return value; }

	LogFA(int v) : value(v) {}
};

const LogFA LOGFA_GENERAL = 0;

namespace LogLevel {
enum type {
	fatal = LOG_CRIT,
	error = LOG_ERR,
	warning = 4, //issue w/ libobs so LOG_WARNING is removed
	note = LOG_NOTICE,
	debug = 7 //issue w/ libobs so LOG_DEBUG is removed
};
}
class Logger;
}
#endif
