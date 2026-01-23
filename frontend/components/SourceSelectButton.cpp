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

#include "SourceSelectButton.hpp"

#include <utility/ThumbnailManager.hpp>
#include <widgets/OBSBasic.hpp>

#include <QDrag>
#include <QFrame>
#include <QMimeData>
#include <QPainter>
#include <QStyleOptionButton>

SourceSelectButton::SourceSelectButton(OBSWeakSource weak, QWidget *parent) : QFrame(parent), weakSource(weak)
{
	OBSSource source{OBSGetStrongRef(weak)};

	if (!source || !weakSource) {
		return;
	}

	const char *sourceName = obs_source_get_name(source);

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	button_ = new QPushButton(this);
	button_->setCheckable(true);
	button_->setAttribute(Qt::WA_Moved);
	button_->setAccessibleName(sourceName);
	button_->show();

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);

	label = new QLabel(sourceName);
	label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	label->setAttribute(Qt::WA_TransparentForMouseEvents);
	label->setObjectName("name");

	image = new QLabel(this);
	image->setObjectName("thumbnail");
	image->setAttribute(Qt::WA_TransparentForMouseEvents);
	image->setMinimumSize(160, 90);
	image->setMaximumSize(160, 90);
	image->setAlignment(Qt::AlignCenter);
	std::optional<QPixmap> cachedThumbnail = OBSBasic::Get()->thumbnails()->getCachedThumbnail(source);

	if (cachedThumbnail.has_value()) {
		updatePixmap(*cachedThumbnail);
	} else {
		setDefaultThumbnail();
	}

	layout->addWidget(image);
	layout->addWidget(label);

	button_->setFixedSize(width(), height());
	button_->move(0, 0);

	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(button());

	signalHandlers.reserve(1);
	signalHandlers.emplace_back(obs_source_get_signal_handler(source), "destroy",
				    &SourceSelectButton::obsSourceRemoved, this);

	connect(button(), &QAbstractButton::pressed, this, &SourceSelectButton::buttonPressed);
}

SourceSelectButton::~SourceSelectButton() {}

QPushButton *SourceSelectButton::button()
{
	return button_;
}

QString SourceSelectButton::text()
{
	return label->text();
}

void SourceSelectButton::resizeEvent(QResizeEvent *)
{
	button()->setFixedSize(width(), height());
	button()->move(0, 0);
}

void SourceSelectButton::moveEvent(QMoveEvent *)
{
	button()->setFixedSize(width(), height());
	button()->move(0, 0);
}

void SourceSelectButton::buttonPressed()
{
	dragStartPosition = QCursor::pos();
}

void SourceSelectButton::obsSourceRemoved(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<SourceSelectButton *>(data), &SourceSelectButton::handleSourceRemoved);
}

void SourceSelectButton::setDefaultThumbnail()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		const char *id = obs_source_get_id(source);
		QIcon icon = OBSBasic::Get()->GetSourceIcon(id);
		image->setPixmap(icon.pixmap(45, 45));
	}
}

void SourceSelectButton::handleSourceRemoved()
{
	emit sourceRemoved();
}

void SourceSelectButton::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}

	if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
		return;
	}

	QMimeData *mimeData = new QMimeData;
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		std::string uuid = obs_source_get_uuid(source);
		mimeData->setData("application/x-obs-source-uuid", uuid.c_str());

		QDrag *drag = new QDrag(this);
		drag->setMimeData(mimeData);
		drag->setPixmap(this->grab());
		drag->exec(Qt::CopyAction);
	}
}

void SourceSelectButton::setRectVisible(bool visible)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	if (rectVisible != visible) {
		rectVisible = visible;

		if (visible) {
			uint32_t flags = obs_source_get_output_flags(source);
			bool hasVideo = (flags & OBS_SOURCE_VIDEO) == OBS_SOURCE_VIDEO;
			if (hasVideo) {
				thumbnail = OBSBasic::Get()->thumbnails()->getThumbnail(source);
				connect(thumbnail.get(), &Thumbnail::updateThumbnail, this,
					&SourceSelectButton::updatePixmap);
				updatePixmap(thumbnail->getPixmap());
			}
		} else {
			thumbnail.reset();
		}
	}

	if (preload && !rectVisible) {
		OBSBasic::Get()->thumbnails()->preloadThumbnail(source, this,
								[=](QPixmap pixmap) { updatePixmap(pixmap); });
	}
	preload = false;
}

void SourceSelectButton::setPreload(bool preload)
{
	this->preload = preload;
}

void SourceSelectButton::updatePixmap(QPixmap pixmap)
{
	if (!pixmap.isNull()) {
		image->setPixmap(pixmap.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}
