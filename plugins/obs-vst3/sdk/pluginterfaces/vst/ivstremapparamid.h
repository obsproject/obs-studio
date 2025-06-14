//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Interfaces
// Filename    : pluginterfaces/vst/ivstremapparamid.h
// Created by  : Steinberg, 02/2024
// Description : VST Edit Controller Interfaces
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
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpush.h"
//------------------------------------------------------------------------

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Extended IEditController interface for a component.
\ingroup vstIPlug vst3711
- [plug imp]
- [extends IEditController]
- [released: 3.7.11]
- [optional]

When replacing one plug-in with another, the host can ask the new plug-in for remapping paramIDs to
new ones.

\n
\see Moduleinfo
\see \ref IPluginCompatibility
\see IEditController
*/
class IRemapParamID : public FUnknown
{
public:
//------------------------------------------------------------------------
	/**	Retrieve the appropriate paramID for a specific plug-in UID and paramID (or index for VST 2
	 * plug-ins).
	 * The retrieved paramID should match the one it replaces, maintaining the same
	 * behavior during automation playback. Called in UI-Thread context.
	 *
	 * @param[in] pluginToReplaceUID - TUID of plug-in (processor) that will be replaced
	 * @param[in] oldParamID - paramID (or index for VST 2 plug-ins) to be replaced
	 * @param[out] newParamID - contains the associated paramID to be used
	 *
	 * @return kResultTrue if a compatible parameter is available (newParamID has the appropriate
	 * value, it could be the same than oldParamID), or kResultFalse if no compatible parameter is
	 * available (newParamID is undefined).
	 *
	 * \note [UI-thread & Initialized] */
	virtual tresult PLUGIN_API getCompatibleParamID (const TUID pluginToReplaceUID /*in*/,
	                                                 ParamID oldParamID /*in*/,
	                                                 ParamID& newParamID /*out*/) = 0;
//------------------------------------------------------------------------
	static const FUID iid;
};

DECLARE_CLASS_IID (IRemapParamID, 0x2B88021E, 0x6286B646, 0xB49DF76A, 0x5663061C)

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
#include "pluginterfaces/base/falignpop.h"
//------------------------------------------------------------------------
