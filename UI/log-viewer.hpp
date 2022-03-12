#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include "obs-app.hpp"

class OBSLogViewer : public QDialog {
	Q_OBJECT

	QPointer<QPlainTextEdit> textArea;

	void InitLog();

private slots:
	void AddLine(int type, const QString &text);
	void ClearText();
	void ToggleShowStartup(bool checked);
	void OpenFile();

public:
	OBSLogViewer(QWidget *parent = 0);
	~OBSLogViewer();
};
