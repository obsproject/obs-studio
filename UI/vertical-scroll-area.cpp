#include <QResizeEvent>
#include "vertical-scroll-area.hpp"

void VScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumWidth(event->size().width());

	QScrollArea::resizeEvent(event);
}
