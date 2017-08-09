#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QProcess>
#include "pluginstore.h"

PluginStore* ss = NULL;

class WebPluginEvent;
PluginStore::PluginStore(QWidget *parent) : QDialog(parent), ui(new Ui::PluginStore)
{
    QString qurl;
    QString qstrLanguage;
    QString qstrSystem;
    QString qstrBigVersion;
    QPalette pe;

    ui->setupUi(this);
    ui->horizontalGroupBox->setFixedWidth(890);
    ui->setButton->setStyleSheet("QPushButton{border:0px;}");
    ui->setButton->setIcon(QPixmap(":/ico_Settings.png"));
    ui->setButton->setIconSize(QPixmap(":/ico_Settings.png").size());
    ui->setButton->resize(QPixmap(":/ico_Settings.png").size());
    ui->setButton->move(QPoint(840, 13));
    ui->setButton->setCursor(Qt::PointingHandCursor);

    ui->closeButton->setStyleSheet("QPushButton{border:0px;}");
    ui->closeButton->setIcon(QPixmap(":/ico_close.png"));
    ui->closeButton->setIconSize(QPixmap(":/ico_close.png").size());
    ui->closeButton->resize(QPixmap(":/ico_close.png").size());
    ui->closeButton->move(QPoint(860,13));
    ui->closeButton->setCursor(Qt::PointingHandCursor);
    pe.setColor(QPalette::WindowText, QColor(101,101,101,255));
    ui->label->setPalette(pe);

    if (this->objectName().isEmpty())
        ui->label->setText(QStringLiteral("PluginStore"));
    else
        ui->label->setText(QApplication::translate("PluginStore", "PluginStore", 0));

    m_lpWebUI = new QWebEngineView(this);
    if (m_lpWebUI != nullptr)
    {
        QWebChannel* lpChannel = new QWebChannel(m_lpWebUI->page());
        m_lpWebEvent = new WebPluginEvent(lpChannel, m_lpWebUI);
        lpChannel->registerObject(QStringLiteral("QCiscik"), m_lpWebEvent);
        m_lpWebUI->page()->setWebChannel(lpChannel);
        m_lpWebUI->setContextMenuPolicy(Qt::NoContextMenu);
        m_lpWebUI->move(QPoint(0, 40));
        m_lpWebUI->setFixedSize(QSize(890,526));
        qstrLanguage = obs_get_locale();

#ifdef _WIN32
        qstrSystem = "2";
#else   //_MAC
        qstrSystem = "1";
#endif // _WIN32

#ifdef __OBS__STUDIO__
        qstrBigVersion = "1";
#else
        qstrBigVersion = "2"
#endif // __OBS__STUDIO__

        qurl = QString(__DEF_PLUGING_MARKET_URL_).arg(qstrLanguage).arg(qstrSystem).arg(qstrBigVersion);
        qDebug() << "request page :" <<qurl;
  
        m_lpWebUI->load(QUrl(qurl));
        m_lpWebUI->show();
    }
}

PluginStore::~PluginStore()
{

}

void PluginStore::on_closeButton_clicked()
{
    if (m_lpWebEvent != NULL)
    {
        m_lpWebEvent->UninitPluginStore();
    }
    if (m_lpWebUI != NULL)
    {
        m_lpWebUI->close();
    }
    
    obs_frontend_save();
    done(0);
}

void PluginStore::on_setButton_clicked()
{
    if (m_lpWebEvent != NULL)
    {
        //m_lpWebEvent->SetDownloadPluginDir();
        m_lpWebUI->page()->runJavaScript("showSettingPage()");
    }
}

void PluginStore::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mouse_press = true;
        move_point = event->pos();
    }
}

void PluginStore::mouseReleaseEvent(QMouseEvent *event)
{
    mouse_press = false;
}

void PluginStore::mouseMoveEvent(QMouseEvent *event)
{
    if (mouse_press)
    {
        QPoint move_pos = event->globalPos();
        this->move(move_pos - move_point);
    }
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

	WebPluginEvent::RemoveAllLabelFile();
    QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(obs_module_text("pluginstore"));
    auto cb = []()
    {
        obs_frontend_push_ui_translation(obs_module_get_string);
        QMainWindow *window = (QMainWindow*)obs_frontend_get_main_window();
        ss  = new PluginStore(window);
        ss->setFixedSize(QSize(890,526));
        ss->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
        ss->show();
    };
    obs_frontend_add_event_callback(OBSEvent, nullptr);
    action->connect(action, &QAction::triggered, cb);
}
