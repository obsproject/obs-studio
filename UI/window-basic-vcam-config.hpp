#pragma once

#include <obs.hpp>
#include <QDialog>
#include <memory>

#include "window-basic-vcam.hpp"

#include "ui_OBSBasicVCamConfig.h"

struct VCamConfig;

class OBSBasicVCamConfig : public QDialog {
	Q_OBJECT

	VCamConfig config;

public:
	explicit OBSBasicVCamConfig(const VCamConfig &config,
				    QWidget *parent = 0);

private slots:
	void OutputTypeChanged(int type);
	void UpdateConfig();

private:
	std::unique_ptr<Ui::OBSBasicVCamConfig> ui;

signals:
	void Accepted(const VCamConfig &config);
};
