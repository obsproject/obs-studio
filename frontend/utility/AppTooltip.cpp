/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "AppTooltip.hpp"

#include <Idian/Tooltip.hpp>

#include <QApplication>
#include <QTimer>

QPointer<QWidget> AppTooltip::targetWidget = nullptr;
QPointer<idian::Tooltip> AppTooltip::tooltipWidget = nullptr;
QPointer<QTimer> AppTooltip::tooltipTimer = nullptr;

void AppTooltip::ensureTooltipWidget(QWidget *target)
{
	if (!targetWidget) {
		targetWidget = target;
	}

	if (!tooltipWidget) {
		tooltipWidget = new idian::Tooltip();
		QObject::connect(qApp, &QApplication::aboutToQuit, tooltipWidget, &QWidget::deleteLater);
	}
}

idian::Tooltip *AppTooltip::getTooltipWidget(QWidget *target)
{
	ensureTooltipWidget(target);
	return tooltipWidget;
}

void AppTooltip::createChildObservers(const QObjectList list)
{
	for (QObject *object : list) {
		if (!object->isWidgetType()) {
			continue;
		}

		QWidget *widget = static_cast<QWidget *>(object);

		// AppTooltipObservers are only created for widgets that have a tooltip when creating them recursively
		// to avoid creating a bunch of unnecessary observers.
		if (!widget->toolTip().isNull()) {
			new AppTooltipObserver(widget);
		}

		if (!widget->children().isEmpty()) {
			createChildObservers(widget->children());
		}
	}
}

void AppTooltip::applyAppTooltip(QWidget *widget)
{
	new AppTooltipObserver(widget);
}

void AppTooltip::applyAppTooltipToChildren(QWidget *widget)
{
	if (!widget->children().isEmpty()) {
		createChildObservers(widget->children());
	}
}

void AppTooltip::hideTooltip()
{
	tooltipWidget->hide();
}

void AppTooltip::showTooltip(QWidget *target, QPoint point, QString text, int millisecondDisplayTime)
{
	// Hide the tooltip container when swapping targets to avoid flicker
	if (tooltipWidget && (!targetWidget || target != targetWidget)) {
		tooltipWidget->setWindowOpacity(0);
	}

	ensureTooltipWidget(target);

	targetWidget = target;
	tooltipWidget->show();
	tooltipWidget->setVisible(true);
	text.isEmpty() ? tooltipWidget->setText(target->toolTip()) : tooltipWidget->setText(text);
	point.isNull() ? tooltipWidget->setAnchorWidget(target, idian::Anchor::TopCenter)
		       : tooltipWidget->setAnchorFixed(point);

	idian::Tooltip *safeWidget = tooltipWidget;
	QTimer::singleShot(0, tooltipWidget, [safeWidget]() {
		if (safeWidget) {
			safeWidget->setWindowOpacity(1);
		}
	});

	if (tooltipTimer) {
		tooltipTimer->stop();
	}

	if (millisecondDisplayTime > 0) {
		// Delete the timer to automatically disconnect the lambda
		delete tooltipTimer;
		tooltipTimer = new QTimer(tooltipWidget);
		tooltipTimer->setSingleShot(true);

		if (tooltipTimer->parent() != tooltipWidget) {
			tooltipTimer->deleteLater();
		}

		QPointer<QWidget> safeTarget = target;
		QObject::connect(tooltipTimer, &QTimer::timeout, tooltipTimer, [safeTarget]() {
			if (!safeTarget) {
				hideTooltip();
				return;
			}

			if (safeTarget->underMouse()) {
				tooltipTimer->start(100);
			} else {
				hideTooltip();
				return;
			}
		});

		tooltipTimer->start(millisecondDisplayTime);
	}
}
