/******************************************************************************
    Copyright (C) 2025-2026 pkv <pkv@obsproject.com>
    This file is part of obs-vst3.
    It uses the Steinberg VST3 SDK, which is licensed under MIT license.
    See https://github.com/steinbergmedia/vst3sdk for details.
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "VST3HostApp.h"

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/ivstattributes.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstpluginterfacesupport.h>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <public.sdk/source/vst/utility/stringconvert.h>

#include <algorithm>

using namespace Steinberg;
using namespace Vst;

VST3HostApp::VST3HostApp(VST3Backend backend) : backend_(backend)
{
	addPlugInterfaceSupported(IComponent::iid);
	addPlugInterfaceSupported(IAudioProcessor::iid);
	addPlugInterfaceSupported(IEditController::iid);
	addPlugInterfaceSupported(IConnectionPoint::iid);
}

VST3HostApp::~VST3HostApp() noexcept {FUNKNOWN_DTOR}

tresult PLUGIN_API VST3HostApp::getName(String128 name)
{
	return StringConvert::convert("OBS VST3 Host", name) ? kResultTrue : kInternalError;
}

tresult PLUGIN_API VST3HostApp::createInstance(TUID cid, TUID iid_, void **obj)
{
	if (FUnknownPrivate::iidEqual(cid, IMessage::iid) && FUnknownPrivate::iidEqual(iid_, IMessage::iid)) {
		*obj = new HostMessage;
		return kResultTrue;
	}
	if (FUnknownPrivate::iidEqual(cid, IAttributeList::iid) &&
	    FUnknownPrivate::iidEqual(iid_, IAttributeList::iid)) {
		if (auto al = HostAttributeList::make()) {
			*obj = al.take();
			return kResultTrue;
		}
		return kOutOfMemory;
	}
	*obj = nullptr;
	return kResultFalse;
}

tresult PLUGIN_API VST3HostApp::isPlugInterfaceSupported(const TUID iid_)
{
	auto uid = FUID::fromTUID(iid_);
	if (std::find(FUIDArray_.begin(), FUIDArray_.end(), uid) != FUIDArray_.end()) {
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API VST3HostApp::queryInterface(const TUID iid_, void **obj)
{
	if (FUnknownPrivate::iidEqual(iid_, FUnknown::iid)) {
		*obj = static_cast<FUnknown *>(static_cast<IHostApplication *>(this));
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual(iid_, IHostApplication::iid)) {
		*obj = static_cast<IHostApplication *>(this);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual(iid_, IPlugInterfaceSupport::iid)) {
		*obj = static_cast<IPlugInterfaceSupport *>(this);
		return kResultOk;
	}
#ifdef __linux__
	if (runLoop && FUnknownPrivate::iidEqual(iid_, Linux::IRunLoop::iid)) {
		*obj = static_cast<Linux::IRunLoop *>(runLoop);
		return kResultOk;
	}
#endif
	*obj = nullptr;
	return kNoInterface;
}
