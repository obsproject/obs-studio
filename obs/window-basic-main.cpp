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

#include <util/util.hpp>
#include <util/platform.h>

#include "obs-app.hpp"
#include "platform.hpp"
#include "window-basic-settings.hpp"
#include "window-namedialog.hpp"
#include "window-basic-source-select.hpp"
#include "window-basic-main.hpp"
#include "window-basic-properties.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"
#include "volume-control.hpp"

#include "ui_OBSBasic.h"

#include <sstream>

#include <QScreen>
#include <QWindow>

using namespace std;

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);

OBSBasic::OBSBasic(QWidget *parent)
	: OBSMainWindow (parent),
	  properties    (nullptr),
	  streamOutput  (nullptr),
	  service       (nullptr),
	  aac           (nullptr),
	  x264          (nullptr),
	  sceneChanging (false),
	  resizeTimer   (0),
	  ui            (new Ui::OBSBasic)
{
	ui->setupUi(this);

	connect(windowHandle(), &QWindow::screenChanged, [this]() {
		struct obs_video_info ovi;

		if (obs_get_video_info(&ovi))
			ResizePreview(ovi.base_width, ovi.base_height);
	});
}

static void SaveAudioDevice(const char *name, int channel, obs_data_t parent)
{
	obs_source_t source = obs_get_output_source(channel);
	if (!source)
		return;

	obs_data_t data = obs_save_source(source);

	obs_data_setobj(parent, name, data);

	obs_data_release(data);
	obs_source_release(source);
}

static obs_data_t GenerateSaveData()
{
	obs_data_t       saveData     = obs_data_create();
	obs_data_array_t sourcesArray = obs_save_sources();
	obs_source_t     currentScene = obs_get_output_source(0);
	const char       *sceneName   = obs_source_getname(currentScene);

	SaveAudioDevice(DESKTOP_AUDIO_1, 1, saveData);
	SaveAudioDevice(DESKTOP_AUDIO_2, 2, saveData);
	SaveAudioDevice(AUX_AUDIO_1,     3, saveData);
	SaveAudioDevice(AUX_AUDIO_2,     4, saveData);
	SaveAudioDevice(AUX_AUDIO_3,     5, saveData);

	obs_data_setstring(saveData, "current_scene", sceneName);
	obs_data_setarray(saveData, "sources", sourcesArray);
	obs_data_array_release(sourcesArray);
	obs_source_release(currentScene);

	return saveData;
}

void OBSBasic::ClearVolumeControls()
{
	VolControl *control;

	for (size_t i = 0; i < volumes.size(); i++) {
		control = volumes[i];
		delete control;
	}

	volumes.clear();
}

void OBSBasic::Save(const char *file)
{
	obs_data_t saveData  = GenerateSaveData();
	const char *jsonData = obs_data_getjson(saveData);

	/* TODO maybe a message box here? */
	if (!os_quick_write_utf8_file(file, jsonData, strlen(jsonData), false))
		blog(LOG_ERROR, "Could not save scene data to %s", file);

	obs_data_release(saveData);
}

static void LoadAudioDevice(const char *name, int channel, obs_data_t parent)
{
	obs_data_t data = obs_data_getobj(parent, name);
	if (!data)
		return;

	obs_source_t source = obs_load_source(data);
	if (source) {
		obs_set_output_source(channel, source);
		obs_source_release(source);
	}

	obs_data_release(data);
}

void OBSBasic::CreateDefaultScene()
{
	obs_scene_t  scene  = obs_scene_create(Str("Studio.Basic.Scene"));
	obs_source_t source = obs_scene_getsource(scene);

	obs_add_source(source);

#ifdef __APPLE__
	source = obs_source_create(OBS_SOURCE_TYPE_INPUT, "display_capture",
			Str("Studio.Basic.DisplayCapture"), NULL);

	if (source) {
		obs_scene_add(scene, source);
		obs_add_source(source);
		obs_source_release(source);
	}
#endif

	obs_set_output_source(0, obs_scene_getsource(scene));
	obs_scene_release(scene);
}

void OBSBasic::Load(const char *file)
{
	if (!file) {
		blog(LOG_ERROR, "Could not find file %s", file);
		return;
	}

	BPtr<char> jsonData = os_quick_read_utf8_file(file);
	if (!jsonData) {
		CreateDefaultScene();
		return;
	}

	obs_data_t       data       = obs_data_create_from_json(jsonData);
	obs_data_array_t sources    = obs_data_getarray(data, "sources");
	const char       *sceneName = obs_data_getstring(data, "current_scene");
	obs_source_t     curScene;

	LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
	LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
	LoadAudioDevice(AUX_AUDIO_1,     3, data);
	LoadAudioDevice(AUX_AUDIO_2,     4, data);
	LoadAudioDevice(AUX_AUDIO_3,     5, data);

	obs_load_sources(sources);

	curScene = obs_get_source_by_name(sceneName);
	obs_set_output_source(0, curScene);
	obs_source_release(curScene);

	obs_data_array_release(sources);
	obs_data_release(data);
}

static inline bool HasAudioDevices(const char *source_id)
{
	const char *output_id = source_id;
	obs_properties_t props = obs_get_source_properties(
			OBS_SOURCE_TYPE_INPUT, output_id, App()->GetLocale());
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

static void OBSStartStreaming(void *data, calldata_t params)
{
	UNUSED_PARAMETER(params);
	QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
			"StreamingStart");
}

static void OBSStopStreaming(void *data, calldata_t params)
{
	int code = (int)calldata_int(params, "errorcode");
	QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
			"StreamingStop", Q_ARG(int, code));
}

#define SERVICE_PATH "obs-studio/basic/service.json"

void OBSBasic::SaveService()
{
	if (!service)
		return;

	BPtr<char> serviceJsonPath(os_get_config_path(SERVICE_PATH));
	if (!serviceJsonPath)
		return;

	obs_data_t data     = obs_data_create();
	obs_data_t settings = obs_service_get_settings(service);

	obs_data_setstring(data, "type", obs_service_gettype(service));
	obs_data_setobj(data, "settings", settings);

	const char *json = obs_data_getjson(data);

	os_quick_write_utf8_file(serviceJsonPath, json, strlen(json), false);

	obs_data_release(settings);
	obs_data_release(data);
}

bool OBSBasic::LoadService()
{
	const char *type;

	BPtr<char> serviceJsonPath(os_get_config_path(SERVICE_PATH));
	if (!serviceJsonPath)
		return false;

	BPtr<char> jsonText = os_quick_read_utf8_file(serviceJsonPath);
	if (!jsonText)
		return false;

	obs_data_t data = obs_data_create_from_json(jsonText);

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_getstring(data, "type");

	obs_data_t settings = obs_data_getobj(data, "settings");

	service = obs_service_create(type, "default", settings);

	obs_data_release(settings);
	obs_data_release(data);

	return !!service;
}

bool OBSBasic::InitOutputs()
{
	streamOutput = obs_output_create("rtmp_output", "default", nullptr);
	if (!streamOutput)
		return false;

	signal_handler_connect(obs_output_signalhandler(streamOutput),
			"start", OBSStartStreaming, this);
	signal_handler_connect(obs_output_signalhandler(streamOutput),
			"stop", OBSStopStreaming, this);

	return true;
}

bool OBSBasic::InitEncoders()
{
	aac = obs_audio_encoder_create("ffmpeg_aac", "aac", nullptr);
	if (!aac)
		return false;

	x264 = obs_video_encoder_create("obs_x264", "h264", nullptr);
	if (!x264)
		return false;

	return true;
}

bool OBSBasic::InitService()
{
	if (LoadService())
		return true;

	service = obs_service_create("rtmp_common", nullptr, nullptr);
	if (!service)
		return false;

	return true;
}

bool OBSBasic::InitBasicConfigDefaults()
{
	bool hasDesktopAudio = HasAudioDevices(App()->OutputAudioSource());
	bool hasInputAudio   = HasAudioDevices(App()->InputAudioSource());

	config_set_default_int(basicConfig, "Window", "PosX",  -1);
	config_set_default_int(basicConfig, "Window", "PosY",  -1);
	config_set_default_int(basicConfig, "Window", "SizeX", -1);
	config_set_default_int(basicConfig, "Window", "SizeY", -1);

	vector<MonitorInfo> monitors;
	GetMonitors(monitors);

	if (!monitors.size()) {
		OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
		                  "technically shouldn't be possible.");
		return false;
	}

	uint32_t cx = monitors[0].cx;
	uint32_t cy = monitors[0].cy;

	/* TODO: temporary */
	config_set_default_string(basicConfig, "SimpleOutput", "path", "");
	config_set_default_uint  (basicConfig, "SimpleOutput", "VBitrate",
			2500);
	config_set_default_uint  (basicConfig, "SimpleOutput", "ABitrate", 128);

	config_set_default_uint  (basicConfig, "Video", "BaseCX",   cx);
	config_set_default_uint  (basicConfig, "Video", "BaseCY",   cy);

	cx = cx * 10 / 15;
	cy = cy * 10 / 15;
	config_set_default_uint  (basicConfig, "Video", "OutputCX", cx);
	config_set_default_uint  (basicConfig, "Video", "OutputCY", cy);

	config_set_default_uint  (basicConfig, "Video", "FPSType", 0);
	config_set_default_string(basicConfig, "Video", "FPSCommon", "30");
	config_set_default_uint  (basicConfig, "Video", "FPSInt", 30);
	config_set_default_uint  (basicConfig, "Video", "FPSNum", 30);
	config_set_default_uint  (basicConfig, "Video", "FPSDen", 1);

	config_set_default_uint  (basicConfig, "Audio", "SampleRate", 44100);
	config_set_default_string(basicConfig, "Audio", "ChannelSetup",
			"Stereo");
	config_set_default_uint  (basicConfig, "Audio", "BufferingTime", 1000);

	config_set_default_string(basicConfig, "Audio", "DesktopDevice1",
			hasDesktopAudio ? "default" : "disabled");
	config_set_default_string(basicConfig, "Audio", "DesktopDevice2",
			"disabled");
	config_set_default_string(basicConfig, "Audio", "AuxDevice1",
			hasInputAudio ? "default" : "disabled");
	config_set_default_string(basicConfig, "Audio", "AuxDevice2",
			"disabled");
	config_set_default_string(basicConfig, "Audio", "AuxDevice3",
			"disabled");

	return true;
}

bool OBSBasic::InitBasicConfig()
{
	BPtr<char> configPath(os_get_config_path("obs-studio/basic/basic.ini"));

	int errorcode = basicConfig.Open(configPath, CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		OBSErrorBox(NULL, "Failed to open basic.ini: %d", errorcode);
		return false;
	}

	return InitBasicConfigDefaults();
}

void OBSBasic::InitOBSCallbacks()
{
	signal_handler_connect(obs_signalhandler(), "source_add",
			OBSBasic::SourceAdded, this);
	signal_handler_connect(obs_signalhandler(), "source_remove",
			OBSBasic::SourceRemoved, this);
	signal_handler_connect(obs_signalhandler(), "channel_change",
			OBSBasic::ChannelChanged, this);
	signal_handler_connect(obs_signalhandler(), "source_activate",
			OBSBasic::SourceActivated, this);
	signal_handler_connect(obs_signalhandler(), "source_deactivate",
			OBSBasic::SourceDeactivated, this);
}

void OBSBasic::OBSInit()
{
	BPtr<char> savePath(os_get_config_path("obs-studio/basic/scenes.json"));

	/* make sure it's fully displayed before doing any initialization */
	show();
	App()->processEvents();

	if (!obs_startup())
		throw "Failed to initialize libobs";
	if (!InitBasicConfig())
		throw "Failed to load basic.ini";
	if (!ResetVideo())
		throw "Failed to initialize video";
	if (!ResetAudio())
		throw "Failed to initialize audio";

	InitOBSCallbacks();

	/* TODO: this is a test, all modules will be searched for and loaded
	 * automatically later */
	obs_load_module("test-input");
	obs_load_module("obs-ffmpeg");
	obs_load_module("obs-x264");
	obs_load_module("obs-outputs");
	obs_load_module("rtmp-services");
#ifdef __APPLE__
	obs_load_module("mac-avcapture");
	obs_load_module("mac-capture");
#elif _WIN32
	obs_load_module("win-wasapi");
	obs_load_module("win-capture");
#else
	obs_load_module("linux-xshm");
	obs_load_module("linux-pulseaudio");
#endif

	if (!InitOutputs())
		throw "Failed to initialize outputs";
	if (!InitEncoders())
		throw "Failed to initialize encoders";
	if (!InitService())
		throw "Failed to initialize service";

	Load(savePath);
	ResetAudioDevices();
}

OBSBasic::~OBSBasic()
{
	BPtr<char> savePath(os_get_config_path("obs-studio/basic/scenes.json"));
	SaveService();
	Save(savePath);

	if (properties)
		delete properties;

	/* free the lists before shutting down to remove the scene/item
	 * references */
	ClearVolumeControls();
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
				window->InsertSceneItem(item);

				UNUSED_PARAMETER(scene);
				return true;
			}, this);
}

void OBSBasic::InsertSceneItem(obs_sceneitem_t item)
{
	obs_source_t source = obs_sceneitem_getsource(item);
	const char   *name  = obs_source_getname(source);

	QListWidgetItem *listItem = new QListWidgetItem(QT_UTF8(name));
	listItem->setData(Qt::UserRole,
			QVariant::fromValue(OBSSceneItem(item)));

	ui->sources->insertItem(0, listItem);
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
	signal_handler_connect(handler, "item_add",
			OBSBasic::SceneItemAdded, this);
	signal_handler_connect(handler, "item_remove",
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

	if (GetCurrentScene() == scene)
		InsertSceneItem(item);

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

		obs_scene_t scene = obs_scene_fromsource(source);
		const char *name = obs_source_getname(source);

		if (!scene)
			return;

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

void OBSBasic::ActivateAudioSource(OBSSource source)
{
	VolControl *vol = new VolControl(source);

	volumes.push_back(vol);
	ui->volumeWidgets->layout()->addWidget(vol);
}

void OBSBasic::DeactivateAudioSource(OBSSource source)
{
	for (size_t i = 0; i < volumes.size(); i++) {
		if (volumes[i]->GetSource() == source) {
			delete volumes[i];
			volumes.erase(volumes.begin() + i);
			break;
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
	OBSBasic *window = static_cast<OBSBasic*>(data);
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	if (obs_scene_fromsource(source) != NULL)
		QMetaObject::invokeMethod(window,
				"AddScene",
				Q_ARG(OBSSource, OBSSource(source)));
	else
		window->sourceSceneRefs[source] = 0;
}

void OBSBasic::SourceRemoved(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	if (obs_scene_fromsource(source) != NULL)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"RemoveScene",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceActivated(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");
	uint32_t     flags  = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"ActivateAudioSource",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceDeactivated(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");
	uint32_t     flags  = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"DeactivateAudioSource",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::ChannelChanged(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");
	uint32_t channel = (uint32_t)calldata_int(params, "channel");

	if (channel == 0)
		QMetaObject::invokeMethod(static_cast<OBSBasic*>(data),
				"UpdateSceneSelection",
				Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::RenderMain(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);
	obs_video_info ovi;
	int newCX, newCY;

	obs_get_video_info(&ovi);

	newCX = int(window->previewScale * float(ovi.base_width));
	newCY = int(window->previewScale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
			-100.0f, 100.0f);
	gs_setviewport(window->previewX, window->previewY, newCX, newCY);

	obs_render_main_view();

	gs_projection_pop();
	gs_viewport_pop();

	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

/* Main class functions */

obs_service_t OBSBasic::GetService()
{
	if (!service)
		service = obs_service_create("rtmp_common", NULL, NULL);
	return service;
}

void OBSBasic::SetService(obs_service_t newService)
{
	if (newService) {
		if (service)
			obs_service_destroy(service);
		service = newService;
	}
}

bool OBSBasic::ResetVideo()
{
	struct obs_video_info ovi;

	GetConfigFPS(ovi.fps_num, ovi.fps_den);

	ovi.graphics_module = App()->GetRenderModule();
	ovi.base_width     = (uint32_t)config_get_uint(basicConfig,
			"Video", "BaseCX");
	ovi.base_height    = (uint32_t)config_get_uint(basicConfig,
			"Video", "BaseCY");
	ovi.output_width   = (uint32_t)config_get_uint(basicConfig,
			"Video", "OutputCX");
	ovi.output_height  = (uint32_t)config_get_uint(basicConfig,
			"Video", "OutputCY");
	ovi.output_format  = VIDEO_FORMAT_NV12;
	ovi.adapter        = 0;
	ovi.gpu_conversion = true;

	QTToGSWindow(ui->preview->winId(), ovi.window);

	//required to make opengl display stuff on osx(?)
	ResizePreview(ovi.base_width, ovi.base_height);

	QSize size = GetPixelSize(ui->preview);
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

	ai.samples_per_sec = config_get_uint(basicConfig, "Audio",
			"SampleRate");

	const char *channelSetupStr = config_get_string(basicConfig,
			"Audio", "ChannelSetup");

	if (strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else
		ai.speakers = SPEAKERS_STEREO;

	ai.buffer_ms = config_get_uint(basicConfig, "Audio", "BufferingTime");

	return obs_reset_audio(&ai);
}

void OBSBasic::ResetAudioDevice(const char *sourceId, const char *deviceName,
		int channel)
{
	const char *deviceId = config_get_string(basicConfig, "Audio",
			deviceName);
	obs_source_t source;
	obs_data_t settings;
	bool same = false;

	source = obs_get_output_source(channel);
	if (source) {
		settings = obs_source_getsettings(source);
		const char *curId = obs_data_getstring(settings, "device_id");

		same = (strcmp(curId, deviceId) == 0);

		obs_data_release(settings);
		obs_source_release(source);
	}

	if (!same)
		obs_set_output_source(channel, nullptr);

	if (!same && strcmp(deviceId, "disabled") != 0) {
		obs_data_t settings = obs_data_create();
		obs_data_setstring(settings, "device_id", deviceId);
		source = obs_source_create(OBS_SOURCE_TYPE_INPUT,
				sourceId, Str(deviceName), settings);
		obs_data_release(settings);

		obs_set_output_source(channel, source);
		obs_source_release(source);
	}
}

void OBSBasic::ResetAudioDevices()
{
	ResetAudioDevice(App()->OutputAudioSource(), "DesktopDevice1", 1);
	ResetAudioDevice(App()->OutputAudioSource(), "DesktopDevice2", 2);
	ResetAudioDevice(App()->InputAudioSource(),  "AuxDevice1", 3);
	ResetAudioDevice(App()->InputAudioSource(),  "AuxDevice2", 4);
	ResetAudioDevice(App()->InputAudioSource(),  "AuxDevice3", 5);
}

void OBSBasic::ResizePreview(uint32_t cx, uint32_t cy)
{
	QSize  targetSize;

	/* resize preview panel to fix to the top section of the window */
	targetSize = GetPixelSize(ui->preview);
	GetScaleAndCenterPos(int(cx), int(cy),
			targetSize.width(), targetSize.height(),
			previewX, previewY, previewScale);

	if (isVisible()) {
		if (resizeTimer)
			killTimer(resizeTimer);
		resizeTimer = startTimer(100);
	}
}

void OBSBasic::closeEvent(QCloseEvent *event)
{
	QWidget::closeEvent(event);
	if (!event->isAccepted())
		return;

	// remove draw callback in case our drawable surfaces go away before
	// the destructor gets called
	obs_remove_draw_callback(OBSBasic::RenderMain, this);
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

void OBSBasic::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == resizeTimer) {
		killTimer(resizeTimer);
		resizeTimer = 0;

		QSize size = GetPixelSize(ui->preview);
		obs_resize(size.width(), size.height());
	}
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

void OBSBasic::on_action_Settings_triggered()
{
	OBSBasicSettings settings(this);
	settings.exec();
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
		if (name.empty()) {
			QMessageBox::information(this,
					QTStr("MainWindow.NoNameEntered"),
					QTStr("MainWindow.NoNameEntered"));
			on_actionAddScene_triggered();
			return;
		}

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

void OBSBasic::AddSource(const char *id)
{
	OBSBasicSourceSelect sourceSelect(this, id);
	sourceSelect.exec();
}

void OBSBasic::AddSourcePopupMenu(const QPoint &pos)
{
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	if (!GetCurrentScene())
		return;

	QMenu popup;
	while (obs_enum_input_types(idx++, &type)) {
		const char *name = obs_source_getdisplayname(
				OBS_SOURCE_TYPE_INPUT,
				type, App()->GetLocale());

		if (strcmp(type, "scene") == 0)
			continue;

		QAction *popupItem = new QAction(QT_UTF8(name), this);
		popupItem->setData(QT_UTF8(type));
		popup.addAction(popupItem);

		foundValues = true;
	}

	if (foundValues) {
		QAction *ret = popup.exec(pos);
		if (ret)
			AddSource(ret->data().toString().toUtf8());
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
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_getsource(item);

	if (source) {
		delete properties;
		properties = new OBSBasicProperties(this, source);
		properties->Init();
	}
}

void OBSBasic::on_actionSourceUp_triggered()
{
}

void OBSBasic::on_actionSourceDown_triggered()
{
}

void OBSBasic::StreamingStart()
{
	ui->streamButton->setText("Stop Streaming");
}

void OBSBasic::StreamingStop(int errorcode)
{
	UNUSED_PARAMETER(errorcode);
	ui->streamButton->setText("Start Streaming");
}

void OBSBasic::on_streamButton_clicked()
{
	if (obs_output_active(streamOutput)) {
		obs_output_stop(streamOutput);

	} else {
		obs_data_t x264Settings = obs_data_create();
		obs_data_t aacSettings  = obs_data_create();

		int videoBitrate = config_get_uint(basicConfig, "SimpleOutput",
				"VBitrate");
		int audioBitrate = config_get_uint(basicConfig, "SimpleOutput",
				"ABitrate");

		SaveService();

		obs_data_setint(x264Settings, "bitrate", videoBitrate);
		obs_data_setbool(x264Settings, "cbr", true);

		obs_data_setint(aacSettings, "bitrate", audioBitrate);

		obs_encoder_update(x264, x264Settings);
		obs_encoder_update(aac,  aacSettings);

		obs_data_release(x264Settings);
		obs_data_release(aacSettings);

		obs_encoder_set_video(x264, obs_video());
		obs_encoder_set_audio(aac,  obs_audio());
		obs_output_set_video_encoder(streamOutput, x264);
		obs_output_set_audio_encoder(streamOutput, aac);
		obs_output_set_service(streamOutput, service);
		obs_output_start(streamOutput);
	}
}

void OBSBasic::on_settingsButton_clicked()
{
	OBSBasicSettings settings(this);
	settings.exec();
}

void OBSBasic::GetFPSCommon(uint32_t &num, uint32_t &den) const
{
	const char *val = config_get_string(basicConfig, "Video", "FPSCommon");

	if (strcmp(val, "10") == 0) {
		num = 10;
		den = 1;
	} else if (strcmp(val, "20") == 0) {
		num = 20;
		den = 1;
	} else if (strcmp(val, "25") == 0) {
		num = 25;
		den = 1;
	} else if (strcmp(val, "29.97") == 0) {
		num = 30000;
		den = 1001;
	} else if (strcmp(val, "48") == 0) {
		num = 48;
		den = 1;
	} else if (strcmp(val, "59.94") == 0) {
		num = 60000;
		den = 1001;
	} else if (strcmp(val, "60") == 0) {
		num = 60;
		den = 1;
	} else {
		num = 30;
		den = 1;
	}
}

void OBSBasic::GetFPSInteger(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSInt");
	den = 1;
}

void OBSBasic::GetFPSFraction(uint32_t &num, uint32_t &den) const
{
	num = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSDen");
}

void OBSBasic::GetFPSNanoseconds(uint32_t &num, uint32_t &den) const
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(basicConfig, "Video", "FPSNS");
}

void OBSBasic::GetConfigFPS(uint32_t &num, uint32_t &den) const
{
	uint32_t type = config_get_uint(basicConfig, "Video", "FPSType");

	if (type == 1) //"Integer"
		GetFPSInteger(num, den);
	else if (type == 2) //"Fraction"
		GetFPSFraction(num, den);
	else if (false) //"Nanoseconds", currently not implemented
		GetFPSNanoseconds(num, den);
	else
		GetFPSCommon(num, den);
}

config_t OBSBasic::Config() const
{
	return basicConfig;
}
