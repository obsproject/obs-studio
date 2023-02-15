#pragma once

#include <QWidget>
#include <memory>

#include "ui_MainToolBar.h"

class OBSBasic;

class MainToolBar : public QWidget {
	Q_OBJECT

	friend class OBSBasic;

private:
	OBSBasic *main;

	std::unique_ptr<Ui_MainToolBar> ui;

private slots:
	void UpdateUndoRedo();

	void on_undoButton_clicked();
	void on_redoButton_clicked();
	void on_copyButton_clicked();
	void on_pasteButton_clicked();
	void on_screenshotButton_clicked();
	void on_previewVis_clicked();
	void on_previewLock_clicked();
	void on_settingsButton_clicked();

public:
	MainToolBar(QWidget *parent = nullptr);
};
