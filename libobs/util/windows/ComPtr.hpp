/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
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

template<typename T> class ComPtr {
	T *ptr;

	inline void Kill()
	{
		if (ptr)
			ptr->Release();
	}

	inline void Replace(T *p)
	{
		if (ptr != p) {
			if (p)   p->AddRef();
			if (ptr) ptr->Release();
			ptr = p;
		}
	}

public:
	inline ComPtr() : ptr(NULL)                  {}
	inline ComPtr(T *p) : ptr(p)                 {if (ptr) ptr->AddRef();}
	inline ComPtr(const ComPtr &c) : ptr(c.ptr)  {if (ptr) ptr->AddRef();}
	inline ComPtr(ComPtr &&c) : ptr(c.ptr)       {c.ptr = NULL;}
	inline ~ComPtr()                             {Kill();}

	inline void Clear()
	{
		if (ptr) {
			ptr->Release();
			ptr = NULL;
		}
	}

	inline ComPtr &operator=(T *p)
	{
		Replace(p);
		return *this;
	}

	inline ComPtr &operator=(const ComPtr &c)
	{
		Replace(c.ptr);
		return *this;
	}

	inline ComPtr &operator=(ComPtr &&c)
	{
		if (this != &c) {
			Kill();
			ptr = c.ptr;
			c.ptr = NULL;
		}

		return *this;
	}

	inline T **Assign()                {Clear(); return &ptr;}
	inline void Set(T *p)              {Kill(); ptr = p;}

	inline T *Get() const              {return ptr;}

	inline    operator T*() const      {return ptr;}
	inline T *operator->() const       {return ptr;}

	inline bool operator==(T *p) const {return ptr == p;}
	inline bool operator!=(T *p) const {return ptr != p;}

	inline bool operator!() const      {return !ptr;}
};
