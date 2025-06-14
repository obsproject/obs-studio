//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/pluginterfacesupport.cpp
// Created by  : Steinberg, 11/2018.
// Description : VST 3 hostclasses, example implementations for IPlugInterfaceSupport
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfacesupport.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/ivstmessage.h"

#include <algorithm>

//-----------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//-----------------------------------------------------------------------------
PlugInterfaceSupport::PlugInterfaceSupport ()
{
	FUNKNOWN_CTOR
	// add minimum set

	//---VST 3.0.0--------------------------------
	addPlugInterfaceSupported (IComponent::iid);
	addPlugInterfaceSupported (IAudioProcessor::iid);
	addPlugInterfaceSupported (IEditController::iid);
	addPlugInterfaceSupported (IConnectionPoint::iid);

	addPlugInterfaceSupported (IUnitInfo::iid);
	addPlugInterfaceSupported (IUnitData::iid);
	addPlugInterfaceSupported (IProgramListData::iid);

	//---VST 3.0.1--------------------------------
	addPlugInterfaceSupported (IMidiMapping::iid);

	//---VST 3.1----------------------------------
	addPlugInterfaceSupported (IEditController2::iid);

	/*
	//---VST 3.0.2--------------------------------
	addPlugInterfaceSupported (IParameterFinder::iid);

	//---VST 3.1----------------------------------
	addPlugInterfaceSupported (IAudioPresentationLatency::iid);

	//---VST 3.5----------------------------------
	addPlugInterfaceSupported (IKeyswitchController::iid);
	addPlugInterfaceSupported (IContextMenuTarget::iid);
	addPlugInterfaceSupported (IEditControllerHostEditing::iid);
	addPlugInterfaceSupported (IXmlRepresentationController::iid);
	addPlugInterfaceSupported (INoteExpressionController::iid);

	//---VST 3.6.5--------------------------------
	addPlugInterfaceSupported (ChannelContext::IInfoListener::iid);
	addPlugInterfaceSupported (IPrefetchableSupport::iid);
	addPlugInterfaceSupported (IAutomationState::iid);

	//---VST 3.6.11--------------------------------
	addPlugInterfaceSupported (INoteExpressionPhysicalUIMapping::iid);

	//---VST 3.6.12--------------------------------
	addPlugInterfaceSupported (IMidiLearn::iid);

	//---VST 3.7-----------------------------------
	addPlugInterfaceSupported (IProcessContextRequirements::iid);
	addPlugInterfaceSupported (IParameterFunctionName::iid);
	addPlugInterfaceSupported (IProgress::iid);

	//----VST 3.8------------------------------------
	addPlugInterfaceSupported (IMidiMapping2::iid)
	addPlugInterfaceSupported (IMidiLearn2::iid)
	*/
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugInterfaceSupport::isPlugInterfaceSupported (const TUID _iid)
{
	auto uid = FUID::fromTUID (_iid);
	if (std::find (mFUIDArray.begin (), mFUIDArray.end (), uid) != mFUIDArray.end ())
		return kResultTrue;
	return kResultFalse;
}

//-----------------------------------------------------------------------------
void PlugInterfaceSupport::addPlugInterfaceSupported (const TUID _iid)
{
	mFUIDArray.push_back (FUID::fromTUID (_iid));
}

//-----------------------------------------------------------------------------
bool PlugInterfaceSupport::removePlugInterfaceSupported (const TUID _iid)
{
	auto uid = FUID::fromTUID (_iid);
	auto it = std::find (mFUIDArray.begin (), mFUIDArray.end (), uid);
	if (it  == mFUIDArray.end ())
		return false;
	mFUIDArray.erase (it);
	return true;
}

IMPLEMENT_FUNKNOWN_METHODS (PlugInterfaceSupport, IPlugInterfaceSupport, IPlugInterfaceSupport::iid)

//-----------------------------------------------------------------------------
} // Vst
} // Steinberg
