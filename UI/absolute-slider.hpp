#pragma once

#include <QMouseEvent>
#include "slider-ignorewheel.hpp"

class AbsoluteSlider : public SliderIgnoreScroll {
	Q_OBJECT

public:
	AbsoluteSlider(QWidget *parent = nullptr);

signals:
	void absoluteSliderHovered(int value);

protected:
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual bool eventFilter(QObject *obj, QEvent *event) override;
};
