#include "OutputObj.hpp"
#include <widgets/OBSBasic.hpp>
#include "moc_OutputObj.cpp"

volatile long OutputObj::activeRefs = 0;

namespace {
void outputStarting(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onStarting");
}

void outputStart(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onStart");
}

void outputStopping(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onStopping");
}

void outputStop(void *data, calldata_t *params)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	int code = (int)calldata_int(params, "code");

	QMetaObject::invokeMethod(obj, "onStop", Q_ARG(int, code));
}

void outputPaused(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onPause");
}

void outputUnpaused(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onUnpause");
}

void outputActivated(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onActivate");
}

void outputDeactivated(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onDeactivate");
}

void outputReconnecting(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onReconnect");
}

void outputReconnectSuccess(void *data, calldata_t * /* params */)
{
	OutputObj *obj = static_cast<OutputObj *>(data);
	QMetaObject::invokeMethod(obj, "onReconnectSuccess");
}
} // namespace

void OutputObj::onFrontendEvent(enum obs_frontend_event event, void *data)
{
	OutputObj *obj = static_cast<OutputObj *>(data);

	if (obj->getType() != OutputType::Preview)
		return;

	if (event == OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED)
		obj->update();
}

void OutputObj::onStarting()
{
	if (requireRestart)
		return;

	emit starting();
}

void OutputObj::onStart()
{
	if (requireRestart)
		return;

	emit started();
	blog(LOG_INFO, "Output %s (%s) started", obs_output_get_name(output), obs_output_get_id(output));
}

void OutputObj::onStopping()
{
	if (requireRestart)
		return;

	emit stopping();
}

void OutputObj::onStop(int code)
{
	if (requireRestart)
		return;

	emit stopped(code);
	blog(LOG_INFO, "Output %s (%s) stopped", obs_output_get_name(output), obs_output_get_id(output));
}

void OutputObj::onPause()
{
	emit paused();
}

void OutputObj::onUnpause()
{
	emit unpaused();
}

void OutputObj::onReconnect()
{
	emit reconnecting();
}

void OutputObj::onReconnectSuccess()
{
	emit reconnected();
}

void OutputObj::onActivate()
{
	emit activated();
}

void OutputObj::onDeactivate()
{
	destroyOutputCanvas();

	emit deactivated();
}

OutputObj::OutputObj(const char *id, const char *name, OBSData settings)
{
	output = obs_output_create(id, name, settings, nullptr);

	signal_handler_t *signal = obs_output_get_signal_handler(output);
	obsSignals.emplace_back(signal, "starting", outputStarting, this);
	obsSignals.emplace_back(signal, "start", outputStart, this);
	obsSignals.emplace_back(signal, "stopping", outputStopping, this);
	obsSignals.emplace_back(signal, "stop", outputStop, this);
	obsSignals.emplace_back(signal, "pause", outputPaused, this);
	obsSignals.emplace_back(signal, "unpause", outputUnpaused, this);
	obsSignals.emplace_back(signal, "activate", outputActivated, this);
	obsSignals.emplace_back(signal, "deactivate", outputDeactivated, this);
	obsSignals.emplace_back(signal, "reconnect", outputReconnecting, this);
	obsSignals.emplace_back(signal, "reconnect_success", outputReconnectSuccess, this);

	obs_frontend_add_event_callback(onFrontendEvent, this);
}

OutputObj::~OutputObj()
{
	obs_frontend_remove_event_callback(onFrontendEvent, this);
	stop();
}

bool OutputObj::outputsActive()
{
	return os_atomic_load_long(&activeRefs) > 0;
}

void OutputObj::setupVideoEncoder(const char *id, const char *name, OBSData settings, uint32_t cx, uint32_t cy,
				  enum obs_scale_type rescaleFilter, enum video_format format,
				  enum video_colorspace colorSpace, enum video_range_type range)
{
	if (!isActive())
		return;

	OBSEncoderAutoRelease encoder = obs_video_encoder_create(id, name, settings, nullptr);
	obs_encoder_set_scaled_size(encoder, cx, cy);
	obs_encoder_set_gpu_scale_type(encoder, rescaleFilter);
	obs_encoder_set_preferred_video_format(encoder, format);
	obs_encoder_set_preferred_color_space(encoder, colorSpace);
	obs_encoder_set_preferred_range(encoder, range);
	setVideoEncoder(encoder.Get());
}

void OutputObj::setVideoEncoder(OBSEncoder encoder)
{
	obs_output_set_video_encoder(output, encoder);
}

OBSEncoder OutputObj::getVideoEncoder()
{
	OBSEncoderAutoRelease encoder = obs_output_get_video_encoder(output);
	return encoder.Get();
}

void OutputObj::setupAudioEncoders(const char *id, const char *name, OBSData settings)
{
	if (!isActive())
		return;

	size_t idx = 0;
	for (size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((audioTracks & ((size_t)1 << i)) != 0) {
			OBSEncoderAutoRelease encoder = obs_audio_encoder_create(id, name, settings, idx, nullptr);
			obs_encoder_set_audio(encoder, obs_get_audio());
			setAudioEncoder(encoder.Get(), idx);
			idx++;
		}
	}
}

void OutputObj::setAudioEncoder(OBSEncoder encoder, size_t track)
{
	obs_output_set_audio_encoder(output, encoder, track);
}

OBSEncoder OutputObj::getAudioEncoder(size_t track)
{
	OBSEncoderAutoRelease encoder = obs_output_get_audio_encoder(output, track);
	return encoder.Get();
}

void OutputObj::setupService(const char *id, const char *name, OBSData settings)
{
	if (!isActive())
		return;

	OBSServiceAutoRelease service = obs_service_create(id, name, settings, nullptr);
	obs_output_set_service(output, service);
}

OBSService OutputObj::getService()
{
	OBSServiceAutoRelease service = obs_output_get_service(output);
	return service.Get();
}

void OutputObj::setAudioTracks(size_t tracks)
{
	audioTracks = tracks;
	obs_output_set_mixers(output, audioTracks);
}

size_t OutputObj::getAudioTracks()
{
	return obs_output_get_mixers(output);
}

void OutputObj::update()
{
	if (!isActive())
		return;

	if (requireRestart) {
		stop();
		return;
	}

	OBSSourceAutoRelease source;

	switch (type) {
	case OutputType::Invalid:
	case OutputType::Multiview:
	case OutputType::Program:
		destroyOutputScene();
		return;
	case OutputType::Preview: {
		destroyOutputScene();
		source = obs_frontend_get_current_preview_scene();
		break;
	}
	case OutputType::Scene:
		destroyOutputScene();
		source = obs_source_get_ref(getSource());
		break;
	case OutputType::Source:
		OBSSource s = getSource();

		if (!sourceScene)
			sourceScene = obs_scene_create_private(nullptr);
		source = obs_source_get_ref(obs_scene_get_source(sourceScene));

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

void OutputObj::logOutputChanged(bool starting)
{
	const char *action = starting ? "Starting" : "Changing";
	const char *name = obs_output_get_name(output);
	const char *id = obs_output_get_id(output);
	const char *sourceName = obs_source_get_name(getSource());

	switch (type) {
	case OutputType::Invalid:
	case OutputType::Multiview:
		break;
	case OutputType::Program:
		blog(LOG_INFO, "%s %s (%s) output to Program", action, name, id);
		break;
	case OutputType::Preview:
		blog(LOG_INFO, "%s %s (%s) output to Preview", action, name, id);
		break;
	case OutputType::Scene:
		blog(LOG_INFO, "%s %s (%s) output to Scene : %s", action, name, id, sourceName);
		break;
	case OutputType::Source:
		blog(LOG_INFO, "%s %s (%s) output to Source : %s", action, name, id, sourceName);
		break;
	}
}

bool OutputObj::start()
{
	if (isActive())
		return false;

	requireRestart = false;

	if (type == OutputType::Program) {
		canvas = obs_get_main_canvas();
	} else {
		obs_video_info ovi;
		obs_get_video_info(&ovi);
		canvas = obs_canvas_create_private(nullptr, &ovi, DEVICE);
	}

	if (!canvas)
		return false;

	os_atomic_inc_long(&activeRefs);
	update();
	logOutputChanged(true);

	obs_output_set_media(output, obs_canvas_get_video(canvas), obs_get_audio());

	OBSEncoder videoEncoder = getVideoEncoder();

	if (videoEncoder)
		obs_encoder_set_video(videoEncoder, obs_canvas_get_video(canvas));

	bool success = obs_output_start(output);
	if (!success)
		destroyOutputCanvas();

	return success;
}

void OutputObj::stop(bool force)
{
	if (!isActive())
		return;

	if (force)
		obs_output_force_stop(output);
	else
		obs_output_stop(output);
}

OBSOutput OutputObj::getOutput()
{
	return output.Get();
}

bool OutputObj::isActive()
{
	return obs_output_active(output);
}

void OutputObj::setType(OutputType type_)
{
	requireRestart = isActive() && ((type == OutputType::Program && type_ != OutputType::Program) ||
					(type != OutputType::Program && type_ == OutputType::Program));

	type = type_;

	if (!isActive())
		logOutputChanged(false);
}

OutputType OutputObj::getType()
{
	return type;
}

void OutputObj::setSource(OBSSource source)
{
	weakSource = OBSGetWeakRef(source);
}

OBSSource OutputObj::getSource()
{
	return type == OutputType::Source ? OBSGetStrongRef(weakSource) : nullptr;
}

void OutputObj::destroyOutputCanvas()
{
	canvas = nullptr;
	destroyOutputScene();
	os_atomic_dec_long(&activeRefs);

	if (requireRestart)
		start();
}

void OutputObj::destroyOutputScene()
{
	if (!sourceScene)
		return;

	sourceScene = nullptr;
	sourceSceneItem = nullptr;
}
