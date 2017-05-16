#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QMainWindow>
#include <QAction>
#include "pluginstore.h"

PluginStore::PluginStore(QWidget *parent) : QDialog(parent), ui(new Ui::PluginStore)
{

    ui->setupUi(this);
    m_lpWebUI = new QWebEngineView(this);
    if (m_lpWebUI != nullptr)
    {
        QWebChannel* lpChannel = new QWebChannel(m_lpWebUI->page());
        lpChannel->registerObject(QStringLiteral("QCiscik"), new WebPluginEvent(lpChannel,m_lpWebUI));
        m_lpWebUI->page()->setWebChannel(lpChannel);
        m_lpWebUI->load(QUrl(__DEF_PLUGING_MARKET_URL_));
        m_lpWebUI->show();
    }
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
void PluginStore::resizeEvent(QResizeEvent *event)
{
    if (m_lpWebUI != nullptr)
        m_lpWebUI->resize(size());
}

extern "C" void FreePluginStore()
{

}
extern "C" void InitOpenGLContexts()
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
}
static void OBSEvent(enum obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_EXIT)
        FreePluginStore();
}


extern "C" void InitPluginStore()
{
    QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(obs_module_text("pluginstore"));

    auto cb = []()
    {
        obs_frontend_push_ui_translation(obs_module_get_string);

        QMainWindow *window = (QMainWindow*)obs_frontend_get_main_window();

        PluginStore ss(window);
        ss.exec();
        obs_frontend_pop_ui_translation();
    };
    obs_frontend_add_event_callback(OBSEvent, nullptr);
    action->connect(action, &QAction::triggered, cb);
}
