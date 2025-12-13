#pragma once

#include <QDockWidget>

class QCloseEvent;
class QShowEvent;
class QString;

class OBSDock : public QDockWidget {
	Q_OBJECT

public:
	inline OBSDock(QWidget *parent = nullptr) : QDockWidget(parent) {}
	inline OBSDock(const QString &title, QWidget *parent = nullptr) : QDockWidget(title, parent) {}

	virtual void closeEvent(QCloseEvent *event);
	virtual void showEvent(QShowEvent *event);
};
