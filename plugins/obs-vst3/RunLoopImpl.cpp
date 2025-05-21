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

#include "RunLoopImpl.h"

#include <pluginterfaces/gui/iplugview.h>

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <algorithm>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

DEF_CLASS_IID(Steinberg::Linux::IRunLoop)

RunLoopImpl::RunLoopImpl() = default;

void RunLoopImpl::updateTimer(TimerSlot *slot)
{
	if (stopping || !slot) {
		return;
	}

	auto *handler = slot->handler;
	if (handler) {
		handler->addRef();
		handler->onTimer();
		handler->release();
	}
}

void RunLoopImpl::dispatchFD(int fd)
{
	if (stopping) {
		return;
	}

	Steinberg::Linux::IEventHandler *handler = nullptr;
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		auto it = fdHandlers.find(fd);
		if (it == fdHandlers.end() || it->second == nullptr) {
			return;
		}
		handler = it->second;
		handler->addRef();
	}
	handler->onFDIsSet(fd);
	handler->release();
}

Steinberg::tresult RunLoopImpl::registerEventHandler(Steinberg::Linux::IEventHandler *handler, int fd)
{
	if (!handler || fd < 0) {
		return Steinberg::kInvalidArgument;
	}
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		if (fdHandlers.count(fd)) {
			return Steinberg::kInvalidArgument;
		}
		fdHandlers[fd] = handler;
		handler->addRef();
	}
	auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);

	QObject::connect(notifier, &QSocketNotifier::activated, this, [this, fd] { dispatchFD(fd); });

	{
		std::lock_guard<std::mutex> lock(eventMutex);
		fdReadNotifiers[fd] = notifier;
	}

	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API RunLoopImpl::unregisterEventHandler(Steinberg::Linux::IEventHandler *handler)
{
	if (!handler) {
		return Steinberg::kInvalidArgument;
	}

	std::vector<int> toRemoveFds;
	std::vector<QSocketNotifier *> toDeleteNotifiers;

	{
		std::lock_guard<std::mutex> lock(eventMutex);
		for (auto it = fdHandlers.begin(); it != fdHandlers.end();) {
			if (it->second == handler) {
				int fd = it->first;
				toRemoveFds.push_back(fd);

				auto notifierIt = fdReadNotifiers.find(fd);
				if (notifierIt != fdReadNotifiers.end()) {
					toDeleteNotifiers.push_back(notifierIt->second);
					fdReadNotifiers.erase(notifierIt);
				}

				it = fdHandlers.erase(it);
			} else {
				++it;
			}
		}
	}

	if (toRemoveFds.empty()) {
		return Steinberg::kResultFalse;
	}

	for (QSocketNotifier *sn : toDeleteNotifiers) {
		if (!sn) {
			continue;
		}

		sn->setEnabled(false);
		sn->deleteLater();
	}

	for (size_t i = 0; i < toRemoveFds.size(); ++i) {
		handler->release();
	}

	return Steinberg::kResultTrue;
}

Steinberg::tresult RunLoopImpl::registerTimer(Steinberg::Linux::ITimerHandler *handler, uint64_t ms)
{
	if (!handler || ms == 0) {
		return Steinberg::kInvalidArgument;
	}

	for (const auto *slot : pluginTimers) {
		if (slot->handler == handler) {
			return Steinberg::kResultFalse;
		}
	}
	handler->addRef();
	auto *slot = new TimerSlot{handler, new QTimer(this)};
	slot->qt->setInterval(static_cast<int>(ms));
	QObject::connect(slot->qt, &QTimer::timeout, this, [this, slot]() { updateTimer(slot); });
	slot->qt->start();
	pluginTimers.push_back(slot);

	return Steinberg::kResultTrue;
}

Steinberg::tresult RunLoopImpl::unregisterTimer(Steinberg::Linux::ITimerHandler *handler)
{
	if (!handler) {
		return Steinberg::kInvalidArgument;
	}

	auto it = std::find_if(pluginTimers.begin(), pluginTimers.end(),
			       [&](TimerSlot *slot) { return slot && slot->handler == handler; });

	if (it == pluginTimers.end()) {
		return Steinberg::kResultFalse;
	}

	TimerSlot *slot = *it;
	if (slot->qt) {
		slot->qt->stop();
		slot->qt->disconnect(this);
		slot->qt->deleteLater();
	}
	if (slot->handler) {
		slot->handler->release();
	}
	delete slot;
	pluginTimers.erase(it);
	return Steinberg::kResultTrue;
}

uint32_t RunLoopImpl::addRef()
{
	return 1000;
}

uint32_t RunLoopImpl::release()
{
	return 1000;
}

Steinberg::tresult RunLoopImpl::queryInterface(const Steinberg::TUID iid, void **obj)
{
	if (Steinberg::FUnknownPrivate::iidEqual(iid, Steinberg::Linux::IRunLoop::iid) ||
	    Steinberg::FUnknownPrivate::iidEqual(iid, Steinberg::FUnknown::iid)) {
		*obj = static_cast<Steinberg::Linux::IRunLoop *>(this);
		return Steinberg::kResultOk;
	}
	*obj = nullptr;
	return Steinberg::kNoInterface;
}

RunLoopImpl::~RunLoopImpl()
{
	stopping = true;

	std::vector<Steinberg::Linux::IEventHandler *> eventHandlers;
	{
		std::lock_guard<std::mutex> lock(eventMutex);

		for (const auto &entry : fdReadNotifiers) {
			auto *notifier = entry.second;
			if (notifier) {
				notifier->setEnabled(false);
			}
		}
		fdReadNotifiers.clear();

		for (const auto &entry : fdHandlers) {
			auto *handler = entry.second;
			if (handler) {
				eventHandlers.push_back(handler);
			}
		}
		fdHandlers.clear();
	}

	for (auto *handler : eventHandlers) {
		handler->release();
	}

	for (auto *slot : pluginTimers) {
		if (slot->qt) {
			slot->qt->stop();
			delete slot->qt;
		}
		if (slot->handler) {
			slot->handler->release();
		}
		delete slot;
	}

	pluginTimers.clear();
}
