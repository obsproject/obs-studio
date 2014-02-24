/******************************************************************************
    Copyright (C) 2013-2014 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#include <obs.hpp>
#include <QMessageBox>
#include <QShowEvent>
#include <QFileDialog>

#include "obs-app.hpp"
#include "window-basic-settings.hpp"
#include "window-namedialog.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#include "ui_OBSBasic.h"

using namespace std;

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);

OBSBasic::OBSBasic(QWidget *parent)
	: OBSMainWindow (parent),
	  outputTest    (NULL),
	  sceneChanging (false),
	  ui            (new Ui::OBSBasic)
{
	ui->setupUi(this);
}

void OBSBasic::OBSInit()
{
	/* make sure it's fully displayed before doing any initialization */
	show();
	App()->processEvents();

	if (!obs_startup())
		throw "Failed to initialize libobs";
	if (!ResetVideo())
		throw "Failed to initialize video";
	if (!ResetAudio())
		throw "Failed to initialize audio";

	signal_handler_connect(obs_signalhandler(), "source-add",
			OBSBasic::SourceAdded, this);
	signal_handler_connect(obs_signalhandler(), "source-remove",
			OBSBasic::SourceRemoved, this);
	signal_handler_connect(obs_signalhandler(), "channel-change",
			OBSBasic::ChannelChanged, this);

	/* TODO: this is a test */
	obs_load_module("test-input");
	obs_load_module("obs-ffmpeg");

	/* HACK: fixes a qt bug with native widgets with native repaint */
	ui->previewContainer->repaint();
}

OBSBasic::~OBSBasic()
{
	/* free the lists before shutting down to remove the scene/item
	 * references */
	ui->sources->clear();
	ui->scenes->clear();
	obs_shutdown();
}

OBSScene OBSBasic::GetCurrentScene()
{
	QListWidgetItem *item = ui->scenes->currentItem();
	return item ? item->data(Qt::UserRole).value<OBSScene>() : nullptr;
}

OBSSceneItem OBSBasic::GetCurrentSceneItem()
{
	QListWidgetItem *item = ui->sources->currentItem();
	return item ? item->data(Qt::UserRole).value<OBSSceneItem>() : nullptr;
}

void OBSBasic::UpdateSources(OBSScene scene)
{
	ui->sources->clear();

	obs_scene_enum_items(scene,
			[] (obs_scene_t scene, obs_sceneitem_t item, void *p)
			{
				OBSBasic *window = static_cast<OBSBasic*>(p);
				window->AddSceneItem(item);

				UNUSED_PARAMETER(scene);
				return true;
			}, this);
}

/* Qt callbacks for invokeMethod */

void OBSBasic::AddScene(OBSSource source)
{
	const char *name  = obs_source_getname(source);
	obs_scene_t scene = obs_scene_fromsource(source);

	QListWidgetItem *item = new QListWidgetItem(QT_UTF8(name));
	item->setData(Qt::UserRole, QVariant::fromValue(OBSScene(scene)));
	ui->scenes->addItem(item);

	signal_handler_t handler = obs_source_signalhandler(source);
	signal_handler_connect(handler, "item-add",
			OBSBasic::SceneItemAdded, this);
	signal_handler_connect(handler, "item-remove",
			OBSBasic::SceneItemRemoved, this);
}

void OBSBasic::RemoveScene(OBSSource source)
{
	const char *name = obs_source_getname(source);

	QListWidgetItem *sel = ui->scenes->currentItem();
	QList<QListWidgetItem*> items = ui->scenes->findItems(QT_UTF8(name),
			Qt::MatchExactly);

	if (sel != nullptr) {
		if (items.contains(sel))
			ui->sources->clear();
		delete sel;
	}
}

void OBSBasic::AddSceneItem(OBSSceneItem item)
{
	obs_scene_t  scene  = obs_sceneitem_getscene(item);
	obs_source_t source = obs_sceneitem_getsource(item);
	const char   *name  = obs_source_getname(source);

	if (GetCurrentScene() == scene) {
		QListWidgetItem *listItem = new QListWidgetItem(QT_UTF8(name));
		listItem->setData(Qt::UserRole,
				QVariant::fromValue(OBSSceneItem(item)));

		ui->sources->insertItem(0, listItem);
	}

	sourceSceneRefs[source] = sourceSceneRefs[source] + 1;
}

void OBSBasic::RemoveSceneItem(OBSSceneItem item)
{
	obs_scene_t scene = obs_sceneitem_getscene(item);

	if (GetCurrentScene() == scene) {
		for (int i = 0; i < ui->sources->count(); i++) {
			QListWidgetItem *listItem = ui->sources->item(i);
			QVariant userData = listItem->data(Qt::UserRole);

			if (userData.value<OBSSceneItem>() == item) {
				delete listItem;
				break;
			}
		}
	}

	obs_source_t source = obs_sceneitem_getsource(item);

	int scenes = sourceSceneRefs[source] - 1;
	if (scenes == 0) {
		obs_source_remove(source);
		sourceSceneRefs.erase(source);
	}
}

void OBSBasic::UpdateSceneSelection(OBSSource source)
{
	if (source) {
		obs_source_type type;
		obs_source_gettype(source, &type, NULL);

		if (type != OBS_SOURCE_TYPE_SCENE)
			return;

		obs_scene_t scene = obs_scene_fromsource(source);
		const char *name = obs_source_getname(source);

		QList<QListWidgetItem*> items =
			ui->scenes->findItems(QT_UTF8(name), Qt::MatchExactly);

		if (items.count()) {
			sceneChanging = true;
			ui->scenes->setCurrentItem(items.first());
			sceneChanging = false;

			UpdateSources(scene);
		}
	}
}

/* OBS Callbacks */

void OBSBasic::SceneItemAdded(void *data, calldata_t params)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);

	obs_sceneitem_t item = (obs_sceneitem_t)calldata_ptr(params, "item");

	QMetaObject::invokeMethod(window, "AddSceneItem",
			Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

void OBSBasic::SceneItemRemoved(void *data, calldata_t params)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);

	obs_sceneitem_t item = (obs_sceneitem_t)calldata_ptr(params, "item");

	QMetaObject::invokeMethod(window, "RemoveSceneItem",
			Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

void OBSBasic::SourceAdded(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == OBS_SOURCE_TYPE_SCENE)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"AddScene",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceRemoved(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == OBS_SOURCE_TYPE_SCENE)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"RemoveScene",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::ChannelChanged(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");
	uint32_t channel = calldata_uint32(params, "channel");

	if (channel == 0)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"UpdateSceneSelection",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::RenderMain(void *data, uint32_t cx, uint32_t cy)
{
	obs_render_main_view();

	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

/* Main class functions */

bool OBSBasic::ResetVideo()
{
	struct obs_video_info ovi;

	App()->GetConfigFPS(ovi.fps_num, ovi.fps_den);

	ovi.graphics_module = App()->GetRenderModule();
	ovi.base_width     = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "BaseCX");
	ovi.base_height    = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "BaseCY");
	ovi.output_width   = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "OutputCX");
	ovi.output_height  = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "OutputCY");
	ovi.output_format  = VIDEO_FORMAT_I420;
	ovi.adapter        = 0;
	ovi.gpu_conversion = true;

	QTToGSWindow(ui->preview, ovi.window);

	//required to make opengl display stuff on osx(?)
	ResizePreview(ovi.base_width, ovi.base_height);

	QSize size = ui->preview->size();
	ovi.window_width  = size.width();
	ovi.window_height = size.height();

	if (!obs_reset_video(&ovi))
		return false;

	obs_add_draw_callback(OBSBasic::RenderMain, this);
	return true;
}

bool OBSBasic::ResetAudio()
{
	struct audio_output_info ai;
	ai.name = "Main Audio Track";
	ai.format = AUDIO_FORMAT_FLOAT;

	ai.samples_per_sec = config_get_uint(GetGlobalConfig(), "Audio",
			"SampleRate");

	const char *channelSetupStr = config_get_string(GetGlobalConfig(),
			"Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else
		ai.speakers = SPEAKERS_STEREO;

	ai.buffer_ms = config_get_uint(GetGlobalConfig(), "Audio",
			"BufferingTime");

	return obs_reset_audio(&ai);
}

void OBSBasic::ResizePreview(uint32_t cx, uint32_t cy)
{
	double targetAspect, baseAspect;
	QSize  targetSize;
	int x, y;

	/* resize preview panel to fix to the top section of the window */
	targetSize   = ui->previewContainer->size();
	targetAspect = double(targetSize.width()) / double(targetSize.height());
	baseAspect   = double(cx) / double(cy);

	if (targetAspect > baseAspect) {
		cx = targetSize.height() * baseAspect;
		cy = targetSize.height();
	} else {
		cx = targetSize.width();
		cy = targetSize.width() / baseAspect;
	}

	x = targetSize.width() /2 - cx/2;
	y = targetSize.height()/2 - cy/2;

	ui->preview->setGeometry(x, y, cx, cy);

	if (isVisible())
		obs_resize(cx, cy);
}

void OBSBasic::closeEvent(QCloseEvent *event)
{
	/* TODO */
	UNUSED_PARAMETER(event);
}

void OBSBasic::changeEvent(QEvent *event)
{
	/* TODO */
	UNUSED_PARAMETER(event);
}

void OBSBasic::resizeEvent(QResizeEvent *event)
{
	struct obs_video_info ovi;

	if (obs_get_video_info(&ovi))
		ResizePreview(ovi.base_width, ovi.base_height);

	UNUSED_PARAMETER(event);
}

void OBSBasic::on_action_New_triggered()
{
	/* TODO */
}

void OBSBasic::on_action_Open_triggered()
{
	/* TODO */
}

void OBSBasic::on_action_Save_triggered()
{
	/* TODO */
}

void OBSBasic::on_scenes_currentItemChanged(QListWidgetItem *current,
		QListWidgetItem *prev)
{
	obs_source_t source = NULL;

	if (sceneChanging)
		return;

	if (current) {
		obs_scene_t scene;

		scene = current->data(Qt::UserRole).value<OBSScene>();
		source = obs_scene_getsource(scene);
		UpdateSources(scene);
	}

	/* TODO: allow transitions */
	obs_set_output_source(0, source);

	UNUSED_PARAMETER(prev);
}

void OBSBasic::on_scenes_customContextMenuRequested(const QPoint &pos)
{
	/* TODO */
	UNUSED_PARAMETER(pos);
}

void OBSBasic::on_actionAddScene_triggered()
{
	string name;
	bool accepted = NameDialog::AskForName(this,
			QTStr("MainWindow.AddSceneDlg.Title"),
			QTStr("MainWindow.AddSceneDlg.Text"),
			name);

	if (accepted) {
		obs_source_t source = obs_get_source_by_name(name.c_str());
		if (source) {
			QMessageBox::information(this,
					QTStr("MainWindow.NameExists.Title"),
					QTStr("MainWindow.NameExists.Text"));

			obs_source_release(source);
			on_actionAddScene_triggered();
			return;
		}

		obs_scene_t scene = obs_scene_create(name.c_str());
		source = obs_scene_getsource(scene);
		obs_add_source(source);
		obs_scene_release(scene);

		obs_set_output_source(0, source);
	}
}

void OBSBasic::on_actionRemoveScene_triggered()
{
	QListWidgetItem *item = ui->scenes->currentItem();
	if (!item)
		return;

	QVariant userData = item->data(Qt::UserRole);
	obs_scene_t scene = userData.value<OBSScene>();
	obs_source_t source = obs_scene_getsource(scene);
	obs_source_remove(source);
}

void OBSBasic::on_actionSceneProperties_triggered()
{
	/* TODO */
}

void OBSBasic::on_actionSceneUp_triggered()
{
	/* TODO */
}

void OBSBasic::on_actionSceneDown_triggered()
{
	/* TODO */
}

void OBSBasic::on_sources_currentItemChanged(QListWidgetItem *current,
		QListWidgetItem *prev)
{
	/* TODO */
	UNUSED_PARAMETER(current);
	UNUSED_PARAMETER(prev);
}

void OBSBasic::on_sources_customContextMenuRequested(const QPoint &pos)
{
	/* TODO */
	UNUSED_PARAMETER(pos);
}

void OBSBasic::AddSource(obs_scene_t scene, const char *id)
{
	string name;

	bool success = false;
	while (!success) {
		bool accepted = NameDialog::AskForName(this,
				Str("MainWindow.AddSourceDlg.Title"),
				Str("MainWindow.AddSourceDlg.Text"),
				name);

		if (!accepted)
			break;

		obs_source_t source = obs_get_source_by_name(name.c_str());
		if (!source) {
			success = true;
		} else {
			QMessageBox::information(this,
					QTStr("MainWindow.NameExists.Title"),
					QTStr("MainWindow.NameExists.Text"));
			obs_source_release(source);
		}
	}

	if (success) {
		obs_source_t source = obs_source_create(OBS_SOURCE_TYPE_INPUT,
				id, name.c_str(), NULL);

		sourceSceneRefs[source] = 0;

		obs_add_source(source);
		obs_scene_add(scene, source);
		obs_source_release(source);
	}
}

void OBSBasic::AddSourcePopupMenu(const QPoint &pos)
{
	OBSScene scene = GetCurrentScene();
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	if (!scene)
		return;

	QMenu popup;
	while (obs_enum_input_types(idx++, &type)) {
		const char *name = obs_source_getdisplayname(
				OBS_SOURCE_TYPE_INPUT,
				type, App()->GetLocale());

		QAction *popupItem = new QAction(QT_UTF8(name), this);
		popupItem->setData(QT_UTF8(type));
		popup.addAction(popupItem);

		foundValues = true;
	}

	if (foundValues) {
		QAction *ret = popup.exec(pos);
		if (ret)
			AddSource(scene, ret->data().toString().toUtf8());
	}
}

void OBSBasic::on_actionAddSource_triggered()
{
	AddSourcePopupMenu(QCursor::pos());
}

void OBSBasic::on_actionRemoveSource_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();
	if (item)
		obs_sceneitem_remove(item);
}

void OBSBasic::on_actionSourceProperties_triggered()
{
}

void OBSBasic::on_actionSourceUp_triggered()
{
}

void OBSBasic::on_actionSourceDown_triggered()
{
}

void OBSBasic::on_recordButton_clicked()
{
	if (outputTest) {
		obs_output_destroy(outputTest);
		outputTest = NULL;
		ui->recordButton->setText("Start Recording");
	} else {
		QString path = QFileDialog::getSaveFileName(this,
				"Please enter a file name", QString(),
				"Video Files (*.avi)");

		if (path.isNull() || path.isEmpty())
			return;

		obs_data_t data = obs_data_create();
		obs_data_setstring(data, "filename", QT_TO_UTF8(path));

		outputTest = obs_output_create("ffmpeg_output", "test", data);
		obs_data_release(data);

		if (!obs_output_start(outputTest)) {
			obs_output_destroy(outputTest);
			outputTest = NULL;
			return;
		}

		ui->recordButton->setText("Stop Recording");
	}
}

void OBSBasic::on_settingsButton_clicked()
{
	OBSBasicSettings settings(this);
	settings.exec();
}
