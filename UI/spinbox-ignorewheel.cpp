#include "spinbox-ignorewheel.hpp"

SpinBoxIgnoreScroll::SpinBoxIgnoreScroll(QWidget *parent) : QSpinBox(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

void SpinBoxIgnoreScroll::wheelEvent(QWheelEvent *event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSpinBox::wheelEvent(event);
}

DoubleSpinBoxIgnoreScroll::DoubleSpinBoxIgnoreScroll(QWidget *parent)
	: QDoubleSpinBox(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

void DoubleSpinBoxIgnoreScroll::wheelEvent(QWheelEvent *event)
{
	if (!hasFocus())
		event->ignore();
	else
		QDoubleSpinBox::wheelEvent(event);
}
