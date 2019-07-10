#pragma once

#include <QDockWidget>

class OBSDock : public QDockWidget {
	Q_OBJECT

signals:
	void dockHidden();
	void dockShown();

public:
	inline OBSDock(QWidget *parent = nullptr) : QDockWidget(parent) {}

	virtual void closeEvent(QCloseEvent *event);
	virtual void hideEvent(QHideEvent *event);
	virtual void showEvent(QShowEvent *event);
};
