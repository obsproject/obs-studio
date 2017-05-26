#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include "webpluginevent.h"
#include "zip_file.hpp"
#include <qcoreapplication>
#include <filesystem>
using namespace std;

/************************************************************************/
/* download files Mangert                                               */
/************************************************************************/
PluginItem::PluginItem(qint64 qiPluginId, WebPluginEvent* lpEvent)
    : m_qiPluginId(qiPluginId)
    , m_lpEvent(lpEvent)
{
    m_lpWebItem = NULL;
}

PluginItem::~PluginItem()
{

}

void PluginItem::SetPluginDownItem(QWebEngineDownloadItem* lpItem)
{
    m_lpWebItem = lpItem;
    if (lpItem != NULL)
    {
        connect(m_lpWebItem, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(on_web_downfile_progress(qint64, qint64)));
        connect(m_lpWebItem, SIGNAL(finished()), this, SLOT(on_web_downfile_finished()));
    }
}

QWebEngineDownloadItem* PluginItem::GetPluginDownItem()
{
    return m_lpWebItem;
}

void PluginItem::SetPluginData(QJsonObject obj, PluginDB::PluginState state)
{
    m_PluginData.plugin_id = obj.value("id").toInt();
    m_PluginData.file_name = obj.value("file_name").toString();
    m_PluginData.local_path = obj.value("local_path").toString();
    m_PluginData.down_size = 0;
    m_PluginData.file_size = obj.value("file_size").toInt();
    m_PluginData.json_data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    m_PluginData.state = state;
}

void PluginItem::SetPluginData(PluginDB::PluginInfo obj)
{
    m_PluginData = obj;
}

PluginDB::PluginInfo PluginItem::GetPluingData()
{
    return m_PluginData;
}

void PluginItem::on_web_downfile_progress(qint64 qiRecvSize, qint64 qiTotalSize)
{
    qDebug() << "file:" << m_qiPluginId << "recv:" << qiRecvSize << "qiTotalSize " << qiTotalSize;
    m_lpEvent->DownLoadState(QString::number(m_qiPluginId), qiRecvSize, qiTotalSize);
}

void PluginItem::on_web_downfile_finished()
{
    if (m_lpWebItem)
    {
        switch (m_lpWebItem->state())
        {
        case QWebEngineDownloadItem::DownloadRequested:
            return;
        case QWebEngineDownloadItem::DownloadInProgress:
        {
            m_PluginData.state = PluginDB::DOWNING;
            Q_UNREACHABLE();
        }
            break;
        case QWebEngineDownloadItem::DownloadCompleted:
            {
                qDebug() << "file:" << m_qiPluginId << " completed";
                m_PluginData.state = PluginDB::CANINSTALL;
            }
            break;
        case QWebEngineDownloadItem::DownloadCancelled:
            break;
        case QWebEngineDownloadItem::DownloadInterrupted:
            {
                m_PluginData.state = PluginDB::DOWNPAUSED;
            }
            break;
        }
    }
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

        if (m_PluginDB.InitPluginDB())
        {
            QList<PluginDB::PluginInfo> list = m_PluginDB.QueryPluginData();
            for each (PluginDB::PluginInfo var in list)
            {
                PluginItem* lpItem = new PluginItem(var.plugin_id, this);
                if (lpItem != NULL)
                {
                    lpItem->SetPluginData(var);
                    m_PluginMap.insert(var.plugin_id, lpItem);
                }
            }
        }
    }
}
WebPluginEvent::~WebPluginEvent()
{
	
}

QString WebPluginEvent::ResultToJsonString(qint64 qiPlugId, bool bResult)
{
    QJsonObject json;
    json.insert("plugid", qiPlugId);
    json.insert("result", bResult);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);

    return json_str;
}

void WebPluginEvent::DownloadSuc(qint64 qiPluginId)
{

}


void WebPluginEvent::RemovePluginMap(qint64 qiPluginId)
{
    QMap<qint64, PluginItem*>::const_iterator itor = m_PluginMap.constFind(qiPluginId);
    if (itor != m_PluginMap.constEnd())
    {
        PluginItem* lpItem = *itor;
        if (lpItem != NULL)
        {
            QWebEngineDownloadItem* lpDownItem = lpItem->GetPluginDownItem();
            if (lpDownItem != NULL)
            {
                lpDownItem->finished();
                delete lpItem;
                lpItem = NULL;
            }
        }
        m_PluginMap.remove(qiPluginId);
    }
}

void WebPluginEvent::ClearPluingMap()
{
    qint64 qiPluginId;
    QMap<qint64, PluginItem*>::const_iterator itor = m_PluginMap.constBegin();
    while (itor != m_PluginMap.constEnd())
    {
        qiPluginId = itor.key();
        RemovePluginMap(qiPluginId);
    }
}

QVariant WebPluginEvent::GetLocalPluginList()
{
    QJsonParseError qJsonError;
    QJsonDocument   qJsonDocument;
    QJsonArray      qJsonArray;
    QList<PluginDB::PluginInfo> list = m_PluginDB.QueryPluginData();

    for each (PluginDB::PluginInfo var in list)
    {
        QJsonDocument doc = QJsonDocument::fromJson(var.json_data.toUtf8(), &qJsonError);
        if (qJsonError.error == QJsonParseError::NoError && doc.isObject())
        {
            QJsonObject obj = doc.object();
            qJsonArray.append(obj);
        }
    }
    qJsonDocument.setArray(qJsonArray);

    return	qJsonDocument.toJson(QJsonDocument::Compact);
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
    bool bRet = false;
    qint64 qiPlugId = param.toInt();
    QMap<qint64, PluginItem*>::const_iterator it = m_PluginMap.constFind(qiPlugId);
    if (it != m_PluginMap.constEnd())
    {
        PluginItem* lpItem = *it;
        if (lpItem != NULL && lpItem->GetPluingData().state == PluginDB::CANINSTALL)
        {
            QString qstrFileName = lpItem->GetPluingData().file_name;
            QString qstrPath = lpItem->GetPluingData().local_path;
            qDebug() << "install file" << qstrPath << "name :" << qstrFileName;
            qstrFileName = "test";
            if (WebPluginEvent::InstallPluginZip(qstrPath, qstrFileName));
            {

            }
        }
        bRet = true;
    }
    return ResultToJsonString(qiPlugId, bRet);
}
QString WebPluginEvent::UninstallPlugin(const QVariant& param)
{
    qint64 qiPlugId = param.toDouble();
    
    return ResultToJsonString(qiPlugId, true);
}

QString WebPluginEvent::RemoveFilePlugin(const QVariant& param)
{
    qint64 qiPlugId = param.toDouble();

    return ResultToJsonString(qiPlugId, true);
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
    qint64  qiPluginId = 0;
    QString qstrUrl = item->url().toString();
    QString qstrPath = item->path();

    QMap<QString, QJsonObject>::const_iterator iter = m_downInfoMap.constFind(qstrUrl);
    if (iter != m_downInfoMap.constEnd())
    {
        QJsonObject obj = *iter;
        qiPluginId = obj.value("plug_id").toInt();
        if (QFile::exists(qstrPath))
            QFile::remove(qstrPath);

        PluginItem* lpPlugItem = new PluginItem(qiPluginId, this);
        if (item != NULL)
        {
            lpPlugItem->SetPluginDownItem(item);

            obj.insert("local_path",qstrPath);
            lpPlugItem->SetPluginData(obj, PluginDB::DOWNSTART);
            m_PluginMap.insert(qiPluginId, lpPlugItem);

            //add web data to database
            m_PluginDB.InsertPluginData(lpPlugItem->GetPluingData());

            //already recive json ,and remove item
            m_downInfoMap.remove(qstrUrl);
            item->accept();
        }
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

bool WebPluginEvent::InstallPluginZip(QString strZipFile, QString strPluginName)
{
	using namespace miniz_cpp;
	if (!QFile::exists(strZipFile))
	{
		return false;
	}
	auto makeDestPath = [](QString strPluginName,QString zipItemFile,bool b32bit)->QString{
		
		QString obsDir = QCoreApplication::applicationDirPath().replace(QRegularExpression("bin/32bit"), "");
		obsDir.replace(QRegularExpression("bin/64bit"), "");
		QRegularExpressionMatch match;
		QRegularExpression reg;
		reg.setPattern(strPluginName+'/');
		match = reg.match(zipItemFile);
		if (match.hasMatch())
		{
			//data
			obsDir += "data/obs-plugins/";
		}
		else
		{
			b32bit ? reg.setPattern(R"(^32bit/bin/)") : reg.setPattern(R"(^64bit/bin/)");
			match = reg.match(zipItemFile);
			if (match.hasMatch())
			{
				//obs bin
				obsDir += b32bit ? "bin/32bit/" : "bin/64bit/";
			}
			else
			{
				//plugin bin
				obsDir += b32bit ? "obs - plugins/32bit/" : "obs - plugins/64bit/";
			}
		}
		obsDir += zipItemFile.replace(QRegularExpression("^64bit/|^32bit/"),"");
		return obsDir;
	};
	auto extractToFile = [&](zip_file& zfile,QString qDestPath, const zip_info &member)
	{
		namespace fs=std::tr2::sys;
		if (!QFile::exists(qDestPath))
		{
			fs::create_directories(fs::path(qDestPath.toStdString()));
		}
		
		std::fstream stream(qDestPath.toStdString(), std::ios::binary | std::ios::out);
		if (!stream.is_open())
		{
			SetLabelDelete(QString::fromStdString(qDestPath.toStdString()));

		}
		if (stream.is_open())
			stream << zfile.open(member).rdbuf();
	};
	try{
	
		zip_file zfile(strZipFile.toStdString());
		std::vector<zip_info> infolist = zfile.infolist();
		for (auto &item : infolist)
		{
			QString itemFileName = QString::fromStdString(item.filename);
			QRegularExpressionMatch match;
			QRegularExpression reg("^64bit/");
			QString extractDst;
			match = reg.match(itemFileName);
			if (match.hasMatch())
			{
				extractDst=makeDestPath(strPluginName, itemFileName, false);
			}
			else
			{
				extractDst=makeDestPath(strPluginName, itemFileName, true);
			}
			extractToFile(zfile, extractDst, item);
			
		}
		//install
	}
	catch (...)
	{

	}
	return true;
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
