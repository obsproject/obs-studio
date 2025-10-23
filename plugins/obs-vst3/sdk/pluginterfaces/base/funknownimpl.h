//-----------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/funknownimpl.h
// Created by  : Steinberg, 10/2021
// Description : Steinberg Module Architecture Interface Implementation Helper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------
#include "pluginterfaces/base/fplatform.h"
#include "pluginterfaces/base/funknown.h"
#include <atomic>
#include <type_traits>

//------------------------------------------------------------------------
#if !(SMTG_CPP11)
#error "C++11 is required for this header"
#endif

// clang-format off
/**
This header provides classes for working with FUnknown.

An interface which shall support Steinbergs Module Architecture should inherit from U::Unknown and provide a typedef
@c IID of type U::UID.

On OS X you can generate an U::UID and copy it to the clipboard with the following shell command:
@code
uuidgen | { read id; echo -n "using IID = U::UID<0x${id:0:8}, 0x${id:9:4}${id:14:4}, 0x${id:19:4}${id:24:4}, 0x${id:28:8}>;" ; } | pbcopy
@endcode

Example:
@code{.cpp}
struct IFoo : public U::Unknown
{
    // Use a generated random uid here.
    using IID = U::UID<0x01234567, 0x89012345, 0x67890123, 0x45678901>;

    virtual void bar () = 0;
};
@endcode

A class implementing the interface @c IFoo uses the U::Implements template to specify the
interfaces it implements. All interfaces which the class should derive from need to be listed in the
U::Directly template.

Example:
@code{.cpp}
struct FooImpl : public U::Implements<U::Directly<IFoo>>
{
    void bar () override {}
};
@endcode

To check if a class can provide a specific interface use the U::cast function.

Example:
@code{.cpp}
void test (U::Unknown* obj)
{
    if (auto foo = U::cast<IFoo> (obj))
    {
        // obj provided IFoo
    }
}
@endcode

The U::Implements class supports a second template parameter U::Indirectly for specifying
a list of interfaces which should be available via @c queryInterface but not inherited from.
This is useful if an interface extends another interface.

Example:
@code{.cpp}
struct IBar : public IFoo
{
    using IID = U::UID<0x11223344, 0x55667788, 0x99001122, 0x33445566>;

    virtual void baz () = 0;
};

struct BarImpl : public U::Implements<U::Directly<IBar>, U::Indirectly<IFoo>>
{
    void bar () override {}
    void baz () override {}
};
@endcode

In some cases a class shall be extended and an additional interface implemented.
This is possible with the U::Extends template which is a generalization of the U::Implements
template and allows specifying a base class from which should be inherited.

Example:
@code{.cpp}
struct ITest : public U::Unknown
{
    using IID = U::UID<0x99887766, 0x55443322, 0x11009988, 0x77665544>;

    virtual bool equal (int a, int b) const = 0;
};

struct TestImpl : public U::Extends<FooImpl, U::Directly<ITest>>
{
    bool equal (int a, int b) const override { return a == b; }
};
@endcode

To pass arbitrary arguments to the specified base class one can use the inherited @c Base
typedef. All arguments passed to @c Base are automatically perfectly forwarded to the base class.

In the following example the value 42 is passed to the @c AlternativeFooImpl base class:
@code{.cpp}
struct AlternativeFooImpl : public U::Implements<U::Directly<IFoo>>
{
    AlternativeFooImpl (int dummy = 0) : dummy {dummy} {}
    void bar () override {}

    int dummy;
};

struct AlternativeTestImpl : public U::Extends<AlternativeFooImpl, U::Directly<ITest>>
{
    AlternativeTestImpl () : Base {42} {}

    bool equal (int a, int b) const override { return a == b; }
};
@endcode
*/
// clang-format on

//------------------------------------------------------------------------
namespace Steinberg {
namespace FUnknownImpl {

/** Typedef to keep everything in this namespace. */
using Unknown = FUnknown;

/** A base class which hides the FUnknown::iid static var */
struct HideIIDBase : FUnknown
{
	using iid = void;
};

/** Common destroyer policy for ski object instances.*/
struct Destroyer
{
	template <typename UnknownT>
	static void destroy (UnknownT* ptr)
	{
		if (!!ptr)
			ptr->release ();
	}
};

template <typename Base, typename D, typename I>
class ImplementsImpl;

/**
 *  This class provides a compile-time uid and enables  interfaces to specify a UID as a simple
 *  typedef. This way the FUID, DECLARE_CLASS_IID and DEF_CLASS_IID code can be omitted.
 */
template <uint32 t1, uint32 t2, uint32 t3, uint32 t4>
struct UID
{
	enum : int8
	{
		l1_1 = static_cast<int8> ((t1 & 0xFF000000) >> 24),
		l1_2 = static_cast<int8> ((t1 & 0x00FF0000) >> 16),
		l1_3 = static_cast<int8> ((t1 & 0x0000FF00) >> 8),
		l1_4 = static_cast<int8> ((t1 & 0x000000FF)),
		l2_1 = static_cast<int8> ((t2 & 0xFF000000) >> 24),
		l2_2 = static_cast<int8> ((t2 & 0x00FF0000) >> 16),
		l2_3 = static_cast<int8> ((t2 & 0x0000FF00) >> 8),
		l2_4 = static_cast<int8> ((t2 & 0x000000FF)),
		l3_1 = static_cast<int8> ((t3 & 0xFF000000) >> 24),
		l3_2 = static_cast<int8> ((t3 & 0x00FF0000) >> 16),
		l3_3 = static_cast<int8> ((t3 & 0x0000FF00) >> 8),
		l3_4 = static_cast<int8> ((t3 & 0x000000FF)),
		l4_1 = static_cast<int8> ((t4 & 0xFF000000) >> 24),
		l4_2 = static_cast<int8> ((t4 & 0x00FF0000) >> 16),
		l4_3 = static_cast<int8> ((t4 & 0x0000FF00) >> 8),
		l4_4 = static_cast<int8> ((t4 & 0x000000FF))
	};

	UID () = delete;

	static const TUID& toTUID ()
	{
		// clang-format off
		static TUID uuid = {
#if COM_COMPATIBLE
			l1_4, l1_3, l1_2, l1_1,
			l2_2, l2_1, l2_4, l2_3,
#else
			l1_1, l1_2, l1_3, l1_4,
			l2_1, l2_2, l2_3, l2_4,
#endif
			l3_1, l3_2, l3_3, l3_4,
			l4_1, l4_2, l4_3, l4_4
		};
		// clang-format on
		return uuid;
	}
};

/** @return the TUID for an interface. */
template <typename T>
const TUID& getTUID ()
{
	return ::Steinberg::getTUID<T> ();
}

/**
 *  Checks if the given Unknown can provide the specified interface and returns it in an IPtr.
 *
 *  @return an IPtr pointing to an instance of the requested interface or nullptr in case the
 *          object does not provide the interface.
 */
template <typename I>
IPtr<I> cast (Unknown* u)
{
	I* out = nullptr;
	return u && u->queryInterface (getTUID<I> (), reinterpret_cast<void**> (&out)) == kResultOk ?
	           owned (out) :
	           nullptr;
}

/** Casts to Unknown* and then to the specified interface. */
template <typename I, typename S, typename T, typename U>
IPtr<I> cast (ImplementsImpl<S, T, U>* u)
{
	return cast<I> (u->unknownCast ());
}

/** Casts to Unknown* and then to the specified interface. */
template <typename I, typename T>
IPtr<I> cast (const IPtr<T>& u)
{
	return cast<I> (u.get ());
}

//------------------------------------------------------------------------
namespace Detail {

/**
 *  This struct implements reference counting for the @c U::Implements template.
 *  It also provides a @c queryInterface method stub to support @c queryInterface
 *  call made in the @c U::Implements template.
 */
struct RefCounted
{
//------------------------------------------------------------------------
	RefCounted () = default;
	RefCounted (const RefCounted&) {}
	RefCounted (RefCounted&& other) SMTG_NOEXCEPT : refCount {other.refCount.load ()} {}
	virtual ~RefCounted () = default;

	RefCounted& operator= (const RefCounted&) { return *this; }
	RefCounted& operator= (RefCounted&& other) SMTG_NOEXCEPT
	{
		refCount = other.refCount.load ();
		return *this;
	}

	uint32 PLUGIN_API addRef () { return ++refCount; }

	uint32 PLUGIN_API release ()
	{
		auto rc = --refCount;
		if (rc == 0)
		{
			destroyInstance ();
			refCount = -1000;
			delete this;
			return uint32 ();
		}
		return rc;
	}

//------------------------------------------------------------------------
private:
	virtual void destroyInstance () {}

	std::atomic<int32> refCount {1};
};

//------------------------------------------------------------------------
struct NonDestroyable
{
//------------------------------------------------------------------------
	NonDestroyable () = default;
	virtual ~NonDestroyable () = default;
	uint32 PLUGIN_API addRef () { return 1000; }
	uint32 PLUGIN_API release () { return 1000; }

private:
	virtual void destroyInstance () {}
};

//------------------------------------------------------------------------
template <typename T>
struct QueryInterfaceEnd : T
{
//------------------------------------------------------------------------
	tresult PLUGIN_API queryInterface (const TUID /*iid*/, void** obj)
	{
		*obj = nullptr;
		return kNoInterface;
	}
//------------------------------------------------------------------------
};

//------------------------------------------------------------------------
} // Detail

/**
 *  This struct is used to group a list of interfaces from which should be inherited and which
 *  should be available via the @c queryInterface method.
 */
template <typename... T>
struct Directly
{
};

/**
 *  This struct is used to group a list of interfaces from which should not be inherited but which
 *  should be available via the @c queryInterface method.
 */
template <typename... T>
struct Indirectly
{
};

template <typename Base, typename D, typename I>
class ImplementsImpl
{
	static_assert (sizeof (Base) == -1, "use U::Directly and U::Indirectly to specify interfaces");
};

template <typename Base, typename... DirectInterfaces, typename... IndirectInterfaces>
class ImplementsImpl<Base, Indirectly<IndirectInterfaces...>, Directly<DirectInterfaces...>>
{
	static_assert (sizeof (Base) == -1, "U::Indirectly only allowed after U::Directly");
};

/** This class implements the required virtual methods for the U::Unknown class. */
template <typename BaseClass, typename I, typename... DirectIFs, typename... IndirectIFs>
class ImplementsImpl<BaseClass, Directly<I, DirectIFs...>, Indirectly<IndirectIFs...>>
: public BaseClass, public I, public DirectIFs...
{
public:
//------------------------------------------------------------------------
	/**
	 *  This is a convenience typedef for the deriving class to pass arguments to the
	 *  constructor, which are in turn passed to the base class of this class.
	 */
	using Base = ImplementsImpl<BaseClass, Directly<I, DirectIFs...>, Indirectly<IndirectIFs...>>;

	template <typename... Args>
	ImplementsImpl (Args&&... args) : BaseClass {std::forward<Args> (args)...}
	{
	}

	tresult PLUGIN_API queryInterface (const TUID tuid, void** obj) override
	{
		if (!obj)
			return kInvalidArgument;

		if (queryInterfaceImpl<I, DirectIFs...> (tuid, *obj) ||
		    queryInterfaceImpl<IndirectIFs...> (tuid, *obj))
		{
			static_cast<Unknown*> (*obj)->addRef ();
			return kResultOk;
		}

		return BaseClass::queryInterface (tuid, obj);
	}

	uint32 PLUGIN_API addRef () override { return BaseClass::addRef (); }
	uint32 PLUGIN_API release () override { return BaseClass::release (); }

	Unknown* unknownCast () { return static_cast<Unknown*> (static_cast<I*> (this)); }

//------------------------------------------------------------------------
private:
	template <typename Interface>
	inline constexpr bool match (const TUID tuid) const noexcept
	{
		return reinterpret_cast<const uint64*> (tuid)[0] ==
		           reinterpret_cast<const uint64*> (getTUID<Interface> ())[0] &&
		       reinterpret_cast<const uint64*> (tuid)[1] ==
		           reinterpret_cast<const uint64*> (getTUID<Interface> ())[1];
	}

	template <int = 0>
	inline constexpr bool queryInterfaceImpl (const TUID, void*&) const noexcept
	{
		return false;
	}

	template <typename Interface, typename... RemainingInterfaces>
	inline bool queryInterfaceImpl (const TUID tuid, void*& obj) noexcept
	{
		if (match<Interface> (tuid) || match<Unknown> (tuid))
		{
			obj = static_cast<Interface*> (this->unknownCast ());
			return true;
		}

		obj = getInterface<RemainingInterfaces...> (tuid);
		return obj != nullptr;
	}

	template <int = 0>
	inline constexpr void* getInterface (const TUID) const noexcept
	{
		return nullptr;
	}

	template <typename Interface, typename... RemainingInterfaces>
	inline void* getInterface (const TUID tuid) noexcept
	{
		return match<Interface> (tuid) ? static_cast<Interface*> (this) :
		                                 getInterface<RemainingInterfaces...> (tuid);
	}
};

/** This typedef enables using a custom base class with the interface implementation. */
template <typename BaseClass, typename D, typename I = Indirectly<>>
using Extends = ImplementsImpl<BaseClass, D, I>;

/** This typedef provides the interface implementation. */
template <typename D, typename I = Indirectly<>>
using Implements = ImplementsImpl<Detail::QueryInterfaceEnd<Detail::RefCounted>, D, I>;

/** This typedef provides the interface implementation for objects which should not be destroyed via
 *	FUnknown::release (like singletons). */
template <typename D, typename I = Indirectly<>>
using ImplementsNonDestroyable =
    ImplementsImpl<Detail::QueryInterfaceEnd<Detail::NonDestroyable>, D, I>;

//------------------------------------------------------------------------
} // FUnknownImpl

//------------------------------------------------------------------------
/** Shortcut namespace for implementing FUnknown based objects. */
namespace U {

using Unknown = FUnknownImpl::HideIIDBase;
using FUnknownImpl::UID;
using FUnknownImpl::Extends;
using FUnknownImpl::Implements;
using FUnknownImpl::ImplementsNonDestroyable;
using FUnknownImpl::Directly;
using FUnknownImpl::Indirectly;
using FUnknownImpl::cast;
using FUnknownImpl::getTUID;

//------------------------------------------------------------------------
} // namespace U
} // namespace Steinberg
