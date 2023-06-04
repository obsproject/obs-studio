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

#pragma once

class WinHandle {
	HANDLE handle = INVALID_HANDLE_VALUE;

	inline void Clear()
	{
		if (handle && handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
	}

public:
	inline WinHandle() {}
	inline WinHandle(HANDLE handle_) : handle(handle_) {}
	inline ~WinHandle() { Clear(); }

	inline operator HANDLE() const { return handle; }

	inline WinHandle &operator=(HANDLE handle_)
	{
		if (handle_ != handle) {
			Clear();
			handle = handle_;
		}

		return *this;
	}

	inline HANDLE *operator&() { return &handle; }

	inline bool Valid() const
	{
		return handle && handle != INVALID_HANDLE_VALUE;
	}
};

class WinModule {
	HMODULE handle = NULL;

	inline void Clear()
	{
		if (handle)
			FreeLibrary(handle);
	}

public:
	inline WinModule() {}
	inline WinModule(HMODULE handle_) : handle(handle_) {}
	inline ~WinModule() { Clear(); }

	inline operator HMODULE() const { return handle; }

	inline WinModule &operator=(HMODULE handle_)
	{
		if (handle_ != handle) {
			Clear();
			handle = handle_;
		}

		return *this;
	}

	inline HMODULE *operator&() { return &handle; }

	inline bool Valid() const { return handle != NULL; }
};
