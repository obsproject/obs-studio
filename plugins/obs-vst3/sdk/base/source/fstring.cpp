//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fstring.cpp
// Created by  : Steinberg, 2008
// Description : String class
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "base/source/fstring.h"
#include "base/source/fdebug.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/fvariant.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <utility>

#if SMTG_OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef _MSC_VER
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4996)

#if DEVELOPMENT
#include <crtdbg.h>

#define malloc(s) _malloc_dbg (s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define realloc(p, s) _realloc_dbg (p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p) _free_dbg (p, _NORMAL_BLOCK)

#endif // DEVELOPMENT
#endif // _MSC_VER
#endif // SMTG_OS_WINDOWS

#ifndef kPrintfBufferSize
#define kPrintfBufferSize 4096
#endif

#if SMTG_OS_MACOS
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStringEncodingExt.h>
#include <CoreFoundation/CoreFoundation.h>
#include <wchar.h>

#if defined(__GNUC__) && (__GNUC__ >= 4) && !__LP64__
// on 32 bit Mac OS X we can safely ignore the format warnings as sizeof(int) == sizeof(long)
#pragma GCC diagnostic ignored "-Wformat"
#endif

#define SMTG_ENABLE_DEBUG_CFALLOCATOR 0
#define SMTG_DEBUG_CFALLOCATOR (DEVELOPMENT && SMTG_ENABLE_DEBUG_CFALLOCATOR)

#if SMTG_DEBUG_CFALLOCATOR
#include <dlfcn.h>
#include <libkern/OSAtomic.h>
#endif

namespace Steinberg {
#if SMTG_DEBUG_CFALLOCATOR
static CFAllocatorRef kCFAllocator = NULL;

struct CFStringDebugAllocator : CFAllocatorContext
{
	CFStringDebugAllocator ()
	{
		version = 0;
		info = this;
		retain = nullptr;
		release = nullptr;
		copyDescription = nullptr;
		allocate = allocateCallBack;
		reallocate = reallocateCallBack;
		deallocate = deallocateCallBack;
		preferredSize = preferredSizeCallBack;

		numAllocations = allocationSize = numDeallocations = 0;
		cfAllocator = CFAllocatorCreate (kCFAllocatorUseContext, this);

		Dl_info info;
		if (dladdr ((const void*)CFStringDebugAllocator::allocateCallBack, &info))
		{
			moduleName = info.dli_fname;
		}
		kCFAllocator = cfAllocator;
	}

	~CFStringDebugAllocator ()
	{
		kCFAllocator = kCFAllocatorDefault;
		CFRelease (cfAllocator);
		FDebugPrint ("CFStringDebugAllocator (%s):\n", moduleName.text8 ());
		FDebugPrint ("\tNumber of allocations  : %u\n", numAllocations);
		FDebugPrint ("\tNumber of deallocations: %u\n", numDeallocations);
		FDebugPrint ("\tAllocated Bytes        : %u\n", allocationSize);
	}

	String moduleName;
	CFAllocatorRef cfAllocator;
	volatile int64_t numAllocations;
	volatile int64_t numDeallocations;
	volatile int64_t allocationSize;

	void* doAllocate (CFIndex allocSize, CFOptionFlags hint)
	{
		void* ptr = CFAllocatorAllocate (kCFAllocatorDefault, allocSize, hint);
		OSAtomicIncrement64 (&numAllocations);
		OSAtomicAdd64 (allocSize, &allocationSize);
		return ptr;
	}
	void* doReallocate (void* ptr, CFIndex newsize, CFOptionFlags hint)
	{
		void* newPtr = CFAllocatorReallocate (kCFAllocatorDefault, ptr, newsize, hint);
		return newPtr;
	}
	void doDeallocate (void* ptr)
	{
		CFAllocatorDeallocate (kCFAllocatorDefault, ptr);
		OSAtomicIncrement64 (&numDeallocations);
	}
	CFIndex getPreferredSize (CFIndex size, CFOptionFlags hint)
	{
		return CFAllocatorGetPreferredSizeForSize (kCFAllocatorDefault, size, hint);
	}

	static void* allocateCallBack (CFIndex allocSize, CFOptionFlags hint, void* info)
	{
		return static_cast<CFStringDebugAllocator*> (info)->doAllocate (allocSize, hint);
	}
	static void* reallocateCallBack (void* ptr, CFIndex newsize, CFOptionFlags hint, void* info)
	{
		return static_cast<CFStringDebugAllocator*> (info)->doReallocate (ptr, newsize, hint);
	}
	static void deallocateCallBack (void* ptr, void* info)
	{
		static_cast<CFStringDebugAllocator*> (info)->doDeallocate (ptr);
	}
	static CFIndex preferredSizeCallBack (CFIndex size, CFOptionFlags hint, void* info)
	{
		return static_cast<CFStringDebugAllocator*> (info)->getPreferredSize (size, hint);
	}
};
static CFStringDebugAllocator gDebugAllocator;
#else

static const CFAllocatorRef kCFAllocator = ::kCFAllocatorDefault;
#endif // SMTG_DEBUG_CFALLOCATOR
}

//-----------------------------------------------------------------------------
static void* toCFStringRef (const Steinberg::char8* source, Steinberg::uint32 encoding)
{
	if (encoding == 0xFFFF)
		encoding = kCFStringEncodingASCII;
	if (source)
		return (void*)CFStringCreateWithCString (Steinberg::kCFAllocator, source, encoding);
	else
		return (void*)CFStringCreateWithCString (Steinberg::kCFAllocator, "", encoding);
}

//-----------------------------------------------------------------------------
static bool fromCFStringRef (Steinberg::char8* dest, Steinberg::int32 destSize, const void* cfStr,
                             Steinberg::uint32 encoding)
{
	CFIndex usedBytes;
	CFRange range = {0, CFStringGetLength ((CFStringRef)cfStr)};
	bool result = CFStringGetBytes ((CFStringRef)cfStr, range, encoding, '?', false, (UInt8*)dest,
	                                destSize, &usedBytes);
	dest[usedBytes] = 0;
	return result;
}
#endif // SMTG_OS_MACOS

#if SMTG_OS_WINDOWS
//-----------------------------------------------------------------------------
static inline int stricmp16 (const Steinberg::tchar* s1, const Steinberg::tchar* s2)
{
	return wcsicmp (Steinberg::wscast (s1), Steinberg::wscast (s2));
}

//-----------------------------------------------------------------------------
static inline int strnicmp16 (const Steinberg::tchar* s1, const Steinberg::tchar* s2, size_t l)
{
	return wcsnicmp (Steinberg::wscast (s1), Steinberg::wscast (s2), l);
}

//-----------------------------------------------------------------------------
static inline int vsnwprintf (Steinberg::char16* buffer, size_t bufferSize,
                              const Steinberg::char16* format, va_list args)
{
	return _vsnwprintf (Steinberg::wscast (buffer), bufferSize, Steinberg::wscast (format), args);
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 sprintf16 (Steinberg::char16* str, const Steinberg::char16* format,
                                          ...)
{
	va_list marker;
	va_start (marker, format);
	return vsnwprintf (str, static_cast<size_t> (-1), format, marker);
}

#elif SMTG_OS_LINUX
#include <cassert>
#include <codecvt>
#include <cstring>
#include <limits>
#include <locale>
#include <string>
#include <wchar.h>

using ConverterFacet = std::codecvt_utf8_utf16<char16_t>;
using Converter = std::wstring_convert<ConverterFacet, char16_t>;

//------------------------------------------------------------------------
static ConverterFacet& converterFacet ()
{
	static ConverterFacet gFacet;
	return gFacet;
}

//------------------------------------------------------------------------
static Converter& converter ()
{
	static Converter gConverter;
	return gConverter;
}

//-----------------------------------------------------------------------------
static inline int stricasecmp (const Steinberg::char8* s1, const Steinberg::char8* s2)
{
	return ::strcasecmp (s1, s2);
}

//-----------------------------------------------------------------------------
static inline int strnicasecmp (const Steinberg::char8* s1, const Steinberg::char8* s2, size_t n)
{
	return ::strncasecmp (s1, s2, n);
}

//-----------------------------------------------------------------------------
static inline int stricmp16 (const Steinberg::char16* s1, const Steinberg::char16* s2)
{
	auto str1 = converter ().to_bytes (s1);
	auto str2 = converter ().to_bytes (s2);
	return stricasecmp (str1.data (), str2.data ());
}

//-----------------------------------------------------------------------------
static inline int strnicmp16 (const Steinberg::char16* s1, const Steinberg::char16* s2, int n)
{
	auto str1 = converter ().to_bytes (s1);
	auto str2 = converter ().to_bytes (s2);
	return strnicasecmp (str1.data (), str2.data (), n);
}

//-----------------------------------------------------------------------------
static inline int sprintf16 (Steinberg::char16* wcs, const Steinberg::char16* format, ...)
{
	assert (false && "DEPRECATED No Linux implementation");
	return 0;
}

//-----------------------------------------------------------------------------
static inline int vsnwprintf (Steinberg::char16* wcs, size_t maxlen,
                              const Steinberg::char16* format, va_list args)
{
	Steinberg::char8 str8[kPrintfBufferSize];
	auto format_utf8 = converter ().to_bytes (format);
	auto len = vsnprintf (str8, kPrintfBufferSize, format_utf8.data (), args);

	auto tmp_str = converter ().from_bytes (str8, str8 + len);
	auto target_len = std::min (tmp_str.size (), maxlen - 1);
	tmp_str.copy (wcs, target_len);
	wcs[target_len] = '\0';

	return tmp_str.size ();
}

//-----------------------------------------------------------------------------
static inline Steinberg::char16* strrchr16 (const Steinberg::char16* str, Steinberg::char16 c)
{
	assert (false && "DEPRECATED No Linux implementation");
	return nullptr;
}

#elif SMTG_OS_MACOS
#define tstrtoi64 strtoll
#define stricmp strcasecmp
#define strnicmp strncasecmp

//-----------------------------------------------------------------------------
static inline Steinberg::int32 strnicmp16 (const Steinberg::char16* str1,
                                           const Steinberg::char16* str2, size_t size)
{
	if (size == 0)
		return 0;

	CFIndex str1Len = Steinberg::strlen16 (str1);
	CFIndex str2Len = Steinberg::strlen16 (str2);
	if (static_cast<CFIndex> (size) < str2Len) // range is not applied to second string
		str2Len = size;
	CFStringRef cfStr1 = CFStringCreateWithCharactersNoCopy (
	    Steinberg::kCFAllocator, (UniChar*)str1, str1Len, kCFAllocatorNull);
	CFStringRef cfStr2 = CFStringCreateWithCharactersNoCopy (
	    Steinberg::kCFAllocator, (UniChar*)str2, str2Len, kCFAllocatorNull);
	CFComparisonResult result = CFStringCompareWithOptions (cfStr1, cfStr2, CFRangeMake (0, size),
	                                                        kCFCompareCaseInsensitive);
	CFRelease (cfStr1);
	CFRelease (cfStr2);
	switch (result)
	{
		case kCFCompareEqualTo: return 0;
		case kCFCompareLessThan: return -1;
		case kCFCompareGreaterThan:
		default: return 1;
	};
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 stricmp16 (const Steinberg::char16* str1, CFIndex str1Len,
                                          const Steinberg::char16* str2, CFIndex str2Len)
{
	CFStringRef cfStr1 = CFStringCreateWithCharactersNoCopy (
	    Steinberg::kCFAllocator, (UniChar*)str1, str1Len, kCFAllocatorNull);
	CFStringRef cfStr2 = CFStringCreateWithCharactersNoCopy (
	    Steinberg::kCFAllocator, (UniChar*)str2, str2Len, kCFAllocatorNull);
	CFComparisonResult result = CFStringCompare (cfStr1, cfStr2, kCFCompareCaseInsensitive);
	CFRelease (cfStr1);
	CFRelease (cfStr2);
	switch (result)
	{
		case kCFCompareEqualTo: return 0;
		case kCFCompareLessThan: return -1;
		case kCFCompareGreaterThan:
		default: return 1;
	};
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 stricmp16 (const Steinberg::ConstString& str1,
                                          const Steinberg::ConstString& str2)
{
	return stricmp16 (str1.text16 (), str1.length (), str2.text16 (), str2.length ());
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 stricmp16 (const Steinberg::char16* str1,
                                          const Steinberg::char16* str2)
{
	CFIndex str1Len = Steinberg::strlen16 (str1);
	CFIndex str2Len = Steinberg::strlen16 (str2);
	return stricmp16 (str1, str1Len, str2, str2Len);
}

//-----------------------------------------------------------------------------
static inline Steinberg::char16* strrchr16 (const Steinberg::char16* str, Steinberg::char16 c)
{
	Steinberg::int32 len = Steinberg::ConstString (str).length ();
	while (len > 0)
	{
		if (str[len] == c)
			return const_cast<Steinberg::char16*> (str + len);
		len--;
	}
	return 0;
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 vsnwprintf (Steinberg::char16* str, Steinberg::int32 size,
                                           const Steinberg::char16* format, va_list ap)
{
	// wrapped using CoreFoundation's CFString
	CFMutableStringRef formatString =
	    (CFMutableStringRef)Steinberg::ConstString (format).toCFStringRef (0xFFFF, true);
	CFStringFindAndReplace (formatString, CFSTR ("%s"), CFSTR ("%S"),
	                        CFRangeMake (0, CFStringGetLength (formatString)), 0);
	CFStringRef resultString =
	    CFStringCreateWithFormatAndArguments (Steinberg::kCFAllocator, 0, formatString, ap);
	CFRelease (formatString);
	if (resultString)
	{
		Steinberg::String res;
		res.fromCFStringRef (resultString);
		res.copyTo16 (str, 0, size);
		CFRelease (resultString);
		return 0;
	}
	return 1;
}

//-----------------------------------------------------------------------------
static inline Steinberg::int32 sprintf16 (Steinberg::char16* str, const Steinberg::char16* format,
                                          ...)
{
	va_list marker;
	va_start (marker, format);
	return vsnwprintf (str, -1, format, marker);
}

#endif // SMTG_OS_LINUX

/*
UTF-8                EF BB BF 
UTF-16 Big Endian    FE FF 
UTF-16 Little Endian FF FE 
UTF-32 Big Endian    00 00 FE FF 
UTF-32 Little Endian FF FE 00 00 
*/

namespace Steinberg {

//-----------------------------------------------------------------------------
static inline bool isCaseSensitive (ConstString::CompareMode mode)
{
	return mode == ConstString::kCaseSensitive;
}

//-----------------------------------------------------------------------------
//	ConstString
//-----------------------------------------------------------------------------
ConstString::ConstString (const char8* str, int32 length)
: buffer8 ((char8*)str)
, len (length < 0 ? (str ? static_cast<uint32> (strlen (str)) : 0) : length)
, isWide (0)
{
}

//-----------------------------------------------------------------------------
ConstString::ConstString (const char16* str, int32 length)
: buffer16 ((char16*)str), len (length < 0 ? (str ? strlen16 (str) : 0) : length), isWide (1)
{
}

//-----------------------------------------------------------------------------
ConstString::ConstString (const ConstString& str, int32 offset, int32 length)
: buffer (str.buffer)
, len (length < 0 ? (str.len - (offset > 0 ? offset : 0)) : length)
, isWide (str.isWide)
{
	if (offset > 0)
	{
		if (isWide)
			buffer16 += offset;
		else
			buffer8 += offset;
	}
}

//-----------------------------------------------------------------------------
ConstString::ConstString (const FVariant& var) : buffer (nullptr), len (0), isWide (0)
{
	switch (var.getType ())
	{
		case FVariant::kString8:
			buffer8 = (char8*)var.getString8 ();
			len = buffer8 ? strlen8 (buffer8) : 0;
			isWide = false;
			break;

		case FVariant::kString16:
			buffer16 = (char16*)var.getString16 ();
			len = buffer16 ? strlen16 (buffer16) : 0;
			isWide = true;
			break;
	}
}

//-----------------------------------------------------------------------------
ConstString::ConstString () : buffer (nullptr), len (0), isWide (0)
{
}

//-----------------------------------------------------------------------------
bool ConstString::testChar8 (uint32 index, char8 c) const
{
	if (index >= len)
		return c == 0;
	if (isWide)
	{
		// make c wide
		char8 src[] = {c, 0};
		char16 dest[2] = {0};
		if (multiByteToWideString (dest, src, 2) > 0)
			return buffer16[index] == dest[0];
		return false;
	}
	return buffer8[index] == c;
}

//-----------------------------------------------------------------------------
bool ConstString::testChar16 (uint32 index, char16 c) const
{
	if (index >= len)
		return c == 0;
	if (!isWide)
	{
		// make c ansi
		char16 src[] = {c, 0};
		char8 dest[8] = {0};
		if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
			return buffer8[index] == dest[0];
		return false;
	}
	return buffer16[index] == c;
}

//-----------------------------------------------------------------------------
bool ConstString::extract (String& result, uint32 idx, int32 n) const
{
	// AddressSanitizer : when extracting part of "this" on itself, it can lead to
	// heap-use-after-free.
	SMTG_ASSERT (this != static_cast<ConstString*> (&result))

	if (len == 0 || idx >= len)
		return false;

	if ((idx + n > len) || n < 0)
		n = len - idx;

	if (isWide)
		result.assign (buffer16 + idx, n);
	else
		result.assign (buffer8 + idx, n);

	return true;
}

//-----------------------------------------------------------------------------
int32 ConstString::copyTo8 (char8* str, uint32 idx, int32 n) const
{
	if (!str)
		return 0;

	if (isWide)
	{
		String tmp (text16 ());
		if (tmp.toMultiByte () == false)
			return 0;
		return tmp.copyTo8 (str, idx, n);
	}

	if (isEmpty () || idx >= len || !buffer8)
	{
		str[0] = 0;
		return 0;
	}

	if ((idx + n > len) || n < 0)
		n = len - idx;

	memcpy (str, &(buffer8[idx]), n * sizeof (char8));
	str[n] = 0;
	return n;
}

//-----------------------------------------------------------------------------
int32 ConstString::copyTo16 (char16* str, uint32 idx, int32 n) const
{
	if (!str)
		return 0;

	if (!isWide)
	{
		String tmp (text8 ());
		if (tmp.toWideString () == false)
			return 0;
		return tmp.copyTo16 (str, idx, n);
	}

	if (isEmpty () || idx >= len || !buffer16)
	{
		str[0] = 0;
		return 0;
	}

	if ((idx + n > len) || n < 0)
		n = len - idx;

	memcpy (str, &(buffer16[idx]), n * sizeof (char16));
	str[n] = 0;
	return n;
}

//-----------------------------------------------------------------------------
int32 ConstString::copyTo (tchar* str, uint32 idx, int32 n) const
{
#ifdef UNICODE
	return copyTo16 (str, idx, n);
#else
	return copyTo8 (str, idx, n);
#endif
}

//-----------------------------------------------------------------------------
void ConstString::copyTo (IStringResult* result) const
{
	if (isWideString () == false)
	{
		result->setText (text8 ());
	}
	else
	{
		FUnknownPtr<IString> iStr (result);
		if (iStr)
		{
			iStr->setText16 (text16 ());
		}
		else
		{
			String tmp (*this);
			tmp.toMultiByte ();
			result->setText (tmp.text8 ());
		}
	}
}

//-----------------------------------------------------------------------------
void ConstString::copyTo (IString& string) const
{
	if (isWideString ())
		string.setText16 (text16 ());
	else
		string.setText8 (text8 ());
}

//-----------------------------------------------------------------------------
int32 ConstString::compare (const ConstString& str, int32 n, CompareMode mode) const
{
	if (n == 0)
		return 0;

	if (str.isEmpty ())
	{
		if (isEmpty ())
			return 0;
		return 1;
	}
	if (isEmpty ())
		return -1;

	if (!isWide && !str.isWide)
	{
		if (n < 0)
		{
			if (isCaseSensitive (mode))
				return strcmp (*this, str);
			return stricmp (*this, str);
		}
		if (isCaseSensitive (mode))
			return strncmp (*this, str, n);
		return strnicmp (*this, str, n);
	}
	if (isWide && str.isWide)
	{
		if (n < 0)
		{
			if (isCaseSensitive (mode))
				return strcmp16 (*this, str);
			return stricmp16 (*this, str);
		}
		if (isCaseSensitive (mode))
			return strncmp16 (*this, str, n);
		return strnicmp16 (*this, str, n);
	}
	return compareAt (0, str, n, mode);
}

//-----------------------------------------------------------------------------
int32 ConstString::compare (const ConstString& str, CompareMode mode) const
{
	return compare (str, -1, mode);
}

//-----------------------------------------------------------------------------
int32 ConstString::compareAt (uint32 index, const ConstString& str, int32 n, CompareMode mode) const
{
	if (n == 0)
		return 0;

	if (str.isEmpty ())
	{
		if (isEmpty ())
			return 0;
		return 1;
	}
	if (isEmpty ())
		return -1;

	if (!isWide && !str.isWide)
	{
		char8* toCompare = buffer8;
		if (index > 0)
		{
			if (index >= len)
			{
				if (str.isEmpty ())
					return 0;
				return -1;
			}
			toCompare += index;
		}

		if (n < 0)
		{
			if (isCaseSensitive (mode))
				return strcmp (toCompare, str);
			return stricmp (toCompare, str);
		}
		if (isCaseSensitive (mode))
			return strncmp (toCompare, str, n);
		return strnicmp (toCompare, str, n);
	}
	if (isWide && str.isWide)
	{
		char16* toCompare = buffer16;
		if (index > 0)
		{
			if (index >= len)
			{
				if (str.isEmpty ())
					return 0;
				return -1;
			}
			toCompare += index;
		}

		if (n < 0)
		{
			if (isCaseSensitive (mode))
				return strcmp16 (toCompare, str.text16 ());
			return stricmp16 (toCompare, str.text16 ());
		}
		if (isCaseSensitive (mode))
			return strncmp16 (toCompare, str.text16 (), n);
		return strnicmp16 (toCompare, str.text16 (), n);
	}

	if (isWide)
	{
		String tmp (str.text8 ());
		if (tmp.toWideString () == false)
			return -1;
		return compareAt (index, tmp, n, mode);
	}

	String tmp (text8 ());
	if (tmp.toWideString () == false)
		return 1;
	return tmp.compareAt (index, str, n, mode);
}

//------------------------------------------------------------------------
Steinberg::int32 ConstString::naturalCompare (const ConstString& str,
                                              CompareMode mode /*= kCaseSensitive*/) const
{
	if (str.isEmpty ())
	{
		if (isEmpty ())
			return 0;
		return 1;
	}
	if (isEmpty ())
		return -1;

	if (!isWide && !str.isWide)
		return strnatcmp8 (buffer8, str.text8 (), isCaseSensitive (mode));
	if (isWide && str.isWide)
		return strnatcmp16 (buffer16, str.text16 (), isCaseSensitive (mode));

	if (isWide)
	{
		String tmp (str.text8 ());
		tmp.toWideString ();
		return strnatcmp16 (buffer16, tmp.text16 (), isCaseSensitive (mode));
	}
	String tmp (text8 ());
	tmp.toWideString ();
	return strnatcmp16 (tmp.text16 (), str.text16 (), isCaseSensitive (mode));
}

//-----------------------------------------------------------------------------
bool ConstString::startsWith (const ConstString& str, CompareMode mode /*= kCaseSensitive*/) const
{
	if (str.isEmpty ())
	{
		return isEmpty ();
	}
	if (isEmpty ())
	{
		return false;
	}
	if (length () < str.length ())
	{
		return false;
	}
	if (!isWide && !str.isWide)
	{
		if (isCaseSensitive (mode))
			return strncmp (buffer8, str.buffer8, str.length ()) == 0;
		return strnicmp (buffer8, str.buffer8, str.length ()) == 0;
	}
	if (isWide && str.isWide)
	{
		if (isCaseSensitive (mode))
			return strncmp16 (buffer16, str.buffer16, str.length ()) == 0;
		return strnicmp16 (buffer16, str.buffer16, str.length ()) == 0;
	}
	if (isWide)
	{
		String tmp (str.text8 ());
		tmp.toWideString ();
		if (tmp.length () > length ())
			return false;
		if (isCaseSensitive (mode))
			return strncmp16 (buffer16, tmp.buffer16, tmp.length ()) == 0;
		return strnicmp16 (buffer16, tmp.buffer16, tmp.length ()) == 0;
	}
	String tmp (text8 ());
	tmp.toWideString ();
	if (str.length () > tmp.length ())
		return false;
	if (isCaseSensitive (mode))
		return strncmp16 (tmp.buffer16, str.buffer16, str.length ()) == 0;
	return strnicmp16 (tmp.buffer16, str.buffer16, str.length ()) == 0;
}

//-----------------------------------------------------------------------------
bool ConstString::endsWith (const ConstString& str, CompareMode mode /*= kCaseSensitive*/) const
{
	if (str.isEmpty ())
	{
		return isEmpty ();
	}
	if (isEmpty ())
	{
		return false;
	}
	if (length () < str.length ())
	{
		return false;
	}
	if (!isWide && !str.isWide)
	{
		if (isCaseSensitive (mode))
			return strncmp (buffer8 + (length () - str.length ()), str.buffer8, str.length ()) == 0;
		return strnicmp (buffer8 + (length () - str.length ()), str.buffer8, str.length ()) == 0;
	}
	if (isWide && str.isWide)
	{
		if (isCaseSensitive (mode))
			return strncmp16 (buffer16 + (length () - str.length ()), str.buffer16,
			                  str.length ()) == 0;
		return strnicmp16 (buffer16 + (length () - str.length ()), str.buffer16, str.length ()) ==
		       0;
	}
	if (isWide)
	{
		String tmp (str.text8 ());
		tmp.toWideString ();
		if (tmp.length () > length ())
			return false;
		if (isCaseSensitive (mode))
			return strncmp16 (buffer16 + (length () - tmp.length ()), tmp.buffer16,
			                  tmp.length ()) == 0;
		return strnicmp16 (buffer16 + (length () - tmp.length ()), tmp.buffer16, tmp.length ()) ==
		       0;
	}
	String tmp (text8 ());
	tmp.toWideString ();
	if (str.length () > tmp.length ())
		return false;
	if (isCaseSensitive (mode))
		return strncmp16 (tmp.buffer16 + (tmp.length () - str.length ()), str.buffer16,
		                  str.length ()) == 0;
	return strnicmp16 (tmp.buffer16 + (tmp.length () - str.length ()), str.buffer16,
	                   str.length ()) == 0;
}

//-----------------------------------------------------------------------------
bool ConstString::contains (const ConstString& str, CompareMode m) const
{
	return findFirst (str, -1, m) != -1;
}

//-----------------------------------------------------------------------------
int32 ConstString::findNext (int32 startIndex, const ConstString& str, int32 n, CompareMode mode,
                             int32 endIndex) const
{
	uint32 endLength = len;
	if (endIndex > -1 && (uint32)endIndex < len)
		endLength = endIndex + 1;

	if (isWide && str.isWide)
	{
		if (startIndex < 0)
			startIndex = 0;

		uint32 stringLength = str.length ();
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

		if (n > 0)
		{
			uint32 i = 0;

			if (isCaseSensitive (mode))
			{
				for (i = startIndex; i < endLength; i++)
					if (strncmp16 (buffer16 + i, str, n) == 0)
						return i;
			}
			else
			{
				for (i = startIndex; i < endLength; i++)
					if (strnicmp16 (buffer16 + i, str, n) == 0)
						return i;
			}
		}
		return -1;
	}
	if (!isWide && !str.isWide)
	{
		uint32 stringLength = str.length ();
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

		if (startIndex < 0)
			startIndex = 0;

		if (n > 0)
		{
			uint32 i = 0;

			if (isCaseSensitive (mode))
			{
				for (i = startIndex; i < endLength; i++)
					if (strncmp (buffer8 + i, str, n) == 0)
						return i;
			}
			else
			{
				for (i = startIndex; i < endLength; i++)
					if (strnicmp (buffer8 + i, str, n) == 0)
						return i;
			}
		}
		return -1;
	}
	String tmp;
	if (isWide)
	{
		tmp = str.text8 ();
		tmp.toWideString ();
		return findNext (startIndex, tmp, n, mode, endIndex);
	}
	tmp = text8 ();
	tmp.toWideString ();
	return tmp.findNext (startIndex, str, n, mode, endIndex);
}

//------------------------------------------------------------------------------------------------
int32 ConstString::findNext (int32 startIndex, char8 c, CompareMode mode, int32 endIndex) const
{
	uint32 endLength = len;
	if (endIndex > -1 && (uint32)endIndex < len)
		endLength = endIndex + 1;

	if (isWide)
	{
		char8 src[] = {c, 0};
		char16 dest[8] = {0};
		if (multiByteToWideString (dest, src, 2) > 0)
			return findNext (startIndex, dest[0], mode, endIndex);
		return -1;
	}

	if (startIndex < 0)
		startIndex = 0;
	uint32 i;

	if (isCaseSensitive (mode))
	{
		for (i = startIndex; i < endLength; i++)
		{
			if (buffer8[i] == c)
				return i;
		}
	}
	else
	{
		c = toLower (c);
		for (i = startIndex; i < endLength; i++)
		{
			if (toLower (buffer8[i]) == c)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
int32 ConstString::findNext (int32 startIndex, char16 c, CompareMode mode, int32 endIndex) const
{
	uint32 endLength = len;
	if (endIndex > -1 && (uint32)endIndex < len)
		endLength = endIndex + 1;

	if (!isWide)
	{
		char16 src[] = {c, 0};
		char8 dest[8] = {0};
		if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
			return findNext (startIndex, dest[0], mode, endIndex);

		return -1;
	}

	uint32 i;
	if (startIndex < 0)
		startIndex = 0;

	if (isCaseSensitive (mode))
	{
		for (i = startIndex; i < endLength; i++)
		{
			if (buffer16[i] == c)
				return i;
		}
	}
	else
	{
		c = toLower (c);
		for (i = startIndex; i < endLength; i++)
		{
			if (toLower (buffer16[i]) == c)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
int32 ConstString::findPrev (int32 startIndex, char8 c, CompareMode mode) const
{
	if (len == 0)
		return -1;

	if (isWide)
	{
		char8 src[] = {c, 0};
		char16 dest[8] = {0};
		if (multiByteToWideString (dest, src, 2) > 0)
			return findPrev (startIndex, dest[0], mode);
		return -1;
	}

	if (startIndex < 0 || startIndex > (int32)len)
		startIndex = len;

	int32 i;

	if (isCaseSensitive (mode))
	{
		for (i = startIndex; i >= 0; i--)
		{
			if (buffer8[i] == c)
				return i;
		}
	}
	else
	{
		c = toLower (c);
		for (i = startIndex; i >= 0; i--)
		{
			if (toLower (buffer8[i]) == c)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
int32 ConstString::findPrev (int32 startIndex, char16 c, CompareMode mode) const
{
	if (len == 0)
		return -1;

	if (!isWide)
	{
		char16 src[] = {c, 0};
		char8 dest[8] = {0};
		if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
			return findPrev (startIndex, dest[0], mode);

		return -1;
	}

	if (startIndex < 0 || startIndex > (int32)len)
		startIndex = len;

	int32 i;

	if (isCaseSensitive (mode))
	{
		for (i = startIndex; i >= 0; i--)
		{
			if (buffer16[i] == c)
				return i;
		}
	}
	else
	{
		c = toLower (c);
		for (i = startIndex; i >= 0; i--)
		{
			if (toLower (buffer16[i]) == c)
				return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
int32 ConstString::findPrev (int32 startIndex, const ConstString& str, int32 n,
                             CompareMode mode) const
{
	if (isWide && str.isWide)
	{
		uint32 stringLength = str.length ();
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

		if (startIndex < 0 || startIndex >= (int32)len)
			startIndex = len - 1;

		if (n > 0)
		{
			int32 i = 0;

			if (isCaseSensitive (mode))
			{
				for (i = startIndex; i >= 0; i--)
					if (strncmp16 (buffer16 + i, str, n) == 0)
						return i;
			}
			else
			{
				for (i = startIndex; i >= 0; i--)
					if (strnicmp16 (buffer16 + i, str, n) == 0)
						return i;
			}
		}
		return -1;
	}
	if (!isWide && !str.isWide)
	{
		uint32 stringLength = str.length ();
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

		if (startIndex < 0 || startIndex >= (int32)len)
			startIndex = len - 1;

		if (n > 0)
		{
			int32 i = 0;

			if (isCaseSensitive (mode))
			{
				for (i = startIndex; i >= 0; i--)
					if (strncmp (buffer8 + i, str, n) == 0)
						return i;
			}
			else
			{
				for (i = startIndex; i >= 0; i--)
					if (strnicmp (buffer8 + i, str, n) == 0)
						return i;
			}
		}
		return -1;
	}
	if (isWide)
	{
		String tmp (str.text8 ());
		tmp.toWideString ();
		return findPrev (startIndex, tmp, n, mode);
	}
	String tmp (text8 ());
	tmp.toWideString ();
	return tmp.findPrev (startIndex, str, n, mode);
}

//-----------------------------------------------------------------------------
int32 ConstString::countOccurences (char8 c, uint32 startIndex, CompareMode mode) const
{
	if (isWide)
	{
		char8 src[] = {c, 0};
		char16 dest[8] = {0};
		if (multiByteToWideString (dest, src, 2) > 0)
			return countOccurences (dest[0], startIndex, mode);
		return -1;
	}

	int32 result = 0;
	int32 next = startIndex;
	while (true)
	{
		next = findNext (next, c, mode);
		if (next >= 0)
		{
			next++;
			result++;
		}
		else
			break;
	}
	return result;
}

//-----------------------------------------------------------------------------
int32 ConstString::countOccurences (char16 c, uint32 startIndex, CompareMode mode) const
{
	if (!isWide)
	{
		char16 src[] = {c, 0};
		char8 dest[8] = {0};
		if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
			return countOccurences (dest[0], startIndex, mode);

		return -1;
	}
	int32 result = 0;
	int32 next = startIndex;
	while (true)
	{
		next = findNext (next, c, mode);
		if (next >= 0)
		{
			next++;
			result++;
		}
		else
			break;
	}
	return result;
}

//-----------------------------------------------------------------------------
int32 ConstString::getFirstDifferent (const ConstString& str, CompareMode mode) const
{
	if (str.isWide != isWide)
	{
		if (isWide)
		{
			String tmp (str.text8 ());
			if (tmp.toWideString () == false)
				return -1;
			return getFirstDifferent (tmp, mode);
		}

		String tmp (text8 ());
		if (tmp.toWideString () == false)
			return -1;
		return tmp.getFirstDifferent (str, mode);
	}

	uint32 len1 = len;
	uint32 len2 = str.len;
	uint32 i;

	if (isWide)
	{
		if (isCaseSensitive (mode))
		{
			for (i = 0; i <= len1 && i <= len2; i++)
			{
				if (buffer16[i] != str.buffer16[i])
					return i;
			}
		}
		else
		{
			for (i = 0; i <= len1 && i <= len2; i++)
			{
				if (toLower (buffer16[i]) != toLower (str.buffer16[i]))
					return i;
			}
		}
	}
	else
	{
		if (isCaseSensitive (mode))
		{
			for (i = 0; i <= len1 && i <= len2; i++)
			{
				if (buffer8[i] != str.buffer8[i])
					return i;
			}
		}
		else
		{
			for (i = 0; i <= len1 && i <= len2; i++)
			{
				if (toLower (buffer8[i]) != toLower (str.buffer8[i]))
					return i;
			}
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
bool ConstString::scanInt64 (int64& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	if (isWide)
		return scanInt64_16 (buffer16 + offset, value, scanToEnd);
	return scanInt64_8 (buffer8 + offset, value, scanToEnd);
}

//-----------------------------------------------------------------------------
bool ConstString::scanUInt64 (uint64& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	if (isWide)
		return scanUInt64_16 (buffer16 + offset, value, scanToEnd);
	return scanUInt64_8 (buffer8 + offset, value, scanToEnd);
}

//-----------------------------------------------------------------------------
bool ConstString::scanHex (uint8& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	if (isWide)
		return scanHex_16 (buffer16 + offset, value, scanToEnd);
	return scanHex_8 (buffer8 + offset, value, scanToEnd);
}

//-----------------------------------------------------------------------------
bool ConstString::scanInt32 (int32& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	if (isWide)
		return scanInt32_16 (buffer16 + offset, value, scanToEnd);
	return scanInt32_8 (buffer8 + offset, value, scanToEnd);
}

//-----------------------------------------------------------------------------
bool ConstString::scanUInt32 (uint32& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	if (isWide)
		return scanUInt32_16 (buffer16 + offset, value, scanToEnd);
	return scanUInt32_8 (buffer8 + offset, value, scanToEnd);
}

//-----------------------------------------------------------------------------
bool ConstString::scanInt64_8 (const char8* text, int64& value, bool scanToEnd)
{
	while (text && text[0])
	{
		if (sscanf (text, "%" FORMAT_INT64A, &value) == 1)
			return true;
		if (scanToEnd == false)
			return false;
		text++;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanInt64_16 (const char16* text, int64& value, bool scanToEnd)
{
	if (text && text[0])
	{
		String str (text);
		str.toMultiByte (kCP_Default);
		return scanInt64_8 (str, value, scanToEnd);
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanUInt64_8 (const char8* text, uint64& value, bool scanToEnd)
{
	while (text && text[0])
	{
		if (sscanf (text, "%" FORMAT_UINT64A, &value) == 1)
			return true;
		if (scanToEnd == false)
			return false;
		text++;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanUInt64_16 (const char16* text, uint64& value, bool scanToEnd)
{
	if (text && text[0])
	{
		String str (text);
		str.toMultiByte (kCP_Default);
		return scanUInt64_8 (str, value, scanToEnd);
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanInt64 (const tchar* text, int64& value, bool scanToEnd)
{
#ifdef UNICODE
	return scanInt64_16 (text, value, scanToEnd);
#else
	return scanInt64_8 (text, value, scanToEnd);
#endif
}

//-----------------------------------------------------------------------------
bool ConstString::scanUInt64 (const tchar* text, uint64& value, bool scanToEnd)
{
#ifdef UNICODE
	return scanUInt64_16 (text, value, scanToEnd);
#else
	return scanUInt64_8 (text, value, scanToEnd);
#endif
}

//-----------------------------------------------------------------------------
bool ConstString::scanHex_8 (const char8* text, uint8& value, bool scanToEnd)
{
	while (text && text[0])
	{
		unsigned int v; // scanf expects an unsigned int for %x
		if (sscanf (text, "%x", &v) == 1)
		{
			value = (uint8)v;
			return true;
		}
		if (scanToEnd == false)
			return false;
		text++;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanHex_16 (const char16* text, uint8& value, bool scanToEnd)
{
	if (text && text[0])
	{
		String str (text);
		str.toMultiByte (kCP_Default); // scanf uses default codepage
		return scanHex_8 (str, value, scanToEnd);
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::scanHex (const tchar* text, uint8& value, bool scanToEnd)
{
#ifdef UNICODE
	return scanHex_16 (text, value, scanToEnd);
#else
	return scanHex_8 (text, value, scanToEnd);
#endif
}

//-----------------------------------------------------------------------------
bool ConstString::scanFloat (double& value, uint32 offset, bool scanToEnd) const
{
	if (isEmpty () || offset >= len)
		return false;

	String str (*this);
	int32 pos = -1;
	if (isWide)
	{
		if ((pos = str.findNext (offset, STR (','))) >= 0 && ((uint32)pos) >= offset)
			str.setChar (pos, STR ('.'));

		str.toMultiByte (kCP_Default); // scanf uses default codepage
	}
	else
	{
		if ((pos = str.findNext (offset, ',')) >= 0 && ((uint32)pos) >= offset)
			str.setChar (pos, '.');
	}

	const char8* txt = str.text8 () + offset;
	while (txt && txt[0])
	{
		if (sscanf (txt, "%lf", &value) == 1)
			return true;
		if (scanToEnd == false)
			return false;
		txt++;
	}
	return false;
}

//-----------------------------------------------------------------------------
char16 ConstString::toLower (char16 c)
{
#if SMTG_OS_WINDOWS
	WCHAR temp[2] = {c, 0};
	::CharLowerW (temp);
	return temp[0];
#elif SMTG_OS_MACOS
	// only convert characters which in lowercase are also single characters
	UniChar characters[2] = {0};
	characters[0] = c;
	CFMutableStringRef str = CFStringCreateMutableWithExternalCharactersNoCopy (
	    kCFAllocator, characters, 1, 2, kCFAllocatorNull);
	if (str)
	{
		CFStringLowercase (str, NULL);
		CFRelease (str);
		if (characters[1] == 0)
			return characters[0];
	}
	return c;
#elif SMTG_OS_LINUX
	assert (false && "DEPRECATED No Linux implementation");
	return c;
#else
	return towlower (c);
#endif
}

//-----------------------------------------------------------------------------
char16 ConstString::toUpper (char16 c)
{
#if SMTG_OS_WINDOWS
	WCHAR temp[2] = {c, 0};
	::CharUpperW (temp);
	return temp[0];
#elif SMTG_OS_MACOS
	// only convert characters which in uppercase are also single characters (don't translate a
	// sharp-s which would result in SS)
	UniChar characters[2] = {0};
	characters[0] = c;
	CFMutableStringRef str = CFStringCreateMutableWithExternalCharactersNoCopy (
	    kCFAllocator, characters, 1, 2, kCFAllocatorNull);
	if (str)
	{
		CFStringUppercase (str, NULL);
		CFRelease (str);
		if (characters[1] == 0)
			return characters[0];
	}
	return c;
#elif SMTG_OS_LINUX
	assert (false && "DEPRECATED No Linux implementation");
	return c;
#else
	return towupper (c);
#endif
}

//-----------------------------------------------------------------------------
char8 ConstString::toLower (char8 c)
{
	if ((c >= 'A') && (c <= 'Z'))
		return c + ('a' - 'A');
#if SMTG_OS_WINDOWS
	CHAR temp[2] = {c, 0};
	::CharLowerA (temp);
	return temp[0];
#else
	return static_cast<char8> (tolower (c));
#endif
}

//-----------------------------------------------------------------------------
char8 ConstString::toUpper (char8 c)
{
	if ((c >= 'a') && (c <= 'z'))
		return c - ('a' - 'A');
#if SMTG_OS_WINDOWS
	CHAR temp[2] = {c, 0};
	::CharUpperA (temp);
	return temp[0];
#else
	return static_cast<char8> (toupper (c));
#endif
}

//-----------------------------------------------------------------------------
bool ConstString::isCharSpace (const char8 character)
{
	return isspace (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharSpace (const char16 character)
{
	switch (character)
	{
		case 0x0020:
		case 0x00A0:
		case 0x2002:
		case 0x2003:
		case 0x2004:
		case 0x2005:
		case 0x2006:
		case 0x2007:
		case 0x2008:
		case 0x2009:
		case 0x200A:
		case 0x200B:
		case 0x202F:
		case 0x205F:
		case 0x3000: return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAlpha (const char8 character)
{
	return isalpha (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAlpha (const char16 character)
{
	return iswalpha (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAlphaNum (const char8 character)
{
	return isalnum (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAlphaNum (const char16 character)
{
	// this may not work on macOSX when another locale is set inside the c-lib
	return iswalnum (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharDigit (const char8 character)
{
	return isdigit (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharDigit (const char16 character)
{
	// this may not work on macOSX when another locale is set inside the c-lib
	return iswdigit (character) != 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAscii (char8 character)
{
	return character >= 0;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharAscii (char16 character)
{
	return character < 128;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharUpper (char8 character)
{
	return toUpper (character) == character;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharUpper (char16 character)
{
	return toUpper (character) == character;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharLower (char8 character)
{
	return toLower (character) == character;
}

//-----------------------------------------------------------------------------
bool ConstString::isCharLower (char16 character)
{
	return toLower (character) == character;
}

//-----------------------------------------------------------------------------
bool ConstString::isDigit (uint32 index) const
{
	if (isEmpty () || index >= len)
		return false;

	if (isWide)
		return ConstString::isCharDigit (buffer16[index]);
	return ConstString::isCharDigit (buffer8[index]);
}

//-----------------------------------------------------------------------------
int32 ConstString::getTrailingNumberIndex (uint32 width) const
{
	if (isEmpty ())
		return -1;

	int32 endIndex = len - 1;
	int32 i = endIndex;
	while (isDigit ((uint32)i) && i >= 0)
		i--;

	// now either all are digits or i is on the first non digit
	if (i < endIndex)
	{
		if (width > 0 && (endIndex - i != static_cast<int32> (width)))
			return -1;

		return i + 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
int64 ConstString::getTrailingNumber (int64 fallback) const
{
	int32 index = getTrailingNumberIndex ();

	int64 number = 0;

	if (index >= 0)
		if (scanInt64 (number, index))
			return number;

	return fallback;
}

//-----------------------------------------------------------------------------
void ConstString::toVariant (FVariant& var) const
{
	if (isWide)
	{
		var.setString16 (buffer16);
	}
	else
	{
		var.setString8 (buffer8);
	}
}

//-----------------------------------------------------------------------------
bool ConstString::isAsciiString () const
{
	uint32 i;
	if (isWide)
	{
		for (i = 0; i < len; i++)
			if (ConstString::isCharAscii (buffer16[i]) == false)
				return false;
	}
	else
	{
		for (i = 0; i < len; i++)
			if (ConstString::isCharAscii (buffer8[i]) == false)
				return false;
	}
	return true;
}

#if SMTG_OS_MACOS
uint32 kDefaultSystemEncoding = kCFStringEncodingMacRoman;
//-----------------------------------------------------------------------------
static CFStringEncoding MBCodePageToCFStringEncoding (uint32 codePage)
{
	switch (codePage)
	{
		case kCP_ANSI:
			return kDefaultSystemEncoding; // MacRoman or JIS
		case kCP_MAC_ROMAN: return kCFStringEncodingMacRoman;
		case kCP_ANSI_WEL: return kCFStringEncodingWindowsLatin1;
		case kCP_MAC_CEE: return kCFStringEncodingMacCentralEurRoman;
		case kCP_Utf8: return kCFStringEncodingUTF8;
		case kCP_ShiftJIS: return kCFStringEncodingShiftJIS_X0213_00;
		case kCP_US_ASCII: return kCFStringEncodingASCII;
	}
	return kCFStringEncodingASCII;
}
#endif

//-----------------------------------------------------------------------------
int32 ConstString::multiByteToWideString (char16* dest, const char8* source, int32 charCount,
                                          uint32 sourceCodePage)
{
	if (source == nullptr || source[0] == 0)
	{
		if (dest && charCount > 0)
		{
			dest[0] = 0;
		}
		return 0;
	}
	int32 result = 0;
#if SMTG_OS_WINDOWS
	result = MultiByteToWideChar (sourceCodePage, MB_ERR_INVALID_CHARS, source, -1, wscast (dest),
	                              charCount);
#endif

#if SMTG_OS_MACOS
	CFStringRef cfStr =
	    (CFStringRef)::toCFStringRef (source, MBCodePageToCFStringEncoding (sourceCodePage));
	if (cfStr)
	{
		CFRange range = {0, CFStringGetLength (cfStr)};
		CFIndex usedBytes;
		if (CFStringGetBytes (cfStr, range, kCFStringEncodingUnicode, ' ', false, (UInt8*)dest,
		                      charCount * 2, &usedBytes) > 0)
		{
			result = static_cast<int32> (usedBytes / 2 + 1);
			if (dest)
				dest[usedBytes / 2] = 0;
		}

		CFRelease (cfStr);
	}
#endif

#if SMTG_OS_LINUX
	if (sourceCodePage == kCP_ANSI || sourceCodePage == kCP_US_ASCII || sourceCodePage == kCP_Utf8)
	{
		if (dest == nullptr)
		{
			auto state = std::mbstate_t ();
			auto maxChars = charCount ? charCount : std::numeric_limits<int32>::max () - 1;
			result = converterFacet ().length (state, source, source + strlen (source), maxChars);
		}
		else
		{
			auto utf16Str = converter ().from_bytes (source);
			if (!utf16Str.empty ())
			{
				result = std::min<int32> (charCount, utf16Str.size ());
				memcpy (dest, utf16Str.data (), result * sizeof (char16));
				dest[result] = 0;
			}
		}
	}
	else
	{
		assert (false && "DEPRECATED No Linux implementation");
	}

#endif

	SMTG_ASSERT (result > 0)
	return result;
}

//-----------------------------------------------------------------------------
int32 ConstString::wideStringToMultiByte (char8* dest, const char16* wideString, int32 charCount,
                                          uint32 destCodePage)
{
#if SMTG_OS_WINDOWS
	return WideCharToMultiByte (destCodePage, 0, wscast (wideString), -1, dest, charCount, nullptr,
	                            nullptr);

#elif SMTG_OS_MACOS
	int32 result = 0;
	if (wideString != 0)
	{
		if (dest)
		{
			CFStringRef cfStr = CFStringCreateWithCharactersNoCopy (
			    kCFAllocator, (const UniChar*)wideString, strlen16 (wideString), kCFAllocatorNull);
			if (cfStr)
			{
				if (fromCFStringRef (dest, charCount, cfStr,
				                     MBCodePageToCFStringEncoding (destCodePage)))
					result = static_cast<int32> (strlen (dest) + 1);
				CFRelease (cfStr);
			}
		}
		else
		{
			return static_cast<int32> (CFStringGetMaximumSizeForEncoding (
			    strlen16 (wideString), MBCodePageToCFStringEncoding (destCodePage)));
		}
	}
	return result;

#elif SMTG_OS_LINUX
	int32 result = 0;
	if (destCodePage == kCP_Utf8)
	{
		if (dest == nullptr)
		{
			auto maxChars = charCount ? charCount : tstrlen (wideString);
			result = converterFacet ().max_length () * maxChars;
		}
		else
		{
			auto utf8Str = converter ().to_bytes (wideString);
			if (!utf8Str.empty ())
			{
				result = std::min<int32> (charCount, utf8Str.size ());
				memcpy (dest, utf8Str.data (), result * sizeof (char8));
				dest[result] = 0;
			}
		}
	}
	else if (destCodePage == kCP_ANSI || destCodePage == kCP_US_ASCII)
	{
		if (dest == nullptr)
		{
			result = strlen16 (wideString) + 1;
		}
		else
		{
			int32 i = 0;
			for (; i < charCount; ++i)
			{
				if (wideString[i] == 0)
					break;
				if (wideString[i] <= 0x007F)
					dest[i] = wideString[i];
				else
					dest[i] = '_';
			}
			dest[i] = 0;
			result = i;
		}
	}
	else
	{
		assert (false && "DEPRECATED No Linux implementation");
	}
	return result;

#else
	assert (false && "DEPRECATED No Linux implementation");
	return 0;
#endif
}

//-----------------------------------------------------------------------------
bool ConstString::isNormalized (UnicodeNormalization n)
{
	if (isWide == false)
		return false;

#if SMTG_OS_WINDOWS
#ifdef UNICODE
	if (n != kUnicodeNormC)
		return false;
	uint32 normCharCount =
	    static_cast<uint32> (FoldString (MAP_PRECOMPOSED, wscast (buffer16), len, nullptr, 0));
	return (normCharCount == len);
#else
	return false;
#endif

#elif SMTG_OS_MACOS
	if (n != kUnicodeNormC)
		return false;

	CFStringRef cfStr = (CFStringRef)toCFStringRef ();
	CFIndex charCount = CFStringGetLength (cfStr);
	CFRelease (cfStr);
	return (charCount == len);
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
//	String
//-----------------------------------------------------------------------------
String::String ()
{
	isWide = kWideStringDefault ? 1 : 0;
}

//-----------------------------------------------------------------------------
String::String (const char8* str, MBCodePage codePage, int32 n, bool isTerminated)
{
	isWide = false;
	if (str)
	{
		if (isTerminated && n >= 0 && str[n] != 0)
		{
			// isTerminated is not always set correctly
			isTerminated = false;
		}

		if (!isTerminated)
		{
			assign (str, n, isTerminated);
			toWideString (codePage);
		}
		else
		{
			if (n < 0)
				n = static_cast<int32> (strlen (str));
			if (n > 0)
				_toWideString (str, n, codePage);
		}
	}
}

//-----------------------------------------------------------------------------
String::String (const char8* str, int32 n, bool isTerminated)
{
	if (str)
		assign (str, n, isTerminated);
}

//-----------------------------------------------------------------------------
String::String (const char16* str, int32 n, bool isTerminated)
{
	isWide = 1;
	if (str)
		assign (str, n, isTerminated);
}

//-----------------------------------------------------------------------------
String::String (const String& str, int32 n)
{
	isWide = str.isWideString ();
	if (!str.isEmpty ())
		assign (str, n);
}

//-----------------------------------------------------------------------------
String::String (const ConstString& str, int32 n)
{
	isWide = str.isWideString ();
	if (!str.isEmpty ())
		assign (str, n);
}

//-----------------------------------------------------------------------------
String::String (const FVariant& var)
{
	isWide = kWideStringDefault ? 1 : 0;
	fromVariant (var);
}

//-----------------------------------------------------------------------------
String::String (IString* str)
{
	isWide = str->isWideString ();
	if (isWide)
		assign (str->getText16 ());
	else
		assign (str->getText8 ());
}

//-----------------------------------------------------------------------------
String::~String ()
{
	if (buffer)
		resize (0, false);
}

#if SMTG_CPP11_STDLIBSUPPORT
//-----------------------------------------------------------------------------
String::String (String&& str)
{
	*this = std::move (str);
}

//-----------------------------------------------------------------------------
String& String::operator= (String&& str)
{
	SMTG_ASSERT (buffer == nullptr || buffer != str.buffer);
	tryFreeBuffer ();

	isWide = str.isWide;
	buffer = str.buffer;
	len = str.len;
	str.buffer = nullptr;
	str.len = 0;
	return *this;
}
#endif

//-----------------------------------------------------------------------------
void String::updateLength ()
{
	if (isWide)
		len = strlen16 (text16 ());
	else
		len = strlen8 (text8 ());
}

//-----------------------------------------------------------------------------
bool String::toWideString (uint32 sourceCodePage)
{
	if (!isWide && buffer8 && len > 0)
		return _toWideString (buffer8, len, sourceCodePage);
	isWide = true;
	return true;
}

//-----------------------------------------------------------------------------
bool String::_toWideString (const char8* src, int32 length, uint32 sourceCodePage)
{
	if (!isWide)
	{
		if (src && length > 0)
		{
			int32 bytesNeeded =
			    multiByteToWideString (nullptr, src, 0, sourceCodePage) * sizeof (char16);
			if (bytesNeeded)
			{
				bytesNeeded += sizeof (char16);
				char16* newStr = (char16*)malloc (bytesNeeded);
				if (multiByteToWideString (newStr, src, length + 1, sourceCodePage) < 0)
				{
					free (newStr);
					return false;
				}
				if (buffer8)
					free (buffer8);

				buffer16 = newStr;
				isWide = true;
				updateLength ();
			}
			else
			{
				return false;
			}
		}
		isWide = true;
	}
	return true;
}

#define SMTG_STRING_CHECK_CONVERSION 1
#define SMTG_STRING_CHECK_CONVERSION_NO_BREAK 1

#if SMTG_STRING_CHECK_CONVERSION_NO_BREAK
#define SMTG_STRING_CHECK_MSG FDebugPrint
#else
#define SMTG_STRING_CHECK_MSG FDebugBreak
#endif
//-----------------------------------------------------------------------------
bool String::checkToMultiByte (uint32 destCodePage) const
{
	if (!isWide || isEmpty ())
		return true;

#if DEVELOPMENT && SMTG_STRING_CHECK_CONVERSION
	int debugLen = length ();
	int debugNonASCII = 0;
	for (int32 i = 0; i < length (); i++)
	{
		if (buffer16[i] > 127)
			++debugNonASCII;
	}

	String* backUp = nullptr;
	if (debugNonASCII > 0)
		backUp = NEW String (*this);
#endif

	// this should be avoided, since it can lead to information loss
	bool result = const_cast<String&> (*this).toMultiByte (destCodePage);

#if DEVELOPMENT && SMTG_STRING_CHECK_CONVERSION
	if (backUp)
	{
		String temp (*this);
		temp.toWideString (destCodePage);

		if (temp != *backUp)
		{
			backUp->toMultiByte (kCP_Utf8);
			SMTG_STRING_CHECK_MSG ("Indirect string conversion information loss !   %d/%d non ASCII chars:   \"%s\"   ->    \"%s\"\n", debugNonASCII, debugLen, backUp->buffer8, buffer8);
		}
		else
			SMTG_STRING_CHECK_MSG ("Indirect string potential conversion information loss !   %d/%d non ASCII chars   result: \"%s\"\n", debugNonASCII, debugLen, buffer8);

		delete backUp;
	}
#endif

	return result;
}

//-----------------------------------------------------------------------------
bool String::toMultiByte (uint32 destCodePage)
{
	if (isWide)
	{
		if (buffer16 && len > 0)
		{
			int32 numChars =
			    wideStringToMultiByte (nullptr, buffer16, 0, destCodePage) + sizeof (char8);
			char8* newStr = (char8*)malloc (numChars * sizeof (char8));
			if (wideStringToMultiByte (newStr, buffer16, numChars, destCodePage) <= 0)
			{
				free (newStr);
				return false;
			}
			free (buffer16);
			buffer8 = newStr;
			isWide = false;
			updateLength ();
		}
		isWide = false;
	}
	else if (destCodePage != kCP_Default)
	{
		if (toWideString () == false)
			return false;
		return toMultiByte (destCodePage);
	}
	return true;
}

//-----------------------------------------------------------------------------
void String::fromUTF8 (const char8* utf8String)
{
	if (buffer8 != utf8String)
		resize (0, false);
	_toWideString (utf8String, static_cast<int32> (strlen (utf8String)), kCP_Utf8);
}

//-----------------------------------------------------------------------------
bool String::normalize (UnicodeNormalization n)
{
	if (isWide == false)
		return false;

	if (buffer16 == nullptr)
		return true;

#if SMTG_OS_WINDOWS
#ifdef UNICODE
	if (n != kUnicodeNormC)
		return false;

	uint32 normCharCount =
	    static_cast<uint32> (FoldString (MAP_PRECOMPOSED, wscast (buffer16), len, nullptr, 0));
	if (normCharCount == len)
		return true;

	char16* newString = (char16*)malloc ((normCharCount + 1) * sizeof (char16));
	uint32 converterCount = static_cast<uint32> (FoldString (
	    MAP_PRECOMPOSED, wscast (buffer16), len, wscast (newString), normCharCount + 1));
	if (converterCount != normCharCount)
	{
		free (newString);
		return false;
	}
	newString[converterCount] = 0;
	free (buffer16);
	buffer16 = newString;
	updateLength ();
	return true;
#else
	return false;
#endif

#elif SMTG_OS_MACOS
	CFMutableStringRef origStr = (CFMutableStringRef)toCFStringRef (0xFFFF, true);
	if (origStr)
	{
		CFStringNormalizationForm normForm = kCFStringNormalizationFormD;
		switch (n)
		{
			case kUnicodeNormC: normForm = kCFStringNormalizationFormC; break;
			case kUnicodeNormD: normForm = kCFStringNormalizationFormD; break;
			case kUnicodeNormKC: normForm = kCFStringNormalizationFormKC; break;
			case kUnicodeNormKD: normForm = kCFStringNormalizationFormKD; break;
		}
		CFStringNormalize (origStr, normForm);
		bool result = fromCFStringRef (origStr);
		CFRelease (origStr);
		return result;
	}
	return false;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
void String::tryFreeBuffer ()
{
	if (buffer)
	{
		free (buffer);
		buffer = nullptr;
	}
}

//-----------------------------------------------------------------------------
bool String::resize (uint32 newLength, bool wide, bool fill)
{
	if (newLength == 0)
	{
		tryFreeBuffer ();
		len = 0;
		isWide = wide ? 1 : 0;
	}
	else
	{
		size_t newCharSize = wide ? sizeof (char16) : sizeof (char8);
		size_t oldCharSize = (isWide != 0) ? sizeof (char16) : sizeof (char8);

		size_t newBufferSize = (newLength + 1) * newCharSize;
		size_t oldBufferSize = (len + 1) * oldCharSize;

		isWide = wide ? 1 : 0;

		if (buffer)
		{
			if (newBufferSize != oldBufferSize)
			{
				void* newstr = realloc (buffer, newBufferSize);
				if (newstr == nullptr)
					return false;
				buffer = newstr;
				if (isWide)
					buffer16[newLength] = 0;
				else
					buffer8[newLength] = 0;
			}
			else if (wide && newCharSize != oldCharSize)
				buffer16[newLength] = 0;
		}
		else
		{
			void* newstr = malloc (newBufferSize);
			if (newstr == nullptr)
				return false;
			buffer = newstr;
			if (isWide)
			{
				buffer16[0] = 0;
				buffer16[newLength] = 0;
			}
			else
			{
				buffer8[0] = 0;
				buffer8[newLength] = 0;
			}
		}

		if (fill && len < newLength && buffer)
		{
			if (isWide)
			{
				char16 c = ' ';
				for (uint32 i = len; i < newLength; i++)
					buffer16[i] = c;
			}
			else
			{
				memset (buffer8 + len, ' ', newLength - len);
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
bool String::setChar8 (uint32 index, char8 c)
{
	if (index == len && c == 0)
		return true;

	if (index >= len)
	{
		if (c == 0)
		{
			if (resize (index, isWide, true) == false)
				return false;
			len = index;
			return true;
		}

		if (resize (index + 1, isWide, true) == false)
			return false;
		len = index + 1;
	}

	if (index < len && buffer)
	{
		if (isWide)
		{
			if (c == 0)
				buffer16[index] = 0;
			else
			{
				char8 src[] = {c, 0};
				char16 dest[8] = {0};
				if (multiByteToWideString (dest, src, 2) > 0)
					buffer16[index] = dest[0];
			}
			SMTG_ASSERT (buffer16[len] == 0)
		}
		else
		{
			buffer8[index] = c;
			SMTG_ASSERT (buffer8[len] == 0)
		}

		if (c == 0)
			updateLength ();

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool String::setChar16 (uint32 index, char16 c)
{
	if (index == len && c == 0)
		return true;

	if (index >= len)
	{
		if (c == 0)
		{
			if (resize (index, isWide, true) == false)
				return false;
			len = index;
			return true;
		}
		if (resize (index + 1, isWide, true) == false)
			return false;
		len = index + 1;
	}

	if (index < len && buffer)
	{
		if (isWide)
		{
			buffer16[index] = c;
			SMTG_ASSERT (buffer16[len] == 0)
		}
		else
		{
			SMTG_ASSERT (buffer8[len] == 0)
			char16 src[] = {c, 0};
			char8 dest[8] = {0};
			if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
				buffer8[index] = dest[0];
			else
				return false;
		}

		if (c == 0)
			updateLength ();

		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
String& String::assign (const ConstString& str, int32 n)
{
	if (str.isWideString ())
		return assign (str.text16 (), n < 0 ? str.length () : n);
	return assign (str.text8 (), n < 0 ? str.length () : n);
}

//-----------------------------------------------------------------------------
String& String::assign (const char8* str, int32 n, bool isTerminated)
{
	if (str == buffer8)
		return *this;

	if (isTerminated)
	{
		uint32 stringLength = (uint32) ((str) ? strlen (str) : 0);
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);
	}
	else if (n < 0)
		return *this;

	if (resize (n, false))
	{
		if (buffer8 && n > 0 && str)
		{
			memcpy (buffer8, str, n * sizeof (char8));
			SMTG_ASSERT (buffer8[n] == 0)
		}
		isWide = 0;
		len = n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::assign (const char16* str, int32 n, bool isTerminated)
{
	if (str == buffer16)
		return *this;

	if (isTerminated)
	{
		uint32 stringLength = (uint32) ((str) ? strlen16 (str) : 0);
		n = n < 0 ? stringLength : Min<uint32> (n, stringLength);
	}
	else if (n < 0)
		return *this;

	if (resize (n, true))
	{
		if (buffer16 && n > 0 && str)
		{
			memcpy (buffer16, str, n * sizeof (char16));
			SMTG_ASSERT (buffer16[n] == 0)
		}
		isWide = 1;
		len = n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::assign (char8 c, int32 n)
{
	if (resize (n, false))
	{
		if (buffer8 && n > 0)
		{
			memset (buffer8, c, n * sizeof (char8));
			SMTG_ASSERT (buffer8[n] == 0)
		}
		isWide = 0;
		len = n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::assign (char16 c, int32 n)
{
	if (resize (n, true))
	{
		if (buffer && n > 0)
		{
			for (int32 i = 0; i < n; i++)
				buffer16[i] = c;
			SMTG_ASSERT (buffer16[n] == 0)
		}
		isWide = 1;
		len = n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::append (const ConstString& str, int32 n)
{
	if (str.isWideString ())
		return append (str.text16 (), n);
	return append (str.text8 (), n);
}

//-----------------------------------------------------------------------------
String& String::append (const char8* str, int32 n)
{
	if (str == buffer8)
		return *this;

	if (len == 0)
		return assign (str, n);

	if (isWide)
	{
		String tmp (str);
		if (tmp.toWideString () == false)
			return *this;

		return append (tmp.buffer16, n);
	}

	uint32 stringLength = (uint32) ((str) ? strlen (str) : 0);
	n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

	if (n > 0)
	{
		int32 newlen = n + len;
		if (!resize (newlen, false))
			return *this;

		if (buffer && str)
		{
			memcpy (buffer8 + len, str, n * sizeof (char8));
			SMTG_ASSERT (buffer8[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::append (const char16* str, int32 n)
{
	if (str == buffer16)
		return *this;

	if (len == 0)
		return assign (str, n);

	if (!isWide)
	{
		if (toWideString () == false)
			return *this;
	}

	uint32 stringLength = (uint32) ((str) ? strlen16 (str) : 0);
	n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

	if (n > 0)
	{
		int32 newlen = n + len;
		if (!resize (newlen, true))
			return *this;

		if (buffer16 && str)
		{
			memcpy (buffer16 + len, str, n * sizeof (char16));
			SMTG_ASSERT (buffer16[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::append (const char8 c, int32 n)
{
	char8 str[] = {c, 0};
	if (n == 1)
	{
		return append (str, 1);
	}
	if (n > 1)
	{
		if (isWide)
		{
			String tmp (str);
			if (tmp.toWideString () == false)
				return *this;

			return append (tmp.buffer16[0], n);
		}

		int32 newlen = n + len;
		if (!resize (newlen, false))
			return *this;

		if (buffer)
		{
			memset (buffer8 + len, c, n * sizeof (char8));
			SMTG_ASSERT (buffer8[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::append (const char16 c, int32 n)
{
	if (n == 1)
	{
		char16 str[] = {c, 0};
		return append (str, 1);
	}
	if (n > 1)
	{
		if (!isWide)
		{
			if (toWideString () == false)
				return *this;
		}

		int32 newlen = n + len;
		if (!resize (newlen, true))
			return *this;

		if (buffer16)
		{
			for (int32 i = len; i < newlen; i++)
				buffer16[i] = c;
			SMTG_ASSERT (buffer16[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::insertAt (uint32 idx, const ConstString& str, int32 n)
{
	if (str.isWideString ())
		return insertAt (idx, str.text16 (), n);
	return insertAt (idx, str.text8 (), n);
}

//-----------------------------------------------------------------------------
String& String::insertAt (uint32 idx, const char8* str, int32 n)
{
	if (idx > len)
		return *this;

	if (isWide)
	{
		String tmp (str);
		if (tmp.toWideString () == false)
			return *this;
		return insertAt (idx, tmp.buffer16, n);
	}

	uint32 stringLength = (uint32) ((str) ? strlen (str) : 0);
	n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

	if (n > 0)
	{
		int32 newlen = len + n;
		if (!resize (newlen, false))
			return *this;

		if (buffer && str)
		{
			if (idx < len)
				memmove (buffer8 + idx + n, buffer8 + idx, (len - idx) * sizeof (char8));
			memcpy (buffer8 + idx, str, n * sizeof (char8));
			SMTG_ASSERT (buffer8[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::insertAt (uint32 idx, const char16* str, int32 n)
{
	if (idx > len)
		return *this;

	if (!isWide)
	{
		if (toWideString () == false)
			return *this;
	}

	uint32 stringLength = (uint32) ((str) ? strlen16 (str) : 0);
	n = n < 0 ? stringLength : Min<uint32> (n, stringLength);

	if (n > 0)
	{
		int32 newlen = len + n;
		if (!resize (newlen, true))
			return *this;

		if (buffer && str)
		{
			if (idx < len)
				memmove (buffer16 + idx + n, buffer16 + idx, (len - idx) * sizeof (char16));
			memcpy (buffer16 + idx, str, n * sizeof (char16));
			SMTG_ASSERT (buffer16[newlen] == 0)
		}

		len += n;
	}
	return *this;
}

//-----------------------------------------------------------------------------
String& String::replace (uint32 idx, int32 n1, const ConstString& str, int32 n2)
{
	if (str.isWideString ())
		return replace (idx, n1, str.text16 (), n2);
	return replace (idx, n1, str.text8 (), n2);
}

// "replace" replaces n1 number of characters at the specified index with
// n2 characters from the specified string.
//-----------------------------------------------------------------------------
String& String::replace (uint32 idx, int32 n1, const char8* str, int32 n2)
{
	if (idx > len || str == nullptr)
		return *this;

	if (isWide)
	{
		String tmp (str);
		if (tmp.toWideString () == false)
			return *this;
		if (tmp.length () == 0 || n2 == 0)
			return remove (idx, n1);
		return replace (idx, n1, tmp.buffer16, n2);
	}

	if (n1 < 0 || idx + n1 > len)
		n1 = len - idx;
	if (n1 == 0)
		return *this;

	uint32 stringLength = (uint32) ((str) ? strlen (str) : 0);
	n2 = n2 < 0 ? stringLength : Min<uint32> (n2, stringLength);

	uint32 newlen = len - n1 + n2;
	if (newlen > len)
		if (!resize (newlen, false))
			return *this;

	if (buffer)
	{
		memmove (buffer8 + idx + n2, buffer8 + idx + n1, (len - (idx + n1)) * sizeof (char8));
		memcpy (buffer8 + idx, str, n2 * sizeof (char8));

		// cannot be removed because resize is not called called in all cases (newlen > len)
		buffer8[newlen] = 0;
	}

	len = newlen;

	return *this;
}

//-----------------------------------------------------------------------------
String& String::replace (uint32 idx, int32 n1, const char16* str, int32 n2)
{
	if (idx > len || str == nullptr)
		return *this;

	if (!isWide)
	{
		if (toWideString () == false)
			return *this;
	}

	if (n1 < 0 || idx + n1 > len)
		n1 = len - idx;
	if (n1 == 0)
		return *this;

	uint32 stringLength = (uint32) ((str) ? strlen16 (str) : 0);
	n2 = n2 < 0 ? stringLength : Min<uint32> (n2, stringLength);

	uint32 newlen = len - n1 + n2;
	if (newlen > len)
		if (!resize (newlen, true))
			return *this;

	if (buffer)
	{
		memmove (buffer16 + idx + n2, buffer16 + idx + n1, (len - (idx + n1)) * sizeof (char16));
		memcpy (buffer16 + idx, str, n2 * sizeof (char16));

		// cannot be removed because resize is not called called in all cases (newlen > len)
		buffer16[newlen] = 0;
	}

	len = newlen;

	return *this;
}

//-----------------------------------------------------------------------------
int32 String::replace (const char8* toReplace, const char8* toReplaceWith, bool all, CompareMode m)
{
	if (toReplace == nullptr || toReplaceWith == nullptr)
		return 0;

	int32 result = 0;

	int32 idx = findFirst (toReplace, -1, m);
	if (idx > -1)
	{
		int32 toReplaceLen = static_cast<int32> (strlen (toReplace));
		int32 toReplaceWithLen = static_cast<int32> (strlen (toReplaceWith));
		while (idx > -1)
		{
			replace (idx, toReplaceLen, toReplaceWith, toReplaceWithLen);
			result++;

			if (all)
				idx = findNext (idx + toReplaceWithLen, toReplace, -1, m);
			else
				break;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
int32 String::replace (const char16* toReplace, const char16* toReplaceWith, bool all,
                       CompareMode m)
{
	if (toReplace == nullptr || toReplaceWith == nullptr)
		return 0;

	int32 result = 0;

	int32 idx = findFirst (toReplace, -1, m);
	if (idx > -1)
	{
		int32 toReplaceLen = strlen16 (toReplace);
		int32 toReplaceWithLen = strlen16 (toReplaceWith);
		while (idx > -1)
		{
			replace (idx, toReplaceLen, toReplaceWith, toReplaceWithLen);
			result++;

			if (all)
				idx = findNext (idx + toReplaceWithLen, toReplace, -1, m);
			else
				break;
		}
	}
	return result;
}

//-----------------------------------------------------------------------------
template <class T>
static bool performReplace (T* str, const T* toReplace, T toReplaceBy)
{
	bool anyReplace = false;
	T* p = str;
	while (*p)
	{
		const T* rep = toReplace;
		while (*rep)
		{
			if (*p == *rep)
			{
				*p = toReplaceBy;
				anyReplace = true;
				break;
			}
			rep++;
		}
		p++;
	}
	return anyReplace;
}

//-----------------------------------------------------------------------------
bool String::replaceChars8 (const char8* toReplace, char8 toReplaceBy)
{
	if (isEmpty ())
		return false;

	if (isWide)
	{
		String toReplaceW (toReplace);
		if (toReplaceW.toWideString () == false)
			return false;

		char8 src[] = {toReplaceBy, 0};
		char16 dest[2] = {0};
		if (multiByteToWideString (dest, src, 2) > 0)
		{
			return replaceChars16 (toReplaceW.text16 (), dest[0]);
		}
		return false;
	}

	if (toReplaceBy == 0)
		toReplaceBy = ' ';

	return performReplace<char8> (buffer8, toReplace, toReplaceBy);
}

//-----------------------------------------------------------------------------
bool String::replaceChars16 (const char16* toReplace, char16 toReplaceBy)
{
	if (isEmpty ())
		return false;

	if (!isWide)
	{
		String toReplaceA (toReplace);
		if (toReplaceA.toMultiByte () == false)
			return false;

		if (toReplaceA.length () > 1)
		{
			SMTG_WARNING ("cannot replace non ASCII chars on non Wide String")
			return false;
		}

		char16 src[] = {toReplaceBy, 0};
		char8 dest[8] = {0};
		if (wideStringToMultiByte (dest, src, 2) > 0 && dest[1] == 0)
			return replaceChars8 (toReplaceA.text8 (), dest[0]);

		return false;
	}

	if (toReplaceBy == 0)
		toReplaceBy = STR16 (' ');

	return performReplace<char16> (buffer16, toReplace, toReplaceBy);
}

// "remove" removes the specified number of characters from the string
// starting at the specified index.
//-----------------------------------------------------------------------------
String& String::remove (uint32 idx, int32 n)
{
	if (isEmpty () || idx >= len || n == 0)
		return *this;

	if ((idx + n > len) || n < 0)
		n = len - idx;
	else
	{
		int32 toMove = len - idx - n;
		if (buffer)
		{
			if (isWide)
				memmove (buffer16 + idx, buffer16 + idx + n, toMove * sizeof (char16));
			else
				memmove (buffer8 + idx, buffer8 + idx + n, toMove * sizeof (char8));
		}
	}

	resize (len - n, isWide);
	updateLength ();

	return *this;
}

//-----------------------------------------------------------------------------
bool String::removeSubString (const ConstString& subString, bool allOccurences)
{
	bool removed = false;
	while (!removed || allOccurences)
	{
		int32 idx = findFirst (subString);
		if (idx < 0)
			break;
		remove (idx, subString.length ());
		removed = true;
	}
	return removed;
}

//-----------------------------------------------------------------------------
template <class T, class F>
static uint32 performTrim (T* str, uint32 length, F func, bool funcResult)
{
	uint32 toRemoveAtHead = 0;
	uint32 toRemoveAtTail = 0;

	T* p = str;

	while ((*p) && ((func (*p) != 0) == funcResult))
		p++;

	toRemoveAtHead = static_cast<uint32> (p - str);

	if (toRemoveAtHead < length)
	{
		p = str + length - 1;

		while (((func (*p) != 0) == funcResult) && (p > str))
		{
			p--;
			toRemoveAtTail++;
		}
	}

	uint32 newLength = length - (toRemoveAtHead + toRemoveAtTail);
	if (newLength != length)
	{
		if (toRemoveAtHead)
			memmove (str, str + toRemoveAtHead, newLength * sizeof (T));
	}
	return newLength;
}

// "trim" trims the leading and trailing unwanted characters from the string.
//-----------------------------------------------------------------------------
bool String::trim (String::CharGroup group)
{
	if (isEmpty ())
		return false;

	uint32 newLength;

	switch (group)
	{
		case kSpace:
			if (isWide)
				newLength = performTrim<char16> (buffer16, len, iswspace, true);
			else
				newLength = performTrim<char8> (buffer8, len, isspace, true);
			break;

		case kNotAlphaNum:
			if (isWide)
				newLength = performTrim<char16> (buffer16, len, iswalnum, false);
			else
				newLength = performTrim<char8> (buffer8, len, isalnum, false);
			break;

		case kNotAlpha:
			if (isWide)
				newLength = performTrim<char16> (buffer16, len, iswalpha, false);
			else
				newLength = performTrim<char8> (buffer8, len, isalpha, false);
			break;

		default: // Undefined enum value
			return false;
	}

	if (newLength != len)
	{
		resize (newLength, isWide);
		len = newLength;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
template <class T, class F>
static uint32 performRemove (T* str, uint32 length, F func, bool funcResult)
{
	T* p = str;

	while (*p)
	{
		if ((func (*p) != 0) == funcResult)
		{
			size_t toMove = length - (p - str);
			memmove (p, p + 1, toMove * sizeof (T));
			length--;
		}
		else
			p++;
	}
	return length;
}
//-----------------------------------------------------------------------------
void String::removeChars (CharGroup group)
{
	if (isEmpty ())
		return;

	uint32 newLength;

	switch (group)
	{
		case kSpace:
			if (isWide)
				newLength = performRemove<char16> (buffer16, len, iswspace, true);
			else
				newLength = performRemove<char8> (buffer8, len, isspace, true);
			break;

		case kNotAlphaNum:
			if (isWide)
				newLength = performRemove<char16> (buffer16, len, iswalnum, false);
			else
				newLength = performRemove<char8> (buffer8, len, isalnum, false);
			break;

		case kNotAlpha:
			if (isWide)
				newLength = performRemove<char16> (buffer16, len, iswalpha, false);
			else
				newLength = performRemove<char8> (buffer8, len, isalpha, false);
			break;

		default: // Undefined enum value
			return;
	}

	if (newLength != len)
	{
		resize (newLength, isWide);
		len = newLength;
	}
}

//-----------------------------------------------------------------------------
template <class T>
static uint32 performRemoveChars (T* str, uint32 length, const T* toRemove)
{
	T* p = str;

	while (*p)
	{
		bool found = false;
		const T* rem = toRemove;
		while (*rem)
		{
			if (*p == *rem)
			{
				found = true;
				break;
			}
			rem++;
		}

		if (found)
		{
			size_t toMove = length - (p - str);
			memmove (p, p + 1, toMove * sizeof (T));
			length--;
		}
		else
			p++;
	}
	return length;
}

//-----------------------------------------------------------------------------
bool String::removeChars8 (const char8* toRemove)
{
	if (isEmpty () || toRemove == nullptr)
		return true;

	if (isWide)
	{
		String wStr (toRemove);
		if (wStr.toWideString () == false)
			return false;
		return removeChars16 (wStr.text16 ());
	}

	uint32 newLength = performRemoveChars<char8> (buffer8, len, toRemove);

	if (newLength != len)
	{
		resize (newLength, false);
		len = newLength;
	}
	return true;
}

//-----------------------------------------------------------------------------
bool String::removeChars16 (const char16* toRemove)
{
	if (isEmpty () || toRemove == nullptr)
		return true;

	if (!isWide)
	{
		String str8 (toRemove);
		if (str8.toMultiByte () == false)
			return false;
		return removeChars8 (str8.text8 ());
	}

	uint32 newLength = performRemoveChars<char16> (buffer16, len, toRemove);

	if (newLength != len)
	{
		resize (newLength, true);
		len = newLength;
	}
	return true;
}

//-----------------------------------------------------------------------------
String& String::printf (const char8* format, ...)
{
	char8 string[kPrintfBufferSize];

	va_list marker;
	va_start (marker, format);

	vsnprintf (string, kPrintfBufferSize - 1, format, marker);
	return assign (string);
}

//-----------------------------------------------------------------------------
String& String::printf (const char16* format, ...)
{
	char16 string[kPrintfBufferSize];

	va_list marker;
	va_start (marker, format);

	vsnwprintf (string, kPrintfBufferSize - 1, format, marker);
	return assign (string);
}

//-----------------------------------------------------------------------------
String& String::vprintf (const char8* format, va_list args)
{
	char8 string[kPrintfBufferSize];

	vsnprintf (string, kPrintfBufferSize - 1, format, args);
	return assign (string);
}

//-----------------------------------------------------------------------------
String& String::vprintf (const char16* format, va_list args)
{
	char16 string[kPrintfBufferSize];

	vsnwprintf (string, kPrintfBufferSize - 1, format, args);
	return assign (string);
}

//-----------------------------------------------------------------------------
String& String::printInt64 (int64 value)
{
	if (isWide)
	{
#if SMTG_CPP11
		return String::printf (STR ("%") STR (FORMAT_INT64A), value);
#else
		return String::printf (STR ("%" FORMAT_INT64A), value);
#endif
	}
	else
		return String::printf ("%" FORMAT_INT64A, value);
}

//-----------------------------------------------------------------------------
String& String::printFloat (double value, uint32 maxPrecision)
{
	static constexpr auto kMaxAfterCommaResolution = 16;
	// escape point for integer values, avoid unnecessary complexity later on
	const bool withinInt64Boundaries = value <= std::numeric_limits<int64>::max () &&
	                                   value >= std::numeric_limits<int64>::lowest ();
	if (withinInt64Boundaries && (maxPrecision == 0 || std::round (value) == value))
		return printInt64 (value);

	const auto absValue = std::abs (value);
	const uint32 valueExponent = absValue >= 1 ? std::log10 (absValue) : -std::log10 (absValue) + 1;

	maxPrecision = std::min<uint32> (kMaxAfterCommaResolution - valueExponent, maxPrecision);

	if (isWide)
		printf (STR ("%s%dlf"), STR ("%."), maxPrecision);
	else
		printf ("%s%dlf", "%.", maxPrecision);

	if (isWide)
		printf (text16 (), value);
	else
		printf (text8 (), value);

	// trim trail zeros
	for (int32 i = length () - 1; i >= 0; i--)
	{
		if ((isWide && testChar16 (i, '0')) || testChar8 (i, '0'))
			remove (i);
		else if ((isWide && testChar16 (i, '.')) || testChar8 (i, '.'))
		{
			remove (i);
			break;
		}
		else
			break;
	}

	return *this;
}

//-----------------------------------------------------------------------------
bool String::incrementTrailingNumber (uint32 width, tchar separator, uint32 minNumber,
                                      bool applyOnlyFormat)
{
	if (width > 32)
		return false;

	int64 number = 1;
	int32 index = getTrailingNumberIndex ();
	if (index >= 0)
	{
		if (scanInt64 (number, index))
			if (!applyOnlyFormat)
				number++;

		if (separator != 0 && index > 0 && testChar (index - 1, separator) == true)
			index--;

		remove (index);
	}

	if (number < minNumber)
		number = minNumber;

	if (isWide)
	{
		char16 format[64];
		char16 trail[128];
		if (separator && isEmpty () == false)
		{
			sprintf16 (format, STR16 ("%%c%%0%uu"), width);
			sprintf16 (trail, format, separator, (uint32)number);
		}
		else
		{
			sprintf16 (format, STR16 ("%%0%uu"), width);
			sprintf16 (trail, format, (uint32)number);
		}
		append (trail);
	}
	else
	{
		static constexpr auto kFormatSize = 64u;
		static constexpr auto kTrailSize = 64u;
		char format[kFormatSize];
		char trail[kTrailSize];
		if (separator && isEmpty () == false)
		{
			snprintf (format, kFormatSize, "%%c%%0%uu", width);
			snprintf (trail, kTrailSize, format, separator, (uint32)number);
		}
		else
		{
			snprintf (format, kFormatSize, "%%0%uu", width);
			snprintf (trail, kTrailSize, format, (uint32)number);
		}
		append (trail);
	}

	return true;
}

//-----------------------------------------------------------------------------
void String::toLower (uint32 index)
{
	if (buffer && index < len)
	{
		if (isWide)
			buffer16[index] = ConstString::toLower (buffer16[index]);
		else
			buffer8[index] = ConstString::toLower (buffer8[index]);
	}
}

//-----------------------------------------------------------------------------
void String::toLower ()
{
	int32 i = len;
	if (buffer && i > 0)
	{
		if (isWide)
		{
#if SMTG_OS_MACOS
			CFMutableStringRef cfStr = CFStringCreateMutableWithExternalCharactersNoCopy (
			    kCFAllocator, (UniChar*)buffer16, len, len + 1, kCFAllocatorNull);
			CFStringLowercase (cfStr, NULL);
			CFRelease (cfStr);
#else
			char16* c = buffer16;
			while (i--)
			{
				*c = ConstString::toLower (*c);
				c++;
			}
#endif
		}
		else
		{
			char8* c = buffer8;
			while (i--)
			{
				*c = ConstString::toLower (*c);
				c++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
void String::toUpper (uint32 index)
{
	if (buffer && index < len)
	{
		if (isWide)
			buffer16[index] = ConstString::toUpper (buffer16[index]);
		else
			buffer8[index] = ConstString::toUpper (buffer8[index]);
	}
}

//-----------------------------------------------------------------------------
void String::toUpper ()
{
	int32 i = len;
	if (buffer && i > 0)
	{
		if (isWide)
		{
#if SMTG_OS_MACOS
			CFMutableStringRef cfStr = CFStringCreateMutableWithExternalCharactersNoCopy (
			    kCFAllocator, (UniChar*)buffer16, len, len + 1, kCFAllocatorNull);
			CFStringUppercase (cfStr, NULL);
			CFRelease (cfStr);
#else
			char16* c = buffer16;
			while (i--)
			{
				*c = ConstString::toUpper (*c);
				c++;
			}
#endif
		}
		else
		{
			char8* c = buffer8;
			while (i--)
			{
				*c = ConstString::toUpper (*c);
				c++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
bool String::fromVariant (const FVariant& var)
{
	switch (var.getType ())
	{
		case FVariant::kString8:
		{
			assign (var.getString8 ());
			return true;
		}

		case FVariant::kString16:
		{
			assign (var.getString16 ());
			return true;
		}

		case FVariant::kFloat:
		{
			printFloat (var.getFloat ());
			return true;
		}

		case FVariant::kInteger:
		{
			printInt64 (var.getInt ());
			return true;
		}

		case FVariant::kObject:
			if (auto string = ICast<Steinberg::IString> (var.getObject ()))
			{
				if (string->isWideString ())
					assign (string->getText16 ());
				else
					assign (string->getText8 ());
			}
			return true;

		default: remove ();
	}
	return false;
}

//-----------------------------------------------------------------------------
void String::toVariant (FVariant& var) const
{
	if (isWide)
	{
		var.setString16 (text16 ());
	}
	else
	{
		var.setString8 (text8 ());
	}
}

//-----------------------------------------------------------------------------
bool String::fromAttributes (IAttributes* a, IAttrID attrID)
{
	FVariant variant;
	if (a->get (attrID, variant) == kResultTrue)
		return fromVariant (variant);
	return false;
}

//-----------------------------------------------------------------------------
bool String::toAttributes (IAttributes* a, IAttrID attrID)
{
	FVariant variant;
	toVariant (variant);
	if (a->set (attrID, variant) == kResultTrue)
		return true;
	return false;
}

// "swapContent" swaps ownership of the strings pointed to
//-----------------------------------------------------------------------------
void String::swapContent (String& s)
{
	void* tmp = s.buffer;
	uint32 tmpLen = s.len;
	bool tmpWide = s.isWide;
	s.buffer = buffer;
	s.len = len;
	s.isWide = isWide;
	buffer = tmp;
	len = tmpLen;
	isWide = tmpWide;
}

//-----------------------------------------------------------------------------
void String::take (String& other)
{
	resize (0, other.isWide);
	buffer = other.buffer;
	len = other.len;

	other.buffer = nullptr;
	other.len = 0;
}

//-----------------------------------------------------------------------------
void String::take (void* b, bool wide)
{
	resize (0, wide);
	buffer = b;
	isWide = wide;
	updateLength ();
}

//-----------------------------------------------------------------------------
void* String::pass ()
{
	void* res = buffer;
	len = 0;
	buffer = nullptr;
	return res;
}

//-----------------------------------------------------------------------------
void String::passToVariant (FVariant& var)
{
	void* passed = pass ();

	if (isWide)
	{
		if (passed)
		{
			var.setString16 ((const char16*)passed);
			var.setOwner (true);
		}
		else
			var.setString16 (kEmptyString16);
	}
	else
	{
		if (passed)
		{
			var.setString8 ((const char8*)passed);
			var.setOwner (true);
		}
		else
			var.setString8 (kEmptyString8);
	}
}

//-----------------------------------------------------------------------------
unsigned char* String::toPascalString (unsigned char* buf)
{
	if (buffer)
	{
		if (isWide)
		{
			String tmp (*this);
			tmp.toMultiByte ();
			return tmp.toPascalString (buf);
		}

		int32 length = len;
		if (length > 255)
			length = 255;
		buf[0] = (uint8)length;
		while (length >= 0)
		{
			buf[length + 1] = buffer8[length];
			length--;
		}
		return buf;
	}

	*buf = 0;
	return buf;
}

//-----------------------------------------------------------------------------
const String& String::fromPascalString (const unsigned char* buf)
{
	resize (0, false);
	isWide = 0;
	int32 length = buf[0];
	resize (length + 1, false);
	buffer8[length] = 0;	// cannot be removed, because we only do the 0-termination for multibyte buffer8
	while (--length >= 0)
		buffer8[length] = buf[length + 1];
	len = buf[0];
	return *this;
}

#if SMTG_OS_MACOS

//-----------------------------------------------------------------------------
bool String::fromCFStringRef (const void* cfStr, uint32 encoding)
{
	if (cfStr == 0)
		return false;

	CFStringRef strRef = (CFStringRef)cfStr;
	if (isWide)
	{
		CFRange range = {0, CFStringGetLength (strRef)};
		CFIndex usedBytes;
		if (resize (static_cast<int32> (range.length + 1), true))
		{
			if (encoding == 0xFFFF)
				encoding = kCFStringEncodingUnicode;
			if (CFStringGetBytes (strRef, range, encoding, ' ', false, (UInt8*)buffer16,
			                      range.length * 2, &usedBytes) > 0)
			{
				buffer16[usedBytes / 2] = 0;
				this->len = strlen16 (buffer16);
				return true;
			}
		}
	}
	else
	{
		if (cfStr == 0)
			return false;
		if (encoding == 0xFFFF)
			encoding = kCFStringEncodingASCII;
		int32 len = static_cast<int32> (CFStringGetLength (strRef) * 2);
		if (resize (++len, false))
		{
			if (CFStringGetCString (strRef, buffer8, len, encoding))
			{
				this->len = static_cast<int32> (strlen (buffer8));
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
void* ConstString::toCFStringRef (uint32 encoding, bool mutableCFString) const
{
	if (mutableCFString)
	{
		CFMutableStringRef str = CFStringCreateMutable (kCFAllocator, 0);
		if (isWide)
		{
			CFStringAppendCharacters (str, (const UniChar*)buffer16, len);
			return str;
		}
		else
		{
			if (encoding == 0xFFFF)
				encoding = kCFStringEncodingASCII;
			CFStringAppendCString (str, buffer8, encoding);
			return str;
		}
	}
	else
	{
		if (isWide)
		{
			if (encoding == 0xFFFF)
				encoding = kCFStringEncodingUnicode;
			return (void*)CFStringCreateWithBytes (kCFAllocator, (const unsigned char*)buffer16,
			                                       len * 2, encoding, false);
		}
		else
		{
			if (encoding == 0xFFFF)
				encoding = kCFStringEncodingASCII;
			if (buffer8)
				return (void*)CFStringCreateWithCString (kCFAllocator, buffer8, encoding);
			else
				return (void*)CFStringCreateWithCString (kCFAllocator, "", encoding);
		}
	}
	return nullptr;
}

#endif

//-----------------------------------------------------------------------------
uint32 hashString8 (const char8* s, uint32 m)
{
	uint32 h = 0;
	if (s)
	{
		for (h = 0; *s != '\0'; s++)
			h = (64 * h + *s) % m;
	}
	return h;
}

//-----------------------------------------------------------------------------
uint32 hashString16 (const char16* s, uint32 m)
{
	uint32 h = 0;
	if (s)
	{
		for (h = 0; *s != 0; s++)
			h = (64 * h + *s) % m;
	}
	return h;
}

//------------------------------------------------------------------------
template <class T>
int32 tstrnatcmp (const T* s1, const T* s2, bool caseSensitive = true)
{
	if (s1 == nullptr && s2 == nullptr)
		return 0;
	if (s1 == nullptr)
		return -1;
	if (s2 == nullptr)
		return 1;

	while (*s1 && *s2)
	{
		if (ConstString::isCharDigit (*s1) && ConstString::isCharDigit (*s2))
		{
			int32 s1LeadingZeros = 0;
			while (*s1 == '0')
			{
				s1++; // skip leading zeros
				s1LeadingZeros++;
			}
			int32 s2LeadingZeros = 0;
			while (*s2 == '0')
			{
				s2++; // skip leading zeros
				s2LeadingZeros++;
			}

			int32 countS1Digits = 0;
			while (*(s1 + countS1Digits) && ConstString::isCharDigit (*(s1 + countS1Digits)))
				countS1Digits++;
			int32 countS2Digits = 0;
			while (*(s2 + countS2Digits) && ConstString::isCharDigit (*(s2 + countS2Digits)))
				countS2Digits++;

			if (countS1Digits != countS2Digits)
				return countS1Digits - countS2Digits; // one number is longer than the other

			for (int32 i = 0; i < countS1Digits; i++)
			{
				// countS1Digits == countS2Digits
				if (*s1 != *s2)
					return (int32) (*s1 - *s2); // the digits differ
				s1++;
				s2++;
			}

			if (s1LeadingZeros != s2LeadingZeros)
				return s1LeadingZeros - s2LeadingZeros; // differentiate by the number of leading zeros
		}
		else
		{
			if (caseSensitive == false)
			{
				T srcToUpper = static_cast<T> (toupper (*s1));
				T dstToUpper = static_cast<T> (toupper (*s2));
				if (srcToUpper != dstToUpper)
					return (int32) (srcToUpper - dstToUpper);
			}
			else if (*s1 != *s2)
				return (int32) (*s1 - *s2);

			s1++;
			s2++;
		}
	}

	if (*s1 == 0 && *s2 == 0)
		return 0;
	if (*s1 == 0)
		return -1;
	if (*s2 == 0)
		return 1;
	return (int32) (*s1 - *s2);
}

//------------------------------------------------------------------------
int32 strnatcmp8 (const char8* s1, const char8* s2, bool caseSensitive /*= true*/)
{
	return tstrnatcmp (s1, s2, caseSensitive);
}

//------------------------------------------------------------------------
int32 strnatcmp16 (const char16* s1, const char16* s2, bool caseSensitive /*= true*/)
{
	return tstrnatcmp (s1, s2, caseSensitive);
}

//-----------------------------------------------------------------------------
// StringObject Implementation
//-----------------------------------------------------------------------------
void PLUGIN_API StringObject::setText (const char8* text)
{
	assign (text);
}

//-----------------------------------------------------------------------------
void PLUGIN_API StringObject::setText8 (const char8* text)
{
	assign (text);
}

//-----------------------------------------------------------------------------
void PLUGIN_API StringObject::setText16 (const char16* text)
{
	assign (text);
}

//-----------------------------------------------------------------------------
const char8* PLUGIN_API StringObject::getText8 ()
{
	return text8 ();
}

//-----------------------------------------------------------------------------
const char16* PLUGIN_API StringObject::getText16 ()
{
	return text16 ();
}

//-----------------------------------------------------------------------------
void PLUGIN_API StringObject::take (void* s, bool _isWide)
{
	String::take (s, _isWide);
}

//-----------------------------------------------------------------------------
bool PLUGIN_API StringObject::isWideString () const
{
	return String::isWideString ();
}

//------------------------------------------------------------------------
} // namespace Steinberg
