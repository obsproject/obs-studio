#pragma once

#include "ui_OBSLogViewer.h"

#include <QDialog>

class OBSLogViewer : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui::OBSLogViewer> ui;

	struct LogLine {
		int type;
		QString text;
	};
	std::vector<LogLine> lines;

	void InitLog();
	void RefreshLogView();
	void FocusSearch();

private slots:
	void AddLine(int type, const QString &text);
	void UpdateSearch();

	void on_openButton_clicked();
	void on_showStartup_clicked(bool checked);
	void on_clearButton_clicked();
	void on_searchNext_clicked();
	void on_searchPrev_clicked();

	void on_searchBox_textChanged(const QString &text);
	void on_regexOption_toggled(bool checked);
	void on_caseSensitive_toggled(bool checked);

	void on_textArea_customContextMenuRequested(const QPoint &pos);

protected:
	void keyPressEvent(QKeyEvent *event) override;

public:
	OBSLogViewer(QWidget *parent = nullptr);
	~OBSLogViewer();
};
