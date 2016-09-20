#include "focus-list.hpp"

FocusList::FocusList(QWidget *parent) : QListWidget(parent) {}

void FocusList::focusInEvent(QFocusEvent *event)
{
	QListWidget::focusInEvent(event);

	emit GotFocus();
}
