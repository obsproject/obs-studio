//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/ringbuffer.h
// Created by  : Steinberg, 01/2018
// Description : Simple RingBuffer with one reader and one writer thread
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"

#include <atomic>
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {
namespace OneReaderOneWriter {

//------------------------------------------------------------------------
/** Ringbuffer
 *
 *	A ringbuffer supporting one reader and one writer thread
 */
template <typename ItemT>
class RingBuffer
{
private:
	using AtomicUInt32 = std::atomic<uint32>;
	using Index = uint32;
	using StorageT = std::vector<ItemT>;

	StorageT buffer;
	Index readPosition {0u};
	Index writePosition {0u};
	AtomicUInt32 elementCount {0u};

public:
	/** Default constructor
	 *
	 *	@param initialNumberOfItems initial ring buffer size
	 */
	RingBuffer (size_t initialNumberOfItems = 0) noexcept
	{
		if (initialNumberOfItems)
			resize (initialNumberOfItems);
	}

	/** size
	 *
	 *	@return number of elements the buffer can hold
	 */
	size_t size () const noexcept { return buffer.size (); }

	/** resize
	 *
	 *	note that you have to make sure that no other thread is reading or writing while calling
	 *	this method
	 *	@param newNumberOfItems resize buffer
	 */
	void resize (size_t newNumberOfItems) noexcept { buffer.resize (newNumberOfItems); }

	/** push a new item into the ringbuffer
	 *
	 *	@param item to push
	 *	@return true on success or false if buffer is full
	 */
	bool push (ItemT&& item) noexcept
	{
		if (elementCount.load () == buffer.size ())
			return false; // full

		auto pos = writePosition;

		buffer[pos] = std::move (item);
		elementCount++;
		++pos;
		if (pos >= buffer.size ())
			pos = 0u;

		writePosition = pos;
		return true;
	}

	/** push a new item into the ringbuffer
	 *
	 *	@param item to push
	 *	@return true on success or false if buffer is full
	 */
	bool push (const ItemT& item) noexcept
	{
		if (elementCount.load () == buffer.size ())
			return false; // full

		auto pos = writePosition;

		buffer[pos] = item;
		elementCount++;
		++pos;
		if (pos >= buffer.size ())
			pos = 0u;

		writePosition = pos;
		return true;
	}

	/** push multiple items at once into the ringbuffer
	 *
	 *	if there are insufficient free slots in the ring buffer, no item will be pushed.
	 *	furthermore, it is guaranteed that the newly added items can only be popped from the buffer
	 *	after all items have been added.
	 *
	 *	@param items list of items to push
	 *	@return true on success or false if there's not enough free space in the buffer
	 */
	bool push (const std::initializer_list<ItemT>& items) noexcept
	{
		if (items.size () == 0)
			return true;
		uint32 elementsPushed = 0u;
		auto freeElementCount = buffer.size () - elementCount.load ();
		if (freeElementCount < items.size ())
			return false;
		auto pos = writePosition;
		for (const auto& el : items)
		{
			buffer[pos] = el;
			++pos;
			if (pos >= buffer.size ())
				pos = 0u;
			++elementsPushed;
		}
		while (true)
		{
			uint32 expected = elementCount.load ();
			uint32 desired = expected + elementsPushed;
			if (elementCount.compare_exchange_strong (expected, desired))
				break;
		}
		writePosition = pos;
		return elementsPushed;
	}

	/** pop an item out of the ringbuffer
	 *
	 *	@param item
	 *	@return true on success or false if buffer is empty
	 */
	bool pop (ItemT& item) noexcept
	{
		if (elementCount.load () == 0)
			return false; // empty

		auto pos = readPosition;
		item = std::move (buffer[pos]);
		elementCount--;
		++pos;
		if (pos >= buffer.size ())
			pos = 0;
		readPosition = pos;
		return true;
	}
};

//------------------------------------------------------------------------
} // OneReaderOneWriter
} // Steinberg
