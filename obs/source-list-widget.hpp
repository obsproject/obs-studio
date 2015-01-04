#pragma once

#include <QListWidget>

class QMouseEvent;

class SourceListWidget : public QListWidget {
	Q_OBJECT
public:
	inline SourceListWidget(QWidget *parent = nullptr)
		: QListWidget(parent)
	{
	}

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
};
