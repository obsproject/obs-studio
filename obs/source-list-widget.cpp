#include <QMouseEvent>
#include "source-list-widget.hpp"

void SourceListWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		QListWidget::mouseDoubleClickEvent(event);
}
