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
#pragma once
#ifdef __linux__
#include "editor/linux/RunLoopImpl.h"
#endif
#include "pluginterfaces/vst/ivstpluginterfacesupport.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
using namespace Steinberg;
using namespace Vst;

class VST3Plugin;

class VST3HostApp : public IHostApplication, public IPlugInterfaceSupport {
public:
	VST3HostApp();
	~VST3HostApp() noexcept;
	//========== IHostApplication methods ===========//
	tresult PLUGIN_API getName(String128 name) override;
	tresult PLUGIN_API createInstance(TUID cid, TUID _iid, void **obj) override;

	//========== IPlugInterfaceSupport methods ===========//
	tresult PLUGIN_API isPlugInterfaceSupported(const TUID _iid) override;

	//========== FUnknown methods ===========//
	tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override;
	// we do not care here of the ref-counting. A plug-in call of release should not destroy this class !
	uint32 PLUGIN_API addRef() override { return 1000; }
	uint32 PLUGIN_API release() override { return 1000; }

	FUnknown *getFUnknown() noexcept { return static_cast<FUnknown *>(static_cast<IHostApplication *>(this)); }

#ifdef __linux__
	//========== Pass Runloop ===========//
	void setRunLoop(Steinberg::Linux::IRunLoop *rl) { runLoop = rl; }

private:
	Steinberg::Linux::IRunLoop *runLoop = nullptr;
#endif
private:
	std::vector<FUID> mFUIDArray;
	void addPlugInterfaceSupported(const TUID _iid) { mFUIDArray.push_back(FUID::fromTUID(_iid)); }
};
