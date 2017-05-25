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

QVariant WebPluginEvent::GetLocalPluginList()
{
    QJsonParseError qJsonError;
    QJsonDocument   qJsonDocument;
    QJsonArray      qJsonArray;
    QList<PluginDB::PluginInfo> list =  m_PluginDB.QueryPluginData();
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
    qDebug() << qJsonDocument.toJson(QJsonDocument::Compact);

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
        info.json_data = QJsonDocument(obj).toJson(QJsonDocument::Compact);

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
