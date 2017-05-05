#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QMainWindow>
#include <QMessageBox>
#include <QAction>
#include <qwebchannel>
#include "pluginstore.h"
#include "ui_pluginstore.h"
#include "WebPluginEvent.h"
PluginStore::PluginStore(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PluginStore)
{
	
    ui->setupUi(this);
	webui = new QWebEngineView(this);
	connect(webui, SIGNAL(loadFinished(bool)), this, SLOT(on_web_loadFinished(bool)));
	QWebChannel* channel = new QWebChannel(webui->page());
	channel->registerObject(QStringLiteral("QCiscik"), new WebPluginEvent(channel));
	webui->page()->setWebChannel(channel);
	webui->load(QUrl("d:/index.html"));
	webui->show();
}

PluginStore::~PluginStore()
{

}
void PluginStore::on_close_clicked()
{
	done(0);
}
void PluginStore::closeEvent(QCloseEvent *event)
{
	obs_frontend_save();
}
void    PluginStore::resizeEvent(QResizeEvent *event)
{
	webui->resize(size());
}
void PluginStore::on_web_loadFinished(bool ok)
{
	
}
extern "C" void FreePluginStore()
{

}
static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT)
		FreePluginStore();
}


extern "C" void InitPluginStore()
{
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
		obs_module_text("pluginstore"));

	

	auto cb = []()
	{
		obs_frontend_push_ui_translation(obs_module_get_string);

		QMainWindow *window =
			(QMainWindow*)obs_frontend_get_main_window();

		PluginStore ss(window);
		ss.exec();

		obs_frontend_pop_ui_translation();
	};
	obs_frontend_add_event_callback(OBSEvent, nullptr);
	action->connect(action, &QAction::triggered, cb);
}
