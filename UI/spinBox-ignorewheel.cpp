#include "spinBox-ignorewheel.hpp"

SpinBoxIgnoreScroll::SpinBoxIgnoreScroll(QWidget *parent) : QSpinBox(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

void SpinBoxIgnoreScroll::wheelEvent(QWheelEvent * event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSpinBox::wheelEvent(event);
}

void SpinBoxIgnoreScroll::leaveEvent(QEvent * event)
{
	clearFocus();
}
