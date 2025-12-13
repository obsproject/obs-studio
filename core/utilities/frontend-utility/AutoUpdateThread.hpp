#pragma once

#include <QThread>

class AutoUpdateThread : public QThread {
	Q_OBJECT

	bool manualUpdate;
	bool repairMode;
	bool user_confirmed = false;

	virtual void run() override;

	void info(const QString &title, const QString &text);
	int queryUpdate(bool manualUpdate, const char *text_utf8);
	bool queryRepair();

private slots:
	void infoMsg(const QString &title, const QString &text);
	int queryUpdateSlot(bool manualUpdate, const QString &text);
	bool queryRepairSlot();

public:
	AutoUpdateThread(bool manualUpdate_, bool repairMode_ = false)
		: manualUpdate(manualUpdate_),
		  repairMode(repairMode_)
	{
	}
};
