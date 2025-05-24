#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Wincrypt.h>

/* ------------------------------------------------------------------------ */

template<typename T, void freefunc(T)> class CustomHandle {
	T handle;

public:
	inline CustomHandle() : handle(0) {}
	inline CustomHandle(T in) : handle(in) {}
	inline ~CustomHandle()
	{
		if (handle)
			freefunc(handle);
	}

	inline T *operator&() { return &handle; }
	inline operator T() const { return handle; }
	inline T get() const { return handle; }

	inline CustomHandle<T, freefunc> &operator=(T in)
	{
		if (handle)
			freefunc(handle);
		handle = in;
		return *this;
	}

	inline bool operator!() const { return !handle; }
};

void FreeProvider(HCRYPTPROV prov);

using CryptProvider = CustomHandle<HCRYPTPROV, FreeProvider>;
