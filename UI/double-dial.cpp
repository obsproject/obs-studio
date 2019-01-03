#include "double-dial.hpp"

#include <cmath>

DoubleDial::DoubleDial(QWidget *parent) : QDial(parent)
{
	connect(this, SIGNAL(valueChanged(int)),
			this, SLOT(intValChanged(int)));
}

void DoubleDial::setDoubleConstraints(double newMin, double newMax,
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

void DoubleDial::intValChanged(int val)
{
	emit doubleValChanged((minVal/minStep + val) * minStep);
}

void DoubleDial::setDoubleVal(double val)
{
	setValue(lround((val - minVal) / minStep));
}
