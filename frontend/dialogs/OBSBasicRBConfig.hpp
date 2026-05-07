#pragma once

#include "ui_OBSBasicRBConfig.h"

#include <utility/ReplayBufferConfig.hpp>

#include <QDialog>

class OBSBasicRBConfig : public QDialog {
	Q_OBJECT

	ReplayBufferConfig config;
	bool rbActive;

public:
	explicit OBSBasicRBConfig(const ReplayBufferConfig &config, bool rbActive, QWidget *parent = 0);

private slots:
	void OutputTypeChanged();
	void UpdateConfig();

private:
	std::unique_ptr<Ui::OBSBasicRBConfig> ui;

signals:
	void Accepted(const ReplayBufferConfig &config);
};
