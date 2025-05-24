#pragma once

#include <QDialog>
#include <QString>
#include <memory>

class Ui_ScriptsTool;

class ScriptLogWindow : public QDialog {
	Q_OBJECT

	QString lines;
	bool bottomScrolled = true;

	void resizeEvent(QResizeEvent *event) override;

public:
	ScriptLogWindow();
	~ScriptLogWindow();

public slots:
	void AddLogMsg(int log_level, QString msg);
	void ClearWindow();
	void Clear();
	void ScrollChanged(int val);
};

class ScriptsTool : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui_ScriptsTool> ui;
	QWidget *propertiesView = nullptr;

	void updatePythonVersionLabel();

public:
	ScriptsTool();
	~ScriptsTool();

	void RemoveScript(const char *path);
	void ReloadScript(const char *path);
	void RefreshLists();
	void SetScriptDefaults(const char *path);

public slots:
	void on_close_clicked();

	void on_addScripts_clicked();
	void on_removeScripts_clicked();
	void on_reloadScripts_clicked();
	void on_editScript_clicked();
	void on_scriptLog_clicked();
	void on_defaults_clicked();

	void OpenScriptParentDirectory();

	void on_scripts_currentRowChanged(int row);

	void on_pythonPathBrowse_clicked();

private slots:
	void on_description_linkActivated(const QString &link);
	void on_scripts_customContextMenuRequested(const QPoint &pos);
};
