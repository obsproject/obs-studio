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
#include "VST3ComponentHolder.h"
#include "VST3Plugin.h"

using namespace Steinberg;
using namespace Vst;

VST3ComponentHolder::VST3ComponentHolder(VST3Plugin *plugin_) : plugin(plugin_) {}

VST3ComponentHolder::~VST3ComponentHolder() noexcept {FUNKNOWN_DTOR}

/* IComponentHandler methods */
tresult PLUGIN_API VST3ComponentHolder::beginEdit(ParamID)
{
	return kResultOk;
}

tresult PLUGIN_API VST3ComponentHolder::performEdit(ParamID id, ParamValue valueNormalized)
{
	if (guiToDsp)
		guiToDsp->push({id, valueNormalized});

	return kResultOk;
}

tresult PLUGIN_API VST3ComponentHolder::endEdit(ParamID)
{
	return kResultOk;
}

tresult PLUGIN_API VST3ComponentHolder::restartComponent(int32 flags)
{
	if (!plugin)
		return kInvalidArgument;

	if ((flags & kReloadComponent) || (flags & kLatencyChanged)) {
		plugin->obsVst3Struct->bypass.store(true, std::memory_order_relaxed);

		if (plugin->audioEffect)
			plugin->audioEffect->setProcessing(false);

		if (plugin->vstPlug) {
			plugin->vstPlug->setActive(false);
			plugin->vstPlug->setActive(true);
		}

		if (plugin->audioEffect)
			plugin->audioEffect->setProcessing(true);

		uint32 latency = plugin->audioEffect ? plugin->audioEffect->getLatencySamples() : 0;
		infovst3plugin("Latency of the plugin is %u samples", latency);

		plugin->obsVst3Struct->bypass.store(false, std::memory_order_relaxed);
		return kResultOk;
	}

	return kNotImplemented;
}

/* IUnitHandler methods */
tresult PLUGIN_API VST3ComponentHolder::notifyUnitSelection(UnitID)
{
	return kResultTrue;
}

tresult PLUGIN_API VST3ComponentHolder::notifyProgramListChange(ProgramListID, int32)
{
	return kResultTrue;
}

/* FUnknown methods */
tresult PLUGIN_API VST3ComponentHolder::queryInterface(const TUID _iid, void **obj)
{
	if (FUnknownPrivate::iidEqual(_iid, IComponentHandler::iid)) {
		*obj = static_cast<IComponentHandler *>(this);
		return kResultOk;
	}
	if (FUnknownPrivate::iidEqual(_iid, IUnitHandler::iid)) {
		*obj = static_cast<IUnitHandler *>(this);
		return kResultOk;
	}
	*obj = nullptr;
	return kNoInterface;
}
