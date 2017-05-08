#ifndef WEBPLUGINEVENT_H
#define WEBPLUGINEVENT_H

#include <QObject>
#include<QMap>
class WebPluginEvent : public QObject
{
	struct PluginInfo
	{
		QString strPluginID;				//pluginID
		QString	strPluginproductName;		//productName
		QString	strPluginInfo;
		QString	strLocalVersion;
		QString	strPluginLocalFileName;			
	};
	
    Q_OBJECT
public:
    explicit WebPluginEvent(QObject *parent = 0);
	~WebPluginEvent();
signals:
	void DownLoadState(QString strPluginID, qint64 iDownLoadSize, qint64 iTotalSize);
public	slots:
	QStringList	GetLocalPluginList();
	QString		GetLocalPluginVersion(QString strPluginID);
	void		DownLoadPluginUrl(QString strPluginID, 
								QString	strPluginproductName, 
								QString	strPluginInfo,
								QString	strLocalVersion,
								QString strMD5,
								qint64  iSize,
								QString strUrl);
	bool		InstallPluginName(QString strPluginID);
	bool		UnInstallPluginName(QString strPluginID);
	QString		GetCurrentSaveDirectory();
	void		SetNewSaveDirectory(QString strDirectory);
private:
	QMap<QString, WebPluginEvent::PluginInfo*>		pluginInfoMap;
};

#endif // WEBPLUGINEVENT_H
