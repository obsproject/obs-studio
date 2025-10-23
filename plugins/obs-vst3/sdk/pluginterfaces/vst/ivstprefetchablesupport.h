//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstprefetchablesupport.h
// Created by  : Steinberg, 02/2015
// Description : VST Prefetchable Support Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstattributes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

// ------------------------------------------------------------------------
/** \ingroup vst3typedef */
/**@{*/
/** Prefetchable Support Type */
typedef uint32 PrefetchableSupport;
/**@}*/

/** Prefetchable Support Enum */
enum ePrefetchableSupport
{
	kIsNeverPrefetchable = 0,	///< every instance of the plug does not support prefetch processing
	kIsYetPrefetchable,			///< in the current state the plug support prefetch processing
	kIsNotYetPrefetchable,		///< in the current state the plug does not support prefetch processing
	kNumPrefetchableSupport
};

//------------------------------------------------------------------------
// IPrefetchableSupport Interface
//------------------------------------------------------------------------
/** Indicates that the plug-in could or not support Prefetch (dynamically): Vst::IPrefetchableSupport
\ingroup vstIPlug vst365
- [plug imp]
- [extends IComponent]
- [released: 3.6.5]
- [optional]

The plug-in should implement this interface if it needs to dynamically change between prefetchable or not.
By default (without implementing this interface) the host decides in which mode the plug-in is processed.
For more info about the prefetch processing mode check the ProcessModes::kPrefetch documentation.

\section IPrefetchableSupportExample Example

\code{.cpp}
//------------------------------------------------------------------------
tresult PLUGIN_API myPlug::getPrefetchableSupport (PrefetchableSupport& prefetchable)
{
	prefetchable = kIsNeverPrefetchable;

	switch (myPrefetchableMode)
	{
		case 0: prefetchable = kIsNeverPrefetchable; break;
		case 1: prefetchable = kIsYetPrefetchable; break;
		case 2: prefetchable = kIsNotYetPrefetchable; break;
	}
	return kResultOk;
}
\endcode
*/
class IPrefetchableSupport : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** retrieve the current prefetch support. Use IComponentHandler::restartComponent
	 * (kPrefetchableSupportChanged) to inform the host that this support has changed.
	 * \note [UI-thread & (Initialized | Connected | Setup Done | Activated | Processing)] */
	virtual tresult PLUGIN_API getPrefetchableSupport (PrefetchableSupport& prefetchable /*out*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IPrefetchableSupport, 0x8AE54FDA, 0xE93046B9, 0xA28555BC, 0xDC98E21E)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
