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
#include "happy-eyeballs.h"
#include "util/darray.h"
#include "util/dstr.h"
#include "util/platform.h"
#include "util/threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifdef _MSC_VER
#pragma warning(disable : 4996) /* depricated warnings */
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#define GetSockError() WSAGetLastError()
#ifdef _MSC_VER
#define snprintf _snprintf
#endif
#else /* !_WIN32 */
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define GetSockError() errno
#endif

/* ------------------------------------------------------------------------- */
/* happy eyeballs coefficients                                               */

/* this is the same default as libcurl */
#define HAPPY_EYEBALLS_DELAY_MS 200
#define HAPPY_EYEBALLS_MAX_ATTEMPTS 6
/* Total time to wait for sockets or to finish trying; whichever is shorter */
#define HAPPY_EYEBALLS_CONNECTION_TIMEOUT_MS 25000

/* ------------------------------------------------------------------------- */

#ifndef INVALID_SOCKET
#define INVALID_SOCKET ~0
#endif

#define STATUS_SUCCESS 0
#define STATUS_FAILURE -1
#define STATUS_INVALID_ARGUMENT -EINVAL

struct happy_eyeballs_candidate {
	SOCKET sockfd;
	os_event_t *socket_completed_event;
	pthread_t thread;
	int error;
};

struct happy_eyeballs_ctx {
	/**
	 * socket_fd will be non-zero upon successful connection to the host
	 * and port specified.
	 */
	SOCKET socket_fd;

	/**
	 * winner_addr will be non-zero upon successful connection to the host
	 * and port specified.
	 */
	struct sockaddr_storage winner_addr;

	/**
	 * winner_addr_len will be non-zero upon successful connection to the
	 * host and port specified.
	 */
	socklen_t winner_addr_len;

	/**
	 * error will be non-zero in case of an error during operation.
	 */
	int error;

	/**
	 * error_message may be set when there is a non-zero error code.
	 */
	const char *error_message;

	/**
	 * Set along with bind_addr to hint which interface to use.
	 */
	socklen_t bind_addr_len;

	/**
	 * Set along with bind_addr_len to hint which interface to use.
	 */
	struct sockaddr_storage bind_addr;

	/**
	 * List of in-flight connection attempts.
	 */
	DARRAY(struct happy_eyeballs_candidate) candidates;

	/**
	 * Protects against multiple simultaneous winners writing socket_fd,
	 * winner_addr, and winner_addr_len
	 */
	pthread_mutex_t winner_mutex;

	/**
	 * Protects against mutating while iterating the candidate list
	 */
	pthread_mutex_t candidate_mutex;

	/**
	 * Event that signals completion of the race, either via winner or
	 * error
	 */
	os_event_t *race_completed_event;

	/**
	 * Addresses gathered by getaddrinfo
	 */
	struct addrinfo *addresses;

	/**
	 * Domain name resolution time, in nanoseconds (0 until resolution
	 * success)
	 */
	uint64_t name_resolution_time_ns;

	/**
	 * Connection time, in nanoseconds
	 */
	uint64_t connection_time_start;

	uint64_t connection_time_end;

	/**
	 * Indicates whether we are in the initial phase of dispatching
	 * connections, or if we are waiting for things to connect or time out.
	 */
	volatile bool is_starting;
};

struct happy_connect_worker_args {
	SOCKET sockfd;
	struct happy_eyeballs_ctx *context;
	struct happy_eyeballs_candidate *candidate;
	struct addrinfo *address;
};

static int check_comodo(struct happy_eyeballs_ctx *context)
{
#ifdef _WIN32
	/* COMODO security software sandbox blocks all DNS by returning "host
	 * not found" */
	HOSTENT *h = gethostbyname("localhost");
	if (!h && GetLastError() == WSAHOST_NOT_FOUND) {
		context->error = WSAHOST_NOT_FOUND;
		context->error_message =
			"happy-eyeballs: Connection test failed. "
			"This error is likely caused by Comodo Internet "
			"Security running OBS in sandbox mode. Please add "
			"OBS to the Comodo automatic sandbox exclusion list, "
			"restart OBS and try again (11001).";
		return STATUS_FAILURE;
	}
#else
	(void)context;
#endif
	return STATUS_SUCCESS;
}

static int build_addr_list(const char *hostname, int port,
			   struct happy_eyeballs_ctx *context)
{
	struct addrinfo hints = {0};

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_ADDRCONFIG;

	if (context->bind_addr_len == sizeof(struct sockaddr_in))
		hints.ai_family = AF_INET;
	else if (context->bind_addr_len == sizeof(struct sockaddr_in6))
		hints.ai_family = AF_INET6;

	struct dstr port_str;
	dstr_init(&port_str);
	dstr_printf(&port_str, "%d", port);

	uint64_t start_time = os_gettime_ns();
	int err = getaddrinfo(hostname, port_str.array, &hints,
			      &context->addresses);
	context->name_resolution_time_ns = os_gettime_ns() - start_time;
	dstr_free(&port_str);
	if (err) {
		context->error = GetSockError();
		context->error_message = strerror(GetSockError());
		return STATUS_FAILURE;
	}

	/* Reorder addresses interleaving address family */
	struct addrinfo *prev = context->addresses;
	struct addrinfo *cur = prev->ai_next;

	while (cur) {
		if (prev->ai_family == cur->ai_family &&
		    (cur->ai_family == AF_INET || cur->ai_family == AF_INET6)) {
			/* If the current protocol family matches the previous
			 * one, look for the next instance of the other kind */
			const int target_family =
				prev->ai_family == AF_INET ? AF_INET6 : AF_INET;
			struct addrinfo *it = cur->ai_next;
			struct addrinfo *prev_it = cur;

			while (it) {
				if (it->ai_family == target_family)
					break;
				prev_it = it;
				it = it->ai_next;
			}

			if (!it) {
				/* we're at the end and haven't found the other
				 * kind, exit the loop early. */
				break;
			}

			prev->ai_next = it;
			prev_it->ai_next = it->ai_next;
			it->ai_next = cur;
		}
		prev = cur;
		cur = cur->ai_next;
	}

	return STATUS_SUCCESS;
}

static void signal_end(struct happy_eyeballs_ctx *context)
{
	if (os_event_try(context->race_completed_event) != EAGAIN)
		return;

	context->connection_time_end = os_gettime_ns();
	os_event_signal(context->race_completed_event);
}

/**
 * Takes the errors that may have been generated by workers, finds the most
 * common one, and sets that to be the overall context error.
 */
static int coalesce_errors(struct happy_eyeballs_ctx *context)
{
	/* Don't coalesce errors while starting */
	if (os_atomic_load_bool(&context->is_starting))
		return STATUS_FAILURE;

	/* Don't coalesce errors if we've already completed */
	if (os_event_try(context->race_completed_event) != EAGAIN)
		return STATUS_FAILURE;

	/* We'll use the mode of the errors for now. */
	struct mode {
		int error;
		int count;
	};
	DARRAY(struct mode) modes = {0};
	da_init(modes);

	/* Gather all the errors into counts for each error */
	pthread_mutex_lock(&context->candidate_mutex);
	for (size_t i = 0; i < context->candidates.num; i++) {
		int err = context->candidates.array[i].error;
		struct mode *mode = NULL;

		/* If the error is 0, just move on. */
		if (err == 0) {
			continue;
		}

		/* Find an existing index containing this error if it exists */
		for (size_t j = 0; j < modes.num && mode == NULL; j++) {
			if (modes.array[j].error == err)
				mode = &modes.array[j];
		}

		/* Existing index doesn't exist, take the next available. */
		if (mode == NULL) {
			mode = da_push_back_new(modes);
		}

		/* Note the error code and increment the count. */
		mode->error = err;
		mode->count++;
	}
	pthread_mutex_unlock(&context->candidate_mutex);

	int max_count = 0;
	int max_value = 0;

	/* Find the error with the most occurrences. */
	for (size_t i = 0; i < modes.num; i++) {
		if (max_count < modes.array[i].count) {
			max_value = modes.array[i].error;
			max_count = modes.array[i].count;
		}
	}
	da_free(modes);

	/* Set the error */
	context->error = max_value;
	context->error_message = strerror(context->error);

	return STATUS_SUCCESS;
}

static void *happy_connect_worker(void *arg)
{
	struct happy_connect_worker_args *args =
		(struct happy_connect_worker_args *)arg;
	struct happy_eyeballs_ctx *context = args->context;

	if (args->sockfd == INVALID_SOCKET) {
		goto success;
	}
	if (os_event_try(args->context->race_completed_event) == 0) {
		/* Already lost, don't bother */
		goto success;
	}

#if !defined(_WIN32) && defined(SO_NOSIGPIPE)
	setsockopt(args->sockfd, SOL_SOCKET, SO_NOSIGPIPE, &(int){1},
		   sizeof(int));
#endif
	if (context->bind_addr.ss_family != 0 &&
	    bind(args->sockfd, (const struct sockaddr *)&context->bind_addr,
		 context->bind_addr_len) < 0) {
		goto failure;
	}

	if (connect(args->sockfd, args->address->ai_addr,
		    (int)args->address->ai_addrlen) == 0) {

		/* success, check if we're the winner. */
		pthread_mutex_lock(&context->winner_mutex);
		os_event_signal(args->candidate->socket_completed_event);

		if (os_event_try(context->race_completed_event) == EAGAIN) {
			/* We are the winner. */
			context->socket_fd = args->sockfd;
			memcpy(&context->winner_addr, args->address->ai_addr,
			       args->address->ai_addrlen);
			context->winner_addr_len =
				(socklen_t)args->address->ai_addrlen;
			signal_end(context);
		}

		pthread_mutex_unlock(&context->winner_mutex);
		goto success;
	}

failure:
	/* failure, note down the error */
	args->candidate->error = GetSockError();
	pthread_mutex_lock(&context->winner_mutex);
	os_event_signal(args->candidate->socket_completed_event);

	/* connection candidates may still be getting dispatched, treat as if
	 * there's an active candidate */
	bool active = os_atomic_load_bool(&context->is_starting);

	/* check if we are the last worker running. If so, we'll set the error
	 * status and signal the completion event. */
	pthread_mutex_lock(&context->candidate_mutex);
	for (size_t i = 0; i < context->candidates.num && !active; i++) {
		active = os_event_try(context->candidates.array[i]
					      .socket_completed_event) ==
			 EAGAIN;
	}

	pthread_mutex_unlock(&context->candidate_mutex);
	pthread_mutex_unlock(&context->winner_mutex);

	if (active || context->error != 0) {
		/* we're not last or there is already an error on the context,
		 * let's exit. */
		goto success;
	}

	/* Ok, we are last. Coalesce errors and signal completion. */
	if (coalesce_errors(context) == STATUS_SUCCESS)
		signal_end(context);

success:
	free(args);
	return NULL;
}

static int launch_worker(struct happy_eyeballs_ctx *context,
			 struct addrinfo *addr)
{
#ifdef _WIN32
	SOCKET fd = WSASocket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP, NULL,
			      0, WSA_FLAG_OVERLAPPED);
#else
	SOCKET fd = socket(addr->ai_family, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (fd == INVALID_SOCKET) {
		context->error = GetSockError();
		return STATUS_FAILURE;
	}

	pthread_mutex_lock(&context->candidate_mutex);

	struct happy_eyeballs_candidate *candidate =
		da_push_back_new(context->candidates);
	candidate->sockfd = fd;
	struct happy_connect_worker_args *args =
		(struct happy_connect_worker_args *)malloc(
			sizeof(struct happy_connect_worker_args));
	if (args == NULL) {
		context->error = ENOMEM;
		context->error_message = "happy-eyeballs: Failed to allocate "
					 "memory for worker context";
		pthread_mutex_unlock(&context->candidate_mutex);
		return STATUS_FAILURE;
	}

	int result = os_event_init(&candidate->socket_completed_event,
				   OS_EVENT_TYPE_MANUAL);
	if (result != 0) {
		/* failure to create the socket completed event */
		context->error = result;
		context->error_message = "happy-eyeballs: Failed to initialize "
					 "socket completed event";
		free(args);
		pthread_mutex_unlock(&context->candidate_mutex);
		return STATUS_FAILURE;
	}

	args->sockfd = fd;
	args->address = addr;
	args->context = context;
	args->candidate = candidate;
	pthread_mutex_unlock(&context->candidate_mutex);

	/* Launch worker thread; `args` ownership is passed to this thread */
	result = pthread_create(&candidate->thread, NULL, happy_connect_worker,
				args);
	if (result != 0) {
		/* failure to start the worker thread */
		context->error = result;
		context->error_message = strerror(result);
		os_event_destroy(candidate->socket_completed_event);
		free(args);
		pthread_mutex_lock(&context->candidate_mutex);
		da_erase_item(context->candidates, candidate);
		pthread_mutex_unlock(&context->candidate_mutex);
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* Public functions                                                          */

int happy_eyeballs_create(struct happy_eyeballs_ctx **context)
{
	if (context == NULL)
		return STATUS_INVALID_ARGUMENT;

	struct happy_eyeballs_ctx *ctx = (struct happy_eyeballs_ctx *)malloc(
		sizeof(struct happy_eyeballs_ctx));

	if (ctx == NULL)
		return -ENOMEM;

	memset(ctx, 0, sizeof(struct happy_eyeballs_ctx));

	ctx->socket_fd = INVALID_SOCKET;
	da_init(ctx->candidates);
	da_reserve(ctx->candidates, HAPPY_EYEBALLS_MAX_ATTEMPTS);

	/* race_completed_event will be signalled when there is a winner or all
	 * attempts have failed */
	int result =
		os_event_init(&ctx->race_completed_event, OS_EVENT_TYPE_MANUAL);

	/* this mutex is used to avoid the situation where we may have two
	 * simultaneous winners and inconsistent values set to the context. */
	if (result == 0)
		result = pthread_mutex_init(&ctx->winner_mutex, NULL);
	bool have_winner_mutex = result == 0;

	/* this mutex is used to avoid the situation where we may be mutating
	 * the candidate array while iterating it. */
	if (result == 0)
		result = pthread_mutex_init(&ctx->candidate_mutex, NULL);
	bool have_candidate_mutex = result == 0;

	if (result == 0) {
		*context = ctx;
		return STATUS_SUCCESS;
	}

	/* Failure, cleanup */
	if (ctx->race_completed_event)
		os_event_destroy((*context)->race_completed_event);

	if (have_winner_mutex)
		pthread_mutex_destroy(&(*context)->winner_mutex);

	if (have_candidate_mutex)
		pthread_mutex_destroy(&(*context)->candidate_mutex);

	free(ctx);
	*context = NULL;

	/* Error codes from pthread_mutex_init are positive, os_event_init is
	 * negative. We have promised to return negative error codes, so we are
	 * making them all negative here. */
	return -abs(result);
}

int happy_eyeballs_connect(struct happy_eyeballs_ctx *context,
			   const char *hostname, int port)
{
	if (hostname == NULL || context == NULL || port == 0)
		return STATUS_INVALID_ARGUMENT;

	int result = check_comodo(context);
	if (result == STATUS_SUCCESS)
		result = build_addr_list(hostname, port, context);
	if (result != STATUS_SUCCESS)
		return result;

	context->connection_time_start = os_gettime_ns();
	int prev_family = 0;
	struct addrinfo *next = context->addresses;
	os_atomic_store_bool(&context->is_starting, true);

	/* Exit the loop under the following conditions:
	 *   1. We have reached the maximum allowed number of attempts
	 *   2. There are no more candidates in the linked list (ie. `next` is
	 *      null)
	 *   3. We have seen two candidates of the same family in a row, stop
	 *      happy eyeballs and let the previous attempt go it alone. */
	for (int i = 0; i < HAPPY_EYEBALLS_MAX_ATTEMPTS && next &&
			next->ai_family != prev_family;
	     i++) {
		/* Launch a worker thread for this address */
		int result = launch_worker(context, next);
		if (result != STATUS_SUCCESS)
			return result;

		/* Wait until the delay between attempts times out or we get
		 * signalled... */
		result = os_event_timedwait(context->race_completed_event,
					    HAPPY_EYEBALLS_DELAY_MS);
		if (result == 0) {
			/* signalled. Break out of the loop. */
			break;
		} else if (result == EINVAL) {
			/* we ran into an error. */
			context->error = result;
			context->error_message = "happy-eyeballs: Encountered "
						 "error waiting for "
						 "race_completed_event";
			return STATUS_FAILURE;
		}

		/* timer timed out, move to the next candidate... */
		prev_family = next->ai_family;
		next = next->ai_next;
	}

	os_atomic_store_bool(&context->is_starting, false);

	if (happy_eyeballs_try(context) == EAGAIN) {
		int active_count = 0;
		for (size_t i = 0; i < context->candidates.num; i++)
			active_count +=
				(os_event_try(
					 context->candidates.array[i]
						 .socket_completed_event) ==
				 EAGAIN);
		if (active_count == 0 &&
		    coalesce_errors(context) == STATUS_SUCCESS)
			signal_end(context);
	}

	return happy_eyeballs_try(context);
}

int happy_eyeballs_try(struct happy_eyeballs_ctx *context)
{
	int status = os_event_try(context->race_completed_event);

	if (context->error != 0)
		return STATUS_FAILURE;

	if (status != 0 && status != EAGAIN) {
		context->error = status;
		context->error_message = strerror(status);
		return STATUS_FAILURE;
	}
	return status;
}

int happy_eyeballs_timedwait_default(struct happy_eyeballs_ctx *context)
{
	return happy_eyeballs_timedwait(context,
					HAPPY_EYEBALLS_CONNECTION_TIMEOUT_MS);
}

int happy_eyeballs_timedwait(struct happy_eyeballs_ctx *context,
			     unsigned long time_in_millis)
{
	if (context == NULL)
		return STATUS_INVALID_ARGUMENT;

	int status = os_event_timedwait(context->race_completed_event,
					time_in_millis);

	if (context->error != 0)
		return STATUS_FAILURE;

	if (status != 0 && status != ETIMEDOUT) {
		context->error = status;
		return STATUS_FAILURE;
	}
	return status;
}

static void *destroy_thread(void *param)
{
	struct happy_eyeballs_ctx *context = param;

	os_set_thread_name("happy-eyeballs destroy thread");
#ifdef _WIN32
#define SHUT_RDWR SD_BOTH
#else
#define closesocket(s) close(s)
#endif
	/* We do not need to lock context->candidate_mutex here because the
	 * candidate array is _only_ mutated in happy_eyeballs_connect. Using
	 * this function at the same time as connect is a programmer error.
	 * Locking it here will cause a deadlock with join.
	 *
	 * Shutdown non-winning sockets (but keep the socket alive so that
	 * things error out in threads) */
	for (size_t i = 0; i < context->candidates.num; i++) {
		if (context->candidates.array[i].sockfd != INVALID_SOCKET &&
		    context->candidates.array[i].sockfd != context->socket_fd) {
			shutdown(context->candidates.array[i].sockfd,
				 SHUT_RDWR);
		}
	}

	/* Join threads */
	for (size_t i = 0; i < context->candidates.num; i++) {
		pthread_join(context->candidates.array[i].thread, NULL);
		os_event_destroy(
			context->candidates.array[i].socket_completed_event);
	}

	/* Close sockets */
	for (size_t i = 0; i < context->candidates.num; i++) {
		if (context->candidates.array[i].sockfd != INVALID_SOCKET &&
		    context->candidates.array[i].sockfd != context->socket_fd) {
			closesocket(context->candidates.array[i].sockfd);
		}
	}

	pthread_mutex_destroy(&context->winner_mutex);
	pthread_mutex_destroy(&context->candidate_mutex);
	os_event_destroy(context->race_completed_event);
	if (context->addresses != NULL)
		freeaddrinfo(context->addresses);

	da_free(context->candidates);
	free(context);

	return NULL;
}

int happy_eyeballs_destroy(struct happy_eyeballs_ctx *context)
{
	if (context == NULL)
		return STATUS_INVALID_ARGUMENT;

	/* The destroy happens asynchronously in another thread due to the
	 * connect() call blocking on Windows. */
	pthread_t thread;
	pthread_create(&thread, NULL, destroy_thread, context);
	pthread_detach(thread);

	return STATUS_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* Setters & Getters                                                         */

int happy_eyeballs_set_bind_addr(struct happy_eyeballs_ctx *context,
				 socklen_t addr_len,
				 struct sockaddr_storage *addr_storage)
{
	if (!context)
		return STATUS_INVALID_ARGUMENT;

	if (addr_storage && addr_len > 0) {
		memcpy(&context->bind_addr, addr_storage,
		       sizeof(struct sockaddr_storage));
		context->bind_addr_len = addr_len;
	} else {
		context->bind_addr_len = 0;
		memset(&context->bind_addr, 0, sizeof(struct sockaddr_storage));
	}
	return STATUS_SUCCESS;
}

SOCKET happy_eyeballs_get_socket_fd(const struct happy_eyeballs_ctx *context)
{
	return context ? context->socket_fd : STATUS_INVALID_ARGUMENT;
}

int happy_eyeballs_get_remote_addr(const struct happy_eyeballs_ctx *context,
				   struct sockaddr_storage *addr)
{
	if (!context || !addr)
		return STATUS_INVALID_ARGUMENT;
	if (context->winner_addr_len > 0)
		memcpy(addr, &context->winner_addr, context->winner_addr_len);
	return context->winner_addr_len;
}

int happy_eyeballs_get_error_code(const struct happy_eyeballs_ctx *context)
{
	return context ? context->error : STATUS_INVALID_ARGUMENT;
}

const char *
happy_eyeballs_get_error_message(const struct happy_eyeballs_ctx *context)
{
	return context ? context->error_message : NULL;
}

uint64_t happy_eyeballs_get_name_resolution_time_ns(
	const struct happy_eyeballs_ctx *context)
{
	return context ? context->name_resolution_time_ns : 0;
}

uint64_t
happy_eyeballs_get_connection_time_ns(const struct happy_eyeballs_ctx *context)
{
	if (!context ||
	    context->connection_time_start > context->connection_time_end)
		return 0;

	return context->connection_time_end - context->connection_time_start;
}
