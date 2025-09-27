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

#include <utility/ScreenshotObj.hpp>
#include <widgets/OBSBasic.hpp>

#include "display-helpers.hpp"

#include <QImageWriter>

constexpr int MIN_THUMBNAIL_UPDATE_INTERVAL_MS = 100;
constexpr int MIN_SOURCE_UPDATE_INTERVAL_MS = 5000;

ThumbnailItem::ThumbnailItem(std::string uuid, OBSSource source) : uuid(uuid), weakSource(OBSGetWeakRef(source)) {}

void ThumbnailItem::init(std::weak_ptr<ThumbnailItem> weakActiveItem)
{
	auto thumbnailManager = OBSBasic::Get()->thumbnails();
	if (!thumbnailManager) {
		return;
	}

	auto it = thumbnailManager->cachedThumbnails.find(uuid);
	if (it != thumbnailManager->cachedThumbnails.end()) {
		auto &cachedItem = it->second;
		pixmap = cachedItem.pixmap.value_or(QPixmap());
		cachedItem.pixmap.reset();
		cachedItem.weakActiveItem = std::move(weakActiveItem);
	}
}

ThumbnailItem::~ThumbnailItem()
{
	auto thumbnailManager = OBSBasic::Get()->thumbnails();
	if (!thumbnailManager) {
		return;
	}

	auto &cachedItem = thumbnailManager->cachedThumbnails[uuid];
	cachedItem.pixmap = pixmap;
	cachedItem.weakActiveItem.reset();
}

void ThumbnailItem::imageUpdated(QImage image)
{
	QPixmap newPixmap;
	if (!image.isNull()) {
		newPixmap = QPixmap::fromImage(image);
	}

	pixmap = newPixmap;
	emit updateThumbnail(pixmap);
}

void Thumbnail::thumbnailUpdated(QPixmap pixmap)
{
	emit updateThumbnail(pixmap);
}

ThumbnailManager::ThumbnailManager(QObject *parent) : QObject(parent)
{
	connect(&updateTimer, &QTimer::timeout, this, &ThumbnailManager::updateTick);
}

ThumbnailManager::~ThumbnailManager() {}

std::shared_ptr<Thumbnail> ThumbnailManager::getThumbnail(OBSSource source)
{
	std::string uuid = obs_source_get_uuid(source);

	for (auto it = thumbnails.begin(); it != thumbnails.end(); ++it) {
		auto item = it->lock();
		if (item && item->uuid == uuid) {
			return std::make_shared<Thumbnail>(item);
		}
	}

	std::shared_ptr<Thumbnail> thumbnail;
	if ((obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) != 0) {
		auto item = std::make_shared<ThumbnailItem>(uuid, source);
		item->init(std::weak_ptr<ThumbnailItem>(item));

		thumbnail = std::make_shared<Thumbnail>(item);
		connect(item.get(), &ThumbnailItem::updateThumbnail, thumbnail.get(), &Thumbnail::thumbnailUpdated);

		newThumbnails.push_back(std::weak_ptr<ThumbnailItem>(item));
	}

	updateIntervalChanged(thumbnails.size());
	return thumbnail;
}

bool ThumbnailManager::updatePixmap(std::shared_ptr<ThumbnailItem> &sharedPointerItem)
{
	ThumbnailItem *item = sharedPointerItem.get();

	OBSSource source = OBSGetStrongRef(item->weakSource);
	if (!source) {
		return true;
	}

	QPixmap pixmap;
	item->pixmap = pixmap;

	if (source) {
		uint32_t sourceWidth = obs_source_get_width(source);
		uint32_t sourceHeight = obs_source_get_height(source);

		if (sourceWidth == 0 || sourceHeight == 0) {
			return true;
		}

		auto obj = new ScreenshotObj(source);
		obj->setSaveToFile(false);
		obj->setSize(Thumbnail::size);

		connect(obj, &ScreenshotObj::imageReady, item, &ThumbnailItem::imageUpdated);
	}

	return true;
}

void ThumbnailManager::updateIntervalChanged(size_t newCount)
{
	int intervalMS = MIN_THUMBNAIL_UPDATE_INTERVAL_MS;
	if (newThumbnails.size() == 0 && newCount > 0) {
		int count = (int)newCount;
		intervalMS = MIN_SOURCE_UPDATE_INTERVAL_MS / count;
		if (intervalMS < MIN_THUMBNAIL_UPDATE_INTERVAL_MS) {
			intervalMS = MIN_THUMBNAIL_UPDATE_INTERVAL_MS;
		}
	}

	updateTimer.start(intervalMS);
}

void ThumbnailManager::updateTick()
{
	std::shared_ptr<ThumbnailItem> item;
	bool changed = false;
	bool newThumbnail = false;

	while (newThumbnails.size() > 0) {
		changed = true;
		item = newThumbnails.front().lock();

		newThumbnails.pop_front();
		if (item) {
			newThumbnail = true;
			break;
		}
	}

	if (!item) {
		while (thumbnails.size() > 0) {
			item = thumbnails.front().lock();
			thumbnails.pop_front();
			if (item) {
				break;
			} else {
				changed = true;
			}
		}
	}
	if (changed && newThumbnails.size() == 0) {
		updateIntervalChanged(thumbnails.size() + (item ? 1 : 0));
	}
	if (!item) {
		return;
	}

	if (updatePixmap(item)) {
		thumbnails.push_back(std::weak_ptr<ThumbnailItem>(item));
	} else {
		thumbnails.push_front(std::weak_ptr<ThumbnailItem>(item));
	}
}

std::optional<QPixmap> ThumbnailManager::getCachedThumbnail(OBSSource source)
{
	if (!source) {
		return std::nullopt;
	}

	std::string uuid = obs_source_get_uuid(source);
	auto it = cachedThumbnails.find(uuid);
	if (it != cachedThumbnails.end()) {
		auto &cachedItem = it->second;
		if (cachedItem.pixmap.has_value()) {
			return cachedItem.pixmap;
		}

		auto activeItem = cachedItem.weakActiveItem.lock();
		return activeItem ? std::make_optional(activeItem->pixmap) : std::nullopt;
	}

	return std::nullopt;
}

void ThumbnailManager::preloadThumbnail(OBSSource source, QObject *object, std::function<void(QPixmap)> callback)
{
	if (!source) {
		return;
	}

	std::string uuid = obs_source_get_uuid(source);

	if (cachedThumbnails.find(uuid) == cachedThumbnails.end()) {
		uint32_t sourceWidth = obs_source_get_width(source);
		uint32_t sourceHeight = obs_source_get_height(source);

		cachedThumbnails[uuid].pixmap = QPixmap();
		if (sourceWidth == 0 || sourceHeight == 0) {
			return;
		}

		auto obj = new ScreenshotObj(source);
		obj->setSaveToFile(false);
		obj->setSize(Thumbnail::size);

		QPointer<QObject> safeObject = qobject_cast<QObject *>(object);
		connect(obj, &ScreenshotObj::imageReady, this, [=](QImage image) {
			QPixmap pixmap;
			if (!image.isNull()) {
				pixmap = QPixmap::fromImage(image);
			}
			cachedThumbnails[uuid].pixmap = pixmap;

			QMetaObject::invokeMethod(
				safeObject,
				[safeObject, callback, pixmap]() {
					if (safeObject) {
						callback(pixmap);
					}
				},
				Qt::QueuedConnection);
		});
	}
}
