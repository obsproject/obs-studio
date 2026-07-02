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

#include <Idian/AbstractPopup.hpp>

#include <QBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QSpacerItem>
#include <QStyleOption>

namespace idian {

class PopupContents : public QFrame {
	Q_OBJECT

	QPointer<QBoxLayout> contentsLayout{};
	QPointer<QWidget> contentsWidget{};

	Qt::Orientation orientation = Qt::Horizontal;

	QSpacerItem *arrowSpacer{};
	QPoint arrowPosition;
	int arrowSize = 8;
	bool showArrow = true;
	Anchor::Position arrowLocation{};

protected:
	void paintEvent(QPaintEvent *) override
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);

		QStyleOptionFrame frameOpt;
		frameOpt.initFrom(this);
		frameOpt.rect = frameRect();

		frameOpt.frameShape = QFrame::NoFrame;
		frameOpt.lineWidth = 0;
		frameOpt.midLineWidth = 0;

		if (!showArrow) {
			style()->drawPrimitive(QStyle::PE_Widget, &frameOpt, &painter, this);
			return;
		}

		QPoint arrowTip = arrowPosition;
		QPoint arrowPoint1;
		QPoint arrowPoint2;

		if (orientation == Qt::Horizontal) {
			if (arrowLocation & Anchor::Top) {
				arrowTip.ry() += arrowSize * 2;
			} else if (arrowLocation & Anchor::Bottom) {
				arrowTip.ry() -= arrowSize * 2;
			}

			if (arrowLocation & Anchor::Right) {
				// Right-facing arrow
				frameOpt.rect.adjust(0, 0, -arrowSize, 0);
				arrowPoint1 = QPoint(arrowTip.x() - arrowSize, arrowTip.y() - arrowSize);
				arrowPoint2 = QPoint(arrowTip.x() - arrowSize, arrowTip.y() + arrowSize);
			} else if (arrowLocation & Anchor::Left) {
				// Left-facing arrow
				frameOpt.rect.adjust(arrowSize, 0, 0, 0);
				arrowPoint1 = QPoint(arrowTip.x() + arrowSize, arrowTip.y() - arrowSize);
				arrowPoint2 = QPoint(arrowTip.x() + arrowSize, arrowTip.y() + arrowSize);
			}
		} else {
			if (arrowLocation & Anchor::Left) {
				arrowTip.rx() += arrowSize * 2;
			} else if (arrowLocation & Anchor::Right) {
				arrowTip.rx() -= arrowSize * 2;
			}

			if (arrowLocation & Anchor::Top) {
				// Up-facing arrow
				frameOpt.rect.adjust(0, arrowSize, 0, 0);
				arrowPoint1 = QPoint(arrowTip.x() - arrowSize, arrowTip.y() + arrowSize);
				arrowPoint2 = QPoint(arrowTip.x() + arrowSize, arrowTip.y() + arrowSize);
			} else if (arrowLocation & Anchor::Bottom) {
				// Down-facing arrow
				frameOpt.rect.adjust(0, 0, 0, -arrowSize);
				arrowPoint1 = QPoint(arrowTip.x() - arrowSize, arrowTip.y() - arrowSize);
				arrowPoint2 = QPoint(arrowTip.x() + arrowSize, arrowTip.y() - arrowSize);
			}
		}

		style()->drawPrimitive(QStyle::PE_Widget, &frameOpt, &painter, this);

		QPainterPath path;
		path.moveTo(arrowTip);
		path.lineTo(arrowPoint1);
		path.lineTo(arrowPoint2);
		path.lineTo(arrowTip);

		QBrush background = frameOpt.palette.brush(QPalette::Window);
		painter.fillPath(path, background);
	}

public:
	PopupContents(QWidget *parent) : QFrame(parent)
	{
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAutoFillBackground(false);
		setAttribute(Qt::WA_OpaquePaintEvent, true);

		contentsLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
		contentsLayout->setContentsMargins(0, 0, 0, 0);
		contentsLayout->setSpacing(0);

		setLayout(contentsLayout);
		setWidget(new QWidget(this));
		updateArrowSpacer();
	}
	~PopupContents() {}

	void updateArrowSpacer()
	{
		contentsLayout->removeItem(arrowSpacer);

		delete arrowSpacer;
		arrowSpacer = nullptr;

		if (!showArrow) {
			return;
		}

		arrowSpacer = new QSpacerItem(arrowSize, arrowSize, QSizePolicy::Fixed, QSizePolicy::Fixed);

		if (orientation == Qt::Horizontal) {
			if (arrowLocation & Anchor::Left) {
				contentsLayout->insertItem(0, arrowSpacer);
			} else if (arrowLocation & Anchor::Right) {
				contentsLayout->insertItem(1, arrowSpacer);
			}
		} else {
			if (arrowLocation & Anchor::Top) {
				contentsLayout->insertItem(0, arrowSpacer);
			} else if (arrowLocation & Anchor::Bottom) {
				contentsLayout->insertItem(1, arrowSpacer);
			}
		}
	}

	void setOrientation(Qt::Orientation orientation)
	{
		if (this->orientation == orientation) {
			return;
		}

		this->orientation = orientation;

		if (this->orientation == Qt::Horizontal) {
			contentsLayout->setDirection(QBoxLayout::LeftToRight);
		} else {
			contentsLayout->setDirection(QBoxLayout::TopToBottom);
		}

		updateArrowSpacer();
	}

	void setWidget(QWidget *widget)
	{
		if (contentsWidget == widget) {
			return;
		}

		if (contentsWidget) {
			contentsLayout->removeWidget(contentsWidget);
			contentsWidget->deleteLater();
		}

		this->contentsWidget = widget;
		contentsLayout->addWidget(contentsWidget);

		updateArrowSpacer();
		adjustSize();
	}

	void setArrowPosition(QPoint position) { arrowPosition = position; }

	void setArrowLocation(Anchor::Position arrowLocation)
	{
		if (this->arrowLocation == arrowLocation) {
			return;
		}

		this->arrowLocation = arrowLocation;

		updateArrowSpacer();
	}

	void setShowArrow(bool enable)
	{
		if (showArrow == enable) {
			return;
		}

		showArrow = enable;
		updateArrowSpacer();
	}

	bool isArrowEnabled() { return showArrow; }
};

} // namespace idian
