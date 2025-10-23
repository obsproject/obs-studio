//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/utility/rttransfer.h
// Created by  : Steinberg, 04/2021
// Description : Realtime Object Transfer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Transfer objects from a non realtime thread to a realtime one
 *
 *	You have to use it from two threads, the realtime context thread where you are not allowed to
 *	block and a non realtime thread from where the object is coming.
 *
 *	It's guaranteed that the function you should only call in the realtime context is wait free and
 *	does not do any allocations or deallocations
 *
 */
template <typename ObjectT, typename Deleter = std::default_delete<ObjectT>>
struct RTTransferT
{
	using ObjectType = ObjectT;
	using ObjectTypePtr = std::unique_ptr<ObjectType, Deleter>;

	RTTransferT () { assert (storage[0].is_lock_free ()); }
	~RTTransferT () noexcept { clear_ui (); }

	/** Access the transfer object.
	 *
	 * 	If there's a new object, the proc is called with the new object. The object is only valid
	 *	inside the proc.
	 *
	 *	To be called from the realtime context.
	 */
	template <typename Proc>
	void accessTransferObject_rt (Proc proc) noexcept
	{
		ObjectType* newObject {nullptr};
		ObjectType* currentObject = storage[0].load ();
		if (currentObject && storage[0].compare_exchange_strong (currentObject, newObject))
		{
			proc (*currentObject);
			ObjectType* transitObj = storage[1].load ();
			if (storage[1].compare_exchange_strong (transitObj, currentObject) == false)
			{
				assert (false);
			}
			ObjectType* oldObject = storage[2].load ();
			if (storage[2].compare_exchange_strong (oldObject, transitObj) == false)
			{
				assert (false);
			}
		}
	}

	/** Transfer an object to the realtime context.
	 *
	 * 	The ownership of newObject is transfered to this object and the Deleter is used to free
	 *	the memory of it afterwards.
	 *
	 *	If there's already an object in transfer the previous object will be deallocated and
	 *	replaced with the new one without passing to the realtime context.
	 *
	 *	To be called from the non realtime context.
	 */
	void transferObject_ui (ObjectTypePtr&& newObjectPtr)
	{
		ObjectType* newObject = newObjectPtr.release ();
		clear_ui ();
		while (true)
		{
			ObjectType* currentObject = storage[0].load ();
			if (storage[0].compare_exchange_strong (currentObject, newObject))
			{
				deallocate (currentObject);
				break;
			}
		}
	}

	/** Clear the transfer.
	 *
	 *	To be called from the non realtime context.
	 */
	void clear_ui ()
	{
		clearStorage (storage[0]);
		clearStorage (storage[1]);
		clearStorage (storage[2]);
	}

private:
	using AtomicObjectPtr = std::atomic<ObjectType*>;

	void clearStorage (AtomicObjectPtr& atomObj)
	{
		ObjectType* newObject = nullptr;
		while (ObjectType* current = atomObj.load ())
		{
			if (atomObj.compare_exchange_strong (current, newObject))
			{
				deallocate (current);
				break;
			}
		}
	}

	void deallocate (ObjectType* object)
	{
		if (object)
		{
			Deleter d;
			d (object);
		}
	}

	std::array<AtomicObjectPtr, 3> storage {};
};

//------------------------------------------------------------------------
} // Vst
} // Steinberg
