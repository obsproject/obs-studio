#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QMessageBox>
#include <QMainWindow>
#include <qmenubar>
#include <QAction>
#include "caboutdlg.h"
#include "cupdateobcn.h"
#include "ui_caboutdlg.h"
#include "cupdateobcn.h"

CAboutDlg::CAboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CAboutDlg)
{
    ui->setupUi(this);
}

CAboutDlg::~CAboutDlg()
{
   
}

void CAboutDlg::on_pushButton_clicked()
{
    CUpdateOBCN cn;
    cn.exec();
}
extern "C" void FreePluginUpdateCn()
{

}
void CAboutDlg::closeEvent(QCloseEvent *event)
{
	obs_frontend_save();
}
static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT)
		FreePluginUpdateCn();
}


extern "C" void InitPluginUpdateCn()
{
	
	
	QAction *action = nullptr;
	QAction *haction = nullptr;
	QMainWindow *window =
		(QMainWindow*)obs_frontend_get_main_window();
	QMenuBar* pBar=window->menuBar();
	QMenu*	  pHelp = pBar->findChild<QMenu*>(QString("menuBasic_MainMenu_Help"));
	blog(LOG_INFO, "ck---InitPluginUpdateCn	init", bnum_allocs());
	auto cb = []()
	{
		//about
		obs_frontend_push_ui_translation(obs_module_get_string);

		QMainWindow *window =
			(QMainWindow*)obs_frontend_get_main_window();

		CAboutDlg ss(window);
		ss.exec();

		obs_frontend_pop_ui_translation();
	};
	//check
	auto checkcb = []()
	{
		obs_frontend_push_ui_translation(obs_module_get_string);
		UpdateImpl::GetUpdateImpl()->CheckUpdate(true);
		obs_frontend_pop_ui_translation();
	};
	//show checkWindow
	auto showDownLoadWindow = []()
	{
		obs_frontend_push_ui_translation(obs_module_get_string);
		CUpdateOBCN updatedlg((QMainWindow*)obs_frontend_get_main_window());
		updatedlg.exec();
		obs_frontend_pop_ui_translation();
	};
	obs_frontend_add_event_callback(OBSEvent, nullptr);
	if (pHelp)
	{
		action = pHelp->addAction(obs_module_text("UpdateCnstudio.about"));
		haction = pHelp->addAction(obs_module_text("UpdateCnstudio.helphide"));
		haction->setObjectName(QString("UpdateCnstudio.helphide"));
		//about
		action->connect(action, &QAction::triggered, cb);
		haction->connect(haction, &QAction::triggered, showDownLoadWindow);
		haction->setVisible(false);
		QAction* oldupdate=pHelp->findChild<QAction*>(QString("actionCheckForUpdates"));
		if (!oldupdate)
		{
			//no old update action
			oldupdate = pHelp->addAction(obs_module_text("UpdateCnstudio.CheckForUpdates"));
		}
		else
		{
			oldupdate->disconnect();
		}
		// connect(sender, signal, sender, slot, Qt::DirectConnection);
		oldupdate->connect(oldupdate, &QAction::triggered, checkcb);
		//checkupate
		UpdateImpl::GetUpdateImpl()->CheckUpdate(false);
	}
	
}
