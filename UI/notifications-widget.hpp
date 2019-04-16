#pragma once

#include <QPointer>
#include <QFrame>
#include "obs.hpp"

class QLabel;
class QPushButton;

class OBSNotification : public QFrame {
	Q_OBJECT

private:
	void *data = nullptr;
	QString notifyUrl;

	uint32_t id;
	uint64_t timestamp;
	bool persist = false;

private slots:
	void LinkActivated();
	void CloseNotification();

public:
	OBSNotification(uint32_t id_, enum obs_notify_type type,
			const QString &message, bool persist_, void *data_,
			QWidget *parent = nullptr);

	uint32_t GetID();
};
