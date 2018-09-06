#pragma once

#include <QSlider>
#include <QMouseEvent>

class OBSSlider : public QSlider {
	Q_OBJECT

public:
	inline OBSSlider(QWidget *parent = 0) : QSlider(parent) {}

signals:
	void doubleClicked();

protected:
	void mouseDoubleClickEvent(QMouseEvent *event)
	{
		emit doubleClicked();
		event->accept();
	}
};
