/******************************************************************************
    Copyright (C) 2022-2025 pkv <pkv@obsproject.com>

    This file is part of win-asio.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
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
#include <QLabel>

#include "./forms/ui_output.h"

#include <util/platform.h>

class ASIOSettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit ASIOSettingsDialog(QWidget *parent = 0, obs_output_t *output = nullptr, OBSData settings = nullptr);
	std::unique_ptr<Ui_Output> ui;
	void ShowHideDialog(bool enabled);
	void SetupPropertiesView(bool enabled);
	void SaveSettings();
	OBSData settings_;
	obs_output_t *output_;
	std::string currentDeviceName;

public slots:
	void PropertiesChanged();

private:
	OBSPropertiesView *propertiesView;
};
