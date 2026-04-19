/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>
                          Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "ThumbnailManager.hpp"

#include <utility/ThumbnailView.hpp>
#include <widgets/OBSBasic.hpp>

#include "display-helpers.hpp"

#include <QImageWriter>

constexpr int kMinimumThumbnailUpdateInterval = 100;
constexpr int kThumbnailUpdateInterval = 5000;

namespace {
bool updateItem(ThumbnailItem *item)
{
	if (!item) {
		return false;
	}

	return item->update();
}
} // namespace

void ThumbnailManager::ThumbnailCache::put(const std::string &key, const QPixmap &value)
{
	auto it = cacheMap.find(key);
	if (it != cacheMap.end()) {
		cacheList.erase(it->second);
		cacheMap.erase(it);
	}
	cacheList.emplace_front(key, value);
	cacheMap[key] = cacheList.begin();
	if (cacheMap.size() > maxSize) {
		const CacheEntry &lastEntry = cacheList.back();
		cacheMap.erase(lastEntry.first);
		cacheList.pop_back();
	}
}

std::optional<QPixmap> ThumbnailManager::ThumbnailCache::get(const std::string &key)
{
	auto it = cacheMap.find(key);
	if (it == cacheMap.end()) {
		return std::nullopt;
	}

	auto entry = it->second;
	return entry->second;
}

ThumbnailManager::ThumbnailManager(QObject *parent) : QObject(parent)
{
	elapsedTimer.start();
	connect(&updateTimer, &QTimer::timeout, this, &ThumbnailManager::updateTick);
	updateTickInterval(kMinimumThumbnailUpdateInterval);

	signalHandlers.emplace_back(obs_get_signal_handler(), "source_destroy", &ThumbnailManager::obsSourceRemoved,
				    this);
}

ThumbnailView *ThumbnailManager::createView(QWidget *parent, obs_source_t *source)
{
	if (!source) {
		return new ThumbnailView(parent, nullptr);
	}

	const char *uuidPointer = obs_source_get_uuid(source);
	if (!uuidPointer) {
		return new ThumbnailView(parent, nullptr);
	}

	std::string uuid{uuidPointer};

	ThumbnailItem *item = getThumbnailItem(uuid);
	ThumbnailView *view = item->createView(parent);

	connect(view, &ThumbnailView::updateRequested, this, [this](const std::string &uuid) {
		bool updateImmediately = true;
		addToPriorityQueue(uuid, updateImmediately);
	});

	return view;
}

std::optional<QPixmap> ThumbnailManager::getCachedPixmap(const std::string &uuid)
{
	return thumbnailCache.get(uuid);
}

void ThumbnailManager::createThumbnailItem(const std::string &uuid)
{
	QPointer<ThumbnailItem> item = new ThumbnailItem(uuid, this);
	thumbnailList[uuid] = item;

	if (item->isDefaultPixmap()) {
		updateQueue.emplace_front(uuid);
		updateTickInterval(kMinimumThumbnailUpdateInterval);
	} else {
		updateQueue.emplace_back(uuid);
	}

	connect(item, &ThumbnailItem::noViewsRemaining, this, [this, uuid]() { deleteItemById(uuid); });
}

ThumbnailItem *ThumbnailManager::getThumbnailItem(const std::string &uuid)
{
	if (thumbnailList.find(uuid) == thumbnailList.end()) {
		createThumbnailItem(uuid);
	}

	return thumbnailList[uuid];
}

void ThumbnailManager::obsSourceRemoved(void *data, calldata_t *params)
{
	auto *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	const char *uuidPointer = obs_source_get_uuid(source);

	if (!uuidPointer) {
		return;
	}

	auto *manager = static_cast<ThumbnailManager *>(data);
	QMetaObject::invokeMethod(manager, "deleteItemById", Qt::QueuedConnection, Q_ARG(std::string, uuidPointer));
}

void ThumbnailManager::updateTickInterval(int newInterval)
{
	if (updateTimer.interval() != newInterval) {
		elapsedTimer.restart();
		updateTimer.start(newInterval);
	}
}

void ThumbnailManager::updateNextItem(size_t cycleDepth)
{
	if (thumbnailList.empty()) {
		return;
	}

	QPointer<ThumbnailItem> item;
	bool quickUpdate = false;

	if (!priorityQueue.empty()) {
		std::string uuid = priorityQueue.front();
		priorityQueue.pop_front();

		item = thumbnailList[uuid];
		updateQueue.emplace_back(std::move(uuid));

		if (!updateItem(item) && cycleDepth < thumbnailList.size()) {
			updateNextItem(cycleDepth + 1);
			return;
		}
	} else if (!updateQueue.empty()) {
		std::string uuid = updateQueue.front();

		updateQueue.pop_front();
		item = thumbnailList[uuid];
		updateQueue.emplace_back(std::move(uuid));

		if (item->isDefaultPixmap()) {
			quickUpdate = true;
		}

		if (!updateItem(item) && cycleDepth < thumbnailList.size()) {
			updateNextItem(cycleDepth + 1);
			return;
		}
	}

	int nextIntervalMS = kMinimumThumbnailUpdateInterval;
	if (!priorityQueue.empty() && !quickUpdate) {
		nextIntervalMS = kThumbnailUpdateInterval;
	}

	updateTickInterval(nextIntervalMS);
}

void ThumbnailManager::updateTick()
{
	updateNextItem();
}

void ThumbnailManager::removeIdFromQueues(const std::string &uuid)
{
	auto it = std::find(updateQueue.begin(), updateQueue.end(), uuid);
	if (it != updateQueue.end()) {
		updateQueue.erase(it);
	}

	it = std::find(priorityQueue.begin(), priorityQueue.end(), uuid);
	if (it != priorityQueue.end()) {
		priorityQueue.erase(it);
	}
}

void ThumbnailManager::addToPriorityQueue(const std::string &uuid, bool immediate)
{
	// Skip if uuid is already at the front of the priority queue
	if (!priorityQueue.empty() && priorityQueue[0] == uuid) {
		return;
	}

	removeIdFromQueues(uuid);

	if (immediate) {
		priorityQueue.emplace_front(uuid);

		qint64 elapsed = elapsedTimer.elapsed();
		if (elapsed > kMinimumThumbnailUpdateInterval) {
			updateTick();
		}
	} else {
		priorityQueue.emplace_back(uuid);
	}
}

void ThumbnailManager::addItemToCache(std::string &uuid, QPixmap &pixmap)
{
	if (pixmap.isNull()) {
		return;
	}

	thumbnailCache.put(uuid, pixmap);
}

void ThumbnailManager::deleteItem(ThumbnailItem *item)
{
	deleteItemById(item->getUuid());
	item->deleteLater();
}

void ThumbnailManager::deleteItemById(const std::string &uuid)
{
	auto entry = thumbnailList.find(uuid);
	if (entry != thumbnailList.end()) {
		removeIdFromQueues(uuid);

		QPointer<ThumbnailItem> item = entry->second;

		if (item && !item->isDefaultPixmap()) {
			std::string uuid = item->getUuid();
			QPixmap pixmap = item->getPixmap();
			addItemToCache(uuid, pixmap);
		}

		thumbnailList.erase(uuid);
	}
}
