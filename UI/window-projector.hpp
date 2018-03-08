#pragma once

#include <obs.hpp>
#include "qt-display.hpp"

enum class ProjectorType {
	Source,
	Scene,
	Preview,
	StudioProgram,
	Multiview
};

class QMouseEvent;

enum class MultiviewLayout : uint8_t {
	HORIZONTAL_TOP_8_SCENES = 0,
	HORIZONTAL_BOTTOM_8_SCENES = 1,
	VERTICAL_LEFT_8_SCENES = 2,
	VERTICAL_RIGHT_8_SCENES = 3
};

class OBSProjector : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSSource source;
	OBSSignal removedSignal;

	static void OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
	static void OBSRender(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceRemoved(void *data, calldata_t *params);

	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	int savedMonitor;
	bool isWindow;
	QString projectorTitle;
	ProjectorType type = ProjectorType::Source;
	OBSWeakSource multiviewScenes[8];
	OBSSource     multiviewLabels[10];
	gs_vertbuffer_t *outerBox = nullptr;
	gs_vertbuffer_t *innerBox = nullptr;
	gs_vertbuffer_t *leftVLine = nullptr;
	gs_vertbuffer_t *rightVLine = nullptr;
	gs_vertbuffer_t *leftLine = nullptr;
	gs_vertbuffer_t *topLine = nullptr;
	gs_vertbuffer_t *rightLine = nullptr;
	bool ready = false;

	// argb colors
	static const uint32_t outerColor      = 0xFFD0D0D0;
	static const uint32_t labelColor      = 0xD91F1F1F;
	static const uint32_t backgroundColor = 0xFF000000;
	static const uint32_t previewColor    = 0xFF00FF00;
	static const uint32_t programColor    = 0xFFFF0000;

	void UpdateMultiview();
	void UpdateProjectorTitle(QString name);

private slots:
	void EscapeTriggered();

public:
	OBSProjector(QWidget *widget, obs_source_t *source_, int monitor,
			QString title, ProjectorType type_);
	~OBSProjector();

	void Init();

	OBSSource GetSource();
	ProjectorType GetProjectorType();
	int GetMonitor();
	static void UpdateMultiviewProjectors();
	static void RenameProjector(QString oldName, QString newName);
};
