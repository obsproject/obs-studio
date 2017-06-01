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
#include <QFileDialog>
#include "plugindb.h"

static PluginDB m_PluginDB;

class WebPluginEvent;
class PluginItem : public QObject
{
    Q_OBJECT
public:
    explicit PluginItem(qint64 qiPluginId, WebPluginEvent* lpEvent);
    ~PluginItem();

    void SetPluginDownItem(QWebEngineDownloadItem* lpItem);
    QWebEngineDownloadItem* GetPluginDownItem();
    void SetPluginData(QJsonObject obj, PluginDB::PluginState state);
    void SetPluginData(PluginDB::PluginInfo obj);
    PluginDB::PluginInfo GetPluingData();
public slots:
    void        on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize);
    void        on_web_downfile_finished();
private:
    qint64                      m_qiPluginId;
    QWebEngineDownloadItem*     m_lpWebItem;
    WebPluginEvent*             m_lpEvent;
    PluginDB::PluginInfo        m_PluginData;
};


class WebPluginEvent : public QObject
{
    Q_OBJECT
public:
    explicit WebPluginEvent(QObject *parent = 0, QWebEngineView* view = 0);
	~WebPluginEvent();

    // for return web json value.
    QString     ResultToJsonString(qint64 qiPlugId, bool bResult);
    void        DownloadSuc(qint64 qiPluginId);
    void        RemovePluginMap(qint64 qiPluginId);
    void        ClearPluginMap();
    void        UninitPluginStore();

    // Find to download plugin item. 
    bool        FindDownloadListItem(qint64 qiPluginId,PluginItem** lpItem = NULL);

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
    void        SetDownloadPluginDir();

    void        on_web_load_finished(bool ok);
    void        on_web_downfile_start(QWebEngineDownloadItem *item);
public:
	static	void	RemoveAllLabelFile();
	static	void	SetLabelDelete(QString strPluginFile);
	static  bool	InstallPluginZip(QString strZipFile, QString strPluginName);
	static  bool	CheckPluginRuning(QString strPluginName);
	static  void	MakePluginPath(QString strPluginName, QString& strPluginBin, QString& strPluginData);
	static	bool	LoadPlugin(QString	strPluginName);
private:
    QWebEngineView*                                 m_lpView;
    QMap<QString, QJsonObject>                      m_downInfoMap;
    QMap<qint64, PluginItem*>                       m_PluginMap;
    QString                                         m_DownPluginDir;
};

#endif // WEBPLUGINEVENT_H
