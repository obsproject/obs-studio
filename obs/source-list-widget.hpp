#pragma once

#include <QListWidget>

class QMouseEvent;

class SourceListWidget : public QListWidget {
	Q_OBJECT

	bool ignoreReorder = false;
public:
	inline SourceListWidget(QWidget *parent = nullptr)
		: QListWidget(parent)
	{
	}

	bool IgnoreReorder() const { return ignoreReorder; }

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void dropEvent(QDropEvent *event) override;
};
