#pragma once

#include <QPushButton>
#include <QResizeEvent>

class OBSBasicControls;

class OBSBasicControlsButton : public QPushButton {
	Q_OBJECT
public:
	inline OBSBasicControlsButton(QWidget *parent = nullptr)
		: QPushButton(parent)
	{
	}
	inline OBSBasicControlsButton(const QString &text,
				      QWidget *parent = nullptr)
		: QPushButton(text, parent)
	{
	}

	inline virtual void resizeEvent(QResizeEvent *event) override
	{
		emit ResizeEvent();
		event->accept();
	}

signals:
	void ResizeEvent();
};
