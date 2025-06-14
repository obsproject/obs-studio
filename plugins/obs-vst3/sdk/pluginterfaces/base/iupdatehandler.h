//------------------------------------------------------------------------
// Project     : Steinberg Module Architecture SDK
//
// Category    : Basic Host Service Interfaces
// Filename    : pluginterfaces/base/iupdatehandler.h
// Created by  : Steinberg, 01/2004
// Description : Update handling
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

class IDependent;

//------------------------------------------------------------------------
/** Host implements dependency handling for plugins.
- [host imp]
- [get this interface from IHostClasses]
- [released N3.1]

- Install/Remove change notifications
- Trigger updates when an object has changed

Can be used between host-objects and the Plug-In or 
inside the Plug-In to handle internal updates!

\see IDependent
\ingroup frameworkHostClasses
*/
class IUpdateHandler: public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Install update notification for given object. It is essential to
	    remove all dependencies again using 'removeDependent'! Dependencies
		are not removed automatically when the 'object' is released! 
	\param object : interface to object that sends change notifications 
	\param dependent : interface through which the update is passed */
	virtual tresult PLUGIN_API addDependent (FUnknown* object, IDependent* dependent) = 0;
	
	/** Remove a previously installed dependency.*/
	virtual tresult PLUGIN_API removeDependent (FUnknown* object, IDependent* dependent) = 0;

	/** Inform all dependents, that object has changed. 
	\param object is the object that has changed
	\param message is a value of enum IDependent::ChangeMessage, usually  IDependent::kChanged - can be
	                 a private message as well (only known to sender and dependent)*/
	virtual	tresult PLUGIN_API triggerUpdates (FUnknown* object, int32 message) = 0;

	/** Same as triggerUpdates, but delivered in idle (usefull to collect updates).*/
	virtual	tresult PLUGIN_API deferUpdates (FUnknown* object, int32 message) = 0;
	static const FUID iid;
};

DECLARE_CLASS_IID (IUpdateHandler, 0xF5246D56, 0x86544d60, 0xB026AFB5, 0x7B697B37)

//------------------------------------------------------------------------
/**  A dependent will get notified about changes of a model.
[plug imp]
- notify changes of a model

\see IUpdateHandler
\ingroup frameworkHostClasses
*/
class IDependent: public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Inform the dependent, that the passed FUnknown has changed. */
	virtual void PLUGIN_API update (FUnknown* changedUnknown, int32 message) = 0;

	enum ChangeMessage 
	{
		kWillChange,
		kChanged,
		kDestroyed,
		kWillDestroy,

		kStdChangeMessageLast = kWillDestroy
	};
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IDependent, 0xF52B7AAE, 0xDE72416d, 0x8AF18ACE, 0x9DD7BD5E)

//------------------------------------------------------------------------
} // namespace Steinberg
