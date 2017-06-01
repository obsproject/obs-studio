#ifndef PLUGINSTORE_H
#define PLUGINSTORE_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>

#include <QMainWindow>
#include <QAction>
#include <QWebEngineView>
#include <qwebchannel>
#include <QWebEnginePage>
#include <QWebEngineDownloadItem>
#include "webpluginevent.h"
#include "ui_pluginstore.h"

#define __DEF_PLUGING_MARKET_URL_           "D://html/test.html"
//#define __DEF_PLUGING_MARKET_URL_         "http://plugin.ciscik.com/obs-store/test1.html"


class PluginStore : public QDialog
{
    Q_OBJECT
private:

    QWebEngineView* m_lpWebUI;
    WebPluginEvent* m_lpWebEvent;
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
