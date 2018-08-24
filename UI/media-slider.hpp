#pragma once

#include <QSlider>
#include <QMouseEvent>

class MediaSlider : public QSlider {
	Q_OBJECT

public:
	MediaSlider(QWidget *parent = nullptr);

signals:
	void mediaSliderHovered(int value);

protected:
	virtual void mouseMoveEvent(QMouseEvent *event) override;
};
