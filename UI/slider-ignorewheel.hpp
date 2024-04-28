#pragma once

#include "obs.hpp"
#include <QSlider>
#include <QInputEvent>
#include <QtCore/QObject>
#include <QStyleOptionSlider>

class SliderIgnoreScroll : public QSlider {
	Q_OBJECT

public:
	SliderIgnoreScroll(QWidget *parent = nullptr);
	SliderIgnoreScroll(Qt::Orientation orientation,
			   QWidget *parent = nullptr);

protected:
	virtual void wheelEvent(QWheelEvent *event) override;
};

class SliderIgnoreClick : public SliderIgnoreScroll {
public:
	inline SliderIgnoreClick(Qt::Orientation orientation,
				 QWidget *parent = nullptr)
		: SliderIgnoreScroll(orientation, parent)
	{
	}

protected:
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
	bool dragging = false;
};
