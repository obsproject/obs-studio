#pragma once

#include <QDialog>

#include "ui_output.h"
#include "../../UI/properties-view.hpp"

class DecklinkOutputUI : public QDialog {
	Q_OBJECT
private:
	OBSPropertiesView *propertiesView;
	OBSPropertiesView *previewPropertiesView;

public slots:
	void StartOutput();
	void StopOutput();
	void PropertiesChanged();

	void OutputStateChanged(bool);

	void StartPreviewOutput();
	void StopPreviewOutput();
	void PreviewPropertiesChanged();

	void PreviewOutputStateChanged(bool);

public:
	std::unique_ptr<Ui_Output> ui;
	DecklinkOutputUI(QWidget *parent);

	void ShowHideDialog();

	void SetupPropertiesView();
	void SaveSettings();

	void SetupPreviewPropertiesView();
	void SavePreviewSettings();
};
