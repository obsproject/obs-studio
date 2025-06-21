#include "OBSBasic.hpp"
#include "dialogs/NotificationDialog.hpp"
#include "widgets/Notification.hpp"

#include <QDesktopServices>
#include <QLabel>
#include <QVBoxLayout>

constexpr int leftMargin = 4;

QPoint OBSBasic::getNotificationPosition(QWidget *widget)
{
	QPoint point(geometry().bottomLeft());
	point.setX(point.x() + leftMargin);
	point.setY(point.y() - widget->height() - statusBar()->height());

	return point;
}

void OBSBasic::initNotifications()
{
	notificationDialog = new NotificationDialog(this);
	connect(ui->statusbar, &OBSBasicStatusBar::notificationBellClicked, this, &OBSBasic::showNotificationDialog);
	connect(notificationDialog, &NotificationDialog::hidden, this, &OBSBasic::resetNotificationCount);

	bool doNotDisturb = config_get_bool(App()->GetUserConfig(), "BasicWindow", "DoNotDisturb");
	notificationDialog->setDoNotDisturbEnabled(doNotDisturb);
}

void OBSBasic::saveNotifications()
{
	bool doNotDisturb = notificationDialog->isDoNotDisturbEnabled();
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "DoNotDisturb", doNotDisturb);
}

Notification *OBSBasic::getCurrentNotification()
{
	return findChild<Notification *>("notification", Qt::FindDirectChildrenOnly);
}

void OBSBasic::showNotificationDialog()
{
	notificationDialog->setFixedSize(QSize(400, height() / 2));
	notificationDialog->move(getNotificationPosition(notificationDialog));
	notificationDialog->show();

	Notification *current = getCurrentNotification();

	if (current)
		notificationDialog->addNotification(current);

	resetNotificationCount();
	notificationDialog->updateWidgets();
}

void OBSBasic::resetNotificationCount()
{
	unreadNotificationCount = 0;
	unreadImportantNotificationCount = 0;
	emit notificationsUpdated(unreadNotificationCount, unreadImportantNotificationCount);
}

Notification *OBSBasic::addNotification(const QString &iconName, const QString &title, const QString &text,
					bool important)
{
	Notification *current = getCurrentNotification();

	if (current)
		notificationDialog->addNotification(current);

	Notification *notification = new Notification(iconName, title, text, important, this);
	connect(notification, &Notification::closed, this, &OBSBasic::closeNotification);
	connect(notificationDialog, &NotificationDialog::hidden, notification, &Notification::setAsRead);
	notification->setObjectName("notification");
	unreadNotificationCount++;

	if (notification->isImportant())
		unreadImportantNotificationCount++;

	return notification;
}

void OBSBasic::showNotification(Notification *notification)
{
	if (notificationDialog->isDoNotDisturbEnabled() || notificationDialog->isVisible()) {
		notificationDialog->addNotification(notification);
	} else {
		auto addToDialog = [this, notification]() {
			notificationDialog->addNotification(notification);
		};

		connect(notification, &Notification::timeout, this, addToDialog, Qt::QueuedConnection);
		notification->show();
		notification->startShowingTimer();
	}

	emit notificationsUpdated(unreadNotificationCount, unreadImportantNotificationCount);
}

void OBSBasic::closeNotification(Notification *notification)
{
	Notification *current = getCurrentNotification();

	if (current == notification) {
		unreadNotificationCount--;

		if (notification->isImportant())
			unreadImportantNotificationCount--;
	}

	notification->deleteLater();

	emit notificationsUpdated(unreadNotificationCount, unreadImportantNotificationCount);
}

void OBSBasic::showSaveNotification(const QString &icon, const QString &title, const QString &text, const QString &path)
{
	Notification *notification = addNotification(icon, title, text.arg(path));
	notification->addAction(QTStr("Notification.GoToFolder"), this, &OBSBasic::on_actionShow_Recordings_triggered);

	auto goToFile = [path]() {
		QUrl url = QUrl::fromLocalFile(path);
		QDesktopServices::openUrl(url);
	};

	notification->addAction(QTStr("Notification.OpenFile"), this, goToFile);
	showNotification(notification);
}
