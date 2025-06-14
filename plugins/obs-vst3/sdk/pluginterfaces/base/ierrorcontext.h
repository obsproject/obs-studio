//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : SDK Core Interfaces
// Filename    : pluginterfaces/base/ierrorcontext.h
// Created by  : Steinberg, 02/2008
// Description : Error Context Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"

namespace Steinberg {

class IString;

//------------------------------------------------------------------------
/** Interface for error handling. 
- [plug imp]
- [released: Sequel 2]
*/
class IErrorContext : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Tells the plug-in to not show any UI elements on errors. */
	virtual void PLUGIN_API disableErrorUI (bool state) = 0;
	/** If an error happens and disableErrorUI was not set this should return kResultTrue if the
	 * plug-in already showed a message to the user what happened. */
	virtual tresult PLUGIN_API errorMessageShown () = 0;
	/** Fill message with error string. The host may show this to the user. */
	virtual tresult PLUGIN_API getErrorMessage (IString* message) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IErrorContext, 0x12BCD07B, 0x7C694336, 0xB7DA77C3, 0x444A0CD0)

//------------------------------------------------------------------------
} // namespace Steinberg
