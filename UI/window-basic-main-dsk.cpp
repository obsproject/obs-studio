#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

void OBSBasic::on_actionAddSceneDSK_triggered()
{
	AddSceneQuery(true);
}

void OBSBasic::on_actionRemoveSceneDSK_triggered()
{
	on_actionRemoveScene_triggered();
}

void OBSBasic::on_actionSceneUpDSK_triggered()
{
	ChangeListIndex(ui->dsk, true, -1, 0);
}

void OBSBasic::on_actionSceneDownDSK_triggered()
{
	ChangeListIndex(ui->dsk, true, 1, ui->dsk->count() - 1);
}

void OBSBasic::MoveDSKToTop()
{
	ChangeListIndex(ui->dsk, false, 0, 0);
}

void OBSBasic::MoveDSKToBottom()
{
	ChangeListIndex(ui->dsk, false, ui->dsk->count() - 1,
			ui->dsk->count() - 1);
}

void OBSBasic::on_dsk_customContextMenuRequested(const QPoint &pos)
{
	ShowScenesMenu(ui->dsk, pos);
}

void OBSBasic::SetDSKSource(OBSSource source)
{
	OBSSource prevSource = obs_get_output_source(7);
	obs_source_release(prevSource);

	if (prevSource == source)
		return;

	obs_set_output_source(7, source);

	for (int i = 0; i < ui->dsk->count(); i++) {
		QListWidgetItem *item = ui->dsk->item(i);
		OBSScene scene = GetSceneFromListItem(item);

		if (obs_dsk_from_source(source) == scene) {
			ui->dsk->blockSignals(true);
			ui->dsk->setCurrentItem(item);
			ui->dsk->blockSignals(false);
			break;
		}
	}

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_GLOBAL_SCENE_CHANGED);

	blog(LOG_INFO, "Switched to global scene '%s'",
	     obs_source_get_name(source));
}

void OBSBasic::on_actionGridModeDSK_triggered()
{
	bool gridMode = !ui->dsk->GetGridMode();
	ui->dsk->SetGridMode(gridMode);
}

obs_data_array_t *OBSBasic::SaveDSKListOrder()
{
	obs_data_array_t *dskOrder = obs_data_array_create();

	for (int i = 0; i < ui->dsk->count(); i++) {
		obs_data_t *data = obs_data_create();
		obs_data_set_string(data, "name",
				    QT_TO_UTF8(ui->dsk->item(i)->text()));
		obs_data_array_push_back(dskOrder, data);
		obs_data_release(data);
	}

	return dskOrder;
}

void OBSBasic::LoadDSKListOrder(obs_data_array_t *array)
{
	size_t num = obs_data_array_count(array);

	for (size_t i = 0; i < num; i++) {
		obs_data_t *data = obs_data_array_item(array, i);
		const char *name = obs_data_get_string(data, "name");

		ReorderItemByName(ui->dsk, name, (int)i);

		obs_data_release(data);
	}
}

void OBSBasic::on_dsk_currentItemChanged(QListWidgetItem *current,
					 QListWidgetItem *prev)
{
	on_dsk_itemClicked(current);
	UNUSED_PARAMETER(prev);
}

void OBSBasic::on_dsk_itemClicked(QListWidgetItem *item)
{
	if (!item)
		return;

	ui->dsk->blockSignals(true);
	OBSScene scene = GetCurrentGlobalScene();
	ui->sources->SetScene(scene);

	SetDSKSource(obs_scene_get_source(scene));
	ui->dsk->blockSignals(false);
}

void OBSBasic::EditDSKName()
{
	QListWidgetItem *item = ui->dsk->currentItem();
	Qt::ItemFlags flags = item->flags();

	item->setFlags(flags | Qt::ItemIsEditable);
	ui->dsk->editItem(item);
	item->setFlags(flags);
}

bool OBSBasic::IsDSKDockVisible()
{
	return ui->dskDock->isVisible();
}
