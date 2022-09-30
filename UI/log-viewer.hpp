#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include "obs-app.hpp"

#include "ui_OBSLogViewer.h"

class OBSLogViewer : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui::OBSLogViewer> ui;

	void InitLog();

private slots:
	void AddLine(int type, const QString &text);
	void on_openButton_clicked();
	void on_showStartup_clicked(bool checked);

public:
	OBSLogViewer(QWidget *parent = 0);
	~OBSLogViewer();
};
