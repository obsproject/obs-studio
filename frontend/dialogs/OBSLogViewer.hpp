#pragma once

#include "ui_OBSLogViewer.h"

#include <QDialog>

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
