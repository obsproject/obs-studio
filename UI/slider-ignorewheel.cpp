#include "slider-ignorewheel.hpp"

SliderIgnoreScroll::SliderIgnoreScroll(QWidget *parent) : QSlider(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

void SliderIgnoreScroll::wheelEvent(QWheelEvent * event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSlider::wheelEvent(event);
}

void SliderIgnoreScroll::leaveEvent(QEvent * event)
{
	clearFocus();
}
