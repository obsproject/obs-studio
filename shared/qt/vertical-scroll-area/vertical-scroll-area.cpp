#include <QResizeEvent>
#include "moc_vertical-scroll-area.cpp"

void VScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumWidth(event->size().width());

	QScrollArea::resizeEvent(event);
}
