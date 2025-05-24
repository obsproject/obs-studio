#include "ObsPluginManager.hpp"

#include <OBSApp.hpp>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QScrollArea>
#include <QDialogButtonBox>
#include <QLabel>

#include "moc_OBSPluginManager.cpp"
OBSPluginManager::OBSPluginManager(std::vector<OBSModuleInfo> const& modules, QWidget* parent)
	: QDialog(parent), _modules(modules), ui(new Ui::OBSPluginManager)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	std::sort(_modules.begin(), _modules.end(), [](const OBSModuleInfo& a, const OBSModuleInfo& b) {
		auto aName = a.display_name != "" ? a.display_name : a.module_name;
		auto bName = b.display_name != "" ? b.display_name : b.module_name;
		return aName < bName;
	});

	for (auto& metadata : _modules) {
		std::string name = metadata.display_name != "" ? metadata.display_name : metadata.module_name;
		auto item = new QListWidgetItem(name.c_str());
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(metadata.enabled ? Qt::Checked : Qt::Unchecked);
		ui->modulesList->addItem(item);
	}

	connect(ui->modulesList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
		auto row = ui->modulesList->row(item);
		bool checked = item->checkState() == Qt::Checked;
		_modules[row].enabled = checked;
	});

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
