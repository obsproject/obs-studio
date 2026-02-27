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
#include "VST3ParamEditQueue.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstunits.h"

#include <vector>

using namespace Steinberg;
using namespace Vst;

class VST3Plugin;

class VST3ComponentHolder : public IComponentHandler, public IUnitHandler {
public:
	VST3ComponentHolder(VST3Plugin *plugin_);
	~VST3ComponentHolder() noexcept;

	//========== IComponentHandler methods ===========//
	VST3ParamEditQueue *guiToDsp = nullptr;

	tresult PLUGIN_API beginEdit(ParamID) override;
	tresult PLUGIN_API performEdit(ParamID id, ParamValue valueNormalized) override;
	tresult PLUGIN_API endEdit(ParamID) override;
	tresult PLUGIN_API restartComponent(int32 flags) override;
	//========== IIUnitHandler methods ===========//
	tresult PLUGIN_API notifyUnitSelection(UnitID) override;
	tresult PLUGIN_API notifyProgramListChange(ProgramListID, int32) override;

	//========== FUnknown methods ===========//
	tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override;
	// we do not care here of the ref-counting. A plug-in call of release should not destroy this class !
	uint32 PLUGIN_API addRef() override { return 1000; }
	uint32 PLUGIN_API release() override { return 1000; }

	IComponentHandler *getComponentHandler() noexcept { return static_cast<IComponentHandler *>(this); }

	//========== VST3 plugin ==========//
	VST3Plugin *plugin = nullptr;
};
