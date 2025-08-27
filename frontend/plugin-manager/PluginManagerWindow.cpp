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

	std::sort(modules_.begin(), modules_.end(), [](const ModuleInfo &a, const ModuleInfo &b) {
		std::string aName = !a.display_name.empty() ? a.display_name : a.module_name;
		std::string bName = !b.display_name.empty() ? b.display_name : b.module_name;
		return aName < bName;
	});

	for (auto &metadata : modules_) {
		std::string id = metadata.module_name;
		// Check if the module is missing:
		bool missing = !obs_get_module(id.c_str()) && !obs_get_disabled_module(id.c_str());

		QString name = !metadata.display_name.empty() ? metadata.display_name.c_str()
							      : metadata.module_name.c_str();
		if (missing) {
			name += " " + QTStr("PluginManager.MissingPlugin");
		}
		auto item = new QListWidgetItem(name);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(metadata.enabled ? Qt::Checked : Qt::Unchecked);

		if (missing) {
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		}
		ui->modulesList->addItem(item);
	}

	connect(ui->modulesList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
		auto row = ui->modulesList->row(item);
		bool checked = item->checkState() == Qt::Checked;
		modules_[row].enabled = checked;
	});

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

}; // namespace OBS
