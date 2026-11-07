#pragma once

#include "ui_OBSBasicControls.h"

#include <QFrame>
#include <QPointer>
#include <QScopedPointer>

#include <memory>

class OBSBasic;
enum class SplitRecordingState; /* defined in OBSBasic.hpp */

class OBSBasicControls : public QFrame {
	Q_OBJECT

	std::unique_ptr<Ui::OBSBasicControls> ui;

	QScopedPointer<QMenu> streamButtonMenu;
	QPointer<QAction> startStreamAction;
	QPointer<QAction> stopStreamAction;

	bool recordingPaused = false;
	SplitRecordingState splitState{}; /* Idle */
	QString splitDetail;
	unsigned int splitStateGeneration = 0;
	void UpdateSplitRecordButton();

private slots:
	void StreamingPreparing();
	void StreamingStarting(bool broadcastAutoStart);
	void StreamingStarted(bool withDelay);
	void StreamingStopping();
	void StreamingStopped(bool withDelay);

	void BroadcastStreamReady(bool ready);
	void BroadcastStreamActive();
	void BroadcastStreamStarted(bool autoStop);

	void RecordingStarted(bool pausable);
	void RecordingPaused();
	void RecordingUnpaused();
	void RecordingStopping();
	void RecordingStopped();
	void RecordingSplitAvailable(bool available);
	void RecordingSplitStateChanged(SplitRecordingState state, const QString &detail);

	void ReplayBufferStarted();
	void ReplayBufferStopping();
	void ReplayBufferStopped();

	void VirtualCamStarted();
	void VirtualCamStopped();

	void UpdateStudioModeState(bool enabled);

	void EnableBroadcastFlow(bool enabled);
	void EnableReplayBufferButtons(bool enabled);
	void EnableVirtualCamButtons();

public:
	OBSBasicControls(OBSBasic *main);
	inline ~OBSBasicControls() {}

signals:
	void StreamButtonClicked();
	void BroadcastButtonClicked();
	void RecordButtonClicked();
	void PauseRecordButtonClicked();
	void SplitRecordButtonClicked(QPoint menuPos);
	void ReplayBufferButtonClicked();
	void SaveReplayBufferButtonClicked();
	void VirtualCamButtonClicked();
	void VirtualCamConfigButtonClicked();
	void StudioModeButtonClicked();
	void SettingsButtonClicked();

	void StartStreamMenuActionClicked();
	void StopStreamMenuActionClicked();
	void ForceStopStreamMenuActionClicked();
};
