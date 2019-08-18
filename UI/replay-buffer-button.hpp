#pragma once

#include <QPushButton>

class ReplayBufferButton : public QPushButton {
	Q_OBJECT

public:
	inline ReplayBufferButton(QWidget *parent = nullptr)
		: QPushButton(parent)
	{
	}

	virtual void resizeEvent(QResizeEvent *event) override;
};
