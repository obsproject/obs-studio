/******************************************************************************
    Copyright (C) 2023 by Sebastian Beckmann

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

#include "window-scene-collections.hpp"

#include <obs-frontend-api.h>
#include <QMenu>
#include <QLabel>
#include <QDir>
#include <QShortcut>

#include <icon-label.hpp>

#include "window-basic-main.hpp"
#include "window-importer.hpp"
#include "window-namedialog.hpp"

#include "qt-wrappers.hpp"
#include "doubleclick-eventfilter.hpp"
#include "idian/obs-widgets.hpp"

OBSSceneCollections::OBSSceneCollections(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::OBSSceneCollections)
{
	ui->setupUi(this);

	const char *order = config_get_string(
		App()->GlobalConfig(), "SceneCollectionsWindow", "Order");
	if (!order)
		order = "LastUsed";

	if (strcmp(order, "Name") == 0) {
		collections_order = kSceneCollectionOrderName;
	} else {
		collections_order = kSceneCollectionOrderLastUsed;
	}

	refreshList();
	connect(this, &OBSSceneCollections::collectionsChanged,
		[this]() { refreshList(); });

	QShortcut *shortcut =
		new QShortcut(QKeySequence("Ctrl+L"), ui->buttonBulkMode);
	connect(shortcut, &QShortcut::activated, ui->buttonBulkMode,
		&QPushButton::click);

	setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSSceneCollections::on_lineeditSearch_textChanged(const QString &text)
{
	QString needle = text.toLower();
	for (auto row : ui->groupBox->properties()->rows()) {
		QString name = row->property("name").value<QString>().toLower();
		row->setVisible(name.contains(needle));
	}
}

void OBSSceneCollections::setBulkMode(bool bulk)
{
	if (ui->buttonBulkMode->isChecked() != bulk) {
		// This will run the method again.
		ui->buttonBulkMode->setChecked(bulk);
		return;
	}

	/* Make old buttons invisible first and then new buttons visible to prevent
	 * the window from resizing */
	if (bulk) {
		ui->buttonNew->setVisible(false);
		ui->buttonImport->setVisible(false);
		ui->buttonExportBulk->setVisible(true);
		ui->buttonDuplicateBulk->setVisible(true);
		ui->buttonDeleteBulk->setVisible(true);

		updateBulkButtons();
	} else {
		ui->buttonExportBulk->setVisible(false);
		ui->buttonDuplicateBulk->setVisible(false);
		ui->buttonDeleteBulk->setVisible(false);
		ui->buttonNew->setVisible(true);
		ui->buttonImport->setVisible(true);
	}

	for (auto child : ui->groupBox->properties()->rows()) {
		OBSActionRow *row = static_cast<OBSActionRow *>(child);
		if (bulk)
			row->setPrefixEnabled(true);
		else
			row->setSuffixEnabled(true);
	}
}

void OBSSceneCollections::on_buttonBulkMode_toggled(bool checked)
{
	setBulkMode(checked);
}

void OBSSceneCollections::updateBulkButtons()
{
	auto rows = selectedRows();
	if (rows.empty()) {
		ui->buttonExportBulk->setEnabled(false);
		ui->buttonDuplicateBulk->setEnabled(false);
		ui->buttonDeleteBulk->setEnabled(false);
	} else {
		ui->buttonExportBulk->setEnabled(true);
		ui->buttonDuplicateBulk->setEnabled(true);

		bool deleteDisabled = (rows.size() == 1) &&
				      rows.first().is_current_collection;
		ui->buttonDeleteBulk->setEnabled(!deleteDisabled);
	}
}

static constexpr int SECOND = 1;
static constexpr int MINUTE = 60 * SECOND;
static constexpr int HOUR = 60 * MINUTE;
static constexpr int DAY = 24 * HOUR;
static constexpr int WEEK = 7 * DAY;
// Yes, I'm aware we're making slightly inaccurate assumptions.
static constexpr int MONTH = 30 * DAY;
// This is also not 100% correct either, but probably good enough.
static constexpr int YEAR = 365 * DAY;

// Upper time limit, Translation key, Translation time divisor
static constexpr std::array<std::tuple<int, const char *, int>, 13> timeInfos = {
	std::make_tuple(1 * MINUTE, "LastUsed.JustNow", 1),
	std::make_tuple(2 * MINUTE, "LastUsed.Minute", MINUTE),
	std::make_tuple(1 * HOUR, "LastUsed.Minutes", MINUTE),
	std::make_tuple(2 * HOUR, "LastUsed.Hour", HOUR),
	std::make_tuple(1 * DAY, "LastUsed.Hours", HOUR),
	std::make_tuple(2 * DAY, "LastUsed.Day", DAY),
	std::make_tuple(1 * WEEK, "LastUsed.Days", DAY),
	std::make_tuple(2 * WEEK, "LastUsed.Week", WEEK),
	std::make_tuple(1 * MONTH, "LastUsed.Weeks", WEEK),
	std::make_tuple(2 * MONTH, "LastUsed.Month", MONTH),
	std::make_tuple(1 * YEAR, "LastUsed.Months", MONTH),
	std::make_tuple(2 * YEAR, "LastUsed.Year", YEAR),
	std::make_tuple(std::numeric_limits<int>::max(), "LastUsed.Years",
			YEAR),
};

static QString format_relative_time(time_t from)
{
	time_t now = time(0);
	int diff = (int)difftime(now, from);

	for (auto &[timeLimit, translationKey, divisor] : timeInfos) {
		if (diff < timeLimit) {
			return QTStr(translationKey).arg(diff / divisor);
		}
	}

	return QTStr("Unknown");
}

void OBSSceneCollections::refreshList()
{
	setBulkMode(false);
	ui->buttonDeleteBulk->setEnabled(true);

	ui->groupBox->properties()->clear();

	BPtr current_collection = obs_frontend_get_current_scene_collection();
	EnumSceneCollectionsOrdered(collections_order, [&](const char *name,
							   const char *file,
							   time_t last_used) {
		bool isCurrentCollection =
			(strcmp(current_collection, name) == 0);

		QString description =
			isCurrentCollection ? QTStr("LastUsed.CurrentlyActive")
					    : format_relative_time(last_used);
		OBSActionRow *row = new OBSActionRow();
		row->setTitle(name);
		row->setDescription(description);
		row->setProperty("name", name);
		row->setProperty("file", file);
		row->setProperty("current_collection", isCurrentCollection);

		QCheckBox *checkbox = new QCheckBox(row);
		row->setPrefix(checkbox);
		connect(checkbox, &QCheckBox::toggled, this,
			&OBSSceneCollections::updateBulkButtons);

		QPushButton *button = new QPushButton(row);
		button->setProperty("themeID", "menuIconSmall");
		button->setProperty("toolButton", true);
		connect(button, &QPushButton::clicked, this,
			[this, name = std::string(name),
			 file = std::string(file), isCurrentCollection]() {
				QMenu *menu = new QMenu(this);
				menu->setAttribute(Qt::WA_DeleteOnClose);
				QAction *openAction = menu->addAction(
					QTStr("SceneCollections.Open"), [&]() {
						OBSBasic *main =
							OBSBasic::Get();
						main->ChangeSceneCollection(
							file);
					});
				openAction->setEnabled(!isCurrentCollection);
				menu->addAction(
					QTStr("SceneCollections.Rename"),
					[this, name, file]() {
						SCRename(name, file);
					});
				menu->addAction(
					QTStr("SceneCollections.Duplicate"),
					[this, name, file]() {
						SCDuplicate(name, file);
					});
				menu->addAction(
					QTStr("SceneCollections.Export"),
					[this, file]() { SCExport(file); });
				menu->addSeparator();
				QAction *deleteAction = menu->addAction(
					QTStr("SceneCollections.Delete"),
					[this, name, file]() {
						SCDelete(name, file);
					});
				deleteAction->setEnabled(!isCurrentCollection);
				menu->popup(QCursor::pos());
			});

		QWidget *suffix;
		if (isCurrentCollection) {
			suffix = new QWidget(row);
			QHBoxLayout *layout = new QHBoxLayout(suffix);
			layout->setContentsMargins(0, 0, 0, 0);

			IconLabel *iconWidget = new IconLabel(suffix);
			iconWidget->setProperty("themeID", "checkmarkIcon");

			layout->addWidget(iconWidget);
			button->setParent(suffix);
			layout->addWidget(button);
			suffix->setLayout(layout);
		} else {
			suffix = button;

			DoubleClickFilter *filter = new DoubleClickFilter(row);
			row->installEventFilter(filter);
			connect(filter, &DoubleClickFilter::doubleClicked, this,
				&OBSSceneCollections::RowDoubleClicked);
		}
		row->setSuffix(suffix);
		ui->groupBox->addRow(row);

		return true;
	});
}

void OBSSceneCollections::on_buttonSort_pressed()
{
	QMenu *menu = new QMenu();
	menu->setAttribute(Qt::WA_DeleteOnClose);
	QAction *lastUsed =
		menu->addAction(Str("SceneCollections.Sort.LastUsed"), [&]() {
			config_set_string(App()->GlobalConfig(),
					  "SceneCollectionsWindow", "Order",
					  "LastUsed");
			collections_order = kSceneCollectionOrderLastUsed;
			refreshList();
		});
	lastUsed->setCheckable(true);
	lastUsed->setChecked(collections_order ==
			     kSceneCollectionOrderLastUsed);
	QAction *name =
		menu->addAction(Str("SceneCollections.Sort.Name"), [&]() {
			config_set_string(App()->GlobalConfig(),
					  "SceneCollectionsWindow", "Order",
					  "Name");
			collections_order = kSceneCollectionOrderName;
			refreshList();
		});
	name->setCheckable(true);
	name->setChecked(collections_order == kSceneCollectionOrderName);
	menu->popup(QCursor::pos());
}

static bool GetSceneCollectionName(QWidget *parent, const QString &title,
				   const QString &text, std::string &name,
				   const std::string &oldName = "")
{
	for (;;) {
		if (!NameDialog::AskForName(parent, title, text, name,
					    QString::fromStdString(oldName)))
			return false;

		if (name.empty()) {
			OBSMessageBox::warning(parent,
					       QTStr("NoNameEntered.Title"),
					       QTStr("NoNameEntered.Text"));
			continue;
		}

		if (SceneCollectionExists(name.c_str())) {
			OBSMessageBox::warning(parent,
					       QTStr("NameExists.Title"),
					       QTStr("NameExists.Text"));
			continue;
		}

		return true;
	}
}

void OBSSceneCollections::on_buttonNew_pressed()
{
	std::string name;

	if (!GetSceneCollectionName(
		    this, Str("SceneCollections.New.Title"),
		    Str("SceneCollections.GenericNamePrompt.Text"), name))
		return;

	CreateSceneCollection(name);

	emit collectionsChanged();
}

void OBSSceneCollections::on_buttonImport_pressed()
{
	OBSImporter imp(this);
	imp.exec();

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	raise();
#endif

	emit collectionsChanged();
}

void OBSSceneCollections::RowDoubleClicked(QObject *obj)
{
	if (ui->buttonBulkMode->isChecked())
		return;

	OBSActionRow *row = static_cast<OBSActionRow *>(obj);
	std::string file = row->property("file").toString().toStdString();

	OBSBasic *main = OBSBasic::Get();
	main->ChangeSceneCollection(file);
}

QList<OBSSceneCollections::SelectedRowInfo> OBSSceneCollections::selectedRows()
{
	QList<SelectedRowInfo> list;
	for (auto child : ui->groupBox->properties()->rows()) {
		OBSActionRow *row = static_cast<OBSActionRow *>(child);

		QCheckBox *checkbox = static_cast<QCheckBox *>(row->prefix());
		if (row->isVisible() && checkbox && checkbox->isChecked()) {
			std::string name =
				row->property("name").toString().toStdString();
			std::string file =
				row->property("file").toString().toStdString();
			bool current_collection =
				row->property("current_collection").toBool();
			list.push_back({name, file, current_collection});
		}
	}
	return list;
}

void OBSSceneCollections::on_buttonExportBulk_pressed()
{
	auto rows = selectedRows();
	if (rows.empty())
		return;

	if (rows.size() == 1) {
		auto row = rows.first();
		SCExport(row.file);
		return;
	}

	std::string folder =
		SelectDirectory(this,
				QTStr("SceneCollections.BulkExport.Title"),
				QDir::homePath())
			.toStdString();
	if (folder == "")
		return;

	for (auto row : rows) {
		std::string file_name;
		if (!GetFileSafeName(row.name.c_str(), file_name)) {
			blog(LOG_WARNING,
			     "Couldn't generate safe file name for '%s'",
			     file_name.c_str());
			continue;
		}
		std::string export_file = folder + "/" + file_name;
		if (!GetClosestUnusedFileName(export_file, "json")) {
			blog(LOG_WARNING,
			     "Couldn't get closest file name for '%s.json' in '%s'",
			     file_name.c_str(), folder.c_str());
			continue;
		}
		ExportSceneCollection(row.file, export_file);
	}

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	raise();
#endif

	refreshList();
}

void OBSSceneCollections::on_buttonDuplicateBulk_pressed()
{
	auto rows = selectedRows();
	if (rows.empty())
		return;

	if (rows.size() == 1) {
		auto row = rows.first();
		SCDuplicate(row.name, row.file);
		return;
	}

	QMessageBox::StandardButton button = OBSMessageBox::question(
		this, QTStr("SceneCollections.BulkDuplicate.Title"),
		QTStr("SceneCollections.BulkDuplicate.Text").arg(rows.size()),
		QMessageBox::StandardButtons(QMessageBox::Yes |
					     QMessageBox::No));

	if (button != QMessageBox::Yes)
		return;

	for (auto row : rows) {
		std::string new_name = row.name;
		do {
			new_name = QTStr("SceneCollections.Duplicate.Default")
					   .arg(new_name.c_str())
					   .toStdString();
		} while (SceneCollectionExists(new_name));

		DuplicateSceneCollection(row.file, new_name);
	}

	emit collectionsChanged();
}

void OBSSceneCollections::on_buttonDeleteBulk_pressed()
{
	auto rows = selectedRows();
	if (rows.empty())
		return;

	if (rows.size() == 1) {
		auto row = rows.first();
		SCDelete(row.name, row.file);
		return;
	}

	for (auto row : rows) {
		if (row.is_current_collection) {
			OBSMessageBox::information(
				this,
				QTStr("SceneCollections.BulkDelete.Title"),
				QTStr("SceneCollections.BulkDelete.ContainsCurrent.Text"));
			return;
		}
	}

	QMessageBox::StandardButton button = OBSMessageBox::question(
		this, QTStr("SceneCollections.BulkDelete.Title"),
		QTStr("SceneCollections.BulkDelete.Text").arg(rows.size()),
		QMessageBox::StandardButtons(QMessageBox::Yes |
					     QMessageBox::No));

	if (button != QMessageBox::Yes)
		return;

	BPtr active_collection = obs_frontend_get_current_scene_collection();

	for (auto row : rows) {
		if (row.is_current_collection) {
			blog(LOG_WARNING,
			     "Tried to delete the currently active scene collection. "
			     "This shouldn't be possible.");
			continue;
		}

		DeleteSceneCollection(row.file);
	}

	emit collectionsChanged();
}

void OBSSceneCollections::SCRename(const std::string &current_name,
				   const std::string &current_file)
{
	std::string new_name;
	if (!GetSceneCollectionName(
		    this, Str("SceneCollections.Rename.Title"),
		    Str("SceneCollections.GenericNamePrompt.Text"), new_name,
		    current_name))
		return;

	RenameSceneCollection(current_file, new_name);

	emit collectionsChanged();
}

void OBSSceneCollections::SCDuplicate(const std::string &current_name,
				      const std::string &current_file)
{
	std::string new_name;
	if (!GetSceneCollectionName(
		    this, Str("SceneCollections.Duplicate.Title"),
		    Str("SceneCollections.GenericNamePrompt.Text"), new_name,
		    QTStr("SceneCollections.Duplicate.Default")
			    .arg(current_name.c_str())
			    .toStdString()))
		return;

	DuplicateSceneCollection(current_file, new_name);

	emit collectionsChanged();
}

void OBSSceneCollections::SCDelete(const std::string &name,
				   const std::string &file)
{
	BPtr active_collection = obs_frontend_get_current_scene_collection();
	if (strcmp(name.c_str(), active_collection) == 0) {
		blog(LOG_WARNING,
		     "Tried to delete the currently active collection. If this "
		     "message is printed then there very obviously is a bug in the "
		     "program since the menu item should be greyed out for the "
		     "collection that is currently active.");
		return;
	}

	QMessageBox::StandardButton button = OBSMessageBox::question(
		this, QTStr("SceneCollections.Delete.Title"),
		QTStr("SceneCollections.Delete.Text").arg(name.c_str()),
		QMessageBox::StandardButtons(QMessageBox::Yes |
					     QMessageBox::No));

	if (button != QMessageBox::Yes)
		return;

	DeleteSceneCollection(file);

	emit collectionsChanged();
}

void OBSSceneCollections::SCExport(const std::string &current_file)
{
	std::string export_file =
		SaveFile(this, QTStr("SceneCollections.Export.Title"),
			 QDir::homePath() + "/" + QString(current_file.c_str()),
			 "JSON Files (*.json)")
			.toStdString();

	if (export_file == "")
		return;

	ExportSceneCollection(current_file, export_file);

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	raise();
#endif
}
