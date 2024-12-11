#include "VolumeSlider.hpp"

#include <QPainter>

#include "moc_VolumeSlider.cpp"

VolumeSlider::VolumeSlider(obs_fader_t *fader, QWidget *parent) : AbsoluteSlider(parent)
{
	fad = fader;
}

VolumeSlider::VolumeSlider(obs_fader_t *fader, Qt::Orientation orientation, QWidget *parent)
	: AbsoluteSlider(orientation, parent)
{
	fad = fader;
}

bool VolumeSlider::getDisplayTicks() const
{
	return displayTicks;
}

void VolumeSlider::setDisplayTicks(bool display)
{
	displayTicks = display;
}

void VolumeSlider::paintEvent(QPaintEvent *event)
{
	if (!getDisplayTicks()) {
		QSlider::paintEvent(event);
		return;
	}

	QPainter painter(this);
	QColor tickColor(91, 98, 115, 255);

	obs_fader_conversion_t fader_db_to_def = obs_fader_db_to_def(fad);

	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
	QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

	if (orientation() == Qt::Horizontal) {
		const int sliderWidth = groove.width() - handle.width();

		float tickLength = groove.height() * 1.5;
		tickLength = std::max((int)tickLength + groove.height(), 8 + groove.height());

		float yPos = groove.center().y() - (tickLength / 2) + 1;

		for (int db = -10; db >= -90; db -= 10) {
			float tickValue = fader_db_to_def(db);

			float xPos = groove.left() + (tickValue * sliderWidth) + (handle.width() / 2);
			painter.fillRect(xPos, yPos, 1, tickLength, tickColor);
		}
	}

	if (orientation() == Qt::Vertical) {
		const int sliderHeight = groove.height() - handle.height();

		float tickLength = groove.width() * 1.5;
		tickLength = std::max((int)tickLength + groove.width(), 8 + groove.width());

		float xPos = groove.center().x() - (tickLength / 2) + 1;

		for (int db = -10; db >= -96; db -= 10) {
			float tickValue = fader_db_to_def(db);

			float yPos =
				groove.height() + groove.top() - (tickValue * sliderHeight) - (handle.height() / 2);
			painter.fillRect(xPos, yPos, tickLength, 1, tickColor);
		}
	}

	QSlider::paintEvent(event);
}
