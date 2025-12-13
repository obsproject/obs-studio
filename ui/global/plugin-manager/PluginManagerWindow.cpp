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

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>

#include "moc_PluginManagerWindow.cpp"

namespace OBS {

PluginManagerWindow::PluginManagerWindow(std::vector<ModuleInfo> const &modules, QWidget *parent)
	: QDialog(parent),
	  modules_(modules),
	  ui(new Ui::PluginManagerWindow)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->modulesListContainer->viewport()->setAutoFillBackground(false);
	ui->modulesListContents->setAutoFillBackground(false);

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

	setSection(ui->sectionList->indexFromItem(installed));

	std::sort(modules_.begin(), modules_.end(), [](const ModuleInfo &a, const ModuleInfo &b) {
		std::string aName = !a.display_name.empty() ? a.display_name : a.module_name;
		std::string bName = !b.display_name.empty() ? b.display_name : b.module_name;
		return aName < bName;
	});

	int row = 0;
	for (auto &metadata : modules_) {
		std::string id = metadata.module_name;
		// Check if the module is missing:
		bool missing = !obs_get_module(id.c_str()) && !obs_get_disabled_module(id.c_str());

		QString name = !metadata.display_name.empty() ? metadata.display_name.c_str()
							      : metadata.module_name.c_str();
		if (missing) {
			name += " " + QTStr("PluginManager.MissingPlugin");
		}

		auto item = new QCheckBox(name);
		item->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
		item->setChecked(metadata.enabled);

		if (!metadata.enabledAtLaunch) {
			item->setProperty("class", "text-muted");
		}

		if (missing) {
			item->setEnabled(false);
		}
		ui->modulesList->layout()->addWidget(item);

		connect(item, &QCheckBox::toggled, this, [this, row](bool checked) {
			modules_[row].enabled = checked;
			ui->manageRestartLabel->setVisible(isEnabledPluginsChanged());
		});

		row++;
	}

	ui->modulesList->adjustSize();
	ui->modulesListContents->adjustSize();

	ui->manageRestartLabel->setVisible(isEnabledPluginsChanged());

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PluginManagerWindow::sectionSelectionChanged()
{
	auto selected = ui->sectionList->selectedItems();
	if (selected.count() != 1) {
		setSection(activeSectionIndex);
	} else {
		auto selectionIndex = ui->sectionList->indexFromItem(selected.first());
		setSection(selectionIndex);
	}
}

void PluginManagerWindow::setSection(QPersistentModelIndex index)
{
	if (ui->sectionList->itemFromIndex(index)) {
		activeSectionIndex = index;
		ui->sectionList->setCurrentIndex(index);
	}
}

bool PluginManagerWindow::isEnabledPluginsChanged()
{
	bool result = false;
	for (auto &metadata : modules_) {
		if (metadata.enabledAtLaunch != metadata.enabled) {
			result = true;
			break;
		}
	}

	return result;
}

}; // namespace OBS
