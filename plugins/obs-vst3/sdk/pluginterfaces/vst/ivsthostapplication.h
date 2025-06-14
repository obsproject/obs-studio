//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivsthostapplication.h
// Created by  : Steinberg, 04/2006
// Description : VST Host Interface
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstmessage.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Basic host callback interface: Vst::IHostApplication
\ingroup vstIHost vst300
- [host imp]
- [passed as 'context' in to IPluginBase::initialize () ]
- [released: 3.0.0]
- [mandatory]

Basic VST host application interface.
*/
class IHostApplication : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** Gets host application name.
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API getName (String128 name) = 0;

	/** Creates host object (for example: Vst::IMessage).
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API createInstance (TUID cid, TUID _iid, void** obj) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IHostApplication, 0x58E595CC, 0xDB2D4969, 0x8B6AAF8C, 0x36A664E5)

//------------------------------------------------------------------------
/** Helper to allocate a message */
inline IMessage* allocateMessage (IHostApplication* host)
{
	TUID iid;
	IMessage::iid.toTUID (iid);
	IMessage* m = nullptr;
	if (host->createInstance (iid, iid, (void**)&m) == kResultOk)
		return m;
	return nullptr;
}

//------------------------------------------------------------------------
/** VST 3 to VST 2 Wrapper interface: Vst::IVst3ToVst2Wrapper
\ingroup vstIHost vst310
- [host imp]
- [passed as 'context' to IPluginBase::initialize () ]
- [released: 3.1.0]
- [mandatory]

Informs the plug-in that a VST 3 to VST 2 wrapper is used between the plug-in and the real host.
Implemented by the VST 2 Wrapper.
*/
class IVst3ToVst2Wrapper : public FUnknown
{
public:
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IVst3ToVst2Wrapper, 0x29633AEC, 0x1D1C47E2, 0xBB85B97B, 0xD36EAC61)

//------------------------------------------------------------------------
/** VST 3 to AU Wrapper interface: Vst::IVst3ToAUWrapper
\ingroup vstIHost vst310
- [host imp]
- [passed as 'context' to IPluginBase::initialize () ]
- [released: 3.1.0]
- [mandatory]

Informs the plug-in that a VST 3 to AU wrapper is used between the plug-in and the real host.
Implemented by the AU Wrapper.
*/
class IVst3ToAUWrapper : public FUnknown
{
public:
	//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IVst3ToAUWrapper, 0xA3B8C6C5, 0xC0954688, 0xB0916F0B, 0xB697AA44)

//------------------------------------------------------------------------
/** VST 3 to AAX Wrapper interface: Vst::IVst3ToAAXWrapper
\ingroup vstIHost vst368
- [host imp]
- [passed as 'context' to IPluginBase::initialize () ]
- [released: 3.6.8]
- [mandatory]

Informs the plug-in that a VST 3 to AAX wrapper is used between the plug-in and the real host.
Implemented by the AAX Wrapper.
*/
class IVst3ToAAXWrapper : public FUnknown
{
public:
	//------------------------------------------------------------------------
	static const FUID iid;
};
DECLARE_CLASS_IID (IVst3ToAAXWrapper, 0x6D319DC6, 0x60C56242, 0xB32C951B, 0x93BEF4C6)

//------------------------------------------------------------------------
/** Wrapper MPE Support interface: Vst::IVst3WrapperMPESupport
\ingroup vstIHost vst3612
- [host imp]
- [passed as 'context' to IPluginBase::initialize () ]
- [released: 3.6.12]
- [optional]

Implemented on wrappers that support MPE to Note Expression translation.

By default, MPE input processing is enabled, the masterChannel will be zero, the memberBeginChannel
will be one and the memberEndChannel will be 14.

As MPE is a subset of the VST3 Note Expression feature, mapping from the three MPE expressions is
handled via the INoteExpressionPhysicalUIMapping interface.
*/
class IVst3WrapperMPESupport : public FUnknown
{
public:
//------------------------------------------------------------------------
	/** enable or disable MPE processing
	 *	@param state true to enable, false to disable MPE processing
	 *	@return kResultTrue on success */
	virtual tresult PLUGIN_API enableMPEInputProcessing (TBool state) = 0;
	/** setup the MPE processing
	 *	@param masterChannel MPE master channel (zero based)
	 *	@param memberBeginChannel MPE member begin channel (zero based)
	 *	@param memberEndChannel MPE member end channel (zero based)
	 *	@return kResultTrue on success */
	virtual tresult PLUGIN_API setMPEInputDeviceSettings (int32 masterChannel,
	                                                      int32 memberBeginChannel,
	                                                      int32 memberEndChannel) = 0;

//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IVst3WrapperMPESupport, 0x44149067, 0x42CF4BF9, 0x8800B750, 0xF7359FE3)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
