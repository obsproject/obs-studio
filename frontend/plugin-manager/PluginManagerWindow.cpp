#include "PluginManagerWindow.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QLabel>
#include <QString>
#include <OBSApp.hpp>

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
