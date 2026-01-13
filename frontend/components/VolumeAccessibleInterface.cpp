#include "VolumeAccessibleInterface.hpp"

#include <components/VolumeSlider.hpp>

VolumeAccessibleInterface::VolumeAccessibleInterface(QWidget *w) : QAccessibleWidget(w) {}

VolumeSlider *VolumeAccessibleInterface::slider() const
{
	return qobject_cast<VolumeSlider *>(object());
}

QString VolumeAccessibleInterface::text(QAccessible::Text t) const
{
	if (slider()->isVisible()) {
		switch (t) {
		case QAccessible::Text::Value:
			return currentValue().toString();
		default:
			break;
		}
	}
	return QAccessibleWidget::text(t);
}

QVariant VolumeAccessibleInterface::currentValue() const
{
	QString text;
	float db = obs_fader_get_db(slider()->fad);

	if (db < -96.0f)
		text = "-inf dB";
	else
		text = QString::number(db, 'f', 1).append(" dB");

	return text;
}

void VolumeAccessibleInterface::setCurrentValue(const QVariant &value)
{
	slider()->setValue(value.toInt());
}

QVariant VolumeAccessibleInterface::maximumValue() const
{
	return slider()->maximum();
}

QVariant VolumeAccessibleInterface::minimumValue() const
{
	return slider()->minimum();
}

QVariant VolumeAccessibleInterface::minimumStepSize() const
{
	return slider()->singleStep();
}

QAccessible::Role VolumeAccessibleInterface::role() const
{
	return QAccessible::Role::Slider;
}
