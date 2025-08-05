/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "display-helpers.hpp"
#include "ThumbnailManager.hpp"
#include <utility/ScreenshotObj.hpp>

#include <QImageWriter>

#define UPDATE_INTERVAL 50
#define THROTTLE_LIMIT 10
#define STALE_SECONDS 60

ThumbnailManager::ThumbnailManager()
{
	texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);

	signal_handler_t *sh = obs_get_signal_handler();
	sigs.emplace_back(sh, "source_create", obsSourceAdded, this);
	sigs.emplace_back(sh, "source_remove", obsSourceRemoved, this);

	updateTimer = new QTimer();
	connect(updateTimer, &QTimer::timeout, this, &ThumbnailManager::updateTick);
	updateTimer->start(UPDATE_INTERVAL);
}

ThumbnailManager::~ThumbnailManager()
{
	updateTimer->stop();
	disconnect(updateTimer, &QTimer::timeout, this, &ThumbnailManager::updateTick);
}

QPixmap ThumbnailManager::getThumbnail(OBSSource source)
{
	std::string uuid = obs_source_get_uuid(source);

	if (thumbnails.find(uuid) != thumbnails.end()) {
		return thumbnails[uuid].pixmap;
	}

	return QPixmap();
}

void ThumbnailManager::obsSourceAdded(void *param, calldata_t *calldata)
{
	OBSSource source((obs_source_t *)calldata_ptr(calldata, "source"));
	QMetaObject::invokeMethod(static_cast<ThumbnailManager *>(param), "sourceAdded", Q_ARG(OBSSource, source));
}

void ThumbnailManager::obsSourceRemoved(void *param, calldata_t *calldata)
{
	OBSSource source((obs_source_t *)calldata_ptr(calldata, "source"));
	QMetaObject::invokeMethod(static_cast<ThumbnailManager *>(param), "sourceRemoved", Q_ARG(OBSSource, source));
}

void ThumbnailManager::sourceAdded(OBSSource source)
{
	std::string uuid = obs_source_get_uuid(source);
	ThumbnailItem newEntry = {uuid, {}, QPixmap()};

	thumbnails.emplace(uuid, newEntry);
}

void ThumbnailManager::sourceRemoved(OBSSource source)
{
	std::string uuid = obs_source_get_uuid(source);
	if (thumbnails.find(uuid) != thumbnails.end()) {
		thumbnails.erase(uuid);
	}
}

void ThumbnailManager::updateTick()
{
	if (thumbnails.size() == 0) {
		return;
	}

	if (updateThrottle > 0) {
		updateThrottle--;
		return;
	}

	/* updateTick() is called every UPDATE_INTERVAL ms but will only run every THROTTLE_LIMIT ticks.
	 * This value decrements every tick and is explicitly set to 0 to force a render without throttling.
	 * This is used for when sources have no preview at all yet, or recently became visible and are out of date.
	 */
	updateThrottle = THROTTLE_LIMIT;

	ThumbnailItem oldestItem;

	auto steadyNow = steady_clock::now();
	long long lastUpdateLimit = STALE_SECONDS * 1000;

	for (auto it = thumbnails.begin(); it != thumbnails.end();) {
		ThumbnailItem entry = it->second;

		OBSSourceAutoRelease source = obs_get_source_by_uuid(entry.uuid.c_str());
		if (!source) {
			it = thumbnails.erase(it);
			continue;
		}
		it++;

		bool isShowing = obs_source_showing(source);
		bool isScene = obs_source_is_scene(source);
		uint32_t flags = obs_source_get_output_flags(source);

		if ((flags & OBS_SOURCE_VIDEO) == 0) {
			continue;
		}

		if (oldestItem.isNull()) {
			oldestItem = entry;
		}

		/* Force updates for items with no update yet */
		if (!entry.lastUpdate.has_value()) {
			updateThrottle = 0;
			oldestItem = entry;
			break;
		}

		/* Force updates for items with no image that are showing now */
		if (entry.pixmap.isNull() && isShowing) {
			updateThrottle = 0;
			oldestItem = entry;
			break;
		}

		long long timeSinceEntryUpdate = duration_cast<milliseconds>(steadyNow - *entry.lastUpdate).count();
		if (timeSinceEntryUpdate < lastUpdateLimit) {
			continue;
		}

		if (!isShowing && !isScene) {
			continue;
		}

		bool entryIsOlder = (steadyNow - *entry.lastUpdate) > (steadyNow - *oldestItem.lastUpdate);
		if (timeSinceEntryUpdate > lastUpdateLimit && entryIsOlder) {
			oldestItem = entry;
			continue;
		}
	}

	if (oldestItem.isNull()) {
		return;
	}

	OBSSourceAutoRelease source = obs_get_source_by_uuid(oldestItem.uuid.c_str());
	if (oldestItem.lastUpdate.has_value()) {
		long long updateTime = duration_cast<milliseconds>(steadyNow - *oldestItem.lastUpdate).count();
		if (updateTime < lastUpdateLimit) {
			return;
		}

		if (updateTime > lastUpdateLimit * 2) {
			/* Update thumbnails faster when the oldest item is more than double the state limit */
			updateThrottle = std::min(updateThrottle, 3);
		}
	}

	QPixmap pixmap;
	thumbnails[oldestItem.uuid].pixmap = pixmap;
	thumbnails[oldestItem.uuid].lastUpdate = steadyNow;
	if (source) {
		uint32_t sourceWidth = obs_source_get_width(source);
		uint32_t sourceHeight = obs_source_get_height(source);

		if (sourceWidth == 0 || sourceHeight == 0) {
			return;
		}

		auto obj = new ScreenshotObj(source);
		obj->setSaveToFile(false);
		obj->setSize(320, 180);

		connect(obj, &ScreenshotObj::imageReady, this, [=](QImage image) {
			QPixmap pixmap;
			if (!image.isNull()) {
				pixmap = QPixmap::fromImage(image);
			}

			auto updateTime = steady_clock::now();
			thumbnails[oldestItem.uuid].pixmap = pixmap;
			thumbnails[oldestItem.uuid].lastUpdate = updateTime;
		});
	}
}
