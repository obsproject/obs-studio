#include "NotificationDialog.hpp"
#include "ui_NotificationDialog.h"

#include "widgets/Notification.hpp"
#include "widgets/OBSBasic.hpp"

#include <QHideEvent>

#include "moc_NotificationDialog.cpp"

NotificationDialog::NotificationDialog(QWidget *parent) : QDialog(parent), ui(new Ui::NotificationDialog)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

	ui->setupUi(this);
	ui->mainLayout->setAlignment(Qt::AlignTop);

	updateWidgets();
}

NotificationDialog::~NotificationDialog() {}

void NotificationDialog::addNotification(Notification *widget)
{
	widget->setNotificationIsInDialog();

	if (widget->isImportant())
		ui->importantLayout->insertWidget(0, widget);
	else
		ui->notificationLayout->insertWidget(0, widget);

	updateWidgets();

	connect(OBSBasic::Get(), &OBSBasic::notificationsUpdated, this, &NotificationDialog::updateWidgets,
		Qt::QueuedConnection);
}

void NotificationDialog::updateWidgets()
{
	bool important = ui->importantLayout->count() > 0;
	bool notifications = ui->notificationLayout->count() > 0;

	ui->importantLabel->setVisible(important);
	ui->moreLabel->setVisible(notifications && important);

	ui->scrollArea->setVisible(important || notifications);
	ui->noNotificationsLabel->setVisible(!important && !notifications);

	adjustSize();
}

void NotificationDialog::setDoNotDisturbEnabled(bool enable)
{
	ui->doNotDisturb->setChecked(enable);
}

bool NotificationDialog::isDoNotDisturbEnabled()
{
	return ui->doNotDisturb->isChecked();
}

void NotificationDialog::hideEvent(QHideEvent *event)
{
	emit hidden();
	QWidget::hideEvent(event);
}
