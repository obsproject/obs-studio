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

constexpr int MIN_THUMBNAIL_UPDATE_INTERVAL_MS = 100;
constexpr int THUMBNAIL_UPDATE_INTERVAL_MS = 5000;

ThumbnailManager::ThumbnailManager(QObject *parent) : QObject(parent)
{
	elapsedTimer.start();
	connect(&updateTimer, &QTimer::timeout, this, &ThumbnailManager::updateTick);
	updateTickInterval(MIN_THUMBNAIL_UPDATE_INTERVAL_MS);

	signalHandlers.emplace_back(obs_get_signal_handler(), "source_destroy", &ThumbnailManager::obsSourceRemoved,
				    this);
}

ThumbnailManager::~ThumbnailManager() {}

QPointer<ThumbnailView> ThumbnailManager::createView(QWidget *parent, obs_source_t *source)
{
	if (!source) {
		return new ThumbnailView(parent, nullptr);
	}

	const char *uuidPointer = obs_source_get_uuid(source);
	if (!uuidPointer) {
		return new ThumbnailView(parent, nullptr);
	}

	std::string uuid{uuidPointer};

	auto item = getThumbnailItem(uuid);
	ThumbnailView *view = item->createView(parent);

	connect(view, &ThumbnailView::updateRequested, this, [this](std::string uuid) {
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
		updateQueue.push_front(uuid);
		updateTickInterval(MIN_THUMBNAIL_UPDATE_INTERVAL_MS);
	} else {
		updateQueue.push_back(uuid);
	}

	connect(item, &ThumbnailItem::noViewsRemaining, this,
		[this, uuid]() { deleteItemById(QString::fromStdString(uuid)); });
}

QPointer<ThumbnailItem> ThumbnailManager::getThumbnailItem(const std::string &uuid)
{
	if (thumbnailList.find(uuid) == thumbnailList.end()) {
		createThumbnailItem(uuid);
	}

	return thumbnailList[uuid];
}

void ThumbnailManager::obsSourceRemoved(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	auto uuidPointer = obs_source_get_uuid(source);

	if (uuidPointer) {
		QMetaObject::invokeMethod(static_cast<ThumbnailManager *>(data), "deleteItemById", Qt::QueuedConnection,
					  Q_ARG(QString, QString::fromStdString(uuidPointer)));
	}
}

bool ThumbnailManager::updateItem(ThumbnailItem *item)
{
	if (!item) {
		return false;
	}

	return item->update();
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
	if (thumbnailList.size() == 0) {
		return;
	}

	QPointer<ThumbnailItem> item;
	bool quickUpdate = false;

	if (priorityQueue.size() > 0) {
		std::string uuid = priorityQueue.front();
		priorityQueue.pop_front();

		item = thumbnailList[uuid];
		updateQueue.push_back(uuid);

		if (!updateItem(item) && cycleDepth < thumbnailList.size()) {
			updateNextItem(cycleDepth + 1);
			return;
		}
	} else if (updateQueue.size() > 0) {
		std::string uuid = updateQueue.front();

		updateQueue.pop_front();
		item = thumbnailList[uuid];
		updateQueue.push_back(uuid);

		if (item->isDefaultPixmap()) {
			quickUpdate = true;
		}

		if (!updateItem(item) && cycleDepth < thumbnailList.size()) {
			updateNextItem(cycleDepth + 1);
			return;
		}
	}

	int nextIntervalMS = MIN_THUMBNAIL_UPDATE_INTERVAL_MS;
	if (priorityQueue.size() == 0 && !quickUpdate) {
		nextIntervalMS = THUMBNAIL_UPDATE_INTERVAL_MS;
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
	if (priorityQueue.size() > 0 && priorityQueue[0] == uuid) {
		return;
	}

	removeIdFromQueues(uuid);

	if (immediate) {
		priorityQueue.push_front(std::string(uuid));

		qint64 elapsed = elapsedTimer.elapsed();
		if (elapsed > MIN_THUMBNAIL_UPDATE_INTERVAL_MS) {
			updateTick();
		}
	} else {
		priorityQueue.push_back(std::string(uuid));
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
	deleteItemById(QString::fromStdString(item->getUuid()));
	item->deleteLater();
}

void ThumbnailManager::deleteItemById(QString uuid_)
{
	std::string uuid = uuid_.toStdString();

	auto entry = thumbnailList.find(uuid);
	if (entry != thumbnailList.end()) {
		removeIdFromQueues(uuid);

		auto item = entry->second;

		if (item && !item->isDefaultPixmap()) {
			std::string uuid = item->getUuid();
			QPixmap pixmap = item->getPixmap();
			addItemToCache(uuid, pixmap);
		}

		thumbnailList.erase(uuid);
	}
}
