#pragma once

#include "ui_OBSBasicVCamConfig.h"

#include <utility/VCamConfig.hpp>

#include <QDialog>

class OBSBasicVCamConfig : public QDialog {
	Q_OBJECT

	VCamConfig config;

	bool vcamActive;
	VCamOutputType activeType;
	bool requireRestart;

public:
	explicit OBSBasicVCamConfig(const VCamConfig &config, bool VCamActive, QWidget *parent = 0);

private slots:
	void OutputTypeChanged();
	void UpdateConfig();

private:
	std::unique_ptr<Ui::OBSBasicVCamConfig> ui;

signals:
	void Accepted(const VCamConfig &config);
	void AcceptedAndRestart(const VCamConfig &config);
};
