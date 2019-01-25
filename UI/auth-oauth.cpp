#include "auth-oauth.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include "window-basic-main.hpp"
#include "remote-text.hpp"

#include <unordered_map>

#include <json11.hpp>

using namespace json11;

#include <browser-panel.hpp>
extern QCef *cef;
extern QCefCookieManager *panel_cookies;

/* ------------------------------------------------------------------------- */

OAuthLogin::OAuthLogin(QWidget *parent, const std::string &url, bool token)
	: QDialog   (parent),
	  get_token (token)
{
	setWindowTitle("Auth");
	resize(700, 700);

	OBSBasic::InitBrowserPanelSafeBlock(true);

	cefWidget = cef->create_widget(nullptr, url, panel_cookies);
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

OAuthLogin::~OAuthLogin()
{
	delete cefWidget;
}

void OAuthLogin::urlChanged(const QString &url)
{
	std::string uri = get_token ? "access_token=" : "code=";
	int code_idx = url.indexOf(uri.c_str());
	if (code_idx == -1)
		return;

	code_idx += (int)uri.size();

	int next_idx = url.indexOf("&", code_idx);
	if (next_idx != -1)
		code = url.mid(code_idx, next_idx - code_idx);
	else
		code = url.right(url.size() - code_idx);

	accept();
}

/* ------------------------------------------------------------------------- */

struct OAuthInfo {
	Auth::Def def;
	OAuth::login_cb login;
	OAuth::delete_cookies_cb delete_cookies;
};

static std::vector<OAuthInfo> loginCBs;

void OAuth::RegisterOAuth(const Def &d, create_cb create, login_cb login,
		delete_cookies_cb delete_cookies)
{
	OAuthInfo info = {d, login, delete_cookies};
	loginCBs.push_back(info);
	RegisterAuth(d, create);
}

std::shared_ptr<Auth> OAuth::Login(QWidget *parent, const std::string &service)
{
	for (auto &a : loginCBs) {
		if (service.find(a.def.service) != std::string::npos) {
			return a.login(parent);
		}
	}

	return nullptr;
}

void OAuth::DeleteCookies(const std::string &service)
{
	for (auto &a : loginCBs) {
		if (service.find(a.def.service) != std::string::npos) {
			a.delete_cookies();
		}
	}
}

void OAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "RefreshToken",
			refresh_token.c_str());
	config_set_string(main->Config(), service(), "Token", token.c_str());
	config_set_uint(main->Config(), service(), "ExpireTime", expire_time);
}

static inline std::string get_config_str(
		OBSBasic *main,
		const char *section,
		const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);
	return val ? val : "";
}

bool OAuth::LoadInternal()
{
	OBSBasic *main = OBSBasic::Get();
	refresh_token = get_config_str(main, service(), "RefreshToken");
	token = get_config_str(main, service(), "Token");
	expire_time = config_get_uint(main->Config(), service(), "ExpireTime");
	return implicit
		? !token.empty()
		: !refresh_token.empty();
}
