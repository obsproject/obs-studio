/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include <QMessageBox>
#include "window-basic-main.hpp"
#include "window-basic-source-select.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

bool OBSBasicSourceSelect::EnumSources(void *data, obs_source_t *source)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect*>(data);
	const char *name = obs_source_get_name(source);
	const char *id   = obs_source_get_id(source);

	if (strcmp(id, window->id) == 0)
		window->ui->sourceList->addItem(QT_UTF8(name));

	return true;
}

void OBSBasicSourceSelect::OBSSourceAdded(void *data, calldata_t *calldata)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect*>(data);
	obs_source_t *source = (obs_source_t*)calldata_ptr(calldata, "source");

	QMetaObject::invokeMethod(window, "SourceAdded",
			Q_ARG(OBSSource, source));
}

void OBSBasicSourceSelect::OBSSourceRemoved(void *data, calldata_t *calldata)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect*>(data);
	obs_source_t *source = (obs_source_t*)calldata_ptr(calldata, "source");

	QMetaObject::invokeMethod(window, "SourceRemoved",
			Q_ARG(OBSSource, source));
}

void OBSBasicSourceSelect::SourceAdded(OBSSource source)
{
	const char *name     = obs_source_get_name(source);
	const char *sourceId = obs_source_get_id(source);

	if (strcmp(sourceId, id) != 0)
		return;

	ui->sourceList->addItem(name);
}

void OBSBasicSourceSelect::SourceRemoved(OBSSource source)
{
	const char *name     = obs_source_get_name(source);
	const char *sourceId = obs_source_get_id(source);

	if (strcmp(sourceId, id) != 0)
		return;

	QList<QListWidgetItem*> items =
		ui->sourceList->findItems(name, Qt::MatchFixedString);

	if (!items.count())
		return;

	delete items[0];
}

static void AddExisting(const char *name)
{
	obs_source_t *source = obs_get_output_source(0);
	obs_scene_t  *scene  = obs_scene_from_source(source);
	if (!scene)
		return;

	source = obs_get_source_by_name(name);
	if (source) {
		obs_scene_add(scene, source);
		obs_source_release(source);
	}

	obs_scene_release(scene);
}

bool AddNew(QWidget *parent, const char *id, const char *name)
{
	obs_source_t *source  = obs_get_output_source(0);
	obs_scene_t  *scene   = obs_scene_from_source(source);
	bool         success = false;
	if (!source)
		return false;

	source = obs_get_source_by_name(name);
	if (source) {
		QMessageBox::information(parent,
				QTStr("NameExists.Title"),
				QTStr("NameExists.Text"));

	} else {
		source = obs_source_create(OBS_SOURCE_TYPE_INPUT,
				id, name, NULL);

		if (source) {
			obs_add_source(source);
			obs_scene_add(scene, source);

			success = true;
		}
	}

	obs_source_release(source);
	obs_scene_release(scene);

	return success;
}

void OBSBasicSourceSelect::on_buttonBox_accepted()
{
	bool useExisting = ui->selectExisting->isChecked();

	if (useExisting) {
		QListWidgetItem *item = ui->sourceList->currentItem();
		if (!item)
			return;

		AddExisting(QT_TO_UTF8(item->text()));
	} else {
		if (ui->sourceName->text().isEmpty()) {
			QMessageBox::information(this,
					QTStr("NoNameEntered.Title"),
					QTStr("NoNameEntered.Text"));
			return;
		}

		if (!AddNew(this, id, QT_TO_UTF8(ui->sourceName->text())))
			return;
	}

	done(DialogCode::Accepted);
}

void OBSBasicSourceSelect::on_buttonBox_rejected()
{
	done(DialogCode::Rejected);
}

OBSBasicSourceSelect::OBSBasicSourceSelect(OBSBasic *parent, const char *id_)
	: QDialog (parent),
	  ui      (new Ui::OBSBasicSourceSelect),
	  id      (id_)
{
	ui->setupUi(this);

	ui->sourceList->setAttribute(Qt::WA_MacShowFocusRect, false);

	QString placeHolderText{QT_UTF8(obs_source_get_display_name(
				OBS_SOURCE_TYPE_INPUT, id))};

	QString text{placeHolderText};
	int i = 1;
	obs_source_t *source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
		obs_source_release(source);
		text = QString("%1 %2").arg(placeHolderText).arg(i++);
	}

	ui->sourceName->setText(text);
	ui->sourceName->setFocus();	//Fixes deselect of text.
	ui->sourceName->selectAll();

	obs_enum_sources(EnumSources, this);
}
