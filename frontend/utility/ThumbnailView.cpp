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

#include "ThumbnailView.hpp"

ThumbnailView::ThumbnailView(QObject *parent, QPointer<ThumbnailItem> item) : QObject(parent)
{
	if (!parent) {
		deleteLater();
		return;
	}

	if (!item) {
		return;
	}

	uuid = item->getUuid();
	setPixmap(item->getPixmap());

	connect(item, &ThumbnailItem::pixmapUpdated, this, &ThumbnailView::setPixmap);

	if (!item->isValid()) {
		setEnabled(false);
	}
}

ThumbnailView::~ThumbnailView() {}

void ThumbnailView::setEnabled(bool enabled)
{
	if (this->enabled != enabled) {
		this->enabled = enabled;

		emit enabledChanged(enabled);
	}
}

void ThumbnailView::requestUpdate()
{
	if (!this->enabled) {
		return;
	}

	emit updateRequested(uuid);
}

void ThumbnailView::setPixmap(QPixmap pixmap)
{
	this->pixmap = pixmap;

	emit updated(this->pixmap);
}
