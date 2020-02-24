#pragma once

#include <QSlider>
#include <QMouseEvent>

class MediaSlider : public QSlider {
	Q_OBJECT

public:
	inline MediaSlider(QWidget *parent = nullptr) : QSlider(parent)
	{
		setMouseTracking(true);
	};

signals:
	void mediaSliderHovered(int value);
	void mediaSliderReleased(int value);
	void mediaSliderClicked();

protected:
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
};
