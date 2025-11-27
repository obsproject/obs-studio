#include "OutputObj.hpp"
#include <widgets/OBSBasic.hpp>
#include "moc_OutputObj.cpp"

static void OutputStarting(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnStarting");
}

static void OutputStart(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnStart");
}

static void OutputStopping(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnStopping");
}

static void OutputStop(void *data, calldata_t *params)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	int code = (int)calldata_int(params, "code");

	QMetaObject::invokeMethod(obj, "OnStop", Q_ARG(int, code));
}

static void OutputPaused(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnPause");
}

static void OutputUnpaused(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnUnpause");
}

static void OutputActivated(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnActivate");
}

static void OutputDeactivated(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnDeactivate");
}

static void OutputReconnecting(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnReconnect");
}

static void OutputReconnectSuccess(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "OnReconnectSuccess");
}

void OutputObj::OnStarting()
{
	emit Starting();
}

void OutputObj::OnStart()
{
	emit Started();
}

void OutputObj::OnStopping()
{
	emit Stopping();
}

void OutputObj::OnStop(int code)
{
	emit Stopped(code);
}

void OutputObj::OnPause()
{
	emit Paused();
}

void OutputObj::OnUnpause()
{
	emit Paused();
}

void OutputObj::OnReconnect()
{
	emit Reconnecting();
}

void OutputObj::OnReconnectSuccess()
{
	emit Reconnected();
}

void OutputObj::OnActivate()
{
	emit Activated();
}

void OutputObj::OnDeactivate()
{
	DestroyOutputCanvas();

	emit Deactivated();
}

OutputObj::OutputObj(const char *id, const char *name, OBSData settings)
{
	output = obs_output_create(id, name, settings, nullptr);

	signal_handler_t *signal = obs_output_get_signal_handler(output);
	sigs.emplace_back(signal, "starting", OutputStarting, this);
	sigs.emplace_back(signal, "start", OutputStart, this);
	sigs.emplace_back(signal, "stopping", OutputStopping, this);
	sigs.emplace_back(signal, "stop", OutputStop, this);
	sigs.emplace_back(signal, "pause", OutputPaused, this);
	sigs.emplace_back(signal, "unpause", OutputUnpaused, this);
	sigs.emplace_back(signal, "activate", OutputActivated, this);
	sigs.emplace_back(signal, "deactivate", OutputDeactivated, this);
	sigs.emplace_back(signal, "reconnect", OutputReconnecting, this);
	sigs.emplace_back(signal, "reconnect_success", OutputReconnectSuccess, this);
}

OutputObj::~OutputObj()
{
	Stop();
}

void OutputObj::Update()
{
	if (!canvas)
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
		source = OBSBasic::Get()->GetCurrentSceneSource();
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

	obs_canvas_set_channel(canvas, 0, source);
}

bool OutputObj::Start()
{
	if (type == OutputType::Program) {
		canvas = obs_get_main_canvas();
	} else {
		obs_video_info ovi;
		obs_get_video_info(&ovi);
		canvas = obs_canvas_create_private(nullptr, &ovi, DEVICE);
	}

	if (!canvas)
		return false;

	Update();

	obs_output_set_media(output, obs_canvas_get_video(canvas), obs_get_audio());

	bool success = obs_output_start(output);
	if (!success)
		DestroyOutputCanvas();

	return success;
}

void OutputObj::Stop()
{
	obs_output_stop(output);
}

OBSOutput OutputObj::GetOutput()
{
	return output.Get();
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
	return type == OutputType::Source ? OBSGetStrongRef(weakSource) : nullptr;
}

void OutputObj::SetAutoStartEnabled(bool enable)
{
	autoStart = enable;
}

bool OutputObj::AutoStartEnabled()
{
	return autoStart;
}

void OutputObj::DestroyOutputCanvas()
{
	canvas = nullptr;

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
