//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/fvariant.h
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

#include "pluginterfaces/base/fstrdefs.h"
#include "pluginterfaces/base/funknown.h"
#include <cstdlib>

//------------------------------------------------------------------------
namespace Steinberg {

class FUnknown;

//------------------------------------------------------------------------
//  FVariant struct declaration
//------------------------------------------------------------------------
/** A Value of variable type.
 \ingroup pluginBase
 */
class FVariant
{
//------------------------------------------------------------------------
public:
	enum
	{
		kEmpty = 0,
		kInteger = 1 << 0,
		kFloat = 1 << 1,
		kString8 = 1 << 2,
		kObject = 1 << 3,
		kOwner = 1 << 4,
		kString16 = 1 << 5
	};

//------------------------------------------------------------------------
	// ctors
	inline FVariant () { memset (this, 0, sizeof (FVariant)); }
	inline FVariant (const FVariant& variant);

	inline FVariant (bool b) : type (kInteger), intValue (b) {}
	inline FVariant (uint32 v) : type (kInteger), intValue (v) {}
	inline FVariant (int64 v) : type (kInteger), intValue (v) {}
	inline FVariant (double v) : type (kFloat), floatValue (v) {}
	inline FVariant (const char8* str) : type (kString8), string8 (str) {}
	inline FVariant (const char16* str) : type (kString16), string16 (str) {}
	inline FVariant (FUnknown* obj, bool owner = false) : type (kObject), object (obj)
	{
		setOwner (owner);
	}
	inline ~FVariant () { empty (); }

//------------------------------------------------------------------------
	inline FVariant& operator= (const FVariant& variant);

	inline void set (bool b)
	{
		setInt (b);
	}

	inline void set (uint32 v)
	{
		setInt (v);
	}

	inline void set (int64 v)
	{
		setInt (v);
	}

	inline void set (double v)
	{
		setFloat (v);
	}

	inline void set (const char8* c)
	{
		setString8 (c);
	}

	inline void set (const char16* c)
	{
		setString16 (c);
	}

	inline void setInt (int64 v)
	{
		empty ();
		type = kInteger;
		intValue = v;
	}

	inline void setFloat (double v)
	{
		empty ();
		type = kFloat;
		floatValue = v;
	}
	inline void setString8 (const char8* v)
	{
		empty ();
		type = kString8;
		string8 = v;
	}
	inline void setString16 (const char16* v)
	{
		empty ();
		type = kString16;
		string16 = v;
	}

	inline void setObject (FUnknown* obj)
	{
		empty ();
		type = kObject;
		object = obj;
	}

	template <typename T>
	inline T get () const;

	inline int64 getInt () const { return (type & kInteger) ? intValue : 0; }
	inline double getFloat () const { return (type & kFloat) ? floatValue : 0.; }
	inline double getNumber () const
	{
		return (type & kInteger) ? static_cast<double> (intValue) : (type & kFloat) ? floatValue :
																					  0.;
	}
	inline const char8* getString8 () const { return (type & kString8) ? string8 : nullptr; }
	inline const char16* getString16 () const { return (type & kString16) ? string16 : nullptr; }

	inline FUnknown* getObject () const { return (type & kObject) ? object : nullptr; }

	inline uint16 getType () const { return static_cast<uint16> (type & ~(kOwner)); }
	inline bool isEmpty () const { return getType () == kEmpty; }
	inline bool isOwner () const { return (type & kOwner) != 0; }
	inline bool isString () const { return (type & (kString8 | kString16)) != 0; }
	inline void setOwner (bool state)
	{
		if (state)
			type |= kOwner;
		else
			type &= ~kOwner;
	}

	void empty ();
//------------------------------------------------------------------------
	uint16 type;
	union
	{
		int64 intValue;
		double floatValue;
		const char8* string8;
		const char16* string16;
		FUnknown* object;
	};
};

//------------------------------------------------------------------------
inline bool operator== (const FVariant& v1, const FVariant& v2)
{
#if SMTG_PLATFORM_64
	return v1.type == v2.type && v1.intValue == v2.intValue;
#else
	if (v1.type != v2.type)
		return false;
	if (v1.type & (FVariant::kString8 | FVariant::kString16 | FVariant::kObject))
		return v1.string8 == v2.string8; // pointer type comparisons
	return v1.intValue == v2.intValue; // intValue & double comparison

#endif
}

template <>
inline bool FVariant::get<bool> () const
{
	return getInt () != 0;
}

template <>
inline uint32 FVariant::get<uint32> () const
{
	return static_cast<uint32> (getInt ());
}

template <>
inline int32 FVariant::get<int32> () const
{
	return static_cast<int32> (getInt ());
}

template <>
inline int64 FVariant::get<int64> () const
{
	return static_cast<int64> (getInt ());
}

template <>
inline float FVariant::get<float> () const
{
	return static_cast<float> (getFloat ());
}

template <>
inline double FVariant::get<double> () const
{
	return getFloat ();
}

template <>
inline const char8* FVariant::get<const char8*> () const
{
	return getString8 ();
}

template <>
inline const char16* FVariant::get<const char16*> () const
{
	return getString16 ();
}

template <>
inline FUnknown* FVariant::get<FUnknown*> () const
{
	return getObject ();
}

//------------------------------------------------------------------------
inline bool operator!= (const FVariant& v1, const FVariant& v2) { return !(v1 == v2); }

//------------------------------------------------------------------------
inline FVariant::FVariant (const FVariant& variant) : type (kEmpty) { *this = variant; }

//------------------------------------------------------------------------
inline void FVariant::empty ()
{
	if (type & kOwner)
	{
		if ((type & kString8) && string8)
			free (const_cast<char8*> (string8)); // should match DELETESTR8
		else if ((type & kString16) && string16)
			free (const_cast<char16*> (string16)); // should match DELETESTR16

		else if ((type & kObject) && object)
			object->release ();
	}
	memset (this, 0, sizeof (FVariant));
}

//------------------------------------------------------------------------
inline FVariant& FVariant::operator= (const FVariant& variant)
{
	empty ();

	type = variant.type;

	if ((type & kString8) && variant.string8)
	{
		string8 = static_cast<char8*> (malloc (strlen (variant.string8) + 1)); // should match NEWSTR8 
		strcpy (const_cast<char8*> (string8), variant.string8);
		type |= kOwner;
	}
	else if ((type & kString16) && variant.string16)
	{
		auto len = static_cast<size_t> (strlen16 (variant.string16));
		string16 = static_cast<char16*> (malloc ((len + 1) * sizeof (char16))); // should match NEWSTR16
		char16* tmp = const_cast<char16*> (string16);
		memcpy (tmp, variant.string16, len * sizeof (char16));
		tmp[len] = 0;
		type |= kOwner;
	}
	else if ((type & kObject) && variant.object)
	{
		object = variant.object;
		object->addRef ();
		type |= kOwner;
	}
	else
		intValue = variant.intValue; // copy memory

	return *this;
}

//------------------------------------------------------------------------
} // namespace Steinberg
