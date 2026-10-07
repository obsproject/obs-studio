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

#include "PluginManagerWindow.hpp"

#include <OBSApp.hpp>
#include <plugin-manager/InstalledPluginRow.hpp>

#include <Idian/ListHeader.hpp>
#include <Idian/RowList.hpp>
#include <Idian/Utils.hpp>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include "moc_PluginManagerWindow.cpp"

extern bool safe_mode;

namespace {
using Category = OBS::PluginManagerWindow::Category;
constexpr int getCategorySortOrder(Category c)
{
	switch (c) {
	case Category::Installed:
		return 0;
	case Category::Error:
		return 1;
	case Category::Missing:
		return 2;
	default:
		return 3;
	}
}
} // namespace

namespace OBS {
PluginManagerWindow::PluginManagerWindow(std::vector<ModuleInfo> const &modules,
					 std::vector<std::string> const &failedModules, QWidget *parent)

	: QDialog(parent),
	  modules_(modules),
	  ui(new Ui::PluginManagerWindow)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	// Set up sidebar entries
	ui->sectionList->clear();
	ui->sectionList->setSelectionMode(QAbstractItemView::SingleSelection);

	connect(ui->sectionList, &QListWidget::itemSelectionChanged, this,
		&PluginManagerWindow::sectionSelectionChanged);

	QListWidgetItem *browse = new QListWidgetItem(QTStr("PluginManager.Section.Discover"));
	browse->setFlags(browse->flags() & ~Qt::ItemIsEnabled);
	browse->setFlags(browse->flags() & ~Qt::ItemIsSelectable);
	browse->setToolTip(QTStr("ComingSoon"));
	ui->sectionList->addItem(browse);

	QListWidgetItem *installed = new QListWidgetItem(QTStr("PluginManager.Section.Manage"));
	ui->sectionList->addItem(installed);

	QListWidgetItem *updates = new QListWidgetItem(QTStr("PluginManager.Section.Updates"));
	updates->setFlags(updates->flags() & ~Qt::ItemIsEnabled);
	updates->setFlags(updates->flags() & ~Qt::ItemIsSelectable);
	updates->setToolTip(QTStr("ComingSoon"));
	ui->sectionList->addItem(updates);

	setupInstalledPage(failedModules);

	setPage(Page::Installed);

	ui->manageRestartLabel->setVisible(isEnabledPluginsChanged());

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, this, &QDialog::close);
}

void PluginManagerWindow::setupInstalledPage(std::vector<std::string> failedModules)
{
	ui->modulesListContainer->viewport()->setAutoFillBackground(false);
	ui->modulesListOuter->layout()->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	ui->modulesListOuter->setAutoFillBackground(false);

	// Set up Idian sections
	auto installedPluginList = new idian::RowList(ui->modulesList);
	auto installedHeader = new idian::ListHeader(installedPluginList, QTStr("PluginManager.Section.Manage.Title"),
						     QTStr("PluginManager.Section.Manage.Description"));
	installedPluginList->addHeader(installedHeader);

	auto errorPluginList = new idian::RowList(ui->modulesList);
	auto errorHeader = new idian::ListHeader(errorPluginList, QTStr("PluginManager.Section.Errors.Title"),
						 QTStr("PluginManager.Section.Errors.Description"));
	errorPluginList->addHeader(errorHeader);

	auto missingPluginList = new idian::RowList(ui->modulesList);
	auto missingHeader = new idian::ListHeader(missingPluginList, QTStr("PluginManager.Section.Missing.Title"),
						   QTStr("PluginManager.Section.Missing.Description"));
	missingPluginList->addHeader(missingHeader);

	bool isSafeMode = obs_frontend_is_safe_mode_enabled();
	if (isSafeMode) {
		auto *safeModeFrame = new QFrame{};
		safeModeFrame->setLayout(new QVBoxLayout);
		safeModeFrame->layout()->setContentsMargins(0, 0, 0, 0);
		safeModeFrame->setProperty("class", "frame-notice");

		auto *safeModeLabel = new QLabel(ui->modulesList);
		safeModeLabel->setText(QTStr("PluginManager.SafeMode"));
		safeModeLabel->setAlignment(Qt::AlignCenter);
		safeModeLabel->setIndent(0);

		safeModeFrame->layout()->addWidget(safeModeLabel);
		ui->installedPage->layout()->addWidget(safeModeFrame);
	}

	ui->modulesList->layout()->addWidget(installedPluginList);
	ui->modulesList->layout()->addWidget(errorPluginList);
	ui->modulesList->layout()->addWidget(missingPluginList);

	installedPluginEntries.reserve(modules_.size() + failedModules.size());
	for (auto &metadata : modules_) {
		std::string_view module_name{metadata.module_name};

		// Check if the module is missing:
		obs_module_t *moduleData = obs_get_module(module_name.data());

		Category category{Category::Installed};

		bool isLoaded = moduleData != nullptr;
		if (!isLoaded) {
			category = Category::Missing;
			moduleData = obs_get_disabled_module(module_name.data());
		}

		bool isDisabled = moduleData != nullptr;
		if (isDisabled) {
			// This module is disabled but check if it also failed to load.
			auto failedModuleIterator = std::find(failedModules.begin(), failedModules.end(), module_name);
			if (failedModuleIterator != failedModules.end()) {
				// This module is in the plugin manager cache so it has loaded properly before but now failed.
				// Remove entry from the failedModules list so we don't create a dummy entry for it.
				category = Category::Error;
				failedModules.erase(failedModuleIterator);
			} else {
				// Module is disabled but did not fail to load.
				category = Category::Installed;
			}
		}

		bool isLegacyModule = obs_is_legacy_module(moduleData);

		QString name = !metadata.display_name.empty() ? metadata.display_name.c_str()
							      : metadata.module_name.c_str();
		QString version = !metadata.version.empty() ? metadata.version.c_str() : "";

		Entry newEntry{metadata, name, category, isLegacyModule};
		installedPluginEntries.push_back(newEntry);
	}

	for (const std::string &moduleName : failedModules) {
		// This failed module is not in the plugin manager cache which means it has never been loaded successfully.
		// Create a dummy visual entry for it.
		OBS::ModuleInfo dummyModule;
		QString name = QString::fromStdString(moduleName.data());
		Category category{Category::Error};
		bool isLegacy{false};
		bool hasLoadedBefore{false};

		dummyModule.module_name = moduleName;
		dummyModule.enabled = false;
		dummyModule.enabledAtLaunch = false;

		Entry newEntry{dummyModule, name, category, isLegacy, hasLoadedBefore};
		installedPluginEntries.push_back(newEntry);
	}

	std::sort(installedPluginEntries.begin(), installedPluginEntries.end(), [](const Entry &a, const Entry &b) {
		if (a.category != b.category) {
			return getCategorySortOrder(a.category) < getCategorySortOrder(b.category);
		}

		return a.name.toLower() < b.name.toLower();
	});

	QWidget *previousRow{nullptr};
	for (Entry &entry : installedPluginEntries) {
		if (isSafeMode) {
			entry.category = Category::Installed;
		}
		auto newRow = new InstalledPluginRow(ui->modulesList, entry);

		if (!previousRow) {
			setTabOrder(ui->modulesListContainer, newRow);
		} else {
			setTabOrder(previousRow, newRow);
		}

		previousRow = newRow;

		if (entry.category == Category::Installed) {
			installedPluginList->addRow(newRow);
		} else if (entry.category == Category::Error) {
			errorPluginList->addRow(newRow);
		} else if (entry.category == Category::Missing) {
			missingPluginList->addRow(newRow);

			connect(newRow, &InstalledPluginRow::trashClicked, this, [this, entry]() {
				auto it = std::find_if(modules_.begin(), modules_.end(),
						       [&entry](const ModuleInfo &module) {
							       return module.module_name == entry.module.module_name;
						       });

				if (it != modules_.end()) {
					modules_.erase(it);
				}
			});
		}

		connect(newRow, &InstalledPluginRow::toggleChanged, this, [this, entry](bool enabled) {
			auto it = std::find_if(modules_.begin(), modules_.end(), [&entry](const ModuleInfo &module) {
				return module.module_name == entry.module.module_name;
			});

			if (it != modules_.end()) {
				it->enabled = enabled;
			}

			ui->manageRestartLabel->setVisible(isEnabledPluginsChanged());
		});
	}

	setTabOrder(previousRow, ui->buttonBox);

	if (installedPluginList->count() == 0) {
		installedHeader->setDescription(QTStr("PluginManager.Section.Manage.NoPlugins"));
	}

	if (errorPluginList->count() == 0) {
		errorPluginList->setVisible(false);
	}

	if (missingPluginList->count() == 0) {
		missingPluginList->setVisible(false);
	}

	// Qt is weird about how styling from dynamic properties affects certain widgets such as scroll areas.
	// This forces a recalculation after the items have all been added.
	// TODO: https://github.com/obsproject/obs-studio/issues/13920
	// Rip this out after #13920 has been done.
	{
		ui->modulesList->style()->polish(ui->modulesList);
		QEvent event(QEvent::StyleChange);
		QApplication::sendEvent(ui->modulesList, &event);
	}
}

void PluginManagerWindow::sectionSelectionChanged()
{
	auto selected = ui->sectionList->selectedItems();
	if (selected.count() != 1) {
		setSection(activeSectionIndex.row());
	} else {
		auto selectionIndex = ui->sectionList->indexFromItem(selected.first());
		setSection(selectionIndex.row());
	}
}

void PluginManagerWindow::setSection(int sidebarRow)
{
	if (auto item = ui->sectionList->item(sidebarRow)) {
		activeSectionIndex = ui->sectionList->indexFromItem(item);
		ui->sectionList->setCurrentIndex(activeSectionIndex);

		ui->stackedContents->setCurrentIndex(sidebarRow);
	}
}

bool PluginManagerWindow::isEnabledPluginsChanged()
{
	if (obs_frontend_is_safe_mode_enabled()) {
		return false;
	}

	bool result = false;
	for (auto &entry : installedPluginEntries) {
		// Only prompt for restart when a loadable plugin entry changed.
		if (entry.category == Category::Installed && entry.module.enabledAtLaunch != entry.module.enabled) {
			result = true;
			break;
		}
	}

	return result;
}

void PluginManagerWindow::setPage(Page page)
{
	switch (page) {
	case Page::Installed:
		setSection(1);
		break;
	default:
		break;
	}
}
}; // namespace OBS
