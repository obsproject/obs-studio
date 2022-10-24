#pragma once

#include <memory>

#include <QFrame>
#include <QPointer>

class OBSBasic;
class OBSBasicControlsButton;

#include "ui_OBSBasicControls.h"

class OBSBasicControls : public QFrame {
	Q_OBJECT

	OBSBasic *main;
	std::unique_ptr<Ui::OBSBasicControls> ui;

	QPointer<QMenu> streamButtonMenu;

	QScopedPointer<QPushButton> pauseButton;

	QPointer<QHBoxLayout> replayBufferLayout;
	QPointer<OBSBasicControlsButton> replayBufferButton;
	QScopedPointer<QPushButton> saveReplayButton;

	QPointer<QHBoxLayout> vcamButtonsLayout;
	QPointer<OBSBasicControlsButton> vcamButton;
	QPointer<QPushButton> vcamConfigButton;

public:
	OBSBasicControls(OBSBasic *main);
	inline ~OBSBasicControls() {}

private:
	void SetupReplayBufferButton(bool remove);

private slots:
	void ResizePauseButton();
	void ResizeSaveReplayButton();
	void ResizeVcamConfigButton();

	void SetStreamingStoppedState(bool add_menu);
	void SetStartingStreamingState(bool broadcast_auto_start);
	void SetStoppingStreamingState();
	void SetStreamingStartedState(bool add_menu);

	void SetBroadcastStartedState(bool auto_stop);
	void SetBroadcastFlowEnabled(bool enabled, bool ready);

	void SetRecordingStoppedState();
	void SetStoppingRecordingState();
	void SetRecordingStartedState(bool add_pause_button);

	void PauseToUnpauseButton();
	void UnpauseToPauseButton();

	void SetReplayBufferStartedState();
	void SetReplayBufferStoppingState();
	void SetReplayBufferStoppedState();

	void EnableVCamButtons();
	void SetVCamStoppedState();
	void SetVCamStartedState();
};
