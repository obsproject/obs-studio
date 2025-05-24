#include "FocusList.hpp"

#include <QDragMoveEvent>

#include "moc_FocusList.cpp"

FocusList::FocusList(QWidget *parent) : QListWidget(parent) {}

void FocusList::focusInEvent(QFocusEvent *event)
{
	QListWidget::focusInEvent(event);

	emit GotFocus();
}

void FocusList::dragMoveEvent(QDragMoveEvent *event)
{
	QPoint pos = event->position().toPoint();
	int itemRow = row(itemAt(pos));

	if ((itemRow == currentRow() + 1) || (currentRow() == count() - 1 && itemRow == -1))
		event->ignore();
	else
		QListWidget::dragMoveEvent(event);
}
