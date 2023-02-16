#include "window-basic-vcam-config.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#include <util/util.hpp>
#include <util/platform.h>

enum class VCamOutputType {
	Internal,
	Scene,
	Source,
};

enum class VCamInternalType {
	Default,
	Preview,
};

struct VCamConfig {
	VCamOutputType type = VCamOutputType::Internal;
	VCamInternalType internal = VCamInternalType::Default;
	std::string scene;
	std::string source;
};

static VCamConfig *vCamConfig = nullptr;

OBSBasicVCamConfig::OBSBasicVCamConfig(QWidget *parent)
	: QDialog(parent), ui(new Ui::OBSBasicVCamConfig)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	auto type = (int)vCamConfig->type;
	ui->outputType->setCurrentIndex(type);
	OutputTypeChanged(type);
	connect(ui->outputType,
		static_cast<void (QComboBox::*)(int)>(
			&QComboBox::currentIndexChanged),
		this, &OBSBasicVCamConfig::OutputTypeChanged);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&OBSBasicVCamConfig::Save);
}

void OBSBasicVCamConfig::OutputTypeChanged(int type)
{
	auto list = ui->outputSelection;
	list->clear();

	switch ((VCamOutputType)type) {
	case VCamOutputType::Internal:
		list->addItem(QTStr("Basic.VCam.InternalDefault"));
		list->addItem(QTStr("Basic.VCam.InternalPreview"));
		list->setCurrentIndex((int)vCamConfig->internal);
		break;

	case VCamOutputType::Scene: {
		// Scenes in default order
		BPtr<char *> scenes = obs_frontend_get_scene_names();
		for (char **temp = scenes; *temp; temp++) {
			list->addItem(*temp);

			if (vCamConfig->scene.compare(*temp) == 0)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}

	case VCamOutputType::Source: {
		// Sources in alphabetical order
		std::vector<std::string> sources;
		auto AddSource = [&](obs_source_t *source) {
			auto name = obs_source_get_name(source);

			if (!(obs_source_get_output_flags(source) &
			      OBS_SOURCE_VIDEO))
				return;

			sources.push_back(name);
		};
		using AddSource_t = decltype(AddSource);

		obs_enum_sources(
			[](void *data, obs_source_t *source) {
				auto &AddSource =
					*static_cast<AddSource_t *>(data);
				if (!obs_source_removed(source))
					AddSource(source);
				return true;
			},
			static_cast<void *>(&AddSource));

		// Sort and select current item
		sort(sources.begin(), sources.end());
		for (auto &&source : sources) {
			list->addItem(source.c_str());

			if (vCamConfig->source == source)
				list->setCurrentIndex(list->count() - 1);
		}
		break;
	}
	}
}

void OBSBasicVCamConfig::Save()
{
	auto type = (VCamOutputType)ui->outputType->currentIndex();
	auto out = ui->outputSelection;
	switch (type) {
	case VCamOutputType::Internal:
		vCamConfig->internal = (VCamInternalType)out->currentIndex();
		break;
	case VCamOutputType::Scene:
		vCamConfig->scene = out->currentText().toStdString();
		break;
	case VCamOutputType::Source:
		vCamConfig->source = out->currentText().toStdString();
		break;
	default:
		// unknown value, don't save type
		return;
	}

	vCamConfig->type = type;

	// If already running just update the source
	if (obs_frontend_virtualcam_active())
		UpdateOutputSource();
}

void OBSBasicVCamConfig::SaveData(obs_data_t *data, bool saving)
{
	if (saving) {
		OBSDataAutoRelease obj = obs_data_create();

		obs_data_set_int(obj, "type", (int)vCamConfig->type);
		obs_data_set_int(obj, "internal", (int)vCamConfig->internal);
		obs_data_set_string(obj, "scene", vCamConfig->scene.c_str());
		obs_data_set_string(obj, "source", vCamConfig->source.c_str());

		obs_data_set_obj(data, "virtual-camera", obj);
	} else {
		OBSDataAutoRelease obj =
			obs_data_get_obj(data, "virtual-camera");

		vCamConfig->type =
			(VCamOutputType)obs_data_get_int(obj, "type");
		vCamConfig->internal =
			(VCamInternalType)obs_data_get_int(obj, "internal");
		vCamConfig->scene = obs_data_get_string(obj, "scene");
		vCamConfig->source = obs_data_get_string(obj, "source");
	}
}

static void EventCallback(enum obs_frontend_event event, void *)
{
	if (vCamConfig->type != VCamOutputType::Internal)
		return;

	// Update output source if the preview scene changes
	// or if the default transition is changed
	switch (event) {
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		if (vCamConfig->internal != VCamInternalType::Preview)
			return;
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
		if (vCamConfig->internal != VCamInternalType::Default)
			return;
		break;
	default:
		return;
	}

	OBSBasicVCamConfig::UpdateOutputSource();
}

static auto staticConfig = VCamConfig{};

void OBSBasicVCamConfig::Init()
{
	if (vCamConfig)
		return;

	vCamConfig = &staticConfig;

	obs_frontend_add_event_callback(EventCallback, nullptr);
}

static obs_view_t *view = nullptr;
static video_t *video = nullptr;

static obs_scene_t *sourceScene = nullptr;
static obs_sceneitem_t *sourceSceneItem = nullptr;

video_t *OBSBasicVCamConfig::StartVideo()
{
	if (!view)
		view = obs_view_create();

	UpdateOutputSource();

	if (!video)
		video = obs_view_add(view);
	return video;
}

void OBSBasicVCamConfig::StopVideo()
{
	obs_view_remove(view);
	obs_view_set_source(view, 0, nullptr);
	video = nullptr;

	if (sourceScene) {
		obs_scene_release(sourceScene);
		sourceScene = nullptr;
		sourceSceneItem = nullptr;
	}
}

void OBSBasicVCamConfig::DestroyView()
{
	StopVideo();
	obs_view_destroy(view);
	view = nullptr;
}

void OBSBasicVCamConfig::UpdateOutputSource()
{
	if (!view)
		return;

	obs_source_t *source = nullptr;

	switch ((VCamOutputType)vCamConfig->type) {
	case VCamOutputType::Internal:
		switch (vCamConfig->internal) {
		case VCamInternalType::Default:
			source = obs_get_output_source(0);
			break;
		case VCamInternalType::Preview:
			OBSSource s = OBSBasic::Get()->GetCurrentSceneSource();
			obs_source_get_ref(s);
			source = s;
			break;
		}
		break;

	case VCamOutputType::Scene:
		source = obs_get_source_by_name(vCamConfig->scene.c_str());
		break;

	case VCamOutputType::Source:
		auto rawSource =
			obs_get_source_by_name(vCamConfig->source.c_str());
		if (!rawSource)
			break;

		// Use a scene transform to fit the source size to the canvas
		if (!sourceScene)
			sourceScene = obs_scene_create_private(nullptr);
		source = obs_source_get_ref(obs_scene_get_source(sourceScene));

		if (sourceSceneItem) {
			if (obs_sceneitem_get_source(sourceSceneItem) !=
			    rawSource) {
				obs_sceneitem_remove(sourceSceneItem);
				sourceSceneItem = nullptr;
			}
		}
		if (!sourceSceneItem) {
			sourceSceneItem = obs_scene_add(sourceScene, rawSource);
			obs_source_release(rawSource);

			obs_sceneitem_set_bounds_type(sourceSceneItem,
						      OBS_BOUNDS_SCALE_INNER);
			obs_sceneitem_set_bounds_alignment(sourceSceneItem,
							   OBS_ALIGN_CENTER);

			const struct vec2 size = {
				(float)obs_source_get_width(source),
				(float)obs_source_get_height(source),
			};
			obs_sceneitem_set_bounds(sourceSceneItem, &size);
		}
		break;
	}

	auto current = obs_view_get_source(view, 0);
	if (source != current)
		obs_view_set_source(view, 0, source);
	obs_source_release(source);
	obs_source_release(current);
}
