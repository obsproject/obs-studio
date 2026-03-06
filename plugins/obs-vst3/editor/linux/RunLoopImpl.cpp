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
#include <QSocketNotifier>
#include <QMetaObject>
#include "RunLoopImpl.h"

RunLoopImpl::RunLoopImpl(Display *dpy) : QObject(), display(dpy)
{
	hostEventTimer = new QTimer(this);
	hostEventTimer->setSingleShot(false);
	hostEventTimer->setInterval(20);
	QObject::connect(hostEventTimer, &QTimer::timeout, [this]() { this->tickHostEvents(); });
}

void RunLoopImpl::UpdateTimer(TimerSlot *slot)
{
	if (stopping || !slot)
		return;

	auto *handler = slot->handler;
	if (handler)
		handler->onTimer();
}

void RunLoopImpl::tickHostEvents()
{
	if (stopping || windowHandlers.empty())
		return;

	static const long mask = StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask | FocusChangeMask |
				 ExposureMask;
	XEvent ev;

	auto it = windowHandlers.begin();
	while (it != windowHandlers.end()) {
		Window w = it->first;
		auto handler = it->second;
		bool erased = false;
		while (XCheckWindowEvent(display, w, mask, &ev)) {
			if (ev.type == DestroyNotify) {
				it = windowHandlers.erase(it);
				erased = true;
				break;
			}
			handler(ev);
		}
		if (erased)
			continue;

		while (XCheckTypedWindowEvent(display, w, ClientMessage, &ev))
			handler(ev);

		++it;
	}
}

void RunLoopImpl::dispatchFD(int fd)
{
	if (stopping)
		return;

	Steinberg::Linux::IEventHandler *handler = nullptr;
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		auto it = fdHandlers.find(fd);
		if (it == fdHandlers.end() || it->second == nullptr)
			return;
		handler = it->second;
		handler->addRef();
	}
	handler->onFDIsSet(fd);
	handler->release();
}

Steinberg::tresult RunLoopImpl::registerEventHandler(Steinberg::Linux::IEventHandler *handler, int fd)
{
	if (!handler || fd < 0)
		return Steinberg::kInvalidArgument;
	{
		std::lock_guard<std::mutex> lock(eventMutex);
		if (fdHandlers.count(fd))
			return Steinberg::kInvalidArgument;
		fdHandlers[fd] = handler;
		handler->addRef();
	}
	auto *rn = new QSocketNotifier(fd, QSocketNotifier::Read, this);

	QObject::connect(rn, &QSocketNotifier::activated, this, [this, fd](int) {
		QMetaObject::invokeMethod(this, "dispatchFD", Qt::QueuedConnection, Q_ARG(int, fd));
	});

	{
		std::lock_guard<std::mutex> lock(eventMutex);
		fdReadNotifiers[fd] = rn;
	}

	return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API RunLoopImpl::unregisterEventHandler(Steinberg::Linux::IEventHandler *handler)
{
	if (!handler)
		return Steinberg::kInvalidArgument;

	std::vector<int> toRemoveFds;
	std::vector<QSocketNotifier *> toDeleteNotifiers;

	{
		std::lock_guard<std::mutex> lock(eventMutex);
		for (auto it = fdHandlers.begin(); it != fdHandlers.end();) {
			if (it->second == handler) {
				int fd = it->first;
				toRemoveFds.push_back(fd);

				auto rnIt = fdReadNotifiers.find(fd);
				if (rnIt != fdReadNotifiers.end()) {
					toDeleteNotifiers.push_back(rnIt->second);
					fdReadNotifiers.erase(rnIt);
				}

				it = fdHandlers.erase(it);
			} else {
				++it;
			}
		}
	}

	if (toRemoveFds.empty())
		return Steinberg::kResultFalse;

	for (QSocketNotifier *sn : toDeleteNotifiers) {
		if (!sn)
			continue;

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
	if (!handler || ms == 0)
		return Steinberg::kInvalidArgument;

	for (const auto *slot : pluginTimers) {
		if (slot->handler == handler)
			return Steinberg::kResultFalse;
	}
	handler->addRef();
	auto *slot = new TimerSlot{handler, new QTimer(this)};
	slot->qt->setInterval(static_cast<int>(ms));
	QObject::connect(slot->qt, &QTimer::timeout, this, [this, slot]() { this->UpdateTimer(slot); });
	slot->qt->setSingleShot(false);
	slot->qt->start();
	pluginTimers.push_back(slot);

	return Steinberg::kResultTrue;
}

Steinberg::tresult RunLoopImpl::unregisterTimer(Steinberg::Linux::ITimerHandler *handler)
{
	if (!handler)
		return Steinberg::kInvalidArgument;

	auto it = std::find_if(pluginTimers.begin(), pluginTimers.end(),
			       [&](TimerSlot *slot) { return slot && slot->handler == handler; });

	if (it == pluginTimers.end())
		return Steinberg::kResultFalse;

	TimerSlot *slot = *it;
	if (slot->qt) {
		slot->qt->stop();
		delete slot->qt;
	}
	if (slot->handler) {
		slot->handler->release();
	}
	delete slot;
	pluginTimers.erase(it);
	return Steinberg::kResultTrue;
}

void RunLoopImpl::registerWindow(Window win, std::function<bool(const XEvent &)> handler)
{
	std::lock_guard<std::mutex> lock(hostEventMutex);
	windowHandlers[win] = std::move(handler);
	if (!hostEventTimer->isActive())
		hostEventTimer->start();
}

void RunLoopImpl::unregisterWindow(Window win)
{
	std::lock_guard<std::mutex> lock(hostEventMutex);
	windowHandlers.erase(win);
	if (windowHandlers.empty())
		hostEventTimer->stop();
}

// FUnknown support
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
	if (Steinberg::FUnknownPrivate::iidEqual(iid, Steinberg::Linux::IRunLoop::iid)) {
		*obj = static_cast<Steinberg::Linux::IRunLoop *>(this);
		return Steinberg::kResultOk;
	}
	*obj = nullptr;
	return Steinberg::kNoInterface;
}

RunLoopImpl::~RunLoopImpl()
{
	stopping = true;
	if (hostEventTimer) {
		std::lock_guard<std::mutex> lock(hostEventMutex);
		hostEventTimer->stop();
		delete hostEventTimer;
	}
	for (auto slot : pluginTimers) {
		slot->qt->stop();
		slot->qt->deleteLater();
		slot->handler->release();
		delete slot;
	}
	pluginTimers.clear();
}
