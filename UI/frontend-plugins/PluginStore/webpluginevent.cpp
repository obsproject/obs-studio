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
    qDebug() << "delete PluginItem :" << m_qiPluginId;
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
    m_PluginData.down_size = qiRecvSize;
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
                m_PluginData.down_size = m_lpWebItem->receivedBytes();
                m_lpEvent->DownloadSuc(m_PluginData);
            }
            break;
        case QWebEngineDownloadItem::DownloadCancelled:
            {
                qDebug() << "file:" << m_qiPluginId << " cancel";
                m_lpEvent->DownloadCancel(m_qiPluginId);
            }
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

            m_DownPluginDir = m_PluginDB.GetConfigValue(__DEF_CFG_DOWN_PATH_NAME__);
            if (m_DownPluginDir.isEmpty())
            {
                m_DownPluginDir = obs_module_config_path("");
                qDebug() << m_DownPluginDir;
            }
        }
    }
}

WebPluginEvent::~WebPluginEvent()
{
    qDebug() << "stop pluging store.";
    return;
}

QString WebPluginEvent::ResultToJsonString(qint64 qiPlugId, bool bResult)
{
    QJsonObject json;
    json.insert("id", qiPlugId);
    json.insert("result", bResult);

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    QString json_str(byte_array);

    return json_str;
}

void WebPluginEvent::DownloadCancel(qint64 qiPluginId)
{
    RemovePluginMap(qiPluginId);
}

void WebPluginEvent::DownloadSuc(PluginDB::PluginInfo obj)
{
    m_PluginDB.UpdatePluginData(obj.plugin_id, obj);
}


void WebPluginEvent::RemovePluginMap(qint64 qiPluginId)
{
    PluginItem* lpItem = NULL;
    if (FindDownloadListItem(qiPluginId, &lpItem))
    {
        if (lpItem != NULL)
        {
            delete lpItem;
            lpItem = NULL;
        }
        m_PluginMap.remove(qiPluginId);
    }
}

void WebPluginEvent::ClearPluginMap()
{
    qint64 qiPluginId;
    QMap<qint64, PluginItem*>::const_iterator itor = m_PluginMap.constBegin();
    while (itor != m_PluginMap.constEnd())
    {
        qiPluginId = itor.key();
        PluginItem* lpItem = const_cast<PluginItem*>(itor.value());

        ++itor;
        if (lpItem != NULL)
        {
            QWebEngineDownloadItem* lpDownItem = lpItem->GetPluginDownItem();
            if (lpDownItem != NULL)
            {
                lpDownItem->cancel();
            }
            else
            {
                RemovePluginMap(qiPluginId);
            }
        }
    }
}

void WebPluginEvent::UninitPluginStore()
{
    ClearPluginMap();
    m_PluginDB.CloseDB();
}

bool WebPluginEvent::FindDownloadListItem(qint64 qiPluginId,PluginItem** lpItem)
{
    bool bRet = false;
    QMap<qint64, PluginItem*>::const_iterator itor = m_PluginMap.constFind(qiPluginId);
    if (itor != m_PluginMap.constEnd())
    {
        *lpItem = const_cast<PluginItem*>(itor.value());
        bRet = true;
    }
    return bRet;
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
            qDebug() << qstrUrl;
            m_downInfoMap.insert(QUrl(qstrUrl).toString(), qjsonObj);
            m_lpView->load(QUrl(qstrUrl));
        }
    }
    return;
}

QString WebPluginEvent::InstallPlugin(const QVariant& param)
{
    bool                           bRet       = false;
    qint64                         qiPluginId = param.toInt();
    PluginItem*                    lpItem     = NULL;
    PluginDB::PluginInfo           data;
    QString                        qstrPath;
    QString                        qstrName;

    FindDownloadListItem(qiPluginId,&lpItem);
    if (lpItem != NULL)
    {
        data = lpItem->GetPluingData();
        qstrPath = data.local_path;
        qstrName = data.file_name;

        if (data.state == PluginDB::CANINSTALL)
        {
            // don't need
            //qstrName = "CalabashWinCapture";
            if (WebPluginEvent::InstallPluginZip(data.local_path, data.file_name));
            {

                data.state = PluginDB::INSTALLED;
                lpItem->SetPluginData(data);

                m_PluginDB.UpdatePluginData(qiPluginId, data);
                qDebug() << "install file" << qstrPath << "name :" << qstrName;
                bRet = true;
            }
        }
    }
    return ResultToJsonString(qiPluginId, bRet);
}
 
QString WebPluginEvent::UninstallPlugin(const QVariant& param)
{
    bool            bRet = false;
    PluginItem*     lpItem = NULL;
    qint64          qiPluginId = param.toDouble();

    QList<PluginDB::PluginInfo> list = m_PluginDB.QueryPluginData(qiPluginId);
    if (list.count() > 0)
    {
        PluginDB::PluginInfo info = list.at(0);
        bRet = FindDownloadListItem(qiPluginId, &lpItem);
        if (lpItem != NULL)
        {
            bRet = SetLabelDelete(info.file_name);
            //if (bRet)
                RemovePluginMap(qiPluginId);
        }
        else
            bRet = false;
    }
    //if (bRet)
    {
        bRet = m_PluginDB.DeletePluginData(qiPluginId);
    }
    return ResultToJsonString(qiPluginId, bRet);
}

QString WebPluginEvent::RemoveFilePlugin(const QVariant& param)
{
    bool bRet = false;
    qint64 qiPluginId = param.toDouble();


    if (bRet)
    {
        bRet = m_PluginDB.DeletePluginData(qiPluginId);
    }
    return ResultToJsonString(qiPluginId, bRet);
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

QString WebPluginEvent::SetDownloadPluginDir()
{
    m_DownPluginDir = QFileDialog::getExistingDirectory(NULL, tr("Open File"), m_DownPluginDir,
        QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);

    if (!m_DownPluginDir.isEmpty())
    {
        m_PluginDB.SetConfigValue(__DEF_CFG_DOWN_PATH_NAME__, m_DownPluginDir);
    }

    qDebug() << m_DownPluginDir;
    return m_DownPluginDir;
}

void WebPluginEvent::on_web_load_finished(bool ok)
{
    return;
}

void WebPluginEvent::on_web_downfile_start(QWebEngineDownloadItem *item)
{
    qint64          qiPluginId  = 0;
    PluginItem*     lpPlugItem  = NULL;
    QJsonObject     obj;
    QString         qstrUrl;
    QString         qstrFileName;
    QString         qstrPath;

    if (item == NULL)
        return;

    qstrUrl = item->url().toString();
    QRegExp rx("[^\/]+$");
    
    int pos = qstrUrl.indexOf(rx);
    if (pos > 0)
    {
        qstrFileName = rx.cap(0);
        qstrPath.append(m_DownPluginDir);
        if (qstrPath.right(0) != "\\" || qstrPath.right(0) != "/")
        {
            qstrPath.append("/");
        }
        qstrPath.append(qstrFileName);
        qDebug() << qstrPath;
    }

    QMap<QString, QJsonObject>::const_iterator iter = m_downInfoMap.constFind(qstrUrl);
    if (iter != m_downInfoMap.constEnd())
    {
        obj = iter.value();
        qiPluginId = obj.value("id").toInt();

        if (QFile::exists(qstrPath))
        {
            if (QFile::remove(qstrPath))
            {
                m_PluginDB.DeletePluginData(qiPluginId);
            }
            else
            {
                return;
            }
        }
        item->setPath(qstrPath);

        obj.insert("local_path", qstrPath);
        if (FindDownloadListItem(qiPluginId,&lpPlugItem))
        {
            if (lpPlugItem != NULL)
            {
                lpPlugItem->SetPluginData(obj, PluginDB::DOWNSTART);
            }
            else
            {
                m_PluginMap.remove(qiPluginId);
                return;
            }
        }
        else
        {
            lpPlugItem = new PluginItem(qiPluginId, this);
        }

        lpPlugItem->SetPluginDownItem(item);

        //add download plugin local dir
        lpPlugItem->SetPluginData(obj, PluginDB::DOWNSTART);
        m_PluginMap.insert(qiPluginId, lpPlugItem);

        //add web data to database
        //note: if the plugin already downloaded , it will again down.
        if (m_PluginDB.GetPluginDataCount(qiPluginId) > 0)
        {
            m_PluginDB.UpdatePluginData(qiPluginId, lpPlugItem->GetPluingData());
        }
        else
        {
            m_PluginDB.InsertPluginData(lpPlugItem->GetPluingData());
        }
           
        item->accept();

        //already recive json ,remove item
        m_downInfoMap.remove(qstrUrl);
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
/*
zip dir test plugin
32bit/-
	  - test/
		-locale/
			-*.ini
	  - test.dll
64bit/-
	  - test/
		-locale/
			-*.ini
	  - test.dll
	
*/
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
				obsDir += b32bit ? "obs-plugins/32bit/" : "obs-plugins/64bit/";
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
			fs::path newPath = qDestPath.toStdString();
			if (!fs::is_directory(newPath))
			{
				newPath.remove_filename();
			}
			fs::create_directories(newPath);
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
		if (!CheckPluginRuning(strPluginName))
		{
			return LoadPlugin(strPluginName);
		}
	}
	catch (...)
	{

	}
	return true;
}
bool WebPluginEvent::SetLabelDelete(QString strPluginFile)
{
    bool bRet = false;
	QFileInfo f(strPluginFile);
	if (f.exists()&&!f.isDir())
	{
		QString strExt = f.suffix();
		if (strExt != "old")
		{
			bRet = QFile::rename(strPluginFile, strPluginFile + ".old");
		}
	}
    return bRet;
}
bool WebPluginEvent::CheckPluginRuning(QString strPluginName)
{
	
	QPair<bool, QString> callBackParam = { false, strPluginName };
	auto findModule = [](void *param, obs_module_t *module)
	{
		auto p = static_cast<QPair<bool, QString>*>(param);
		QString strModuleName = obs_get_module_name(module);
		if (strModuleName.compare(p->second, Qt::CaseInsensitive) == 0)
		{
			p->first = true;
		}
	};
	obs_enum_modules(findModule, &callBackParam);
	return callBackParam.first;

}

void WebPluginEvent::MakePluginPath(QString strPluginName, QString& strPluginBin, QString& strPluginData)
{
	obs_module_t* hModule = obs_current_module();
	QString strBinPath = QString::fromUtf8(obs_get_module_binary_path(hModule));
	QString strDataPath = QString::fromUtf8(obs_get_module_data_path(hModule));
	strPluginBin=strBinPath.replace(QRegularExpression("pluginstore"), strPluginName);
	strPluginData=strDataPath.replace(QRegularExpression("pluginstore"), strPluginName);
}

bool WebPluginEvent::LoadPlugin(QString	strPluginName)
{
	QString strBinPath;
	QString	strDataPath;
	MakePluginPath(strPluginName, strBinPath, strDataPath);
	if (!QFile::exists(strBinPath))
		return false;

	obs_module_t *module;
	int code = obs_open_module(&module, strBinPath.toUtf8(), strDataPath.toUtf8());
	if (code !=MODULE_SUCCESS) {
		
		return false;
	}
	return obs_init_module(module);
}