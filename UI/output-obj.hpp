#pragma once

#include <QObject>
#include <obs.hpp>

enum class OutputType {
	Invalid,
	Preview,
	Program,
	Scene,
	Source,
	Multiview, // TODO
};

class OutputObj : public QObject {
	Q_OBJECT

private:
	OBSOutputAutoRelease output;

	std::vector<OBSSignal> sigs;

	OutputType type = OutputType::Invalid;

	obs_view_t *view = nullptr;
	video_t *video = nullptr;

	OBSWeakSourceAutoRelease weakSource;
	OBSSceneAutoRelease sourceScene;
	OBSSceneItem sourceSceneItem;

	bool autoStart = false;

	void DestroyOutputScene();

private slots:
	void OnStart();
	void OnStop(int code);

	void DestroyOutputView();

public:
	OutputObj(const char *id, const char *name, OBSData settings);
	~OutputObj();

	OBSOutput GetOutput();

	bool Start();
	void Stop();
	bool Active();

	void SetType(OutputType type_);
	OutputType GetType();

	void SetSource(OBSSource source);
	OBSSource GetSource();

	void Reset();

	void SetAutoStartEnabled(bool enable);
	bool AutoStartEnabled();

	OBSData Save();
	void Load(OBSData data);

signals:
	void OutputStarted();
	void OutputStopped(int code);
};
