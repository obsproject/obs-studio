/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

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

	inline    operator T*() const      {return ptr;}
	inline T *operator->() const       {return ptr;}

	inline bool operator==(T *p) const {return ptr == p;}
	inline bool operator!=(T *p) const {return ptr != p;}

	inline bool operator!() const      {return !ptr;}
};
