#pragma once

#include <QDialog>

class Notification;
class QHideEvent;
class Ui_NotificationDialog;

class NotificationDialog : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui_NotificationDialog> ui;

public:
	NotificationDialog(QWidget *parent = nullptr);
	~NotificationDialog();

	void addNotification(Notification *widget);

	void setDoNotDisturbEnabled(bool enable);
	bool isDoNotDisturbEnabled();

public slots:
	void updateWidgets();

protected:
	virtual void hideEvent(QHideEvent *event);

signals:
	void hidden();
};
