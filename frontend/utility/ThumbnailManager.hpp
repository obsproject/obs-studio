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

#pragma once

#include <obs.hpp>

#include <utility/ScreenshotObj.hpp>
#include <utility/ThumbnailItem.hpp>

#include <QElapsedTimer>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

#include <deque>
#include <functional>

class ThumbnailView;

class ThumbnailManager : public QObject {
	Q_OBJECT

	std::vector<OBSSignal> signalHandlers;
	static void obsSourceRemoved(void *param, calldata_t *calldata);

	struct ThumbnailCache {
		size_t maxSize;

		using CacheEntry = std::pair<std::string, QPixmap>;

		std::list<CacheEntry> cacheList;
		std::unordered_map<std::string, std::list<CacheEntry>::iterator> cacheMap;

		ThumbnailCache(size_t maxSize) : maxSize(maxSize) {}

		void put(const std::string &key, const QPixmap &value)
		{
			auto it = cacheMap.find(key);
			if (it != cacheMap.end()) {
				cacheList.erase(it->second);
				cacheMap.erase(it);
			}
			cacheList.push_front(CacheEntry{key, value});
			cacheMap[key] = cacheList.begin();
			if (cacheMap.size() > maxSize) {
				const CacheEntry &lastEntry = cacheList.back();
				cacheMap.erase(lastEntry.first);
				cacheList.pop_back();
			}
		}
		std::optional<QPixmap> get(const std::string &key)
		{
			auto it = cacheMap.find(key);
			if (it == cacheMap.end()) {
				return std::nullopt;
			}

			std::list<CacheEntry>::iterator entry = it->second;
			return entry->second;
		}
	};

	std::deque<std::string> priorityQueue;
	std::deque<std::string> updateQueue;
	std::unordered_map<std::string, QPointer<ThumbnailItem>> thumbnailList;
	ThumbnailCache thumbnailCache{64};

	QTimer updateTimer;
	QElapsedTimer elapsedTimer;

public:
	explicit ThumbnailManager(QObject *parent = nullptr);
	~ThumbnailManager();

	QPointer<ThumbnailView> createView(QWidget *parent, obs_source_t *source);
	std::optional<QPixmap> getCachedPixmap(const std::string &uuid);

private:
	Q_DISABLE_COPY_MOVE(ThumbnailManager);

	bool updateItem(ThumbnailItem *item);
	void updateNextItem(size_t cycleDepth = 0);

	void removeIdFromQueues(const std::string &uuid);
	void updateTickInterval(int newInterval);

	void createThumbnailItem(const std::string &uuid);
	QPointer<ThumbnailItem> getThumbnailItem(const std::string &uuid);

private slots:
	void updateTick();
	void addToPriorityQueue(const std::string &uuid, bool immediate = false);
	void addItemToCache(std::string &uuid, QPixmap &pixmap);
	void deleteItem(ThumbnailItem *item);
	void deleteItemById(QString uuid);
};
