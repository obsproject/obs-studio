#include "slider-ignorewheel.hpp"

SliderIgnoreScroll::SliderIgnoreScroll(QWidget *parent) : QSlider(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

SliderIgnoreScroll::SliderIgnoreScroll(Qt::Orientation orientation,
				       QWidget *parent)
	: QSlider(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setOrientation(orientation);
}

void SliderIgnoreScroll::wheelEvent(QWheelEvent *event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSlider::wheelEvent(event);
}

void SliderIgnoreClick::mousePressEvent(QMouseEvent *event)
{
	QStyleOptionSlider styleOption;
	initStyleOption(&styleOption);
	QRect handle = style()->subControlRect(QStyle::CC_Slider, &styleOption,
					       QStyle::SC_SliderHandle, this);
	if (handle.contains(event->position().toPoint())) {
		SliderIgnoreScroll::mousePressEvent(event);
		dragging = true;
	} else {
		event->accept();
	}
}

void SliderIgnoreClick::mouseReleaseEvent(QMouseEvent *event)
{
	dragging = false;
	SliderIgnoreScroll::mouseReleaseEvent(event);
}

void SliderIgnoreClick::mouseMoveEvent(QMouseEvent *event)
{
	if (dragging) {
		SliderIgnoreScroll::mouseMoveEvent(event);
	} else {
		event->accept();
	}
}

void SliderIgnoreClick::mousePressEvent(QMouseEvent *event)
{
	QStyleOptionSlider styleOption;
	initStyleOption(&styleOption);
	QRect handle = style()->subControlRect(QStyle::CC_Slider, &styleOption,
					       QStyle::SC_SliderHandle, this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	QPointF pointExact = event->position();
#endif
	if (handle.contains(
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		    QPoint(pointExact.x(), pointExact.y())
#else
		    // Ubuntu 20.04. Sigh.
		    QPoint(event->x(), event->y())
#endif
			    )) {
		SliderIgnoreScroll::mousePressEvent(event);
		dragging = true;
	} else {
		event->accept();
	}
}

void SliderIgnoreClick::mouseReleaseEvent(QMouseEvent *event)
{
	dragging = false;
	SliderIgnoreScroll::mouseReleaseEvent(event);
}

void SliderIgnoreClick::mouseMoveEvent(QMouseEvent *event)
{
	if (dragging) {
		SliderIgnoreScroll::mouseMoveEvent(event);
	} else {
		event->accept();
	}
}
