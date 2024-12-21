#include <QResizeEvent>
#include "HScrollArea.hpp"
#include "moc_HScrollArea.cpp"

void HScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumHeight(event->size().height());

	QScrollArea::resizeEvent(event);
}
