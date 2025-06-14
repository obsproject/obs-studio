//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fobject.h
// Created by  : Steinberg, 2008
// Description : Basic Object implementing FUnknown
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------
/** @file base/source/fobject.h
	Basic Object implementing FUnknown. */
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/iupdatehandler.h"
#include "base/source/fdebug.h" // use of NEW

#define SMTG_DEPENDENCY_COUNT	DEVELOPMENT

namespace Steinberg {

//----------------------------------

using FClassID = FIDString;

//------------------------------------------------------------------------
// Basic FObject - implements FUnknown + IDependent
//------------------------------------------------------------------------
/** Implements FUnknown and IDependent.

FObject is a polymorphic class that implements IDependent (of SKI module) and therefore derived from
FUnknown, which is the most abstract base class of all.

All COM-like virtual methods of FUnknown such as queryInterface(), addRef(), release() are
implemented here. On top of that, dependency-related methods are implemented too.

Pointer casting is done via the template methods FCast, either FObject to FObject or FUnknown to
FObject.

FObject supports a new singleton concept, therefore these objects are deleted automatically upon
program termination.

- Runtime type information: An object can be queried at runtime, of what class it is. To do this
correctly, every class must override some methods. This is simplified by using the OBJ_METHODS
macros

@see
    - FUnknown
    - IDependent
    - IUpdateHandler
*/
//------------------------------------------------------------------------
class FObject : public IDependent
{
public:
	//------------------------------------------------------------------------
	FObject () = default;													///< default constructor...
	FObject (const FObject&)												///< overloaded constructor...
		: refCount (1)
#if SMTG_DEPENDENCY_COUNT
		, dependencyCount (0) 
#endif		
	{}			
	FObject& operator= (const FObject&) { return *this; }					///< overloads operator "=" as the reference assignment
	virtual ~FObject ();													///< destructor...

	// OBJECT_METHODS
	static inline FClassID getFClassID () {return "FObject";}				///< return Class ID as an ASCII string (statically)
	virtual FClassID isA () const {return FObject::getFClassID ();}			///< a local alternative to getFClassID ()
	virtual bool isA (FClassID s) const {return isTypeOf (s, false);}		///< evaluates if the passed ID is of the FObject type
	virtual bool isTypeOf (FClassID s, bool /*askBaseClass*/ = true) const {return classIDsEqual (s, FObject::getFClassID ());}
																			///< evaluates if the passed ID is of the FObject type
	int32 getRefCount () {return refCount;}									///< returns the current interface reference count
	FUnknown* unknownCast () {return this;}									///< get FUnknown interface from object

	// FUnknown
	tresult PLUGIN_API queryInterface (const TUID _iid, void** obj) SMTG_OVERRIDE; ///< please refer to FUnknown::queryInterface ()
	uint32 PLUGIN_API addRef () SMTG_OVERRIDE;						///< please refer to FUnknown::addref ()
	uint32 PLUGIN_API release () SMTG_OVERRIDE;						///< please refer to FUnknown::release ()

	// IDependent
	void PLUGIN_API update (FUnknown* /*changedUnknown*/, int32 /*message*/) SMTG_OVERRIDE {}
																			///< empty virtual method that should be overridden by derived classes for data updates upon changes	
	// IDependency
	virtual void addDependent (IDependent* dep);							///< adds dependency to the object
	virtual void removeDependent (IDependent* dep);							///< removes dependency from the object
	virtual void changed (int32 msg = kChanged);							///< Inform all dependents, that the object has changed.
	virtual void deferUpdate (int32 msg = kChanged);						///< Similar to triggerUpdates, except only delivered in idle (usefull in collecting updates).
	virtual void updateDone (int32 /* msg */) {}							///< empty virtual method that should be overridden by derived classes
	virtual bool isEqualInstance (FUnknown* d) {return this == d;}
	
	static void setUpdateHandler (IUpdateHandler* handler) {gUpdateHandler = handler;}	///< set method for the local attribute
	static IUpdateHandler* getUpdateHandler () {return gUpdateHandler;}					///< get method for the local attribute 

	// static helper functions
	static inline bool classIDsEqual (FClassID ci1, FClassID ci2);			///< compares (evaluates) 2 class IDs
	static inline FObject* unknownToObject (FUnknown* unknown);				///< pointer conversion from FUnknown to FObject
	/** convert from FUnknown to FObject */
	template <class Class>
	static inline IPtr<Class> fromUnknown (FUnknown* unknown);

	/** Special UID that is used to cast an FUnknown pointer to a FObject */
	static const FUID iid;

//------------------------------------------------------------------------
protected:
	int32 refCount = 1;															///< COM-model local reference count
#if SMTG_DEPENDENCY_COUNT
	int16 dependencyCount = 0;
#endif
	static IUpdateHandler* gUpdateHandler;
};


//------------------------------------------------------------------------
// conversion from FUnknown to FObject subclass
//------------------------------------------------------------------------
template <class C>
inline IPtr<C> FObject::fromUnknown (FUnknown* unknown)
{
	if (unknown)
	{
		FObject* object = nullptr;
		if (unknown->queryInterface (FObject::iid, (void**)&object) == kResultTrue && object)
		{
			if (object->isTypeOf (C::getFClassID (), true))
				return IPtr<C> (static_cast<C*> (object), false);
			object->release ();
		}
	}
	return {};
}

//------------------------------------------------------------------------
inline FObject* FObject::unknownToObject (FUnknown* unknown)
{
	FObject* object = nullptr;
	if (unknown) 
	{
		unknown->queryInterface (FObject::iid, (void**)&object);
		if (object)
		{
			if (object->release () == 0)
				object = nullptr;
		}
	}
	return object;
}

//------------------------------------------------------------------------
inline bool FObject::classIDsEqual (FClassID ci1, FClassID ci2)
{
	return (ci1 && ci2) ? (strcmp (ci1, ci2) == 0) : false;
}

//-----------------------------------------------------------------------
/** FCast overload 1 - FObject to FObject */
//-----------------------------------------------------------------------
template <class C>
inline C* FCast (const FObject* object)
{
	if (object && object->isTypeOf (C::getFClassID (), true))
		return (C*) object;
	return nullptr;
}

//-----------------------------------------------------------------------
/** FCast overload 2 - FUnknown to FObject */
//-----------------------------------------------------------------------
template <class C>
inline C* FCast (FUnknown* unknown)
{
	FObject* object = FObject::unknownToObject (unknown);
	return FCast<C> (object);
}

//-----------------------------------------------------------------------
/** ICast - casting from FObject to FUnknown Interface */
//-----------------------------------------------------------------------
template<class I>
inline IPtr<I> ICast (FObject* object)
{
	return FUnknownPtr<I> (object ? object->unknownCast () : nullptr);
}

//-----------------------------------------------------------------------
/** ICast - casting from FUnknown to another FUnknown Interface */
//-----------------------------------------------------------------------
template<class I>
inline IPtr<I> ICast (FUnknown* object)
{
	return FUnknownPtr<I> (object);
}

//------------------------------------------------------------------------
template <class C>
inline C* FCastIsA (const FObject* object)
{
	if (object && object->isA (C::getFClassID ()))
		return (C*)object;
	return nullptr;
}

#ifndef SMTG_HIDE_DEPRECATED_INLINE_FUNCTIONS
//-----------------------------------------------------------------------
/** \deprecated FUCast - casting from FUnknown to Interface */
//-----------------------------------------------------------------------
template <class C>
SMTG_DEPRECATED_MSG("use ICast<>") inline C* FUCast (FObject* object)
{
	return FUnknownPtr<C> (object ? object->unknownCast () : nullptr);
}

template <class C>
SMTG_DEPRECATED_MSG("use ICast<>") inline C* FUCast (FUnknown* object)
{
	return FUnknownPtr<C> (object);
}
#endif // SMTG_HIDE_DEPRECATED_FUNCTIONS

//------------------------------------------------------------------------
/** @name Convenience methods that call release or delete respectively
	on a pointer if it is non-zero, and then set the pointer to zero.
	Note: you should prefer using IPtr or OPtr instead of these methods
	whenever possible. 
	<b>Examples:</b>
	@code
	~Foo ()
	{
		// instead of ...
		if (somePointer)
		{
			somePointer->release ();
			somePointer = 0;
		}
		// ... just being lazy I write
		SafeRelease (somePointer)
	}
	@endcode
*/
///@{
//-----------------------------------------------------------------------
template <class I>
inline void SafeRelease (I *& ptr) 
{ 
	if (ptr) 
	{
		ptr->release (); 
		ptr = 0;
	}
}

//-----------------------------------------------------------------------
template <class I>
inline void SafeRelease (IPtr<I> & ptr) 
{ 
	ptr = 0;
}


//-----------------------------------------------------------------------
template <class T>
inline void SafeDelete (T *& ptr)
{
	if (ptr) 
	{
		delete ptr;
		ptr = 0;
	}
}
///@}

//-----------------------------------------------------------------------
template <class T>
inline void AssignShared (T*& dest, T* newPtr)
{
	if (dest == newPtr)
		return;
	
	if (dest) 
		dest->release (); 
	dest = newPtr; 
	if (dest) 
		dest->addRef ();
}

//-----------------------------------------------------------------------
template <class T>
inline void AssignSharedDependent (IDependent* _this, T*& dest, T* newPtr)
{
	if (dest == newPtr)
		return;

	if (dest)
		dest->removeDependent (_this);
	AssignShared (dest, newPtr);
	if (dest)
		dest->addDependent (_this);
}

//-----------------------------------------------------------------------
template <class T>
inline void AssignSharedDependent (IDependent* _this, IPtr<T>& dest, T* newPtr)
{
	if (dest == newPtr)
		return;

	if (dest)
		dest->removeDependent (_this);
	dest = newPtr;
	if (dest)
		dest->addDependent (_this);
}
	
//-----------------------------------------------------------------------
template <class T>
inline void SafeReleaseDependent (IDependent* _this, T*& dest)
{
	if (dest)
		dest->removeDependent (_this);
	SafeRelease (dest);
}
	
//-----------------------------------------------------------------------
template <class T>
inline void SafeReleaseDependent (IDependent* _this, IPtr<T>& dest)
{
	if (dest)
		dest->removeDependent (_this);
	SafeRelease (dest);
}


//------------------------------------------------------------------------
/** Automatic creation and destruction of singleton instances. */
namespace Singleton {
	/** registers an instance (type FObject) */
	void registerInstance (FObject** o);

	/** Returns true when singleton instances were already released. */
	bool isTerminated ();

	/** lock and unlock the singleton registration for multi-threading safety */
	void lockRegister ();
	void unlockRegister ();
}

//------------------------------------------------------------------------
} // namespace Steinberg

//-----------------------------------------------------------------------
#define SINGLETON(ClassName)	\
	static ClassName* instance (bool create = true) \
	{ \
		static Steinberg::FObject* inst = nullptr; \
		if (inst == nullptr && create && Steinberg::Singleton::isTerminated () == false) \
		{	\
			Steinberg::Singleton::lockRegister (); \
			if (inst == nullptr) \
			{ \
				inst = NEW ClassName; \
				Steinberg::Singleton::registerInstance (&inst); \
			} \
			Steinberg::Singleton::unlockRegister (); \
		}	\
		return (ClassName*)inst; \
	}

//-----------------------------------------------------------------------
#define OBJ_METHODS(className, baseClass)								\
	static inline Steinberg::FClassID getFClassID () {return (#className);}		\
	virtual Steinberg::FClassID isA () const SMTG_OVERRIDE {return className::getFClassID ();}	\
	virtual bool isA (Steinberg::FClassID s) const SMTG_OVERRIDE {return isTypeOf (s, false);}	\
	virtual bool isTypeOf (Steinberg::FClassID s, bool askBaseClass = true) const SMTG_OVERRIDE	\
    {  return (FObject::classIDsEqual (s, #className) ? true : (askBaseClass ? baseClass::isTypeOf (s, true) : false)); } 

//------------------------------------------------------------------------
/** Delegate refcount functions to BaseClass.
	BaseClase must implement ref counting. 
*/
//------------------------------------------------------------------------
#define REFCOUNT_METHODS(BaseClass) \
virtual Steinberg::uint32 PLUGIN_API addRef ()SMTG_OVERRIDE{ return BaseClass::addRef (); } \
virtual Steinberg::uint32 PLUGIN_API release ()SMTG_OVERRIDE{ return BaseClass::release (); }

//------------------------------------------------------------------------
/** @name Macros to implement FUnknown::queryInterface ().

	<b>Examples:</b>
	@code
	class Foo : public FObject, public IFoo2, public IFoo3 
	{
	    ...
		DEFINE_INTERFACES
	        DEF_INTERFACE (IFoo2)
	        DEF_INTERFACE (IFoo3)
	    END_DEFINE_INTERFACES (FObject)
	    REFCOUNT_METHODS(FObject)
	    // Implement IFoo2 interface ...
	    // Implement IFoo3 interface ...
	    ...
	};
	@endcode	
*/
///@{
//------------------------------------------------------------------------
/** Start defining interfaces. */
//------------------------------------------------------------------------
#define DEFINE_INTERFACES \
Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) SMTG_OVERRIDE \
{

//------------------------------------------------------------------------
/** Add a interfaces. */
//------------------------------------------------------------------------
#define DEF_INTERFACE(InterfaceName) \
	QUERY_INTERFACE (iid, obj, Steinberg::getTUID<InterfaceName> (), InterfaceName)

//------------------------------------------------------------------------
/** End defining interfaces. */
//------------------------------------------------------------------------
#define END_DEFINE_INTERFACES(BaseClass) \
	return BaseClass::queryInterface (iid, obj); \
}
///@}

//------------------------------------------------------------------------
/** @name Convenient macros to implement Steinberg::FUnknown::queryInterface ().
	<b>Examples:</b>
	@code
	class Foo : public FObject, public IFoo2, public IFoo3 
	{
	    ...
	    DEF_INTERFACES_2(IFoo2,IFoo3,FObject)
	    REFCOUNT_METHODS(FObject)
	    ...
	};
	@endcode
*/
///@{
//------------------------------------------------------------------------
#define DEF_INTERFACES_1(InterfaceName,BaseClass) \
DEFINE_INTERFACES \
DEF_INTERFACE (InterfaceName) \
END_DEFINE_INTERFACES (BaseClass)

//------------------------------------------------------------------------
#define DEF_INTERFACES_2(InterfaceName1,InterfaceName2,BaseClass) \
DEFINE_INTERFACES \
DEF_INTERFACE (InterfaceName1) \
DEF_INTERFACE (InterfaceName2) \
END_DEFINE_INTERFACES (BaseClass)

//------------------------------------------------------------------------
#define DEF_INTERFACES_3(InterfaceName1,InterfaceName2,InterfaceName3,BaseClass) \
DEFINE_INTERFACES \
DEF_INTERFACE (InterfaceName1) \
DEF_INTERFACE (InterfaceName2) \
DEF_INTERFACE (InterfaceName3) \
END_DEFINE_INTERFACES (BaseClass)

//------------------------------------------------------------------------
#define DEF_INTERFACES_4(InterfaceName1,InterfaceName2,InterfaceName3,InterfaceName4,BaseClass) \
	DEFINE_INTERFACES \
	DEF_INTERFACE (InterfaceName1) \
	DEF_INTERFACE (InterfaceName2) \
	DEF_INTERFACE (InterfaceName3) \
	DEF_INTERFACE (InterfaceName4) \
	END_DEFINE_INTERFACES (BaseClass)
///@}

//------------------------------------------------------------------------
/** @name Convenient macros to implement Steinberg::FUnknown methods.
	<b>Examples:</b>
	@code
	class Foo : public FObject, public IFoo2, public IFoo3 
	{
	    ...
	    FUNKNOWN_METHODS2(IFoo2,IFoo3,FObject)
	    ...
	};
	@endcode
*/
///@{
#define FUNKNOWN_METHODS(InterfaceName,BaseClass) \
DEF_INTERFACES_1(InterfaceName,BaseClass) \
REFCOUNT_METHODS(BaseClass)

#define FUNKNOWN_METHODS2(InterfaceName1,InterfaceName2,BaseClass) \
DEF_INTERFACES_2(InterfaceName1,InterfaceName2,BaseClass) \
REFCOUNT_METHODS(BaseClass)

#define FUNKNOWN_METHODS3(InterfaceName1,InterfaceName2,InterfaceName3,BaseClass) \
DEF_INTERFACES_3(InterfaceName1,InterfaceName2,InterfaceName3,BaseClass) \
REFCOUNT_METHODS(BaseClass)

#define FUNKNOWN_METHODS4(InterfaceName1,InterfaceName2,InterfaceName3,InterfaceName4,BaseClass) \
DEF_INTERFACES_4(InterfaceName1,InterfaceName2,InterfaceName3,InterfaceName4,BaseClass) \
REFCOUNT_METHODS(BaseClass)
///@}


//------------------------------------------------------------------------
//------------------------------------------------------------------------
#if COM_COMPATIBLE
//------------------------------------------------------------------------
/** @name Macros to implement IUnknown interfaces with FObject.
	<b>Examples:</b>
	@code
	class MyEnumFormat : public FObject, IEnumFORMATETC
	{
	    ...
		COM_UNKNOWN_METHODS (IEnumFORMATETC, IUnknown)
	    ...
	};
	@endcode
*/
///@{
//------------------------------------------------------------------------
#define IUNKNOWN_REFCOUNT_METHODS(BaseClass) \
STDMETHOD_ (ULONG, AddRef) (void) {return BaseClass::addRef ();} \
STDMETHOD_ (ULONG, Release) (void) {return BaseClass::release ();}

//------------------------------------------------------------------------
#define COM_QUERY_INTERFACE(iid, obj, InterfaceName)     \
if (riid == __uuidof(InterfaceName))                     \
{                                                        \
	addRef ();                                           \
	*obj = (InterfaceName*)this;                         \
	return kResultOk;                                    \
}

//------------------------------------------------------------------------
#define COM_OBJECT_QUERY_INTERFACE(InterfaceName,BaseClass)        \
STDMETHOD (QueryInterface) (REFIID riid, void** object)            \
{                                                                  \
	COM_QUERY_INTERFACE (riid, object, InterfaceName)              \
	return BaseClass::queryInterface ((FIDString)&riid, object);   \
}

//------------------------------------------------------------------------
#define COM_UNKNOWN_METHODS(InterfaceName,BaseClass) \
COM_OBJECT_QUERY_INTERFACE(InterfaceName,BaseClass) \
IUNKNOWN_REFCOUNT_METHODS(BaseClass)
///@}

#endif // COM_COMPATIBLE
