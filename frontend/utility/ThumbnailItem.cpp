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

#include "ThumbnailItem.hpp"

#include <utility/ScreenshotObj.hpp>
#include <utility/ThumbnailView.hpp>
#include <widgets/OBSBasic.hpp>

#include <QIcon>
#include <QPainter>
#include <QPixmap>

static constexpr int kDefaultWidth = 320;
static constexpr int kDefaultHeight = 180;

namespace {
QPixmap getDefaultThumbnail(obs_source_t *source)
{
	const char *id = obs_source_get_id(source);
	OBSBasic *main = OBSBasic::Get();
	if (main && id) {
		QIcon icon = OBSBasic::Get()->GetSourceIcon(id);
		QPixmap iconPixmap = icon.pixmap(90, 90);

		QPixmap defaultPixmap(kDefaultWidth, kDefaultHeight);
		defaultPixmap.fill(Qt::transparent);

		QPainter painter(&defaultPixmap);

		const int x = (defaultPixmap.width() - iconPixmap.width()) / 2;
		const int y = (defaultPixmap.height() - iconPixmap.height()) / 2;

		painter.drawPixmap(x, y, iconPixmap);

		return defaultPixmap;
	}

	return QPixmap();
}
} // namespace

ThumbnailItem::ThumbnailItem(const std::string &uuid, ThumbnailManager *manager) : QObject(manager), uuid(uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
	if (!source) {
		return;
	}

	weakSource = OBSGetWeakRef(source);

	std::optional<QPixmap> cachedPixmap = manager->getCachedPixmap(uuid);
	if (cachedPixmap.has_value()) {
		setPixmap(cachedPixmap.value());
		isDefaultPixmap_ = false;
	} else {
		setPixmap(getDefaultThumbnail(source));
	}

	if ((obs_source_get_output_flags(source) & OBS_SOURCE_VIDEO) != 0) {
		isVideoSource = true;
	}
}

void ThumbnailItem::updatePixmapFromImage(const QImage &image)
{
	if (!image.isNull()) {
		setPixmap(QPixmap::fromImage(image));
		isDefaultPixmap_ = false;
	}
}

bool ThumbnailItem::shouldUpdate() const
{
	return isValid() && (viewCount > 0) && (enabledCount > 0);
}

QPixmap ThumbnailItem::getPixmap() const
{
	return pixmap;
}

void ThumbnailItem::setPixmap(QPixmap pixmap)
{
	this->pixmap = std::move(pixmap);

	emit pixmapUpdated(this->pixmap);
}

void ThumbnailItem::incrementViewCount()
{
	++viewCount;
}

ThumbnailView *ThumbnailItem::createView(QObject *parent)
{
	auto *view = new ThumbnailView(parent, this);

	incrementViewCount();
	if (view->isEnabled()) {
		incrementEnabledCount();
	}

	// Connect ThumbnailView signals
	connect(view, &QObject::destroyed, this, [this, view]() {
		decrementViewCount();
		if (view->isEnabled()) {
			decrementEnabledCount();
		}
	});
	connect(view, &ThumbnailView::enabledChanged, this, [this](bool enabled) {
		if (enabled) {
			incrementEnabledCount();
		} else {
			decrementEnabledCount();
		}
	});

	return view;
}

void ThumbnailItem::decrementViewCount()
{
	if (viewCount > 0) {
		--viewCount;
	}

	if (viewCount <= 0) {
		emit noViewsRemaining();
	}
}

void ThumbnailItem::incrementEnabledCount()
{
	++enabledCount;
}

void ThumbnailItem::decrementEnabledCount()
{
	if (enabledCount > 0) {
		--enabledCount;
	}
}

bool ThumbnailItem::update()
{
	if (!isVideoSource) {
		return false;
	}

	if (!shouldUpdate()) {
		return false;
	}

	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return false;
	}

	QPixmap pixmap;
	if (this->pixmap.isNull()) {
		this->pixmap = pixmap;
	}

	if (source) {
		uint32_t sourceWidth = obs_source_get_width(source);
		uint32_t sourceHeight = obs_source_get_height(source);

		if (sourceWidth == 0 || sourceHeight == 0) {
			return false;
		}

		auto *obj = new ScreenshotObj(source);
		obj->setSaveToFile(false);
		obj->setSize(kDefaultWidth, kDefaultHeight);

		connect(obj, &ScreenshotObj::imageReady, this, &ThumbnailItem::updatePixmapFromImage);
	}

	return true;
}
