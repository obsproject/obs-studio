#pragma once

#include <QSlider>
#include <QMouseEvent>

class DoubleSlider : public QSlider {
	Q_OBJECT

	double minVal, maxVal, minStep;

public:
	DoubleSlider(QWidget *parent = nullptr);

	void setDoubleConstraints(double newMin, double newMax,
			double newStep, double val);

signals:
	void doubleValChanged(double val);
	void doubleClicked();

public slots:
	void intValChanged(int val);
	void setDoubleVal(double val);

protected:
	void mouseDoubleClickEvent(QMouseEvent *event)
	{
		emit doubleClicked();
		event->accept();
	}
};
