#include <QMessageBox>
#include <QUrl>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "url-push-button.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

#include "auth-oauth.hpp"

#include "ui-config.h"

#if YOUTUBE_ENABLED
#include "youtube-api-wrappers.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

enum class ListOpt : int {
	ShowAll = 1,
	Custom,
};

enum class Section : int {
	Connect,
	StreamKey,
};

inline bool OBSBasicSettings::IsCustomService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Custom;
}

void OBSBasicSettings::InitStreamPage()
{
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);

	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);

	int vertSpacing = ui->topStreamLayout->verticalSpacing();

	QMargins m = ui->topStreamLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topStreamLayout->setContentsMargins(m);

	m = ui->loginPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->loginPageLayout->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	LoadServices(false);

	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.None"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.BTTV"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.FFZ"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.Both"));

	connect(ui->ignoreRecommended, SIGNAL(clicked(bool)), this,
		SLOT(DisplayEnforceWarning(bool)));
	connect(ui->ignoreRecommended, SIGNAL(toggled(bool)), this,
		SLOT(UpdateResFPSLimits()));
}

void OBSBasicSettings::LoadStream1Settings()
{
	bool ignoreRecommended =
		config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");

	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);

	const char *service = obs_data_get_string(settings, "service");
	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");
	protocol = QT_UTF8(obs_service_get_protocol(service_obj));

	if (strcmp(type, "rtmp_custom") == 0) {
		ui->service->setCurrentIndex(0);
		ui->customServer->setText(server);
		lastServiceIdx = 0;
		lastCustomServer = ui->customServer->text();

		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);

		/* add tooltips for stream key, user, password fields */
		QString file = !App()->IsThemeDark()
				       ? ":/res/images/help.svg"
				       : ":/res/images/help_light.svg";
		QString lStr = "<html>%1 <img src='%2' style=' \
				vertical-align: bottom;  \
				' /></html>";

		ui->streamKeyLabel->setText(
			lStr.arg(ui->streamKeyLabel->text(), file));
		ui->streamKeyLabel->setToolTip(
			QTStr("Basic.AutoConfig.StreamPage.StreamKey.ToolTip"));

		ui->authUsernameLabel->setText(
			lStr.arg(ui->authUsernameLabel->text(), file));
		ui->authUsernameLabel->setToolTip(
			QTStr("Basic.Settings.Stream.Custom.Username.ToolTip"));

		ui->authPwLabel->setText(
			lStr.arg(ui->authPwLabel->text(), file));
		ui->authPwLabel->setToolTip(
			QTStr("Basic.Settings.Stream.Custom.Password.ToolTip"));
	} else {
		int idx = ui->service->findText(service);
		if (idx == -1) {
			if (service && *service)
				ui->service->insertItem(1, service);
			idx = 1;
		}
		ui->service->setCurrentIndex(idx);
		lastServiceIdx = idx;

		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);

		idx = config_get_int(main->Config(), "Twitch", "AddonChoice");
		ui->twitchAddonDropdown->setCurrentIndex(idx);
	}

	UpdateServerList();

	if (strcmp(type, "rtmp_common") == 0) {
		int idx = ui->server->findData(server);
		if (idx == -1) {
			if (server && *server)
				ui->server->insertItem(0, server, server);
			idx = 0;
		}
		ui->server->setCurrentIndex(idx);
	}

	ui->key->setText(key);

	lastService.clear();
	on_service_currentIndexChanged(0);

	UpdateKeyLink();
	UpdateMoreInfoLink();
	UpdateVodTrackSetting();
	UpdateServiceRecommendations();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	ui->ignoreRecommended->setChecked(ignoreRecommended);

	loading = false;

	QMetaObject::invokeMethod(this, "UpdateResFPSLimits",
				  Qt::QueuedConnection);
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

	obs_service_t *oldService = main->GetService();
	OBSDataAutoRelease hotkeyData = obs_hotkeys_save_service(oldService);

	OBSDataAutoRelease settings = obs_data_create();

	if (!customServer) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->customServer->text().trimmed()));
		obs_data_set_bool(settings, "use_auth",
				  ui->useAuth->isChecked());
		if (ui->useAuth->isChecked()) {
			obs_data_set_string(
				settings, "username",
				QT_TO_UTF8(ui->authUsername->text()));
			obs_data_set_string(settings, "password",
					    QT_TO_UTF8(ui->authPw->text()));
		}
	}

	if (!!auth && strcmp(auth->service(), "Twitch") == 0) {
		bool choiceExists = config_has_user_value(
			main->Config(), "Twitch", "AddonChoice");
		int currentChoice =
			config_get_int(main->Config(), "Twitch", "AddonChoice");
		int newChoice = ui->twitchAddonDropdown->currentIndex();

		config_set_int(main->Config(), "Twitch", "AddonChoice",
			       newChoice);

		if (choiceExists && currentChoice != newChoice)
			forceAuthReload = true;

		obs_data_set_bool(settings, "bwtest",
				  ui->bandwidthTestEnable->isChecked());
	} else {
		obs_data_set_bool(settings, "bwtest", false);
	}

	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSServiceAutoRelease newService = obs_service_create(
		service_id, "default_service", settings, hotkeyData);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = auth;
	if (!!main->auth) {
		main->auth->LoadUI();
		main->SetBroadcastFlowEnabled(main->auth->broadcastFlow());
	} else {
		main->SetBroadcastFlowEnabled(false);
	}

	SaveCheckBox(ui->ignoreRecommended, "Stream1", "IgnoreRecommended");
}

void OBSBasicSettings::UpdateMoreInfoLink()
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

	const char *more_info_link =
		obs_data_get_string(settings, "more_info_link");

	if (!more_info_link || (*more_info_link == '\0')) {
		ui->moreInfoButton->hide();
	} else {
		ui->moreInfoButton->setTargetUrl(QUrl(more_info_link));
		ui->moreInfoButton->show();
	}
	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateKeyLink()
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
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	}

	if (serviceName == "Dacast") {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.EncoderKey"));
	} else if (!IsCustomService()) {
		ui->streamKeyLabel->setText(
			QTStr("Basic.AutoConfig.StreamPage.StreamKey"));
	}

	if (QString(streamKeyLink).isNull() ||
	    QString(streamKeyLink).isEmpty()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
	obs_properties_destroy(props);
}

void OBSBasicSettings::LoadServices(bool showAll)
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
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant((int)ListOpt::ShowAll));
	}

	ui->service->insertItem(
		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
		QVariant((int)ListOpt::Custom));

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

static inline bool is_external_oauth(const std::string &service)
{
	return Auth::External(service);
}

static void reset_service_ui_fields(Ui::OBSBasicSettings *ui,
				    std::string &service, bool loading)
{
	bool external_oauth = is_external_oauth(service);
	if (external_oauth) {
		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(true);
		ui->useStreamKeyAdv->setVisible(true);
		ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
	} else if (cef) {
		QString key = ui->key->text();
		bool can_auth = is_auth_service(service);
		int page = can_auth && (!loading || key.isEmpty())
				   ? (int)Section::Connect
				   : (int)Section::StreamKey;

		ui->streamStackWidget->setCurrentIndex(page);
		ui->streamKeyWidget->setVisible(true);
		ui->streamKeyLabel->setVisible(true);
		ui->connectAccount2->setVisible(can_auth);
		ui->useStreamKeyAdv->setVisible(false);
	} else {
		ui->connectAccount2->setVisible(false);
		ui->useStreamKeyAdv->setVisible(false);
		ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
	}

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);
	ui->disconnectAccount->setVisible(false);
}

#if YOUTUBE_ENABLED
static void get_yt_ch_title(Ui::OBSBasicSettings *ui)
{
	const char *name = config_get_string(OBSBasic::Get()->Config(),
					     "YouTube", "ChannelName");
	if (name) {
		ui->connectedAccountText->setText(name);
	} else {
		// if we still not changed the service page
		if (IsYouTubeService(QT_TO_UTF8(ui->service->currentText()))) {
			ui->connectedAccountText->setText(
				QTStr("Auth.LoadingChannel.Error"));
		}
	}
}
#endif

void OBSBasicSettings::UseStreamKeyAdvClicked()
{
	ui->streamKeyWidget->setVisible(true);
}

void OBSBasicSettings::on_service_currentIndexChanged(int idx)
{
	if (ui->service->currentData().toInt() == (int)ListOpt::ShowAll) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	}

	ServiceChanged();

	UpdateMoreInfoLink();
	UpdateServerList();
	UpdateKeyLink();
	UpdateServiceRecommendations();

	UpdateVodTrackSetting();

	protocol = FindProtocol();
	UpdateAdvNetworkGroup();

	if (ServiceSupportsCodecCheck() && UpdateResFPSLimits()) {
		lastServiceIdx = idx;
		if (idx == 0)
			lastCustomServer = ui->customServer->text();
	}
}

void OBSBasicSettings::on_customServer_textChanged(const QString &)
{
	UpdateKeyLink();

	protocol = FindProtocol();
	UpdateAdvNetworkGroup();

	if (ServiceSupportsCodecCheck())
		lastCustomServer = ui->customServer->text();
}

void OBSBasicSettings::ServiceChanged()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool custom = IsCustomService();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

	if (lastService != service.c_str()) {
		reset_service_ui_fields(ui.get(), service, loading);
	}

	ui->useAuth->setVisible(custom);
	ui->authUsernameLabel->setVisible(custom);
	ui->authUsername->setVisible(custom);
	ui->authPwLabel->setVisible(custom);
	ui->authPwWidget->setVisible(custom);

	if (custom) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);

		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
		on_useAuth_toggled();
	} else {
		ui->serverStackedWidget->setCurrentIndex(0);
	}

	auth.reset();

	if (!main->auth) {
		return;
	}

	auto system_auth_service = main->auth->service();
	bool service_check = service.find(system_auth_service) !=
			     std::string::npos;
#if YOUTUBE_ENABLED
	service_check = service_check ? service_check
				      : IsYouTubeService(system_auth_service) &&
						IsYouTubeService(service);
#endif
	if (service_check) {
		auth = main->auth;
		OnAuthConnected();
	}
}

QString OBSBasicSettings::FindProtocol()
{
	if (IsCustomService()) {
		if (ui->customServer->text().isEmpty())
			return QString("RTMP");

		QString server = ui->customServer->text();

		if (obs_is_output_protocol_registered("RTMPS") &&
		    server.startsWith("rtmps://"))
			return QString("RTMPS");

		if (server.startsWith("ftl://"))
			return QString("FTL");

		if (server.startsWith("srt://"))
			return QString("SRT");

		if (server.startsWith("rist://"))
			return QString("RIST");

	} else {
		obs_properties_t *props =
			obs_get_service_properties("rtmp_common");
		obs_property_t *services = obs_properties_get(props, "service");

		OBSDataAutoRelease settings = obs_data_create();

		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_property_modified(services, settings);

		obs_properties_destroy(props);

		return QT_UTF8(obs_data_get_string(settings, "protocol"));
	}

	return QString("RTMP");
}

void OBSBasicSettings::UpdateServerList()
{
	QString serviceName = ui->service->currentText();

	lastService = serviceName;

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

void OBSBasicSettings::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_authPwShow_clicked()
{
	if (ui->authPw->echoMode() == QLineEdit::Password) {
		ui->authPw->setEchoMode(QLineEdit::Normal);
		ui->authPwShow->setText(QTStr("Hide"));
	} else {
		ui->authPw->setEchoMode(QLineEdit::Password);
		ui->authPwShow->setText(QTStr("Show"));
	}
}

OBSService OBSBasicSettings::SpawnTempService()
{
	bool custom = IsCustomService();
	const char *service_id = custom ? "rtmp_custom" : "rtmp_common";

	OBSDataAutoRelease settings = obs_data_create();

	if (!custom) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->customServer->text().trimmed()));
	}
	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSServiceAutoRelease newService = obs_service_create(
		service_id, "temp_service", settings, nullptr);
	return newService.Get();
}

void OBSBasicSettings::OnOAuthStreamKeyConnected()
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

		if (strcmp(a->service(), "Twitch") == 0) {
			ui->bandwidthTestEnable->setVisible(true);
			ui->twitchAddonLabel->setVisible(true);
			ui->twitchAddonDropdown->setVisible(true);
		} else {
			ui->bandwidthTestEnable->setChecked(false);
		}
#if YOUTUBE_ENABLED
		if (IsYouTubeService(a->service())) {
			ui->key->clear();

			ui->connectedAccountLabel->setVisible(true);
			ui->connectedAccountText->setVisible(true);

			ui->connectedAccountText->setText(
				QTStr("Auth.LoadingChannel.Title"));

			get_yt_ch_title(ui.get());
		}
#endif
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::OnAuthConnected()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	Auth::Type type = Auth::AuthType(service);

	if (type == Auth::Type::OAuth_StreamKey ||
	    type == Auth::Type::OAuth_LinkedAccount) {
		OnOAuthStreamKeyConnected();
	}

	if (!loading) {
		stream1Changed = true;
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::on_connectAccount_clicked()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth) {
		OnAuthConnected();

		ui->useStreamKeyAdv->setVisible(false);
	}
}

#define DISCONNECT_COMFIRM_TITLE \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Title"
#define DISCONNECT_COMFIRM_TEXT \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Text"

void OBSBasicSettings::on_disconnectAccount_clicked()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr(DISCONNECT_COMFIRM_TITLE),
					 QTStr(DISCONNECT_COMFIRM_TEXT));

	if (button == QMessageBox::No) {
		return;
	}

	main->auth.reset();
	auth.reset();
	main->SetBroadcastFlowEnabled(false);

	std::string service = QT_TO_UTF8(ui->service->currentText());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

	ui->bandwidthTestEnable->setChecked(false);

	reset_service_ui_fields(ui.get(), service, loading);

	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->key->setText("");

	ui->connectedAccountLabel->setVisible(false);
	ui->connectedAccountText->setVisible(false);
}

void OBSBasicSettings::on_useStreamKey_clicked()
{
	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::on_useAuth_toggled()
{
	if (!IsCustomService())
		return;

	bool use_auth = ui->useAuth->isChecked();

	ui->authUsernameLabel->setVisible(use_auth);
	ui->authUsername->setVisible(use_auth);
	ui->authPwLabel->setVisible(use_auth);
	ui->authPwWidget->setVisible(use_auth);
}

void OBSBasicSettings::UpdateVodTrackSetting()
{
	bool enableForCustomServer = config_get_bool(
		GetGlobalConfig(), "General", "EnableCustomServerVodTrack");
	bool enableVodTrack = ui->service->currentText() == "Twitch";
	bool wasEnabled = !!vodTrackCheckbox;

	if (enableForCustomServer && IsCustomService())
		enableVodTrack = true;

	if (enableVodTrack == wasEnabled)
		return;

	if (!enableVodTrack) {
		delete vodTrackCheckbox;
		delete vodTrackContainer;
		delete simpleVodTrack;
		return;
	}

	/* -------------------------------------- */
	/* simple output mode vod track widgets   */

	bool simpleAdv = ui->simpleOutAdvanced->isChecked();
	bool vodTrackEnabled = config_get_bool(main->Config(), "SimpleOutput",
					       "VodTrackEnabled");

	simpleVodTrack = new QCheckBox(this);
	simpleVodTrack->setText(
		QTStr("Basic.Settings.Output.Simple.TwitchVodTrack"));
	simpleVodTrack->setVisible(simpleAdv);
	simpleVodTrack->setChecked(vodTrackEnabled);

	int pos;
	ui->simpleStreamingLayout->getWidgetPosition(ui->simpleOutAdvanced,
						     &pos, nullptr);
	ui->simpleStreamingLayout->insertRow(pos + 1, nullptr, simpleVodTrack);

	HookWidget(simpleVodTrack, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));
	connect(ui->simpleOutAdvanced, SIGNAL(toggled(bool)),
		simpleVodTrack.data(), SLOT(setVisible(bool)));

	/* -------------------------------------- */
	/* advanced output mode vod track widgets */

	vodTrackCheckbox = new QCheckBox(this);
	vodTrackCheckbox->setText(
		QTStr("Basic.Settings.Output.Adv.TwitchVodTrack"));
	vodTrackCheckbox->setLayoutDirection(Qt::RightToLeft);

	vodTrackContainer = new QWidget(this);
	QHBoxLayout *vodTrackLayout = new QHBoxLayout();
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i] = new QRadioButton(QString::number(i + 1));
		vodTrackLayout->addWidget(vodTrack[i]);

		HookWidget(vodTrack[i], SIGNAL(clicked(bool)),
			   SLOT(OutputsChanged()));
	}

	HookWidget(vodTrackCheckbox, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));

	vodTrackLayout->addStretch();
	vodTrackLayout->setContentsMargins(0, 0, 0, 0);

	vodTrackContainer->setLayout(vodTrackLayout);

	ui->advOutTopLayout->insertRow(2, vodTrackCheckbox, vodTrackContainer);

	vodTrackEnabled =
		config_get_bool(main->Config(), "AdvOut", "VodTrackEnabled");
	vodTrackCheckbox->setChecked(vodTrackEnabled);
	vodTrackContainer->setEnabled(vodTrackEnabled);

	connect(vodTrackCheckbox, SIGNAL(clicked(bool)), vodTrackContainer,
		SLOT(setEnabled(bool)));

	int trackIndex =
		config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i]->setChecked((i + 1) == trackIndex);
	}
}

OBSService OBSBasicSettings::GetStream1Service()
{
	return stream1Changed ? SpawnTempService()
			      : OBSService(main->GetService());
}

void OBSBasicSettings::UpdateServiceRecommendations()
{
	bool customServer = IsCustomService();
	ui->ignoreRecommended->setVisible(!customServer);
	ui->enforceSettingsLabel->setVisible(!customServer);

	OBSService service = GetStream1Service();

	int vbitrate, abitrate;
	BPtr<obs_service_resolution> res_list;
	size_t res_count;
	int fps;

	obs_service_get_max_bitrate(service, &vbitrate, &abitrate);
	obs_service_get_supported_resolutions(service, &res_list, &res_count);
	obs_service_get_max_fps(service, &fps);

	QString text;

#define ENFORCE_TEXT(x) QTStr("Basic.Settings.Stream.Recommended." x)
	if (vbitrate)
		text += ENFORCE_TEXT("MaxVideoBitrate")
				.arg(QString::number(vbitrate));
	if (abitrate) {
		if (!text.isEmpty())
			text += "<br>";
		text += ENFORCE_TEXT("MaxAudioBitrate")
				.arg(QString::number(abitrate));
	}
	if (res_count) {
		if (!text.isEmpty())
			text += "<br>";

		obs_service_resolution best_res = {};
		int best_res_pixels = 0;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution res = res_list[i];
			int res_pixels = res.cx + res.cy;
			if (res_pixels > best_res_pixels) {
				best_res = res;
				best_res_pixels = res_pixels;
			}
		}

		QString res_str =
			QString("%1x%2").arg(QString::number(best_res.cx),
					     QString::number(best_res.cy));
		text += ENFORCE_TEXT("MaxResolution").arg(res_str);
	}
	if (fps) {
		if (!text.isEmpty())
			text += "<br>";

		text += ENFORCE_TEXT("MaxFPS").arg(QString::number(fps));
	}
#undef ENFORCE_TEXT

#if YOUTUBE_ENABLED
	if (IsYouTubeService(QT_TO_UTF8(ui->service->currentText()))) {
		if (!text.isEmpty())
			text += "<br><br>";

		text += "<a href=\"https://www.youtube.com/t/terms\">"
			"YouTube Terms of Service</a><br>"
			"<a href=\"http://www.google.com/policies/privacy\">"
			"Google Privacy Policy</a><br>"
			"<a href=\"https://security.google.com/settings/security/permissions\">"
			"Google Third-Party Permissions</a>";
	}
#endif
	ui->enforceSettingsLabel->setText(text);
}

void OBSBasicSettings::DisplayEnforceWarning(bool checked)
{
	if (IsCustomService())
		return;

	if (!checked) {
		SimpleRecordingEncoderChanged();
		return;
	}

	QMessageBox::StandardButton button;

#define ENFORCE_WARNING(x) \
	QTStr("Basic.Settings.Stream.IgnoreRecommended.Warn." x)

	button = OBSMessageBox::question(this, ENFORCE_WARNING("Title"),
					 ENFORCE_WARNING("Text"));
#undef ENFORCE_WARNING

	if (button == QMessageBox::No) {
		QMetaObject::invokeMethod(ui->ignoreRecommended, "setChecked",
					  Qt::QueuedConnection,
					  Q_ARG(bool, false));
		return;
	}

	SimpleRecordingEncoderChanged();
}

bool OBSBasicSettings::ResFPSValid(obs_service_resolution *res_list,
				   size_t res_count, int max_fps)
{
	if (!res_count && !max_fps)
		return true;

	if (res_count) {
		QString res = ui->outputResolution->currentText();
		bool found_res = false;

		int cx, cy;
		if (sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy) != 2)
			return false;

		for (size_t i = 0; i < res_count; i++) {
			if (res_list[i].cx == cx && res_list[i].cy == cy) {
				found_res = true;
				break;
			}
		}

		if (!found_res)
			return false;
	}

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();
		if (fpsType != 0)
			return false;

		std::string fps_str = QT_TO_UTF8(ui->fpsCommon->currentText());
		float fps;
		sscanf(fps_str.c_str(), "%f", &fps);
		if (fps > (float)max_fps)
			return false;
	}

	return true;
}

extern void set_closest_res(int &cx, int &cy,
			    struct obs_service_resolution *res_list,
			    size_t count);

/* Checks for and updates the resolution and FPS limits of a service, if any.
 *
 * If the service has a resolution and/or FPS limit, this will enforce those
 * limitations in the UI itself, preventing the user from selecting a
 * resolution or FPS that's not supported.
 *
 * This is an unpleasant thing to have to do to users, but there is no other
 * way to ensure that a service's restricted resolution/framerate values are
 * properly enforced, otherwise users will just be confused when things aren't
 * working correctly. The user can turn it off if they're partner (or if they
 * want to risk getting in trouble with their service) by selecting the "Ignore
 * recommended settings" option in the stream section of settings.
 *
 * This only affects services that have a resolution and/or framerate limit, of
 * which as of this writing, and hopefully for the foreseeable future, there is
 * only one.
 */
bool OBSBasicSettings::UpdateResFPSLimits()
{
	if (loading)
		return false;

	int idx = ui->service->currentIndex();
	if (idx == -1)
		return false;

	bool ignoreRecommended = ui->ignoreRecommended->isChecked();
	BPtr<obs_service_resolution> res_list;
	size_t res_count = 0;
	int max_fps = 0;

	if (!IsCustomService() && !ignoreRecommended) {
		OBSService service = GetStream1Service();
		obs_service_get_supported_resolutions(service, &res_list,
						      &res_count);
		obs_service_get_max_fps(service, &max_fps);
	}

	/* ------------------------------------ */
	/* Check for enforced res/FPS           */

	QString res = ui->outputResolution->currentText();
	QString fps_str;
	int cx = 0, cy = 0;
	double max_fpsd = (double)max_fps;
	int closest_fps_index = -1;
	double fpsd;

	sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy);

	if (res_count)
		set_closest_res(cx, cy, res_list, res_count);

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();

		if (fpsType == 1) { //Integer
			fpsd = (double)ui->fpsInteger->value();
		} else if (fpsType == 2) { //Fractional
			fpsd = (double)ui->fpsNumerator->value() /
			       (double)ui->fpsDenominator->value();
		} else { //Common
			sscanf(QT_TO_UTF8(ui->fpsCommon->currentText()), "%lf",
			       &fpsd);
		}

		double closest_diff = 1000000000000.0;

		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				continue;
			}

			double diff = fabs(com_fpsd - fpsd);
			if (diff < closest_diff) {
				closest_diff = diff;
				closest_fps_index = i;
				fps_str = ui->fpsCommon->itemText(i);
			}
		}
	}

	QString res_str =
		QString("%1x%2").arg(QString::number(cx), QString::number(cy));

	/* ------------------------------------ */
	/* Display message box if res/FPS bad   */

	bool valid = ResFPSValid(res_list, res_count, max_fps);

	if (!valid) {
		/* if the user was already on facebook with an incompatible
		 * resolution, assume it's an upgrade */
		if (lastServiceIdx == -1 && lastIgnoreRecommended == -1) {
			ui->ignoreRecommended->setChecked(true);
			ui->ignoreRecommended->setProperty("changed", true);
			stream1Changed = true;
			EnableApplyButton(true);
			return UpdateResFPSLimits();
		}

		QMessageBox::StandardButton button;

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.EnforceResolutionFPS." x)

		QString str;
		if (res_count)
			str += WARNING_VAL("Resolution").arg(res_str);
		if (max_fps) {
			if (!str.isEmpty())
				str += "\n";
			str += WARNING_VAL("FPS").arg(fps_str);
		}

		button = OBSMessageBox::question(this, WARNING_VAL("Title"),
						 WARNING_VAL("Msg").arg(str));
#undef WARNING_VAL

		if (button == QMessageBox::No) {
			if (idx != lastServiceIdx)
				QMetaObject::invokeMethod(
					ui->service, "setCurrentIndex",
					Qt::QueuedConnection,
					Q_ARG(int, lastServiceIdx));
			else
				QMetaObject::invokeMethod(ui->ignoreRecommended,
							  "setChecked",
							  Qt::QueuedConnection,
							  Q_ARG(bool, true));
			return false;
		}
	}

	/* ------------------------------------ */
	/* Update widgets/values if switching   */
	/* to/from enforced resolution/FPS      */

	ui->outputResolution->blockSignals(true);
	if (res_count) {
		ui->outputResolution->clear();
		ui->outputResolution->setEditable(false);
		HookWidget(ui->outputResolution,
			   SIGNAL(currentIndexChanged(int)),
			   SLOT(VideoChangedResolution()));

		int new_res_index = -1;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution val = res_list[i];
			QString str =
				QString("%1x%2").arg(QString::number(val.cx),
						     QString::number(val.cy));
			ui->outputResolution->addItem(str);

			if (val.cx == cx && val.cy == cy)
				new_res_index = (int)i;
		}

		ui->outputResolution->setCurrentIndex(new_res_index);
		if (!valid) {
			ui->outputResolution->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		QString baseRes = ui->baseResolution->currentText();
		int baseCX, baseCY;
		sscanf(QT_TO_UTF8(baseRes), "%dx%d", &baseCX, &baseCY);

		if (!ui->outputResolution->isEditable()) {
			RecreateOutputResolutionWidget();
			ui->outputResolution->blockSignals(true);
			ResetDownscales((uint32_t)baseCX, (uint32_t)baseCY,
					true);
			ui->outputResolution->setCurrentText(res);
		}
	}
	ui->outputResolution->blockSignals(false);

	if (max_fps) {
		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				SetComboItemEnabled(ui->fpsCommon, i, false);
				continue;
			}
		}

		ui->fpsType->setCurrentIndex(0);
		ui->fpsCommon->setCurrentIndex(closest_fps_index);
		if (!valid) {
			ui->fpsType->setProperty("changed", true);
			ui->fpsCommon->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		for (int i = 0; i < ui->fpsCommon->count(); i++)
			SetComboItemEnabled(ui->fpsCommon, i, true);
	}

	SetComboItemEnabled(ui->fpsType, 1, !max_fps);
	SetComboItemEnabled(ui->fpsType, 2, !max_fps);

	/* ------------------------------------ */

	lastIgnoreRecommended = (int)ignoreRecommended;

	return true;
}

static bool service_supports_codec(const char **codecs, const char *codec)
{
	if (!codecs)
		return true;

	while (*codecs) {
		if (strcmp(*codecs, codec) == 0)
			return true;
		codecs++;
	}

	return false;
}

extern bool EncoderAvailable(const char *encoder);
extern const char *get_simple_output_encoder(const char *name);

static inline bool service_supports_encoder(const char **codecs,
					    const char *encoder)
{
	if (!EncoderAvailable(encoder))
		return false;

	const char *codec = obs_get_encoder_codec(encoder);
	return service_supports_codec(codecs, codec);
}

static bool return_first_id(void *data, const char *id)
{
	const char **output = (const char **)data;

	*output = id;
	return false;
}

bool OBSBasicSettings::ServiceAndVCodecCompatible()
{
	bool simple = (ui->outputMode->currentIndex() == 0);
	bool ret;

	const char *codec;

	if (simple) {
		QString encoder =
			ui->simpleOutStrEncoder->currentData().toString();
		const char *id = get_simple_output_encoder(QT_TO_UTF8(encoder));
		codec = obs_get_encoder_codec(id);
	} else {
		QString encoder = ui->advOutEncoder->currentData().toString();
		codec = obs_get_encoder_codec(QT_TO_UTF8(encoder));
	}

	OBSService service = SpawnTempService();
	const char **codecs = obs_service_get_supported_video_codecs(service);

	if (!codecs || IsCustomService()) {
		const char *output;
		char **output_codecs;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
						    &output, return_first_id);

		output_codecs = strlist_split(
			obs_get_output_supported_video_codecs(output), ';',
			false);

		ret = service_supports_codec((const char **)output_codecs,
					     codec);

		strlist_free(output_codecs);
	} else {
		ret = service_supports_codec(codecs, codec);
	}

	return ret;
}

bool OBSBasicSettings::ServiceAndACodecCompatible()
{
	bool simple = (ui->outputMode->currentIndex() == 0);
	bool ret;

	QString codec;

	if (simple) {
		codec = ui->simpleOutStrAEncoder->currentData().toString();
	} else {
		QString encoder = ui->advOutAEncoder->currentData().toString();
		codec = obs_get_encoder_codec(QT_TO_UTF8(encoder));
	}

	OBSService service = SpawnTempService();
	const char **codecs = obs_service_get_supported_audio_codecs(service);

	if (!codecs || IsCustomService()) {
		const char *output;
		char **output_codecs;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
						    &output, return_first_id);
		output_codecs = strlist_split(
			obs_get_output_supported_audio_codecs(output), ';',
			false);

		ret = service_supports_codec((const char **)output_codecs,
					     QT_TO_UTF8(codec));

		strlist_free(output_codecs);
	} else {
		ret = service_supports_codec(codecs, QT_TO_UTF8(codec));
	}

	return ret;
}

/* we really need a way to find fallbacks in a less hardcoded way. maybe. */
static QString get_adv_fallback(const QString &enc)
{
	if (enc == "jim_hevc_nvenc")
		return "jim_nvenc";
	if (enc == "h265_texture_amf")
		return "h264_texture_amf";
	if (enc == "com.apple.videotoolbox.videoencoder.ave.hevc")
		return "com.apple.videotoolbox.videoencoder.ave.avc";
	return "obs_x264";
}

static QString get_adv_audio_fallback(const QString &enc)
{
	const char *codec = obs_get_encoder_codec(QT_TO_UTF8(enc));

	if (strcmp(codec, "aac") == 0)
		return "ffmpeg_opus";

	QString aac_default = "ffmpeg_aac";
	if (EncoderAvailable("CoreAudio_AAC"))
		aac_default = "CoreAudio_AAC";
	else if (EncoderAvailable("libfdk_aac"))
		aac_default = "libfdk_aac";

	return aac_default;
}

static QString get_simple_fallback(const QString &enc)
{
	if (enc == SIMPLE_ENCODER_NVENC_HEVC)
		return SIMPLE_ENCODER_NVENC;
	if (enc == SIMPLE_ENCODER_NVENC_AV1)
		return SIMPLE_ENCODER_NVENC;
	if (enc == SIMPLE_ENCODER_AMD_HEVC)
		return SIMPLE_ENCODER_AMD;
	if (enc == SIMPLE_ENCODER_APPLE_HEVC)
		return SIMPLE_ENCODER_APPLE_H264;
	return SIMPLE_ENCODER_X264;
}

bool OBSBasicSettings::ServiceSupportsCodecCheck()
{
	if (loading)
		return false;

	bool vcodec_compat = ServiceAndVCodecCompatible();
	bool acodec_compat = ServiceAndACodecCompatible();

	if (vcodec_compat && acodec_compat) {
		if (lastServiceIdx != ui->service->currentIndex() ||
		    IsCustomService())
			ResetEncoders(true);
		return true;
	}

	QString service = ui->service->currentText();
	QString cur_video_name;
	QString fb_video_name;
	QString cur_audio_name;
	QString fb_audio_name;
	bool simple = (ui->outputMode->currentIndex() == 0);

	/* ------------------------------------------------- */
	/* get current codec                                 */

	if (simple) {
		QString cur_enc =
			ui->simpleOutStrEncoder->currentData().toString();
		QString fb_enc = get_simple_fallback(cur_enc);

		int cur_idx = ui->simpleOutStrEncoder->findData(cur_enc);
		int fb_idx = ui->simpleOutStrEncoder->findData(fb_enc);

		cur_video_name = ui->simpleOutStrEncoder->itemText(cur_idx);
		fb_video_name = ui->simpleOutStrEncoder->itemText(fb_idx);

		cur_enc = ui->simpleOutStrAEncoder->currentData().toString();
		fb_enc = (cur_enc == "opus") ? "aac" : "opus";

		cur_audio_name = ui->simpleOutStrAEncoder->itemText(
			ui->simpleOutStrAEncoder->findData(cur_enc));
		fb_audio_name =
			(cur_enc == "opus")
				? QTStr("Basic.Settings.Output.Simple.Codec.AAC")
				: QTStr("Basic.Settings.Output.Simple.Codec.Opus");
	} else {
		QString cur_enc = ui->advOutEncoder->currentData().toString();
		QString fb_enc = get_adv_fallback(cur_enc);

		cur_video_name =
			obs_encoder_get_display_name(QT_TO_UTF8(cur_enc));
		fb_video_name =
			obs_encoder_get_display_name(QT_TO_UTF8(fb_enc));

		cur_enc = ui->advOutAEncoder->currentData().toString();
		fb_enc = get_adv_audio_fallback(cur_enc);

		cur_audio_name =
			obs_encoder_get_display_name(QT_TO_UTF8(cur_enc));
		fb_audio_name =
			obs_encoder_get_display_name(QT_TO_UTF8(fb_enc));
	}

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.ServiceCodecCompatibility." x)

	QString msg = WARNING_VAL("Msg").arg(
		service, vcodec_compat ? cur_audio_name : cur_video_name,
		vcodec_compat ? fb_audio_name : fb_video_name);
	if (!vcodec_compat && !acodec_compat)
		msg = WARNING_VAL("Msg2").arg(service, cur_video_name,
					      cur_audio_name, fb_video_name,
					      fb_audio_name);

	auto button = OBSMessageBox::question(this, WARNING_VAL("Title"), msg);
#undef WARNING_VAL

	if (button == QMessageBox::No) {
		if (lastServiceIdx == 0 &&
		    lastServiceIdx == ui->service->currentIndex())
			QMetaObject::invokeMethod(ui->customServer, "setText",
						  Qt::QueuedConnection,
						  Q_ARG(QString,
							lastCustomServer));
		else
			QMetaObject::invokeMethod(ui->service,
						  "setCurrentIndex",
						  Qt::QueuedConnection,
						  Q_ARG(int, lastServiceIdx));
		return false;
	}

	ResetEncoders(true);
	return true;
}

#define TEXT_USE_STREAM_ENC \
	QTStr("Basic.Settings.Output.Adv.Recording.UseStreamEncoder")

void OBSBasicSettings::ResetEncoders(bool streamOnly)
{
	QString lastAdvVideoEnc = ui->advOutEncoder->currentData().toString();
	QString lastVideoEnc =
		ui->simpleOutStrEncoder->currentData().toString();
	QString lastAdvAudioEnc = ui->advOutAEncoder->currentData().toString();
	QString lastAudioEnc =
		ui->simpleOutStrAEncoder->currentData().toString();
	OBSService service = SpawnTempService();
	const char **vcodecs = obs_service_get_supported_video_codecs(service);
	const char **acodecs = obs_service_get_supported_audio_codecs(service);
	const char *type;
	BPtr<char *> output_vcodecs;
	BPtr<char *> output_acodecs;
	size_t idx = 0;

	if (!vcodecs || IsCustomService()) {
		const char *output;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
						    &output, return_first_id);
		output_vcodecs = strlist_split(
			obs_get_output_supported_video_codecs(output), ';',
			false);
		vcodecs = (const char **)output_vcodecs.Get();
	}

	if (!acodecs || IsCustomService()) {
		const char *output;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
						    &output, return_first_id);
		output_acodecs = strlist_split(
			obs_get_output_supported_audio_codecs(output), ';',
			false);
		acodecs = (const char **)output_acodecs.Get();
	}

	QSignalBlocker s1(ui->simpleOutStrEncoder);
	QSignalBlocker s2(ui->advOutEncoder);
	QSignalBlocker s3(ui->simpleOutStrAEncoder);
	QSignalBlocker s4(ui->advOutAEncoder);

	/* ------------------------------------------------- */
	/* clear encoder lists                               */

	ui->simpleOutStrEncoder->clear();
	ui->advOutEncoder->clear();
	ui->simpleOutStrAEncoder->clear();
	ui->advOutAEncoder->clear();

	if (!streamOnly) {
		ui->advOutRecEncoder->clear();
		ui->advOutRecEncoder->addItem(TEXT_USE_STREAM_ENC, "none");
		ui->advOutRecAEncoder->clear();
		ui->advOutRecAEncoder->addItem(TEXT_USE_STREAM_ENC, "none");
	}

	/* ------------------------------------------------- */
	/* load advanced stream/recording encoders           */

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (obs_get_encoder_type(type) == OBS_ENCODER_VIDEO) {
			if ((caps & ENCODER_HIDE_FLAGS) != 0)
				continue;

			if (service_supports_codec(vcodecs, codec))
				ui->advOutEncoder->addItem(qName, qType);
			if (!streamOnly)
				ui->advOutRecEncoder->addItem(qName, qType);
		}

		if (obs_get_encoder_type(type) == OBS_ENCODER_AUDIO) {
			if (service_supports_codec(acodecs, codec))
				ui->advOutAEncoder->addItem(qName, qType);
			if (!streamOnly)
				ui->advOutRecAEncoder->addItem(qName, qType);
		}
	}

	/* ------------------------------------------------- */
	/* load simple stream encoders                       */

#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ui->simpleOutStrEncoder->addItem(ENCODER_STR("Software"),
					 QString(SIMPLE_ENCODER_X264));
	if (service_supports_encoder(vcodecs, "obs_qsv11"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.QSV.H264"),
			QString(SIMPLE_ENCODER_QSV));
	if (service_supports_encoder(vcodecs, "obs_qsv11_av1"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.QSV.AV1"),
			QString(SIMPLE_ENCODER_QSV_AV1));
	if (service_supports_encoder(vcodecs, "ffmpeg_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.H264"),
			QString(SIMPLE_ENCODER_NVENC));
	if (service_supports_encoder(vcodecs, "jim_av1_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.AV1"),
			QString(SIMPLE_ENCODER_NVENC_AV1));
#ifdef ENABLE_HEVC
	if (service_supports_encoder(vcodecs, "h265_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.HEVC"),
			QString(SIMPLE_ENCODER_AMD_HEVC));
	if (service_supports_encoder(vcodecs, "ffmpeg_hevc_nvenc"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.NVENC.HEVC"),
			QString(SIMPLE_ENCODER_NVENC_HEVC));
#endif
	if (service_supports_encoder(vcodecs, "h264_texture_amf"))
		ui->simpleOutStrEncoder->addItem(
			ENCODER_STR("Hardware.AMD.H264"),
			QString(SIMPLE_ENCODER_AMD));
/* Preprocessor guard required for the macOS version check */
#ifdef __APPLE__
	if (service_supports_encoder(
		    vcodecs, "com.apple.videotoolbox.videoencoder.ave.avc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->simpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.H264"),
				QString(SIMPLE_ENCODER_APPLE_H264));
		}
	}
#ifdef ENABLE_HEVC
	if (service_supports_encoder(
		    vcodecs, "com.apple.videotoolbox.videoencoder.ave.hevc")
#ifndef __aarch64__
	    && os_get_emulation_status() == true
#endif
	) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->simpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.HEVC"),
				QString(SIMPLE_ENCODER_APPLE_HEVC));
		}
	}
#endif
#endif
	if (service_supports_encoder(acodecs, "CoreAudio_AAC") ||
	    service_supports_encoder(acodecs, "libfdk_aac") ||
	    service_supports_encoder(acodecs, "ffmpeg_aac"))
		ui->simpleOutStrAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"),
			"aac");
	if (service_supports_encoder(acodecs, "ffmpeg_opus"))
		ui->simpleOutStrAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.Opus"),
			"opus");
#undef ENCODER_STR

	/* ------------------------------------------------- */
	/* Find fallback encoders                            */

	if (!lastAdvVideoEnc.isEmpty()) {
		int idx = ui->advOutEncoder->findData(lastAdvVideoEnc);
		if (idx == -1) {
			lastAdvVideoEnc = get_adv_fallback(lastAdvVideoEnc);
			ui->advOutEncoder->setProperty("changed",
						       QVariant(true));
			OutputsChanged();
		}

		idx = ui->advOutEncoder->findData(lastAdvVideoEnc);
		s2.unblock();
		ui->advOutEncoder->setCurrentIndex(idx);
	}

	if (!lastAdvAudioEnc.isEmpty()) {
		int idx = ui->advOutAEncoder->findData(lastAdvAudioEnc);
		if (idx == -1) {
			lastAdvAudioEnc =
				get_adv_audio_fallback(lastAdvAudioEnc);
			ui->advOutAEncoder->setProperty("changed",
							QVariant(true));
			OutputsChanged();
		}

		idx = ui->advOutAEncoder->findData(lastAdvAudioEnc);
		s4.unblock();
		ui->advOutAEncoder->setCurrentIndex(idx);
	}

	if (!lastVideoEnc.isEmpty()) {
		int idx = ui->simpleOutStrEncoder->findData(lastVideoEnc);
		if (idx == -1) {
			lastVideoEnc = get_simple_fallback(lastVideoEnc);
			ui->simpleOutStrEncoder->setProperty("changed",
							     QVariant(true));
			OutputsChanged();
		}

		idx = ui->simpleOutStrEncoder->findData(lastVideoEnc);
		s1.unblock();
		ui->simpleOutStrEncoder->setCurrentIndex(idx);
	}

	if (!lastAudioEnc.isEmpty()) {
		int idx = ui->simpleOutStrAEncoder->findData(lastAudioEnc);
		if (idx == -1) {
			lastAudioEnc = (lastAudioEnc == "opus") ? "aac"
								: "opus";
			ui->simpleOutStrAEncoder->setProperty("changed",
							      QVariant(true));
			OutputsChanged();
		}

		idx = ui->simpleOutStrAEncoder->findData(lastAudioEnc);
		s3.unblock();
		ui->simpleOutStrAEncoder->setCurrentIndex(idx);
	}
}
