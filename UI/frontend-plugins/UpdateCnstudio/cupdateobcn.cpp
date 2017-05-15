#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
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
	strOBSCN =OBS_VERSION;				//
	strBuild="4096";				//build VER
	g_window =(QMainWindow*)obs_frontend_get_main_window();
	g_UpdateThreadRun=false;
	g_exitDown = false;
	g_LocalFileSize = 0;
	strJsonCfgSize = 1;
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

}
void UpdateImpl::SaveJson()
{

}
int  UpdateImpl::CompVersion(string strVersion1, string strVersion2)
{
	return 1;
}
void UpdateImpl::ExecuteInstall()
{
	fs::copy_file(updatePath, tempExePath, fs::copy_option::overwrite_if_exists);
	QProcess p(Q_NULLPTR);
	p.startDetached(QString(tempExePath.directory_string().c_str()), QStringList());
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
		h.DownUrlToFile(url, localPath, g_LocalFileSize,[](void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded)->int
		{
			if (UpdateImpl::GetUpdateImpl()->g_exitDown)
				return -1;
			return 0;;
		});
		if (h.m_curlretcode != CURLcode::CURLE_OK || h.m_httpretcode > 400)
		{
			
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
		if (CUIMD5::MD5File(updatePath.directory_string().c_str()) == strJsonCfgMd5)
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
			strJson = R"({"md5":"6B882F69B146F333FE5CCDFBC3400F0A","url":"https://github.com/jp9000/obs-studio/releases/download/18.0.1/OBS-Studio-18.0.1-Full-Installer.exe","size":"113034688","version":"18.0.3"})";
			//更新本地josn
			string strErr;
			auto jsVar = json11::Json::parse(strJson, strErr);
			UpdateImpl::GetUpdateImpl()->LoadJson();
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
	std::thread UpdateCfgThred(UpdateCfg, manualUpdate, "http://intf.soft.360.cn/index.php?c=Coop&a=update&version=19.0.1");
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
	UpdateImpl::GetUpdateImpl()->ExecuteInstall();
}
void	CUpdateOBCN::On_DownLoadError()
{
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

