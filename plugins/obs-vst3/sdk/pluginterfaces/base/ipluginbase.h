//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/ipluginbase.h
// Created by  : Steinberg, 01/2004
// Description : Basic Plug-in Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "funknown.h"
#include "fstrdefs.h"

namespace Steinberg {

//------------------------------------------------------------------------
/**  Basic interface to a plug-in component: IPluginBase
\ingroup pluginBase
- [plug imp]
- initialize/terminate the plug-in component

The host uses this interface to initialize and to terminate the plug-in component.
The context that is passed to the initialize method contains any interface to the
host that the plug-in will need to work. These interfaces can vary from category to category.
A list of supported host context interfaces should be included in the documentation
of a specific category. 
*/
class IPluginBase: public FUnknown
{
public:
//------------------------------------------------------------------------
	/** The host passes a number of interfaces as context to initialize the plug-in class.
		\param context, passed by the host, is mandatory and should implement IHostApplication
		@note Extensive memory allocations etc. should be performed in this method rather than in
	   the class' constructor! If the method does NOT return kResultOk, the object is released
	   immediately. In this case terminate is not called! */
	virtual tresult PLUGIN_API initialize (FUnknown* context /*in*/) = 0;

	/** This function is called before the plug-in is unloaded and can be used for
	    cleanups. You have to release all references to any host application interfaces. */
	virtual tresult PLUGIN_API terminate () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPluginBase, 0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625)


//------------------------------------------------------------------------
/** Basic Information about the class factory of the plug-in.
\ingroup pluginBase
*/
struct PFactoryInfo
{
//------------------------------------------------------------------------
	enum FactoryFlags
	{
		/** Nothing */
		kNoFlags = 0,

		/** The number of exported classes can change each time the Module is loaded. If this flag
		   is set, the host does not cache class information. This leads to a longer startup time
		   because the host always has to load the Module to get the current class information. */
		kClassesDiscardable = 1 << 0,

		/** This flag is deprecated, do not use anymore, resp. it will get ignored from
		   Cubase/Nuendo 12 and later. */
		kLicenseCheck = 1 << 1,

		/** Component will not be unloaded until process exit */
		kComponentNonDiscardable = 1 << 3,

		/** Components have entirely unicode encoded strings (True for VST 3 plug-ins so far). */
		kUnicode = 1 << 4
	};

	enum
	{
		kURLSize = 256,
		kEmailSize = 128,
		kNameSize = 64
	};

//------------------------------------------------------------------------
	char8 vendor[kNameSize];	///< e.g. "Steinberg Media Technologies"
	char8 url[kURLSize];		///< e.g. "http://www.steinberg.de"
	char8 email[kEmailSize];	///< e.g. "info@steinberg.de"
	int32 flags;				///< (see FactoryFlags above)
//------------------------------------------------------------------------
	SMTG_CONSTEXPR14 PFactoryInfo (const char8* _vendor, const char8* _url, const char8* _email,
	                               int32 _flags)
#if SMTG_CPP14
	: vendor (), url (), email (), flags ()
#endif
	{
		strncpy8 (vendor, _vendor, kNameSize);
		strncpy8 (url, _url, kURLSize);
		strncpy8 (email, _email, kEmailSize);
		flags = _flags;
#ifdef UNICODE
		flags |= kUnicode;
#endif
	}
#if SMTG_CPP11
	constexpr PFactoryInfo () : vendor (), url (), email (), flags () {}
#else
	PFactoryInfo () { memset (this, 0, sizeof (PFactoryInfo)); }
#endif
};

//------------------------------------------------------------------------
/**  Basic Information about a class provided by the plug-in.
\ingroup pluginBase
*/
struct PClassInfo
{
//------------------------------------------------------------------------
	enum ClassCardinality
	{
		kManyInstances = 0x7FFFFFFF
	};

	enum
	{
		kCategorySize = 32,
		kNameSize = 64
	};
//------------------------------------------------------------------------
	/** Class ID 16 Byte class GUID */
	TUID cid;

	/** Cardinality of the class, set to kManyInstances (see \ref PClassInfo::ClassCardinality) */
	int32 cardinality;

	/** Class category, host uses this to categorize interfaces */
	char8 category[kCategorySize];

	/** Class name, visible to the user */
	char8 name[kNameSize];

//------------------------------------------------------------------------

	SMTG_CONSTEXPR14 PClassInfo (const TUID _cid, int32 _cardinality, const char8* _category,
	                             const char8* _name)
#if SMTG_CPP14
	: cid (), cardinality (), category (), name ()
#endif
	{
#if SMTG_CPP14
		copyTUID (cid, _cid);
#else
		memset (this, 0, sizeof (PClassInfo));
		memcpy (cid, _cid, sizeof (TUID));
#endif
		if (_category)
			strncpy8 (category, _category, kCategorySize);
		if (_name)
			strncpy8 (name, _name, kNameSize);
		cardinality = _cardinality;
	}
#if SMTG_CPP11
	constexpr PClassInfo () : cid (), cardinality (), category (), name () {}
#else
	PClassInfo () { memset (this, 0, sizeof (PClassInfo)); }
#endif
};

//------------------------------------------------------------------------
//  IPluginFactory interface declaration
//------------------------------------------------------------------------
/**	Class factory that any plug-in defines for creating class instances: IPluginFactory
\ingroup pluginBase
- [plug imp]

From the host's point of view a plug-in module is a factory which can create
a certain kind of object(s). The interface IPluginFactory provides methods
to get information about the classes exported by the plug-in and a
mechanism to create instances of these classes (that usually define the IPluginBase interface).

<b> An implementation is provided in public.sdk/source/common/pluginfactory.cpp </b>
\see GetPluginFactory
*/
class IPluginFactory : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Fill a PFactoryInfo structure with information about the plug-in vendor. */
	virtual tresult PLUGIN_API getFactoryInfo (PFactoryInfo* info /*inout*/) = 0;

	/** Returns the number of exported classes by this factory. If you are using the CPluginFactory
	 * implementation provided by the SDK, it returns the number of classes you registered with
	 * CPluginFactory::registerClass. */
	virtual int32 PLUGIN_API countClasses () = 0;

	/** Fill a PClassInfo structure with information about the class at the specified index. */
	virtual tresult PLUGIN_API getClassInfo (int32 index /*in*/, PClassInfo* info /*inout*/) = 0;

	/** Create a new class instance. */
	virtual tresult PLUGIN_API createInstance (FIDString cid /*in*/, FIDString _iid /*in*/,
	                                           void** obj /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPluginFactory, 0x7A4D811C, 0x52114A1F, 0xAED9D2EE, 0x0B43BF9F)


//------------------------------------------------------------------------
/**  Version 2 of Basic Information about a class provided by the plug-in.
\ingroup pluginBase
*/
struct PClassInfo2
{
//------------------------------------------------------------------------
	/** Class ID 16 Byte class GUID */
	TUID cid;

	/** Cardinality of the class, set to kManyInstances (see \ref PClassInfo::ClassCardinality) */
	int32 cardinality;

	/** Class category, host uses this to categorize interfaces */
	char8 category[PClassInfo::kCategorySize];

	/** Class name, visible to the user */
	char8 name[PClassInfo::kNameSize];

	enum {
		kVendorSize = 64,
		kVersionSize = 64,
		kSubCategoriesSize = 128
	};

	/** flags used for a specific category, must be defined where category is defined */
	uint32 classFlags;

	/** module specific subcategories, can be more than one, logically added by the OR operator */
	char8 subCategories[kSubCategoriesSize];
	
	/** overwrite vendor information from factory info */
	char8 vendor[kVendorSize];

	/** Version string (e.g. "1.0.0.512" with Major.Minor.Subversion.Build) */
	char8 version[kVersionSize];

	/** SDK version used to build this class (e.g. "VST 3.0") */
	char8 sdkVersion[kVersionSize];
//------------------------------------------------------------------------

	SMTG_CONSTEXPR14 PClassInfo2 (const TUID _cid, int32 _cardinality, const char8* _category,
	                              const char8* _name, int32 _classFlags,
	                              const char8* _subCategories, const char8* _vendor,
	                              const char8* _version, const char8* _sdkVersion)
#if SMTG_CPP14
	: cid ()
	, cardinality ()
	, category ()
	, name ()
	, classFlags ()
	, subCategories ()
	, vendor ()
	, version ()
	, sdkVersion ()
#endif
	{
#if SMTG_CPP14
		copyTUID (cid, _cid);
#else
		memset (this, 0, sizeof (PClassInfo2));
		memcpy (cid, _cid, sizeof (TUID));
#endif
		cardinality = _cardinality;
		if (_category)
			strncpy8 (category, _category, PClassInfo::kCategorySize);
		if (_name)
			strncpy8 (name, _name, PClassInfo::kNameSize);
		classFlags = static_cast<uint32> (_classFlags);
		if (_subCategories)
			strncpy8 (subCategories, _subCategories, kSubCategoriesSize);
		if (_vendor)
			strncpy8 (vendor, _vendor, kVendorSize);
		if (_version)
			strncpy8 (version, _version, kVersionSize);
		if (_sdkVersion)
			strncpy8 (sdkVersion, _sdkVersion, kVersionSize);
	}
#if SMTG_CPP11
	constexpr PClassInfo2 ()
	: cid ()
	, cardinality ()
	, category ()
	, name ()
	, classFlags ()
	, subCategories ()
	, vendor ()
	, version ()
	, sdkVersion ()
	{
	}
#else
	PClassInfo2 () { memset (this, 0, sizeof (PClassInfo2)); }
#endif
};

//------------------------------------------------------------------------
//  IPluginFactory2 interface declaration
//------------------------------------------------------------------------
/**	Version 2 of class factory supporting PClassInfo2: IPluginFactory2
\ingroup pluginBase
\copydoc IPluginFactory
*/
class IPluginFactory2 : public IPluginFactory
{
public:
//------------------------------------------------------------------------
	/** Returns the class info (version 2) for a given index. */
	virtual tresult PLUGIN_API getClassInfo2 (int32 index /*in*/, PClassInfo2* info /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IPluginFactory2, 0x0007B650, 0xF24B4C0B, 0xA464EDB9, 0xF00B2ABB)


//------------------------------------------------------------------------
/** Unicode Version of Basic Information about a class provided by the plug-in
*/
struct PClassInfoW
{
//------------------------------------------------------------------------
	TUID cid;							///< see \ref PClassInfo
	int32 cardinality;					///< see \ref PClassInfo
	char8 category[PClassInfo::kCategorySize];	///< see \ref PClassInfo
	char16 name[PClassInfo::kNameSize];	///< see \ref PClassInfo

	enum {
		kVendorSize = 64,
		kVersionSize = 64,
		kSubCategoriesSize = 128
	};

	/** flags used for a specific category, must be defined where category is defined */
	uint32 classFlags;

	/** module specific subcategories, can be more than one, logically added by the OR operator */
	char8 subCategories[kSubCategoriesSize];
	
	/** overwrite vendor information from factory info */
	char16 vendor[kVendorSize];
	
	/** Version string (e.g. "1.0.0.512" with Major.Minor.Subversion.Build) */
	char16 version[kVersionSize];
	
	/** SDK version used to build this class (e.g. "VST 3.0") */
	char16 sdkVersion[kVersionSize];

//------------------------------------------------------------------------
	SMTG_CONSTEXPR14 PClassInfoW (const TUID _cid, int32 _cardinality, const char8* _category,
	                              const char16* _name, int32 _classFlags,
	                              const char8* _subCategories, const char16* _vendor,
	                              const char16* _version, const char16* _sdkVersion)
#if SMTG_CPP14
	: cid ()
	, cardinality ()
	, category ()
	, name ()
	, classFlags ()
	, subCategories ()
	, vendor ()
	, version ()
	, sdkVersion ()
#endif
	{
#if SMTG_CPP14
		copyTUID (cid, _cid);
#else
		memset (this, 0, sizeof (PClassInfoW));
		memcpy (cid, _cid, sizeof (TUID));
#endif
		cardinality = _cardinality;
		if (_category)
			strncpy8 (category, _category, PClassInfo::kCategorySize);
		if (_name)
			strncpy16 (name, _name, PClassInfo::kNameSize);
		classFlags = static_cast<uint32> (_classFlags);
		if (_subCategories)
			strncpy8 (subCategories, _subCategories, kSubCategoriesSize);
		if (_vendor)
			strncpy16 (vendor, _vendor, kVendorSize);
		if (_version)
			strncpy16 (version, _version, kVersionSize);
		if (_sdkVersion)
			strncpy16 (sdkVersion, _sdkVersion, kVersionSize);
	}
#if SMTG_CPP11
	constexpr PClassInfoW ()
	: cid ()
	, cardinality ()
	, category ()
	, name ()
	, classFlags ()
	, subCategories ()
	, vendor ()
	, version ()
	, sdkVersion ()
	{
	}
#else
	PClassInfoW () { memset (this, 0, sizeof (PClassInfoW)); }
#endif

	SMTG_CONSTEXPR14 void fromAscii (const PClassInfo2& ci2)
	{
#if SMTG_CPP14
		copyTUID (cid, ci2.cid);
#else
		memcpy (cid, ci2.cid, sizeof (TUID));
#endif
		cardinality = ci2.cardinality;
		strncpy8 (category, ci2.category, PClassInfo::kCategorySize);
		str8ToStr16 (name, ci2.name, PClassInfo::kNameSize);
		classFlags = ci2.classFlags;
		strncpy8 (subCategories, ci2.subCategories, kSubCategoriesSize);

		str8ToStr16 (vendor, ci2.vendor, kVendorSize);
		str8ToStr16 (version, ci2.version, kVersionSize);
		str8ToStr16 (sdkVersion, ci2.sdkVersion, kVersionSize);
	}
};

//------------------------------------------------------------------------
//  IPluginFactory3 interface declaration
//------------------------------------------------------------------------
/**	Version 3 of class factory supporting PClassInfoW: IPluginFactory3
\ingroup pluginBase
\copydoc IPluginFactory
*/
class IPluginFactory3 : public IPluginFactory2
{
public:
//------------------------------------------------------------------------
	/** Returns the unicode class info for a given index. */
	virtual tresult PLUGIN_API getClassInfoUnicode (int32 index /*in*/,
	                                                PClassInfoW* info /*out*/) = 0;

	/** Receives information about host*/
	virtual tresult PLUGIN_API setHostContext (FUnknown* context /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IPluginFactory3, 0x4555A2AB, 0xC1234E57, 0x9B122910, 0x36878931)
//------------------------------------------------------------------------
} // namespace Steinberg

//------------------------------------------------------------------------
#define LICENCE_UID(l1, l2, l3, l4) \
{ \
	(int8)((l1 & 0xFF000000) >> 24), (int8)((l1 & 0x00FF0000) >> 16), \
	(int8)((l1 & 0x0000FF00) >>  8), (int8)((l1 & 0x000000FF)      ), \
	(int8)((l2 & 0xFF000000) >> 24), (int8)((l2 & 0x00FF0000) >> 16), \
	(int8)((l2 & 0x0000FF00) >>  8), (int8)((l2 & 0x000000FF)      ), \
	(int8)((l3 & 0xFF000000) >> 24), (int8)((l3 & 0x00FF0000) >> 16), \
	(int8)((l3 & 0x0000FF00) >>  8), (int8)((l3 & 0x000000FF)      ), \
	(int8)((l4 & 0xFF000000) >> 24), (int8)((l4 & 0x00FF0000) >> 16), \
	(int8)((l4 & 0x0000FF00) >>  8), (int8)((l4 & 0x000000FF)      )  \
}

//------------------------------------------------------------------------
// GetPluginFactory
//------------------------------------------------------------------------
/**  Plug-in entry point.
\ingroup pluginBase
Any plug-in must define and export this function. \n
A typical implementation of GetPluginFactory looks like this
\code{.cpp}
SMTG_EXPORT_SYMBOL IPluginFactory* PLUGIN_API GetPluginFactory ()
{
	if (!gPluginFactory)
	{
		static PFactoryInfo factoryInfo =
		{
			"My Company Name",
			"http://www.mywebpage.com",
			"mailto:myemail@address.com",
			PFactoryInfo::kNoFlags
		};

		gPluginFactory = new CPluginFactory (factoryInfo);

		static PClassInfo componentClass =
		{
			INLINE_UID (0x00000000, 0x00000000, 0x00000000, 0x00000000), // replace by a valid uid
			1,
			"Service",    // category
			"Name"
		};

		gPluginFactory->registerClass (&componentClass, MyComponentClass::newInstance);
	}
	else
		gPluginFactory->addRef ();

	return gPluginFactory;
}
\endcode
\see \ref loadPlugin
*/
extern "C"
{
	SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory* PLUGIN_API GetPluginFactory ();
	typedef Steinberg::IPluginFactory* (PLUGIN_API *GetFactoryProc) ();
}
