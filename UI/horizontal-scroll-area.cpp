#include <QResizeEvent>
#include "horizontal-scroll-area.hpp"

void HScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumHeight(event->size().height());

	QScrollArea::resizeEvent(event);
}
