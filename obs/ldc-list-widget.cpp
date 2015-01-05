#include <QMouseEvent>
#include "ldc-list-widget.hpp"

void LDCListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		QListWidget::mouseDoubleClickEvent(event);
}
