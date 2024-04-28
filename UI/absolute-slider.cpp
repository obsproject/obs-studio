#include "slider-absoluteset-style.hpp"
#include "absolute-slider.hpp"
#include <QStyleFactory>

AbsoluteSlider::AbsoluteSlider(QWidget *parent) : SliderIgnoreScroll(parent)
{
	installEventFilter(this);
	setMouseTracking(true);

	QString styleName = style()->objectName();
	QStyle *style;
	style = QStyleFactory::create(styleName);
	if (!style) {
		style = new SliderAbsoluteSetStyle();
	} else {
		style = new SliderAbsoluteSetStyle(style);
	}

	style->setParent(this);
	this->setStyle(style);
}

void AbsoluteSlider::mouseMoveEvent(QMouseEvent *event)
{
	int val = minimum() +
		  ((maximum() - minimum()) * event->pos().x()) / width();

	if (val > maximum())
		val = maximum();
	else if (val < minimum())
		val = minimum();

	emit absoluteSliderHovered(val);
	event->accept();
	QSlider::mouseMoveEvent(event);
}

bool AbsoluteSlider::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Up ||
		    keyEvent->key() == Qt::Key_Down) {
			return true;
		}
	}

	if (event->type() == QEvent::Wheel)
		return true;

	return QSlider::eventFilter(obj, event);
}
