#include "auth-twitch.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include "window-basic-main.hpp"
#include "remote-text.hpp"

#include <json11.hpp>

using namespace json11;

#include <browser-panel.hpp>
extern QCef *cef;
extern QCefCookieManager *panel_cookies;

/* ------------------------------------------------------------------------- */

#define TWITCH_AUTH_URL \
	"https://obsproject.com/app-auth/twitch?action=redirect"

TwitchLogin::TwitchLogin(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle("Auth");
	resize(700, 700);

	OBSBasic::InitBrowserPanelSafeBlock(true);

	cefWidget = cef->create_widget(nullptr, TWITCH_AUTH_URL, panel_cookies);
	if (!cefWidget) {
		fail = true;
		return;
	}

	connect(cefWidget, SIGNAL(titleChanged(const QString &)),
			this, SLOT(setWindowTitle(const QString &)));
	connect(cefWidget, SIGNAL(urlChanged(const QString &)),
			this, SLOT(urlChanged(const QString &)));

	QPushButton *close = new QPushButton(QTStr("Cancel"));
	connect(close, &QAbstractButton::clicked,
			this, &QDialog::reject);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout(this);
	topLayout->addWidget(cefWidget);
	topLayout->addLayout(bottomLayout);
}

TwitchLogin::~TwitchLogin()
{
	delete cefWidget;
}

void TwitchLogin::urlChanged(const QString &url)
{
	int token_idx = url.indexOf("access_token=");
	if (token_idx == -1)
		return;

	token_idx += 13;

	int next_idx = url.indexOf("&", token_idx);
	if (next_idx != -1)
		token = url.mid(token_idx, next_idx - token_idx);
	else
		token = url.right(url.size() - token_idx);

	accept();
}

/* ------------------------------------------------------------------------- */

TwitchAuth::TwitchAuth()
{
	cef->add_popup_url_callback(
			"https://twitch.tv/popout/frankerfacez/chat?ffz-settings",
			this, "OnFFZPopup");
}

#define CLIENT_ID_HEADER "Client-ID: selj7uigdty0j5ijt41glcce29ehb4"
#define ACCEPT_HEADER    "Accept: application/vnd.twitchtv.v5+json"

struct ErrorInfo {
	std::string message;
	std::string error;

	ErrorInfo(std::string message_, std::string error_)
		: message(message_), error(error_)
	{}
};

bool TwitchAuth::GetChannelInfo()
try {
	if (token.empty())
		return false;

	std::string auth;
	auth += "Authorization: OAuth ";
	auth += token;

	std::vector<std::string> headers;
	headers.push_back(CLIENT_ID_HEADER);
	headers.push_back(ACCEPT_HEADER);
	headers.push_back(std::move(auth));

	std::string output;
	std::string error;

	bool success = GetRemoteFileSafeBlock(
			"https://api.twitch.tv/kraken/channel",
			output,
			error,
			nullptr,
			"application/json",
			nullptr,
			headers,
			nullptr,
			5);
	if (!success || output.empty())
		throw ErrorInfo("Failed to get text from remote", error);

	Json json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	title_ = json["status"].string_value();
	game_  = json["game"].string_value();
	name_  = json["name"].string_value();
	key_   = json["stream_key"].string_value();
	id_    = json["_id"].string_value();

	return true;
} catch (ErrorInfo info) {
	blog(LOG_DEBUG, "%s: %s: %s",
			__FUNCTION__,
			info.message.c_str(),
			info.error.c_str());
	return false;
}

bool TwitchAuth::SetChannelInfo()
try {
	if (token.empty() || id_.empty())
		return false;

	std::string auth;
	auth += "Authorization: OAuth ";
	auth += token;

	std::string url;
	url += "https://api.twitch.tv/kraken/channels/";
	url += id_;

	std::vector<std::string> headers;
	headers.push_back(CLIENT_ID_HEADER);
	headers.push_back(ACCEPT_HEADER);
	headers.push_back(std::move(auth));

	Json obj = Json::object {
		{"status", title_},
		{"game", game_}
	};

	std::string output;
	std::string error;

	bool success = GetRemoteFileSafeBlock(
			url.c_str(),
			output,
			error,
			nullptr,
			"application/json",
			obj.dump().c_str(),
			headers,
			nullptr,
			5);
	if (!success || output.empty())
		throw ErrorInfo("Failed to post text to remote", error);

	Json json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	title_ = json["status"].string_value();
	game_  = json["game"].string_value();

	return true;
} catch (ErrorInfo info) {
	blog(LOG_DEBUG, "%s: %s: %s",
			__FUNCTION__,
			info.message.c_str(),
			info.error.c_str());
	return false;
}

void TwitchAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), typeName(), "Token", token.c_str());
	config_set_string(main->Config(), typeName(), "Name", name_.c_str());
	config_set_string(main->Config(), typeName(), "DockState",
			main->saveState().toBase64().constData());
}

static inline std::string get_config_str(
		OBSBasic *main,
		const char *section,
		const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool TwitchAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	token = get_config_str(main, typeName(), "Token");
	name_ = get_config_str(main, typeName(), "Name");
	firstLoad = false;
	return !token.empty();
}

class TwitchChat : public QDockWidget {
public:
	inline TwitchChat() : QDockWidget() {}

	QScopedPointer<QCefWidget> widget;
};

static const char *ffz_script = "\
var ffz = document.createElement('script');\
ffz.setAttribute('src','https://cdn.frankerfacez.com/script/script.min.js');\
document.head.appendChild(ffz);";

static const char *bttv_script = "\
var bttv = document.createElement('script');\
bttv.setAttribute('src','https://cdn.betterttv.net/betterttv.js');\
document.head.appendChild(bttv);";

void TwitchAuth::LoadUI()
{
	if (uiLoaded)
		return;
	if (!GetChannelInfo())
		return;

	OBSBasic::InitBrowserPanelSafeBlock(false);
	OBSBasic *main = OBSBasic::Get();

	std::string url;
	url += "https://www.twitch.tv/popout/";
	url += name();
	url += "/chat?darkpopout";

	chat.reset(new TwitchChat());
	chat->setObjectName("twitchChat");
	chat->resize(300, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("TwitchAuth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	QCefWidget *browser = cef->create_widget(nullptr, url, panel_cookies);
	chat->setWidget(browser);

	std::string script;
	script += ffz_script;
	script += bttv_script;

	browser->setStartupScript(script);

	main->addDockWidget(Qt::RightDockWidgetArea, chat.data());
	chatMenu.reset(main->AddDockWidgetMenu(chat.data()));

	chat->setFloating(true);
	if (firstLoad) {
		chat->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(main->Config(),
				typeName(), "DockState");
		QByteArray dockState =
			QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}

	uiLoaded = true;
}

std::shared_ptr<Auth> TwitchAuth::Login(QWidget *parent)
{
	TwitchLogin login(parent);
	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	std::shared_ptr<TwitchAuth> auth = std::make_shared<TwitchAuth>();
	auth->token = QT_TO_UTF8(login.GetToken());

	std::string error;
	if (auth->GetChannelInfo()) {
		return auth;
	}

	return std::shared_ptr<Auth>();
}

void TwitchAuth::OnStreamConfig()
{
	OBSBasic *main = OBSBasic::Get();
	obs_service_t *service = main->GetService();

	obs_data_t *settings = obs_service_get_settings(service);

	obs_data_set_string(settings, "key", key().c_str());
	obs_service_update(service, settings);

	obs_data_release(settings);
}

void TwitchAuth::OnFFZPopup(const QString &url)
{
	QDialog dlg(OBSBasic::Get());
	dlg.setWindowTitle("FFZ Settings");
	dlg.resize(800, 600);

	QCefWidget *cefWidget = cef->create_widget(nullptr, QT_TO_UTF8(url));
	if (!cefWidget) {
		return;
	}

	std::string script;
	script += ffz_script;
	script += bttv_script;

	cefWidget->setStartupScript(script);

	connect(cefWidget, SIGNAL(titleChanged(const QString &)),
			&dlg, SLOT(setWindowTitle(const QString &)));

	QPushButton *close = new QPushButton(QTStr("Close"));
	connect(close, &QAbstractButton::clicked,
			&dlg, &QDialog::accept);

	QHBoxLayout *bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	QVBoxLayout *topLayout = new QVBoxLayout(&dlg);
	topLayout->addWidget(cefWidget);
	topLayout->addLayout(bottomLayout);

	dlg.exec();
}
