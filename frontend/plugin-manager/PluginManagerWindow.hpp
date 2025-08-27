/******************************************************************************
    Copyright (C) 2025 by FiniteSingularity <finitesingularityttv@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "ui_PluginManagerWindow.h"
#include "PluginManager.hpp"

#include <QDialog>
#include <QWidget>

namespace OBS {

class PluginManagerWindow : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui::PluginManagerWindow> ui;

public:
	explicit PluginManagerWindow(std::vector<ModuleInfo> const &modules, QWidget *parent = nullptr);
	inline std::vector<ModuleInfo> const result() { return modules_; }

private:
	std::vector<ModuleInfo> modules_;
};

}; // namespace OBS
