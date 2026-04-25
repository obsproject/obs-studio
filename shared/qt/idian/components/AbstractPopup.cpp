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

#include <Idian/AbstractPopup.hpp>
#include <Idian/PopupContents.hpp>

#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QMainWindow>
#include <QScreen>
#include <QTimer>
#include <QWindow>

namespace {
QPoint getAnchorCoordinate(QWidget *widget, idian::Anchor::Position anchor)
{
	QPoint newPoint{0, 0};

	if (anchor == idian::Anchor::TopLeft) {
		newPoint = widget->geometry().topLeft();
	} else if (anchor == idian::Anchor::TopCenter) {
		newPoint = QPoint(widget->geometry().center().x(), widget->geometry().top());
	} else if (anchor == idian::Anchor::TopRight) {
		newPoint = widget->geometry().topRight();
	} else if (anchor == idian::Anchor::LeftCenter) {
		newPoint = QPoint(widget->geometry().left(), widget->geometry().center().y());
	} else if (anchor == idian::Anchor::RightCenter) {
		newPoint = QPoint(widget->geometry().right(), widget->geometry().center().y());
	} else if (anchor == idian::Anchor::BottomLeft) {
		newPoint = widget->geometry().bottomLeft();
	} else if (anchor == idian::Anchor::BottomCenter) {
		newPoint = QPoint(widget->geometry().center().x(), widget->geometry().bottom());
	} else if (anchor == idian::Anchor::BottomRight) {
		newPoint = widget->geometry().bottomRight();
	}

	return newPoint;
}

QPoint getArrowPosition(QWidget *widget, idian::Anchor::Position offsetLocation)
{
	QRect rect = widget->rect();

	QPoint arrowPosition{};
	switch (offsetLocation) {
	case idian::Anchor::TopLeft:
		arrowPosition = rect.topLeft();
		break;
	case idian::Anchor::TopCenter:
		arrowPosition = QPoint(rect.center().x(), rect.top());
		break;
	case idian::Anchor::TopRight:
		arrowPosition = rect.topRight();
		break;
	case idian::Anchor::LeftCenter:
		arrowPosition = QPoint(rect.left(), rect.center().y());
		break;
	case idian::Anchor::Center:
		arrowPosition = rect.center();
		break;
	case idian::Anchor::RightCenter:
		arrowPosition = QPoint(rect.right(), rect.center().y());
		break;
	case idian::Anchor::BottomLeft:
		arrowPosition = rect.bottomLeft();
		break;
	case idian::Anchor::BottomCenter:
		arrowPosition = QPoint(rect.center().x(), rect.bottom());
		break;
	case idian::Anchor::BottomRight:
		arrowPosition = rect.bottomRight();
		break;
	}

	return arrowPosition;
}
} // namespace

namespace idian {

AbstractPopup::AbstractPopup(QWidget *parent) : QWidget(parent)
{
	setWindowFlag(Qt::Tool, true);
	setWindowFlag(Qt::FramelessWindowHint, true);
	setAttribute(Qt::WA_TranslucentBackground, true);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
	setLayout(mainLayout);

	innerFrame = new PopupContents(this);
	innerFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

	mainLayout->addWidget(innerFrame);
}

AbstractPopup::~AbstractPopup()
{
	if (anchorWidget) {
		anchorWidget->removeEventFilter(this);
	}

	clearAncestorFilters();
}

void AbstractPopup::clearAncestorFilters()
{
	for (auto widget : ancestorWidgets) {
		if (!widget) {
			continue;
		}

		widget->removeEventFilter(this);

		if (widget->isWindow() && widget->windowHandle()) {
			disconnect(widget->windowHandle(), &QWindow::screenChanged, this,
				   &AbstractPopup::updateWidgetAnchorPosition);
		}
	}
	ancestorWidgets.clear();
}

QPoint AbstractPopup::calculateOffset(Anchor::Position offsetLocation)
{
	int offsetX = 0;
	int offsetY = 0;

	if (offsetLocation & Anchor::HCenter) {
		offsetX = -width() / 2;
	} else if (offsetLocation & Anchor::Right) {
		offsetX = -width();
	}

	if (offsetLocation & Anchor::VCenter) {
		offsetY = -height() / 2;
	} else if (offsetLocation & Anchor::Bottom) {
		offsetY = -height();
	}

	return QPoint(offsetX, offsetY);
}

void AbstractPopup::queuePositionUpdate()
{
	if (!positionTimer) {
		positionTimer = new QTimer(this);
		positionTimer->setSingleShot(true);

		connect(positionTimer, &QTimer::timeout, this, &AbstractPopup::updatePosition);
	} else {
		positionTimer->stop();
	}

	positionTimer->start(0);
}

void AbstractPopup::updatePosition()
{
	if (!isVisible()) {
		return;
	}

	if (isUpdating) {
		return;
	}

	isUpdating = true;

	if (anchorPoint.isNull()) {
		isUpdating = false;
		return;
	}

	updateGeometry();
	adjustSize();

	QPoint offset = calculateOffset(offsetLocation);
	QPoint popupPos = anchorPoint + offset;

	int titleHeight = 0;
	if (anchorWidget && anchorWidget->isWindow()) {
		titleHeight = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);

		if (offsetLocation & Anchor::Top) {
			popupPos.ry() += titleHeight;
		} else {
			popupPos.ry() -= titleHeight;
		}
	}

	// Constrain popup to screen bounds
	QScreen *screen = QGuiApplication::screenAt(anchorPoint);
	QPoint arrowShift{0, 0};
	auto arrowLocation = offsetLocation;
	if (screen && screen->availableGeometry().width() > width() &&
	    screen->availableGeometry().height() > height()) {
		int constrainedX = std::clamp(popupPos.x(), screen->availableGeometry().left(),
					      screen->availableGeometry().right() - width());
		int constrainedY = std::clamp(popupPos.y(), screen->availableGeometry().top(),
					      screen->availableGeometry().bottom() - height());

		arrowShift = {popupPos.x() - constrainedX, popupPos.y() - constrainedY};

		popupPos.rx() = constrainedX;
		popupPos.ry() = constrainedY;

		if (anchorWidget && anchorLocation & Anchor::Top && offsetLocation & Anchor::Bottom) {
			auto anchorTop = anchorWidget->parentWidget()->mapToGlobal(anchorWidget->geometry().topLeft());

			if (popupPos.y() + height() > anchorTop.y()) {
				// Popup positioned above widget but is overlapping it
				popupPos.ry() += (anchorTop.y() - popupPos.y()) + anchorWidget->height();
				Anchor::flipVertical(arrowLocation);
				arrowShift = {0, 0};
			}
		} else if (anchorWidget && anchorLocation & Anchor::Bottom && offsetLocation & Anchor::Top) {
			auto anchorBottom =
				anchorWidget->parentWidget()->mapToGlobal(anchorWidget->geometry().bottomLeft());

			if (popupPos.y() < anchorBottom.y()) {
				// Popup positioned below widget but is overlapping it
				popupPos.ry() += (anchorBottom.y() - popupPos.y()) - height() - anchorWidget->height();
				Anchor::flipVertical(arrowLocation);
				arrowShift = {0, 0};
			}
		}
	}

	if (innerFrame->isArrowEnabled()) {
		innerFrame->setArrowLocation(arrowLocation);

		QPoint arrowPosition = getArrowPosition(this, arrowLocation);
		arrowPosition += arrowShift;
		innerFrame->setArrowPosition(arrowPosition);
	}

	move(popupPos);
	update();
	raise();

	isUpdating = false;
}

void AbstractPopup::updateVisibility()
{
	if (!anchorWidget) {
		if (isVisible()) {
			hide();
		}
		return;
	}
}

bool AbstractPopup::eventFilter(QObject *obj, QEvent *event)
{
	if (!anchorWidget) {
		return false;
	}

	// Events to handle for target widget and any ancestor
	if (event->type() == QEvent::Resize) {
		updateWidgetAnchorPosition();
	} else if (event->type() == QEvent::Move) {
		updateWidgetAnchorPosition();
	}

	// Events to handle for only the target widget
	if (obj == anchorWidget) {
		if (event->type() == QEvent::Show) {
			updateVisibility();
			updateWidgetAnchorPosition();
		} else if (event->type() == QEvent::Hide) {
			updateVisibility();
		} else if (event->type() == QEvent::Destroy) {
			deleteLater();
		}
	}

	return false;
}

void AbstractPopup::setAnchorPoint(QPoint point)
{
	if (this->anchorPoint == point) {
		return;
	}

	if (point.isNull()) {
		return;
	}

	this->anchorPoint = point;
}

void AbstractPopup::setAnchorFrom(Anchor::Position location)
{
	if (offsetLocation == location) {
		return;
	}

	offsetLocation = location;

	innerFrame->setArrowLocation(offsetLocation);
	queuePositionUpdate();
}

void AbstractPopup::setAnchorWidget(QWidget *anchor)
{
	if (anchor == anchorWidget) {
		updateWidgetAnchorPosition();
		return;
	}

	setAnchorWidget(anchor, anchorLocation);
}

void AbstractPopup::setAnchorWidget(QWidget *anchor, Anchor::Position location)
{
	if (!anchor) {
		return;
	}

	if (anchorWidget) {
		anchorWidget->removeEventFilter(this);
	}

	fixedAnchor.reset();

	anchorWidget = anchor;
	anchorLocation = location;

	anchorWidget->installEventFilter(this);

	clearAncestorFilters();
	// Set up event filters on all ancestor widgets
	QWidget *ancestor = anchor->parentWidget();
	while (ancestor) {
		ancestorWidgets.append(ancestor);
		ancestor->installEventFilter(this);

		if (ancestor->isWindow()) {
			// Handle screen change for window
			if (ancestor->windowHandle()) {
				connect(ancestor->windowHandle(), &QWindow::screenChanged, this,
					[this]() { updateWidgetAnchorPosition(); });
			}

			// Works around an issue where switching between tabbed docks puts the central widget above this widget
			QMainWindow *mainWindow = qobject_cast<QMainWindow *>(ancestor);
			if (mainWindow) {
				connect(mainWindow, &QMainWindow::tabifiedDockWidgetActivated, this,
					&AbstractPopup::updateWidgetAnchorPosition);
			}
		}

		ancestor = ancestor->parentWidget();
	}

	updateWidgetAnchorPosition();
}

void AbstractPopup::setAnchorFixed(QPoint point)
{
	if (fixedAnchor == point) {
		return;
	}

	if (anchorWidget) {
		anchorWidget->removeEventFilter(this);
		anchorWidget.clear();
	}

	clearAncestorFilters();

	fixedAnchor = point;
	setAnchorPoint(point);

	queuePositionUpdate();
}

void AbstractPopup::updateWidgetAnchorPosition()
{
	if (!anchorWidget) {
		return;
	}

	if (fixedAnchor.has_value()) {
		return;
	}

	if (anchorWidget->isWindow()) {
		setAnchorPoint(getAnchorPoint());
	} else if (anchorWidget->parentWidget()) {
		setAnchorPoint(anchorWidget->parentWidget()->mapToGlobal(getAnchorPoint()));
	}

	updatePosition();
}

QPoint AbstractPopup::getAnchorPoint()
{
	return getAnchorCoordinate(anchorWidget, anchorLocation);
}

void AbstractPopup::setWidget(QWidget *widget)
{
	innerFrame->setWidget(widget);
	updateGeometry();
	adjustSize();
}

void AbstractPopup::setOrientation(Qt::Orientation orientation)
{
	innerFrame->setOrientation(orientation);
	queuePositionUpdate();
}

void AbstractPopup::show()
{
	QWidget::show();
	queuePositionUpdate();
}

} // namespace idian
