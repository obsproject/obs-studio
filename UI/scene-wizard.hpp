#pragma once

#include <QDialog>
#include <QPointer>
#include "ui_OBSSceneWizard.h"
#include "window-basic-main.hpp"
#include "qt-display.hpp"
#include "volume-control.hpp"
#include "mute-checkbox.hpp"

class SceneWizard : public QDialog {
	Q_OBJECT

public:
	SceneWizard(QWidget *parent = nullptr);
	~SceneWizard();

	OBSSource webcam = nullptr;
	OBSSource mic = nullptr;
	OBSSource desktopAudio = nullptr;
	OBSSource cam = nullptr;
	QPointer<OBSQTDisplay> preview;

private:
	std::unique_ptr<Ui::SceneWizard> ui;
	void InitTemplates();
	void UpdateSceneCollection();
	void LoadDevices();
	void SaveMicrophone();
	void SaveDesktopAudio();
	void SaveWebcam();
	void RemoveWebcam();

	QPointer<VolControl> micVol;
	QPointer<VolControl> desktopVol;

	static void OBSRender(void *data, uint32_t cx, uint32_t cy);

private slots:
	void on_templateButton_clicked();
	void on_importButton_clicked();
	void on_setupOnOwnButton_clicked();

	void on_back_clicked();
	void on_next_clicked();
	void on_desktopAudioCombo_currentIndexChanged(int index);
	void on_micCombo_currentIndexChanged(int index);
	void on_webcamCombo_currentIndexChanged(int index);
	void on_disableWebcam_toggled(bool checked);
	void on_disableMic_toggled(bool checked);
	void on_disableDesktopAudio_toggled(bool checked);

	void GoToOptionsPage();
	void GoToTemplatePage();
	void GoToDevicesPage();

protected:
	virtual void closeEvent(QCloseEvent *event);
};
