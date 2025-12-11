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

#include "AppTooltipObserver.hpp"

#include <OBSApp.hpp>
#include <utility/AppTooltip.hpp>

#include <QSlider>
#include <QTimer>

AppTooltipObserver::AppTooltipObserver(QWidget *target) : targetWidget(target), QObject(target)
{
	if (!target) {
		deleteLater();
		return;
	}

	if (target->isWindow()) {
		deleteLater();
		return;
	}

	targetWidget->installEventFilter(this);

	setText(targetWidget->toolTip());
}

AppTooltipObserver::~AppTooltipObserver() {}

void AppTooltipObserver::setText(QString text)
{
	tooltipText = text;
}

void AppTooltipObserver::showTooltip()
{
	if (!targetWidget) {
		deleteLater();
		return;
	}

	if (!targetWidget->window()->isActiveWindow()) {
		return;
	}

	if (targetWidget->toolTip().isEmpty()) {
		return;
	}

	AppTooltip::showTooltip(targetWidget);
}

void AppTooltipObserver::hideTooltip()
{
	if (!targetWidget) {
		deleteLater();
		return;
	}

	auto tooltipWidget = AppTooltip::getTooltipWidget(targetWidget);
	if (tooltipWidget && tooltipWidget->isVisible()) {
		AppTooltip::hideTooltip();
	}
}

bool AppTooltipObserver::eventFilter(QObject *obj, QEvent *event)
{
	if (!obj->isWidgetType()) {
		return false;
	}

	QWidget *widget = static_cast<QWidget *>(obj);

	// Discard the default tooltip event
	if (event->type() == QEvent::ToolTip) {
		return true;
	}

	if (event->type() == QEvent::ToolTipChange) {
		setText(targetWidget->toolTip());

		if (widget->underMouse() || widget->hasFocus()) {
			tooltipText.isEmpty() ? hideTooltip() : showTooltip();

			return true;
		}
	}

	if (event->type() == QEvent::Enter) {
		if (widget->isEnabled() && widget->underMouse()) {
			showTooltip();
			return false;
		}
	} else if (event->type() == QEvent::Leave) {
		hideTooltip();
		return false;
	}

	if (event->type() == QEvent::EnabledChange) {
		if (widget->underMouse() || widget->hasFocus()) {
			widget->isEnabled() ? showTooltip() : hideTooltip();
		}

		return false;
	}

	return false;
}
