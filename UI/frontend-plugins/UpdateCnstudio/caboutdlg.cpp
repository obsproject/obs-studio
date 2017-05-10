#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QMessageBox>
#include <QMainWindow>
#include <QAction>
#include "caboutdlg.h"
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
	
	
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
		obs_module_text("UpdateCnstudio.about"));



	auto cb = []()
	{
		obs_frontend_push_ui_translation(obs_module_get_string);

		QMainWindow *window =
			(QMainWindow*)obs_frontend_get_main_window();

		CAboutDlg ss(window);
		ss.exec();

		obs_frontend_pop_ui_translation();
	};
	obs_frontend_add_event_callback(OBSEvent, nullptr);
	action->connect(action, &QAction::triggered, cb);
}
