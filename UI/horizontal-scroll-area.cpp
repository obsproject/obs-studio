#include <QResizeEvent>
#include "moc_horizontal-scroll-area.cpp"

void HScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumHeight(event->size().height());

	QScrollArea::resizeEvent(event);
}
