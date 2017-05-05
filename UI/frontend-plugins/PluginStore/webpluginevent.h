#ifndef WEBPLUGINEVENT_H
#define WEBPLUGINEVENT_H

#include <QObject>
class WebPluginEvent : public QObject
{
    Q_OBJECT
public:
    explicit WebPluginEvent(QObject *parent = 0);
	~WebPluginEvent();
signals:
	void DownLoadState(QString strPluginName, qint64 iDownLoadSize,qint64 iTotalSize);
public	slots:
	QStringList	GetLocalPluginList();
	QString		GetLocalPluginVersion(QString strName);
	void		DownLoadPluginUrl(QString strName,QString strUrl);
	bool		InstallPluginName(QString strName);
	bool		UnInstallPluginName(QString strName);
	QString		GetCurrentSaveDirectory();
	void		SetNewSaveDirectory(QString strDirectory);
private:
};

#endif // WEBPLUGINEVENT_H
