#pragma once

#include <QObject>
#include <obs.hpp>

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

	std::vector<OBSSignal> sigs;

	OutputType type = OutputType::Program;

	OBSWeakSourceAutoRelease weakSource;
	OBSSceneAutoRelease sourceScene;
	OBSSceneItem sourceSceneItem;

	bool autoStart = false;

	void DestroyOutputScene();
	void DestroyOutputCanvas();

private slots:
	void OnStarting();
	void OnStart();
	void OnStopping();
	void OnStop(int code);
	void OnPause();
	void OnUnpause();
	void OnReconnect();
	void OnReconnectSuccess();
	void OnActivate();
	void OnDeactivate();

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

	void SetAutoStartEnabled(bool enable);
	bool AutoStartEnabled();

	OBSData Save();
	void Load(OBSData data);

	void Update();

signals:
	void Starting();
	void Started();
	void Stopping();
	void Stopped(int code);
	void Paused();
	void Unpaused();
	void Reconnecting();
	void Reconnected();
	void Activated();
	void Deactivated();
};
