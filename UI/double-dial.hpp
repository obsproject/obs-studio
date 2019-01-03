#pragma once

#include <QDial>

class DoubleDial : public QDial {
	Q_OBJECT

	double minVal, maxVal, minStep;

public:
	DoubleDial(QWidget *parent = nullptr);

	void setDoubleConstraints(double newMin, double newMax,
			double newStep, double val);

signals:
	void doubleValChanged(double val);

public slots:
	void intValChanged(int val);
	void setDoubleVal(double val);
};
