#include "double-slider.hpp"

DoubleSlider::DoubleSlider(QWidget *parent) : QSlider(parent)
{
	connect(this, SIGNAL(valueChanged(int)),
			this, SLOT(intValChanged(int)));
}

void DoubleSlider::setDoubleConstraints(double newMin, double newMax,
		double newStep, double val)
{
	minVal = newMin;
	maxVal = newMax;
	minStep = newStep;

	double total = maxVal - minVal;
	int intMax = int(total / minStep);

	setMinimum(0);
	setMaximum(intMax);
	setSingleStep(1);
	setDoubleVal(val);
}

void DoubleSlider::intValChanged(int val)
{
	emit doubleValChanged(double(val) * minStep + minVal);
}

void DoubleSlider::setDoubleVal(double val)
{
	setValue(int((val - minVal) / minStep));
}
