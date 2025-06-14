//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/updatehandler.h
// Created by  : Steinberg, 2008
// Description :
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "base/source/fobject.h"
#include "base/thread/include/flock.h"
#include "pluginterfaces/base/iupdatehandler.h"

namespace Steinberg {

/// @cond ignore
namespace Update { struct Table; }
/// @endcond

//------------------------------------------------------------------------
/** Handle Send and Cancel pending message for a given object*/
//------------------------------------------------------------------------
class IUpdateManager : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** cancel pending messages send by \param object or by any if object is 0 */
	virtual tresult PLUGIN_API cancelUpdates (FUnknown* object) = 0;
	/** send pending messages send by \param object or by any if object is 0 */
	virtual tresult PLUGIN_API triggerDeferedUpdates (FUnknown* object = nullptr) = 0;
	static const FUID iid;
};

DECLARE_CLASS_IID (IUpdateManager, 0x030B780C, 0xD6E6418D, 0x8CE00BC2, 0x09C834D4)

//------------------------------------------------------------------------------
/**
UpdateHandler implements IUpdateManager and IUpdateHandler to handle dependencies
between objects to store and forward messages to dependent objects.

This implementation is thread save, so objects can send message, add or remove
dependents from different threads.
Do do so it uses mutex, so be aware of locking.
*/
//------------------------------------------------------------------------------
class UpdateHandler : public FObject, public IUpdateHandler, public IUpdateManager
{
public:
//------------------------------------------------------------------------------
	UpdateHandler ();
	~UpdateHandler () SMTG_OVERRIDE;

	using FObject::addDependent;
	using FObject::removeDependent;
	using FObject::deferUpdate;

	// IUpdateHandler
//private:
	friend class FObject;
	/** register \param dependent to get messages from \param object */
	tresult PLUGIN_API addDependent (FUnknown* object, IDependent* dependent) SMTG_OVERRIDE;
	/** unregister \param dependent to get no messages from \param object */
	tresult PLUGIN_API removeDependent (FUnknown* object, IDependent* dependent, size_t& earseCount);
	tresult PLUGIN_API removeDependent (FUnknown* object,
	                                            IDependent* dependent) SMTG_OVERRIDE;
public:
	/** send \param message to all dependents of \param object immediately */
	tresult PLUGIN_API triggerUpdates (FUnknown* object, int32 message) SMTG_OVERRIDE;
	/** send \param message to all dependents of \param object when idle */
	tresult PLUGIN_API deferUpdates (FUnknown* object, int32 message) SMTG_OVERRIDE;

	// IUpdateManager
	/** cancel pending messages send by \param object or by any if object is 0 */
	tresult PLUGIN_API cancelUpdates (FUnknown* object) SMTG_OVERRIDE;
	/** send pending messages send by \param object or by any if object is 0 */
	tresult PLUGIN_API triggerDeferedUpdates (FUnknown* object = nullptr) SMTG_OVERRIDE;

	/// @cond ignore
	// obsolete functions kept for compatibility
	void checkUpdates (FObject* object = nullptr)
	{
		triggerDeferedUpdates (object ? object->unknownCast () : nullptr);
	}
	void flushUpdates (FObject* object)
	{
		if (object)
			cancelUpdates (object->unknownCast ());
	}
	void deferUpdate (FObject* object, int32 message)
	{
		if (object)
			deferUpdates (object->unknownCast (), message);
	}
	void signalChange (FObject* object, int32 message, bool suppressUpdateDone = false)
	{
		if (object)
			doTriggerUpdates (object->unknownCast (), message, suppressUpdateDone);
	}
#if DEVELOPMENT
	bool checkDeferred (FUnknown* object);
	bool hasDependencies (FUnknown* object);
	void printForObject (FObject* object) const;
#endif
	/// @endcond
	size_t countDependencies (FUnknown* object = nullptr);
	
	OBJ_METHODS (UpdateHandler, FObject)
	FUNKNOWN_METHODS2 (IUpdateHandler, IUpdateManager, FObject)
	SINGLETON (UpdateHandler)
//------------------------------------------------------------------------------
private:
	tresult doTriggerUpdates (FUnknown* object, int32 message, bool suppressUpdateDone);

	Steinberg::Base::Thread::FLock lock;
	Update::Table* table = nullptr;
};


//------------------------------------------------------------------------
} // namespace Steinberg
