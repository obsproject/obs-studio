//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstmessage.h
// Created by  : Steinberg, 04/2005
// Description : VST Message Interfaces
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstattributes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Private plug-in message: Vst::IMessage
\ingroup vstIHost vst300
- [host imp]
- [create via IHostApplication::createInstance]
- [released: 3.0.0]
- [mandatory]

Messages are sent from a VST controller component to a VST editor component and vice versa.
\see IAttributeList, IConnectionPoint, \ref vst3Communication
*/
class IMessage : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Returns the message ID (for example "TextMessage"). */
	virtual FIDString PLUGIN_API getMessageID () = 0;

	/** Sets a message ID (for example "TextMessage"). */
	virtual void PLUGIN_API setMessageID (FIDString id /*in*/) = 0;

	/** Returns the attribute list associated to the message. */
	virtual IAttributeList* PLUGIN_API getAttributes () = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IMessage, 0x936F033B, 0xC6C047DB, 0xBB0882F8, 0x13C1E613)

//------------------------------------------------------------------------
/** Connect a component with another one: Vst::IConnectionPoint
\ingroup vstIPlug vst300
- [plug imp]
- [host imp]
- [released: 3.0.0]
- [mandatory]

This interface is used for the communication of separate components.
Note that some hosts will place a proxy object between the components so that they are not directly
connected.

\see \ref vst3Communication
*/
class IConnectionPoint : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Connects this instance with another connection point.
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API connect (IConnectionPoint* other /*in*/) = 0;

	/** Disconnects a given connection point from this.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API disconnect (IConnectionPoint* other /*in*/) = 0;

	/** Called when a message has been sent from the connection point to this.
	 * \note [UI-thread & Connected] */
	virtual tresult PLUGIN_API notify (IMessage* message /*in*/) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IConnectionPoint, 0x70A4156F, 0x6E6E4026, 0x989148BF, 0xAA60D8D1)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
