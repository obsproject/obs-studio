#pragma once

#include <QDialog>

class OBSPlainTextEdit;

class OBSCrashReport : public QDialog {
	Q_OBJECT

	OBSPlainTextEdit *textBox;

public:
	OBSCrashReport(QWidget *parent, const char *text);

public slots:
	void ExitClicked();
	void CopyClicked();
};
