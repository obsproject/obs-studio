#ifndef WEBPLUGINEVENT_H
#define WEBPLUGINEVENT_H

#include <QObject>
#include <QMap>
#include <QList>

#include <string>
#include <iostream>

#include <QDesktopServices>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineDownloadItem>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "plugindb.h"

class WebPluginEvent;
class PluginItem : public QObject
{
    Q_OBJECT
public:
    explicit PluginItem(double dblId, QWebEngineDownloadItem* lpItem,WebPluginEvent* lpEvent);
    ~PluginItem();

public slots:
    void        on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize);
    void        on_web_downfile_finished();
private:
    double                      m_qstrPluginId;
    QWebEngineDownloadItem*     m_lpWebItem;
    WebPluginEvent*             m_lpEvent;
};


class WebPluginEvent : public QObject
{
    Q_OBJECT
public:
    explicit WebPluginEvent(QObject *parent = 0, QWebEngineView* view = 0);
	~WebPluginEvent();

    QString     ResultToJsonString(double dblPlugId,bool bResult);
signals:
	void        DownLoadState(QString strPluginID, qint64 iDownLoadSize, qint64 iTotalSize);
public	slots:
    QVariant	GetLocalPluginList();
	QString     GetLocalPluginVersion(QString strPluginID);
    void        DownLoadPluginUrl(const QVariantMap& param);
	QString     InstallPlugin(const QVariant& param);
    QString     UninstallPlugin(const QVariant& param);
    QString     RemoveFilePlugin(const QVariant& param);
	QString     GetCurrentSaveDirectory();
	void        SetNewSaveDirectory(QString strDirectory);
    void        OpenUrl(QString url);

    void        on_web_load_finished(bool ok);
    void        on_web_downfile_start(QWebEngineDownloadItem *item);
public:
	static	void	RemoveAllLabelFile();
	static	void	SetLabelDelete(QString strPluginFile);

private:
    QWebEngineView*                                 m_lpView;
    QMap<QString, QJsonObject>                      m_downInfoMap;
    PluginDB                                        m_PluginDB;
};

#endif // WEBPLUGINEVENT_H
