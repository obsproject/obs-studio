//-----------------------------------------------------------------------------
// Project     : VST SDK
// Flags       : clang-format SMTGSequencer
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/memoryibstream.h
// Created by  : Steinberg, 12/2023
// Description :
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/base/ibstream.h"
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
class ResizableMemoryIBStream : public U::Implements<U::Directly<IBStream>>
{
public:
	inline ResizableMemoryIBStream (size_t reserve = 0);

	inline tresult PLUGIN_API read (void* buffer, int32 numBytes, int32* numBytesRead) override;
	inline tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten) override;
	inline tresult PLUGIN_API seek (int64 pos, int32 mode, int64* result) override;
	inline tresult PLUGIN_API tell (int64* pos) override;

	inline size_t getCursor () const;
	inline const void* getData () const;
	inline void rewind ();
	inline std::vector<uint8>&& take ();

private:
	std::vector<uint8> data;
	size_t cursor {0};
};

//------------------------------------------------------------------------
inline ResizableMemoryIBStream::ResizableMemoryIBStream (size_t reserve)
{
	if (reserve)
		data.reserve (reserve);
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::read (void* buffer, int32 numBytes,
                                                         int32* numBytesRead)
{
	if (numBytes < 0 || buffer == nullptr)
		return kInvalidArgument;
	auto byteCount = std::min<int64> (numBytes, data.size () - cursor);
	if (byteCount > 0)
	{
		memcpy (buffer, data.data () + cursor, byteCount);
		cursor += byteCount;
	}
	if (numBytesRead)
		*numBytesRead = static_cast<int32> (byteCount);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::write (void* buffer, int32 numBytes,
                                                          int32* numBytesWritten)
{
	if (numBytes < 0 || buffer == nullptr)
		return kInvalidArgument;
	auto requiredSize = cursor + numBytes;
	if (requiredSize >= data.capacity ())
	{
		auto mod = (requiredSize % 1024);
		if (mod)
		{
			auto reserve = requiredSize + (1024 - mod);
			data.reserve (reserve);
		}
	}
	if (data.size () < requiredSize)
		data.resize (requiredSize);
	memcpy (data.data () + cursor, buffer, numBytes);
	cursor += numBytes;
	if (numBytesWritten)
		*numBytesWritten = numBytes;

	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::seek (int64 pos, int32 mode, int64* result)
{
	int64 newCursor = static_cast<int64> (cursor);
	switch (mode)
	{
		case kIBSeekSet: newCursor = pos; break;
		case kIBSeekCur: newCursor += pos; break;
		case kIBSeekEnd: newCursor = data.size () + pos; break;
		default: return kInvalidArgument;
	}
	if (newCursor < 0)
		return kInvalidArgument;
	if (newCursor > static_cast<int64> (data.size ()))
		return kInvalidArgument;
	if (result)
		*result = newCursor;
	cursor = static_cast<size_t> (newCursor);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline tresult PLUGIN_API ResizableMemoryIBStream::tell (int64* pos)
{
	if (pos == nullptr)
		return kInvalidArgument;
	*pos = static_cast<int64> (cursor);
	return kResultTrue;
}

//------------------------------------------------------------------------
inline size_t ResizableMemoryIBStream::getCursor () const
{
	return cursor;
}

//------------------------------------------------------------------------
inline const void* ResizableMemoryIBStream::getData () const
{
	return data.data ();
}

//------------------------------------------------------------------------
inline std::vector<uint8>&& ResizableMemoryIBStream::take ()
{
	cursor = 0;
	return std::move (data);
}

//------------------------------------------------------------------------
inline void ResizableMemoryIBStream::rewind ()
{
	cursor = 0;
}

//------------------------------------------------------------------------
} // Steinberg
