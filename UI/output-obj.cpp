#include "output-obj.hpp"

static void OutputStart(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnStart");
}

static void OutputStop(void *data, calldata_t *params)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	int code = (int)calldata_int(params, "code");

	QMetaObject::invokeMethod(obj, "OnStop", Q_ARG(int, code));
}

static void OutputDeactivated(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "DestroyOutputView");
}

void OutputObj::OnStart()
{
	emit OutputStarted();
}

void OutputObj::OnStop(int code)
{
	emit OutputStopped(code);
}

OutputObj::OutputObj(const char *id, const char *name, OBSData settings)
{
	output = obs_output_create(id, name, settings, nullptr);

	signal_handler_t *signal = obs_output_get_signal_handler(output);
	sigs.emplace_back(signal, "start", OutputStart, this);
	sigs.emplace_back(signal, "stop", OutputStop, this);
	sigs.emplace_back(signal, "deactivate", OutputDeactivated, this);
}

OutputObj::~OutputObj()
{
	Stop();
}

OBSOutput OutputObj::GetOutput()
{
	return output.Get();
}

void OutputObj::SetAutoStartEnabled(bool enable)
{
	autoStart = enable;
}

bool OutputObj::AutoStartEnabled()
{
	return autoStart;
}

bool OutputObj::Start()
{
	bool typeIsProgram = type == OutputType::Program;

	if (!view && !typeIsProgram)
		view = obs_view_create();

	Reset();

	if (!video) {
		video = typeIsProgram ? obs_get_video() : obs_view_add(view);

		if (!video)
			return false;
	}

	obs_output_set_media(output, video, obs_get_audio());

	bool success = obs_output_start(output);
	if (!success)
		DestroyOutputView();

	return success;
}

void OutputObj::Stop()
{
	obs_output_stop(output);
}

bool OutputObj::Active()
{
	return obs_output_active(output);
}

void OutputObj::SetType(OutputType type_)
{
	type = type_;
}

OutputType OutputObj::GetType()
{
	return type;
}

void OutputObj::SetSource(OBSSource source)
{
	weakSource = OBSGetWeakRef(source);
}

OBSSource OutputObj::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

void OutputObj::Reset()
{
	if (!view)
		return;

	OBSSource source;

	switch (type) {
	case OutputType::Invalid:
	case OutputType::Multiview:
	case OutputType::Program:
		DestroyOutputScene();
		return;
	case OutputType::Preview: {
		DestroyOutputScene();
		source = GetSource();
		break;
	}
	case OutputType::Scene:
		DestroyOutputScene();
		source = GetSource();
		break;
	case OutputType::Source:
		OBSSource s = GetSource();

		if (!sourceScene)
			sourceScene = obs_scene_create_private(nullptr);
		source = obs_scene_get_source(sourceScene);

		if (sourceSceneItem && (obs_sceneitem_get_source(sourceSceneItem) != s)) {
			obs_sceneitem_remove(sourceSceneItem);
			sourceSceneItem = nullptr;
		}

		if (!sourceSceneItem) {
			sourceSceneItem = obs_scene_add(sourceScene, s);

			obs_sceneitem_set_bounds_type(sourceSceneItem, OBS_BOUNDS_SCALE_INNER);
			obs_sceneitem_set_bounds_alignment(sourceSceneItem, OBS_ALIGN_CENTER);

			const struct vec2 size = {
				(float)obs_source_get_width(source),
				(float)obs_source_get_height(source),
			};
			obs_sceneitem_set_bounds(sourceSceneItem, &size);
		}
		break;
	}

	OBSSourceAutoRelease current = obs_view_get_source(view, 0);
	if (source != current)
		obs_view_set_source(view, 0, source);
}

void OutputObj::DestroyOutputView()
{
	if (type == OutputType::Program) {
		video = nullptr;
		return;
	}

	obs_view_remove(view);
	obs_view_set_source(view, 0, nullptr);
	video = nullptr;

	obs_view_destroy(view);
	view = nullptr;

	DestroyOutputScene();
}

void OutputObj::DestroyOutputScene()
{
	if (!sourceScene)
		return;

	sourceScene = nullptr;
	sourceSceneItem = nullptr;
}

OBSData OutputObj::Save()
{
	OBSDataAutoRelease data = obs_data_create();

	obs_data_set_obj(data, "settings", obs_output_get_settings(output));
	obs_data_set_string(data, "source_uuid", obs_source_get_uuid(GetSource()));
	obs_data_set_int(data, "type", (int)GetType());
	obs_data_set_bool(data, "auto_start", AutoStartEnabled());

	return data.Get();
}

void OutputObj::Load(OBSData data)
{
	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
	obs_output_update(output, settings);

	const char *uuid = obs_data_get_string(data, "source_uuid");
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid);
	SetSource(source.Get());

	type = (OutputType)obs_data_get_int(data, "type");
	autoStart = obs_data_get_bool(data, "auto_start");
}
