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
#include <pluginterfaces/gui/iplugview.h>

#include <QObject>
#include <QTimer>

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

class QSocketNotifier;

class RunLoopImpl : public QObject, public Steinberg::Linux::IRunLoop {
public:
	RunLoopImpl();
	~RunLoopImpl() override;

	RunLoopImpl(const RunLoopImpl &) = delete;
	RunLoopImpl &operator=(const RunLoopImpl &) = delete;
	RunLoopImpl(RunLoopImpl &&) = delete;
	RunLoopImpl &operator=(RunLoopImpl &&) = delete;

	Steinberg::tresult PLUGIN_API registerEventHandler(Steinberg::Linux::IEventHandler *handler, int fd) override;
	Steinberg::tresult PLUGIN_API unregisterEventHandler(Steinberg::Linux::IEventHandler *handler) override;
	Steinberg::tresult PLUGIN_API registerTimer(Steinberg::Linux::ITimerHandler *handler, uint64_t ms) override;
	Steinberg::tresult PLUGIN_API unregisterTimer(Steinberg::Linux::ITimerHandler *handler) override;
	Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void **obj) override;
	uint32_t PLUGIN_API addRef() override;
	uint32_t PLUGIN_API release() override;

	void stop() { stopping = true; }

private:
	struct TimerSlot {
		Steinberg::Linux::ITimerHandler *handler;
		QTimer *qt;
	};

	void dispatchFD(int fd);
	void updateTimer(TimerSlot *slot);

	bool stopping = false;
	std::mutex eventMutex;
	std::map<int, Steinberg::Linux::IEventHandler *> fdHandlers;
	std::map<int, QSocketNotifier *> fdReadNotifiers;
	std::vector<TimerSlot *> pluginTimers;
};