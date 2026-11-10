#include "moc_int-slider.cpp"

#include <cmath>

IntSlider::IntSlider(QWidget *parent) : SliderIgnoreScroll(parent)
{
	connect(this, &IntSlider::valueChanged, this,
		[this](int val) { emit intValChanged(((double)minVal / minStep + val) * minStep); });
}

void IntSlider::setIntConstraints(int newMin, int newMax, int newStep, int val)
{
	minVal = newMin;
	maxVal = newMax;
	minStep = newStep;

	double total = maxVal - minVal;
	int intMax = int(total / minStep);

	setMinimum(0);
	setMaximum(intMax);
	setSingleStep(1);
	setIntVal(val);
}

void IntSlider::setIntVal(int val)
{
	setValue((int)lround((double)(val - minVal) / minStep));
}
