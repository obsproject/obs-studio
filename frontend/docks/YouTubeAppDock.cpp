#include "YouTubeAppDock.hpp"

#include <utility/YoutubeApiWrappers.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QUuid>
#include <nlohmann/json.hpp>

#include "moc_YouTubeAppDock.cpp"

using json = nlohmann::json;

extern bool cef_js_avail;

#ifdef YOUTUBE_WEBAPP_PLACEHOLDER
static constexpr const char *YOUTUBE_WEBAPP_PLACEHOLDER_URL = YOUTUBE_WEBAPP_PLACEHOLDER;
#else
static constexpr const char *YOUTUBE_WEBAPP_PLACEHOLDER_URL =
	"https://studio.youtube.com/live/channel/UC/console?kc=OBS";
#endif

#ifdef YOUTUBE_WEBAPP_ADDRESS
static constexpr const char *YOUTUBE_WEBAPP_ADDRESS_URL = YOUTUBE_WEBAPP_ADDRESS;
#else
static constexpr const char *YOUTUBE_WEBAPP_ADDRESS_URL = "https://studio.youtube.com/live/channel/%1/console?kc=OBS";
#endif

static constexpr const char *BROADCAST_CREATED = "BROADCAST_CREATED";
static constexpr const char *BROADCAST_SELECTED = "BROADCAST_SELECTED";
static constexpr const char *INGESTION_STARTED = "INGESTION_STARTED";
static constexpr const char *INGESTION_STOPPED = "INGESTION_STOPPED";

YouTubeAppDock::YouTubeAppDock(const QString &title) : BrowserDock(title), dockBrowser(nullptr)
{
	cef->init_browser();
	OBSBasic::InitBrowserPanelSafeBlock();
	AddYouTubeAppDock();
}

bool YouTubeAppDock::IsYTServiceSelected()
{
	if (!cef_js_avail)
		return false;

	obs_service_t *service_obj = OBSBasic::Get()->GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
	const char *service = obs_data_get_string(settings, "service");
	return IsYouTubeService(service);
}

void YouTubeAppDock::AccountConnected()
{
	channelId.clear(); // renew channel id
	UpdateChannelId();
}

void YouTubeAppDock::AccountDisconnected()
{
	SettingsUpdated(true);
}

void YouTubeAppDock::SettingsUpdated(bool cleanup)
{
	bool ytservice = IsYTServiceSelected();
	SetVisibleYTAppDockInMenu(ytservice);

	// definitely cleanup if YT switched off
	if (!ytservice || cleanup) {
		if (panel_cookies) {
			panel_cookies->DeleteCookies("youtube.com", "");
			panel_cookies->DeleteCookies("google.com", "");
		}
	}

	if (ytservice)
		Update();
}

std::string YouTubeAppDock::InitYTUserUrl()
{
	std::string user_url(YOUTUBE_WEBAPP_PLACEHOLDER_URL);

	if (IsUserSignedIntoYT()) {
		YoutubeApiWrappers *apiYouTube = GetYTApi();
		if (apiYouTube) {
			ChannelDescription channel_description;
			if (apiYouTube->GetChannelDescription(channel_description)) {
				QString url = QString(YOUTUBE_WEBAPP_ADDRESS_URL).arg(channel_description.id);
				user_url = url.toStdString();
			} else {
				blog(LOG_ERROR, "YT: InitYTUserUrl() Failed to get channel id");
			}
		}
	} else {
		blog(LOG_ERROR, "YT: InitYTUserUrl() User is not signed");
	}

	blog(LOG_DEBUG, "YT: InitYTUserUrl() User url: %s", user_url.c_str());
	return user_url;
}

void YouTubeAppDock::AddYouTubeAppDock()
{
	QString bId(QUuid::createUuid().toString());
	bId.replace(QRegularExpression("[{}-]"), "");
	this->setProperty("uuid", bId);
	this->setObjectName("youtubeLiveControlPanel");
	this->resize(580, 500);
	this->setMinimumSize(400, 300);
	this->setAllowedAreas(Qt::AllDockWidgetAreas);

	OBSBasic::Get()->AddDockWidget(this, Qt::RightDockWidgetArea);

	if (IsYTServiceSelected()) {
		const std::string url = InitYTUserUrl();
		CreateBrowserWidget(url);
	} else {
		this->setVisible(false);
		this->toggleViewAction()->setVisible(false);
	}
}

void YouTubeAppDock::CreateBrowserWidget(const std::string &url)
{
	if (dockBrowser)
		delete dockBrowser;
	dockBrowser = cef->create_widget(this, url, panel_cookies);
	if (!dockBrowser)
		return;

	if (obs_browser_qcef_version() >= 1)
		dockBrowser->allowAllPopups(true);

	this->SetWidget(dockBrowser);

	QWidget::connect(dockBrowser.get(), &QCefWidget::urlChanged, this, &YouTubeAppDock::ReloadChatDock);

	Update();
}

void YouTubeAppDock::SetVisibleYTAppDockInMenu(bool visible)
{
	if (visible && toggleViewAction()->isVisible())
		return;

	toggleViewAction()->setVisible(visible);
	this->setVisible(visible);
}

// only 'ACCOUNT' mode supported
void YouTubeAppDock::BroadcastCreated(const char *stream_id)
{
	DispatchYTEvent(BROADCAST_CREATED, stream_id, YTSM_ACCOUNT);
}

// only 'ACCOUNT' mode supported
void YouTubeAppDock::BroadcastSelected(const char *stream_id)
{
	DispatchYTEvent(BROADCAST_SELECTED, stream_id, YTSM_ACCOUNT);
}

// both 'ACCOUNT' and 'STREAM_KEY' modes supported
void YouTubeAppDock::IngestionStarted()
{
	obs_service_t *service_obj = OBSBasic::Get()->GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
	const char *service = obs_data_get_string(settings, "service");
	if (IsYouTubeService(service)) {
		if (IsUserSignedIntoYT()) {
			const char *broadcast_id = obs_data_get_string(settings, "broadcast_id");
			this->IngestionStarted(broadcast_id, YouTubeAppDock::YTSM_ACCOUNT);
		} else {
			const char *stream_key = obs_data_get_string(settings, "key");
			this->IngestionStarted(stream_key, YouTubeAppDock::YTSM_STREAM_KEY);
		}
	}
}

void YouTubeAppDock::IngestionStarted(const char *stream_id, streaming_mode_t mode)
{
	DispatchYTEvent(INGESTION_STARTED, stream_id, mode);
}

// both 'ACCOUNT' and 'STREAM_KEY' modes supported
void YouTubeAppDock::IngestionStopped()
{
	obs_service_t *service_obj = OBSBasic::Get()->GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
	const char *service = obs_data_get_string(settings, "service");

	if (IsYouTubeService(service)) {
		if (IsUserSignedIntoYT()) {
			const char *broadcast_id = obs_data_get_string(settings, "broadcast_id");
			this->IngestionStopped(broadcast_id, YouTubeAppDock::YTSM_ACCOUNT);
		} else {
			const char *stream_key = obs_data_get_string(settings, "key");
			this->IngestionStopped(stream_key, YouTubeAppDock::YTSM_STREAM_KEY);
		}
	}
}

void YouTubeAppDock::IngestionStopped(const char *stream_id, streaming_mode_t mode)
{
	DispatchYTEvent(INGESTION_STOPPED, stream_id, mode);
}

void YouTubeAppDock::showEvent(QShowEvent *)
{
	if (!dockBrowser)
		Update();
}

void YouTubeAppDock::closeEvent(QCloseEvent *event)
{
	BrowserDock::closeEvent(event);
	this->SetWidget(nullptr);
}

void YouTubeAppDock::DispatchYTEvent(const char *event, const char *video_id, streaming_mode_t mode)
{
	if (!dockBrowser)
		return;

	// update channelId if empty:
	UpdateChannelId();

	// notify YouTube Live Streaming API:
	std::string script;
	if (mode == YTSM_ACCOUNT) {
		script = QString(R"""(
			if (window.location.hostname == 'studio.youtube.com') {
				let event = {
					type: '%1',
					channelId: '%2',
					videoId: '%3',
				};
				console.log(event);
				if (window.ytlsapi && window.ytlsapi.dispatchEvent)
					window.ytlsapi.dispatchEvent(event);
			}
		)""")
				 .arg(event)
				 .arg(channelId)
				 .arg(video_id)
				 .toStdString();
	} else {
		const char *stream_key = video_id;
		script = QString(R"""(
			if (window.location.hostname == 'studio.youtube.com') {
				let event = {
					type: '%1',
					streamKey: '%2',
				};
				console.log(event);
				if (window.ytlsapi && window.ytlsapi.dispatchEvent)
					window.ytlsapi.dispatchEvent(event);
			}
		)""")
				 .arg(event)
				 .arg(stream_key)
				 .toStdString();
	}
	dockBrowser->executeJavaScript(script);

	// in case of user still not logged in in dock panel, remember last event
	SetInitEvent(mode, event, video_id, channelId.toStdString().c_str());
}

void YouTubeAppDock::Update()
{
	std::string url = InitYTUserUrl();

	if (!dockBrowser) {
		CreateBrowserWidget(url);
	} else {
		dockBrowser->setURL(url);
	}

	// if streaming already run, let's notify YT about past event
	if (OBSBasic::Get()->StreamingActive()) {
		obs_service_t *service_obj = OBSBasic::Get()->GetService();
		OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
		if (IsUserSignedIntoYT()) {
			channelId.clear(); // renew channelId
			UpdateChannelId();
			const char *broadcast_id = obs_data_get_string(settings, "broadcast_id");
			SetInitEvent(YTSM_ACCOUNT, INGESTION_STARTED, broadcast_id, channelId.toStdString().c_str());
		} else {
			const char *stream_key = obs_data_get_string(settings, "key");
			SetInitEvent(YTSM_STREAM_KEY, INGESTION_STARTED, stream_key);
		}
	} else {
		SetInitEvent(IsUserSignedIntoYT() ? YTSM_ACCOUNT : YTSM_STREAM_KEY);
	}

	dockBrowser->reloadPage();
}

void YouTubeAppDock::UpdateChannelId()
{
	if (channelId.isEmpty()) {
		YoutubeApiWrappers *apiYouTube = GetYTApi();
		if (apiYouTube) {
			ChannelDescription channel_description;
			if (apiYouTube->GetChannelDescription(channel_description)) {
				channelId = channel_description.id;
			} else {
				blog(LOG_ERROR, "YT: AccountConnected() Failed "
						"to get channel id");
			}
		}
	}
}

void YouTubeAppDock::ReloadChatDock()
{
	if (IsUserSignedIntoYT()) {
		YoutubeApiWrappers *apiYouTube = GetYTApi();
		if (apiYouTube) {
			apiYouTube->ReloadChat();
		}
	}
}

void YouTubeAppDock::SetInitEvent(streaming_mode_t mode, const char *event, const char *video_id, const char *channelId)
{
	const std::string version = App()->GetVersionString();

	QString api_event;
	if (event) {
		if (mode == YTSM_ACCOUNT) {
			api_event = QString(R"""(,
					initEvent: {
						type: '%1',
						channelId: '%2',
						videoId: '%3',
					}
			)""")
					    .arg(event)
					    .arg(channelId)
					    .arg(video_id);
		} else {
			api_event = QString(R"""(,
					initEvent: {
						type: '%1',
						streamKey: '%2',
					}
			)""")
					    .arg(event)
					    .arg(video_id);
		}
	}

	std::string script = QString(R"""(
		let obs_name = '%1';
		let obs_version = '%2';
		let client_mode = %3;
		if (window.location.hostname == 'studio.youtube.com') {
			console.log("name:", obs_name);
			console.log("version:", obs_version);
			console.log("initEvent:", {
					initClientMode: client_mode
					%4 });
			if (window.ytlsapi && window.ytlsapi.init)
				window.ytlsapi.init(obs_name, obs_version, undefined, {
					initClientMode: client_mode
					%4 });
		}
	)""")
				     .arg("OBS")
				     .arg(version.c_str())
				     .arg(mode == YTSM_ACCOUNT ? "'ACCOUNT'" : "'STREAM_KEY'")
				     .arg(api_event)
				     .toStdString();
	dockBrowser->setStartupScript(script);
}

YoutubeApiWrappers *YouTubeAppDock::GetYTApi()
{
	Auth *auth = OBSBasic::Get()->GetAuth();
	if (auth) {
		YoutubeApiWrappers *apiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth));
		if (apiYouTube) {
			return apiYouTube;
		} else {
			blog(LOG_ERROR, "YT: GetYTApi() Failed to get YoutubeApiWrappers");
		}
	} else {
		blog(LOG_ERROR, "YT: GetYTApi() Failed to get Auth");
	}
	return nullptr;
}

void YouTubeAppDock::CleanupYouTubeUrls()
{
	if (!cef_js_avail)
		return;

	static constexpr const char *YOUTUBE_VIDEO_URL = "://studio.youtube.com/video/";
	// remove legacy YouTube Browser Docks (once)

	bool youtube_cleanup_done = config_get_bool(App()->GetUserConfig(), "General", "YtDockCleanupDone");

	if (youtube_cleanup_done)
		return;

	config_set_bool(App()->GetUserConfig(), "General", "YtDockCleanupDone", true);

	const char *jsonStr = config_get_string(App()->GetUserConfig(), "BasicWindow", "ExtraBrowserDocks");
	if (!jsonStr)
		return;

	json array = json::parse(jsonStr);
	if (!array.is_array())
		return;

	json save_array;
	std::string removedYTUrl;

	for (json &item : array) {
		auto url = item["url"].get<std::string>();

		if (url.find(YOUTUBE_VIDEO_URL) != std::string::npos) {
			blog(LOG_DEBUG, "YT: found legacy url: %s", url.c_str());
			removedYTUrl += url;
			removedYTUrl += ";\n";
		} else {
			save_array.push_back(item);
		}
	}

	if (!removedYTUrl.empty()) {
		const QString msg_title = QTStr("YouTube.DocksRemoval.Title");
		const QString msg_text = QTStr("YouTube.DocksRemoval.Text").arg(QT_UTF8(removedYTUrl.c_str()));
		OBSMessageBox::warning(OBSBasic::Get(), msg_title, msg_text);

		std::string output = save_array.dump();
		config_set_string(App()->GetUserConfig(), "BasicWindow", "ExtraBrowserDocks", output.c_str());
	}
}
