// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "restream-oauth.hpp"

#include <QPushButton>
#include <QMessageBox>

#include <obf.h>
#include <oauth-obs-browser-login.hpp>
#include <obs.hpp>

constexpr const char *AUTH_BASE_URL = (const char *)OAUTH_BASE_URL;
constexpr const char *AUTH_ENDPOINT = "v1/restream/redirect";
constexpr const char *TOKEN_URL = (const char *)OAUTH_BASE_URL
	"v1/restream/token";
constexpr const char *API_URL = "https://api.restream.io/v2/user/";

constexpr const char *CLIENT_ID = (const char *)RESTREAM_CLIENTID;
constexpr uint64_t CLIENT_ID_HASH = RESTREAM_HASH;
constexpr int64_t SCOPE_VERSION = 1;

constexpr const char *CHAT_URL = "https://restream.io/chat-application";
constexpr const char *INFO_URL = "https://restream.io/titles/embed";
constexpr const char *CHANNELS_URL = "https://restream.io/channel/embed";

constexpr const char *CHAT_DOCK_NAME = "restreamChat";
constexpr const char *INFO_DOCK_NAME = "restreamInfo";
constexpr const char *CHANNELS_DOCK_NAME = "restreamChannel";

namespace RestreamApi {

std::string ServiceOAuth::UserAgent()
{
	std::string userAgent("obs-restream ");
	userAgent += obs_get_version_string();

	return userAgent;
}

const char *ServiceOAuth::TokenUrl()
{
	return TOKEN_URL;
}

std::string ServiceOAuth::ClientId()
{
	std::string clientId = CLIENT_ID;
	deobfuscate_str(&clientId[0], CLIENT_ID_HASH);

	return clientId;
}

int64_t ServiceOAuth::ScopeVersion()
{
	return SCOPE_VERSION;
}

static inline void WarningBox(const QString &title, const QString &text,
			      QWidget *parent)
{
	QMessageBox warn(QMessageBox::Warning, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *ok = warn.addButton(QMessageBox::Ok);
	ok->setText(ok->tr("Ok"));
	warn.exec();
}

static int LoginConfirmation(const OAuth::LoginReason &reason, QWidget *parent)
{
	const char *reasonText = nullptr;
	switch (reason) {
	case OAuth::LoginReason::CONNECT:
		return QMessageBox::Ok;
	case OAuth::LoginReason::SCOPE_CHANGE:
		reasonText = obs_module_text(
			"Restream.Auth.ReLoginDialog.ScopeChange");
		break;
	case OAuth::LoginReason::REFRESH_TOKEN_FAILED:
		reasonText = obs_module_text(
			"Restream.Auth.ReLoginDialog.RefreshTokenFailed");
		break;
	case OAuth::LoginReason::PROFILE_DUPLICATION:
		reasonText = obs_module_text(
			"Restream.Auth.ReLoginDialog.ProfileDuplication");
		break;
	}

	QString title = QString::fromUtf8(
		obs_module_text("Restream.Auth.ReLoginDialog.Title"), -1);
	QString text =
		QString::fromUtf8(
			obs_module_text("Restream.Auth.ReLoginDialog.Text"))
			.arg(reasonText);

	QMessageBox ques(QMessageBox::Warning, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *button = ques.addButton(QMessageBox::Yes);
	button->setText(button->tr("Yes"));
	button = ques.addButton(QMessageBox::No);
	button->setText(button->tr("No"));

	return ques.exec();
}

bool ServiceOAuth::LoginInternal(const OAuth::LoginReason &reason,
				 std::string &code, std::string &)
{
	if (!obs_frontend_is_browser_available()) {
		blog(LOG_ERROR,
		     "[%s][%s]: Browser is not available, unable to login",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());

	if (reason != OAuth::LoginReason::CONNECT &&
	    LoginConfirmation(reason, parent) != QMessageBox::Ok) {
		return false;
	}

	OAuth::OBSBrowserLogin login(AUTH_BASE_URL, AUTH_ENDPOINT,
				     {"about:blank"}, parent);

	if (login.exec() != QDialog::Accepted) {
		QString error = login.GetLastError();

		if (!error.isEmpty()) {
			blog(LOG_ERROR, "[%s][%s]: %s", PluginLogName(),
			     __FUNCTION__, error.toUtf8().constData());

			QString title = QString::fromUtf8(
				obs_module_text(
					"Restream.Auth.LoginError.Title"),
				-1);
			QString text =
				QString::fromUtf8(
					obs_module_text(
						"Restream.Auth.LoginError.Text"),
					-1)
					.arg(error);

			WarningBox(title, text, parent);
		}

		return false;
	}

	code = login.GetCode().toStdString();

	return true;
}

bool ServiceOAuth::SignOutInternal()
{
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());

	QString title = QString::fromUtf8(
		obs_module_text("Restream.Auth.SignOutDialog.Title"), -1);
	QString text = QString::fromUtf8(
		obs_module_text("Restream.Auth.SignOutDialog.Text"), -1);

	QMessageBox ques(QMessageBox::Question, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *button = ques.addButton(QMessageBox::Yes);
	button->setText(button->tr("Yes"));
	button = ques.addButton(QMessageBox::No);
	button->setText(button->tr("No"));

	if (ques.exec() != QMessageBox::Yes)
		return false;

	obs_frontend_delete_browser_cookie("restream.io");

	return true;
}

void ServiceOAuth::LoadFrontendInternal()
{
	if (!obs_frontend_is_browser_available())
		return;

	if (chat || info || channels) {
		blog(LOG_ERROR, "[%s][%s] The docks were not unloaded",
		     PluginLogName(), __FUNCTION__);
		return;
	}

	chat = new RestreamBrowserWidget(QString::fromUtf8(CHAT_URL, -1));
	chat->resize(420, 600);
	obs_frontend_add_dock_by_id(
		CHAT_DOCK_NAME, obs_module_text("Restream.Dock.Chat"), chat);

	info = new RestreamBrowserWidget(QString::fromUtf8(INFO_URL, -1));
	info->resize(410, 650);
	obs_frontend_add_dock_by_id(INFO_DOCK_NAME,
				    obs_module_text("Restream.Dock.StreamInfo"),
				    info);

	channels =
		new RestreamBrowserWidget(QString::fromUtf8(CHANNELS_URL, -1));
	channels->resize(410, 250);
	obs_frontend_add_dock_by_id(CHANNELS_DOCK_NAME,
				    obs_module_text("Restream.Dock.Channels"),
				    channels);
}

void ServiceOAuth::UnloadFrontendInternal()
{
	if (!obs_frontend_is_browser_available())
		return;

	obs_frontend_remove_dock(CHAT_DOCK_NAME);
	chat = nullptr;
	obs_frontend_remove_dock(INFO_DOCK_NAME);
	info = nullptr;
	obs_frontend_remove_dock(CHANNELS_DOCK_NAME);
	channels = nullptr;
}

void ServiceOAuth::DuplicationResetInternal()
{
	obs_frontend_delete_browser_cookie("restream.io");
}

void ServiceOAuth::LoginError(RequestError &error)
{
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());
	QString title = QString::fromUtf8(
		obs_module_text("Restream.Auth.LoginError.Title"), -1);
	QString text =
		QString::fromUtf8(
			obs_module_text("Restream.Auth.LoginError.Text2"), -1)
			.arg(QString::fromStdString(error.message))
			.arg(QString::fromStdString(error.error));

	WarningBox(title, text, parent);
}

bool ServiceOAuth::WrappedGetRemoteFile(const char *url, std::string &str,
					long *responseCode,
					const char *contentType,
					std::string request_type,
					const char *postData, int postDataSize)
{
	std::string error;
	CURLcode curlCode =
		GetRemoteFile(UserAgent().c_str(), url, str, error,
			      responseCode, contentType, request_type, postData,
			      {"Client-ID: " + ClientId(),
			       "Authorization: Bearer " + AccessToken()},
			      nullptr, 5, postDataSize);

	if (curlCode != CURLE_OK && curlCode != CURLE_HTTP_RETURNED_ERROR) {
		lastError = RequestError(RequestErrorType::CURL_REQUEST_FAILED,
					 error);
		return false;
	}

	if (*responseCode == 200)
		return true;

	return false;
}

bool ServiceOAuth::TryGetRemoteFile(const char *funcName, const char *url,
				    std::string &str, const char *contentType,
				    std::string request_type,
				    const char *postData, int postDataSize)
{
	long responseCode = 0;
	if (WrappedGetRemoteFile(url, str, &responseCode, contentType,
				 request_type, postData, postDataSize)) {
		return true;
	}

	if (lastError.type == RequestErrorType::CURL_REQUEST_FAILED) {
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     lastError.message.c_str(), lastError.error.c_str());
	} else if (lastError.type ==
		   RequestErrorType::ERROR_JSON_PARSING_FAILED) {
		blog(LOG_ERROR, "[%s][%s] HTTP status: %ld\n%s",
		     PluginLogName(), funcName, responseCode, str.c_str());
	} else {
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     lastError.error.c_str(), lastError.message.c_str());
	}

	if (responseCode != 401)
		return false;

	lastError = RequestError();
	if (!RefreshAccessToken(lastError))
		return false;

	return WrappedGetRemoteFile(url, str, &responseCode, contentType,
				    request_type, postData, postDataSize);
}

bool ServiceOAuth::GetStreamKey(std::string &streamKey_)
{
	if (!streamKey.empty()) {
		streamKey_ = streamKey;
		return true;
	}

	std::string url = API_URL;
	url += "streamKey";

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "")) {
		blog(LOG_ERROR, "[%s][%s] Failed to retrieve stream key",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	StreamKey response;
	try {
		response = nlohmann::json::parse(output);

		streamKey = response.streamKey;
		streamKey_ = streamKey;
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	return false;
}

}
