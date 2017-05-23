#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include "webpluginevent.h"


using namespace std;

/************************************************************************/
/* download files Mangert                                               */
/************************************************************************/
PluginItem::PluginItem()
{
    m_qstrPluginId = "";
    m_lpWebItem = NULL;
}

PluginItem::~PluginItem()
{
}


void PluginItem::SetPluginId(QString qstrId)
{
    m_qstrPluginId = qstrId;
}

QString PluginItem::GetPluginId()
{
    return m_qstrPluginId;
}


void PluginItem::on_web_downfile_start(QWebEngineDownloadItem *item)
{
    if (item)
    {
        m_lpWebItem = item;

        connect(item, SIGNAL(downloadProgress(qint64, qint64)),
            this, SLOT(on_web_downfile_progress(qint64, qint64)));
        connect(item, SIGNAL(finished()), this, SLOT(on_web_downfile_finished()));
        item->accept();
    }
}


void PluginItem::on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize)
{
    qDebug("Plugins down progresss %s :%d -- %d", m_qstrPluginId, qiRecvSize, qiTotalSize);
}

void PluginItem::on_web_downfile_finished()
{

}

/************************************************************************/
/* Web page's event                                                     */
/************************************************************************/
WebPluginEvent::WebPluginEvent(QObject *parent, QWebEngineView* view) : QObject(parent)
{
    if (view)
    {
        m_lpView = view;
        connect(view, SIGNAL(loadFinished(bool)), this, SLOT(on_web_load_finished(bool)));
    }
}
WebPluginEvent::~WebPluginEvent()
{
	
}

QStringList	WebPluginEvent::GetLocalPluginList()
{
    return	{ "1", "2", "3" };
}
QString	WebPluginEvent::GetLocalPluginVersion(QString strPluginID)
{
    return	QString("1.1.1.1");
}
void WebPluginEvent::DownLoadPluginUrl(QString qstrId, QString strDownUrl)
{
    if (m_lpView)
    {

        PluginItem item;
        item.SetPluginId(qstrId);
        m_DownItemsMap.insert(qstrId, &item);

        connect(m_lpView->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
            &item, SLOT(on_web_downfile_start(QWebEngineDownloadItem*)));

        m_lpView->load(QUrl(strDownUrl));
    }
    return;
}
bool WebPluginEvent::InstallPluginName(QString strPluginID)
{
    return true;
}
bool WebPluginEvent::UnInstallPluginName(QString strPluginID)
{
    return true;
}

QString WebPluginEvent::GetCurrentSaveDirectory()
{
    return QString("");
}
void WebPluginEvent::SetNewSaveDirectory(QString strDirectory)
{

}

void WebPluginEvent::OpenUrl(QString url)
{
    QDesktopServices::openUrl(QUrl(url));
}

void WebPluginEvent::on_web_load_finished(bool ok)
{
    return;
}
void WebPluginEvent::RemoveAllLabelFile()
{
	obs_module_t* hModule = obs_current_module();
	QString strBinPath = QString::fromUtf8(obs_get_module_binary_path(hModule));
	QString strDataPath = QString::fromUtf8(obs_get_module_data_path(hModule));
	qDebug() << strBinPath.remove(QRegularExpression("pluginstore.dll"));
	qDebug() << strDataPath.remove(QRegularExpression("pluginstore"));
	auto DelFiles = [](QDir d, QStringList filters)
	{


		d.setNameFilters(filters);
		QDirIterator it(d, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			QString strFile = it.next();
			qDebug() << strFile;
			QFile::remove(strFile);
		}
	};
	QStringList filters;
	filters << "*.old";
	DelFiles(strBinPath, filters);
	DelFiles(strDataPath, filters);
}
void	WebPluginEvent::SetLabelDelete(QString strPluginFile)
{
	QFileInfo f(strPluginFile);
	if (f.exists())
	{
		QString strExt = f.suffix();
		if (strExt != "old")
		{
			//if(!QFile::exists(strPluginFile + ".old"))
			QFile::rename(strPluginFile, strPluginFile + ".old");
		}
	}
}
