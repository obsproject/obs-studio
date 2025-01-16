/******************************************************************************
    Copyright (C) 2019-2020 by Dillon Pentz <dillon@vodbox.io>

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

#include "OBSImporter.hpp"
#include "ImporterEntryPathItemDelegate.hpp"
#include "ImporterModel.hpp"

#include <importers/importers.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QDirIterator>
#include <QDropEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>

#include "moc_OBSImporter.cpp"
OBSImporter::OBSImporter(QWidget *parent) : QDialog(parent), optionsModel(new ImporterModel), ui(new Ui::OBSImporter)
{
	setAcceptDrops(true);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->tableView->setModel(optionsModel);
	ui->tableView->setItemDelegateForColumn(ImporterColumn::Path, new ImporterEntryPathItemDelegate());
	ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
	ui->tableView->horizontalHeader()->setSectionResizeMode(ImporterColumn::Path, QHeaderView::ResizeMode::Stretch);

	connect(optionsModel, &ImporterModel::dataChanged, this, &OBSImporter::dataChanged);

	ui->tableView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged);

	ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("Import"));
	ui->buttonBox->button(QDialogButtonBox::Open)->setText(QTStr("Add"));

	connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
		&OBSImporter::importCollections);
	connect(ui->buttonBox->button(QDialogButtonBox::Open), &QPushButton::clicked, this, &OBSImporter::browseImport);
	connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &OBSImporter::close);

	ImportersInit();

	bool autoSearchPrompt = config_get_bool(App()->GetUserConfig(), "General", "AutoSearchPrompt");

	if (!autoSearchPrompt) {
		QMessageBox::StandardButton button = OBSMessageBox::question(
			parent, QTStr("Importer.AutomaticCollectionPrompt"), QTStr("Importer.AutomaticCollectionText"));

		if (button == QMessageBox::Yes) {
			config_set_bool(App()->GetUserConfig(), "General", "AutomaticCollectionSearch", true);
		} else {
			config_set_bool(App()->GetUserConfig(), "General", "AutomaticCollectionSearch", false);
		}

		config_set_bool(App()->GetUserConfig(), "General", "AutoSearchPrompt", true);
	}

	bool autoSearch = config_get_bool(App()->GetUserConfig(), "General", "AutomaticCollectionSearch");

	OBSImporterFiles f;
	if (autoSearch)
		f = ImportersFindFiles();

	for (size_t i = 0; i < f.size(); i++) {
		QString path = f[i].c_str();
		path.replace("\\", "/");
		addImportOption(path, true);
	}

	f.clear();

	ui->tableView->resizeColumnsToContents();

	QModelIndex index = optionsModel->createIndex(optionsModel->rowCount() - 1, 2);
	QMetaObject::invokeMethod(ui->tableView, "setCurrentIndex", Qt::QueuedConnection,
				  Q_ARG(const QModelIndex &, index));
}

void OBSImporter::addImportOption(QString path, bool automatic)
{
	QStringList list;

	list.append(path);

	QModelIndex insertIndex = optionsModel->index(optionsModel->rowCount() - 1, ImporterColumn::Path);

	optionsModel->setData(insertIndex, list, automatic ? ImporterEntryRole::AutoPath : ImporterEntryRole::NewPath);
}

void OBSImporter::dropEvent(QDropEvent *ev)
{
	for (QUrl url : ev->mimeData()->urls()) {
		QFileInfo fileInfo(url.toLocalFile());
		if (fileInfo.isDir()) {

			QDirIterator dirIter(fileInfo.absoluteFilePath(), QDir::Files);

			while (dirIter.hasNext()) {
				addImportOption(dirIter.next(), false);
			}
		} else {
			addImportOption(fileInfo.canonicalFilePath(), false);
		}
	}
}

void OBSImporter::dragEnterEvent(QDragEnterEvent *ev)
{
	if (ev->mimeData()->hasUrls())
		ev->accept();
}

void OBSImporter::browseImport()
{
	QString Pattern = "(*.json *.bpres *.xml *.xconfig)";

	QStringList paths = OpenFiles(this, QTStr("Importer.SelectCollection"), "",
				      QTStr("Importer.Collection") + QString(" ") + Pattern);

	if (!paths.empty()) {
		for (int i = 0; i < paths.count(); i++) {
			addImportOption(paths[i], false);
		}
	}
}

bool GetUnusedName(std::string &name)
{
	OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	if (!basic->GetSceneCollectionByName(name)) {
		return false;
	}

	std::string newName;
	int inc = 2;
	do {
		newName = name;
		newName += " ";
		newName += std::to_string(inc++);
	} while (basic->GetSceneCollectionByName(newName));

	name = newName;
	return true;
}

constexpr std::string_view OBSSceneCollectionPath = "obs-studio/basic/scenes/";

void OBSImporter::importCollections()
{
	setEnabled(false);

	const std::filesystem::path sceneCollectionLocation =
		App()->userScenesLocation / std::filesystem::u8path(OBSSceneCollectionPath);

	for (int i = 0; i < optionsModel->rowCount() - 1; i++) {
		int selected = optionsModel->index(i, ImporterColumn::Selected).data(Qt::CheckStateRole).value<int>();

		if (selected == Qt::Unchecked)
			continue;

		std::string pathStr = optionsModel->index(i, ImporterColumn::Path)
					      .data(Qt::DisplayRole)
					      .value<QString>()
					      .toStdString();
		std::string nameStr = optionsModel->index(i, ImporterColumn::Name)
					      .data(Qt::DisplayRole)
					      .value<QString>()
					      .toStdString();

		json11::Json res;
		ImportSC(pathStr, nameStr, res);

		if (res != json11::Json()) {
			json11::Json::object out = res.object_items();
			std::string name = res["name"].string_value();
			std::string file;

			if (GetUnusedName(name)) {
				json11::Json::object newOut = out;
				newOut["name"] = name;
				out = newOut;
			}

			std::string fileName;
			if (!GetFileSafeName(name.c_str(), fileName)) {
				blog(LOG_WARNING, "Failed to create safe file name for '%s'", fileName.c_str());
			}

			std::string collectionFile;
			collectionFile.reserve(sceneCollectionLocation.u8string().size() + fileName.size());
			collectionFile.append(sceneCollectionLocation.u8string()).append(fileName);

			if (!GetClosestUnusedFileName(collectionFile, "json")) {
				blog(LOG_WARNING, "Failed to get closest file name for %s", fileName.c_str());
			}

			std::string out_str = json11::Json(out).dump();

			bool success = os_quick_write_utf8_file(collectionFile.c_str(), out_str.c_str(), out_str.size(),
								false);

			blog(LOG_INFO, "Import Scene Collection: %s (%s) - %s", name.c_str(), fileName.c_str(),
			     success ? "SUCCESS" : "FAILURE");
		}
	}

	close();
}

void OBSImporter::dataChanged()
{
	ui->tableView->resizeColumnToContents(ImporterColumn::Name);
}
