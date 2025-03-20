#include "AbsoluteSlider.hpp"

#include <QPainter>

#include "moc_AbsoluteSlider.cpp"

AbsoluteSlider::AbsoluteSlider(QWidget *parent) : SliderIgnoreScroll(parent)
{
	installEventFilter(this);
	setMouseTracking(true);

	tickColor.setRgb(0x5b, 0x62, 0x73);
}

AbsoluteSlider::AbsoluteSlider(Qt::Orientation orientation, QWidget *parent) : SliderIgnoreScroll(orientation, parent)
{
	installEventFilter(this);
	setMouseTracking(true);

	tickColor.setRgb(0x5b, 0x62, 0x73);
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

bool AbsoluteSlider::getDisplayTicks() const
{
	return displayTicks;
}

void AbsoluteSlider::setDisplayTicks(bool display)
{
	displayTicks = display;
}

QColor AbsoluteSlider::getTickColor() const
{
	return tickColor;
}

void AbsoluteSlider::setTickColor(QColor c)
{
	tickColor = std::move(c);
}

void AbsoluteSlider::paintEvent(QPaintEvent *event)
{
	if (!getDisplayTicks()) {
		QSlider::paintEvent(event);
		return;
	}

	QPainter painter(this);

	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
	QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

	const bool isHorizontal = orientation() == Qt::Horizontal;

	const int sliderLength = isHorizontal ? groove.width() - handle.width() : groove.height() - handle.height();
	const int handleSize = isHorizontal ? handle.width() : handle.height();
	const int grooveSize = isHorizontal ? groove.height() : groove.width();
	const int grooveStart = isHorizontal ? groove.left() : groove.top();
	const int tickLinePos = isHorizontal ? groove.center().y() : groove.center().x();
	const int tickLength = std::max((int)(grooveSize * 1.5) + grooveSize, 8 + grooveSize);
	const int tickLineStart = tickLinePos - (tickLength / 2) + 1;

	for (double offset = minimum(); offset <= maximum(); offset += singleStep()) {
		double tickPercent = (offset - minimum()) / (maximum() - minimum());
		const int tickLineOffset = grooveStart + std::floor(sliderLength * tickPercent) + (handleSize / 2);

		const int xPos = isHorizontal ? tickLineOffset : tickLineStart;
		const int yPos = isHorizontal ? tickLineStart : tickLineOffset;

		const int tickWidth = isHorizontal ? 1 : tickLength;
		const int tickHeight = isHorizontal ? tickLength : 1;

		painter.fillRect(xPos, yPos, tickWidth, tickHeight, tickColor);
	}

	QSlider::paintEvent(event);
}
