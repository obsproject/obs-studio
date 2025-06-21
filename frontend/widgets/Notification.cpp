#include "Notification.hpp"
#include "ui_Notification.h"

#include "OBSApp.hpp"
#include "widgets/OBSBasic.hpp"
#include <util/platform.h>

#include <QDateTime>
#include <QMainWindow>
#include <QShowEvent>
#include <QStatusBar>
#include <QTimer>

#include "moc_Notification.cpp"

constexpr int leftMargin = 4;
constexpr int timerUpdateInterval = 60000;
constexpr int notificationShowTime = 10000;

Notification::Notification(const QString &iconName, const QString &title, const QString &text, bool important_,
			   QWidget *parent)
	: QFrame(parent),
	  timestamp(QDateTime::currentSecsSinceEpoch()),
	  important(important_),
	  ui(new Ui::Notification)
{
	BPtr<char> uuid_ = os_generate_uuid();
	uuid = uuid_.Get();

	ui->setupUi(this);

	if (!iconName.isEmpty()) {
		QIcon icon(iconName);

		if (!icon.isNull())
			ui->icon->setIcon(icon);
		else
			ui->icon->setProperty("class", iconName);
	}

	if (ui->icon->icon().isNull())
		ui->icon->hide();

	ui->title->setText(title);
	ui->text->setText(text);

	updateTimer = new QTimer(this);
	connect(updateTimer, &QTimer::timeout, this, &Notification::updateTimeLabel);
	updateTimer->start(timerUpdateInterval);

	connect(OBSBasic::Get(), &OBSBasic::resized, this, &Notification::onResize);
}

Notification::~Notification() {}

void Notification::onResize()
{
	if (inDialog)
		return;

	adjustSize();
	resize(400, height());

	QMainWindow *window = qobject_cast<QMainWindow *>(this->parent());

	if (window)
		move(leftMargin, window->height() - height() - window->statusBar()->height());
}

void Notification::startShowingTimer()
{
	showingTimer.reset(new QTimer(this));
	showingTimer->setSingleShot(true);
	showingTimer->setTimerType(Qt::PreciseTimer);
	connect(showingTimer.data(), &QTimer::timeout, this, [this]() { emit timeout(); });
	showingTimer->start(notificationShowTime);
}

void Notification::resetShowingTimer()
{
	showingTimer.reset();
}

void Notification::setAsRead()
{
	ui->title->setDisabled(true);
	ui->text->setDisabled(true);

	read = true;
}

bool Notification::isRead()
{
	return read;
}

void Notification::updateTimeLabel()
{
	qint64 seconds = QDateTime::currentSecsSinceEpoch() - timestamp;

	qint64 minutes = seconds / 60;
	qint64 hours = minutes / 60;
	qint64 days = hours / 24;

	QString text = "Now";

	if (minutes)
		text = minutes == 1 ? QTStr("MinuteAgo") : QTStr("MinutesAgo").arg(QString::number(minutes));

	if (hours)
		text = hours == 1 ? QTStr("HourAgo") : QTStr("HoursAgo").arg(QString::number(hours));

	if (days)
		text = days == 1 ? QTStr("Yesterday") : QTStr("DaysAgo").arg(QString::number(days));

	ui->timeLabel->setText(text);
}

void Notification::setNotificationIsInDialog()
{
	resetShowingTimer();
	inDialog = true;
}

void Notification::on_closeButton_clicked()
{
	emit closed(this);
}

void Notification::enterEvent(QEnterEvent *event)
{
	if (inDialog) {
		QWidget::enterEvent(event);
		return;
	}

	resetShowingTimer();
	QWidget::enterEvent(event);
}

void Notification::leaveEvent(QEvent *event)
{
	if (inDialog) {
		QWidget::leaveEvent(event);
		return;
	}

	startShowingTimer();
	QWidget::leaveEvent(event);
}

void Notification::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	onResize();
}
