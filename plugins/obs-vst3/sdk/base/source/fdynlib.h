//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fdynlib.h
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

//------------------------------------------------------------------------
/** @file base/source/fdynlib.h
	Platform independent dynamic library loading. */
//------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/base/ftypes.h"
#include "base/source/fobject.h"

namespace Steinberg {

//------------------------------------------------------------------------
/** Platform independent dynamic library loader. */
//------------------------------------------------------------------------
class FDynLibrary : public FObject
{
public:
//------------------------------------------------------------------------
	/** Constructor.
	
		Loads the specified dynamic library.

		@param[in] name the path of the library to load.
		@param[in] addExtension if @c true append the platform dependent default extension to @c name.

		@remarks
		- If @c name specifies a full path, the FDynLibrary searches only that path for the library.
		- If @c name specifies a relative path or a name without path, 
		  FDynLibrary uses a standard search strategy of the current platform to find the library; 
		- If @c name is @c NULL the library is not loaded. 
		  - Use init() to load the library. */
	FDynLibrary (const tchar* name = nullptr, bool addExtension = true);
	
	/** Destructor.
		The destructor unloads the library.*/
	~FDynLibrary () override;

	/** Loads the library if not already loaded.
		This function is normally called by FDynLibrary(). 
		@remarks If the library is already loaded, this call has no effect. */
	bool init (const tchar* name, bool addExtension = true);

	/** Returns the address of the procedure @c name */ 
	void* getProcAddress (const char* name);

	/** Returns true when the library was successfully loaded. */
	bool isLoaded () {return isloaded;} 

	/** Unloads the library if it is loaded.
		This function is called by ~FDynLibrary (). */
	bool unload ();
	
	/** Returns the platform dependent representation of the library instance. */
	void* getPlatformInstance () const { return instance; } 

#if SMTG_OS_MACOS
	/** Returns @c true if the library is a bundle (Mac only). */
	bool isBundleLib () const { return isBundle; }
#endif

//------------------------------------------------------------------------
	OBJ_METHODS(FDynLibrary, FObject)
protected:	
	bool isloaded;

	void* instance;

#if SMTG_OS_MACOS
	void* firstSymbol;
	bool isBundle;
#endif
};

//------------------------------------------------------------------------
} // namespace Steinberg
