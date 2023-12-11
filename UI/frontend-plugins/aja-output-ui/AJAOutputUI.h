#pragma once

#include <QDialog>

#include "ui_output.h"

namespace aja {
class CardManager;
}

class AJAOutputUI : public QDialog {
	Q_OBJECT
private:
	QWidget *propertiesView;
	QWidget *previewPropertiesView;
	QWidget *miscPropertiesView;
	aja::CardManager *cardManager;
public slots:
	void on_outputButton_clicked();
	void OutputStateChanged(bool);

	void on_previewOutputButton_clicked();
	void PreviewOutputStateChanged(bool);

public:
	std::unique_ptr<Ui_Output> ui;
	AJAOutputUI(QWidget *parent);

	void SetCardManager(aja::CardManager *cm);
	aja::CardManager *GetCardManager();

	void ShowHideDialog();
	void SetupPropertiesView();
	void SetupPreviewPropertiesView();
	void SetupMiscPropertiesView();
};
