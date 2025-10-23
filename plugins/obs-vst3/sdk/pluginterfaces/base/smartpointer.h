//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/smartpointer.h
// Created by  : Steinberg, 01/2004
// Description : Basic Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/fplatform.h"
#if SMTG_CPP11_STDLIBSUPPORT
#include <utility>
#endif

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
// IPtr
//------------------------------------------------------------------------
/**	IPtr - Smart pointer template class.
 \ingroup pluginBase

 - can be used as an I* pointer
 - handles refCount of the interface
 - Usage example:
 \code
IPtr<IPath> path (sharedPath);
if (path)
	path->ascend ();
 \endcode
 */
template <class I>
class IPtr
{
public:
//------------------------------------------------------------------------
	inline IPtr (I* ptr, bool addRef = true);
	inline IPtr (const IPtr&);

	template <class T>
	inline IPtr (const IPtr<T>& other) : ptr (other.get ())
	{
		if (ptr)
			ptr->addRef ();
	}

	inline IPtr ();
	inline ~IPtr ();

	inline I* operator= (I* ptr);

	inline IPtr& operator= (const IPtr& other);

	template <class T>
	inline IPtr& operator= (const IPtr<T>& other)
	{
		operator= (other.get ());
		return *this;
	}

	inline operator I* () const { return ptr; } // act as I*
	inline I* operator-> () const { return ptr; } // act as I*

	inline I* get () const { return ptr; }

#if SMTG_CPP11_STDLIBSUPPORT
	inline IPtr (IPtr<I>&& movePtr) SMTG_NOEXCEPT : ptr (movePtr.take ()) { }
	
	template <typename T>
	inline IPtr (IPtr<T>&& movePtr) SMTG_NOEXCEPT : ptr (movePtr.take ()) {  }

	inline IPtr& operator= (IPtr<I>&& movePtr) SMTG_NOEXCEPT
	{
		if (ptr)
			ptr->release ();

		ptr = movePtr.take ();
		return *this;
	}
	
	template <typename T>
	inline IPtr& operator= (IPtr<T>&& movePtr) 
	{
		if (ptr)
			ptr->release ();
		
		ptr = movePtr.take ();
		return *this;
	}
#endif

	inline void reset (I* obj = nullptr) 
	{
		if (ptr)
			ptr->release();
		ptr = obj;
	}

	I* take () SMTG_NOEXCEPT 
	{
		I* out = ptr; 
		ptr = nullptr; 
		return out;
	}

	template <typename T>
	static IPtr<T> adopt (T* obj) SMTG_NOEXCEPT { return IPtr<T> (obj, false); }

//------------------------------------------------------------------------
protected:
	I* ptr;
};

//------------------------------------------------------------------------
template <class I>
inline IPtr<I>::IPtr (I* _ptr, bool addRef) : ptr (_ptr)
{
	if (ptr && addRef)
		ptr->addRef ();
}

//------------------------------------------------------------------------
template <class I>
inline IPtr<I>::IPtr (const IPtr<I>& other) : ptr (other.ptr)
{
	if (ptr)
		ptr->addRef ();
}

//------------------------------------------------------------------------
template <class I>
inline IPtr<I>::IPtr () : ptr (0)
{
}

//------------------------------------------------------------------------
template <class I>
inline IPtr<I>::~IPtr ()
{
	if (ptr) 
	{
		ptr->release ();
		ptr = nullptr;  //TODO_CORE: how much does this cost? is this something hiding for us?
	}
}

//------------------------------------------------------------------------
template <class I>
inline I* IPtr<I>::operator= (I* _ptr)
{
	if (_ptr != ptr)
	{
		if (ptr)
			ptr->release ();
		ptr = _ptr;
		if (ptr)
			ptr->addRef ();
	}
	return ptr;
}

//------------------------------------------------------------------------
template <class I>
inline IPtr<I>& IPtr<I>::operator= (const IPtr<I>& _ptr)
{
	operator= (_ptr.ptr);
	return *this;
}

//------------------------------------------------------------------------
/** OPtr - "owning" smart pointer used for newly created FObjects.
 \ingroup pluginBase

 FUnknown implementations are supposed to have a refCount of 1 right after creation.
 So using an IPtr on newly created objects would lead to a leak.
 Instead the OPtr can be used in this case. \n
 Example:
 \code
 OPtr<IPath> path = FHostCreate (IPath, hostClasses);
 // no release is needed...
 \endcode
 The assignment operator takes ownership of a new object and releases the old.
 So its safe to write:
 \code
 OPtr<IPath> path = FHostCreate (IPath, hostClasses);
 path = FHostCreate (IPath, hostClasses);
 path = 0;
 \endcode
 This is the difference to using an IPtr with addRef=false.
 \code
 // DONT DO THIS:
 IPtr<IPath> path (FHostCreate (IPath, hostClasses), false);
 path = FHostCreate (IPath, hostClasses);
 path = 0;
 \endcode
 This will lead to a leak!
 */
template <class I>
class OPtr : public IPtr<I>
{
public:
//------------------------------------------------------------------------
	inline OPtr (I* p) : IPtr<I> (p, false) {}
	inline OPtr (const IPtr<I>& p) : IPtr<I> (p) {}
	inline OPtr (const OPtr<I>& p) : IPtr<I> (p) {}
	inline OPtr () {}
	inline I* operator= (I* _ptr)
	{
		if (_ptr != this->ptr)
		{
			if (this->ptr)
				this->ptr->release ();
			this->ptr = _ptr;
		}
		return this->ptr;
	}
};

//------------------------------------------------------------------------
/** Assigning newly created object to an IPtr.
 Example:
 \code
 IPtr<IPath> path = owned (FHostCreate (IPath, hostClasses));
 \endcode
 which is a slightly shorter form of writing:
 \code
 IPtr<IPath> path = OPtr<IPath> (FHostCreate (IPath, hostClasses));
 \endcode
 */
template <class I>
IPtr<I> owned (I* p)
{
	return IPtr<I> (p, false);
}

/** Assigning shared object to an IPtr.
 Example:
 \code
 IPtr<IPath> path = shared (iface.getXY ());
 \endcode
 */
template <class I>
IPtr<I> shared (I* p)
{
	return IPtr<I> (p, true);
}

#if SMTG_CPP11_STDLIBSUPPORT
//------------------------------------------------------------------------
// Ownership functionality
//------------------------------------------------------------------------
namespace SKI {
namespace Detail {
struct Adopt;
} // Detail

/** Strong typedef for shared reference counted objects. 
  *	Use SKI::adopt to unwrap the provided object.
  * @tparam T Referenced counted type.
  */
template <typename T>
class Shared
{
	friend struct Detail::Adopt;
	T* obj = nullptr;
};

/** Strong typedef for transferring the ownership of reference counted objects. 
  *	Use SKI::adopt to unwrap the provided object. 
  * After calling adopt the reference in this object is null.
  * @tparam T Referenced counted type.
  */
template <typename T>
class Owned
{
	friend struct Detail::Adopt;
	T* obj = nullptr;
};

/** Strong typedef for using reference counted objects. 
  *	Use SKI::adopt to unwrap the provided object. 
  * After calling adopt the reference in this object is null.
  * @tparam T Referenced counted type.
  */
template <typename T>
class Used
{
	friend struct Detail::Adopt;
	T* obj = nullptr;
};
	
namespace Detail {

struct Adopt 
{
	template <typename T>
	static IPtr<T> adopt (Shared<T>& ref) 
	{ 
		using Steinberg::shared;
		return shared (ref.obj); 
	}

	template <typename T>
	static IPtr<T> adopt (Owned<T>& ref) 
	{ 
		using Steinberg::owned;
		IPtr<T> out = owned (ref.obj);
		ref.obj = nullptr;
		return out;
	}

	template <typename T>
	static T* adopt (Used<T>& ref)
	{
		return ref.obj;
	}

	template <template <typename> class OwnerType, typename T>
	static OwnerType<T> toOwnerType (T* obj) 
	{ 
		OwnerType<T> out;
		out.obj = obj;
		return out; 
	}
};

} // Detail

/** Common function to adopt referenced counted object. 
  *	@tparam T			Referenced counted type.
  * @param ref			The reference to be adopted in a smart pointer.
  */
template <typename T>
IPtr<T> adopt (Shared<T>& ref) { return Detail::Adopt::adopt (ref); }

template <typename T>
IPtr<T> adopt (Shared<T>&& ref) { return Detail::Adopt::adopt (ref); }

/** Common function to adopt referenced counted object. 
  *	@tparam T			Referenced counted type.
  * @param ref			The reference to be adopted in a smart pointer.
  */
template <typename T>
IPtr<T> adopt (Owned<T>& ref) { return Detail::Adopt::adopt (ref); }

template <typename T>
IPtr<T> adopt (Owned<T>&& ref) { return Detail::Adopt::adopt (ref); }

/** Common function to adopt referenced counted object. 
  *	@tparam T			Referenced counted type.
  * @param ref			The reference to be adopted in a smart pointer.
  */
template <typename T>
T* adopt (Used<T>& ref) { return Detail::Adopt::adopt (ref); }

template <typename T>
T* adopt (Used<T>&& ref) { return Detail::Adopt::adopt (ref); }

/** Common function to wrap owned instances. */
template <typename T>
Owned<T> toOwned (T* obj) { return Detail::Adopt::toOwnerType<Owned> (obj); }

/** Common function to wrap shared instances. */
template <typename T>
Shared<T> toShared (T* obj) { return Detail::Adopt::toOwnerType<Shared> (obj); }

/** Common function to wrap used instances. */
template <typename T>
Used<T> toUsed (T* obj) { return Detail::Adopt::toOwnerType<Used> (obj); }

//------------------------------------------------------------------------
} // SKI
#endif

//------------------------------------------------------------------------
} // namespace Steinberg
