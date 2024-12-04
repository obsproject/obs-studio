#pragma once

#import <QObject>

class QAction;
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
