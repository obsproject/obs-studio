#ifndef PLUGINSTORE_H
#define PLUGINSTORE_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>

#include <QWebEngineView>
#include <qwebchannel>
#include <QWebEnginePage>
#include <QWebEngineDownloadItem>
#include "ui_pluginstore.h"
#include "webpluginevent.h"

#define __DEF_PLUGING_MARKET_URL_   "D:/index.html"

class PluginStore : public QDialog
{
    Q_OBJECT
private:
    QWebEngineView* m_lpWebUI;
    std::unique_ptr<Ui_PluginStore> ui;

public:
    explicit PluginStore(QWidget *parent = 0);
    ~PluginStore();

	void        closeEvent(QCloseEvent *event) override;
	void        resizeEvent(QResizeEvent *event)override;
private slots:
	void        on_close_clicked();
};

#endif // PLUGINSTORE_H
