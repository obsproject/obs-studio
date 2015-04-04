#pragma once

#include <obs.hpp>
#include "qt-display.hpp"

class OBSProjector : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSDisplay display;
	OBSSource source;
	OBSSignal removedSignal;

	static void OBSRender(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceRemoved(void *data, calldata_t *params);

private slots:
	void EscapeTriggered();

public:
	OBSProjector(QWidget *parent, obs_source_t *source);
	~OBSProjector();

	void Init(int monitor);
};
