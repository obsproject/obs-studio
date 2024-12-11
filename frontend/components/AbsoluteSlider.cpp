#include "AbsoluteSlider.hpp"
#include "moc_AbsoluteSlider.cpp"

AbsoluteSlider::AbsoluteSlider(QWidget *parent) : SliderIgnoreScroll(parent)
{
	installEventFilter(this);
	setMouseTracking(true);
}

AbsoluteSlider::AbsoluteSlider(Qt::Orientation orientation, QWidget *parent) : SliderIgnoreScroll(orientation, parent)
{
	installEventFilter(this);
	setMouseTracking(true);
}

void AbsoluteSlider::mousePressEvent(QMouseEvent *event)
{
	dragging = (event->buttons() & Qt::LeftButton || event->buttons() & Qt::MiddleButton);

	if (dragging) {
		setSliderDown(true);
		setValue(posToRangeValue(event));
		emit AbsoluteSlider::sliderMoved(posToRangeValue(event));
	}

	event->accept();
}

void AbsoluteSlider::mouseReleaseEvent(QMouseEvent *event)
{
	dragging = false;
	setSliderDown(false);
	event->accept();
}

void AbsoluteSlider::mouseMoveEvent(QMouseEvent *event)
{
	int val = posToRangeValue(event);

	if (val > maximum())
		val = maximum();
	else if (val < minimum())
		val = minimum();

	emit absoluteSliderHovered(val);

	if (dragging) {
		setValue(posToRangeValue(event));
		emit AbsoluteSlider::sliderMoved(posToRangeValue(event));
	}

	QSlider::mouseMoveEvent(event);
	event->accept();
}

bool AbsoluteSlider::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

		if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
			return true;
		}
	}

	return QSlider::eventFilter(obj, event);
}

int AbsoluteSlider::posToRangeValue(QMouseEvent *event)
{
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	int pos;
	int sliderMin;
	int sliderMax;
	int handleLength;

	const QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
	const QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

	if (orientation() == Qt::Horizontal) {
		pos = event->pos().x();
		handleLength = handle.width();
		sliderMin = groove.left() + (handleLength / 2);
		sliderMax = groove.right() - (handleLength / 2) + 1;
	} else {
		pos = event->pos().y();
		handleLength = handle.height();
		sliderMin = groove.top() + (handleLength / 2);
		sliderMax = groove.bottom() - (handleLength / 2) + 1;
	}

	int sliderValue = style()->sliderValueFromPosition(minimum(), maximum(), pos - sliderMin, sliderMax - sliderMin,
							   opt.upsideDown);

	return sliderValue;
}
