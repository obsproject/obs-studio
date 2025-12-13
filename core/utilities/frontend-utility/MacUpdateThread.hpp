#pragma once

#include <QThread>

class MacUpdateThread : public QThread {
	Q_OBJECT

	bool manualUpdate;

	virtual void run() override;

	void info(const QString &title, const QString &text);

signals:
	void Result(const QString &branch, bool manual);

private slots:
	void infoMsg(const QString &title, const QString &text);

public:
	MacUpdateThread(bool manual) : manualUpdate(manual) {}
};
