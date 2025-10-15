#include <Idian/StateEventFilter.hpp>
#include <Idian/Utils.hpp>

#include <QAbstractButton>
#include <QLabel>

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

	switch (event->type()) {
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

		break;
	case QEvent::MouseButtonPress:
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
		bool widgetEnabled = widget->isEnabled();
		utils->toggleClass(widget, "disabled", !widgetEnabled);

		utils->polishChildren(widget);
		break;
	}

	return QObject::eventFilter(obj, event);
}

void StateEventFilter::updateCheckedState(bool checked)
{
	utils->toggleClass(target, "checked", checked);
}

} // namespace idian
