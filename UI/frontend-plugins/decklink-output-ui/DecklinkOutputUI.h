#pragma once

#include <QDialog>

#include "ui_output.h"
#include "../../UI/properties-view.hpp"

extern bool main_output_running;
extern bool preview_output_running;

extern obs_output_t *output_main;
extern obs_output_t *output_preview;

struct preview_output {
	bool enabled;
	obs_source_t *current_source;
	obs_output_t *output;

	video_t *video_queue;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;
	uint8_t *video_data;
	uint32_t video_linesize;

	obs_video_info ovi;
};

extern struct preview_output context;

class DecklinkOutputUI : public QDialog {
	Q_OBJECT
private:
	OBSPropertiesView *propertiesView;
	OBSPropertiesView *previewPropertiesView;

	OBSSignal decklinkOutStart;
	OBSSignal decklinkOutStop;
	OBSSignal decklinkPreviewOutStart;
	OBSSignal decklinkPreviewOutStop;

public slots:
	void StartOutput();
	void StopOutput();
	void PropertiesChanged();

	void StartPreviewOutput();
	void StopPreviewOutput();
	void PreviewPropertiesChanged();

	void ToggleOutput();
	void TogglePreviewOutput();

	void OutputButtonSetStarted();
	void OutputButtonSetStopped();
	void PreviewOutputButtonSetStarted();
	void PreviewOutputButtonSetStopped();

public:
	std::unique_ptr<Ui_Output> ui;
	DecklinkOutputUI(QWidget *parent);
	~DecklinkOutputUI();

	void ShowHideDialog();

	void SetupPropertiesView();
	void SaveSettings();

	void SetupPreviewPropertiesView();
	void SavePreviewSettings();
};
