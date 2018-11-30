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
extern CREATE_BROWSER_WIDGET_PROC create_browser_widget;

/* ------------------------------------------------------------------------- */

#define TWITCH_AUTH_URL \
	"https://obsproject.com/app-auth/twitch?action=redirect"

TwitchLogin::TwitchLogin(QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle("Auth");
	resize(700, 700);

	cefWidget = create_browser_widget(nullptr, TWITCH_AUTH_URL);
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

#define CLIENT_ID_HEADER "Client-ID: selj7uigdty0j5ijt41glcce29ehb4"
#define ACCEPT_HEADER    "Accept: application/vnd.twitchtv.v5+json"

bool TwitchAuth::GetChannelInfo(std::string &error)
{
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
		return false;

	Json json = Json::parse(output, error);
	if (!error.empty())
		return false;

	title_ = json["status"].string_value();
	game_  = json["game"].string_value();
	key_   = json["stream_key"].string_value();
	id_    = json["_id"].string_value();

	return true;
}

bool TwitchAuth::SetChannelInfo(std::string &error)
{
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
		return false;

	Json json = Json::parse(output, error);
	if (!error.empty())
		return false;

	title_ = json["status"].string_value();
	game_  = json["game"].string_value();

	return true;
}

void TwitchAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), typeName(), "Token", token.c_str());
}

bool TwitchAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	token = config_get_string(main->Config(), typeName(), "Token");
	return !token.empty();
}

Auth *TwitchAuth::Clone() const
{
	TwitchAuth *auth = new TwitchAuth;
	*auth = *this;
	return auth;
}

void TwitchAuth::LoadUI()
{
}

Auth *TwitchAuth::Login(QWidget *parent)
{
	TwitchLogin login(parent);
	if (login.exec() == QDialog::Rejected) {
		return nullptr;
	}

	TwitchAuth auth;
	auth.token = QT_TO_UTF8(login.GetToken());

	std::string error;
	if (auth.GetChannelInfo(error)) {
		return auth.Clone();
	}

	return nullptr;
}
