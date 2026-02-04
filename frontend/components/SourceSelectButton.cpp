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
#include <utility/ThumbnailView.hpp>
#include <widgets/OBSBasic.hpp>

#include <QDrag>
#include <QFrame>
#include <QMimeData>
#include <QPainter>
#include <QStyleOptionButton>

SourceSelectButton::SourceSelectButton(OBSWeakSource weak, QWidget *parent) : QAbstractButton(parent), weakSource(weak)
{
	OBSSource source{OBSGetStrongRef(weak)};

	if (!source || !weakSource) {
		return;
	}

	const char *sourceName = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);

	if (!sourceName || !uuid) {
		return;
	}

	sourceUuid = uuid;

	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	setCheckable(true);
	setAccessibleName(sourceName);

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

	thumbnail = OBSBasic::Get()->thumbnails()->createView(this, source);
	connect(thumbnail, &ThumbnailView::updated, this, &SourceSelectButton::updatePixmap);
	updatePixmap(thumbnail->getPixmap());

	layout->addWidget(image);
	layout->addWidget(label);

	setFocusPolicy(Qt::StrongFocus);

	signalHandlers.reserve(2);
	signalHandlers.emplace_back(obs_source_get_signal_handler(source), "destroy",
				    &SourceSelectButton::obsSourceRemoved, this);
	signalHandlers.emplace_back(obs_source_get_signal_handler(source), "rename",
				    &SourceSelectButton::obsSourceRenamed, this);

	connect(this, &QAbstractButton::pressed, this, &SourceSelectButton::buttonPressed);
}

SourceSelectButton::~SourceSelectButton() {}

void SourceSelectButton::paintEvent(QPaintEvent *)
{
	QPainter painter{this};

	QStyleOptionButton option;
	option.initFrom(this);

	if (isChecked()) {
		option.state |= QStyle::State_On;
	}
	if (isDown()) {
		option.state |= QStyle::State_Sunken;
	}
	if (hasFocus()) {
		option.state |= QStyle::State_HasFocus;
	}
	if (underMouse()) {
		option.state |= QStyle::State_MouseOver;
	}

	style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);
}

void SourceSelectButton::resizeEvent(QResizeEvent *)
{
	QStyleOptionButton option;
	option.initFrom(this);

	QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);
	if (!contentRect.isValid()) {
		contentRect = rect();
	}

	int left = contentRect.left();
	int top = contentRect.top();
	int right = rect().width() - contentRect.right() - 1;
	int bottom = rect().height() - contentRect.bottom() - 1;

	left = std::max(0, left);
	top = std::max(0, top);
	right = std::max(0, right);
	bottom = std::max(0, bottom);

	if (auto layout = this->layout()) {
		layout->setContentsMargins(left, top, right, bottom);
	}
}

void SourceSelectButton::enterEvent(QEnterEvent *)
{
	update();
	thumbnail->requestUpdate();
}

void SourceSelectButton::leaveEvent(QEvent *)
{
	update();
}

void SourceSelectButton::buttonPressed()
{
	dragStartPosition = mapFromGlobal(QCursor::pos());
}

void SourceSelectButton::obsSourceRemoved(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<SourceSelectButton *>(data), &SourceSelectButton::handleSourceRemoved);
}

void SourceSelectButton::obsSourceRenamed(void *data, calldata_t *params)
{
	const char *newNamePtr = static_cast<const char *>(calldata_ptr(params, "new_name"));
	std::string newName = newNamePtr;

	QMetaObject::invokeMethod(static_cast<SourceSelectButton *>(data), "handleSourceRenamed", Qt::QueuedConnection,
				  Q_ARG(QString, QString::fromStdString(newName)));
}

void SourceSelectButton::handleSourceRemoved()
{
	emit sourceRemoved();
}

void SourceSelectButton::handleSourceRenamed(QString name)
{
	label->setText(name);
}

void SourceSelectButton::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}

	if ((event->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance() * 3) {
		return;
	}

	QMimeData *mimeData = new QMimeData;

	mimeData->setData("application/x-obs-source-uuid", sourceUuid.c_str());

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->setPixmap(this->grab());
	drag->exec(Qt::CopyAction);

	if (isDown()) {
		setDown(false);
	}
}

void SourceSelectButton::setThumbnailEnabled(bool enabled)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	if (thumbnailEnabled != enabled) {
		thumbnailEnabled = enabled;

		thumbnail->setEnabled(thumbnailEnabled);
	}
}

void SourceSelectButton::updateThumbnail()
{
	thumbnail->requestUpdate();
}

void SourceSelectButton::updatePixmap(QPixmap pixmap)
{
	if (!pixmap.isNull()) {
		image->setPixmap(pixmap.scaled(160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}
