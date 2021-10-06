#pragma once

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QInputEvent>
#include <QtCore/QObject>

class SpinBoxIgnoreScroll : public QSpinBox {
	Q_OBJECT

public:
	SpinBoxIgnoreScroll(QWidget *parent = nullptr);

protected:
	virtual void wheelEvent(QWheelEvent *event) override;
};

class DoubleSpinBoxIgnoreScroll : public QDoubleSpinBox {

public:
	DoubleSpinBoxIgnoreScroll(QWidget *parent = nullptr);

protected:
	virtual void wheelEvent(QWheelEvent *event) override;
};
