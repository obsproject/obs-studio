#pragma once

#include <obs.hpp>
#include <QDialog>
#include <memory>

#include "ui_OBSBasicVCamConfig.h"

class OBSBasicVCamConfig : public QDialog {
	Q_OBJECT

public:
	static void Init();

	static video_t *StartVideo();
	static void StopVideo();
	static void UpdateOutputSource();

	explicit OBSBasicVCamConfig(QWidget *parent = 0);

private slots:
	void OutputTypeChanged(int type);
	void Save();

private:
	std::unique_ptr<Ui::OBSBasicVCamConfig> ui;
};
