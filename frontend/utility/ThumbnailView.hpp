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

#include <utility/ThumbnailItem.hpp>

#include <QObject>
#include <QPixmap>
#include <QPointer>

class ThumbnailView : public QObject {
	Q_OBJECT

	QPixmap pixmap{};
	bool enabled = true;

public slots:
	void setPixmap(QPixmap pixmap);

public:
	ThumbnailView(QObject *parent, QPointer<ThumbnailItem> item);
	~ThumbnailView();

	std::string uuid{};
	QPixmap getPixmap() const { return pixmap; }

	static constexpr int cx = 320;
	static constexpr int cy = 180;

	bool isEnabled() const { return enabled; }
	void setEnabled(bool enabled);

	void requestUpdate();

signals:
	void updated(QPixmap pixmap);
	void enabledChanged(bool enabled);
	void updateRequested(std::string &uuid);
};
