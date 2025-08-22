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

#pragma once

#include <obs.hpp>

#include <QObject>
#include <QPixmap>
#include <QTimer>

using namespace std::chrono;

struct ThumbnailItem {
	std::string uuid;
	std::optional<steady_clock::time_point> lastUpdate;
	QPixmap pixmap;

	bool isNull() { return uuid.empty(); }
};

class ThumbnailManager : public QObject {
	Q_OBJECT

public:
	ThumbnailManager();
	~ThumbnailManager();

	QPixmap getThumbnail(OBSSource source);

private:
	std::unordered_map<std::string, ThumbnailItem> thumbnails;

	QTimer *updateTimer;

	std::vector<OBSSignal> sigs;

	static void obsSourceAdded(void *param, calldata_t *calldata);
	static void obsSourceRemoved(void *param, calldata_t *calldata);

	int updateThrottle = 0;
	void updateTick();

	gs_texrender_t *texrender;

public slots:
	void sourceAdded(OBSSource source);
	void sourceRemoved(OBSSource source);
};
