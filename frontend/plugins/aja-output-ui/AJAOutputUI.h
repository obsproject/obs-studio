#pragma once

#include <QDialog>
#include <properties-view.hpp>

#include "ui_output.h"

namespace aja {
class CardManager;
}

class AJAOutputUI : public QDialog {
	Q_OBJECT
private:
	OBSPropertiesView *propertiesView;
	OBSPropertiesView *previewPropertiesView;
	OBSPropertiesView *miscPropertiesView;
	aja::CardManager *cardManager;
public slots:
	void on_outputButton_clicked();
	void PropertiesChanged();
	void OutputStateChanged(bool);

	void on_previewOutputButton_clicked();
	void PreviewPropertiesChanged();
	void PreviewOutputStateChanged(bool);

	void MiscPropertiesChanged();

public:
	std::unique_ptr<Ui_Output> ui;
	AJAOutputUI(QWidget *parent);

	void SetCardManager(aja::CardManager *cm);
	aja::CardManager *GetCardManager();

	void ShowHideDialog();
	void SaveSettings(const char *filename, obs_data_t *settings);
	void SetupPropertiesView();
	void SetupPreviewPropertiesView();
	void SetupMiscPropertiesView();
};
