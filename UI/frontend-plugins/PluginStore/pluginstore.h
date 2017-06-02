#ifndef PLUGINSTORE_H
#define PLUGINSTORE_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>

#include <QMainWindow>
#include <QMouseEvent>
#include <QAction>
#include <QWebEngineView>
#include <qwebchannel>
#include <QWebEnginePage>
#include <QWebEngineDownloadItem>
#include "webpluginevent.h"
#include "ui_pluginstore.h"

//#define __DEF_PLUGING_MARKET_URL_           "D://html/test.html"
#define __DEF_PLUGING_MARKET_URL_             "http://plugin.ciscik.com/obs-store/index.html?language=%1&system=%2&big_version=%3"
#define __OBS__STUDIO__         


class PluginStore : public QDialog
{
    Q_OBJECT
private:

    QWebEngineView* m_lpWebUI;
    WebPluginEvent* m_lpWebEvent;
    std::unique_ptr<Ui_PluginStore> ui;
    QPoint move_point;
    bool mouse_press = false;

public:
    explicit PluginStore(QWidget *parent = 0);
    ~PluginStore();

	void        closeEvent(QCloseEvent *event) override;
	void        resizeEvent(QResizeEvent *event) override;
    void        mousePressEvent(QMouseEvent *event) override;
    void        mouseReleaseEvent(QMouseEvent *event) override;
    void        mouseMoveEvent(QMouseEvent *event) override;
private slots:
    void        on_closeButton_clicked();
    void        on_setButton_clicked();
};

#endif // PLUGINSTORE_H
