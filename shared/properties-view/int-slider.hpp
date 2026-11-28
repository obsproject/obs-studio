#pragma once

#include <QSlider>
#include <slider-ignorewheel.hpp>

class IntSlider : public SliderIgnoreScroll {
	Q_OBJECT

	int minVal, maxVal, minStep;

public:
	IntSlider(QWidget *parent = nullptr);

	void setIntConstraints(int newMin, int newMax, int newStep, int val);

signals:
	void intValChanged(int val);

public slots:
	void setIntVal(int val);
};
