#include "youtube-chat.hpp"

#include <QMessageBox>
#include <QThread>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/dstr.h>

constexpr const char *CHAT_PLACEHOLDER_URL =
	"https://obsproject.com/placeholders/youtube-chat";
constexpr const char *CHAT_POPOUT_URL =
	"https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v=";

constexpr const char *CHAT_SCRIPT = "\
const obsCSS = document.createElement('style');\
obsCSS.innerHTML = \"#panel-pages.yt-live-chat-renderer {display: none;}\
yt-live-chat-viewer-engagement-message-renderer {display: none;}\";\
document.querySelector('head').appendChild(obsCSS);";

static QString GetTranslatedError(const RequestError &error)
{
	if (error.type != RequestErrorType::UNKNOWN_OR_CUSTOM ||
	    error.error.empty())
		return QString::fromStdString(error.message);

	std::string lookupString = "YouTube.Errors.";
	lookupString += error.error;

	return QT_UTF8(obs_module_text(lookupString.c_str()));
}

YoutubeChat::YoutubeChat(YouTubeApi::ServiceOAuth *api)
	: apiYouTube(api),
	  url(CHAT_PLACEHOLDER_URL),
	  QWidget()
{

	resize(300, 600);
	setMinimumSize(200, 300);

	lineEdit = new LineEditAutoResize();
	lineEdit->setVisible(false);
	lineEdit->setMaxLength(200);
	lineEdit->setPlaceholderText(
		obs_module_text("YouTube.Chat.Input.Placeholder"));
	sendButton =
		new QPushButton(obs_module_text("YouTube.Chat.Input.Send"));
	sendButton->setVisible(false);

	chatLayout = new QHBoxLayout();
	chatLayout->setContentsMargins(0, 0, 0, 0);
	chatLayout->addWidget(lineEdit, 1);
	chatLayout->addWidget(sendButton);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(chatLayout);

	setLayout(layout);

	connect(lineEdit, SIGNAL(returnPressed()), this,
		SLOT(SendChatMessage()));
	connect(sendButton, SIGNAL(pressed()), this, SLOT(SendChatMessage()));
}

YoutubeChat::~YoutubeChat()
{
	if (!sendingMessage.isNull() && sendingMessage->isRunning())
		sendingMessage->wait();
}

void YoutubeChat::SetChatIds(const std::string &broadcastId,
			     const std::string &chatId_)
{
	url = CHAT_POPOUT_URL;
	url += broadcastId;
	chatId = chatId_;

	if (!cefWidget.isNull())
		QMetaObject::invokeMethod(this, "UpdateCefWidget",
					  Qt::QueuedConnection);

	QMetaObject::invokeMethod(this, "EnableChatInput", Qt::QueuedConnection,
				  Q_ARG(bool, true));
}

void YoutubeChat::ResetIds()
{
	url = CHAT_PLACEHOLDER_URL;
	chatId.clear();

	if (!cefWidget.isNull())
		QMetaObject::invokeMethod(this, "UpdateCefWidget",
					  Qt::QueuedConnection);

	QMetaObject::invokeMethod(this, "EnableChatInput", Qt::QueuedConnection,
				  Q_ARG(bool, false));
}

void YoutubeChat::showEvent(QShowEvent *event)
{
	UpdateCefWidget();

	QWidget::showEvent(event);
}

void YoutubeChat::hideEvent(QHideEvent *event)
{
	cefWidget.reset(nullptr);
	QWidget::hideEvent(event);
}

void YoutubeChat::SendChatMessage()
{
	if (lineEdit->text().isEmpty())
		return;

	this->lineEdit->setEnabled(false);
	this->sendButton->setEnabled(false);

	sendingMessage.reset(new QuickThread([&]() {
		YouTubeApi::LiveChatMessage chatMessage;
		chatMessage.snippet.type =
			YouTubeApi::LiveChatTextMessageType::TEXT_MESSAGE_EVENT;
		chatMessage.snippet.liveChatId = chatId;
		chatMessage.snippet.textMessageDetails.messageText =
			lineEdit->text().toStdString();

		lineEdit->setText("");
		lineEdit->setPlaceholderText(
			obs_module_text("YouTube.Chat.Input.Sending"));

		if (apiYouTube->InsertLiveChatMessage(chatMessage)) {
			os_sleep_ms(3000);
		} else {
			QString error =
				GetTranslatedError(apiYouTube->GetLastError());
			QMetaObject::invokeMethod(
				this, "ShowErrorMessage", Qt::QueuedConnection,
				Q_ARG(const QString &, error));
		}
		lineEdit->setPlaceholderText(
			obs_module_text("YouTube.Chat.Input.Placeholder"));
	}));

	connect(sendingMessage.get(), &QThread::finished, this, [=]() {
		this->lineEdit->setEnabled(true);
		this->sendButton->setEnabled(true);
	});

	sendingMessage->start();
}

void YoutubeChat::ShowErrorMessage(const QString &error)
{
	QMessageBox::warning(
		this, obs_module_text("YouTube.Chat.Error.Title"),
		QString::fromUtf8(obs_module_text("YouTube.Chat.Error.Text"),
				  -1)
			.arg(error));
}

void YoutubeChat::UpdateCefWidget()
{
	obs_frontend_browser_params params = {0};

	params.url = url.c_str();
	params.startup_script = CHAT_SCRIPT;

	cefWidget.reset((QWidget *)obs_frontend_get_browser_widget(&params));

	QVBoxLayout *layout = (QVBoxLayout *)this->layout();
	layout->insertWidget(0, cefWidget.get(), 1);
}

void YoutubeChat::EnableChatInput(bool enable)
{
	lineEdit->setVisible(enable);
	sendButton->setVisible(enable);
}
