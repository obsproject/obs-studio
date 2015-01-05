#pragma once

#include<QListWidget>

class QMouseEvent;

class LDCListWidget : public QListWidget{
	Q_OBJECT
public:
	inline LDCListWidget(QWidget *parent = nullptr)
		:QListWidget(parent)
	{
	}

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
};
