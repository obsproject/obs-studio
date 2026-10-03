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
#include <QLabel>

namespace idian {
StateEventFilter::StateEventFilter(QWidget *target) : QObject(target), target(target)
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
		Utils::repolish(widget);

		Utils::polishChildren(widget);

		break;
	case QEvent::FocusIn:
		Utils::toggleClass(widget, "focus", true);

		focusEvent = static_cast<QFocusEvent *>(event);
		if (focusEvent->reason() != Qt::MouseFocusReason && focusEvent->reason() != Qt::PopupFocusReason) {
			Utils::toggleClass(widget, "keyFocus", true);
		} else {
			Utils::toggleClass(widget, "keyFocus", false);
		}

		Utils::polishChildren(widget);

		break;
	case QEvent::FocusOut:
		Utils::toggleClass(widget, "focus", false);

		focusEvent = static_cast<QFocusEvent *>(event);
		if (focusEvent->reason() != Qt::PopupFocusReason) {
			Utils::toggleClass(widget, "keyFocus", false);
			Utils::polishChildren(widget);
		}

		Utils::polishChildren(widget);

		break;
	case QEvent::HoverEnter:
		if (widget->isEnabled()) {
			Utils::toggleClass(widget, "hover", true);
		}

		Utils::polishChildren(widget);

		break;
	case QEvent::HoverLeave:
		Utils::toggleClass(widget, "hover", false);

		Utils::polishChildren(widget);

		break;
	case QEvent::EnabledChange:
		Utils::toggleClass(widget, "disabled", !widget->isEnabled());

		Utils::polishChildren(widget);

		break;
	default:
		updateIconColors = false;
		break;
	}

	if (updateIconColors) {
		// Delay icon update
		if (QLabel *label = qobject_cast<QLabel *>(widget)) {
			QMetaObject::invokeMethod(
				this, [this, label]() { Utils::applyColorToIcon(label); }, Qt::QueuedConnection);
		} else if (QAbstractButton *button = qobject_cast<QAbstractButton *>(widget)) {
			QMetaObject::invokeMethod(
				this, [this, button]() { Utils::applyColorToIcon(button); }, Qt::QueuedConnection);
		}
	}

	return QObject::eventFilter(obj, event);
}

void StateEventFilter::updateCheckedState(bool checked)
{
	Utils::toggleClass(target, "checked", checked);
}

} // namespace idian
