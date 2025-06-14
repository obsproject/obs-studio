//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstinitiids.cpp
// Created by  : Steinberg, 10/2009
// Description : Interface symbols file
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "pluginterfaces/base/funknown.h"

#include "pluginterfaces/base/iplugincompatibility.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstautomationstate.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "pluginterfaces/vst/ivstdataexchange.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstinterappaudio.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstmidimapping2.h"
#include "pluginterfaces/vst/ivstmidilearn.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstparameterfunctionname.h"
#include "pluginterfaces/vst/ivstphysicalui.h"
#include "pluginterfaces/vst/ivstpluginterfacesupport.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "pluginterfaces/vst/ivstprefetchablesupport.h"
#include "pluginterfaces/vst/ivstremapparamid.h"
#include "pluginterfaces/vst/ivstrepresentation.h"
#include "pluginterfaces/vst/ivsttestplugprovider.h"
#include "pluginterfaces/vst/ivstunits.h"

//------------------------------------------------------------------------
namespace Steinberg {

//----VST 3.0--------------------------------
DEF_CLASS_IID (Vst::IComponent)
DEF_CLASS_IID (Vst::IAudioProcessor)
DEF_CLASS_IID (Vst::IUnitData)
DEF_CLASS_IID (Vst::IProgramListData)

DEF_CLASS_IID (Vst::IEditController)
DEF_CLASS_IID (Vst::IUnitInfo)

DEF_CLASS_IID (Vst::IConnectionPoint)

DEF_CLASS_IID (Vst::IComponentHandler)
DEF_CLASS_IID (Vst::IUnitHandler)

DEF_CLASS_IID (Vst::IParamValueQueue)
DEF_CLASS_IID (Vst::IParameterChanges)

DEF_CLASS_IID (Vst::IEventList)
DEF_CLASS_IID (Vst::IMessage)

DEF_CLASS_IID (Vst::IHostApplication)
DEF_CLASS_IID (Vst::IAttributeList)

//----VST 3.0.1--------------------------------
DEF_CLASS_IID (Vst::IMidiMapping)

//----VST 3.0.2--------------------------------
DEF_CLASS_IID (Vst::IParameterFinder)

//----VST 3.1----------------------------------
DEF_CLASS_IID (Vst::IComponentHandler2)
DEF_CLASS_IID (Vst::IEditController2)
DEF_CLASS_IID (Vst::IAudioPresentationLatency)
DEF_CLASS_IID (Vst::IVst3ToVst2Wrapper)
DEF_CLASS_IID (Vst::IVst3ToAUWrapper)

//----VST 3.5----------------------------------
DEF_CLASS_IID (Vst::INoteExpressionController)
DEF_CLASS_IID (Vst::IKeyswitchController)
DEF_CLASS_IID (Vst::IContextMenuTarget)
DEF_CLASS_IID (Vst::IContextMenu)
DEF_CLASS_IID (Vst::IComponentHandler3)
DEF_CLASS_IID (Vst::IEditControllerHostEditing)
DEF_CLASS_IID (Vst::IXmlRepresentationController)

//----VST 3.6----------------------------------
DEF_CLASS_IID (Vst::IInterAppAudioHost)
DEF_CLASS_IID (Vst::IInterAppAudioConnectionNotification)
DEF_CLASS_IID (Vst::IInterAppAudioPresetManager)
DEF_CLASS_IID (Vst::IStreamAttributes)

//----VST 3.6.5--------------------------------
DEF_CLASS_IID (Vst::ChannelContext::IInfoListener)
DEF_CLASS_IID (Vst::IPrefetchableSupport)
DEF_CLASS_IID (Vst::IUnitHandler2)
DEF_CLASS_IID (Vst::IAutomationState)

//----VST 3.6.8--------------------------------
DEF_CLASS_IID (Vst::IComponentHandlerBusActivation)
DEF_CLASS_IID (Vst::IVst3ToAAXWrapper)

//----VST 3.6.11--------------------------------
DEF_CLASS_IID (Vst::INoteExpressionPhysicalUIMapping)

//----VST 3.6.12--------------------------------
DEF_CLASS_IID (Vst::IMidiLearn)
DEF_CLASS_IID (Vst::IPlugInterfaceSupport)
DEF_CLASS_IID (Vst::IVst3WrapperMPESupport)

//----VST 3.6.13--------------------------------
DEF_CLASS_IID (Vst::ITestPlugProvider)

//----VST 3.7-----------------------------------
DEF_CLASS_IID (Vst::IParameterFunctionName)
DEF_CLASS_IID (Vst::IProcessContextRequirements)
DEF_CLASS_IID (Vst::IProgress)
DEF_CLASS_IID (Vst::ITestPlugProvider2)

//----VST 3.7.5---------------------------------
DEF_CLASS_IID (IPluginCompatibility)

//----VST 3.7.9---------------------------------
DEF_CLASS_IID (Vst::IComponentHandlerSystemTime)
DEF_CLASS_IID (Vst::IDataExchangeHandler)
DEF_CLASS_IID (Vst::IDataExchangeReceiver)

//----VST 3.7.11---------------------------------
DEF_CLASS_IID (Vst::IRemapParamID)

//----VST 3.8------------------------------------
DEF_CLASS_IID (Vst::IMidiMapping2)
DEF_CLASS_IID (Vst::IMidiLearn2)

} // Steinberg
