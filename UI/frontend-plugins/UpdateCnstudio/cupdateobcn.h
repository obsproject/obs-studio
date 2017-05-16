#ifndef CUPDATEOBCN_H
#define CUPDATEOBCN_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <atomic>
#include "ui_cupdateobcn.h"
using namespace std;
namespace fs=std::tr2::sys;
class CUpdateOBCN;
class QMainWindow;
class UpdateImpl
{
public:
	
	static UpdateImpl* GetUpdateImpl();

public:
	QAction* GetHelpMenuItem(QString strClassName);
	bool CheckUpdate(bool manualUpdate);
	void LoadJson();
	void SaveJson();
	int  CompVersion(string strVersion1, string strVersion2);
	void StartUpdate(QWidget* wid);
	void StopUpdate();
	void ExecuteInstall();
	void UpdateProgress(double totalToDownload, double nowDownloaded);
	void	DownLoadFinished();
	void	DownLoadError();
public:
	string					cache_dir;				//缓存目录
	fs::path				updatePath;				//安装包下载目录
	fs::path				updateJsonPath;			//json
	fs::path				updateDlPath;			//dl
	fs::path				tempExePath;			//temp
	string					strOBSCN;				//
	string					strBuild;				//build VER
	string					strJsonCfgVer;
	string					strJsonCfgMd5;
	long long				strJsonCfgSize;
	string					strJsonCfgUrl;
	atomic_bool					g_UpdateThreadRun;
	long long					g_LocalFileSize;
	long long					g_remoteFileSize;
	atomic_bool					g_exitDown;
	QMainWindow *				g_window;
	QWidget*					g_RecvMsg;
protected:
	UpdateImpl();
	~UpdateImpl();
private:
	

};
class CUpdateOBCN : public QDialog
{
    Q_OBJECT

public:
	std::unique_ptr<Ui_CUpdateOBCN> ui;
    explicit CUpdateOBCN(QWidget *parent = 0);
    ~CUpdateOBCN();
	void closeEvent(QCloseEvent *event) override;
public slots :
	void	On_DownLoadFinished();
	void	On_DownLoadError();
	void	On_DownLoadPause();
	void	On_DownLoadStart();
	void	On_InstallStart();
	void	On_DownLoadProgress(int iProgress);
};

#endif // CUPDATEOBCN_H
