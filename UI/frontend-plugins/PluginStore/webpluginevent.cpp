#include <QMessageBox>
#include "webpluginevent.h"

WebPluginEvent::WebPluginEvent(QObject *parent) : QObject(parent)
{

}
WebPluginEvent::~WebPluginEvent()
{
	
}
QStringList	WebPluginEvent::GetLocalPluginList()
{
	return	{ "1", "2", "3" };
}
QString		WebPluginEvent::GetLocalPluginVersion(QString strPluginID)
{
	return	QString("1.1.1.1");
}
void		WebPluginEvent::DownLoadPluginUrl(QString strPluginID,
	QString	strPluginproductName,
	QString	strPluginInfo,
	QString	strLocalVersion,
	QString strMD5,
	qint64  iSize,
	QString strUrl)
{
	emit DownLoadState(strPluginID, 0, 0);
}
bool		WebPluginEvent::InstallPluginName(QString strPluginID)
{
	
	return true;
}
bool		WebPluginEvent::UnInstallPluginName(QString strPluginID)
{
	return true;
}

QString		WebPluginEvent::GetCurrentSaveDirectory()
{
	return QString("");
}
void		WebPluginEvent::SetNewSaveDirectory(QString strDirectory)
{

}