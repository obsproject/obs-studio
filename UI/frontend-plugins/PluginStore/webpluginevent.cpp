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
PluginItem::PluginItem(double dblId, QWebEngineDownloadItem* lpItem, WebPluginEvent* lpEvent)
    : m_qstrPluginId(dblId)
    , m_lpWebItem(lpItem)
    , m_lpEvent(lpEvent)
{   
    connect(m_lpWebItem, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(on_web_downfile_progress(qint64, qint64)));
    connect(m_lpWebItem, SIGNAL(finished()), this, SLOT(on_web_downfile_finished()));
}

PluginItem::~PluginItem()
{

}

void PluginItem::on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize)
{
    qDebug() << "file:" << m_qstrPluginId << "recv:" << qiRecvSize << "qiTotalSize " << qiTotalSize;
    m_lpEvent->DownLoadState(QString::number(m_qstrPluginId),qiRecvSize,qiTotalSize);
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
        connect(m_lpView->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
            this, SLOT(on_web_downfile_start(QWebEngineDownloadItem*)));

        m_PluginDB.InitPluginDB();
    }
}
WebPluginEvent::~WebPluginEvent()
{
	
}

QString WebPluginEvent::ResultToJsonString(double dblPlugId, bool bResult)
{
    QJsonObject json;
    json.insert("plugid", dblPlugId);
    json.insert("result", bResult);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);

    return json_str;
}

QString	WebPluginEvent::GetLocalPluginList()
{
    QString qstrResult;
    QJsonArray qArray;
    QList<PluginDB::PluginInfo> list =  m_PluginDB.QueryPluginData();
    for each (PluginDB::PluginInfo var in list)
    {
        QString qstrValue = var.json_data;
        qDebug() << qstrValue;
        qArray.append(qstrValue);
    }
    qstrResult = QJsonDocument(qArray).toJson();
    qDebug() << qstrResult;

    return	qstrResult;
}
QString	WebPluginEvent::GetLocalPluginVersion(QString strPluginID)
{
    return	QString("1.1.1.1");
}
void WebPluginEvent::DownLoadPluginUrl(const QVariantMap& param)
{
    QString qstrUrl;
    if (m_lpView)
    {  
        QJsonObject qjsonObj = QJsonObject::fromVariantMap(param);
        if (!qjsonObj.isEmpty())
        {   
            qstrUrl = qjsonObj.value("file_url").toString();
            if (qstrUrl.isEmpty())
                return;

            m_downInfoMap.insert(qstrUrl, qjsonObj);
            m_lpView->load(QUrl(qstrUrl));
        }
    }
    return;
}
QString WebPluginEvent::InstallPlugin(const QVariant& param)
{
    double dblPlugId = param.toDouble();

    return ResultToJsonString(dblPlugId, true);
}
QString WebPluginEvent::UninstallPlugin(const QVariant& param)
{
    double dblPlugId = param.toDouble();
    
    return ResultToJsonString(dblPlugId,true);
}

QString WebPluginEvent::RemoveFilePlugin(const QVariant& param)
{
    double dblPlugId = param.toDouble();

    return ResultToJsonString(dblPlugId, true);
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

void WebPluginEvent::on_web_downfile_start(QWebEngineDownloadItem *item)
{
    QString qstrUrl = item->url().toString();

    QMap<QString, QJsonObject>::const_iterator iter = m_downInfoMap.constFind(qstrUrl);
    if (iter != m_downInfoMap.constEnd())
    {
        QJsonObject obj = *iter;
        PluginItem* lpPlugItem = new PluginItem(obj.value("plug_id").toDouble(), item, this);


        //add web data to database
        PluginDB::PluginInfo info;
        info.plugin_id = obj.value("plug_id").toInt();
        info.local_path = item->path();
        info.down_size = 0;
        info.file_size = obj.value("file_size").toInt();
        info.json_data = QJsonDocument(obj).toJson();

        m_PluginDB.InsertPluginData(info);

        item->accept();
    }
}

void WebPluginEvent::RemoveAllLabelFile()
{
	obs_module_t* hModule = obs_current_module();
	QString strBinPath = QString::fromUtf8(obs_get_module_binary_path(hModule));
	QString strDataPath = QString::fromUtf8(obs_get_module_data_path(hModule));
	qDebug() << strBinPath.remove(QRegularExpression("pluginstore.*"));
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

void WebPluginEvent::SetLabelDelete(QString strPluginFile)
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
