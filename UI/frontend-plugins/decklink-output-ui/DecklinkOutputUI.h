#pragma once

#include <QDialog>

#include "ui_output.h"
#include <obs.hpp>

class DecklinkOutputUI : public QDialog {
	Q_OBJECT
private:
	QWidget *propertiesView;
	QWidget *previewPropertiesView;

public slots:
	void on_outputButton_clicked();
	void OutputStateChanged(bool);

	void on_previewOutputButton_clicked();
	void PreviewOutputStateChanged(bool);

public:
	std::unique_ptr<Ui_Output> ui;
	DecklinkOutputUI(QWidget *parent);

	void ShowHideDialog();

	void SetupPropertiesView();
	void SetupPreviewPropertiesView();
};
