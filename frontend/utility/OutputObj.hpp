#pragma once

#include <QObject>
#include <obs.hpp>
#include <obs-frontend-api.h>

enum class OutputType {
	Invalid,
	Program,
	Preview,
	Scene,
	Source,
	Multiview, // TODO
};

class OutputObj : public QObject {
	Q_OBJECT

private:
	OBSOutputAutoRelease output;
	OBSCanvasAutoRelease canvas;

	std::vector<OBSSignal> obsSignals;

	OutputType type = OutputType::Program;

	OBSWeakSourceAutoRelease weakSource;
	OBSSceneAutoRelease sourceScene;
	OBSSceneItem sourceSceneItem;

	size_t audioTracks = 0;
	bool autoStart = false;
	bool requireRestart = false;
	static volatile long activeRefs;

	void destroyOutputScene();
	void destroyOutputCanvas();
	void logOutputChanged(bool starting);

	static void onFrontendEvent(enum obs_frontend_event event, void *data);

private slots:
	void onStarting();
	void onStart();
	void onStopping();
	void onStop(int code);
	void onPause();
	void onUnpause();
	void onReconnect();
	void onReconnectSuccess();
	void onActivate();
	void onDeactivate();

public:
	OutputObj(const char *id, const char *name, OBSData settings = nullptr);
	~OutputObj();

	void setupVideoEncoder(const char *id, const char *name, OBSData settings = nullptr, uint32_t cx = 0,
			       uint32_t cy = 0, enum obs_scale_type rescaleFilter = OBS_SCALE_DISABLE,
			       enum video_format format = VIDEO_FORMAT_NONE,
			       enum video_colorspace colorSpace = VIDEO_CS_DEFAULT,
			       enum video_range_type range = VIDEO_RANGE_DEFAULT);
	void setupAudioEncoders(const char *id, const char *name, OBSData settings = nullptr);
	void setupService(const char *id, const char *name, OBSData settings = nullptr);

	void setVideoEncoder(OBSEncoder encoder);
	void setAudioEncoder(OBSEncoder encoder, size_t track);

	OBSOutput getOutput();
	OBSEncoder getVideoEncoder();
	OBSEncoder getAudioEncoder(size_t track);
	OBSService getService();

	void setAudioTracks(size_t audioTracks);
	size_t getAudioTracks();

	bool start();
	void stop(bool force = false);
	bool isActive();

	void setType(OutputType type_);
	OutputType getType();

	void setSource(OBSSource source);
	OBSSource getSource();

	void update();

	static bool outputsActive();

signals:
	void starting();
	void started();
	void stopping();
	void stopped(int code);
	void paused();
	void unpaused();
	void reconnecting();
	void reconnected();
	void activated();
	void deactivated();
};
