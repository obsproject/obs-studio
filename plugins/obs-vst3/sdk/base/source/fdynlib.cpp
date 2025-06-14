//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fdynlib.cpp
// Created by  : Steinberg, 1998
// Description : Dynamic library loading
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "base/source/fdynlib.h"
#include "pluginterfaces/base/fstrdefs.h"
#include "base/source/fstring.h"

#if SMTG_OS_WINDOWS
#include <windows.h>

#elif SMTG_OS_MACOS
#include <mach-o/dyld.h>
#include <CoreFoundation/CoreFoundation.h>

#if !SMTG_OS_IOS
static const Steinberg::tchar kUnixDelimiter = STR ('/');
#endif
#endif

namespace Steinberg {

#if SMTG_OS_MACOS
#include <dlfcn.h>

// we ignore for the moment that the NSAddImage functions are deprecated
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static bool CopyProcessPath (Steinberg::String& name)
{
	Dl_info info;
	if (dladdr ((const void*)CopyProcessPath, &info))
	{
		if (info.dli_fname)
		{
			name.assign (info.dli_fname);
			#ifdef UNICODE
			name.toWideString ();
			#endif
			return true;
		}
	}
	return false;
}

#endif

//------------------------------------------------------------------------
// FDynLibrary
//------------------------------------------------------------------------
FDynLibrary::FDynLibrary (const tchar* n, bool addExtension)
: isloaded (false)
, instance (nullptr)
{
	if (n)
		init (n, addExtension);
}

//------------------------------------------------------------------------
FDynLibrary::~FDynLibrary ()
{
	unload ();
}

//------------------------------------------------------------------------
bool FDynLibrary::init (const tchar* n, bool addExtension)
{
	if (isLoaded ())
		return true;

	Steinberg::String name (n);
		
#if SMTG_OS_WINDOWS
	if (addExtension)
		name.append (STR (".dll"));

	instance = LoadLibrary (name);
	if (instance)
		isloaded = true;
		
#elif SMTG_OS_MACOS
	isBundle = false;
	// first check if it is a bundle
	if (addExtension)
		name.append (STR (".bundle"));
	if (name.getChar16 (0) != STR('/')) // no absoltue path
	{
		Steinberg::String p;
		if (CopyProcessPath (p))
		{
			Steinberg::int32 index = p.findLast (STR ('/'));
			p.remove (index+1);
			name = p + name;
		}
	}
	CFStringRef fsString = (CFStringRef)name.toCFStringRef ();
	CFURLRef url = CFURLCreateWithFileSystemPath (NULL, fsString, kCFURLPOSIXPathStyle, true);
	if (url)
	{
		CFBundleRef bundle = CFBundleCreate (NULL, url);
		if (bundle)
		{
			if (CFBundleLoadExecutable (bundle))
			{
				isBundle = true;
				instance = (void*)bundle;
			}
			else
				CFRelease (bundle);
		}
		CFRelease (url);
	}
	CFRelease (fsString);

	name.assign (n);

#if !SMTG_OS_IOS
	if (!isBundle)
	{
		// now we check for a dynamic library
		firstSymbol = NULL;
		if (addExtension)
		{
			name.append (STR (".dylib"));
		}
		// Only if name is a relative path we use the Process Path as root:
		if (name[0] != kUnixDelimiter)
		{
			Steinberg::String p;
			if (CopyProcessPath (p))
			{
				Steinberg::int32 index = p.findLast (STR ("/"));
				p.remove (index+1);
				p.append (name);
				p.toMultiByte (Steinberg::kCP_Utf8);
				instance = (void*) NSAddImage (p, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
			}
		}
		// Last but not least let the system search for it
		// 
		if (instance == 0)
		{
			name.toMultiByte (Steinberg::kCP_Utf8);
			instance = (void*) NSAddImage (name, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
		}
	}
#endif // !SMTG_OS_IOS

	if (instance)
		isloaded = true;

#endif

	return isloaded;
}

//------------------------------------------------------------------------
bool FDynLibrary::unload ()
{
	if (!isLoaded ())
		return false;

#if SMTG_OS_WINDOWS
	FreeLibrary ((HINSTANCE)instance);
	
#elif SMTG_OS_MACOS
	if (isBundle)
	{
		if (CFGetRetainCount ((CFTypeRef)instance) == 1)
			CFBundleUnloadExecutable ((CFBundleRef)instance);
		CFRelease ((CFBundleRef)instance);
	}
	else
	{
		// we don't use this anymore as the darwin dyld can't unload dynamic libraries yet and may crash
/*		if (firstSymbol)
		{
			NSModule module = NSModuleForSymbol ((NSSymbol)firstSymbol);
			if (module)
				NSUnLinkModule (module, NSUNLINKMODULE_OPTION_NONE);
		}*/
	}
#endif
	instance = nullptr;
	isloaded = false;
	return true;
}

//------------------------------------------------------------------------
void* FDynLibrary::getProcAddress (const char* name)
{
	if (!isloaded)
		return nullptr;
	
#if SMTG_OS_WINDOWS
	return (void*)GetProcAddress ((HINSTANCE)instance, name);
	
#elif SMTG_OS_MACOS
	if (isBundle)
	{
		CFStringRef functionName = CFStringCreateWithCString (NULL, name, kCFStringEncodingASCII);
		void* result = CFBundleGetFunctionPointerForName ((CFBundleRef)instance, functionName);
		CFRelease (functionName);
		return result;
	}
#if !SMTG_OS_IOS
	else
	{
		char* symbolName = (char*) malloc (strlen (name) + 2);
		strcpy (symbolName, "_");
		strcat (symbolName, name);
		NSSymbol symbol;
		symbol = NSLookupSymbolInImage ((const struct mach_header*)instance, symbolName, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
		free (symbolName);
		if (symbol)
		{
			if (firstSymbol == NULL)
				firstSymbol = symbol;
			return NSAddressOfSymbol (symbol);
		}
	}
#endif // !SMTG_OS_IOS

	return nullptr;
#else
	return nullptr;
#endif
}

//------------------------------------------------------------------------
} // namespace Steinberg
