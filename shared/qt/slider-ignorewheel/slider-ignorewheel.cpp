#include "moc_slider-ignorewheel.cpp"

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
