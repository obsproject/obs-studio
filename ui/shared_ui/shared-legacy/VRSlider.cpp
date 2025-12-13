#include "VRSlider.h"

VRSlider::VRSlider(Qt::Orientation orientation, QWidget *parent) : QSlider(orientation, parent)
{
	setStyleSheet(VRTheme::sliderStyle());
	setCursor(Qt::PointingHandCursor);
}
