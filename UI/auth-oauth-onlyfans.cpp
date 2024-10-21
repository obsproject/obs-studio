#include "auth-oauth-onlyfans.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <qt-wrappers.hpp>
#include <obs-app.hpp>

#include "window-basic-main.hpp"
#include "remote-text.hpp"

#include <unordered_map>

#include <json11.hpp>

#include "ui-config.h"

using namespace json11;

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
extern QCef *cef;
extern QCefCookieManager *panel_cookies;
#endif
//-------------------------------------------------------------------------//
OAuthOnlyfansLogin::OAuthOnlyfansLogin(QWidget *parent, const std::string &url, const std::string &base_url)
	: QDialog(parent),
	  of_base_url(base_url)
{
#ifdef BROWSER_AVAILABLE
	if (cef == nullptr) {
		return;
	}

	setWindowTitle("Auth");
	setMinimumSize(400, 400);
	resize(700, 700);

	Qt::WindowFlags flags = windowFlags();
	Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags & (~helpFlag));

	OBSBasic::InitBrowserPanelSafeBlock();

	this->cefWidget = cef->create_widget(nullptr, url, panel_cookies);
	if (this->cefWidget == nullptr) {
		fail = true;
		return;
	}

	connect(this->cefWidget, SIGNAL(titleChanged(const QString &)), this, SLOT(setWindowTitle(const QString &)));
	connect(this->cefWidget, SIGNAL(urlChanged(const QString &)), this, SLOT(urlChanged(const QString &)));

	auto close = new QPushButton(QTStr("Cancel"));
	connect(close, &QAbstractButton::clicked, this, &QDialog::reject);

	auto bottomLayout = new QHBoxLayout();
	bottomLayout->addStretch();
	bottomLayout->addWidget(close);
	bottomLayout->addStretch();

	auto topLayout = new QVBoxLayout(this);
	topLayout->addWidget(this->cefWidget);
	topLayout->addLayout(bottomLayout);
#else
	UNUSED_PARAMETER(url);
#endif
}

auto OAuthOnlyfansLogin::exec() -> int
{
#ifdef BROWSER_AVAILABLE
	if (this->cefWidget) {
		return QDialog::exec();
	}
#endif // BROWSER_AVAILABLE
	return QDialog::Rejected;
}

auto OAuthOnlyfansLogin::reject() -> void
{
#ifdef BROWSER_AVAILABLE
	if (this->cefWidget != nullptr) {
		delete this->cefWidget, this->cefWidget = nullptr;
	}
#endif // BROWSER_AVAILABLE
	QDialog::reject();
}

auto OAuthOnlyfansLogin::accept() -> void
{
#ifdef BROWSER_AVAILABLE
	if (this->cefWidget != nullptr) {
		delete this->cefWidget, this->cefWidget = nullptr;
	}
#endif // BROWSER_AVAILABLE
	QDialog::accept();
}

void OAuthOnlyfansLogin::urlChanged(const QString &url)
{
	const char *params[] = {"accessToken=", "refreshToken="};

	if (not url.startsWith(this->of_base_url.c_str())) {
		return;
	}

	for (const auto &param : params) {
		int code_idx = url.indexOf(param);
		if (code_idx == -1) {
			return;
		}

		code_idx += int(strlen(param));

		int next_idx = url.indexOf("&", code_idx);
		if (next_idx != -1) {
			if (strcmp(param, "accessToken=") == 0) {
				this->token = url.mid(code_idx, next_idx - code_idx);
			} else {
				this->refresh_token = url.mid(code_idx, next_idx - code_idx);
			}
		} else {
			if (strcmp(param, "refreshToken=") == 0) {
				this->refresh_token = url.right(url.size() - code_idx);
			} else {
				this->refresh_token = url.right(url.size() - code_idx);
			}
		}
	}
	accept();
}
//-------------------------------------------------------------------------//
struct OAuthInfo {
	Auth::Def def;
	OAuthOnlyfansStreamKey::login_cb login;
	OAuthOnlyfansStreamKey::delete_cookies_cb delete_cookies;
};
//-------------------------------------------------------------------------//
static std::string get_config_str(OBSBasic *main, const char *section, const char *name)
{
	const char *val = config_get_string(main->Config(), section, name);

	return val != nullptr ? val : "";
}
//-------------------------------------------------------------------------//
auto OAuthOnlyfansStreamKey::GetToken(const char *url, const std::string &auth_token) -> bool
{
	return GetTokenInternal(url, auth_token);
}

auto OAuthOnlyfansStreamKey::GetToken(const std::string &url, const std::string &auth_token) -> bool
{
	return GetTokenInternal(url.c_str(), auth_token);
}
//-------------------------------------------------------------------------//
auto OAuthOnlyfansStreamKey::GetTokenInternal(const char *url, const std::string &token) -> bool
{
	try {
		std::string output;
		std::string error;
		std::string desc;

		if (this->currentScopeVer > 0) {
			if (this->RetryLogin()) {
				return true;
			} else {
				QString title = QTStr("Auth.InvalidScope.Title");
				QString text = QTStr("Auth.InvalidScope.Text").arg(service());

				QMessageBox::warning(OBSBasic::Get(), title, text);
			}
		}

		if (token.empty() && !TokenExpired()) {
			return true;
		}

		char buffer[256] = {0};
		bool success = false;

		// Making post body.
		std::snprintf(buffer, sizeof(buffer) - 1, R"({"refreshToken": "%s"})", token.c_str());

		auto func = [&]() {
			success = GetRemoteFile(url, output, error, nullptr, "application/json", "POST", buffer, {},
						nullptr, 5);
		};

		ExecThreadedWithoutBlocking(func, QTStr("Auth.Authing.Title"),
					    QTStr("Auth.Authing.Text").arg(service()));
		if (not success || output.empty()) {
			throw ErrorInfo("Failed to get token from remote", error);
		}

		Json json = Json::parse(output, error);
		if (not error.empty()) {
			throw ErrorInfo("Failed to parse json", error);
		}

		// error handling
		auto status_code = json["statusCode"].int_value();
		if (status_code != 200) {
			throw ErrorInfo(error, "Refresh token failed: " + json["error"]["description"].string_value());
		}
		// success
		this->token = json["data"]["accessToken"].string_value();

		if (this->token.empty()) {
			throw ErrorInfo("Failed to get token from remote", error);
		}
		return true;
	} catch (ErrorInfo &info) {
		QString title = QTStr("Auth.AuthFailure.Title");
		QString text = QTStr("Auth.AuthFailure.Text").arg(service(), info.message.c_str(), info.error.c_str());

		QMessageBox::warning(OBSBasic::Get(), title, text);

		blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(), info.error.c_str());
	}
	return false;
}
//-------------------------------------------------------------------------//
auto OAuthOnlyfansStreamKey::OnStreamConfig() -> void
{
	if (this->key_.empty()) {
		return;
	}

	auto main = OBSBasic::Get();
	obs_service_t *service = main->GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service);
	auto bwtest = obs_data_get_bool(settings, "bwtest");

	if (bwtest && strcmp(this->service(), "OnlyFans.com") == 0) {
		obs_data_set_string(settings, "key", (this->key_ + "?bandwidthtest=true").c_str());
	} else {
		obs_data_set_string(settings, "key", this->key_.c_str());
		obs_data_set_string(settings, "server", this->server_.c_str());
	}
	obs_service_update(service, settings);
}
