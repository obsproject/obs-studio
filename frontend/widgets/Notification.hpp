#pragma once

#include <QFrame>
#include <QDateTime>
#include <QPushButton>
#include "ui_Notification.h"

class QShowEvent;
class QTimer;
class Notification;

class Notification : public QFrame {
	Q_OBJECT

private:
	std::unique_ptr<Ui_Notification> ui;

	QTimer *updateTimer = nullptr;
	QScopedPointer<QTimer> showingTimer;

	qint64 timestamp;
	QString uuid;

	bool read = false;
	bool important = false;
	bool inDialog = false;

	void resetShowingTimer();

private slots:
	void onResize();
	void updateTimeLabel();
	void on_closeButton_clicked();

public:
	explicit Notification(const QString &iconName, const QString &title, const QString &text, bool important_,
			      QWidget *parent = nullptr);
	~Notification();

	bool isRead();

	void startShowingTimer();
	void setNotificationIsInDialog();

	inline bool isImportant() { return important; };
	inline QString getUUID() const { return uuid; };

	template<typename Receiver, typename... Args>
	inline void addAction(const QString &actionText, Receiver *target, void (Receiver::*slot)(Args...))
	{
		QPushButton *button = new QPushButton(actionText, this);
		connect(button, &QPushButton::clicked, target, slot);
		ui->buttonLayout->addWidget(button);
	}

	template<typename Receiver, typename Lambda>
	inline void addAction(const QString &actionText, Receiver *target, Lambda lambda)
	{
		QPushButton *button = new QPushButton(actionText, this);
		connect(button, &QPushButton::clicked, target, lambda);
		ui->buttonLayout->addWidget(button);
	}

public slots:
	void setAsRead();

protected:
	virtual void enterEvent(QEnterEvent *event) override;
	virtual void leaveEvent(QEvent *event) override;
	virtual void showEvent(QShowEvent *event) override;

signals:
	void timeout();
	void closed(Notification *notification);
};
