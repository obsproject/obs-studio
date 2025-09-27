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

#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

#include <deque>
#include <functional>

class ThumbnailItem : public QObject {
	Q_OBJECT

	friend class ThumbnailManager;
	friend class Thumbnail;

	std::string uuid;
	OBSWeakSource weakSource;
	QPixmap pixmap;

	void init(std::weak_ptr<ThumbnailItem> weakActiveItem);
	void imageUpdated(QImage image);

public:
	ThumbnailItem(std::string uuid, OBSSource source);
	~ThumbnailItem();

	inline bool isNull() const { return !weakSource || obs_weak_source_expired(weakSource); }
	inline const std::string &getUuid() const { return uuid; }

signals:
	void updateThumbnail(QPixmap pixmap);
};

class Thumbnail : public QObject {
	Q_OBJECT

	friend class ThumbnailManager;

	std::shared_ptr<ThumbnailItem> item;

private slots:
	void thumbnailUpdated(QPixmap pixmap);

public:
	inline Thumbnail(std::shared_ptr<ThumbnailItem> item) : item(item) {}

	inline QPixmap getPixmap() const { return item->pixmap; }

	static constexpr int cx = 320;
	static constexpr int cy = 180;

signals:
	void updateThumbnail(QPixmap pixmap);
};

class ThumbnailManager : public QObject {
	Q_OBJECT

	friend class ThumbnailItem;

	struct CachedItem {
		std::optional<QPixmap> pixmap;
		std::weak_ptr<ThumbnailItem> weakActiveItem;
	};

	std::deque<std::weak_ptr<ThumbnailItem>> newThumbnails;
	std::deque<std::weak_ptr<ThumbnailItem>> thumbnails;
	std::unordered_map<std::string, CachedItem> cachedThumbnails;
	QTimer updateTimer;

	bool updatePixmap(std::shared_ptr<ThumbnailItem> &item);
	void updateTick();

	void updateIntervalChanged(size_t newCount);

public:
	explicit ThumbnailManager(QObject *parent = nullptr);
	~ThumbnailManager();

	std::shared_ptr<Thumbnail> getThumbnail(OBSSource source);
	std::optional<QPixmap> getCachedThumbnail(OBSSource source);
	void preloadThumbnail(OBSSource source, QObject *object, std::function<void(QPixmap)> callback);

private:
	Q_DISABLE_COPY_MOVE(ThumbnailManager);
};
