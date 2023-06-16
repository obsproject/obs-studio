/**
 * An implementation of RFC 6555 (Happy Eyeballs) to connect to IPv6 hosts with
 * IPv4 fast fallback. This implementation currently only works for TCP.
 *
 * More precisely this algorithm will attempt the configured preferred protocol
 * family, followed by the other. So if the user has configured their operating
 * system to prefer IPv4 it will attempt IPv4 first, followed by IPv6.
 * Generally, though, the default is to prefer IPv6.
 *
 * Initial implementation by james hurley <jamesghurley@gmail.com>
 */

/**
 * Copyright (c) 2023 Twitch Interactive, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct happy_eyeballs_ctx;

/**
 * Create and initialize a Happy Eyeballs session. This context is expected to
 * be accessed and mutated on a single thread. Accessing and mutating this
 * context from multiple threads may lead to undefined behavior and deadlocks.
 *
 * On failure *context will always be NULL.
 *
 * You must call happy_eyeballs_destroy() to free any resources.
 * This function will return 0 on success and *context will be non-NULL.
 *
 * -EINVAL if context is NULL
 * -ENOMEM if there is an allocation failure.
 * Another negative error code if there is a failure creating required
 * resources.
 */
int happy_eyeballs_create(struct happy_eyeballs_ctx **context);

/**
 * Connect to a provided hostname and port. This function will block for up to
 * 1.2 seconds. After this function returns with a success result (0), you need
 * to check happy_eyeballs_try or happy_eyeballs_timedwait to determine if a
 * connection has been established.
 *
 * Upon successfully starting the connection process and waiting for the
 * initial delay timers, this function will return 0 if the connection has been
 * successfully established, or EAGAIN if the caller should wait longer.
 *
 * This function will return -EINVAL if context, hostname, or port are not set.
 */
int happy_eyeballs_connect(struct happy_eyeballs_ctx *context,
			   const char *hostname, int port);

/**
 * Optionally set the interface address. You may pass 0 and NULL for these
 * parameters, respectively, to clear this setting. This must be called before
 * happy_eyeballs_connect.
 *
 * Returns 0 on success or -EINVAL if context is not set.
 */
int happy_eyeballs_set_bind_addr(struct happy_eyeballs_ctx *context,
				 socklen_t addr_len,
				 struct sockaddr_storage *addr_storage);

/**
 * Returns a positive socket file descriptor if a winner is available.
 *
 * This function will only return a valid socket after happy_eyeballs_try or
 * happy_eyeballs_timedwait has returned 0.
 *
 * Returns -1 if there is no socket available yet or -EINVAL if context is not
 * set.
 */
SOCKET happy_eyeballs_get_socket_fd(const struct happy_eyeballs_ctx *context);

/**
 * Fills the addr struct with the winning socket address, if there is one.
 *
 * This function will only return a valid address after happy_eyeballs_try or
 * happy_eyeballs_timedwait has returned 0.
 *
 * On success, returns the address length of the sockaddr or -EINVAL if context
 * is not set.
 */
int happy_eyeballs_get_remote_addr(const struct happy_eyeballs_ctx *context,
				   struct sockaddr_storage *addr);

/**
 * Returns the current error code for the context or -EINVAL if context is not
 * set.
 */
int happy_eyeballs_get_error_code(const struct happy_eyeballs_ctx *context);

/**
 * Returns the current error message for the context. The returned value may be
 * NULL if there is no error message.
 */
const char *
happy_eyeballs_get_error_message(const struct happy_eyeballs_ctx *context);

/**
 * Returns the amount of time domain name resolution took, in nanoseconds
 */
uint64_t happy_eyeballs_get_name_resolution_time_ns(
	const struct happy_eyeballs_ctx *context);

/**
 * Returns the amount of time connection (or failure) took, in nanoseconds, or
 * 0 if happy eyeballs has not yet completed.
 */
uint64_t
happy_eyeballs_get_connection_time_ns(const struct happy_eyeballs_ctx *context);

/**
 * Test if the process has completed without blocking. This function will
 * return the following values:
 *
 * 0 on successful completion. You may use context.socket_fd.
 *
 * EAGAIN if the process is ongoing.
 *
 * -1 if some other error has occurred. Check happy_eyeballs_get_error_code and
 *  happy_eyeballs_get_error_message.
 */
int happy_eyeballs_try(struct happy_eyeballs_ctx *context);

/**
 * Block until the process has completed.
 *
 * This function will return the following values:
 *
 * 0 on successful completion. You may use context.socket_fd
 *
 * -1 on failure. Check happy_eyeballs_get_error_code and
 * happy_eyeballs_get_error_message for more details.
 *
 * ETIMEDOUT on timeout. It is unlikely that context.socket_fd is set. You may
 * continue to wait by calling this function again, or call
 * happy_eyeballs_destroy to close the session.
 */
int happy_eyeballs_timedwait(struct happy_eyeballs_ctx *context,
			     unsigned long time_in_millis);

/**
 * Call timedwait with a default wait duration of 25 seconds.
 */
int happy_eyeballs_timedwait_default(struct happy_eyeballs_ctx *context);

/**
 * Releases any resources claimed. Accessing context after calling this
 * function will be a use-after-free error.
 *
 * This function will not close the socket specified in context->socket_fd. You
 * must close that yourself!
 */
int happy_eyeballs_destroy(struct happy_eyeballs_ctx *context);

#ifdef __cplusplus
}
#endif
