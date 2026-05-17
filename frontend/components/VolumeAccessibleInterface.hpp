#pragma once

#include <QAccessibleWidget>

class VolumeSlider;

class VolumeAccessibleInterface : public QAccessibleWidget {

public:
	VolumeAccessibleInterface(QWidget *w);

	QVariant currentValue() const;
	void setCurrentValue(const QVariant &value);

	QVariant maximumValue() const;
	QVariant minimumValue() const;

	QVariant minimumStepSize() const;

private:
	VolumeSlider *slider() const;

protected:
	virtual QAccessible::Role role() const override;
	virtual QString text(QAccessible::Text t) const override;
};
