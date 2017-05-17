#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <Qmessagebox>
#include <QMainWindow>
#include<QMenu>
#include<QMenuBar>

//#include "ui_cupdateobcn.h"
#include <curl/curl.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "json11.hpp"
#include "UrlHttp.h"
#include "uimd5.h"
#include <thread>
#include <qdir>
#include <qprocess>
#include "cupdateobcn.h"
#include <functional>
#include <qurl>
#include<sstream>
using namespace std;
UpdateImpl* UpdateImpl::GetUpdateImpl()
{

	static	UpdateImpl			g_UpdateImpl;
	return &g_UpdateImpl;
}
UpdateImpl::UpdateImpl()
{
	cache_dir = obs_module_config_path("");
	string  updateInstallPack = cache_dir + "obscn.exe";
	updatePath = (updateInstallPack);
	updateJsonPath = updatePath.replace_extension("json");
	updateDlPath = updatePath.replace_extension("dl");
	updatePath = updatePath.replace_extension("exe");
	tempExePath = QDir::tempPath().toStdString() + "/obscn.exe";
	fs::create_directories(updatePath.parent_path());
	strOBSCN =OBS_VERSION;				//用作更新
	strBuild="4096";				//build VER
	g_window =(QMainWindow*)obs_frontend_get_main_window();
	QString qString=g_window->windowTitle();
	string str = qString.toStdString();
	g_UpdateThreadRun=false;
	g_exitDown = false;
	g_LocalFileSize = 0;
	strJsonCfgSize = 1;
	LoadServiceInfo();
	
}
void	UpdateImpl::LoadServiceInfo()
{
	config_t * baseIni = obs_frontend_get_profile_config();
	config_t * globalCfg=obs_frontend_get_global_config();
	strInstallGUID=config_get_string(globalCfg, "General", "InstallGUID");
	//推流目录名
	QString strUtf8 = QString::fromUtf8(config_get_string(baseIni, "General", "Name"));
	fs::path tPath = cache_dir;
	tPath.remove_filename();// '/'
	tPath.remove_filename();// UpdateCnOBS
	tPath.remove_filename();//plugin_config
	tPath /= R"(basic\profiles)";

	QString fileName(tPath.directory_string().c_str());
	fileName += R"(\)";
	fileName += strUtf8;
	fileName += R"(\service.json)";
	

	QFile f(fileName);
	bool bexit = f.open(QIODevice::ReadOnly);
	if (f.isOpen())
	{
		QByteArray ar=f.readAll();
		f.close();
		string strErr;
		auto jsonvar = json11::Json::parse(ar.toStdString(), strErr);
		if (!jsonvar.is_null() && strErr.empty())
		{
			auto setting = jsonvar["settings"].object_items();
			strtype = jsonvar["type"].string_value();
			strserver = setting["server"].string_value();
			strkey = setting["key"].string_value();
		}
		
	}
	
}
string  UpdateImpl::GetUpdateFullUrl()
{
	//v=18.0.2.456&g=DAFADSF&type=rtmp_common&key=dsafa&server=rtmp://live.twitch.tv/app
	string qString=R"(http://192.168.110.243:90/storeapi/update.php?)";
	CHttpClient h;
	stringstream urlParam;
	urlParam << "v="
		<< strOBSCN
		<< "."
		<< strBuild
		<< "&g="
		<< h.UrlEncode(strInstallGUID)
		<< "&type="
		<< h.UrlEncode(strtype)
		<< "&key="
		<< h.UrlEncode(strkey)
		<< "&server="
		<< h.UrlEncode(strserver);
	
	return qString + urlParam.str();
}
UpdateImpl::~UpdateImpl()
{

}
QAction* UpdateImpl::GetHelpMenuItem(QString strClassName)
{
	QMainWindow *window =
		(QMainWindow*)obs_frontend_get_main_window();
	QMenuBar* pBar = window->menuBar();
	QMenu*	  pHelp = pBar->findChild<QMenu*>(QString("menuBasic_MainMenu_Help"));
	return pHelp->findChild<QAction*>(strClassName);
}
void UpdateImpl::LoadJson()
{

	fstream f(updateJsonPath);
	if (f.is_open())
	{
		string strJson;
		long long fSize=fs::file_size(updateJsonPath);
		strJson.assign(fSize, 0x0);
		f.read(strJson._Myptr(),fSize);
		f.close();
		string strErr;
		auto jsVar = json11::Json::parse(strJson, strErr);
		if (!jsVar.is_null() && strErr.empty())
		{
			strJsonCfgMd5 = jsVar["md5"].string_value();
			strJsonCfgSize = std::stoll(jsVar["size"].string_value());
			if (strJsonCfgSize == 0)strJsonCfgSize = 1;
			strJsonCfgUrl = jsVar["url"].string_value();
			strJsonCfgVer = jsVar["version"].string_value();
		}

	}
}
void UpdateImpl::SaveJson()
{
	fstream f(updateJsonPath,ios_base::out);
	if (f.is_open())
	{
		json11::Json::object jsVar;
		jsVar["md5"] = strJsonCfgMd5;
		jsVar["url"] = strJsonCfgUrl;
		jsVar["version"] = strJsonCfgVer;
		jsVar["size"] = to_string(strJsonCfgSize);
		json11::Json js = jsVar;
		string strJson = js.dump();
		f.write(strJson.c_str(), strJson.length());
	}

}
int  UpdateImpl::CompVersion(string strVersion1, string strVersion2)
{
	
	char* pstr = NULL;
	char* pstr2 = NULL;
	int				iRet = 0;
	long			lVer1;
	long			lVer2;
	if (strVersion1 == strVersion2)
		return 0;
	lVer1 = strtol(strVersion1.c_str(), &pstr, 10);
	lVer2 = strtol(strVersion2.c_str(), &pstr2, 10);
	if (lVer1 != lVer2)
		return lVer1 - lVer2;
	while (*pstr != '\0'&&*pstr2 != '\0')
	{
		lVer1 = strtol(pstr + 1, &pstr, 10);
		lVer2 = strtol(pstr2 + 1, &pstr2, 10);
		if (lVer1 != lVer2)
			return lVer1 - lVer2;
	}
	return iRet;
}
void UpdateImpl::ExecuteInstall()
{
	fs::copy_file(updatePath, tempExePath, fs::copy_option::overwrite_if_exists);
	QProcess p(Q_NULLPTR);
	p.startDetached(QString(tempExePath.directory_string().c_str()), QStringList());
}
void UpdateImpl::UpdateProgress(double totalToDownload, double nowDownloaded)
{
	int Progress = ((long double)(g_LocalFileSize + nowDownloaded)/ (long double)strJsonCfgSize) * 100;
	QMetaObject::invokeMethod(g_RecvMsg, "On_DownLoadProgress", Q_ARG(int, Progress));
}
void  UpdateImpl::DownLoadFinished()
{
	if (fs::exists(updateDlPath))
	{
		QString qMd5Str(CUIMD5::MD5File(updateDlPath.directory_string().c_str()).c_str());
		
		if (qMd5Str.compare(strJsonCfgMd5.c_str(), Qt::CaseInsensitive) == 0)
		{
			fs::remove(updatePath);
			fs::rename(updateDlPath, updatePath);
			QMetaObject::invokeMethod(g_RecvMsg, "On_DownLoadProgress", Q_ARG(int, 100));
			QMetaObject::invokeMethod(g_RecvMsg, "On_DownLoadFinished");
		}
		else
		{
			fs::remove(updateDlPath);
			QMetaObject::invokeMethod(g_RecvMsg, "On_DownLoadError");
		}
	}
}
void UpdateImpl::DownLoadError()
{
	QMetaObject::invokeMethod(g_RecvMsg, "On_DownLoadError");
}
void UpdateImpl::StartUpdate(QWidget* wid)
{
	g_RecvMsg = wid;
	auto DownUrlToFile = [&](const string& url, const string& localPath)
	{
		CHttpClient h;
		h.InitHttpConnect();
		curl_easy_setopt(h.GetLibCurl(), CURLOPT_TIMEOUT, 0);
		curl_easy_setopt(h.GetLibCurl(), CURLOPT_SSL_ENABLE_ALPN, 0);
		g_LocalFileSize = fs::file_size(updateDlPath);
		h.DownUrlToFile(url, localPath, g_LocalFileSize,[](void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)->int
		{
			if (UpdateImpl::GetUpdateImpl()->g_exitDown)
				return -3;
			if (nowDownloaded>0)
			{
				UpdateImpl::GetUpdateImpl()->UpdateProgress(totalToDownload, nowDownloaded);
			}
			return 0;
		});
		if (h.m_curlretcode == CURLcode::CURLE_OK && (h.m_httpretcode == 200 || h.m_httpretcode == 206))
		{
			UpdateImpl::GetUpdateImpl()->DownLoadFinished();
		}
		else
		{
			// down break
			UpdateImpl::GetUpdateImpl()->DownLoadError();
		}
		
	};
	if (fs::exists(updatePath))
	{
		g_LocalFileSize = fs::file_size(updatePath);
		QMetaObject::invokeMethod(wid, "On_DownLoadProgress", Q_ARG(int,100));
		QMetaObject::invokeMethod(wid, "On_DownLoadFinished");
		return;
	}
	if (fs::exists(updateDlPath))
	{
		g_LocalFileSize = fs::file_size(updateDlPath);
		QString qMd5Str = CUIMD5::MD5File(updateDlPath.directory_string().c_str()).c_str();
		if (qMd5Str.compare(strJsonCfgMd5.c_str(), Qt::CaseInsensitive) == 0)
		{
			fs::remove(updatePath);
			fs::rename(updateDlPath,updatePath);
			QMetaObject::invokeMethod(wid, "On_DownLoadProgress", Q_ARG(int, 100));
			QMetaObject::invokeMethod(wid, "On_DownLoadFinished");
			return;
		}
		if (g_LocalFileSize>=strJsonCfgSize)
		{
			fs::remove(updateDlPath);
			g_LocalFileSize = 0;
		}
		
	}
	QMetaObject::invokeMethod(wid, "On_DownLoadProgress", Q_ARG(int, ((long double)g_LocalFileSize / (long double)strJsonCfgSize) * 100));
	UpdateImpl::GetUpdateImpl()->g_exitDown = false;
	std::thread down(DownUrlToFile, strJsonCfgUrl, updateDlPath);
	down.detach();
}
void UpdateImpl::StopUpdate()
{
	g_exitDown = true;
}
bool UpdateImpl::CheckUpdate(bool manualUpdate)
{
	if (g_UpdateThreadRun)
	{
		if (manualUpdate)
		{
			QMessageBox::about(g_window, obs_module_text("UpdateCnstudio.CheckForUpdates"),
				obs_module_text("UpdateCnstudio.updatethreadrun"));

		}
		return false;
	}
	auto UpdateCfg = [](bool manulUpdate, string strUrl)
	{
		
		CHttpClient q;
		string		strJson;
		q.InitHttpConnect();
		//url参数未定
		//json {"md5":"","url":"","size":"","version",""}
		

		int iret = q.Get(strUrl, strJson);
		if (iret == CURLcode::CURLE_OK&&q.m_httpretcode == 200)
		{
			//strJson = R"({"md5":"4FB691E0629A45FF2F464DBFCC2EC29D","url":"http://cktools.cikevideo.com/installer/pc/setup/setup_1.6.2.10893_0510.exe","size":"63016248","version":"18.0.3"})";
			//更新本地josn
			string strErr;
			auto jsVarRoot = json11::Json::parse(strJson, strErr);
			UpdateImpl::GetUpdateImpl()->LoadJson();
			auto jsd = jsVarRoot["d"].object_items();
			auto jsVar = jsd["data"];
			if (!jsVar.is_null()&&strErr.empty())
			{
				string		obsFileVer = UpdateImpl::GetUpdateImpl()->strOBSCN;
				obsFileVer += ".";
				obsFileVer += UpdateImpl::GetUpdateImpl()->strBuild;

				if (UpdateImpl::GetUpdateImpl()->CompVersion(obsFileVer, jsVar["version"].string_value()) != 0)
				{
					//需要更新
					int iret = UpdateImpl::GetUpdateImpl()->CompVersion(UpdateImpl::GetUpdateImpl()->strJsonCfgVer, jsVar["version"].string_value());
					if (iret !=0)
					{
						//无论是否本地存在未完成的文件都删除取最新版
						fs::remove(UpdateImpl::GetUpdateImpl()->updateDlPath);
						fs::remove(UpdateImpl::GetUpdateImpl()->updatePath);
						fs::remove(UpdateImpl::GetUpdateImpl()->updateJsonPath);
					}
					UpdateImpl::GetUpdateImpl()->strJsonCfgMd5	= jsVar["md5"].string_value();
					UpdateImpl::GetUpdateImpl()->strJsonCfgSize = std::stoll(jsVar["size"].string_value());
					if (UpdateImpl::GetUpdateImpl()->strJsonCfgSize == 0)UpdateImpl::GetUpdateImpl()->strJsonCfgSize = 1;
					UpdateImpl::GetUpdateImpl()->strJsonCfgUrl = jsVar["url"].string_value();
					UpdateImpl::GetUpdateImpl()->strJsonCfgVer = jsVar["version"].string_value();
					UpdateImpl::GetUpdateImpl()->SaveJson();
					//弹出更新框
					//UpdateImpl::GetUpdateImpl()->GetHelpMenuItem(QString("UpdateCnstudio.helphide"))->activate(QAction::ActionEvent::Trigger);
					QMetaObject::invokeMethod(UpdateImpl::GetUpdateImpl()->GetHelpMenuItem(QString("UpdateCnstudio.helphide")), "trigger");
					return;
				}
				
				
			}
		}
		else
		{
			//访问不了服务器也认为不需要更新
			if (manulUpdate)
			{
				QMessageBox::about((QMainWindow*)obs_frontend_get_main_window(), obs_module_text("UpdateCnstudio.CheckForUpdates"),
					obs_module_text("UPdateCnstudio.NoUpdate"));
			}
		}

	};
	std::thread UpdateCfgThred(UpdateCfg, manualUpdate, GetUpdateFullUrl());
	UpdateCfgThred.detach();
	return  false;
}
//////////////////////////////////////////////////////////////////////////////
CUpdateOBCN::CUpdateOBCN(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CUpdateOBCN)
{
	ui->setupUi(this);
	
	ui->progress_down->setValue(0);
	ui->progress_down->setMaximum(100);
	ui->pushButton_update->setEnabled(true);
	ui->pushButton_stop->setEnabled(false);
	//UpdateImpl::GetUpdateImpl()->StartUpdate(this);
	QObject::connect(ui->pushButton_stop, SIGNAL(clicked()), this,
		SLOT(On_DownLoadPause()));
	QObject::connect(ui->pushButton_update, SIGNAL(clicked()), this,
		SLOT(On_DownLoadStart()));
}

CUpdateOBCN::~CUpdateOBCN()
{
    
}
void CUpdateOBCN::closeEvent(QCloseEvent *event)
{
	obs_frontend_save();
}
void	CUpdateOBCN::On_DownLoadFinished()
{
	ui->pushButton_update->setEnabled(false);
	ui->pushButton_stop->setEnabled(false);
	ui->label_downtext->setText(obs_module_text("UpdateCnstuido.downloadFinished"));
	UpdateImpl::GetUpdateImpl()->ExecuteInstall();
}
void	CUpdateOBCN::On_DownLoadError()
{
	ui->label_downtext->setText(obs_module_text("UpdateCnstuido.downloaderr"));
	ui->pushButton_update->setEnabled(true);
	ui->pushButton_stop->setEnabled(false);
}
void	CUpdateOBCN::On_DownLoadPause()
{
	UpdateImpl::GetUpdateImpl()->StopUpdate();
	ui->pushButton_update->setEnabled(true);
	ui->pushButton_stop->setEnabled(false);
}
void	CUpdateOBCN::On_DownLoadStart()
{
	ui->label_downtext->setText(obs_module_text("UPdateCnstuido.downloading"));
	UpdateImpl::GetUpdateImpl()->StartUpdate(this);
	ui->pushButton_update->setEnabled(false);
	ui->pushButton_stop->setEnabled(true);
}
void	CUpdateOBCN::On_InstallStart()
{
	
	UpdateImpl::GetUpdateImpl()->ExecuteInstall();
}
void	CUpdateOBCN::On_DownLoadProgress(int iProgress)
{
	ui->progress_down->setValue(iProgress);
}

