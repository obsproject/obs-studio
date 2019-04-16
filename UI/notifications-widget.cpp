#include <QDesktopServices>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>

#include "notifications-widget.hpp"
#include "window-basic-main.hpp"
#include "clickable-label.hpp"
#include "qt-wrappers.hpp"

static QString GetHyperlinkURL(const QString &text)
{
	if (!(text.contains("<a") && text.contains("</a>")))
		return QString();

	QString link = text.section("href=\"", 1).section("\"", 0, 0);

	if (link.isEmpty())
		link = text.section("href='", 1).section("'", 0, 0);

	return link;
}

static QString GetHyperlinkText(const QString &text)
{
	return text.section(">", 1).section("</a>", 0, 0);
}

OBSNotification::OBSNotification(uint32_t id_, enum obs_notify_type type,
				 const QString &message, bool persist_,
				 void *data_, QWidget *parent)
	: id(id_), persist(persist_), data(data_), QFrame(parent)
{
	timestamp = os_gettime_ns();

	setAttribute(Qt::WA_DeleteOnClose, true);
	setObjectName("notificationWidget");

	QHBoxLayout *layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	QLabel *notificationText = new QLabel(this);
	notificationText->setObjectName("notificationText");
	notificationText->setStyleSheet("background: transparent;");

	notifyUrl = GetHyperlinkURL(message);
	notificationText->setText(message.split("<a").first());

	QPushButton *linkButton = nullptr;

	if (!notifyUrl.isEmpty()) {
		QString buttonText = GetHyperlinkText(message);

		linkButton = new QPushButton(buttonText);
		linkButton->setObjectName("notificationButton");
		connect(linkButton, SIGNAL(clicked()), this,
			SLOT(LinkActivated()));
	}

	QPushButton *closeButton = new QPushButton(this);
	closeButton->setToolTip(QTStr("Dismiss"));
	closeButton->setIconSize(QSize(16, 16));
	closeButton->setFixedSize(22, 22);

	closeButton->setIcon(QPixmap(":/res/images/notifications/close.svg"));
	closeButton->setObjectName("notificationButton");

	OBSBasic *main = OBSBasic::Get();
	connect(closeButton, SIGNAL(clicked()), this,
		SLOT(CloseNotification()));

	QIcon icon;

	if (type == OBS_NOTIFY_TYPE_INFO) {
		icon = QIcon(":/res/images/notifications/info.svg");
		setProperty("notificationType", "info");
	} else if (type == OBS_NOTIFY_TYPE_WARNING) {
		icon = QIcon(":/res/images/notifications/warning.svg");
		setProperty("notificationType", "warning");
	} else if (type == OBS_NOTIFY_TYPE_ERROR) {
		icon = QIcon(":/res/images/notifications/error.svg");
		setProperty("notificationType", "error");
	}

	QPixmap pixmap = icon.pixmap(QSize(26, 26));

	QLabel *iconLabel = new QLabel(this);
	iconLabel->setStyleSheet("background: transparent;");
	iconLabel->setPixmap(pixmap);

	layout->addStretch();
	layout->addWidget(iconLabel);
	layout->addSpacing(10);
	layout->addWidget(notificationText);
	layout->addSpacing(10);

	if (linkButton)
		layout->addWidget(linkButton);

	layout->addStretch();
	layout->addWidget(closeButton);
	layout->addSpacing(10);

	setLayout(layout);

	setFixedHeight(36);
}

uint32_t OBSNotification::GetID()
{
	return id;
}

void OBSNotification::LinkActivated()
{
	OBSBasic *main = OBSBasic::Get();

	if (notifyUrl.isEmpty())
		return;

	obs_source_t *source = static_cast<obs_source_t *>(data);

	if (notifyUrl == "settings://general")
		main->OpenSettings(0);
	else if (notifyUrl == "settings://stream")
		main->OpenSettings(1);
	else if (notifyUrl == "settings://output")
		main->OpenSettings(2);
	else if (notifyUrl == "settings://audio")
		main->OpenSettings(3);
	else if (notifyUrl == "settings://video")
		main->OpenSettings(4);
	else if (notifyUrl == "settings://hotkeys")
		main->OpenSettings(5);
	else if (notifyUrl == "settings://advanced")
		main->OpenSettings(6);
	else if (notifyUrl == "settings://")
		main->OpenSettings();
	else if (notifyUrl == "source://properties" && source)
		main->CreatePropertiesWindow(source);
	else if (notifyUrl == "source://filters" && source)
		main->CreateFiltersWindow(source);
	else
		QDesktopServices::openUrl(QUrl(notifyUrl));
}

void OBSNotification::CloseNotification()
{
	OBSBasic *main = OBSBasic::Get();
	main->CloseNotification(this);
}
