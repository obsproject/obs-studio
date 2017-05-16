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

class WebPluginEvent;
class PluginItem : public QObject
{
    Q_OBJECT
public:
    explicit PluginItem();
    ~PluginItem();

    void        SetPluginId(QString qstrId);
    QString     GetPluginId();

public slots:
    void        on_web_downfile_start(QWebEngineDownloadItem *item);
    void        on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize);
    void        on_web_downfile_finished();
private:
    QString                     m_qstrPluginId;
    QWebEngineDownloadItem*     m_lpWebItem;
};


class WebPluginEvent : public QObject
{
	struct PluginInfo
	{
		QString strPluginID;				//PluginId
		QString	strPluginProductName;		//ProductName
		QString	strPluginInfo;              //PluginInfo
		QString	strLocalVersion;            //Vesion
		QString	strPluginLocalFileName;     //Local File Name
	};

    Q_OBJECT
public:
    explicit WebPluginEvent(QObject *parent = 0, QWebEngineView* view = 0);
	~WebPluginEvent();

signals:
	void        DownLoadState(QString strPluginID, qint64 iDownLoadSize, qint64 iTotalSize);
public	slots:
	QStringList	GetLocalPluginList();
	QString		GetLocalPluginVersion(QString strPluginID);
    void        DownLoadPluginUrl(QString obj, QString strDownUrl);
	bool        InstallPluginName(QString strPluginID);
	bool        UnInstallPluginName(QString strPluginID);
	QString     GetCurrentSaveDirectory();
	void        SetNewSaveDirectory(QString strDirectory);
    void        OpenUrl(QString url);

    void        on_web_load_finished(bool ok);

private:
    QWebEngineView*                                 m_lpView;
    QMap<QString, PluginItem*>                      m_DownItemsMap;
};

#endif // WEBPLUGINEVENT_H
