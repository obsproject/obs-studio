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
QString		WebPluginEvent::GetLocalPluginVersion(QString strName)
{
	return	QString("1.1.1.1");
}
void		WebPluginEvent::DownLoadPluginUrl(QString strName,QString strUrl)
{
	emit DownLoadState(strName, 0, 0);
}
bool		WebPluginEvent::InstallPluginName(QString strName)
{
	
	return true;
}
bool		WebPluginEvent::UnInstallPluginName(QString strName)
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