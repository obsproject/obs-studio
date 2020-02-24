#include "media-slider.hpp"

void MediaSlider::mouseMoveEvent(QMouseEvent *event)
{
	int val = minimum() + ((maximum() - minimum()) * event->x()) / width();

	if (val > maximum())
		val = maximum();
	else if (val < minimum())
		val = minimum();

	emit mediaSliderHovered(val);
	event->accept();
	QSlider::mouseMoveEvent(event);
}

void MediaSlider::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		emit mediaSliderClicked();
		event->accept();
	}

	QSlider::mousePressEvent(event);
}

void MediaSlider::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		int val = minimum() +
			  ((maximum() - minimum()) * event->x()) / width();

		if (val > maximum())
			val = maximum();
		else if (val < minimum())
			val = minimum();

		emit mediaSliderReleased(val);
		event->accept();
	}

	QSlider::mouseReleaseEvent(event);
}
