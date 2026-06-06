#include "RestreamAuth.hpp"

#include <dialogs/OAuthLogin.hpp>
#include <docks/BrowserDock.hpp>
#include <utility/RemoteTextThread.hpp>
#include <utility/obf.h>
#include <widgets/OBSBasic.hpp>
#include <qt-wrappers.hpp>
#include <ui-config.h>
#include <json11.hpp>

#include "moc_RestreamAuth.cpp"

using namespace json11;

/* ------------------------------------------------------------------------- */

#define RESTREAM_AUTH_URL OAUTH_BASE_URL "v1/restream/redirect"
#define RESTREAM_TOKEN_URL OAUTH_BASE_URL "v1/restream/token"
#define RESTREAM_API_URL "https://api.restream.io/v2/user"
#define RESTREAM_SCOPE_VERSION 1
#define RESTREAM_CHAT_DOCK_NAME "restreamChat"
#define RESTREAM_INFO_DOCK_NAME "restreamInfo"
#define RESTREAM_CHANNELS_DOCK_NAME "restreamChannel"
#define RESTREAM_SECTION_NAME "Restream"

static Auth::Def restreamDef = {"Restream", Auth::Type::OAuth_StreamKey, false, true};

/* ------------------------------------------------------------------------- */

RestreamAuth::RestreamAuth(const Def &d) : OAuthStreamKey(d) {}

RestreamAuth::~RestreamAuth()
{
	if (!uiLoaded)
		return;

	OBSBasic *main = OBSBasic::Get();

	main->RemoveDockWidget(RESTREAM_CHAT_DOCK_NAME);
	main->RemoveDockWidget(RESTREAM_INFO_DOCK_NAME);
	main->RemoveDockWidget(RESTREAM_CHANNELS_DOCK_NAME);
}

QVector<RestreamEventDescription> RestreamAuth::GetBroadcastInfo()
try {
	QVector<RestreamEventDescription> events;

	std::string client_id = RESTREAM_CLIENTID;
	deobfuscate_str(&client_id[0], RESTREAM_HASH);

	if (!GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION))
		return events;

	if (token.empty())
		return events;

	std::string auth;
	auth += "Authorization: Bearer ";
	auth += token;

	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(std::move(auth));

	std::string output;
	std::string error;
	Json json;
	bool success;

	auto func = [&]() {
		auto url = QString("%1/events/upcoming?source=2&sort=scheduled").arg(RESTREAM_API_URL);
		success = GetRemoteFile(url.toUtf8(), output, error, nullptr, "application/json", "", nullptr, headers,
					nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QTStr("Auth.LoadingChannel.Title"),
				    QTStr("Auth.LoadingChannel.Text").arg(service()));
	if (!success || output.empty())
		throw ErrorInfo("Failed to get upcoming events info from remote", error);

	json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	error = json["error"].string_value();
	if (!error.empty())
		throw ErrorInfo(error, json["error_description"].string_value());

	auto items = json.array_items();
	if (!items.size()) {
		OBSMessageBox::warning(OBSBasic::Get(), QTStr("Restream.Actions.BroadcastLoadingFailureTitle"),
				       QTStr("Restream.Actions.BroadcastLoadingEmptyText"));
		return QVector<RestreamEventDescription>();
	}

	for (auto item : items) {
		RestreamEventDescription event;
		event.id = item["id"].string_value();
		event.title = item["title"].string_value();
		event.scheduledFor = item["scheduledFor"].is_number() ? item["scheduledFor"].int_value() : 0;
		event.showId = item["showId"].string_value();
		events.push_back(event);
	}

	std::sort(events.begin(), events.end(),
		  [](const RestreamEventDescription &a, const RestreamEventDescription &b) {
			  return a.scheduledFor && (!b.scheduledFor || a.scheduledFor < b.scheduledFor);
		  });

	return events;
} catch (ErrorInfo info) {
	QString title = QTStr("Restream.Actions.BroadcastLoadingFailureTitle");
	QString text = QTStr("Restream.Actions.BroadcastLoadingFailureText")
			       .arg(service(), info.message.c_str(), info.error.c_str());
	QMessageBox::warning(OBSBasic::Get(), title, text);
	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(), info.error.c_str());
	return QVector<RestreamEventDescription>();
}

std::string RestreamAuth::GetStreamingKey(std::string eventId)
try {
	std::string client_id = RESTREAM_CLIENTID;
	deobfuscate_str(&client_id[0], RESTREAM_HASH);

	if (!GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION))
		return "";

	if (token.empty())
		return "";

	std::string auth;
	auth += "Authorization: Bearer ";
	auth += token;

	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(std::move(auth));

	auto url = eventId.empty() || eventId == "default"
			   ? QString("%1/streamKey").arg(RESTREAM_API_URL)
			   : QString("%1/events/%2/streamKey").arg(RESTREAM_API_URL, QString::fromStdString(eventId));

	std::string output;
	std::string error;
	Json json;
	bool success;

	auto func = [&, url]() {
		success = GetRemoteFile(url.toUtf8(), output, error, nullptr, "application/json", "", nullptr, headers,
					nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QTStr("Auth.LoadingChannel.Title"),
				    QTStr("Auth.LoadingChannel.Text").arg(service()));
	if (!success || output.empty())
		throw ErrorInfo("Failed to get the stream key from remote", error);

	json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	error = json["error"].string_value();
	if (!error.empty())
		throw ErrorInfo(error, json["error_description"].string_value());

	return json["streamKey"].string_value();
} catch (ErrorInfo info) {
	QString title = QTStr("Restream.Actions.BroadcastLoadingFailureTitle");
	QString text = QTStr("Restream.Actions.BroadcastLoadingFailureText")
			       .arg(service(), info.message.c_str(), info.error.c_str());
	QMessageBox::warning(OBSBasic::Get(), title, text);
	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(), info.error.c_str());
	return "";
}

void RestreamAuth::ResetShow()
{
	this->key_ = "";
	this->showId = "";
}

bool RestreamAuth::SelectShow(std::string eventId, std::string showId)
{
	auto key = GetStreamingKey(eventId);
	if (key.empty()) {
		this->key_ = "";
		this->showId = "";
		return false;
	}

	if (this->key_ != key || this->showId != showId) {
		this->key_ = key;
		this->showId = showId;

		OBSBasic *main = OBSBasic::Get();
		obs_service_t *service = main->GetService();
		OBSDataAutoRelease settings = obs_service_get_settings(service);
		obs_data_set_string(settings, "key", key.c_str());
		obs_service_update(service, settings);

		auto showIdParam = !showId.empty() ? QString("?show-id=%1").arg(QString::fromStdString(showId))
						   : QString("");

		if (chatWidgetBrowser) {
			auto url = QString("https://restream.io/chat-application%1").arg(showIdParam);
			chatWidgetBrowser->setURL(url.toStdString());
		}

		if (titlesWidgetBrowser) {
			auto url = QString("https://restream.io/titles/embed%1").arg(showIdParam);
			titlesWidgetBrowser->setURL(url.toStdString());
		}

		if (channelWidgetBrowser) {
			auto url = QString("https://restream.io/channel/embed%1").arg(showIdParam);
			channelWidgetBrowser->setURL(url.toStdString());
		}
	}

	return true;
}

std::string RestreamAuth::GetShowId()
{
	return showId;
}

bool RestreamAuth::IsBroadcastReady()
{
	return !key_.empty();
}

void RestreamAuth::SaveInternal()
{
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "DockState", main->saveState().toBase64().constData());
	config_set_string(main->Config(), service(), "ShowId", showId.c_str());

	OAuthStreamKey::SaveInternal();
}

bool RestreamAuth::LoadInternal()
{
	firstLoad = false;

	OBSBasic *main = OBSBasic::Get();
	auto showIdVal = config_get_string(main->Config(), service(), "ShowId");
	showId = showIdVal ? showIdVal : "";

	return OAuthStreamKey::LoadInternal();
}

void RestreamAuth::LoadUI()
{
	if (uiLoaded)
		return;

	// Select the previous event
	if (key_.empty()) {
		auto foundEventId = std::string("");
		auto foundShowId = std::string("");

		auto events = GetBroadcastInfo();
		for (auto event : events) {
			if (event.showId == showId) {
				foundEventId = event.id;
				foundShowId = event.showId;
				break;
			}
		}

		if (foundShowId.empty() && events.size()) {
			auto event = events.at(0);
			foundEventId = event.id;
			foundShowId = event.showId;
		}

		SelectShow(foundEventId, foundShowId);
	}

#ifdef BROWSER_AVAILABLE
	if (!cef)
		return;

	OBSBasic::InitBrowserPanelSafeBlock();
	OBSBasic *main = OBSBasic::Get();
	auto showIdParam = !showId.empty() ? QString("?show-id=%1").arg(QString::fromStdString(showId)) : QString("");

	QSize size = main->frameSize();
	QPoint pos = main->pos();

	BrowserDock *chat = new BrowserDock(QTStr("Auth.Chat"));
	chat->setObjectName(RESTREAM_CHAT_DOCK_NAME);
	chat->resize(420, 600);
	chat->setMinimumSize(200, 300);
	chat->setWindowTitle(QTStr("Auth.Chat"));
	chat->setAllowedAreas(Qt::AllDockWidgetAreas);

	auto url = QString("https://restream.io/chat-application%1").arg(showIdParam);
	chatWidgetBrowser = cef->create_widget(chat, url.toStdString(), panel_cookies);
	chat->SetWidget(chatWidgetBrowser);

	main->AddDockWidget(chat, Qt::RightDockWidgetArea);

	/* ----------------------------------- */

	BrowserDock *info = new BrowserDock(QTStr("Auth.StreamInfo"));
	info->setObjectName(RESTREAM_INFO_DOCK_NAME);
	info->resize(410, 600);
	info->setMinimumSize(200, 150);
	info->setWindowTitle(QTStr("Auth.StreamInfo"));
	info->setAllowedAreas(Qt::AllDockWidgetAreas);

	url = QString("https://restream.io/titles/embed%1").arg(showIdParam);
	titlesWidgetBrowser = cef->create_widget(info, url.toStdString(), panel_cookies);
	info->SetWidget(titlesWidgetBrowser);

	main->AddDockWidget(info, Qt::LeftDockWidgetArea);

	/* ----------------------------------- */

	BrowserDock *channels = new BrowserDock(QTStr("RestreamAuth.Channels"));
	channels->setObjectName(RESTREAM_CHANNELS_DOCK_NAME);
	channels->resize(410, 600);
	channels->setMinimumSize(410, 300);
	channels->setWindowTitle(QTStr("RestreamAuth.Channels"));
	channels->setAllowedAreas(Qt::AllDockWidgetAreas);

	url = QString("https://restream.io/channel/embed%1").arg(showIdParam);
	channelWidgetBrowser = cef->create_widget(channels, url.toStdString(), panel_cookies);
	channels->SetWidget(channelWidgetBrowser);

	main->AddDockWidget(channels, Qt::LeftDockWidgetArea);

	/* ----------------------------------- */

	chat->setFloating(true);
	info->setFloating(true);
	channels->setFloating(true);

	chat->move(pos.x() + size.width() - chat->width() - 30, pos.y() + 60);
	info->move(pos.x() + 20, pos.y() + 60);
	channels->move(pos.x() + 20 + info->width() + 10, pos.y() + 60);

	if (firstLoad) {
		chat->setVisible(true);
		info->setVisible(true);
		channels->setVisible(true);
	} else {
		const char *dockStateStr = config_get_string(main->Config(), service(), "DockState");
		QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));
		main->restoreState(dockState);
	}
#endif

	uiLoaded = true;
}

bool RestreamAuth::RetryLogin()
{
	OAuthLogin login(OBSBasic::Get(), RESTREAM_AUTH_URL, false);
	cef->add_popup_whitelist_url("about:blank", &login);
	if (login.exec() == QDialog::Rejected) {
		return false;
	}

	std::shared_ptr<RestreamAuth> auth = std::make_shared<RestreamAuth>(restreamDef);

	std::string client_id = RESTREAM_CLIENTID;
	deobfuscate_str(&client_id[0], RESTREAM_HASH);

	return GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION, QT_TO_UTF8(login.GetCode()), true);
}

std::shared_ptr<Auth> RestreamAuth::Login(QWidget *parent, const std::string &)
{
	OAuthLogin login(parent, RESTREAM_AUTH_URL, false);
	cef->add_popup_whitelist_url("about:blank", &login);

	if (login.exec() == QDialog::Rejected)
		return nullptr;

	std::shared_ptr<RestreamAuth> restreamAuth = std::make_shared<RestreamAuth>(restreamDef);

	std::string client_id = RESTREAM_CLIENTID;
	deobfuscate_str(&client_id[0], RESTREAM_HASH);

	if (!restreamAuth->GetToken(RESTREAM_TOKEN_URL, client_id, RESTREAM_SCOPE_VERSION, QT_TO_UTF8(login.GetCode())))
		return nullptr;

	return restreamAuth;
}

static std::shared_ptr<Auth> CreateRestreamAuth()
{
	return std::make_shared<RestreamAuth>(restreamDef);
}

static void DeleteCookies()
{
	if (panel_cookies) {
		panel_cookies->DeleteCookies("restream.io", std::string());
	}
}

void RegisterRestreamAuth()
{
#if !defined(__APPLE__) && !defined(_WIN32)
	if (QApplication::platformName().contains("wayland"))
		return;
#endif

	OAuth::RegisterOAuth(restreamDef, CreateRestreamAuth, RestreamAuth::Login, DeleteCookies);
}

bool IsRestreamService(const std::string &service)
{
	return service == restreamDef.service;
}
