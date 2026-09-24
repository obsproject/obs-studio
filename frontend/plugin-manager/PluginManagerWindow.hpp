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

#include "PluginManager.hpp"

#include <QDialog>
#include <QWidget>

#include "ui_PluginManagerWindow.h"

namespace OBS {

class PluginManagerWindow : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui::PluginManagerWindow> ui;

public:
	enum class Status { Invalid = 0, Loadable, Error, Missing };

	struct Entry {
		OBS::ModuleInfo *module{nullptr};
		QString name{};
		Status status{Status::Invalid};
		bool isLegacy{false};
	};

	enum class Page { Installed };

	explicit PluginManagerWindow(std::vector<ModuleInfo> const &modules,
				     std::vector<std::string> const &failedModules, QWidget *parent = nullptr);

	std::vector<ModuleInfo> const getModules() { return modules_; }
	bool isEnabledPluginsChanged();

	void setPage(Page page);

private:
	std::vector<ModuleInfo> modules_;
	std::vector<Entry> installedPluginEntries;

	void setupInstalledPage(std::vector<std::string> failedModules);

	void sectionSelectionChanged();
	QPersistentModelIndex activeSectionIndex;
	void setSection(int sidebarRow);
};

}; // namespace OBS
