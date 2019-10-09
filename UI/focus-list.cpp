#include "focus-list.hpp"
#include <QDragMoveEvent>

FocusList::FocusList(QWidget *parent) : QListWidget(parent) {}

void FocusList::focusInEvent(QFocusEvent *event)
{
	QListWidget::focusInEvent(event);

	emit GotFocus();
}

void FocusList::dragMoveEvent(QDragMoveEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QPoint pos = event->position().toPoint();
#else
	QPoint pos = event->pos();
#endif
	int itemRow = row(itemAt(pos));

	if ((itemRow == currentRow() + 1) ||
	    (currentRow() == count() - 1 && itemRow == -1))
		event->ignore();
	else
		QListWidget::dragMoveEvent(event);
}
