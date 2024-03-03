/*  Copyright (c) 2022-2025 pkv <pkv@obsproject.com>
 *
 * This file is part of win-asio.
 * 
 * This file and all modifications by pkv <pkv@obsproject.com> are licensed under
 * the GNU General Public License, version 3 or later.
 */
#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

#pragma once
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.hpp>
#include <properties-view.hpp>

#include <QDialog>
#include <QAction>
#include <QMainWindow>

#include "./forms/ui_output.h"

#include <util/platform.h>

class ASIOSettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit ASIOSettingsDialog(QWidget *parent = 0, obs_output_t *output = nullptr, OBSData settings = nullptr);
	std::unique_ptr<Ui_Output> ui;
	void ShowHideDialog();
	void SetupPropertiesView();
	void SaveSettings();
	OBSData _settings;
	obs_output_t *_output;
	std::string currentDeviceName;

public slots:
	void PropertiesChanged();

private:
	OBSPropertiesView *propertiesView;
};
