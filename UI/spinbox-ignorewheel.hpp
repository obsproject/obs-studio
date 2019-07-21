#pragma once

#include <QSpinBox>
#include <QInputEvent>
#include <QtCore/QObject>

class SpinBoxIgnoreScroll : public QSpinBox {
	Q_OBJECT

public:
	SpinBoxIgnoreScroll(QWidget *parent = nullptr);

protected:
	void wheelEvent(QWheelEvent *event) override;
};
