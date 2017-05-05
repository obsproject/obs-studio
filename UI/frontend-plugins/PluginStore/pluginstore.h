#ifndef PLUGINSTORE_H
#define PLUGINSTORE_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>
#include "ui_pluginstore.h"
#include <QWebEngineView>
class PluginStore : public QDialog
{
    Q_OBJECT

public:
    std::unique_ptr<Ui_PluginStore> ui;
	QWebEngineView* webui;
     bool loading = true;
    explicit PluginStore(QWidget *parent = 0);
    ~PluginStore();
	void closeEvent(QCloseEvent *event) override;
	void    resizeEvent(QResizeEvent *event)override;
private slots:
	void on_close_clicked();
	void on_web_loadFinished(bool ok);
private:

};

#endif // PLUGINSTORE_H
