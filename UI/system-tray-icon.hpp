#pragma once

#include <QSystemTrayIcon>
#include <QPointer>

class QAction;
class OBSBasic;

class OBSSystemTrayIcon : public QSystemTrayIcon {
	Q_OBJECT

private:
	QScopedPointer<QMenu> menu;
	QPointer<QAction> showHide;
	QPointer<QAction> stream;
	QPointer<QAction> record;
	QPointer<QAction> replay;
	QPointer<QAction> virtualCam;
	QScopedPointer<QMenu> previewProjector;
	QScopedPointer<QMenu> studioProgramProjector;

	void OnActivate(bool force = false);
	void OnDeactivate();

private slots:
	void VisibilityChanged(bool visible);
	void IconActivated(QSystemTrayIcon::ActivationReason reason);

	void StreamingPreparing();
	void StreamingStarting(bool broadcastAutoStart);
	void StreamingStarted(bool delay);
	void StreamingStopping();
	void StreamingStopped(bool delay);

	void RecordingStarted(bool pausible);
	void RecordingStopping();
	void RecordingStopped();
	void RecordingPaused();
	void RecordingUnpaused();

	void ReplayBufferStarted();
	void ReplayBufferStopping();
	void ReplayBufferStopped();

	void VirtualCamStarted();
	void VirtualCamStopped();

	void UpdateReplayBuffer(bool enable);
	void EnableVirtualCam();

public:
	OBSSystemTrayIcon(OBSBasic *main);
	~OBSSystemTrayIcon();

	void ShowNotification(const QString &text,
			      QSystemTrayIcon::MessageIcon n);

signals:
	void VisibilityActionTriggered();
	void StreamActionTriggered();
	void RecordActionTriggered();
	void ReplayBufferActionTriggered();
	void VirtualCamActionTriggered();
};
