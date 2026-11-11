#pragma once

#include <QDockWidget>

class QCloseEvent;
class QShowEvent;
class QString;

class OBSDock : public QDockWidget {
	Q_OBJECT

public:
	OBSDock(QWidget *parent = nullptr);
	OBSDock(const QString &title, QWidget *parent = nullptr);

	virtual void closeEvent(QCloseEvent *event);
	virtual void showEvent(QShowEvent *event);
};
