#pragma once

#include <obs.hpp>
#include "qt-display.hpp"
#include "window-basic-main.hpp"

class QMouseEvent;

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

	int savedMonitor = 0;
	bool isWindow = false;
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

	void UpdateMultiview();

private slots:
	void EscapeTriggered();

public:
	OBSProjector(QWidget *parent, obs_source_t *source, bool window);
	~OBSProjector();

	void Init(int monitor, bool window, QString title,
			ProjectorType type = ProjectorType::Source);

	static void UpdateMultiviewProjectors();
};
