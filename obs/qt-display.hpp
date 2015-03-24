#pragma once

#include <QWidget>

class OBSQTDisplay : public QWidget {
	Q_OBJECT

	virtual void resizeEvent(QResizeEvent *event)
	{
		emit DisplayResized();
		QWidget::resizeEvent(event);
	}

signals:
	void DisplayResized();

public:
	inline OBSQTDisplay(QWidget *parent = 0, Qt::WindowFlags flags = 0)
		: QWidget(parent, flags)
	{
		setAttribute(Qt::WA_PaintOnScreen);
		setAttribute(Qt::WA_StaticContents);
		setAttribute(Qt::WA_NoSystemBackground);
		setAttribute(Qt::WA_OpaquePaintEvent);
		setAttribute(Qt::WA_DontCreateNativeAncestors);
		setAttribute(Qt::WA_NativeWindow);
	}

	virtual QPaintEngine *paintEngine() const override {return nullptr;}
};
