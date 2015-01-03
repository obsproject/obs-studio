#pragma once

#include <QDialog>

class QPlainTextEdit;

class OBSCrashReport : public QDialog {
	Q_OBJECT

	QPlainTextEdit *textBox;

public:
	OBSCrashReport(QWidget *parent, const char *text);

public slots:
	void ExitClicked();
	void CopyClicked();
};
