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

#include <Idian/StateEventFilter.hpp>
#include <Idian/Utils.hpp>
#include <QAbstractButton>
#include <QFocusEvent>
#include <QStyleOptionButton>
#include <QTimer>

namespace idian {
StateEventFilter::StateEventFilter(idian::Utils *utils, QWidget *target) : QObject(target), target(target), utils(utils)
{
	QAbstractButton *button = qobject_cast<QAbstractButton *>(target);
	if (button) {
		connect(button, &QAbstractButton::toggled, this, &StateEventFilter::updateCheckedState);
	}
}

bool StateEventFilter::eventFilter(QObject *obj, QEvent *event)
{
	if (!obj->isWidgetType()) {
		return QObject::eventFilter(obj, event);
	}

	QWidget *widget = qobject_cast<QWidget *>(obj);
	QFocusEvent *focusEvent = nullptr;

	bool updateIconColors = true;

	switch (event->type()) {
	case QEvent::StyleChange:
	case QEvent::ThemeChange:
		utils->repolish(widget);

		utils->polishChildren(widget);

		break;
	case QEvent::FocusIn:
		utils->toggleClass(widget, "focus", true);

		focusEvent = static_cast<QFocusEvent *>(event);
		if (focusEvent->reason() != Qt::MouseFocusReason && focusEvent->reason() != Qt::PopupFocusReason) {
			utils->toggleClass(widget, "keyFocus", true);
		} else {
			utils->toggleClass(widget, "keyFocus", false);
		}

		utils->polishChildren(widget);

		break;
	case QEvent::FocusOut:
		utils->toggleClass(widget, "focus", false);

		focusEvent = static_cast<QFocusEvent *>(event);
		if (focusEvent->reason() != Qt::PopupFocusReason) {
			utils->toggleClass(widget, "keyFocus", false);
			utils->polishChildren(widget);
		}

		utils->polishChildren(widget);

		break;
	case QEvent::HoverEnter:
		if (widget->isEnabled()) {
			utils->toggleClass(widget, "hover", true);
		}

		utils->polishChildren(widget);

		break;
	case QEvent::HoverLeave:
		utils->toggleClass(widget, "hover", false);

		utils->polishChildren(widget);

		break;
	case QEvent::EnabledChange:
		utils->toggleClass(widget, "disabled", !widget->isEnabled());

		utils->polishChildren(widget);

		break;
	default:
		updateIconColors = false;
		break;
	}

	if (updateIconColors) {
		// Delay icon update
		QTimer::singleShot(0, this, [this, widget]() { utils->applyColorToIcon(widget); });
	}

	return QObject::eventFilter(obj, event);
}

void StateEventFilter::updateCheckedState(bool checked)
{
	utils->toggleClass(target, "checked", checked);
}

} // namespace idian
