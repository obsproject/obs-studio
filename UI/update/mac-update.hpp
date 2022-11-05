#ifndef MAC_UPDATER_H
#define MAC_UPDATER_H

#include <string>

#include <QThread>
#include <QString>
#include <QObject>

class QAction;

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

#ifdef __OBJC__
@class OBSUpdateDelegate;
#endif

class OBSSparkle : public QObject {
	Q_OBJECT

public:
	OBSSparkle(const char *branch, QAction *checkForUpdatesAction);
	void setBranch(const char *branch);
	void checkForUpdates(bool manualCheck);

private:
#ifdef __OBJC__
	OBSUpdateDelegate *updaterDelegate;
#else
	void *updaterDelegate;
#endif
};

#endif
