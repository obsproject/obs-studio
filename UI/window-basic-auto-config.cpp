#include <QMessageBox>
#include <QScreen>

#include <obs.hpp>
#include <qt-wrappers.hpp>

#include <nlohmann/json.hpp>

#include "moc_window-basic-auto-config.cpp"
#include "window-basic-main.hpp"
#include "obs-app.hpp"
#include "url-push-button.hpp"

#include "goliveapi-postdata.hpp"
#include "goliveapi-network.hpp"
#include "multitrack-video-error.hpp"

#include "ui_AutoConfigStartPage.h"
#include "ui_AutoConfigVideoPage.h"
#include "ui_AutoConfigStreamPage.h"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#include "auth-oauth.hpp"
#include "ui-config.h"
#ifdef YOUTUBE_ENABLED
#include "youtube-api-wrappers.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

#define wiz reinterpret_cast<AutoConfig *>(wizard())

/* ------------------------------------------------------------------------- */

constexpr std::string_view OBSServiceFileName = "service.json";

static OBSData OpenServiceSettings(std::string &type)
{
	const OBSBasic *basic = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	const OBSProfile &currentProfile = basic->GetCurrentProfile();

	const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(OBSServiceFileName);

	if (!std::filesystem::exists(jsonFilePath)) {
		return OBSData();
	}

	OBSDataAutoRelease data = obs_data_create_from_json_file_safe(jsonFilePath.u8string().c_str(), "bak");

	obs_data_set_default_string(data, "type", "rtmp_common");
	type = obs_data_get_string(data, "type");

	OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");

	return settings.Get();
}

static void GetServiceInfo(std::string &type, std::string &service, std::string &server, std::string &key)
{
	OBSData settings = OpenServiceSettings(type);

	service = obs_data_get_string(settings, "service");
	server = obs_data_get_string(settings, "server");
	key = obs_data_get_string(settings, "key");
}

/* ------------------------------------------------------------------------- */

AutoConfigStartPage::AutoConfigStartPage(QWidget *parent) : QWizardPage(parent), ui(new Ui_AutoConfigStartPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StartPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StartPage.SubTitle"));

	OBSBasic *main = OBSBasic::Get();
	if (main->VCamEnabled()) {
		QRadioButton *prioritizeVCam =
			new QRadioButton(QTStr("Basic.AutoConfig.StartPage.PrioritizeVirtualCam"), this);
		QBoxLayout *box = reinterpret_cast<QBoxLayout *>(layout());
		box->insertWidget(2, prioritizeVCam);

		connect(prioritizeVCam, &QPushButton::clicked, this, &AutoConfigStartPage::PrioritizeVCam);
	}
}

AutoConfigStartPage::~AutoConfigStartPage() {}

int AutoConfigStartPage::nextId() const
{
	return wiz->type == AutoConfig::Type::VirtualCam ? AutoConfig::TestPage : AutoConfig::VideoPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

void AutoConfigStartPage::PrioritizeVCam()
{
	wiz->type = AutoConfig::Type::VirtualCam;
}

/* ------------------------------------------------------------------------- */

#define RES_TEXT(x) "Basic.AutoConfig.VideoPage." x
#define RES_USE_CURRENT RES_TEXT("BaseResolution.UseCurrent")
#define RES_USE_DISPLAY RES_TEXT("BaseResolution.Display")
#define FPS_USE_CURRENT RES_TEXT("FPS.UseCurrent")
#define FPS_PREFER_HIGH_FPS RES_TEXT("FPS.PreferHighFPS")
#define FPS_PREFER_HIGH_RES RES_TEXT("FPS.PreferHighRes")

AutoConfigVideoPage::AutoConfigVideoPage(QWidget *parent) : QWizardPage(parent), ui(new Ui_AutoConfigVideoPage)
{
	ui->setupUi(this);

	setTitle(QTStr("Basic.AutoConfig.VideoPage"));
	setSubTitle(QTStr("Basic.AutoConfig.VideoPage.SubTitle"));

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	long double fpsVal = (long double)ovi.fps_num / (long double)ovi.fps_den;

	QString fpsStr = (ovi.fps_den > 1) ? QString::number(fpsVal, 'f', 2) : QString::number(fpsVal, 'g');

	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_FPS), (int)AutoConfig::FPSType::PreferHighFPS);
	ui->fps->addItem(QTStr(FPS_PREFER_HIGH_RES), (int)AutoConfig::FPSType::PreferHighRes);
	ui->fps->addItem(QTStr(FPS_USE_CURRENT).arg(fpsStr), (int)AutoConfig::FPSType::UseCurrent);
	ui->fps->addItem(QStringLiteral("30"), (int)AutoConfig::FPSType::fps30);
	ui->fps->addItem(QStringLiteral("60"), (int)AutoConfig::FPSType::fps60);
	ui->fps->setCurrentIndex(0);

	QString cxStr = QString::number(ovi.base_width);
	QString cyStr = QString::number(ovi.base_height);

	int encRes = int(ovi.base_width << 16) | int(ovi.base_height);

	// Auto config only supports testing down to 240p, don't allow current
	// resolution if it's lower than that.
	if (ovi.base_height >= 240)
		ui->canvasRes->addItem(QTStr(RES_USE_CURRENT).arg(cxStr, cyStr), (int)encRes);

	QList<QScreen *> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QSize as = screen->size();
		int as_width = as.width();
		int as_height = as.height();

		// Calculate physical screen resolution based on the virtual screen resolution
		// They might differ if scaling is enabled, e.g. for HiDPI screens
		as_width = round(as_width * screen->devicePixelRatio());
		as_height = round(as_height * screen->devicePixelRatio());

		encRes = as_width << 16 | as_height;

		QString str =
			QTStr(RES_USE_DISPLAY)
				.arg(QString::number(i + 1), QString::number(as_width), QString::number(as_height));

		ui->canvasRes->addItem(str, encRes);
	}

	auto addRes = [&](int cx, int cy) {
		encRes = (cx << 16) | cy;
		QString str = QString("%1x%2").arg(QString::number(cx), QString::number(cy));
		ui->canvasRes->addItem(str, encRes);
	};

	addRes(1920, 1080);
	addRes(1280, 720);

	ui->canvasRes->setCurrentIndex(0);
}

AutoConfigVideoPage::~AutoConfigVideoPage() {}

int AutoConfigVideoPage::nextId() const
{
	return wiz->type == AutoConfig::Type::Recording ? AutoConfig::TestPage : AutoConfig::StreamPage;
}

bool AutoConfigVideoPage::validatePage()
{
	int encRes = ui->canvasRes->currentData().toInt();
	wiz->baseResolutionCX = encRes >> 16;
	wiz->baseResolutionCY = encRes & 0xFFFF;
	wiz->fpsType = (AutoConfig::FPSType)ui->fps->currentData().toInt();

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	switch (wiz->fpsType) {
	case AutoConfig::FPSType::PreferHighFPS:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = true;
		break;
	case AutoConfig::FPSType::PreferHighRes:
		wiz->specificFPSNum = 0;
		wiz->specificFPSDen = 0;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::UseCurrent:
		wiz->specificFPSNum = ovi.fps_num;
		wiz->specificFPSDen = ovi.fps_den;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::fps30:
		wiz->specificFPSNum = 30;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	case AutoConfig::FPSType::fps60:
		wiz->specificFPSNum = 60;
		wiz->specificFPSDen = 1;
		wiz->preferHighFPS = false;
		break;
	}

	return true;
}

/* ------------------------------------------------------------------------- */

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

AutoConfigStreamPage::AutoConfigStreamPage(QWidget *parent) : QWizardPage(parent), ui(new Ui_AutoConfigStreamPage)
{
	ui->setupUi(this);
	ui->bitrateLabel->setVisible(false);
	ui->bitrate->setVisible(false);
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->useMultitrackVideo->setVisible(false);

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);

	int vertSpacing = ui->topLayout->verticalSpacing();

	QMargins m = ui->topLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topLayout->setContentsMargins(m);

	m = ui->loginPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->loginPageLayout->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	setTitle(QTStr("Basic.AutoConfig.StreamPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StreamPage.SubTitle"));

	LoadServices(false);

	connect(ui->service, &QComboBox::currentIndexChanged, this, &AutoConfigStreamPage::ServiceChanged);
	connect(ui->customServer, &QLineEdit::textChanged, this, &AutoConfigStreamPage::ServiceChanged);
	connect(ui->customServer, &QLineEdit::textChanged, this, &AutoConfigStreamPage::UpdateKeyLink);
	connect(ui->customServer, &QLineEdit::editingFinished, this, &AutoConfigStreamPage::UpdateKeyLink);
	connect(ui->doBandwidthTest, &QCheckBox::toggled, this, &AutoConfigStreamPage::ServiceChanged);

	connect(ui->service, &QComboBox::currentIndexChanged, this, &AutoConfigStreamPage::UpdateServerList);

	connect(ui->service, &QComboBox::currentIndexChanged, this, &AutoConfigStreamPage::UpdateKeyLink);
	connect(ui->service, &QComboBox::currentIndexChanged, this, &AutoConfigStreamPage::UpdateMoreInfoLink);

	connect(ui->useStreamKeyAdv, &QPushButton::clicked, [&]() {
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->useStreamKeyAdv->setVisible(false);
	});

	connect(ui->key, &QLineEdit::textChanged, this, &AutoConfigStreamPage::UpdateCompleted);
	connect(ui->regionUS, &QCheckBox::toggled, this, &AutoConfigStreamPage::UpdateCompleted);
	connect(ui->regionEU, &QCheckBox::toggled, this, &AutoConfigStreamPage::UpdateCompleted);
	connect(ui->regionAsia, &QCheckBox::toggled, this, &AutoConfigStreamPage::UpdateCompleted);
	connect(ui->regionOther, &QCheckBox::toggled, this, &AutoConfigStreamPage::UpdateCompleted);
}

AutoConfigStreamPage::~AutoConfigStreamPage() {}

bool AutoConfigStreamPage::isComplete() const
{
	return ready;
}

int AutoConfigStreamPage::nextId() const
{
	return AutoConfig::TestPage;
}

inline bool AutoConfigStreamPage::IsCustomService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Custom;
}

bool AutoConfigStreamPage::validatePage()
{
	OBSDataAutoRelease service_settings = obs_data_create();

	wiz->customServer = IsCustomService();

	const char *serverType = wiz->customServer ? "rtmp_custom" : "rtmp_common";

	if (!wiz->customServer) {
		obs_data_set_string(service_settings, "service", QT_TO_UTF8(ui->service->currentText()));
	}

	OBSServiceAutoRelease service = obs_service_create(serverType, "temp_service", service_settings, nullptr);

	int bitrate;
	if (!ui->doBandwidthTest->isChecked()) {
		bitrate = ui->bitrate->value();
		wiz->idealBitrate = bitrate;
	} else {
		/* Default test target is 10 Mbps */
		bitrate = 10000;
#ifdef YOUTUBE_ENABLED
		if (IsYouTubeService(wiz->serviceName)) {
			/* Adjust upper bound to YouTube limits
			 * for resolutions above 1080p */
			if (wiz->baseResolutionCY > 1440)
				bitrate = 51000;
			else if (wiz->baseResolutionCY > 1080)
				bitrate = 18000;
		}
#endif
	}

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", bitrate);
	obs_service_apply_encoder_settings(service, settings, nullptr);

	if (wiz->customServer) {
		QString server = ui->customServer->text().trimmed();
		wiz->server = wiz->serverName = QT_TO_UTF8(server);
	} else {
		wiz->serverName = QT_TO_UTF8(ui->server->currentText());
		wiz->server = QT_TO_UTF8(ui->server->currentData().toString());
	}

	wiz->bandwidthTest = ui->doBandwidthTest->isChecked();
	wiz->startingBitrate = (int)obs_data_get_int(settings, "bitrate");
	wiz->idealBitrate = wiz->startingBitrate;
	wiz->regionUS = ui->regionUS->isChecked();
	wiz->regionEU = ui->regionEU->isChecked();
	wiz->regionAsia = ui->regionAsia->isChecked();
	wiz->regionOther = ui->regionOther->isChecked();
	wiz->serviceName = QT_TO_UTF8(ui->service->currentText());
	if (ui->preferHardware)
		wiz->preferHardware = ui->preferHardware->isChecked();
	wiz->key = QT_TO_UTF8(ui->key->text());

	if (!wiz->customServer) {
		if (wiz->serviceName == "Twitch")
			wiz->service = AutoConfig::Service::Twitch;
#ifdef YOUTUBE_ENABLED
		else if (IsYouTubeService(wiz->serviceName))
			wiz->service = AutoConfig::Service::YouTube;
#endif
		else if (wiz->serviceName == "Amazon IVS")
			wiz->service = AutoConfig::Service::AmazonIVS;
		else
			wiz->service = AutoConfig::Service::Other;
	} else {
		wiz->service = AutoConfig::Service::Other;
	}

	if (wiz->service == AutoConfig::Service::Twitch) {
		wiz->testMultitrackVideo = ui->useMultitrackVideo->isChecked();

		if (wiz->testMultitrackVideo) {
			auto postData = constructGoLivePost(QString::fromStdString(wiz->key), std::nullopt,
							    std::nullopt, false);

			OBSDataAutoRelease service_settings = obs_service_get_settings(service);
			auto multitrack_video_name = QTStr("Basic.Settings.Stream.MultitrackVideoLabel");
			if (obs_data_has_user_value(service_settings, "multitrack_video_name")) {
				multitrack_video_name = obs_data_get_string(service_settings, "multitrack_video_name");
			}

			try {
				auto config = DownloadGoLiveConfig(this, MultitrackVideoAutoConfigURL(service),
								   postData, multitrack_video_name);

				for (const auto &endpoint : config.ingest_endpoints) {
					if (qstrnicmp("RTMP", endpoint.protocol.c_str(), 4) != 0)
						continue;

					std::string address = endpoint.url_template;
					auto pos = address.find("/{stream_key}");
					if (pos != address.npos)
						address.erase(pos);

					wiz->serviceConfigServers.push_back({address, address});
				}

				int multitrackVideoBitrate = 0;
				for (auto &encoder_config : config.encoder_configurations) {
					auto it = encoder_config.settings.find("bitrate");
					if (it == encoder_config.settings.end())
						continue;

					if (!it->is_number_integer())
						continue;

					int bitrate = 0;
					it->get_to(bitrate);
					multitrackVideoBitrate += bitrate;
				}

				// grab a streamkey from the go live config if we can
				for (auto &endpoint : config.ingest_endpoints) {
					const char *p = endpoint.protocol.c_str();
					const char *auth = endpoint.authentication ? endpoint.authentication->c_str()
										   : nullptr;
					if (qstrnicmp("RTMP", p, 4) == 0 && auth && *auth) {
						wiz->key = auth;
						break;
					}
				}

				if (multitrackVideoBitrate > 0) {
					wiz->startingBitrate = multitrackVideoBitrate;
					wiz->idealBitrate = multitrackVideoBitrate;
					wiz->multitrackVideo.targetBitrate = multitrackVideoBitrate;
					wiz->multitrackVideo.testSuccessful = true;
				}
			} catch (const MultitrackVideoError & /*err*/) {
				// FIXME: do something sensible
			}
		}
	}

	if (wiz->service != AutoConfig::Service::Twitch && wiz->service != AutoConfig::Service::YouTube &&
	    wiz->service != AutoConfig::Service::AmazonIVS && wiz->bandwidthTest) {
		QMessageBox::StandardButton button;
#define WARNING_TEXT(x) QTStr("Basic.AutoConfig.StreamPage.StreamWarning." x)
		button = OBSMessageBox::question(this, WARNING_TEXT("Title"), WARNING_TEXT("Text"));
#undef WARNING_TEXT

		if (button == QMessageBox::No)
			return false;
	}

	return true;
}

void AutoConfigStreamPage::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void AutoConfigStreamPage::OnOAuthStreamKeyConnected()
{
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());

	if (a) {
		bool validKey = !a->key().empty();

		if (validKey)
			ui->key->setText(QT_UTF8(a->key().c_str()));

		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);
		ui->useStreamKeyAdv->setVisible(false);

		ui->connectedAccountLabel->setVisible(false);
		ui->connectedAccountText->setVisible(false);

#ifdef YOUTUBE_ENABLED
		if (IsYouTubeService(a->service())) {
			ui->key->clear();

			ui->connectedAccountLabel->setVisible(true);
			ui->connectedAccountText->setVisible(true);

			ui->connectedAccountText->setText(QTStr("Auth.LoadingChannel.Title"));

			YoutubeApiWrappers *ytAuth = reinterpret_cast<YoutubeApiWrappers *>(a);
			ChannelDescription cd;
			if (ytAuth->GetChannelDescription(cd)) {
				ui->connectedAccountText->setText(cd.title);

				/* Create throwaway stream key for bandwidth test */
				if (ui->doBandwidthTest->isChecked()) {
					StreamDescription stream = {"", "", "OBS Studio Test Stream"};
					if (ytAuth->InsertStream(stream)) {
						ui->key->setText(stream.name);
					}
				}
			}
		}
#endif
	}

	ui->stackedWidget->setCurrentIndex((int)Section::StreamKey);
	UpdateCompleted();
}

void AutoConfigStreamPage::OnAuthConnected()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	Auth::Type type = Auth::AuthType(service);

	if (type == Auth::Type::OAuth_StreamKey || type == Auth::Type::OAuth_LinkedAccount) {
		OnOAuthStreamKeyConnected();
	}
}

void AutoConfigStreamPage::on_connectAccount_clicked()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth) {
		OnAuthConnected();

		ui->useStreamKeyAdv->setVisible(false);
	}
}

#define DISCONNECT_COMFIRM_TITLE "Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Title"
#define DISCONNECT_COMFIRM_TEXT "Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Text"

void AutoConfigStreamPage::on_disconnectAccount_clicked()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr(DISCONNECT_COMFIRM_TITLE), QTStr(DISCONNECT_COMFIRM_TEXT));

	if (button == QMessageBox::No) {
		return;
	}

	OBSBasic *main = OBSBasic::Get();

	main->auth.reset();
	auth.reset();

	std::string service = QT_TO_UTF8(ui->service->currentText());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

	reset_service_ui_fields(service);

	ui->streamKeyWidget->setVisible(true);
	ui->streamKeyLabel->setVisible(true);
	ui->key->setText("");

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);

	/* Restore key link when disconnecting account */
	UpdateKeyLink();
}

void AutoConfigStreamPage::on_useStreamKey_clicked()
{
	ui->stackedWidget->setCurrentIndex((int)Section::StreamKey);
	UpdateCompleted();
}

void AutoConfigStreamPage::on_preferHardware_clicked()
{
	auto *main = OBSBasic::Get();
	bool multitrackVideoEnabled = config_has_user_value(main->Config(), "Stream1", "EnableMultitrackVideo")
					      ? config_get_bool(main->Config(), "Stream1", "EnableMultitrackVideo")
					      : true;

	ui->useMultitrackVideo->setEnabled(ui->preferHardware->isChecked());
	ui->multitrackVideoInfo->setEnabled(ui->preferHardware->isChecked());
	ui->useMultitrackVideo->setChecked(ui->preferHardware->isChecked() && multitrackVideoEnabled);
}

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

static inline bool is_external_oauth(const std::string &service)
{
	return Auth::External(service);
}

void AutoConfigStreamPage::reset_service_ui_fields(std::string &service)
{
#ifdef YOUTUBE_ENABLED
	// when account is already connected:
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());
	if (a && service == a->service() && IsYouTubeService(a->service())) {
		ui->connectedAccountLabel->setVisible(true);
		ui->connectedAccountText->setVisible(true);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);
		return;
	}
#endif

	bool external_oauth = is_external_oauth(service);
	if (external_oauth) {
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(true);
		ui->useStreamKeyAdv->setVisible(true);

		ui->stackedWidget->setCurrentIndex((int)Section::StreamKey);

	} else if (cef) {
		QString key = ui->key->text();
		bool can_auth = is_auth_service(service);
		int page = can_auth && key.isEmpty() ? (int)Section::Connect : (int)Section::StreamKey;

		ui->stackedWidget->setCurrentIndex(page);
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->connectAccount2->setVisible(can_auth);
		ui->useStreamKeyAdv->setVisible(false);
	} else {
		ui->connectAccount2->setVisible(false);
		ui->useStreamKeyAdv->setVisible(false);
	}

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);
	ui->disconnectAccount->setVisible(false);
}

void AutoConfigStreamPage::ServiceChanged()
{
	bool showMore = ui->service->currentData().toInt() == (int)ListOpt::ShowAll;
	if (showMore)
		return;

	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool regionBased = service == "Twitch";
	bool testBandwidth = ui->doBandwidthTest->isChecked();
	bool custom = IsCustomService();

	bool ertmp_multitrack_video_available = service == "Twitch";

	bool custom_disclaimer = false;
	auto multitrack_video_name = QTStr("Basic.Settings.Stream.MultitrackVideoLabel");
	if (!custom) {
		OBSDataAutoRelease service_settings = obs_data_create();
		obs_data_set_string(service_settings, "service", service.c_str());
		OBSServiceAutoRelease obs_service =
			obs_service_create("rtmp_common", "temp service", service_settings, nullptr);

		if (obs_data_has_user_value(service_settings, "multitrack_video_name")) {
			multitrack_video_name = obs_data_get_string(service_settings, "multitrack_video_name");
		}

		if (obs_data_has_user_value(service_settings, "multitrack_video_disclaimer")) {
			ui->multitrackVideoInfo->setText(
				obs_data_get_string(service_settings, "multitrack_video_disclaimer"));
			custom_disclaimer = true;
		}
	}

	if (!custom_disclaimer) {
		ui->multitrackVideoInfo->setText(
			QTStr("MultitrackVideo.Info").arg(multitrack_video_name, service.c_str()));
	}

	ui->multitrackVideoInfo->setVisible(ertmp_multitrack_video_available);
	ui->useMultitrackVideo->setVisible(ertmp_multitrack_video_available);
	ui->useMultitrackVideo->setText(
		QTStr("Basic.AutoConfig.StreamPage.UseMultitrackVideo").arg(multitrack_video_name));
	ui->multitrackVideoInfo->setEnabled(wiz->hardwareEncodingAvailable);
	ui->useMultitrackVideo->setEnabled(wiz->hardwareEncodingAvailable);

	reset_service_ui_fields(service);

	/* Test three closest servers if "Auto" is available for Twitch */
	if ((service == "Twitch" && wiz->twitchAuto) || (service == "Amazon IVS" && wiz->amazonIVSAuto))
		regionBased = false;

	ui->streamkeyPageLayout->removeWidget(ui->serverLabel);
	ui->streamkeyPageLayout->removeWidget(ui->serverStackedWidget);

	if (custom) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel, ui->serverStackedWidget);

		ui->region->setVisible(false);
		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
	} else {
		if (!testBandwidth)
			ui->streamkeyPageLayout->insertRow(2, ui->serverLabel, ui->serverStackedWidget);

		ui->region->setVisible(regionBased && testBandwidth);
		ui->serverStackedWidget->setCurrentIndex(0);
		ui->serverStackedWidget->setHidden(testBandwidth);
		ui->serverLabel->setHidden(testBandwidth);
	}

	wiz->testRegions = regionBased && testBandwidth;

	ui->bitrateLabel->setHidden(testBandwidth);
	ui->bitrate->setHidden(testBandwidth);

	OBSBasic *main = OBSBasic::Get();

	if (main->auth) {
		auto system_auth_service = main->auth->service();
		bool service_check = service.find(system_auth_service) != std::string::npos;
#ifdef YOUTUBE_ENABLED
		service_check = service_check ? service_check
					      : IsYouTubeService(system_auth_service) && IsYouTubeService(service);
#endif
		if (service_check) {
			auth.reset();
			auth = main->auth;
			OnAuthConnected();
		}
	}

	UpdateCompleted();
}

void AutoConfigStreamPage::UpdateMoreInfoLink()
{
	if (IsCustomService()) {
		ui->moreInfoButton->hide();
		return;
	}

	QString serviceName = ui->service->currentText();
	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	const char *more_info_link = obs_data_get_string(settings, "more_info_link");

	if (!more_info_link || (*more_info_link == '\0')) {
		ui->moreInfoButton->hide();
	} else {
		ui->moreInfoButton->setTargetUrl(QUrl(more_info_link));
		ui->moreInfoButton->show();
	}
	obs_properties_destroy(props);
}

void AutoConfigStreamPage::UpdateKeyLink()
{
	QString serviceName = ui->service->currentText();
	QString customServer = ui->customServer->text().trimmed();
	QString streamKeyLink;

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	streamKeyLink = obs_data_get_string(settings, "stream_key_link");

	if (customServer.contains("fbcdn.net") && IsCustomService()) {
		streamKeyLink = "https://www.facebook.com/live/producer?ref=OBS";
	}

	if (serviceName == "Dacast") {
		ui->streamKeyLabel->setText(QTStr("Basic.AutoConfig.StreamPage.EncoderKey"));
		ui->streamKeyLabel->setToolTip("");
	} else if (!IsCustomService()) {
		ui->streamKeyLabel->setText(QTStr("Basic.AutoConfig.StreamPage.StreamKey"));
		ui->streamKeyLabel->setToolTip("");
	} else {
		/* add tooltips for stream key */
		QString file = !App()->IsThemeDark() ? ":/res/images/help.svg" : ":/res/images/help_light.svg";
		QString lStr = "<html>%1 <img src='%2' style=' \
				vertical-align: bottom;  \
				' /></html>";

		ui->streamKeyLabel->setText(lStr.arg(QTStr("Basic.AutoConfig.StreamPage.StreamKey"), file));
		ui->streamKeyLabel->setToolTip(QTStr("Basic.AutoConfig.StreamPage.StreamKey.ToolTip"));
	}

	if (QString(streamKeyLink).isNull() || QString(streamKeyLink).isEmpty()) {
		ui->streamKeyButton->hide();
	} else {
		ui->streamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->streamKeyButton->show();
	}
	obs_properties_destroy(props);
}

void AutoConfigStreamPage::LoadServices(bool showAll)
{
	obs_properties_t *props = obs_get_service_properties("rtmp_common");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_bool(settings, "show_all", showAll);

	obs_property_t *prop = obs_properties_get(props, "show_all");
	obs_property_modified(prop, settings);

	ui->service->blockSignals(true);
	ui->service->clear();

	QStringList names;

	obs_property_t *services = obs_properties_get(props, "service");
	size_t services_count = obs_property_list_item_count(services);
	for (size_t i = 0; i < services_count; i++) {
		const char *name = obs_property_list_item_string(services, i);
		names.push_back(name);
	}

	if (showAll)
		names.sort(Qt::CaseInsensitive);

	for (QString &name : names)
		ui->service->addItem(name);

	if (!showAll) {
		ui->service->addItem(QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
				     QVariant((int)ListOpt::ShowAll));
	}

	ui->service->insertItem(0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"), QVariant((int)ListOpt::Custom));

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

void AutoConfigStreamPage::UpdateServerList()
{
	QString serviceName = ui->service->currentText();
	bool showMore = ui->service->currentData().toInt() == (int)ListOpt::ShowAll;

	if (showMore) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	} else {
		lastService = serviceName;
	}

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	obs_property_t *servers = obs_properties_get(props, "server");

	ui->server->clear();

	size_t servers_count = obs_property_list_item_count(servers);
	for (size_t i = 0; i < servers_count; i++) {
		const char *name = obs_property_list_item_name(servers, i);
		const char *server = obs_property_list_item_string(servers, i);
		ui->server->addItem(name, server);
	}

	obs_properties_destroy(props);
}

void AutoConfigStreamPage::UpdateCompleted()
{
	const bool custom = IsCustomService();
	if (ui->stackedWidget->currentIndex() == (int)Section::Connect ||
	    (ui->key->text().isEmpty() && !auth && !custom)) {
		ready = false;
	} else {
		if (custom) {
			ready = !ui->customServer->text().isEmpty();
		} else {
			ready = !wiz->testRegions || ui->regionUS->isChecked() || ui->regionEU->isChecked() ||
				ui->regionAsia->isChecked() || ui->regionOther->isChecked();
		}
	}
	emit completeChanged();
}

/* ------------------------------------------------------------------------- */

AutoConfig::AutoConfig(QWidget *parent) : QWizard(parent)
{
	EnableThreadedMessageBoxes(true);

	calldata_t cd = {0};
	calldata_set_int(&cd, "seconds", 5);

	proc_handler_t *ph = obs_get_proc_handler();
	proc_handler_call(ph, "twitch_ingests_refresh", &cd);
	proc_handler_call(ph, "amazon_ivs_ingests_refresh", &cd);
	calldata_free(&cd);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(parent);
	main->EnableOutputs(false);

	installEventFilter(CreateShortcutFilter());

	std::string serviceType;
	GetServiceInfo(serviceType, serviceName, server, key);
#if defined(_WIN32) || defined(__APPLE__)
	setWizardStyle(QWizard::ModernStyle);
#endif
	streamPage = new AutoConfigStreamPage();

	setPage(StartPage, new AutoConfigStartPage());
	setPage(VideoPage, new AutoConfigVideoPage());
	setPage(StreamPage, streamPage);
	setPage(TestPage, new AutoConfigTestPage());
	setWindowTitle(QTStr("Basic.AutoConfig"));
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	baseResolutionCX = ovi.base_width;
	baseResolutionCY = ovi.base_height;

	/* ----------------------------------------- */
	/* check to see if Twitch's "auto" available */

	OBSDataAutoRelease twitchSettings = obs_data_create();

	obs_data_set_string(twitchSettings, "service", "Twitch");

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_properties_apply_settings(props, twitchSettings);

	obs_property_t *p = obs_properties_get(props, "server");
	const char *first = obs_property_list_item_string(p, 0);
	twitchAuto = strcmp(first, "auto") == 0;

	obs_properties_destroy(props);

	/* ----------------------------------------- */
	/* check to see if Amazon IVS "auto" entries are available */

	OBSDataAutoRelease amazonIVSSettings = obs_data_create();

	obs_data_set_string(amazonIVSSettings, "service", "Amazon IVS");

	props = obs_get_service_properties("rtmp_common");
	obs_properties_apply_settings(props, amazonIVSSettings);

	p = obs_properties_get(props, "server");
	first = obs_property_list_item_string(p, 0);
	amazonIVSAuto = strncmp(first, "auto", 4) == 0;

	obs_properties_destroy(props);

	/* ----------------------------------------- */
	/* load service/servers                      */

	customServer = serviceType == "rtmp_custom";

	QComboBox *serviceList = streamPage->ui->service;

	if (!serviceName.empty()) {
		serviceList->blockSignals(true);

		int count = serviceList->count();
		bool found = false;

		for (int i = 0; i < count; i++) {
			QString name = serviceList->itemText(i);

			if (name == serviceName.c_str()) {
				serviceList->setCurrentIndex(i);
				found = true;
				break;
			}
		}

		if (!found) {
			serviceList->insertItem(0, serviceName.c_str());
			serviceList->setCurrentIndex(0);
		}

		serviceList->blockSignals(false);
	}

	streamPage->UpdateServerList();
	streamPage->UpdateKeyLink();
	streamPage->UpdateMoreInfoLink();
	streamPage->lastService.clear();

	if (!customServer) {
		QComboBox *serverList = streamPage->ui->server;
		int idx = serverList->findData(QString(server.c_str()));
		if (idx == -1)
			idx = 0;

		serverList->setCurrentIndex(idx);
	} else {
		streamPage->ui->customServer->setText(server.c_str());
		int idx = streamPage->ui->service->findData(QVariant((int)ListOpt::Custom));
		streamPage->ui->service->setCurrentIndex(idx);
	}

	if (!key.empty())
		streamPage->ui->key->setText(key.c_str());

	TestHardwareEncoding();

	int bitrate = config_get_int(main->Config(), "SimpleOutput", "VBitrate");
	bool multitrackVideoEnabled = config_has_user_value(main->Config(), "Stream1", "EnableMultitrackVideo")
					      ? config_get_bool(main->Config(), "Stream1", "EnableMultitrackVideo")
					      : true;
	streamPage->ui->bitrate->setValue(bitrate);
	streamPage->ui->useMultitrackVideo->setChecked(hardwareEncodingAvailable && multitrackVideoEnabled);
	streamPage->ServiceChanged();

	TestSoftwareEncoding();
	TestHardwareEncoding();
	if (!hardwareEncodingAvailable) {
		delete streamPage->ui->preferHardware;
		streamPage->ui->preferHardware = nullptr;
	} else {
		/* Newer generations of NVENC have a high enough quality to
		 * bitrate ratio that if NVENC is available, it makes sense to
		 * just always prefer hardware encoding by default */
		bool preferHardware = nvencAvailable || appleAvailable || os_get_physical_cores() <= 4;
		streamPage->ui->preferHardware->setChecked(preferHardware);
	}

	setOptions(QWizard::WizardOptions());
	setButtonText(QWizard::FinishButton, QTStr("Basic.AutoConfig.ApplySettings"));
	setButtonText(QWizard::BackButton, QTStr("Back"));
	setButtonText(QWizard::NextButton, QTStr("Next"));
	setButtonText(QWizard::CancelButton, QTStr("Cancel"));
}

AutoConfig::~AutoConfig()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	main->EnableOutputs(true);
	EnableThreadedMessageBoxes(false);
}

void AutoConfig::TestSoftwareEncoding()
{
	size_t idx = 0;
	const char *id;
	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "obs_x264") == 0)
			x264Available = true;
	}
}

void AutoConfig::TestHardwareEncoding()
{
	size_t idx = 0;
	const char *id;
	while (obs_enum_encoder_types(idx++, &id)) {
		if (strcmp(id, "ffmpeg_nvenc") == 0)
			hardwareEncodingAvailable = nvencAvailable = true;
		else if (strcmp(id, "obs_qsv11") == 0)
			hardwareEncodingAvailable = qsvAvailable = true;
		else if (strcmp(id, "h264_texture_amf") == 0)
			hardwareEncodingAvailable = vceAvailable = true;
#ifdef __APPLE__
		else if (strcmp(id, "com.apple.videotoolbox.videoencoder.ave.avc") == 0
#ifndef __aarch64__
			 && os_get_emulation_status() == true
#endif
		)
			if (__builtin_available(macOS 13.0, *))
				hardwareEncodingAvailable = appleAvailable = true;
#endif
	}
}

bool AutoConfig::CanTestServer(const char *server)
{
	if (!testRegions || (regionUS && regionEU && regionAsia && regionOther))
		return true;

	if (service == Service::Twitch) {
		if (astrcmp_n(server, "US West:", 8) == 0 || astrcmp_n(server, "US East:", 8) == 0 ||
		    astrcmp_n(server, "US Central:", 11) == 0) {
			return regionUS;
		} else if (astrcmp_n(server, "EU:", 3) == 0) {
			return regionEU;
		} else if (astrcmp_n(server, "Asia:", 5) == 0) {
			return regionAsia;
		} else if (regionOther) {
			return true;
		}
	} else {
		return true;
	}

	return false;
}

void AutoConfig::done(int result)
{
	QWizard::done(result);

	if (result == QDialog::Accepted) {
		if (type == Type::Streaming)
			SaveStreamSettings();
		SaveSettings();

#ifdef YOUTUBE_ENABLED
		if (YouTubeAppDock::IsYTServiceSelected()) {
			OBSBasic *main = OBSBasic::Get();
			main->NewYouTubeAppDock();
		}
#endif
	}
}

inline const char *AutoConfig::GetEncoderId(Encoder enc)
{
	switch (enc) {
	case Encoder::NVENC:
		return SIMPLE_ENCODER_NVENC;
	case Encoder::QSV:
		return SIMPLE_ENCODER_QSV;
	case Encoder::AMD:
		return SIMPLE_ENCODER_AMD;
	case Encoder::Apple:
		return SIMPLE_ENCODER_APPLE_H264;
	case Encoder::x264:
		return SIMPLE_ENCODER_X264;
	default:
		return SIMPLE_ENCODER_OPENH264;
	}
};

void AutoConfig::SaveStreamSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	/* ---------------------------------- */
	/* save service                       */

	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSDataAutoRelease hotkeyData = obs_hotkeys_save_service(oldService);

	OBSDataAutoRelease settings = obs_data_create();

	if (!customServer)
		obs_data_set_string(settings, "service", serviceName.c_str());
	obs_data_set_string(settings, "server", server.c_str());
#ifdef YOUTUBE_ENABLED
	if (!streamPage->auth || !IsYouTubeService(serviceName))
		obs_data_set_string(settings, "key", key.c_str());
#else
	obs_data_set_string(settings, "key", key.c_str());
#endif

	OBSServiceAutoRelease newService = obs_service_create(service_id, "default_service", settings, hotkeyData);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = streamPage->auth;
	if (!!main->auth) {
		main->auth->LoadUI();
		main->SetBroadcastFlowEnabled(main->auth->broadcastFlow());
	} else {
		main->SetBroadcastFlowEnabled(false);
	}

	/* ---------------------------------- */
	/* save stream settings               */

	config_set_int(main->Config(), "SimpleOutput", "VBitrate", idealBitrate);
	config_set_string(main->Config(), "SimpleOutput", "StreamEncoder", GetEncoderId(streamingEncoder));
	config_remove_value(main->Config(), "SimpleOutput", "UseAdvanced");

	config_set_bool(main->Config(), "Stream1", "EnableMultitrackVideo", multitrackVideo.testSuccessful);

	if (multitrackVideo.targetBitrate.has_value())
		config_set_int(main->Config(), "Stream1", "MultitrackVideoTargetBitrate",
			       *multitrackVideo.targetBitrate);
	else
		config_remove_value(main->Config(), "Stream1", "MultitrackVideoTargetBitrate");

	if (multitrackVideo.bitrate.has_value() && multitrackVideo.targetBitrate.has_value() &&
	    (static_cast<double>(*multitrackVideo.bitrate) / *multitrackVideo.targetBitrate) >= 0.90) {
		config_set_bool(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", true);
	} else if (multitrackVideo.bitrate.has_value()) {
		config_set_bool(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", false);
		config_set_int(main->Config(), "Stream1", "MultitrackVideoMaximumAggregateBitrate",
			       *multitrackVideo.bitrate);
	}
}

void AutoConfig::SaveSettings()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	if (recordingEncoder != Encoder::Stream)
		config_set_string(main->Config(), "SimpleOutput", "RecEncoder", GetEncoderId(recordingEncoder));

	const char *quality = recordingQuality == Quality::High ? "Small" : "Stream";

	config_set_string(main->Config(), "Output", "Mode", "Simple");
	config_set_string(main->Config(), "SimpleOutput", "RecQuality", quality);
	config_set_int(main->Config(), "Video", "BaseCX", baseResolutionCX);
	config_set_int(main->Config(), "Video", "BaseCY", baseResolutionCY);
	config_set_int(main->Config(), "Video", "OutputCX", idealResolutionCX);
	config_set_int(main->Config(), "Video", "OutputCY", idealResolutionCY);

	if (fpsType != FPSType::UseCurrent) {
		config_set_uint(main->Config(), "Video", "FPSType", 0);
		config_set_string(main->Config(), "Video", "FPSCommon", std::to_string(idealFPSNum).c_str());
	}

	main->ResetVideo();
	main->ResetOutputs();
	config_save_safe(main->Config(), "tmp", nullptr);
}
