/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
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

#include "pipe.h"

bool ipc_pipe_server_start(ipc_pipe_server_t *pipe, const char *name, ipc_pipe_read_t read_callback, void *param)
{
	(void)pipe;
	(void)name;
	(void)read_callback;
	(void)param;
	return false;
}

void ipc_pipe_server_free(ipc_pipe_server_t *pipe)
{
	(void)pipe;
}

bool ipc_pipe_client_open(ipc_pipe_client_t *pipe, const char *name)
{
	(void)pipe;
	(void)name;
	return false;
}

void ipc_pipe_client_free(ipc_pipe_client_t *pipe)
{
	(void)pipe;
}

bool ipc_pipe_client_write(ipc_pipe_client_t *pipe, const void *data, size_t size)
{
	(void)pipe;
	(void)data;
	(void)size;
	return false;
}

bool ipc_pipe_client_valid(ipc_pipe_client_t *pipe)
{
	(void)pipe;
	return false;
}
