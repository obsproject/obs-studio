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
#include "VST3Plugin.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/ivstmessage.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <string>
using namespace Steinberg;
using namespace Vst;

VST3HostApp::VST3HostApp()
{
	addPlugInterfaceSupported(IComponent::iid);
	addPlugInterfaceSupported(IAudioProcessor::iid);
	addPlugInterfaceSupported(IEditController::iid);
	addPlugInterfaceSupported(IConnectionPoint::iid);
}

VST3HostApp::~VST3HostApp() noexcept {FUNKNOWN_DTOR}

/* IHostApplication methods */
tresult PLUGIN_API VST3HostApp::getName(String128 name)
{
	std::memset(name, 0, sizeof(String128));
#if defined(_WIN32)
	const char *src = "OBS VST3 Host";
	MultiByteToWideChar(CP_UTF8, 0, src, -1, (wchar_t *)name, 128);
#else
	std::u16string src = u"OBS VST3 Host";
	src.copy(name, src.size());
#endif
	return kResultOk;
}

tresult PLUGIN_API VST3HostApp::createInstance(TUID cid, TUID _iid, void **obj)
{
	if (FUnknownPrivate::iidEqual(cid, IMessage::iid) && FUnknownPrivate::iidEqual(_iid, IMessage::iid)) {
		*obj = new HostMessage;
		return kResultTrue;
	}
	if (FUnknownPrivate::iidEqual(cid, IAttributeList::iid) &&
	    FUnknownPrivate::iidEqual(_iid, IAttributeList::iid)) {
		if (auto al = HostAttributeList::make()) {
			*obj = al.take();
			return kResultTrue;
		}
		return kOutOfMemory;
	}
	*obj = nullptr;
	return kResultFalse;
}

/* IPlugInterfaceSupport methods */
tresult PLUGIN_API VST3HostApp::isPlugInterfaceSupported(const TUID _iid)
{
	auto uid = FUID::fromTUID(_iid);
	if (std::find(mFUIDArray.begin(), mFUIDArray.end(), uid) != mFUIDArray.end())
		return kResultTrue;
	return kResultFalse;
}

/* FUnknown methods */
tresult PLUGIN_API VST3HostApp::queryInterface(const TUID _iid, void **obj)
{
	if (FUnknownPrivate::iidEqual(_iid, IHostApplication::iid)) {
		*obj = static_cast<IHostApplication *>(this);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual(_iid, IPlugInterfaceSupport::iid)) {
		*obj = static_cast<IPlugInterfaceSupport *>(this);
		return kResultOk;
	}
#ifdef __linux__
	if (runLoop && FUnknownPrivate::iidEqual(_iid, Linux::IRunLoop::iid)) {
		*obj = static_cast<Linux::IRunLoop *>(runLoop);
		return kResultOk;
	}
#endif
	*obj = nullptr;
	return kNoInterface;
}
