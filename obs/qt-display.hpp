#pragma once

#include <QWidget>

class OBSQTDisplay : public QWidget {
	Q_OBJECT

public:
	inline OBSQTDisplay(QWidget *parent = 0, Qt::WindowFlags f = 0)
		: QWidget(parent, f)
	{
		setAutoFillBackground(false);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAttribute(Qt::WA_NativeWindow);
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_NoSystemBackground);
	}

	virtual QPaintEngine *paintEngine() const {return nullptr;}
};
